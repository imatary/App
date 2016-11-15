/*
 * atlocal.c
 *
 * Created: 10.11.2016 13:14:32
 *  Author: TOE
 */ 

#include <stdlib.h>							// size_t, strtol
#include <ctype.h>							// malloc, free
#include <stdint.h>							// PRIX8/16/32

#include "../header/_global.h"				// RFmodul struct
#include "../header/atlocal.h"				// prototypes
#include "../header/circularBuffer.h"		// buffer
#include "../../ATuracoli/stackrelated.h"	// UART_print(f)
#include "../../ATuracoli/stackdefines.h"	// defined register addresses

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
 * last modified: 2016/11/14
 */
ATERROR AT_localMode(void)
{
	unsigned int *counter = NULL;
	
	counter = malloc(sizeof(unsigned int));
	if ( !counter ) { free(counter); return COMMAND_MODE_FAIL; }
	*counter = 0;
	
	// TIMER_start(); // timer_hdl_t timer_start(timer_handler_t *thfunc, time_t duration, timer_arg_t arg);
	
	UART_print("\r\nOK, enter Command Mode.\r\n");
	while (noTimeout)	// runs until timer interrupt returns
	{
		inchar = UART_getc();
		if ( EOF != inchar )
		{
			// TIMER_refresh();
			UART_printf("%c", inchar );
			
			/*
			 * If within the first 5 characters a space character, don't store it in the buffer and don't count,
			 * count the length of the command
			 */
			if ( ' ' == inchar && *counter <= 4 ) continue;
			else
			{
				if (isalpha(inchar)) inchar = toupper(inchar);
				cli();	ret = BufferIn( &UART_deBuf, inchar ); sei();
				if ( ret )	{ free(counter); return COMMAND_MODE_FAIL; }
				
				*counter +=1;
			}
			
			if( '\r' == inchar ) 
			{ 
				/*
				 * - counter <  4 -> not a valid command
				 * - counter == 2 -> request/exec
				 * - counter >  4 -> write
				 * - reset counter to 0 for next cmd
				 */
				if     ( *counter <  4 ) UART_print("\nInvalid command!");
				else if( *counter == 4 ) CMD_readOrExec();
				else if( *counter >  4 ) CMD_write(counter);
				
				*counter = 0;
				
			}/* end of command handle */
			
		}/* end of uart condition */
		
	}/* end of while loop */
	
	free(counter);
	UART_print("\r\nLeave Command Mode.\r\n");
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
 * last modified: 2016/11/14
 */
static CMD* CMD_findInTable(void)
{
	uint8_t *pCmdString = NULL;
	pCmdString = malloc( sizeof(uint8_t) * 5 );
	if ( !pCmdString )
	{
		free(pCmdString);
		UART_print("No memory available!");
		return NO_AT_CMD;
	}
	
	for (int i = 0; i < 4 ; i++)
	{
		BufferOut( &UART_deBuf, pCmdString+i );
	}
	*(pCmdString+4) = '\0';
	
	// TODO -> search parser
	for (int i = 0; i < command_count ; i++, pStdCmdTable++)
	{
		if( strcmp( (const char*) pCmdString, pStdCmdTable->name ) == 0 )
		{
			free(pCmdString);
			return (CMD*) pStdCmdTable;
		}
	}
	
	free(pCmdString);	
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
 * last modified: 2016/11/14
 */
static void CMD_readOrExec(void) 
{
	CMD *pCommand = NULL;
	pCommand = malloc( sizeof(CMD) );
	if ( !pCommand )
	{
		free(pCommand);
		UART_print("Command not accomplishable, because no memory available!");
		return;
	}
	
	pCommand = CMD_findInTable();
	/*
	 * if there no valid command, free pCommand and leave function
	 * else if execute allowed perform the command
	 * else if reading allowed print the value
	 * else there is no valid option for this command
	 */
	if (NO_AT_CMD != pCommand)
	{
		free(pCommand);
		UART_print("Command not found.");
		return;
	}
	else if ( pCommand->rwxAttrib & EXEC )	// exec
	{
		switch(pCommand->ID)
		{
			// leave command mode command
			case AT_CN : {
							noTimeout = FALSE; 
							UART_print("OK\r\n");
						{
						break;
			
			default: break;
		}
	} 
	else if ( pCommand->rwxAttrib & READ )	// read
	{
		switch(pCommand->ID)
		{
			case AT_CH : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_ch); break;
			case AT_ID : UART_printf("%"PRIX16"\r\n",RFmodul.netCMD_id); break;
			case AT_DH : UART_printf("%"PRIX32"\r\n",RFmodul.netCMD_dh); break;
			case AT_DL : UART_printf("%"PRIX32"\r\n",RFmodul.netCMD_dl); break;
			case AT_MY : UART_printf("%"PRIX16"\r\n",RFmodul.netCMD_my); break;
			case AT_SH : UART_printf("%"PRIX32"\r\n",RFmodul.netCMD_sh); break;
			case AT_SL : UART_printf("%"PRIX32"\r\n",RFmodul.netCMD_sl); break;
			case AT_CE : UART_printf("%d\r\n",       RFmodul.netCMD_ce); break;
			case AT_SC : UART_printf("%"PRIX16"\r\n",RFmodul.netCMD_sc); break;
			case AT_NI : UART_printf("%s\r\n",       RFmodul.netCMD_ni); break;
			case AT_MM : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_mm); break;
			case AT_RR : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_rr); break;
			case AT_RN : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_rn); break;
			case AT_NT : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_nt); break;
			case AT_NO : UART_printf("%d\r\n",       RFmodul.netCMD_no); break;
			case AT_SD : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_sd); break;
			case AT_A1 : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_a1); break;
			case AT_A2 : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_a2); break;
			case AT_AI : UART_printf("%"PRIX8"\r\n", RFmodul.netCMD_ai); break;

			case AT_EE : UART_printf("%d\r\n",       RFmodul.secCMD_ee); break;

			case AT_PL : UART_printf("%"PRIX8"\r\n", RFmodul.rfiCMD_pl); break;
			case AT_CA : UART_printf("%"PRIX8"\r\n", RFmodul.rfiCMD_ca); break;

			case AT_SM : UART_printf("%"PRIX8"\r\n", RFmodul.sleepmCMD_sm); break;
			case AT_ST : UART_printf("%"PRIX16"\r\n",RFmodul.sleepmCMD_st); break;
			case AT_SP : UART_printf("%"PRIX16"\r\n",RFmodul.sleepmCMD_sp); break;
			case AT_DP : UART_printf("%"PRIX16"\r\n",RFmodul.sleepmCMD_dp); break;
			case AT_SO : UART_printf("%"PRIX8"\r\n", RFmodul.sleepmCMD_so); break;

			case AT_AP : UART_printf("%"PRIX8"\r\n",RFmodul.serintCMD_ap); break;
			case AT_BD : UART_printf("%"PRIX8"\r\n",RFmodul.serintCMD_bd); break;
			case AT_NB : UART_printf("%"PRIX8"\r\n",RFmodul.serintCMD_nb); break;
			case AT_RO : UART_printf("%"PRIX8"\r\n",RFmodul.serintCMD_ro); break;

			case AT_D8 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d8); break;
			case AT_D7 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d7); break;
			case AT_D6 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d6); break;
			case AT_D5 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d5); break;
			case AT_D4 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d4); break;
			case AT_D3 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d3); break;
			case AT_D2 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d2); break;
			case AT_D1 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d1); break;
			case AT_D0 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_d0); break;
			case AT_PR : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_pr); break;
			case AT_IU : UART_printf("%d\r\n",       RFmodul.ioserCMD_iu); break;
			case AT_IT : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_it); break;
			case AT_IC : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_ic); break;
			case AT_IR : UART_printf("%"PRIX16"\r\n",RFmodul.ioserCMD_ir); break;
			case AT_P0 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_p0); break;
			case AT_P1 : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_p1); break;
			case AT_PT : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_pt); break;
			case AT_RP : UART_printf("%"PRIX8"\r\n", RFmodul.ioserCMD_rp); break;

			case AT_IA : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_ia); break;
			case AT_T0 : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_T0); break;
			case AT_T1 : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_T1); break;
			case AT_T2 : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_T2); break;
			case AT_T3 : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_T3); break;
			case AT_T4 : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_T4); break;
			case AT_T5 : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_T5); break;
			case AT_T6 : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_T6); break;
			case AT_T7 : UART_printf("%"PRIX8"\r\n",RFmodul.iolpCMD_T7); break;

			case AT_VR : UART_printf("%"PRIX16"\r\n",RFmodul.diagCMD_vr); break;
			case AT_HV : UART_printf("%"PRIX16"\r\n",RFmodul.diagCMD_hv); break;
			case AT_DB : UART_printf("%"PRIX8"\r\n", RFmodul.diagCMD_db); break;
			case AT_EC : UART_printf("%"PRIX16"\r\n",RFmodul.diagCMD_ec); break;
			case AT_EA : UART_printf("%"PRIX16"\r\n",RFmodul.diagCMD_ea); break;
			case AT_DD : UART_printf("%"PRIX32"\r\n",RFmodul.diagCMD_dd); break;

			case AT_CT : UART_printf("%"PRIX16"\r\n",RFmodul.atcopCMD_ct); break;
			case AT_GT : UART_printf("%"PRIX16"\r\n",RFmodul.atcopCMD_gt); break;
			case AT_CC : UART_printf("%"PRIX8"\r\n", RFmodul.atcopCMD_cc); break;
			
			default : break;	 
					 
		}	
	}
	else	
	{		 
		UART_print("Invalid operation!\r\n");
	}
}

/*
 * Function write values into the memory
 *
 * Returns:
 *     nothing
 *				 
 * last modified: 2016/11/11
 */				 
static void CMD_write(unsigned int *len)
{
	CMD *pCommand = NULL;
	pCommand = malloc( sizeof(CMD) );
	if ( !pCommand )
	{
		free(pCommand);
		UART_print("Command not accomplishable, because no memory available!");
		return;
	}
	
	pCommand = CMD_findInTable();
	/*
	 * if there no valid command, free pCommand and leave function
	 */
	if ( NO_AT_CMD != pCommand )
	{
		free(pCommand);
		UART_print("Command not found.");
		return;
	}
	
	/* 
	 * special handle if
	 * - network identifier string command
	 * - buffer content <= 20 characters
	 */
	if ( AT_NI == pCommand->ID && \
		 (*len)-4 <= 20 )
	{
		for (int i = 0; i < (*len)-4; i++)
		{
			BufferOut(&UART_deBuf, &RFmodul.netCMD_ni[i]);
		}
		
		RFmodul.netCMD_ni[(*len)-3] = 0x0;
		UART_print("OK\r\n");
		free(pCommand);
		return;
	}
	if (AT_NI == pCommand->ID && \
		(*len)-4 > 20 )
	{
		UART_print("Invalid parameter!\r\n");
		free(pCommand);
		return;
	}


	/*
	 * if there a valid command, allocate mem for the command string
	 * fill the command string with content of the buffer
	 */
	uint8_t *pCmdString = NULL;
	size_t cmdSize = (*len % 2) + (*len-4);
	pCmdString = malloc( sizeof(uint8_t) * (cmdSize/2) );
	if ( !pCmdString )
	{
		free(pCommand);
		free(pCmdString);
		UART_print("No memory available!");
		return;
	}
	
	if ( !charToUint8(&pCmdString, len) )
	{
		free(pCommand);
		free(pCmdString);
		return;
	}

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
			 * last modified: 2016/11/15
			 */
			case AT_CH : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0B && *pCmdString <= 0x1A )
							{
								TRX_writeBit(deSR_CHANNEL, *pCmdString);
								RFmodul.netCMD_ch = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}						
						break;
			
			case AT_ID : {
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							
							if ( cmdSize/2 <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
							{
								RFmodul.netCMD_id = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_DH : {
							uint32_t tmp = (uint32_t) *(pCmdString+0) << 24 | (uint32_t) *(pCmdString+1) << 16 | (uint32_t) *(pCmdString+2) <<  8 | *(pCmdString+3);
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.netCMD_dh = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_DL : {
							uint32_t tmp = (uint32_t) *(pCmdString+0) << 24 | (uint32_t) *(pCmdString+1) << 16 | (uint32_t) *(pCmdString+2) <<  8 | *(pCmdString+3);
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.netCMD_dl = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_MY : {
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							
							if ( cmdSize/2 <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
							{
								RFmodul.netCMD_my = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_SH : {
							uint32_t tmp = (uint32_t) *(pCmdString+0) << 24 | (uint32_t) *(pCmdString+1) << 16 | (uint32_t) *(pCmdString+2) <<  8 | *(pCmdString+3);
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.netCMD_dh = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_SL : { 
							uint32_t tmp = (uint32_t) *(pCmdString+0) << 24 | (uint32_t) *(pCmdString+1) << 16 | (uint32_t) *(pCmdString+2) <<  8 | *(pCmdString+3);
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.netCMD_dh = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_CE : { 						
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x1 )
							{
								RFmodul.netCMD_ce = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_SC : { 
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							 
							if ( cmdSize/2 <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
							{
								RFmodul.netCMD_sc = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_MM : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x00 && *pCmdString <= 0x3 )
							{
								RFmodul.netCMD_mm = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_RR : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x6 )
							{
								RFmodul.netCMD_rr = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_RN : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x00 && *pCmdString <= 0x3 )
							{
								RFmodul.netCMD_rn = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_NT : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x01 && *pCmdString <= 0xFC )
							{
								RFmodul.netCMD_nt = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_NO : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x1 )
							{
								RFmodul.netCMD_no = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_SD : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xF )
							{
								RFmodul.netCMD_sd = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_A1 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xF )
							{
								RFmodul.netCMD_a1 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_A2 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xF )
							{
								RFmodul.netCMD_a2 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			/*
			 * AT commands: security
			 *
			 * last modified: 2016/11/15
			 */			
			case AT_KY : UART_print("Not implemented.\r\n"); break;

			case AT_EE : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x1 )
							{
								RFmodul.secCMD_ee = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			/*
			 * AT commands: RF interface
			 *
			 * last modified: 2016/11/15
			 */
			case AT_PL : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x4 )
							{
								RFmodul.rfiCMD_pl = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_CA : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x24 && *pCmdString <= 0x50 )
							{
								RFmodul.rfiCMD_ca = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			/*
			 * AT commands: sleep modes
			 *
			 * last modified: 2016/11/15
			 */
			case AT_SM : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x6 )
							{
								RFmodul.sleepmCMD_sm = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_ST : { 
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							
							if ( cmdSize/2 <= 2 && *pCmdString >= 0x1 && *pCmdString <= 0xFFFF )
							{
								RFmodul.sleepmCMD_st = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_SP : {
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							
							if ( cmdSize/2 <= 2 && *pCmdString >= 0x0 && *pCmdString <= 0x68B0 )
							{
								RFmodul.sleepmCMD_sp = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_DP : { 
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							
							if ( cmdSize/2 <= 2 && *pCmdString >= 0x1 && *pCmdString <= 0x68B0 )
							{
								RFmodul.sleepmCMD_dp = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_SO : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x6 )
							{
								RFmodul.sleepmCMD_so = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			/*
			 * AT commands: serial interfacing
			 *
			 * last modified: 2016/11/15
			 */
			case AT_AP : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x2 )
							{
								RFmodul.serintCMD_ap = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_BD : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x7 )
							{
								RFmodul.serintCMD_bd = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_NB : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x4 )
							{
								RFmodul.serintCMD_nb = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_RO : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.serintCMD_ro = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			/*
			 * AT commands: IO settings
			 *
			 * last modified: 2016/11/15
			 */
			case AT_D8 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d8 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_D7 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d7 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_D6 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d6 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_D5 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d5 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_D4 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d4 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_D3 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d3 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_D2 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d2 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_D1 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d1 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_D0 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x5 )
							{
								RFmodul.ioserCMD_d0 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_PR : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.ioserCMD_pr = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_IU : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x1 )
							{
								RFmodul.ioserCMD_iu = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_IT : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x1 && *pCmdString <= 0xFF )
							{
								RFmodul.ioserCMD_it = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_IC : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.ioserCMD_ic = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_IR : {
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							
							if ( cmdSize/2 <= 2 && *pCmdString >= 0x0 && *pCmdString <= 0xFFFF )
							{
								RFmodul.ioserCMD_ir = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_P0 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x2 )
							{
								RFmodul.ioserCMD_p0 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_P1 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0x2 )
							{
								RFmodul.ioserCMD_p1 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_PT : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0xB && *pCmdString <= 0xFF )
							{
								RFmodul.ioserCMD_pt = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_RP : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.ioserCMD_rp = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			/*
			 * AT commands: IO line passing
			 *
			 * last modified: 2016/11/15
			 */
			case AT_IA : {
							uint64_t tmp = (uint64_t) *(pCmdString+0) << 56 | (uint64_t) *(pCmdString+1) << 48 | (uint64_t) *(pCmdString+2) <<  40 | (uint64_t) *(pCmdString+3) << 32 |\
										   (uint64_t) *(pCmdString+4) << 24 | (uint64_t) *(pCmdString+5) << 16 | (uint64_t) *(pCmdString+6) <<   8 | (uint64_t) *(pCmdString+7);
							
							if ( cmdSize/2 <= 8 && tmp >= 0x0 && tmp <= 0xFFFFFFFFFFFFFFFF )
							{
								RFmodul.iolpCMD_ia = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_T0 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.iolpCMD_T0 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_T1 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.iolpCMD_T1 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_T2 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.iolpCMD_T2 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
	
			case AT_T3 : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.iolpCMD_T3 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_T4 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.iolpCMD_T4 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_T5 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.iolpCMD_T5 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_T6 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.iolpCMD_T6 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_T7 : {
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.iolpCMD_T7 = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			/*
			 * AT commands: diagnostics
			 *
			 * last modified: 2016/11/15
			 */
			case AT_DD : { 
							uint32_t tmp = (uint32_t) *(pCmdString+0) << 24 | (uint32_t) *(pCmdString+1) << 16 | (uint32_t) *(pCmdString+2) <<  8 | *(pCmdString+3);
							
							if ( cmdSize/2 <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
							{
								RFmodul.diagCMD_dd = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			/*
			 * AT commands: AT command options
			 *
			 * last modified: 2016/11/15
			 */
			case AT_CT : { 
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							
							if ( cmdSize/2 <= 2 && tmp >= 0x02 && tmp <= 0x1770 )
							{
								RFmodul.atcopCMD_ct = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_GT : { 
							uint16_t tmp = (uint16_t) *(pCmdString+0) << 8 | *(pCmdString+1);
							
							if ( cmdSize/2 <= 2 && tmp >= 0x02 && tmp <= 0xCE4 )
							{
								RFmodul.atcopCMD_gt = tmp;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			case AT_CC : { 
							if ( cmdSize/2 <= 1 && *pCmdString >= 0x0 && *pCmdString <= 0xFF )
							{
								RFmodul.atcopCMD_cc = *pCmdString;
								UART_print("OK\r\n");
								
							}
							else { UART_print("Invalid parameter!\r\n"); }
						}
						break;
			
			default : UART_print("No function available.\r\n"); break;		
		}
	}
	else
	{
		UART_print("Invalid operation!");
	}
	
	free(pCmdString);
	free(pCommand);
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
static bool_t charToUint8(uint8_t **ppCmdString, int *len)
{
	uint8_t *tmp = NULL;
	tmp = malloc( sizeof(uint8_t)*2 );
	if ( !tmp )
	{ 
		free(tmp);
		UART_print("Error while converting input!");
		return FALSE;
	}
	int pos = 0, c = 0;
	
	if ( ((*len)-4) % 2 == 1 )
	{
		*(tmp+c) = '0';
		c = 1;
	}
	
	do{
		for(; c < 2; c++)
		{
			cli(); BufferOut( &UART_deBuf, &tmp[c] ); sei();
			if( *(tmp+c) == 0x20 ) BufferOut( &UART_deBuf, &tmp[c] );	// ' ' == 0x20
			if( *(tmp+c) == 0x0D ) return TRUE;							// '\r' == 0x0D
		}
		sprintf((char*)(ppCmdString+pos),"%c", (unsigned int) strtol((const char*)tmp,NULL,16) >> 8 );
		c = 0;
		pos++;

	}while(TRUE);
}