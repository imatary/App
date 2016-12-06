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
static CMD*		CMD_findInTable(void);
static uint32_t CMD_timeHandle(uint32_t arg);


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
 * last modified: 2016/12/02
 */
void AT_localMode(void)
{
	ATERROR ret       = 0;
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
				else if( counter == 5 ) ret = CMD_readOrExec(NULL, NULL, &th);
				else if( counter >  5 ) ret = CMD_write(NULL, NULL, &counter);
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
 * CMD_findInTable() 
 * - searched in the command table for the command id
 *
 * Returns:
 *	   CMD struct		on success
 *     INVALID_COMMAND	on fail
 *
 * last modified: 2016/11/18
 */
static CMD* CMD_findInTable(void)
{
#if DEBUG
	UART_print("== search AT cmd (AT mode)\r\n");
#endif
	CMD *workPointer = (CMD*) pStdCmdTable;
	uint8_t pCmdString[5] = {0};
	
	for (int i = 0; i < 4 ; i++)
	{
		cli(); BufferOut( &UART_deBuf, &pCmdString[i] ); sei();
	}
	pCmdString[4] = '\0';
#if DEBUG
	UART_printf(">> CMD name: %s\r\n", pCmdString);
#endif
	// TODO -> search parser
	for (int i = 0; i < command_count ; i++, workPointer++)
	{
		if( strncmp( (const char*) pCmdString, workPointer->name, 4 ) == 0 )
		{
			return workPointer;
		}
	}
#if DEBUG
	UART_printf(">> No CMD available, read: %s\r\n", pCmdString);
#endif
	return NO_AT_CMD;
}

/*
 * CMD_readOrExec() 
 * - read the values of the memory
 * - or execute commands
 *
 * Received:
 *		if in API Mode, a frame struct else a NULL pointer
 *		if in API Mode, a pointer to an already allocated array for processing else  a NULL pointer
 *		if in  AT Mode, a time handler pointer else a NULL pointer
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *		ERROR				if an process error occurred
 *
 * last modified: 2016/12/02
 */
ATERROR CMD_readOrExec(struct api_f *frame, uint8_t *array, uint32_t *th) 
{
#if DEBUG
	UART_print("== rx Active\r\n");
	UART_printf("== AP Mode?: %s\r\n", (RFmodul.serintCMD_ap == 0)? "off" : "on" );
#endif
	CMD *pCommand  = NULL;
	
	if ( RFmodul.serintCMD_ap != 0 && frame != NULL ) // API frame
	{
		pCommand = API_findInTable(frame, array);
	} 
	else // AT CMD
	{
		pCommand = CMD_findInTable();
		/*
		 * remove the '\r'/'\n' from the buffer
		 */
		deBufferReadReset( &UART_deBuf, '+', 1); 
	}
	
	/*
	 * if there no valid command leave function
	 */
	if ( NO_AT_CMD == pCommand->ID || NULL == pCommand ) 
	{
#if DEBUG
	UART_print("== Invalide CMD exit!\r\n");
#endif
		return INVALID_COMMAND;
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
#if DEBUG
	UART_printf(">> exec cmd #%u\r\n",pCommand->ID);
#endif
		if ( frame != NULL ) frame->rwx = EXEC;	
		switch( pCommand->ID )
		{
			// leave command mode command
/* CN */    case AT_CN : 
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( th != NULL) *th = deTIMER_stop(*th);					
					noTimeout = FALSE;
					UART_print("OK\r");
				} 
				else
				{
					frame->length = 0;
					return INVALID_COMMAND;
				}
				break;
			
			// write config to firmware
/* WR */    case AT_WR : 
				SET_userValInEEPROM();
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) 
				{
					UART_print("OK\r");
				}
				else
				{
					frame->length = 0;
					return OP_SUCCESS;
				}
				break;
			
			// apply changes - currently only a dummy
/* AC */    case AT_AC : {
				/*
				 * TODO - run config for trx/uart
				 */
				TRX_writeBit(deSR_CHANNEL, RFmodul.netCMD_ch);
				uint32_t baud = deHIF_DEFAULT_BAUDRATE;
				switch( RFmodul.serintCMD_bd )
				{
					case 0x0 : baud =   1200; break;
					case 0x1 : baud =   2400; break;
					case 0x2 : baud =   4800; break;
					case 0x3 : baud =   9600; break;
					case 0x4 : baud =  19200; break;
					case 0x5 : baud =  38400; break
					case 0x6 : baud =  57600; break;
					case 0x7 : baud = 115200; break;
					default : baud = deHIF_DEFAULT_BAUDRATE; break;
				}
				hif_init(baud);
				
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_print("OK\r");
				}
				else
				{
					frame->length = 0;
					return OP_SUCCESS;
				}
			}
			break;
			
			// reset all parameter
/* RE */    case AT_RE : 
				SET_allDefault();
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) 
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
		if ( frame != NULL ) frame->rwx = READ;	
#if DEBUG
	UART_printf(">> read cmd #%u\r\n",pCommand->ID);
#endif
		switch( pCommand->ID )
		{
/* CH */	case AT_CH : 
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_ch); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_ch;
				}
				break;
			
/* ID */	case AT_ID :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_id);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.netCMD_id >> down);
						frame->length = up+1;
					}
				}  
				break;
			
/* DH */	case AT_DH :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_dh);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 24; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.netCMD_dh >> down);
						frame->length = up+1;
					}	
				}  
				break;
			
/* DL */	case AT_DL :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX32"\r", RFmodul.netCMD_dl);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 24; down >= 0; down -= 8, up+=1 )
					{
						frame->msg[up] = (uint8_t) (RFmodul.netCMD_dl >> down);
						frame->length = up+1;
					}
				}  
				break;
			
/* MY */	case AT_MY :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_my);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.netCMD_my >> down);
						frame->length = up+1;
					}
				}  
				break;
			
/* SH */	case AT_SH :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_sh);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 24; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.netCMD_sh >> down);
						frame->length = up+1;
					}
				}  
				break;
			
/* SL */	case AT_SL :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX32"\r",RFmodul.netCMD_sl);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 24; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.netCMD_sl >> down);
						frame->length = up+1;
					}
				}  
				break;
			
/* CE */	case AT_CE :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%d\r",       RFmodul.netCMD_ce);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_ce;
				}  
				break;
			
/* SC */	case AT_SC :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.netCMD_sc);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.netCMD_sc >> down);
						frame->length = up+1;
					}
				}  
				break;
			
/* NI */	case AT_NI :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%s\r", RFmodul.netCMD_ni); 
				}
				else if ( frame != NULL )
				{
					frame->length = strlen( (const char*) RFmodul.netCMD_ni)+1;
					memcpy(frame->msg, &RFmodul.netCMD_ni, strlen( (const char*) RFmodul.netCMD_ni)+1 );
				}
				break;
			
/* MM */	case AT_MM :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_mm);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_mm;
				}  
				break;
			
/* RR */	case AT_RR :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_rr); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_rr;
				} 
				break;
			
/* RN */	case AT_RN :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_rn);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_rn;
				}  
				break;
			
/* NT */	case AT_NT :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_nt);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_nt;
				} 
				break;
			
/* NO */	case AT_NO :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%d\r", RFmodul.netCMD_no);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_no;
				} 
				break;
			
/* SD */	case AT_SD :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_sd);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_sd;
				}  
				break;
			
/* A1 */	case AT_A1 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_a1);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_a1;
				}  
				break;
			
/* A2 */	case AT_A2 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_a2);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_a2;
				}  
				break;
			
/* AI */	case AT_AI :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.netCMD_ai);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.netCMD_ai;
				}  
				break;

/* EE */	case AT_EE :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%d\r",       RFmodul.secCMD_ee); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.secCMD_ee;
				} 
				break;
			
/* KY */	case AT_KY :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_print("\r");
				}
				break;

/* PL */	case AT_PL :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.rfiCMD_pl);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.rfiCMD_pl;
				}  
				break;
			
/* CA */	case AT_CA :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.rfiCMD_ca);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.rfiCMD_ca;
				}  
				break;

/* SM */	case AT_SM :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.sleepmCMD_sm);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.sleepmCMD_sm;
				}  
				break;
			
/* CH */	case AT_ST :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_st);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.sleepmCMD_st >> down);
						frame->length = up+1;
					}
				}  
				break;
			
/* SP */	case AT_SP :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.sleepmCMD_sp); 
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.sleepmCMD_sp >> down);
						frame->length = up+1;
					}
				} 
				break;
			
/* DP */	case AT_DP :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r", RFmodul.sleepmCMD_dp);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.sleepmCMD_dp >> down);
						frame->length = up+1;
					}
				}  
				break;
			
/* SO */	case AT_SO :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.sleepmCMD_so);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.sleepmCMD_so;
				}  
				break;
			
/* SS */	case AT_SS : 
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					 UART_print("ERROR\r");
				} 
				else if ( frame != NULL )
				{
					frame->length = 0;
					return ERROR;
				}
				break;
				
/* AP */	case AT_AP : 
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.serintCMD_ap);
				} 
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.serintCMD_ap;
				}
				break;
			
/* BD */    case AT_BD :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.serintCMD_bd);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.serintCMD_bd;
				} 
				break;
			
/* NB */    case AT_NB :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.serintCMD_nb);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.serintCMD_nb;
				}  
				break;
				
/* RO */	case AT_RO :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					 UART_printf("%"PRIX8"\r",RFmodul.serintCMD_ro);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.serintCMD_ro;
				} 
				break;

/* D8 */	case AT_D8 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d8); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d8;
				} 
				break;
			
/* D7 */	case AT_D7 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d7); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d7;
				} 
				break;
			
/* D6 */	case AT_D6 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d6); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d6;
				} 
				break;
			
/* D5 */	case AT_D5 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d5); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d5;
				} 
				break;
			
/* D4 */	case AT_D4 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d4); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d4;
				} 
				break;
			
/* D3 */	case AT_D3 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d3); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d3;
				} 
				break;
			
/* D2 */	case AT_D2 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d2);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d2;
				} 
				break;
			
/* D1 */	case AT_D1 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d1); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d1;
				} 
				break;
			
/* D0 */	case AT_D0 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_d0); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_d0;
				} 
				break;
			
/* PR */	case AT_PR :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_pr);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_pr;
				}
				break;
			
/* IU */	case AT_IU :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%d\r", RFmodul.ioserCMD_iu);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_iu;
				} 
				break;
			
/* IT */	case AT_IT :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_it);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_it;
				} 
				break;
			
/* IC */	case AT_IC :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_ic);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_ic;
				} 
				break;
			
/* IR */	case AT_IR :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r", RFmodul.ioserCMD_ir); 
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.ioserCMD_ir >> down);
						frame->length = up+1;
					}
				} 
				break;
			
/* P0 */	case AT_P0 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_p0);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_p0;
				} 
				break;
			
/* P1 */	case AT_P1 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_p1); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_p1;
				} 
				break;
			
/* PT */	case AT_PT :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_pt); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_pt;
				} 
				break;
			
/* RP */	case AT_RP :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.ioserCMD_rp); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.ioserCMD_rp;
				} 
				break;

/* IA */	case AT_IA :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					// the compiler don't like the PRIX64 command
					uint32_t a = RFmodul.iolpCMD_ia >> 32;
					uint32_t b = RFmodul.iolpCMD_ia & 0xFFFFFFFF;
					UART_printf("%"PRIX32"%"PRIX32"\r",a,b);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 56; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.iolpCMD_ia >> down);
						frame->length = up+1;
					}
				}
				break;
			
/* T0 */	case AT_T0 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T0); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.iolpCMD_T0;
				} 
				break;
			
/* T1 */	case AT_T1 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T1); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.iolpCMD_T1;
				}
				break;
			
/* T2 */	case AT_T2 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T2); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.iolpCMD_T2;
				}
				break;
			
/* T3 */	case AT_T3 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T3); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.iolpCMD_T3;
				}
				break;
			
/* T4 */	case AT_T4 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T4); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.iolpCMD_T4;
				}
				break;
			
/* T5 */	case AT_T5 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T5); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.iolpCMD_T5;
				}
				break;
			
/* T6 */	case AT_T6 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T6);
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.iolpCMD_T6;
				}  
				break;
			
/* T7 */	case AT_T7 :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r",RFmodul.iolpCMD_T7); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.iolpCMD_T7;
				} 
				break;

/* VR */	case AT_VR :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_vr);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.diagCMD_vr >> down);
						frame->length = up+1;
					}
				} 
				break;
			
/* HV */	case AT_HV :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_hv);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.diagCMD_hv >> down);
						frame->length = up+1;
					}
				} 
				break;
			
/* DB */	case AT_DB :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.diagCMD_db); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.diagCMD_db;
				} 
				break;
			
/* EC */	case AT_EC :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_ec);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.diagCMD_ec >> down);
						frame->length = up+1;
					}
				} 
				break;
			
/* EA */	case AT_EA :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.diagCMD_ea); 
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.diagCMD_ea >> down);
						frame->length = up+1;
					}
				} 
				break;
			
/* DD */	case AT_DD :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX32"\r",RFmodul.diagCMD_dd);
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 24; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.diagCMD_dd >> down);
						frame->length = up+1;
					}
				}  
				break;

/* CT */	case AT_CT :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.atcopCMD_ct); 
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.atcopCMD_ct >> down);
						frame->length = up+1;
					}
				} 
				break;
			
/* GT */	case AT_GT :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX16"\r",RFmodul.atcopCMD_gt); 
				}
				else if ( frame != NULL )
				{
					for (char up = 0, down = 8; down >= 0; down -= 8, up++ )
					{
						frame->msg[up] = (uint8_t) (RFmodul.atcopCMD_gt >> down);
						frame->length = up+1;
					}
				} 
				break;
			
/* CC */	case AT_CC :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.atcopCMD_cc); 
				}
				else if ( frame != NULL )
				{
					frame->length = 1;
					frame->msg[0] = RFmodul.atcopCMD_cc;
				} 
				break;
			
/* Rq */	case AT_Rq :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_print("ERROR\r");
				}
				else if ( frame != NULL )
				{
					frame->length = 0;
					return ERROR;
				} 
				break;
			
/* pC*/	case AT_pC :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_print("1\r"); 
				}
				else if ( frame != NULL )
				{
					frame->msg[0] = 0x01;
					frame->length = 1;
				}
				break;
			
/* SB */	case AT_SB :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_print("ERROR\r"); 
				}
				else if ( frame != NULL )
				{
					frame->length = 0;
					return ERROR;
				} 
				break;
			
/* RU */	case DE_RU :
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					UART_printf("%"PRIX8"\r", RFmodul.deCMD_ru);
				}
				// No API handle at this place -> check API_0x18_localDevice function
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
 *		if in API Mode, a frame struct else a NULL pointer
 *		if in API Mode, a pointer to an already allocated array for processing else  a NULL pointer
 *		if in  AT Mode, a time handler pointer else a NULL pointer
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *		INVALID_PARAMETER	if the delivered parameter is not valid
 *		ERROR				if an process error occurred
 *				 
 * last modified: 2016/12/02
 */				 
ATERROR CMD_write(struct api_f *frame, uint8_t *array, size_t *len)
{
	CMD *pCommand  = NULL;
	size_t cmdSize = 0;
	
	if ( RFmodul.serintCMD_ap != 0 && frame != NULL ) // API frame
	{
		cmdSize = frame->length-4;
		pCommand = API_findInTable(frame, array);
		frame->rwx = WRITE;	
	} 
	else // AT CMD
	{
		cmdSize = (((*len-1) % 2) + (*len-5))/2;
		pCommand = CMD_findInTable();
	}
	
	/*
	 * if there no valid command leave function
	 */
	if ( INVALID_COMMAND == pCommand->ID || NULL == pCommand ) return INVALID_COMMAND;

#if DEBUG
	UART_printf(">> write cmd #%u\r\n",pCommand->ID);
#endif
	
	/* 
	 * special handle if
	 * - network identifier string command
	 * - buffer content <= 20 characters
	 */
	*len -= (RFmodul.serintCMD_ap == 0)? 4 : 15;
	if ( AT_NI == pCommand->ID && *len <= 20 )
	{
		for (int i = 0; i < (*len); i++)
		{
			cli(); BufferOut(&UART_deBuf, &RFmodul.netCMD_ni[i]); sei();
			if ( frame != NULL ) frame->crc -= RFmodul.netCMD_ni[i];
		}	
		RFmodul.netCMD_ni[(*len)+1] = 0x0;
		if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
		
		if ( frame != NULL )
		{
			frame->crc -= 0x0;
			if ( API_compareCRC(frame) == FALSE )
			{
				if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
				return ERROR;
			}
			if( frame->type == 0x8 ) SET_userValInEEPROM();
		}
		return OP_SUCCESS;
	}
	if (AT_NI == pCommand->ID && *len > 20 )
	{
		if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("Please insert max 20 characters!\r");
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0B && tmp <= 0x1A )
				{
					TRX_writeBit(deSR_CHANNEL, tmp);
					RFmodul.netCMD_ch = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.netCMD_id = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 4 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					for (uint8_t i = 0; i < 4; i++)
					{
						cli(); BufferOut( &UART_deBuf, &cmdString[i] ); sei();
						frame->crc -=  cmdString[0];
					}
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
					if( frame->type == 0x8 ) SET_userValInEEPROM();
				}
				
				uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
				if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
				{
					RFmodul.netCMD_dh = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 4 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					for (uint8_t i = 0; i < 4; i++)
					{
						cli(); BufferOut( &UART_deBuf, &cmdString[i] ); sei();
						frame->crc -=  cmdString[0];
					}
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
				if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
				{
					RFmodul.netCMD_dl = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.netCMD_my = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */								
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
				{
					RFmodul.netCMD_ce = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.netCMD_sc = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x00 && tmp <= 0x3 )
				{
					RFmodul.netCMD_mm = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
				{
					RFmodul.netCMD_rr = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x00 && tmp <= 0x3 )
				{
					RFmodul.netCMD_rn = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x01 && tmp <= 0xFC )
				{
					RFmodul.netCMD_nt = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
				{
					RFmodul.netCMD_no = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
				{
					RFmodul.netCMD_sd = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
				{
					RFmodul.netCMD_a1 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
			}
			break;
			
/* A2 */	case AT_A2 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xF )
				{
					RFmodul.netCMD_a2 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("Not implemented.\r"); 
				return ERROR;

/* EE */	case AT_EE : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
				{
					RFmodul.secCMD_ee = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x4 )
				{
					RFmodul.rfiCMD_pl = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* CA */	case AT_CA : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x24 && tmp <= 0x50 )
				{
					RFmodul.rfiCMD_ca = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
				{
					RFmodul.sleepmCMD_sm = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else
{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* ST */	case AT_ST : { 
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.sleepmCMD_st = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* SP */	case AT_SP : {
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0x68B0 )
				{
					RFmodul.sleepmCMD_sp = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0x68B0 )
				{
					RFmodul.sleepmCMD_dp = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x6 )
				{
					RFmodul.sleepmCMD_so = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				#if DEBUG
					UART_printf("<< write: %"PRIX8"\r\n", tmp);
				#endif
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
				{
					RFmodul.serintCMD_ap = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* BD */	case AT_BD : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x7 )
				{
					RFmodul.serintCMD_bd = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* NB */	case AT_NB : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x4 )
				{
					RFmodul.serintCMD_nb = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* RO */	case AT_RO : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.serintCMD_ro = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d8 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D7 */	case AT_D7 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d7 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D6 */	case AT_D6 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d6 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D5 */	case AT_D5 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d5 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D4 */	case AT_D4 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d4 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D3 */	case AT_D3 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d3 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D2 */	case AT_D2 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d2 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D1 */	case AT_D1 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d1 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* D0 */	case AT_D0 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x5 )
				{
					RFmodul.ioserCMD_d0 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* PR */	case AT_PR : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_pr = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* IU */	case AT_IU : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x1 )
				{
					RFmodul.ioserCMD_iu = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* IT */	case AT_IT : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x1 && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_it = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* IC */	case AT_IC : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_ic = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* IR */	case AT_IR : {
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x0 && tmp <= 0xFFFF )
				{
					RFmodul.ioserCMD_ir = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
				{
					RFmodul.ioserCMD_p0 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* P1 */	case AT_P1 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0x2 )
				{
					RFmodul.ioserCMD_p1 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* PT */	case AT_PT : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0xB && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_pt = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* RP */	case AT_RP : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.ioserCMD_rp = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 8 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					for (uint8_t i = 0; i < 8; i++)
					{
						cli(); BufferOut( &UART_deBuf, &cmdString[i] ); sei();
						frame->crc -=  cmdString[0];
					}
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				uint64_t tmp = (uint64_t) cmdString[0] << 56 | (uint64_t) cmdString[1] << 48 | (uint64_t) cmdString[2] <<  40 | (uint64_t) cmdString[3] << 32 |\
							   (uint64_t) cmdString[4] << 24 | (uint64_t) cmdString[5] << 16 | (uint64_t) cmdString[6] <<   8 | (uint64_t) cmdString[7];
							
				if ( cmdSize <= 8 && tmp >= 0x0 && tmp <= 0xFFFFFFFFFFFFFFFF )
				{
					RFmodul.iolpCMD_ia = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T0 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T1 */	case AT_T1 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T1 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T2 */	case AT_T2 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T2 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
	
/* T3 */	case AT_T3 : { 
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T3 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T4 */	case AT_T4 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T4 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T5 */	case AT_T5 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T5 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T6 */	case AT_T6 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T6 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* T7 */	case AT_T7 : {
				uint8_t tmp = 0;
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				/*
				 * compare the parameter
				 */							
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.iolpCMD_T7 = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
					
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			/*
			 * AT commands: diagnostics
			 *
			 * last modified: 2016/12/02
			 */
/* DD */	case AT_DD : { 
				uint8_t cmdString[4] = {0x0};
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 4 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					for (uint8_t i = 0; i < 4; i++)
					{
						cli(); BufferOut( &UART_deBuf, &cmdString[i] ); sei();
						frame->crc -=  cmdString[0];
					}
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}
				
				uint32_t tmp = (uint32_t) cmdString[0] << 24 | (uint32_t) cmdString[1] << 16 | (uint32_t) cmdString[2] <<   8 | (uint32_t) cmdString[3];
							
				if ( cmdSize <= 4 && tmp >= 0x0 && tmp <= 0xFFFFFFFF )
				{
					RFmodul.diagCMD_dd = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x02 && tmp <= 0x1770 )
				{
					RFmodul.atcopCMD_ct = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
				}
				else 
				{ 
					return INVALID_PARAMETER; 
				}
			}
			break;
			
/* GT */	case AT_GT : { 
				uint8_t cmdString[2] = {0x0};
				
				/*
				 * get the parameter
				 */
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &cmdString[0], len, &cmdSize, 2 ) == FALSE ) return ERROR;
				}
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &cmdString[0] ); sei();
					cli(); BufferOut( &UART_deBuf, &cmdString[1] ); sei();
					
					frame->crc -=  ( cmdString[0] + cmdString[1]);
					
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
				}			
				uint16_t tmp = (uint16_t) cmdString[0] << 8 | cmdString[1];
				
				/*
				 * compare the parameter
				 */		
				if ( cmdSize <= 2 && tmp >= 0x02 && tmp <= 0xCE4 )
				{
					RFmodul.atcopCMD_gt = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
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
				if ( RFmodul.serintCMD_ap == 0  || frame == NULL )
				{
					if ( charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				} 
				else if ( frame != NULL )
				{
					cli(); BufferOut( &UART_deBuf, &tmp ); sei();
					frame->crc -= tmp;
					if ( API_compareCRC(frame) == FALSE )
					{
						if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
						return ERROR;
					}
					if( frame->type == 0x8 ) SET_userValInEEPROM();
				}
				
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && tmp >= 0x0 && tmp <= 0xFF )
				{
					RFmodul.atcopCMD_cc = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");	
				}
				else { return INVALID_PARAMETER;}
			}
			break;
			
/* RU */	case DE_RU : {
				// no API handling
				if ( RFmodul.serintCMD_ap > 0 && frame != NULL ) return ERROR;
				
				uint8_t tmp = 0;
				if (charToUint8( &tmp, len, &cmdSize, 1 ) == FALSE ) return ERROR;
				if ( cmdSize <= 1 && tmp >= FALSE && tmp <= TRUE )
				{
					RFmodul.deCMD_ru = tmp;
					if ( RFmodul.serintCMD_ap == 0  || frame == NULL ) UART_print("OK\r");
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
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