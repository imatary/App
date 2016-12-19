/*
 * atlocal.c
 *
 * Created: 10.11.2016 13:14:32
 *  Author: TOE
 */
#include <inttypes.h>							// PRIX8/16/32
#include <stdlib.h>								// size_t, strtol
#include <ctype.h>								// malloc, free

#include "../header/_global.h"					// RFmodul struct
#include "../header/atlocal.h"					// prototypes
#include "../header/rfmodul.h"
#include "../header/cmd.h"						// CMD
#include "../header/circularBuffer.h"			// buffer
#include "../../ATuracoli/stackrelated.h"		// UART_print(f)
#include "../../ATuracoli/stackrelated_timer.h"	// UART_print(f)
#include "../../ATuracoli/stackdefines.h"		// defined register addresses

// === Prototypes =========================================
/*
 * Time handler to interrupt the command mode
 */
static uint32_t CMD_timeHandle(uint32_t arg);

/*
 * translate ASCII parameter to Hex values
 */
bool_t charToUint8(uint8_t *cmdString, size_t *strlength, size_t *cmdSize ,size_t maxCmdSize);


// === c-File Globals =====================================
bool_t noTimeout = TRUE;

// === Functions ==========================================
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
 * last modified: 2016/12/19
 */
void AT_localMode(void)
{
	at_status_t ret   = 0;
	int		inchar    = 0;
	size_t  counter   = 0;
	uint32_t th		  = 0;
	noTimeout         = TRUE;
	
	th = deTIMER_start(CMD_timeHandle, deMSEC( RFmodul.atcopCMD_ct * 0x64), 0); // start( *thfunc, duration, arg for callback );
	if ( FALSE == th ) 
	{
		UART_print("Timer could not start!\rPlease quit commandmode with ATCN.\r");
	}
	
	UART_print("OK\r");
	while (noTimeout)	// runs until timer interrupt returns
	{
		inchar = UART_getc();
		if ( EOF != inchar && (isgraph(inchar) || isspace(inchar)) )
		{			
			/*
			 * If within the first 5 characters a space character, don't store it in the buffer and don't count,
			 * count the length of the command
			 */
			if ( ' ' == inchar && counter <= 4 ) continue;
			else
			{
				if ( isalpha(inchar) && islower(inchar) ) inchar = toupper(inchar);

				cli(); ret = BufferIn( &UART_deBuf, inchar ); sei();
				if ( ret ) 
				{ 
					UART_print("Buffer in fail, please reenter command mode!\r");
					return; 
				}
				
				counter +=1;
			}
			
			if( '\r' == inchar || '\n' == inchar ) 
			{ 
				/*
				 * - counter <  5 -> not a valid command
				 * - counter == 5 -> request/exec
				 * - counter >  5 -> write
				 * - reset counter to 0 for next cmd
				 */
				th = deTIMER_restart(th, deMSEC( RFmodul.atcopCMD_ct * 0x64) );
				if ( FALSE == th )
				{
					UART_print("Timer could not start!\rPlease quit commandmode with ATCN.\r");
				}
				if     ( counter <  5 ) 
				{
					UART_print("Invalid command!\r");
					deBufferReadReset( &UART_deBuf, '+', counter );
				}
				else if( counter == 5 ) ret = CMD_readOrExec(&th);
				else if( counter >  5 ) ret = CMD_write(&counter, FALSE);
				if ( ret ) 
				{
					switch( ret )
					{
						case ERROR             : UART_print("ERROR!\r"); break;
						case INVALID_COMMAND   : UART_print("Invalid command!\r"); break;
						case INVALID_PARAMETER : UART_print("Invalid parameter!\r"); break;
						case TRANSMIT_OUT_FAIL : UART_print("TX send fail!\r"); break;
					}
					ret = 0;
				}
				counter = 0;
				
			}/* end of command handle */
			
		}/* end of uart condition */
		
	}/* end of while loop */

	UART_print("Leave Command Mode.\r");
}

/*
 * CMD_readOrExec() 
 * - read the values of the memory
 * - or execute commands
 *
 * Received:
 *		if in  AT Mode, a time handler pointer else a NULL pointer
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *		ERROR				if an process error occurred
 *
 * last modified: 2016/12/19
 */
at_status_t CMD_readOrExec(uint32_t *th) 
{
	CMD *pCommand  = NULL;
	uint8_t pCmdString[5] = {'A','T',0,0,0};
		
	if ( RFmodul.serintCMD_ap > 0 && NULL == th ) // AP frame
	{ 
		cli(); BufferOut( &UART_deBuf, &pCmdString[2] ); sei();
		cli(); BufferOut( &UART_deBuf, &pCmdString[3] ); sei();
		AP_setATcmd(pCmdString);
	}
	else // AT CMD
	{
		for (int i = 0; i < 4 ; i++)
		{
			cli(); BufferOut( &UART_deBuf, &pCmdString[i] ); sei();
		}
	}
	
	/*
	 * if there not a valid command, leave function
	 */
	pCommand = CMD_findInTable(pCmdString);
	if ( NO_AT_CMD == pCommand->ID || NULL == pCommand ) return INVALID_COMMAND;
		
	/*
	 * handle CMD
	 * if AP frame, check frame crc
	 * exec is allowed
	 */
	if ( RFmodul.serintCMD_ap > 0 && th == NULL ) 
	{
		if ( AP_compareCRC() == FALSE )	return ERROR;
	}
	else
	{
		// remove the '\r'/'\n' from the buffer if in CMD Mode
		deBufferReadReset( &UART_deBuf, '+', 1); 
	}
	
	if ( pCommand->rwxAttrib & EXEC )
	{
		if ( RFmodul.serintCMD_ap > 0 && th == NULL ) AP_setRWXopt(EXEC);	
		switch( pCommand->ID )
		{
			// leave command mode command
/* CN */    case AT_CN : 
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
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
				if ( RFmodul.serintCMD_ap == 0  || th != NULL ) 
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
				uint32_t baud = deHIF_DEFAULT_BAUDRATE;
				switch( RFmodul.serintCMD_bd )
				{
					case 0x0 : baud =   1200; break;
					case 0x1 : baud =   2400; break;
					case 0x2 : baud =   4800; break;
					case 0x3 : baud =   9600; break;
					case 0x4 : baud =  19200; break;
					case 0x5 : baud =  38400; break;
					case 0x6 : baud =  57600; break;
					case 0x7 : baud = 115200; break;
					default : baud = deHIF_DEFAULT_BAUDRATE; break;
				}
				hif_init(baud);
				TRX_setPanId( RFmodul.netCMD_id );
				TRX_writeBit(deSR_CHANNEL, RFmodul.netCMD_ch);
				
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_print("OK\r");
				}
				else
				{
					return OP_SUCCESS;
				}
			}
			break;
			
			// reset all parameter
/* RE */    case AT_RE : 
				SET_allDefault();
				if ( RFmodul.serintCMD_ap == 0  || th != NULL ) 
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
		bool_t gtZereo = FALSE;
		if ( RFmodul.serintCMD_ap > 0 && th == NULL ) AP_setRWXopt(READ);	

		switch( pCommand->ID )
		{
/* CH */	case AT_CH : 
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_ch); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_ch;
					AP_setMSG( &val, 1, TRUE);
				}
				break;
			
/* ID */	case AT_ID :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_id);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_id, 2, TRUE );
				}  
				break;
			
/* DH */	case AT_DH :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_dh);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_dh, 4, TRUE );
				}  
				break;
			
/* DL */	case AT_DL :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX32"\r", RFmodul.netCMD_dl);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_dl, 4, TRUE );
				}  
				break;
			
/* MY */	case AT_MY :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_my);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_my, 2, TRUE );
				}  
				break;
			
/* SH */	case AT_SH :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_sh);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_sh, 4, TRUE );
				}  
				break;
			
/* SL */	case AT_SL :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_sl);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_sl, 4, TRUE );
				}  
				break;
			
/* CE */	case AT_CE :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%d\r",       RFmodul.netCMD_ce);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_ce;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* SC */	case AT_SC :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_sc);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_sc, 2, TRUE );
				}
				break;
			
/* NI */	case AT_NI :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%s\r", RFmodul.netCMD_ni); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_ni, strlen( (const char*) RFmodul.netCMD_ni)+1, FALSE );
				}
				break;
			
/* MM */	case AT_MM :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_mm);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_mm;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* RR */	case AT_RR :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_rr); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_rr;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* RN */	case AT_RN :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_rn);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_rn;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* NT */	case AT_NT :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_nt);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.netCMD_nt, 1, TRUE );
				} 
				break;
			
/* NO */	case AT_NO :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%d\r", RFmodul.netCMD_no);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_no;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* SD */	case AT_SD :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_sd);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_sd;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* A1 */	case AT_A1 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_a1);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_a1;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* A2 */	case AT_A2 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_a2);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_a2;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* AI */	case AT_AI :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_ai);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.netCMD_ai;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;

/* EE */	case AT_EE :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%d\r",       RFmodul.secCMD_ee); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.secCMD_ee;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* KY */	case AT_KY :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_print("\r");
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setRWXopt(EXEC); // no value should returned
				}
				break;

/* PL */	case AT_PL :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.rfiCMD_pl);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.rfiCMD_pl;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* CA */	case AT_CA :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.rfiCMD_ca);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.rfiCMD_ca;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;

/* SM */	case AT_SM :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.sleepmCMD_sm);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.sleepmCMD_sm;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* CH */	case AT_ST :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_st);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.sleepmCMD_st, 2, TRUE );
				}  
				break;
			
/* SP */	case AT_SP :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_sp); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.sleepmCMD_sp;
					AP_setMSG( &val, 2, TRUE );
				} 
				break;
			
/* DP */	case AT_DP :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r", RFmodul.sleepmCMD_dp);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint16_t val = RFmodul.sleepmCMD_dp;
					AP_setMSG( &val, 2, TRUE );
				}  
				break;
			
/* SO */	case AT_SO :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.sleepmCMD_so);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.sleepmCMD_so;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
			
/* SS */	case AT_SS : 
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					 UART_print("ERROR\r");
				} 
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					return ERROR;
				}
				break;
				
/* AP */	case AT_AP : 
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.serintCMD_ap);
				} 
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.serintCMD_ap;
					AP_setMSG( &val, 1, TRUE );
				}
				break;
			
/* BD */    case AT_BD :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.serintCMD_bd);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.serintCMD_bd;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* NB */    case AT_NB :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.serintCMD_nb);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.serintCMD_nb;
					AP_setMSG( &val, 1, TRUE );
				}  
				break;
				
/* RO */	case AT_RO :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					 UART_printf("%"PRIX8"\r",RFmodul.serintCMD_ro);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.serintCMD_ro, 1, TRUE );
				} 
				break;

/* D8 */	case AT_D8 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d8); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d8;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* D7 */	case AT_D7 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d7); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d7;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* D6 */	case AT_D6 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d6); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d6;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* D5 */	case AT_D5 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d5); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d5;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* D4 */	case AT_D4 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d4); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d4;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* D3 */	case AT_D3 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d3); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d3;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* D2 */	case AT_D2 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d2);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d2;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* D1 */	case AT_D1 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d1); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d1;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* D0 */	case AT_D0 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d0); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_d0;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* PR */	case AT_PR :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_pr);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.ioserCMD_pr, 1, TRUE );
				}
				break;
			
/* IU */	case AT_IU :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%d\r", RFmodul.ioserCMD_iu);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_iu;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* IT */	case AT_IT :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_it);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.ioserCMD_it, 1, TRUE );
				} 
				break;
			
/* IC */	case AT_IC :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_ic);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.ioserCMD_ic, 1, TRUE );
				} 
				break;
			
/* IR */	case AT_IR :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r", RFmodul.ioserCMD_ir); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.ioserCMD_ir, 2, TRUE );
				} 
				break;
			
/* P0 */	case AT_P0 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_p0);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_p0;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* P1 */	case AT_P1 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_p1); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = RFmodul.ioserCMD_p1;
					AP_setMSG( &val, 1, TRUE );
				} 
				break;
			
/* PT */	case AT_PT :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_pt); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.ioserCMD_pt, 1, TRUE );
				} 
				break;
			
/* RP */	case AT_RP :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_rp); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.ioserCMD_rp, 1, TRUE );
				} 
				break;

/* IA */	case AT_IA :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					// the compiler don't like the PRIX64 command
					uint32_t a = RFmodul.iolpCMD_ia >> 32;
					uint32_t b = RFmodul.iolpCMD_ia & 0xFFFFFFFF;
					UART_printf("%"PRIX32"%"PRIX32"\r",a,b);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_ia, 8, TRUE );
				}
				break;
			
/* T0 */	case AT_T0 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T0); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_T0, 1, TRUE );
				} 
				break;
			
/* T1 */	case AT_T1 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T1); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_T1, 1, TRUE );
				}
				break;
			
/* T2 */	case AT_T2 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T2); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_T2, 1, TRUE );
				}
				break;
			
/* T3 */	case AT_T3 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T3); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_T3, 1, TRUE );
				}
				break;
			
/* T4 */	case AT_T4 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T4); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_T4, 1, TRUE );
				}
				break;
			
/* T5 */	case AT_T5 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T5); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_T5, 1, TRUE );
				}
				break;
			
/* T6 */	case AT_T6 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T6);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_T6, 1, TRUE );
				}  
				break;
			
/* T7 */	case AT_T7 :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T7); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.iolpCMD_T7, 1, TRUE );
				} 
				break;

/* VR */	case AT_VR :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_vr);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.diagCMD_vr, 2, TRUE );
				} 
				break;
			
/* HV */	case AT_HV :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_hv);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.diagCMD_hv, 2, TRUE );
				} 
				break;
			
/* DB */	case AT_DB :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.diagCMD_db); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.diagCMD_db, 1, TRUE );
				} 
				break;
			
/* EC */	case AT_EC :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_ec);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.diagCMD_ec, 2, TRUE );
				} 
				break;
			
/* EA */	case AT_EA :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_ea); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.diagCMD_ea, 2, TRUE );
				} 
				break;
			
/* DD */	case AT_DD :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX32"\r",RFmodul.diagCMD_dd);
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.diagCMD_dd, 4, TRUE );
				}  
				break;

/* CT */	case AT_CT :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.atcopCMD_ct); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint16_t val = RFmodul.atcopCMD_ct;
					AP_setMSG( &val, 2, TRUE );
				} 
				break;
			
/* GT */	case AT_GT :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.atcopCMD_gt); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint16_t val = RFmodul.atcopCMD_gt;
					AP_setMSG( &val, 2, TRUE );
				} 
				break;
			
/* CC */	case AT_CC :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.atcopCMD_cc); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					AP_setMSG( &RFmodul.atcopCMD_cc, 1, TRUE );
				} 
				break;
			
/* Rq */	case AT_Rq :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_print("ERROR\r");
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					return ERROR;
				} 
				break;
			
/* pC*/	case AT_pC :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_print("1\r"); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					uint8_t val = 0x01;
					AP_setMSG( &val, 1, TRUE );
				}
				break;
			
/* SB */	case AT_SB :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_print("ERROR\r"); 
				}
				else if ( RFmodul.serintCMD_ap > 0 && th == NULL )
				{
					return ERROR;
				} 
				break;
			
/* RU */	case DE_RU :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.deCMD_ru);
				}
				// No AP handle at this place -> check AP_0x18_localDevice function
				break;

/* FV */	case DE_FV :
				if ( RFmodul.serintCMD_ap == 0  || th != NULL )
				{
					UART_printf("%s\r", AT_VERSION);
				}
				// No AP handle at this place -> check AP_0x18_localDevice function
				break;
			
			default : return INVALID_COMMAND;
		}
	} 
	else
	{
		return ERROR;
	}
	return OP_SUCCESS;
}

/*
 * CMD_write()
 * write values into the memory
 *
 * Received:
 *		if in AP Mode, a frame struct else a NULL pointer
 *		if in AP Mode, a pointer to an already allocated array for processing else  a NULL pointer
 *		if in  AT Mode, a time handler pointer else a NULL pointer
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *		INVALID_PARAMETER	if the delivered parameter is not valid
 *		ERROR				if an process error occurred
 *				 
 * last modified: 2016/12/19
 */				 
at_status_t CMD_write(size_t *len, bool_t apFrame)
{	
	CMD *pCommand  = NULL;
	size_t cmdSize = 0;
	uint8_t pCmdString[5] = {'A','T',0,0,0};
		
	if ( RFmodul.serintCMD_ap > 0 && TRUE == apFrame ) // AP frame
	{ 
		cmdSize = (*len)-4;
		cli(); BufferOut( &UART_deBuf, &pCmdString[2] ); sei();
		cli(); BufferOut( &UART_deBuf, &pCmdString[3] ); sei();
		AP_setATcmd(pCmdString);
		AP_setRWXopt(WRITE);
	}
	else // AT CMD
	{
		cmdSize = (((*len-1) % 2) + (*len-5))/2;
		for (int i = 0; i < 4 ; i++)
		{
			cli(); BufferOut( &UART_deBuf, &pCmdString[i] ); sei();
		}
	}
	
	/*
	 * if there not a valid command, leave function
	 */
	pCommand = CMD_findInTable(pCmdString);
	if ( NO_AT_CMD == pCommand->ID || NULL == pCommand ) return INVALID_COMMAND;
		
	/* 
	 * special handle if
	 * - network identifier string command
	 * - buffer content <= 20 characters
	 */
	*len -= 4;
	if ( AT_NI == pCommand->ID && *len <= 20 )
	{
		for (int i = 0; i < (*len); i++)
		{
			cli(); BufferOut(&UART_deBuf, &RFmodul.netCMD_ni[i]); sei();
			if ( TRUE == apFrame ) AP_updateCRC(&RFmodul.netCMD_ni[i]);
		}	
		RFmodul.netCMD_ni[(*len)+1] = 0x0;
		if ( RFmodul.serintCMD_ap == 0 ) UART_print("OK\r");
		
		if ( TRUE == apFrame && AP_compareCRC() == FALSE ) return ERROR;
		else										       return OP_SUCCESS;
	}
	if (AT_NI == pCommand->ID && *len > 20 )
	{
		if ( RFmodul.serintCMD_ap == 0 ) UART_print("Please insert max 20 characters!\r");
		return INVALID_PARAMETER;
	}

	/*
	 * if writing is allowed store the value of the string into RFmodel struct and register
	 * copy value for comparison in tmp buffer
	 * if - the command size smaller or equal then the unit of the tmp buffer (related to the uint8 buffer)
	 *    - the buffer value greater or equal than the min value
	 *    - the buffer value smaller or equal than the max value
	 * write to RFmodul struct [and to EEPROM]
	 * else it is a invalid parameter
	 */	
	if (pCommand->rwxAttrib & WRITE )
	{
		switch(pCommand->ID)
		{
			/*
			 * AT commands: network
			 *
			 * last modified: 2016/12/02
			 */
/* CH */	case AT_CH : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0 )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0B && tmp <= 0x1A )
				{
					TRX_writeBit(deSR_CHANNEL, tmp);
					RFmodul.netCMD_ch = tmp;
					if ( RFmodul.serintCMD_ap == 0 ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* ID */	case AT_ID : {
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0 )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.netCMD_id = tmp;
					if ( RFmodul.serintCMD_ap == 0 ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* DH */	case AT_DH : {
				uint8_t cmdString[4] = {0x0};
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0 )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 4 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					for (uint8_t i = 0; i < 4; i++)
					{
						cli(); BufferOut( &UART_deBuf, &cmdString[i] ); sei();
						AP_updateCRC(&cmdString[i]);
					}
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
				if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
				{
					RFmodul.netCMD_dh = tmp;
					if ( RFmodul.serintCMD_ap == 0 ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* DL */	case AT_DL : {
				uint8_t cmdString[4] = {0x0};
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 4 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					for (uint8_t i = 0; i < 4; i++)
					{
						cli(); BufferOut( &UART_deBuf, &cmdString[i] ); sei();
						AP_updateCRC(&cmdString[i]);
					}
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
				if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
				{
					RFmodul.netCMD_dl = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* MY */	case AT_MY : {
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE )	return ERROR;
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.netCMD_my = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* CE */	case AT_CE : { 	
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */								
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
				{
					RFmodul.netCMD_ce = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");			
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* SC */	case AT_SC : { 
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.netCMD_sc = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* MM */	case AT_MM : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x00 && tmp <= 0x3 )
				{
					RFmodul.netCMD_mm = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* RR */	case AT_RR : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
				{
					RFmodul.netCMD_rr = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* RN */	case AT_RN : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x00 && tmp <= 0x3 )
				{
					RFmodul.netCMD_rn = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
			}
			break;
			
/* NT */	case AT_NT : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x01 && tmp <= 0xFC )
				{
					RFmodul.netCMD_nt = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* NO */	case AT_NO : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
				{
					RFmodul.netCMD_no = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
			}
			break;
			
/* SD */	case AT_SD : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
				{
					RFmodul.netCMD_sd = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
			}
			break;
			
/* A1 */	case AT_A1 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
				{
					RFmodul.netCMD_a1 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER; }
			}
			break;
			
/* A2 */	case AT_A2 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
				{
					RFmodul.netCMD_a2 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			/*
			 * AT commands: security
			 *
			 * last modified: 2016/12/02
			 */			
/* KY */	case AT_KY : 
				if ( RFmodul.serintCMD_ap == 0  ) UART_print("Not implemented.\r"); 
				return ERROR;

/* EE */	case AT_EE : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
				{
					RFmodul.secCMD_ee = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			/*
			 * AT commands: RF interface
			 *
			 * last modified: 2016/12/02
			 */
/* PL */	case AT_PL : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x4 )
				{
					RFmodul.rfiCMD_pl = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* CA */	case AT_CA : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x24 && tmp <= 0x50 )
				{
					RFmodul.rfiCMD_ca = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			/*
			 * AT commands: sleep modes
			 *
			 * last modified: 2016/12/02
			 */
/* SM */	case AT_SM : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
				{
					RFmodul.sleepmCMD_sm = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER; }
			}
			break;
			
/* ST */	case AT_ST : { 
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.sleepmCMD_st = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else { return INVALID_PARAMETER; }
			}
			break;
			
/* SP */	case AT_SP : {
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0x68B0 )
				{
					RFmodul.sleepmCMD_sp = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* DP */	case AT_DP : { 
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0x68B0 )
				{
					RFmodul.sleepmCMD_dp = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* SO */	case AT_SO : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
				{
					RFmodul.sleepmCMD_so = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
			}
			break;
			/*
			 * AT commands: serial interfacing
			 *
			 * last modified: 2016/12/02
			 */
/* AP */	case AT_AP : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
				{
					RFmodul.serintCMD_ap = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* BD */	case AT_BD : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x7 )
				{
					RFmodul.serintCMD_bd = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* NB */	case AT_NB : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x4 )
				{
					RFmodul.serintCMD_nb = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* RO */	case AT_RO : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.serintCMD_ro = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			/*
			 * AT commands: IO settings
			 *
			 * last modified: 2016/12/02
			 */
/* D8 */	case AT_D8 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d8 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D7 */	case AT_D7 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d7 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D6 */	case AT_D6 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d6 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D5 */	case AT_D5 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d5 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D4 */	case AT_D4 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d4 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D3 */	case AT_D3 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d3 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D2 */	case AT_D2 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d2 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D1 */	case AT_D1 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d1 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D0 */	case AT_D0 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d0 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* PR */	case AT_PR : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_pr = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* IU */	case AT_IU : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
				{
					RFmodul.ioserCMD_iu = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* IT */	case AT_IT : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x1 && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_it = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* IC */	case AT_IC : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_ic = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* IR */	case AT_IR : {
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.ioserCMD_ir = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* P0 */	case AT_P0 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
				{
					RFmodul.ioserCMD_p0 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* P1 */	case AT_P1 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
				{
					RFmodul.ioserCMD_p1 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* PT */	case AT_PT : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0xB && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_pt = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* RP */	case AT_RP : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_rp = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			/*
			 * AT commands: IO line passing
			 *
			 * last modified: 2016/12/02
			 */
/* IA */	case AT_IA : {
				uint8_t cmdString[8] = {0x0};
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 8 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					for (uint8_t i = 0; i < 8; i++)
					{
						cli(); BufferOut( &UART_deBuf, &cmdString[i] ); sei();
						AP_updateCRC(&cmdString[i]);
					} 
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				uint64_t tmp = (uint64_t) cmdString[0] << 56 | (uint64_t) cmdString[1] << 48 | (uint64_t) cmdString[2] <<  40 | (uint64_t) cmdString[3] << 32 |\
							   (uint64_t) cmdString[4] << 24 | (uint64_t) cmdString[5] << 16 | (uint64_t) cmdString[6] <<   8 | (uint64_t) cmdString[7];
							
				if ( cmdSize <= 8 && tmp >= 0x0 && tmp <= 0xFFFFFFFFFFFFFFFF )
				{
					RFmodul.iolpCMD_ia = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
			}
			break;
			
/* T0 */	case AT_T0 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T0 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T1 */	case AT_T1 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T1 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T2 */	case AT_T2 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T2 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
	
/* T3 */	case AT_T3 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T3 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T4 */	case AT_T4 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T4 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T5 */	case AT_T5 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T5 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T6 */	case AT_T6 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T6 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T7 */	case AT_T7 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				/*
				 * compare the parameter
				 */							
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T7 = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r"); 
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			/*
			 * AT commands: diagnostics
			 *
			 * last modified: 2016/12/02
			 */
/* VR */	case AT_VR : {
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.ioserCMD_ir = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else { return INVALID_PARAMETER; }
			}
			break;
				
/* DD */	case AT_DD : { 
				uint8_t cmdString[4] = {0x0};
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 4 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					for (uint8_t i = 0; i < 4; i++)
					{
						cli(); BufferOut( &UART_deBuf, &cmdString[i] ); sei();
						AP_updateCRC(&cmdString[i]);
					}
					
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}
				
				uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
				if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
				{
					RFmodul.diagCMD_dd = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
				}
				else { return INVALID_PARAMETER; }
			}
			break;
			/*
			 * AT commands: AT command options
			 *
			 * last modified: 2016/12/02
			 */
/* CT */	case AT_CT : { 
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE ) return ERROR; 
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x02 && tmp <= 0x1770 )
				{
					RFmodul.atcopCMD_ct = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else { return INVALID_PARAMETER; }
			}
			break;
			
/* GT */	case AT_GT : { 
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					AP_updateCRC( &cmdString[0] ); AP_updateCRC( &cmdString[1] );
					
					if ( AP_compareCRC() == FALSE ) return ERROR;
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x02 && tmp <= 0xCE4 )
				{
					RFmodul.atcopCMD_gt = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* CC */	case AT_CC : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( TRUE == apFrame )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					AP_updateCRC(&tmp);
					if ( AP_compareCRC() == FALSE ) return ERROR;  
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.atcopCMD_cc = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");	
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* RU */	case DE_RU : {
				// no AP handling
				if ( RFmodul.serintCMD_ap > 0 && TRUE == apFrame ) return ERROR;
				
				uint8_t tmp = 0;
				if (charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				if ( cmdSize <= 1 && tmp >= FALSE && tmp <= TRUE )
				{
					RFmodul.deCMD_ru = tmp;
					if ( RFmodul.serintCMD_ap == 0  ) UART_print("OK\r");
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
	
	return OP_SUCCESS;
}

/*
 * CMD_timeHandle()
 * 
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2016/11/24
 */
uint32_t CMD_timeHandle(uint32_t arg)
{
	noTimeout = FALSE;
	return 0;
}


/*
 * helper function charToUint8()
 * - received the buffer content and converted content to uint8 hex values
 * - the char 'A' will be uint8_t hex 0xA and so on
 * 
 * Received:
 *		uint8_tcmdString		pointer to the string buffer
 *		size_t  *strlength		complete string length
 *		size_t  *cmdSize		final command size 
 *		size_t  *maxCmdSize		maximum length of the string buffer
 *		
 * Returns:
 *     TRUE  on success
 *	   FALSE on failure
 *
 * last modified: 2016/12/02
 */
bool_t charToUint8(uint8_t *cmdString, size_t *strlength, size_t *cmdSize, size_t maxCmdSize)
{
#if DEBUG
	UART_print("== char to int fkt\r\n");
#endif
	uint8_t tmp[2] = {0};	
	/*
	 * this case should not occur: 
	 * if only a '\r' or '\n' left (one byte after the AT command) return false because there is nothing to write
	 */
	if ( 1 >= *strlength ) 
	{
		if ( 1 == *strlength ) { cli(); BufferOut( &UART_deBuf, &tmp[0] ); sei(); } // dummy read to reset the buffer read pointer
		return FALSE;
	}

	int pos = (*cmdSize < maxCmdSize)? maxCmdSize - *cmdSize : 0 , c = 0;
	
	/*
	 * if the len is an uneven number, set the first field to zero
	 * to avoid a false calculation
	 */
	if ( ((*strlength)-1) % 2 == 1 )
	{
		tmp[c] = '0';
		c = 1;
	}
#if DEBUG
	UART_printf("==> Sring length: %u\r\n", *strlength);
	UART_printf("==> put in t[0] a '0'? : %s\r\n", (c==1)? "true" : "false");
#endif
	
	do{
		for(; c < 2; c++)
		{
			cli(); BufferOut( &UART_deBuf, &tmp[c] ); sei();
#if DEBUG
	UART_printf("== char %"PRIX8"\r\n", tmp[c]);
#endif
			if( tmp[c] == 0x20 ) BufferOut( &UART_deBuf, &tmp[c] );	// ' ' == 0x20	ignore spaces
			if( tmp[c] == '\r' || tmp[c] == '\n') return TRUE;		// line break	return true
		}
		/*
		 * if the calculated position size greater than calculated array size return failure
		 * else print the new hex value into the array
		 */
		if ( pos > maxCmdSize ) return FALSE;
		sprintf((char*)(cmdString+pos),"%c", (uint8_t) strtoul( (const char*) tmp, NULL, 16) );
		c = 0;
		pos++;
#if DEBUG
	UART_printf("<< convert tmp[] to hex: %"PRIX8"\r\n", (uint8_t) strtoul( (const char*) tmp, NULL, 16) );
#endif

	}while(TRUE);
}