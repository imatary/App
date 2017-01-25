/*
 * cmd_exec.c
 *
 * Created: 25.01.2017 08:11:34
 *  Author: TOE
 */ 
#include <inttypes.h>

#include "../header/_global.h"					// TRANSPARENT_MODE = 0
#include "../header/cmd.h"
#include "../header/enum_cmd.h"					// cmd ID definition
#include "../header/rfmodul.h"					// RFmodul GET_ and SET_ functions
#include "../header/apiframe.h"					// AP set functions
#include "../../ATuracoli/stackrelated.h"		// init functions
#include "../../ATuracoli/stackrelated_timer.h"	// timer function

// === function ===========================================
/*
 * Command exec function executes commands
 *
 * Received:
 *		uint32_t	if in  AT Mode, a time handler else NULL pointer
 *		cmdIDs		command ID
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *
 * last modified: 2017/01/25
 */
at_status_t CMD_exec(uint32_t *th, cmdIDs cmdID) 
{
	if ( TRANSPARENT_MODE != GET_serintCMD_ap() ) AP_setRWXopt(EXEC);
	
	switch ( cmdID )
	{
	/* CN - leave command mode command (only in AT mode) */
	case AT_CN :
		{
			if ( devMode == GET_serintCMD_ap() )
			{
				if ( th != NULL) *th = deTIMER_restart(*th, deMSEC( 0x5 ));
			}
			else { return INVALID_COMMAND; }
		}
		break;
	
	/* WR - write config to firmware */
	case AT_WR :
		{
			SET_userValInEEPROM();
		}	
		break;
	
	/* AC - apply changes */
	case AT_AC :
		{
			UART_init();
			TRX_baseInit();
			SET_serintCMD_ap( GET_atAP_tmp() );
			SET_atcopCMD_ct ( GET_atCT_tmp() );
		}
		break;
	
	/* RE reset all parameter */
	case AT_RE :
		{
			SET_allDefault();
		}
		break;
	
	/* KY */	
	case AT_KY : 
		{
			if ( devMode == GET_serintCMD_ap() ) { UART_print("\r"); } 
		}
		break;
	
	default: return INVALID_COMMAND;
	}
		
	if ( devMode == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
		
	return OP_SUCCESS;
}