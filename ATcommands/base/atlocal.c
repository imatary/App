/*
 * atlocal.c
 *
 * Created: 10.11.2016 13:14:32
 *  Author: TOE
 */
#include <inttypes.h>						// PRIX8/16/32
#include <stdlib.h>							// size_t, strtol
#include <ctype.h>							// malloc, free

#include "../header/_global.h"				// RFmodul struct
#include "../header/atlocal.h"				// prototypes
#include "../header/setter.h"
#include "../header/circularBuffer.h"		// buffer
#include "../../ATuracoli/stackrelated.h"	// UART_print(f)
#include "../../ATuracoli/stackdefines.h"	// defined register addresses

bool_t noTimeout = TRUE;

/*
 * AT_localMode()
 * - reset the timer
 * - return character immediately
 * - push the character into the buffer (neither interrupts allowed)
 * - if a carriage return (0xD) received, handle the buffer content
 *
 * Returns:
 *     OP_SUCCESS			if no error occurred
 *     COMMAND_MODE_FAIL	if the working buffer not correctly initialized
 *							if the incoming buffer unable to receive characters
 *
 * last modified: 2016/11/18
 */
ATERROR AT_localMode(void)
{
	ATERROR ret     = 0;
	int		inchar  = 0;
	int     counter = 0;	
	noTimeout       = TRUE;
	
	// TIMER_start(); // timer_hdl_t timer_start(timer_handler_t *thfunc, time_t duration, timer_arg_t arg);
	
	UART_print("\rOK\r");
	while (noTimeout)	// runs until timer interrupt returns
	{
		inchar = UART_getc();
		if ( EOF != inchar && (isgraph(inchar) || isspace(inchar)) )
		{
			// TIMER_refresh();
			
			/*
			 * If within the first 5 characters a space character, don't store it in the buffer and don't count,
			 * count the length of the command
			 */
			if ( ' ' == inchar && counter <= 4 ) continue;
			else
			{
				if ( isalpha(inchar) && islower(inchar) ) inchar = toupper(inchar);

				cli(); ret = BufferIn( &UART_deBuf, inchar ); sei();
				if ( ret )	{ return COMMAND_MODE_FAIL; }
				
				counter +=1;
			}
			
			if( '\r' == inchar ) 
			{ 
				/*
				 * - counter <  5 -> not a valid command
				 * - counter == 5 -> request/exec
				 * - counter >  5 -> write
				 * - reset counter to 0 for next cmd
				 */
				if     ( counter <  5 ) 
				{
					UART_print("Invalid command!\r");
					deBufferReadReset( &UART_deBuf, '+', counter );
				}
				else if( counter == 5 ) CMD_readOrExec();
				else if( counter >  5 ) CMD_write(&counter);
				
				counter = 0;
				
			}/* end of command handle */
			
		}/* end of uart condition */
		
	}/* end of while loop */

	UART_print("Leave Command Mode.\r");
	return OP_SUCCESS;
}

/*
 * CMD_findInTable() 
 * - searched in the command table for the command id
 *
 * Returns:
 *	   CMD struct	on success
 *     NO_AT_CMD	on fail
 *
 * last modified: 2016/11/18
 */
static CMD* CMD_findInTable(void)
{
	CMD *workPointer = (CMD*) pStdCmdTable;
	uint8_t pCmdString[5] = {0};
	
	for (int i = 0; i < 4 ; i++)
	{
		cli(); BufferOut( &UART_deBuf, &pCmdString[i] ); sei();
	}
	pCmdString[4] = '\0';
	// TODO -> search parser
	for (int i = 0; i < command_count ; i++, workPointer++)
	{
		if( strncmp( (const char*) pCmdString, workPointer->name, 4 ) == 0 )
		{
			return workPointer;
		}
	}

	return NO_AT_CMD;
}

/*
 * CMD_readOrExec() 
 * - read the values of the memory
 * - or execute commands
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/11/18
 */
static void CMD_readOrExec(void) 
{
	CMD *pCommand = CMD_findInTable();
	/*
	 * remove the '\r' from the buffer
	 */
	deBufferReadReset( &UART_deBuf, '+', 1); 
	/*
	 * if there no valid command, leave function
	 * else if execute allowed perform the command
	 * else if reading allowed print the value
	 * else there is no valid option for this command
	 */
	if (NO_AT_CMD == pCommand) 
	{ 
		UART_print("Command not found.\r"); 
		return; 
	}
	else if ( pCommand->rwxAttrib & EXEC )	// exec
	{
		switch(pCommand->ID)
		{
			// leave command mode command
			case AT_CN : {
				noTimeout = FALSE; 
				UART_print("OK\r");
			}
			break;
			
			// write config to firmware - currently only a dummy
			case AT_WR : {
				UART_print("OK\r");
			}
			break;
			
			// apply changes - currently only a dummy
			case AT_AC : {
				UART_print("OK\r");
			}
			break;
			
			// reset all parameter
			case AT_RE : {
				SET_allDefault();
				UART_print("OK\r");
			}
			break;
			
			default: break;
		}
	} 
	else if ( pCommand->rwxAttrib & READ )	// read
	{
		switch(pCommand->ID)
		{
			case AT_CH : UART_printf("%"PRIX8"\r", RFmodul.netCMD_ch); break;
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

			case AT_IA : {	// the compile don't like the PRIX64 command
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
	else	
	{		 
		UART_print("Invalid operation!\r");
	}
}

/*
 * Function write values into the memory
 *
 * Returns:
 *     nothing
 *				 
 * last modified: 2016/11/18
 */				 
static void CMD_write(unsigned int *len)
{
	CMD *pCommand = CMD_findInTable();
	/*
	 * if there no valid command, free pCommand and leave function
	 */
	if ( NO_AT_CMD == pCommand )
	{
		UART_print("Command not found.\r");
		return;
	}
	
	/* 
	 * special handle if
	 * - network identifier string command
	 * - buffer content <= 20 characters
	 */
	if ( AT_NI == pCommand->ID && (*len)-4 <= 20 )
	{
		for (int i = 0; i < (*len)-4; i++)
		{
			cli(); BufferOut(&UART_deBuf, &RFmodul.netCMD_ni[i]); sei();
		}
		
		RFmodul.netCMD_ni[(*len)-3] = 0x0;
		UART_print("OK\r");
		return;
	}
	if (AT_NI == pCommand->ID && (*len)-4 > 20 )
	{
		UART_print("Please insert max 20 characters!\r");
		return;
	}


	/*
	 * if there a valid command, allocate mem for the command string
	 * fill the command string with content of the buffer
	 */
	size_t cmdSize = ( (*len-1) % 2) + (*len-4);
	uint8_t cmdString[cmdSize/2];
	
	if ( charToUint8( &cmdString[0], len ) == FALSE ) return;

	/*
	 * if writing is allowed store the value of the string into RFmodel struct and register
	 * copy value for comparison in tmp buffer
	 * if - the command size smaller or equal then the unit of the tmp buffer (related to the uint8 buffer)
	 *    - the buffer value greater or equal than the min value
	 *    - the buffer value smaller or equal than the max value
	 * save the value into the memory
	 * else it is a invalid parameter
	 */	
	if (pCommand->rwxAttrib & WRITE )
	{
		switch(pCommand->ID)
		{
			/*
			 * AT commands: network
			 *
			 * last modified: 2016/11/18
			 */
			case AT_CH : { 
							uint8_t tmp = cmdString[0];
							UART_printf("%d - %d - %d\r", cmdSize/2 <= 1, tmp >= 0x0B, tmp <= 0x1A);
							UART_printf("%d - %x\r", cmdSize/2, tmp);
							if ( cmdSize/2 <= 1 && tmp >= 0x0B && tmp <= 0x1A )
							{
								TRX_writeBit(deSR_CHANNEL, cmdString[0]);
								RFmodul.netCMD_ch = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}						
						break;
			
			case AT_ID : {
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							
							if ( cmdSize/2 <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
							{
								RFmodul.netCMD_id = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_DH : {
							uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.netCMD_dh = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_DL : {
							uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.netCMD_dl = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_MY : {
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							
							if ( cmdSize/2 <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
							{
								RFmodul.netCMD_my = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_SH : {
							uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.netCMD_dh = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_SL : { 
							uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.netCMD_dh = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_CE : { 		
							uint8_t tmp = cmdString[0];
											
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x1 )
							{
								RFmodul.netCMD_ce = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_SC : { 
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							 
							if ( cmdSize/2 <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
							{
								RFmodul.netCMD_sc = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_MM : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x00 && tmp <= 0x3 )
							{
								RFmodul.netCMD_mm = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_RR : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x6 )
							{
								RFmodul.netCMD_rr = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_RN : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x00 && tmp <= 0x3 )
							{
								RFmodul.netCMD_rn = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_NT : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x01 && tmp <= 0xFC )
							{
								RFmodul.netCMD_nt = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_NO : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x1 )
							{
								RFmodul.netCMD_no = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_SD : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xF )
							{
								RFmodul.netCMD_sd = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_A1 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xF )
							{
								RFmodul.netCMD_a1 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_A2 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xF )
							{
								RFmodul.netCMD_a2 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			/*
			 * AT commands: security
			 *
			 * last modified: 2016/11/18
			 */			
			case AT_KY : UART_print("Not implemented.\r"); break;

			case AT_EE : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x1 )
							{
								RFmodul.secCMD_ee = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			/*
			 * AT commands: RF interface
			 *
			 * last modified: 2016/11/18
			 */
			case AT_PL : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x4 )
							{
								RFmodul.rfiCMD_pl = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_CA : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x24 && tmp <= 0x50 )
							{
								RFmodul.rfiCMD_ca = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			/*
			 * AT commands: sleep modes
			 *
			 * last modified: 2016/11/18
			 */
			case AT_SM : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x6 )
							{
								RFmodul.sleepmCMD_sm = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_ST : { 
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							
							if ( cmdSize/2 <= 2 && tmp >= 0x1 && tmp <= 0xFFFF )
							{
								RFmodul.sleepmCMD_st = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_SP : {
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							
							if ( cmdSize/2 <= 2 && tmp >= 0x0 && tmp <= 0x68B0 )
							{
								RFmodul.sleepmCMD_sp = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_DP : { 
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							
							if ( cmdSize/2 <= 2 && tmp >= 0x1 && tmp <= 0x68B0 )
							{
								RFmodul.sleepmCMD_dp = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_SO : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x6 )
							{
								RFmodul.sleepmCMD_so = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			/*
			 * AT commands: serial interfacing
			 *
			 * last modified: 2016/11/18
			 */
			case AT_AP : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x2 )
							{
								RFmodul.serintCMD_ap = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_BD : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x7 )
							{
								RFmodul.serintCMD_bd = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_NB : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x4 )
							{
								RFmodul.serintCMD_nb = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_RO : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.serintCMD_ro = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			/*
			 * AT commands: IO settings
			 *
			 * last modified: 2016/11/18
			 */
			case AT_D8 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d8 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_D7 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d7 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_D6 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d6 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_D5 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d5 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_D4 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d4 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_D3 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d3 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_D2 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d2 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_D1 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d1 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_D0 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x5 )
							{
								RFmodul.ioserCMD_d0 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_PR : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.ioserCMD_pr = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_IU : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x1 )
							{
								RFmodul.ioserCMD_iu = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_IT : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x1 && tmp <= 0xFF )
							{
								RFmodul.ioserCMD_it = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_IC : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.ioserCMD_ic = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_IR : {
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							
							if ( cmdSize/2 <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
							{
								RFmodul.ioserCMD_ir = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_P0 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x2 )
							{
								RFmodul.ioserCMD_p0 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_P1 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0x2 )
							{
								RFmodul.ioserCMD_p1 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_PT : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0xB && tmp <= 0xFF )
							{
								RFmodul.ioserCMD_pt = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_RP : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.ioserCMD_rp = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			/*
			 * AT commands: IO line passing
			 *
			 * last modified: 2016/11/18
			 */
			case AT_IA : {
							uint64_t tmp = (uint64_t) cmdString[0] << 56 | (uint64_t) cmdString[1] << 48 | (uint64_t) cmdString[2] <<  40 | (uint64_t) cmdString[3] << 32 |\
										   (uint64_t) cmdString[4] << 24 | (uint64_t) cmdString[5] << 16 | (uint64_t) cmdString[6] <<   8 | (uint64_t) cmdString[7];
							
							if ( cmdSize/2 <= 8 && tmp >= 0x0 && tmp <= 0xFFFFFFFFFFFFFFFF )
							{
								RFmodul.iolpCMD_ia = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_T0 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.iolpCMD_T0 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_T1 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.iolpCMD_T1 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_T2 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.iolpCMD_T2 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
	
			case AT_T3 : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.iolpCMD_T3 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_T4 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.iolpCMD_T4 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_T5 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.iolpCMD_T5 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_T6 : {
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.iolpCMD_T6 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_T7 : {
							uint8_t tmp = cmdString[0];
														
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.iolpCMD_T7 = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			/*
			 * AT commands: diagnostics
			 *
			 * last modified: 2016/11/18
			 */
			case AT_DD : { 
							uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.diagCMD_dd = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			/*
			 * AT commands: AT command options
			 *
			 * last modified: 2016/11/18
			 */
			case AT_CT : { 
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							
							if ( cmdSize/2 <= 2 && tmp >= 0x02 && tmp <= 0x1770 )
							{
								RFmodul.atcopCMD_ct = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_GT : { 
							uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
							
							if ( cmdSize/2 <= 2 && tmp >= 0x02 && tmp <= 0xCE4 )
							{
								RFmodul.atcopCMD_gt = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			case AT_CC : { 
							uint8_t tmp = cmdString[0];
							
							if ( cmdSize/2 <= 1 && tmp >= 0x0 && tmp <= 0xFF )
							{
								RFmodul.atcopCMD_cc = tmp;
								UART_print("OK\r");
								
							}
							else { UART_print("Invalid parameter!\r"); }
						}
						break;
			
			default : UART_print("No function available.\r"); break;		
		}
	}
	else
	{
		UART_print("Invalid operation!\r");
	}
}


/*
 * helper function charToUint8()
 * - received the buffer content and converted content to uint8 hex values
 * - the char 'A' will be uint8_t hex 0xA and so on
 * 
 * Returns:
 *     TRUE  on success
 *	   FALSE on failure
 *
 * last modified: 2016/11/14
 */
static bool_t charToUint8(uint8_t *cmdString, int *len)
{
	if ( 1 >= *len ) return FALSE;
	uint8_t tmp[2] = {0};
	int pos = 0, c = 0;
	
	/*
	 * if the len is an uneven number, set the first field to zero
	 * to avoid a false calculation
	 */
	if ( ((*len)-5) % 2 == 1 )
	{
		tmp[c] = '0';
		c = 1;
	}
	
	do{
		for(; c < 2; c++)
		{
			cli(); BufferOut( &UART_deBuf, &tmp[c] ); sei();
			if( tmp[c] == 0x20 ) BufferOut( &UART_deBuf, &tmp[c] );	// ' ' == 0x20	ignore spaces
			if( tmp[c] == 0x0D ) return TRUE;						// '\r' == 0x0D and line break
		}
		/*
		 * if the calculated position size greater than calculated array size return failure
		 * else print the new hex value into the array
		 */
		if ( ((*len-1) % 2) + (*len-4) < pos ) return FALSE;
		sprintf((char*)(cmdString+pos),"%c", (unsigned int) strtol( (const char*) &tmp, NULL, 16) );
		c = 0;
		pos++;

	}while(TRUE);
}