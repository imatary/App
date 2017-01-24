/*
 * cmd_read_exec.c
 *
 * Created: CMD_print( 18.01.2017 13: CMD_print(07: CMD_print(13
 *  Author: CMD_print( TOE
 */ 
// === includes ===========================================
#include <inttypes.h>

#include "../header/_global.h"
#include "../header/cmd.h"						// AT command search parser
#include "../header/apiframe.h"					// AP set functions
#include "../header/rfmodul.h"					// RFmodul struct
#include "../../ATuracoli/stackrelated.h"		// uart functions
#include "../../ATuracoli/stackrelated_timer.h"	// timer function

// === prototypes =========================================
static void CMD_print(size_t len, uint64_t value);

// === functions ==========================================
/*
 * CMD_readOrExec() 
 * - read the values of the memory
 * - or execute commands
 *
 * Received: CMD_print(
 *		if in  AT Mode, a time handler pointer else a NULL pointer
 *
 * Returns: CMD_print(
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *		ERROR				if an process error occurred
 *
 * last modified: CMD_print( 2017/01/18
 */
at_status_t CMD_readOrExec(uint32_t *th, bufType_n bufType) 
{
	CMD *pCommand  = NULL;
	uint8_t pCmdString[5] = {'A','T',0,0,0};
		
	if ( TRANSPARENT_MODE != GET_serintCMD_ap() ) // AP frame
	{ 
		deBufferOut( bufType, &pCmdString[2] );
		deBufferOut( bufType, &pCmdString[3] );
		if ( 'a' <= pCmdString[2] && 'z' >= pCmdString[2] ) pCmdString[2] -= 0x20;
		if ( 'a' <= pCmdString[3] && 'z' >= pCmdString[3] ) pCmdString[3] -= 0x20;
		AP_setATcmd(pCmdString);
	}
	else // AT CMD
	{
		for (int i = 0; i < 4 ; i++)
		{
			deBufferOut( bufType, &pCmdString[i] );
			if ( 'a' <= pCmdString[i] && 'z' >= pCmdString[i] ) pCmdString[i] -= 0x20;
		}
		// remove the '\0' from the buffer if in AT cmd mode
		deBufferReadReset( bufType, '+', 1);
	}
	
	/*
	 * if there not a valid command, leave function
	 */
	pCommand = CMD_findInTable(pCmdString);
	if ( NO_AT_CMD == pCommand->ID || NULL == pCommand ) return INVALID_COMMAND;
		
	/*
	 * handle CMD
	 * 
	 * exec is allowed
	 */
	if ( pCommand->rwxAttrib & EXEC )
	{
		if ( TRANSPARENT_MODE != GET_serintCMD_ap() ) AP_setRWXopt(EXEC);	
		switch( pCommand->ID )
		{
			// leave command mode command (only in AT mode)
/* CN */    case AT_CN :
				if ( TRANSPARENT_MODE == GET_serintCMD_ap() )
				{
					if ( th != NULL) *th = deTIMER_restart(*th, deMSEC( 0x10 ));
					UART_print_status(OP_SUCCESS);
				} 
				else { return INVALID_COMMAND; }
				break;
			
			// write config to firmware
/* WR */    case AT_WR : 
				if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
				SET_userValInEEPROM();
			break;
			
			// apply changes - currently only a dummy
/* AC */    case AT_AC : 
			{
				UART_init();
				TRX_baseInit();	
				SET_serintCMD_ap( GET_atAP_tmp() );
				SET_atcopCMD_ct ( GET_atCT_tmp() );
	
				if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
			}
			break;
			
			// reset all parameter
/* RE */    case AT_RE :
				if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
				SET_allDefault();
			break;
				
/* KY */	case AT_KY : if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print("\r"); } break;
			
			default: return INVALID_COMMAND;
		}
	}
	/*
	 * frame length
	 * string length of input
	 * reading the command is allowed
	 */		
	else if ( pCommand->rwxAttrib & READ )
	{
		if ( TRANSPARENT_MODE != GET_serintCMD_ap() ) AP_setRWXopt(READ);	

		switch( pCommand->ID )
		{	
/* pC*/		case AT_pC : CMD_print( 1,                0x1 ); break;	

/* SS */	case AT_SS :
/* Rq */	case AT_Rq : 	
/* SB */	case AT_SB : return ERROR;

/* CE */	case AT_CE :
					if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_decimal( GET_netCMD_ce() ); }
					else
					{
						uint8_t val = GET_netCMD_ce();
						AP_setMSG( &val, 1 );
					}
				break;

/* NO */	case AT_NO :
					if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_decimal( GET_netCMD_no() ); }
					else
					{
						uint8_t val = GET_netCMD_no();
						AP_setMSG( &val, 1 );
					}
				break;

/* EE */	case AT_EE :
					if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_decimal( GET_secCMD_ee() ); }
					else
					{
						uint8_t val = GET_secCMD_ee();
						AP_setMSG( &val, 1 );
					}
				break;

/* IU */	case AT_IU :
					if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_decimal( GET_ioserCMD_iu() ); }
					else
					{
						uint8_t val = GET_ioserCMD_iu();
						AP_setMSG( &val, 1 );
					}
				break;

/* AP */	case AT_AP :
					{
						CMD_print( 1, GET_serintCMD_ap() );
						uint8_t tmp = GET_serintCMD_ap();
						SET_atAP_tmp( &tmp, 1 );
					}
				break;

/* CT */	case AT_CT : 
					{
						CMD_print( 2, GET_atcopCMD_ct() );
						uint16_t tmp = GET_atcopCMD_ct();
						SET_atCT_tmp( &tmp, 2 );
					}
				break;

/* NI */	case AT_NI :
					if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_printf("%s\r", GET_netCMD_ni() ); }
					else          { AP_setMSG( GET_netCMD_ni(), strlen( (const char*) GET_netCMD_ni() ) ); }
				break;

/* FV */	case DE_FV : 
					if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_printf("%s\r", AT_VERSION); }
				         /* No AP handle at this place -> check AP_localDevice function */
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
 * CMD print send data to
 * - UART if AT mode is active
 * - API frame message buffer
 *
 * Received:
 *		uint8_t		length of transmitted data (in bytes)
 *		uint64_t	data value
 *		bool_t		boolean value whether the transmitted data need to be swapped before printed
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/19
 */
static void CMD_print( size_t len, uint64_t value)
{
	if ( TRANSPARENT_MODE == GET_serintCMD_ap() )
	{
		UART_print_data( len, value );
	}
	else
	{
		AP_setMSG( &value, len );
	}
}