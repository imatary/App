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
	ATERROR  ret;			// 1 Byte
	uint8_t	 delimiter;		// 1 Byte
	uint16_t length;		// 2 Byte
	uint8_t  type;			// 1 Byte
	uint8_t  cmd[3];		// 3 Byte
	uint8_t  id;			// 1 Byte
	uint8_t  msg[256];		// 256 Byte
	/*
	 * create the frame & calc checksum
	 * 0xFF - (API type + frame ID [+ target address] [+ options] + main content [+ parameter]) = checksum
	 *        |<---------------------------------- frame frame->bufLength ------------------->|
	 */
	uint8_t  crc;			// 1 Byte
	short    bufLen;		// 2 byte
};

// === Prototypes =========================================
static ATERROR API_0x08_atLocal(struct api_f *frame, uint8_t *array);
static void    API_0x88_atLocal_response(struct api_f *frame, uint64_t *val, short length);
static ATERROR API_readAndExec(struct api_f *frame);
static ATERROR API_write(struct api_f *frame);
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
ATERROR API_frameHandle_uart(short *len)
{
	uint8_t  outchar[5]	= {0x0};
	struct api_f frame  = {0,0,0,0,{0},0,{0},0xFF,*len};
	
	// Start delimiter	1 byte	
	cli(); BufferOut( &UART_deBuf, &frame.delimiter ); sei();
	if ( frame.delimiter != STD_DELIMITER ) return API_NOT_AVAILABLE;
	if (RFmodul.deCMD_ru) UART_printf("Start delimiter\r%02"PRIX8"\r\r", STD_DELIMITER );
	
	// frame->bufLength	2 byte
	cli(); BufferOut( &UART_deBuf, &outchar[0] ); sei();
	cli(); BufferOut( &UART_deBuf, &outchar[1] ); sei();
	frame.length = (uint16_t) outchar[0] << 2 | outchar[1] ;
	if (RFmodul.deCMD_ru) UART_printf("Length\r%02"PRIX8" %02"PRIX8" (%u)\r\r", outchar[0], outchar[1], frame.length );
	
	// frame type	1 byte
	cli(); BufferOut( &UART_deBuf, &frame.type ); sei();
	switch ( frame.type )
	{
		case AT_COMMAND    : 
			if (RFmodul.deCMD_ru) UART_print("Frame type\r08 (AT Command)\r\r");
			frame.crc -= 0x08;
			if ( frame->length == 4)
			{
				frame.ret = CMD_readOrExec(&frame, outchar, NULL);
			}
			else
			{
				frame.ret = CMD_write(&frame, outchar);
			}
		break;
		
		case AT_COMMAND_Q  : 
			if (RFmodul.deCMD_ru) UART_print("Frame type\r09 (AT Command Queue)\r\r");
			frame.crc -= 0x09;
			frame.ret = API_0x08_atLocal(&frame, outchar);
		break;
		
		case REMOTE_AT_CMD : 
			if (RFmodul.deCMD_ru) UART_print("Frame type\r17 (AT Remote Command)\r\r");
			frame.crc -= 0x17;
			frame.ret = TRX_send(); 
		break;
		
		default : UART_print("Not a valid command type!\r\r"); return INVALID_COMMAND;
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
	CMD *pCommand  = NULL;
	
	if ( RFmodul.serintCMD_ap != 0 && frame != NULL ) // API frame
	{
		pCommand = API_findInTable(frame, array);
		if ( INVALID_COMMAND == pCommand ) return INVALID_COMMAND;
	} 
	else // AT CMD
	{
		pCommand = CMD_findInTable();
		if ( INVALID_COMMAND == pCommand )
		{
			UART_print("Command not in table!\r");
			return;
		}
	}
	
	
	/*
	 * handle CMD
	 * if API frame, check frame length
	 * exec is allowed
	 */
	if ( frame != NULL ) 
	{
		if ( API_compareCRC(frame) == FALSE )
		{
			if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
			return ERROR;
		}
	}
	
	if ( pCommand->rwxAttrib & EXEC )
	{		
		switch( pCommand->ID )
		{
			// leave command mode command
/* CN */    case AT_CN : 
				if ( RFmodul.serintCMD_ap == 0 )
				{
					if ( th != NULL) *th = deTIMER_stop(*th);					
					noTimeout = FALSE;
					UART_print("OK\r");
				} 
				else
				{
					return INVALID_COMMAND;
				}
				break;
			
			// write config to firmware
/* WR */    case AT_WR : 
				SET_userValInEEPROM();
				if ( RFmodul.serintCMD_ap == 0 ) 
				{
					UART_print("OK\r");
				}
				else
				{
					return OP_SUCCESS;
				}
				break;
			
			// apply changes - currently only a dummy
/* AC */    case AT_AC : {
				/*
				 * TODO - run config for trx/uart
				 */
			}
			break;
			
			// reset all parameter
/* RE */    case AT_RE : 
				SET_allDefault();
				if ( RFmodul.serintCMD_ap == 0 ) 
				{
					UART_print("OK\r");
				}
				else
				{
					return OP_SUCCESS;
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
	else if ( pCommand->rwxAttrib & READ )
	{
		switch( pCommand->ID )
		{
/* CH */	case AT_CH : 
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_ch); 
				}
				else if ( frame != NULL )
				{
					frame->msg = RFmodul.netCMD_ch;
				}
				break;
			
/* ID */	case AT_ID :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_id);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.netCMD_id >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.netCMD_id & 0xFF);
				}  
				break;
			
/* DH */	case AT_DH :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_dh);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, char down = 24; down >= 0; down -= 8, up++)
					{
						frame->msg[up] = (uint8_t *) (RFmodul.netCMD_dh >> down)
					}
				}  
				break;
			
/* DL */	case AT_DL :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX32"\r", RFmodul.netCMD_dl);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, char down = 24; down >= 0; down -= 8, up++)
					{
						frame->msg[up] = (uint8_t *) (RFmodul.netCMD_dl >> down)
					}
				}  
				break;
			
/* MY */	case AT_MY :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_my);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.netCMD_my >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.netCMD_my & 0xFF);
				}  
				break;
			
/* SH */	case AT_SH :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_sh);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, char down = 24; down >= 0; down -= 8, up++)
					{
						frame->msg[up] = (uint8_t *) (RFmodul.netCMD_sh >> down)
					}
				}  
				break;
			
/* SL */	case AT_SL :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_sl);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, char down = 24; down >= 0; down -= 8, up++)
					{
						frame->msg[up] = (uint8_t *) (RFmodul.netCMD_dl >> down)
					}
				}  
				break;
			
/* CE */	case AT_CE :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%d\r",       RFmodul.netCMD_ce);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_ce;
				}  
				break;
			
/* SC */	case AT_SC :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_sc);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.netCMD_sc >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.netCMD_sc & 0xFF);
				}  
				break;
			
/* NI */	case AT_NI :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%s\r", RFmodul.netCMD_ni); 
				}
				else if ( frame != NULL )
				{
					memcpy(frame->msg, &RFmodul.netCMD_ni, strlen(&RFmodul.netCMD_ni)+1 );
				}
				break;
			
/* MM */	case AT_MM :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_mm);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_mm;
				}  
				break;
			
/* RR */	case AT_RR :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_rr); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_rr;
				} 
				break;
			
/* RN */	case AT_RN :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_rn);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_rn;
				}  
				break;
			
/* NT */	case AT_NT :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_nt);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_nt;
				} 
				break;
			
/* NO */	case AT_NO :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%d\r", RFmodul.netCMD_no);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_no;
				} ; 
				break;
			
/* SD */	case AT_SD :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_sd);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_sd;
				}  
				break;
			
/* A1 */	case AT_A1 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_a1);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_a1;
				}  
				break;
			
/* A2 */	case AT_A2 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_a2);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_a2;
				}  
				break;
			
/* AI */	case AT_AI :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_ai);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.netCMD_ai;
				}  
				break;

/* EE */	case AT_EE :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%d\r",       RFmodul.secCMD_ee); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.secCMD_ee;
				} 
				break;
			
/* KY */	case AT_KY :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_print("\r");
				}
				break;

/* PL */	case AT_PL :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.rfiCMD_pl);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.rfiCMD_pl;
				}  
				break;
			
/* CA */	case AT_CA :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.rfiCMD_ca);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.rfiCMD_ca;
				}  
				break;

/* SM */	case AT_SM :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.sleepmCMD_sm);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.sleepmCMD_sm;
				}  
				break;
			
/* CH */	case AT_ST :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_st);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.sleepmCMD_st >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.sleepmCMD_st & 0xFF);
				}  
				break;
			
/* SP */	case AT_SP :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_sp); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.sleepmCMD_sp >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.sleepmCMD_sp & 0xFF);
				} 
				break;
			
/* DP */	case AT_DP :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r", RFmodul.sleepmCMD_dp);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.sleepmCMD_dp >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.sleepmCMD_dp & 0xFF);
				}  
				break;
			
/* SO */	case AT_SO :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.sleepmCMD_so);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.sleepmCMD_so;
				}  
				break;
			
/* SS */	case AT_SS : 
				if ( RFmodul.serintCMD_ap == 0 )
				{
					 UART_print("ERROR\r");
				} 
				else if ( frame != NULL )
				{
					return ERROR;
				}
				break;
				
/* AP */	case AT_AP : 
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.serintCMD_ap);
				} 
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.serintCMD_ap;
				}
				break;
			
/* BD */    case AT_BD :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.serintCMD_bd);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.serintCMD_bd;
				} 
				break;
			
/* NB */    case AT_NB :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.serintCMD_nb);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.serintCMD_nb;
				}  
				break;
				
/* RO */	case AT_RO :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					 UART_printf("%"PRIX8"\r",RFmodul.serintCMD_ro);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.serintCMD_ro;
				} 
				break;

/* D8 */	case AT_D8 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d8); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d8;
				} 
				break;
			
/* D7 */	case AT_D7 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d7); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d7;
				} 
				break;
			
/* D6 */	case AT_D6 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d6); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d6;
				} 
				break;
			
/* D5 */	case AT_D5 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d5); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d5;
				} 
				break;
			
/* D4 */	case AT_D4 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d4); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d4;
				} 
				break;
			
/* D3 */	case AT_D3 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d3); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d3;
				} 
				break;
			
/* D2 */	case AT_D2 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d2);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d2;
				} 
				break;
			
/* D1 */	case AT_D1 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d1); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d1;
				} 
				break;
			
/* D0 */	case AT_D0 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d0); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_d0;
				} 
				break;
			
/* PR */	case AT_PR :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_pr);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_pr;
				}
				break;
			
/* IU */	case AT_IU :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%d\r", RFmodul.ioserCMD_iu);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_iu;
				} 
				break;
			
/* IT */	case AT_IT :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_it);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_it;
				} 
				break;
			
/* IC */	case AT_IC :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_ic);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_ic;
				} 
				break;
			
/* IR */	case AT_IR :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r", RFmodul.ioserCMD_ir); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.ioserCMD_ir >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.ioserCMD_ir & 0xFF);
				} 
				break;
			
/* P0 */	case AT_P0 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_p0);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_p0;
				} 
				break;
			
/* P1 */	case AT_P1 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_p1); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_p1;
				} 
				break;
			
/* PT */	case AT_PT :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_pt); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_pt;
				} 
				break;
			
/* RP */	case AT_RP :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_rp); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.ioserCMD_rp;
				} 
				break;

/* IA */	case AT_IA :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					// the compiler don't like the PRIX64 command
					uint32_t a = RFmodul.iolpCMD_ia >> 32;
					uint32_t b = RFmodul.iolpCMD_ia & 0xFFFFFFFF;
					UART_printf("%"PRIX32"%"PRIX32"\r",a,b);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, char down = 56; down >= 0; down -= 8, up++)
					{
						frame->msg[up] = (uint8_t *) (RFmodul.iolpCMD_ia >> down)
					}
				}
				break;
			
/* T0 */	case AT_T0 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T0); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.iolpCMD_T0;
				} 
				break;
			
/* T1 */	case AT_T1 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T1); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.iolpCMD_T1;
				}
				break;
			
/* T2 */	case AT_T2 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T2); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.iolpCMD_T2;
				}
				break;
			
/* T3 */	case AT_T3 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T3); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.iolpCMD_T3;
				}
				break;
			
/* T4 */	case AT_T4 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T4); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.iolpCMD_T4;
				}
				break;
			
/* T5 */	case AT_T5 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T5); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.iolpCMD_T5;
				}
				break;
			
/* T6 */	case AT_T6 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T6);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.iolpCMD_T6;
				}  
				break;
			
/* T7 */	case AT_T7 :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T7); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.iolpCMD_T7;
				} 
				break;

/* VR */	case AT_VR :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_vr);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.diagCMD_vr >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.diagCMD_vr & 0xFF);
				} 
				break;
			
/* HV */	case AT_HV :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_hv);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.diagCMD_hv >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.diagCMD_hv & 0xFF);
				} 
				break;
			
/* DB */	case AT_DB :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.diagCMD_db); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.diagCMD_db;
				} 
				break;
			
/* EC */	case AT_EC :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_ec);
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.diagCMD_ec >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.diagCMD_ec & 0xFF);
				} 
				break;
			
/* EA */	case AT_EA :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_ea); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.diagCMD_ea >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.diagCMD_ea & 0xFF);
				} 
				break;
			
/* DD */	case AT_DD :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX32"\r",RFmodul.diagCMD_dd);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, char down = 24; down >= 0; down -= 8, up++)
					{
						frame->msg[up] = (uint8_t *) (RFmodul.diagCMD_dd >> down)
					}
				}  
				break;

/* CT */	case AT_CT :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.atcopCMD_ct); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.atcopCMD_ct >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.atcopCMD_ct & 0xFF);
				} 
				break;
			
/* GT */	case AT_GT :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX16"\r",RFmodul.atcopCMD_gt); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = (uint8_t *) (RFmodul.atcopCMD_gt >> 8);
					frame->msg[1] = (uint8_t *) (RFmodul.atcopCMD_gt & 0xFF);
				} 
				break;
			
/* CC */	case AT_CC :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.atcopCMD_cc); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = RFmodul.atcopCMD_cc;
				} 
				break;
			
/* Rq */	case AT_Rq :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_print("ERROR\r");
				}
				else if ( frame != NULL )
				{
					return ERROR;
				} 
				break;
			
/* pC*/	case AT_pC :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_print("1\r"); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = 0x01;
				}
				break;
			
/* SB */	case AT_SB :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_print("ERROR\r"); 
				}
				else if ( frame != NULL )
				{
					return ERROR;
				} 
				break;
			
/* RU */	case DE_RU :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.deCMD_ru);
				}
				// No API handle at this place -> check API_0x18_localDevice function
				break;
			
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
			if ( RFmodul.serintCMD_ap == 0 )
			{
				UART_print("OK\r");
			} 
	    }
	    if ( AT_NI == workPointer->ID && frame->bufLen-15 > 20 )
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
	    						if ( frame->type == 0x8 )        SET_userValInEEPROM();
								if ( RFmodul.serintCMD_ap == 0 ) UART_print("OK\r");

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
	else { return INVALID_COMMAND; }
	
	return OP_SUCCESS;

/*	
ATERROR_msg :
	if ( RFmodul.serintCMD_ap == 0 )
	{
		switch( frame->ret ) 
		{
			case ERROR             : UART_print("ERROR!\r"); break;
			case INVALID_COMMAND   : UART_print("Invalid command!\r"); break;
			case INVALID_PARAMETER : UART_print("Invalid parameter!\r"); break;
			case TRANSMIT_OUT_FAIL : UART_print("TX send fail!\r"); break;
		}
	}
	else
	{
		API_0x88_atLocal_response( frame, array, NULL, 0 );
	}
	return frame->ret;
*/
}

/*
 * Specific device commands (API)
 *
 * Returns:
 *		OP_SUCCESS
 *		
 */
static ATERROR API_0x18_localDevice(struct api_f *frame, uint8_t *array)
{
	// frame id		1 byte
	cli(); BufferOut( &UART_deBuf, &frame->id ); sei();
	if (RFmodul.deCMD_ru) UART_printf("Frame ID\r%02"PRIX8"\r\r", frame->id );
	frame->crc -= frame->id;
	
	// AT command 2 bytes
	/*
	 * Why this intricate way and not simply compare 2 characters at a specific start position?
	 *   The answer is quiet simple, now there are less commands but if the cmd table grows you'll find
	 *   double matches, that's why it's better to compare 4 letters.
	 *
	 * But what is with the special device commands (DE..)?
	 *   For this case the frame type 0x18 is added to the library.
	 */
	*(array+0) = 'D';
	*(array+1) = 'E';
	cli(); BufferOut( &UART_deBuf, array+2 ); sei();
	cli(); BufferOut( &UART_deBuf, array+3 ); sei();
	
	if (RFmodul.deCMD_ru) UART_printf("DE Command\r%02"PRIX8" %02"PRIX8" (%c%c)\r\r", *(array+2), *(array+3), *(array+2), *(array+3));
	frame->crc -= (*(array+2) + *(array+3));
	frame->cmd[0] = array+2;
	frame->cmd[1] = array+3;
	
	// search for CMD in table
	CMD *workPointer = (CMD*) pStdCmdTable;
	for (int i = 0; i < command_count ; i++, workPointer++)
	{
		if( strncmp( (const char*) array, workPointer->name, 4 ) == 0 ) break;
	}
	
	/*
	 * handle CMD
	 *
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
			default: break;
		}
	}
	/*
	 * frame length
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
		
		switch (workPointer->ID)
		{
/* RU */	case DE_RU :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.deCMD_ru);
				}
				else
				{
					uint8_t val = RFmodul.deCMD_ru;
					API_0x88_atLocal_response( frame, array, (uint64_t*) &val, (uint16_t) sizeof(uint8_t) );
				}
				break;
				
			default: break;
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
		
		switch( workPointer->ID )
		{
/* RU */   case DE_RU : {
				uint8_t tmp = 0;
				if (charToUint8( &tmp, &frame->bufLen, &cmdSize, 1 ) == FALSE ) return ERROR;
				if ( cmdSize <= 1 && tmp >= FALSE && tmp <= TRUE )
				{
					RFmodul.deCMD_ru = tmp;
					if( frame->type == 0x8 ) SET_userValInEEPROM();
				}
				else { return INVALID_PARAMETER }
			}
			
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
 *		nothing
 *
 * last modified: 2016/11/30
 */
static void API_0x88_atLocal_response(struct api_f *frame, uint8_t *array, uint64_t *val, int size)
{
	uint8_t crc = 0xFF;
	char x = 0;
	
	if      ( size == 1 ) x = 0;					//          val <=  8 bit
	else if ( size == 2 ) x = 8;					//  8 bit < val <= 16 bit
	else if ( size == 4 ) x = 24;					// 16 bit < val <= 32 bit
	else if ( size == 8 ) x = 56;					// 32 bit < val <= 64 bit
	
	size += 5;
	UART_putc( frame->delimiter );					// start delimiter
	UART_putc( (uint8_t) (size >> 8) );				// frame length
	UART_putc( (uint8_t) (size & 0xFF) );
	UART_putc( 0x88 );
	crc -= 0x88;
	UART_putc( frame->id );
	crc -= frame->id;
	UART_putc( *(array+2) );						// AT cmd
	crc -= *(array+2);
	UART_putc( *(array+3) );
	crc -= *(array+3);
	UART_putc( frame->ret * (-1) );					// cmd option (successful/ not successful etc.)
	crc -= (frame->ret * (-1));
	
	if ( 'N' == *(array+2) && 'I' == *(array+3) && size > 5 )	// content
	{
		/*
		 * If val points to the NI array, we will need something special.
		 *
		 * If the original content greater than 8 bytes, than you will find the first 8 bytes 
		 * in the first array field and in the next array field are the next 8 bytes stored.
		 * example: "Hallo World", size = 11 bytes
		 * val[0] : Hallo Wo
		 * val[1] : rld\0
		 */
		for (; size > 0 && val != NULL; size--, val++)			
		{
			for (x = 0; x <= 56 && size > 0; size--, x+= 8)
			{
				UART_putc( (uint8_t) (*val >> x) );
				crc -= (uint8_t) (*val >> x);
			}
		}
	}
	else if ( size > 5 )
	{
		for (; x >= 0 ; x -= 8 )
		{
			UART_putc( (uint8_t) (*val >> x) );
			crc -= (uint8_t) (*val >> x);
		}
	}
	UART_putc(crc);									// checksum
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
static bool_t API_compareCRC(struct api_f *frame)
{
	BufferOut( &UART_deBuf, userCRC);
	if ( frame->crc == *userCRC )
	{
		return TRUE;
	} 
	else
	{
		return FALSE;
	}
	
}

CMD* API_findInTable(struct api_f *frame, uint8_t *array)
{
	// frame id		1 byte
	cli(); BufferOut( &UART_deBuf, &frame->id ); sei();
	if ( RFmodul.deCMD_ru ) UART_printf("Frame ID\r%02"PRIX8"\r\r", frame->id );
	frame->crc -= frame->id;
	
	// AT command 2 bytes
	/*
	 * Why this intricate way and not simply compare 2 characters at a specific start position?
	 *   The answer is quiet simple, now there are less commands but if the cmd table grows you'll find
	 *   double matches, that's why it's better to compare 4 letters.
	 *
	 * But what is with the special device commands (DE..)?
	 *   For this case the frame type 0x18 is added to the library.
	 */
	*(array+0) = 'A';
	*(array+1) = 'T';
	cli(); BufferOut( &UART_deBuf, array+2 ); sei();
	cli(); BufferOut( &UART_deBuf, array+3 ); sei();
	
	if (RFmodul.deCMD_ru) UART_printf("AT Command\r%02"PRIX8" %02"PRIX8" (%c%c)\r\r", *(array+2), *(array+3), *(array+2), *(array+3));
	frame->crc -= (*(array+2) + *(array+3));
	frame->cmd[0] = array+2;
	frame->cmd[1] = array+3;
	
	// search for CMD in table
	CMD *workPointer = (CMD*) pStdCmdTable;
	for (int i = 0; i < command_count ; i++, workPointer++)
	{
		if( strncmp( (const char*) array, workPointer->name, 4 ) == 0 ) 
		{
			return workPointer;
		}
	}
	
	return frame->ret = INVALID_COMMAND;
}