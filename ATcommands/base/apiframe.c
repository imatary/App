/*
 * apiframe.c
 *
 * Created: 25.11.2016 14:37:35
 *  Author: TOE
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <avr/interrupt.h>

#include "../header/cmd.h"
#include "../header/rfmodul.h"
#include "../header/atlocal.h"
#include "../header/apiframe.h"
#include "../header/circularBuffer.h"			// buffer
#include "../../ATuracoli/stackrelated.h"		// UART_print(f)
#include "../../ATuracoli/stackdefines.h"

// === c-File structs =====================================
struct api_f 
{
	ATERROR  ret;
	uint8_t	 delimiter;
	uint16_t length;
	uint8_t  type;
	uint8_t  id;
	/*
	 * create the frame & calc checksum
	 * 0xFF - (API type + frame ID [+ target address] [+ options] + main content [+ parameter]) = checksum
	 *        |<---------------------------------- frame frame->bufLength ----------------------------->|
	 */
	uint8_t  crc;
	int      bufLen;
};

// === Prototypes =========================================
static ATERROR API_0x08_atLocal(struct api_f *frame, uint8_t *array);
static ATERROR API_0x09_atLocal_queue(struct api_f *frame, uint8_t *array);
static bool_t  API_compareCRC(uint8_t *frameCRC, uint8_t *userCRC);

// === Functions ==========================================

/*
 *
 *
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/11/28
 */
ATERROR API_frameHandle_uart(int *len)
{
	uint8_t  outchar[5]	= {0x0};
	struct api_f frame  = {0,0,0,0,0,0xFF,*len};
	
	// Start delimiter	1 byte
	UART_print("Start delimiter\r");	
	cli(); BufferOut( &UART_deBuf, &frame.delimiter ); sei();
	if ( frame.delimiter != STD_DELIMITER ) return API_NOT_AVAILABLE;
	UART_printf("%"PRIX8"\r\r", STD_DELIMITER );
	
	// frame->bufLength	2 byte
	UART_print("Length\r");
	cli(); BufferOut( &UART_deBuf, &outchar[0] ); sei();
	cli(); BufferOut( &UART_deBuf, &outchar[1] ); sei();
	frame.length = (uint16_t) outchar[0] << 2 | outchar[1] & 0xFF ;
	UART_printf("%"PRIX8" %"PRIX8" (%u)\r\r", (uint8_t) frame.length >> 2, frame.length & 0xFF, frame.length );
	
	// frame type	1 byte
	UART_print("Frame type\r"); 
	cli(); BufferOut( &UART_deBuf, &frame.type ); sei();
	switch ( frame.type )
	{
		case AT_COMMAND    : 
			UART_print("08 (AT Command)\r\r");
			frame.crc -= 0x08;
			frame.ret = API_0x08_atLocal(&frame, outchar);
		break;
		
		case AT_COMMAND_Q  : 
			UART_print("09 (AT Command Queue)\r\r");
			frame.crc -= 0x09;
			frame.ret = API_0x09_atLocal_queue(&frame, outchar);
		break;
		
		case REMOTE_AT_CMD : 
			UART_print("17 (AT Remote Command)\r\r");
			frame.crc -= 0x17;
			frame.ret = TRX_send(); 
		break;
		
		default : break;
	}	
	return frame.ret;
}

/*
 *
 *
 * Received:
 *		the pointer to the frame struct
 *
 * Returns:
 *		OP_SUCCESS
 *		INVALID_COMMAND
 *		INVALID_PARAMETER
 *
 * last modified: 2016/11/28
 */
static ATERROR API_0x08_atLocal(struct api_f *frame, uint8_t *array)
{
	// frame id		1 byte
	UART_print("Frame ID\r");
	cli(); BufferOut( &UART_deBuf, &frame->id ); sei();
	UART_printf("%"PRIX8"\r\r", frame->id );
	frame->crc -= frame->id;
	
	// AT command 2 bytes
	*(array+0) = 'A';
	*(array+1) = 'T';
	cli(); BufferOut( &UART_deBuf, array+2 ); sei();
	cli(); BufferOut( &UART_deBuf, array+3 ); sei();
	
	UART_print("AT Command");
	UART_printf("%"PRIX8" %"PRIX8" (%c%c)\r\r", *(array+2), *(array+3), *(array+2), *(array+3));
	frame->crc -= (*(array+2) + *(array+3));
	
	// search for CMD in table
	CMD *workPointer = (CMD*) pStdCmdTable;
	for (int i = 0; i < command_count ; i++, workPointer++)
	{
		if( strncmp( (const char*) array, workPointer->name, 4 ) == 0 ) break;
	}
	
	/*
	 * handle CMD
	 * frame length
	 * exec is allowed
	 */
	if ( frame->length == 4 &&\
		 workPointer->rwxAttrib & EXEC )
	{
		uint8_t userCRC;
		if ( API_compareCRC(&frame->crc, &userCRC) == FALSE )
		{
			UART_printf("Calculated CRC: %"PRIX8" vs read CRC:  %"PRIX8"\r\r",  frame->crc, userCRC );
			return ERROR;
		}
		
		switch( workPointer->rwxAttrib )
		{
			// leave command mode command
			case AT_CN : return INVALID_COMMAND;
			
			// write config to firmware
			case AT_WR : {
				SET_userValInEEPROM();

			}
			break;
			
			// apply changes - currently only a dummy
			case AT_AC : {
				/*
				 * TODO - run config for trx/uart
				 */
			}
			break;
			
			// reset all parameter
			case AT_RE : {
				SET_allDefault();
			}
			break;
			
			default: break;
		}
	}
	/*
	 * frame length
	 * string length of input
	 * reading the command is allowed
	 */
	else if ( frame->length == 4 &&\
			  workPointer->rwxAttrib & READ )
	{
		uint8_t userCRC;
		if ( API_compareCRC( &frame->crc, &userCRC ) == FALSE )
		{
			UART_printf("Calculated CRC: %"PRIX8" vs read CRC:  %"PRIX8"\r\r", frame->crc, userCRC );
			return ERROR;
		}
		
		switch( workPointer->ID )
		{
			case AT_CH : {
				UART_printf("%"PRIX8"\r", RFmodul.netCMD_ch); 
				} break;
			case AT_ID : UART_printf("%"PRIX16"\r",RFmodul.netCMD_id); break;
			case AT_DH : UART_printf("%"PRIX32"\r",RFmodul.netCMD_dh); break;
			case AT_DL : UART_printf("%"PRIX32"\r",RFmodul.netCMD_dl); break;
			case AT_MY : UART_printf("%"PRIX16"\r",RFmodul.netCMD_my); break;
			case AT_SH : UART_printf("%"PRIX32"\r",RFmodul.netCMD_sh); break;
			case AT_SL : UART_printf("%"PRIX32"\r",RFmodul.netCMD_sl); break;
			case AT_CE : UART_printf("%d\r",       RFmodul.netCMD_ce); break;
			case AT_SC : UART_printf("%"PRIX16"\r",RFmodul.netCMD_sc); break;
			case AT_NI : UART_printf("%s\r",       RFmodul.netCMD_ni); break;
			case AT_MM : UART_printf("%"PRIX8"\r", RFmodul.netCMD_mm); break;
			case AT_RR : UART_printf("%"PRIX8"\r", RFmodul.netCMD_rr); break;
			case AT_RN : UART_printf("%"PRIX8"\r", RFmodul.netCMD_rn); break;
			case AT_NT : UART_printf("%"PRIX8"\r", RFmodul.netCMD_nt); break;
			case AT_NO : UART_printf("%d\r",       RFmodul.netCMD_no); break;
			case AT_SD : UART_printf("%"PRIX8"\r", RFmodul.netCMD_sd); break;
			case AT_A1 : UART_printf("%"PRIX8"\r", RFmodul.netCMD_a1); break;
			case AT_A2 : UART_printf("%"PRIX8"\r", RFmodul.netCMD_a2); break;
			case AT_AI : UART_printf("%"PRIX8"\r", RFmodul.netCMD_ai); break;

			case AT_EE : UART_printf("%d\r",       RFmodul.secCMD_ee); break;
			case AT_KY : UART_print("\r"); break;

			case AT_PL : UART_printf("%"PRIX8"\r", RFmodul.rfiCMD_pl); break;
			case AT_CA : UART_printf("%"PRIX8"\r", RFmodul.rfiCMD_ca); break;

			case AT_SM : UART_printf("%"PRIX8"\r", RFmodul.sleepmCMD_sm); break;
			case AT_ST : UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_st); break;
			case AT_SP : UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_sp); break;
			case AT_DP : UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_dp); break;
			case AT_SO : UART_printf("%"PRIX8"\r", RFmodul.sleepmCMD_so); break;
			case AT_SS : UART_print("ERROR\r"); break;

			case AT_AP : UART_printf("%"PRIX8"\r",RFmodul.serintCMD_ap); break;
			case AT_BD : UART_printf("%"PRIX8"\r",RFmodul.serintCMD_bd); break;
			case AT_NB : UART_printf("%"PRIX8"\r",RFmodul.serintCMD_nb); break;
			case AT_RO : UART_printf("%"PRIX8"\r",RFmodul.serintCMD_ro); break;

			case AT_D8 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d8); break;
			case AT_D7 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d7); break;
			case AT_D6 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d6); break;
			case AT_D5 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d5); break;
			case AT_D4 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d4); break;
			case AT_D3 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d3); break;
			case AT_D2 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d2); break;
			case AT_D1 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d1); break;
			case AT_D0 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d0); break;
			case AT_PR : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_pr); break;
			case AT_IU : UART_printf("%d\r",       RFmodul.ioserCMD_iu); break;
			case AT_IT : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_it); break;
			case AT_IC : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_ic); break;
			case AT_IR : UART_printf("%"PRIX16"\r",RFmodul.ioserCMD_ir); break;
			case AT_P0 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_p0); break;
			case AT_P1 : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_p1); break;
			case AT_PT : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_pt); break;
			case AT_RP : UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_rp); break;

			case AT_IA : {	// the compiler don't like the PRIX64 command
				uint32_t a = RFmodul.iolpCMD_ia >> 32;
				uint32_t b = RFmodul.iolpCMD_ia & 0xFFFFFFFF;
				UART_printf("%"PRIX32"%"PRIX32"\r",a,b);
			}
			break;
			case AT_T0 : UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T0); break;
			case AT_T1 : UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T1); break;
			case AT_T2 : UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T2); break;
			case AT_T3 : UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T3); break;
			case AT_T4 : UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T4); break;
			case AT_T5 : UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T5); break;
			case AT_T6 : UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T6); break;
			case AT_T7 : UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T7); break;

			case AT_VR : UART_printf("%"PRIX16"\r",RFmodul.diagCMD_vr); break;
			case AT_HV : UART_printf("%"PRIX16"\r",RFmodul.diagCMD_hv); break;
			case AT_DB : UART_printf("%"PRIX8"\r", RFmodul.diagCMD_db); break;
			case AT_EC : UART_printf("%"PRIX16"\r",RFmodul.diagCMD_ec); break;
			case AT_EA : UART_printf("%"PRIX16"\r",RFmodul.diagCMD_ea); break;
			case AT_DD : UART_printf("%"PRIX32"\r",RFmodul.diagCMD_dd); break;

			case AT_CT : UART_printf("%"PRIX16"\r",RFmodul.atcopCMD_ct); break;
			case AT_GT : UART_printf("%"PRIX16"\r",RFmodul.atcopCMD_gt); break;
			case AT_CC : UART_printf("%"PRIX8"\r", RFmodul.atcopCMD_cc); break;
			
			case AT_Rq : UART_print("ERROR\r"); break;
			case AT_pC : UART_print("1\r"); break;
			
			case AT_SB : UART_print("ERROR\r"); break;
			
			default : break;
		}
	} 
	/* 
	 * if writing is allowed store the value of the string into RFmodel struct and register 
	 * frame length
	 * string length of input
	 * writing to RFmodul struct [and to EEPROM] is allowed
	 */
	else if ( frame->length > 4 &&\
			  workPointer->rwxAttrib & WRITE )
	{
		size_t cmdSize = (((frame->bufLen-15) % 2) + (frame->bufLen-15))/2;
	    /* 
	     * special handle if
	     * - network identifier string command
	     * - buffer content <= 20 characters
	     */
	    if ( AT_NI == workPointer->ID && frame->bufLen-15 <= 20 )
	    {
	    	for (int i = 0; i < frame->bufLen-15; i++)
	    	{
	    		cli(); BufferOut(&UART_deBuf, &RFmodul.netCMD_ni[i]); sei();
	    	}
	    	
	    	RFmodul.netCMD_ni[frame->bufLen-14] = 0x0;
	    	if( frame->type == 0x8 ) SET_userValInEEPROM();
	    }
	    if (AT_NI == workPointer->ID && frame->bufLen-15 > 20 )
	    {
	    	return INVALID_PARAMETER;
	    }
	    
	    /*
	     * copy value for comparison in tmp buffer
	     * if - the command size smaller or equal then the unit of the tmp buffer (related to the uint8 buffer)
	     *    - the buffer value greater or equal than the min value
	     *    - the buffer value smaller or equal than the max value
	     * save the value into the memory
	     * else it is a invalid parameter
	     */	
	    switch(workPointer->ID)
	    {
	    	/*
	    	 * AT commands: network
	    	 *
	    	 * last modified: 2016/11/24
	    	 */
	    	case AT_CH : { 
	    					uint8_t tmp = 0;								
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0B && tmp <= 0x1A )
	    					{
	    						TRX_writeBit(deSR_CHANNEL, tmp);
	    						RFmodul.netCMD_ch = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}						
	    				break;
	    	
	    	case AT_ID : {
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					
	    					if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
	    					{
	    						RFmodul.netCMD_id = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_DH : {
	    					uint8_t cmdString[4] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 4 ) == FALSE ) return ERROR;
	    					uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
	    					
	    					if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
	    					{
	    						RFmodul.netCMD_dh = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_DL : {
	    					uint8_t cmdString[4] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 4 ) == FALSE ) return ERROR;
	    					uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
	    					
	    					if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
	    					{
	    						RFmodul.netCMD_dl = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_MY : {
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					
	    					if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
	    					{
	    						RFmodul.netCMD_my = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_CE : { 	
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    									
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
	    					{
	    						RFmodul.netCMD_ce = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_SC : { 
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					 
	    					if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
	    					{
	    						RFmodul.netCMD_sc = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_MM : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x00 && tmp <= 0x3 )
	    					{
	    						RFmodul.netCMD_mm = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_RR : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
	    					{
	    						RFmodul.netCMD_rr = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_RN : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x00 && tmp <= 0x3 )
	    					{
	    						RFmodul.netCMD_rn = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_NT : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x01 && tmp <= 0xFC )
	    					{
	    						RFmodul.netCMD_nt = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_NO : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
	    					{
	    						RFmodul.netCMD_no = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_SD : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
	    					{
	    						RFmodul.netCMD_sd = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_A1 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
	    					{
	    						RFmodul.netCMD_a1 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_A2 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
	    					{
	    						RFmodul.netCMD_a2 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	/*
	    	 * AT commands: security
	    	 *
	    	 * last modified: 2016/11/24
	    	 */			
	    	case AT_KY : UART_print("Not implemented.\r"); break;
	    
	    	case AT_EE : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
	    					{
	    						RFmodul.secCMD_ee = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	/*
	    	 * AT commands: RF interface
	    	 *
	    	 * last modified: 2016/11/24
	    	 */
	    	case AT_PL : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x4 )
	    					{
	    						RFmodul.rfiCMD_pl = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_CA : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x24 && tmp <= 0x50 )
	    					{
	    						RFmodul.rfiCMD_ca = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	/*
	    	 * AT commands: sleep modes
	    	 *
	    	 * last modified: 2016/11/24
	    	 */
	    	case AT_SM : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
	    					{
	    						RFmodul.sleepmCMD_sm = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_ST : { 
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					
	    					if ( cmdSize <= 2 && tmp >= 0x1 && tmp <= 0xFFFF )
	    					{
	    						RFmodul.sleepmCMD_st = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_SP : {
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					
	    					if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0x68B0 )
	    					{
	    						RFmodul.sleepmCMD_sp = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_DP : { 
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					
	    					if ( cmdSize <= 2 && tmp >= 0x1 && tmp <= 0x68B0 )
	    					{
	    						RFmodul.sleepmCMD_dp = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_SO : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
	    					{
	    						RFmodul.sleepmCMD_so = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	/*
	    	 * AT commands: serial interfacing
	    	 *
	    	 * last modified: 2016/11/24
	    	 */
	    	case AT_AP : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
	    					{
	    						RFmodul.serintCMD_ap = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_BD : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x7 )
	    					{
	    						RFmodul.serintCMD_bd = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_NB : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x4 )
	    					{
	    						RFmodul.serintCMD_nb = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_RO : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.serintCMD_ro = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	/*
	    	 * AT commands: IO settings
	    	 *
	    	 * last modified: 2016/11/24
	    	 */
	    	case AT_D8 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d8 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_D7 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d7 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_D6 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d6 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_D5 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d5 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_D4 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d4 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_D3 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d3 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_D2 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d2 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_D1 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d1 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_D0 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
	    					{
	    						RFmodul.ioserCMD_d0 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_PR : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.ioserCMD_pr = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_IU : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
	    					{
	    						RFmodul.ioserCMD_iu = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_IT : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x1 && tmp <= 0xFF )
	    					{
	    						RFmodul.ioserCMD_it = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_IC : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.ioserCMD_ic = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_IR : {
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					
	    					if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
	    					{
	    						RFmodul.ioserCMD_ir = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_P0 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
	    					{
	    						RFmodul.ioserCMD_p0 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_P1 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
	    					{
	    						RFmodul.ioserCMD_p1 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_PT : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0xB && tmp <= 0xFF )
	    					{
	    						RFmodul.ioserCMD_pt = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_RP : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.ioserCMD_rp = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	/*
	    	 * AT commands: IO line passing
	    	 *
	    	 * last modified: 2016/11/24
	    	 */
	    	case AT_IA : {
	    					uint8_t cmdString[8] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 8 ) == FALSE ) return ERROR;
	    					uint64_t tmp = (uint64_t) cmdString[0] << 56 | (uint64_t) cmdString[1] << 48 | (uint64_t) cmdString[2] <<  40 | (uint64_t) cmdString[3] << 32 |\
	    								   (uint64_t) cmdString[4] << 24 | (uint64_t) cmdString[5] << 16 | (uint64_t) cmdString[6] <<   8 | (uint64_t) cmdString[7];
	    					
	    					if ( cmdSize <= 8 && tmp >= 0x0 && tmp <= 0xFFFFFFFFFFFFFFFF )
	    					{
	    						RFmodul.iolpCMD_ia = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_T0 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.iolpCMD_T0 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_T1 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.iolpCMD_T1 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_T2 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.iolpCMD_T2 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    
	    	case AT_T3 : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.iolpCMD_T3 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_T4 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.iolpCMD_T4 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_T5 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.iolpCMD_T5 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_T6 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.iolpCMD_T6 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_T7 : {
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    												
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.iolpCMD_T7 = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	/*
	    	 * AT commands: diagnostics
	    	 *
	    	 * last modified: 2016/11/24
	    	 */
	    	case AT_DD : { 
	    					uint8_t cmdString[4] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 4 ) == FALSE ) return ERROR;
	    					uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
	    					
	    					if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
	    					{
	    						RFmodul.diagCMD_dd = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	/*
	    	 * AT commands: AT command options
	    	 *
	    	 * last modified: 2016/11/24
	    	 */
	    	case AT_CT : { 
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					
	    					if ( cmdSize <= 2 && tmp >= 0x02 && tmp <= 0x1770 )
	    					{
	    						RFmodul.atcopCMD_ct = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_GT : { 
	    					uint8_t cmdString[2] = {0x0};
	    					if ( charToUint8( &cmdString[0], &frame->bufLen, &cmdSize, 2 ) == FALSE ) return ERROR;
	    					uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
	    					
	    					if ( cmdSize <= 2 && tmp >= 0x02 && tmp <= 0xCE4 )
	    					{
	    						RFmodul.atcopCMD_gt = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	case AT_CC : { 
	    					uint8_t tmp = 0;
	    					if ( charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
	    					
	    					if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
	    					{
	    						RFmodul.atcopCMD_cc = tmp;
	    						if( frame->type == 0x8 ) SET_userValInEEPROM();
	    						
	    					}
	    					else { return INVALID_PARAMETER; }
	    				}
	    				break;
	    	
	    	default : return INVALID_COMMAND;		
	    }
	}
	else
	{
		return INVALID_COMMAND;
	}
	
	

}

/*
 *
 *
 *
 * Returns:
 *		OP_SUCCESS
 *		INVALID_COMMAND
 *		INVALID_PARAMETER 
 *
 * last modified: 2016/11/28
 */
static ATERROR API_0x09_atLocal_queue(struct api_f *frame)
{
	/*
	UART_print("Frame ID\r");
	UART_printf("%"PRIX8"\r\r", 0 );					// frame id		1 byte
	
	UART_print("AT Command")
	UART_printf("%"PRIX8" %"PRIX8"\r\r", 0, 0 );		// AT CMD		2 byte
	*/
	return ERROR;
}

/*
 * The API compare function red the user crc value from the buffer
 * and compared it with the calculated crc sum
 *
 * Received:
 *		the calculated crc sum
 *
 * Returned:
 *		TRUE	if calculated crc equal to user crc
 *		FALSE	if calculated crc is not equal to user crc
 *
 * last modified: 2016/11/29
 */
static bool_t API_compareCRC(uint8_t *frameCRC, uint8_t *userCRC)
{
	BufferOut( &UART_deBuf, userCRC);
	if ( *frameCRC == *userCRC )
	{
		return TRUE;
	} 
	else
	{
		return FALSE;
	}
	
}
