/*
 * ap_exec.c
 *
 * Created: 26.01.2017 11:25:39
 *  Author: TOE
 */
// === includes ===========================================
#include "../header/_global.h"				// TRANSPARENT_MODE = 0
#include "../header/cmd.h"					// EXEC definition
#include "../header/enum_cmd.h"				// cmd ID definition
#include "../header/rfmodul.h"				// RFmodul GET_ and SET_ functions
#include "../header/apiframe.h"				// AP set function
#include "../../ATuracoli/stackrelated.h"	// init functions

// === function ===========================================
/*
 * Command exec function executes commands
 *
 * Received:
 *		cmdIDs		command ID
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *
 * last modified: 2017/01/26
 */
at_status_t AP_exec( cmdIDs cmdID )
{
	SET_apFrameRWXopt(EXEC);

	switch ( cmdID )
	{
	/* WR - write config to firmware */
	case AT_WR :
		{
			SET_userValInEEPROM();
		}
		break;

	/* AC - apply changes */
	case AT_AC :
		{
			if ( DIRTYB_BD & dirtyBits ) { UART_init(); dirtyBits ^= DIRTYB_BD; }
			if ( DIRTYB_CH & dirtyBits ||\
			     DIRTYB_ID & dirtyBits ) { TRX_baseInit(); dirtyBits ^= (DIRTYB_CH | DIRTYB_ID); }
			if ( DIRTYB_AP & dirtyBits ) { SET_serintCMD_ap( GET_atAP_tmp() ); dirtyBits ^= DIRTYB_AP; }
			if ( DIRTYB_CT & dirtyBits ) { SET_atcopCMD_ct ( GET_atCT_tmp() ); dirtyBits ^= DIRTYB_CT; }
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
			/* do nothing */
		}
		break;

	default:
		{
			return INVALID_COMMAND;
		}
	}

	return OP_SUCCESS;
}