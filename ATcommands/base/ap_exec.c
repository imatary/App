/*
 * ap_exec.c
 *
 * Created: 26.01.2017 11:25:39
 *  Author: TOE
 */
// === includes ===========================================
#include "../header/cmd.h"					// EXEC definition
#include "../header/enum_cmd.h"				// cmd ID definition
#include "../header/rfmodul.h"				// RFmodul set user values in EEPROM and set default values functions
#include "../header/ap_frames.h"			// Set API Frame RWX option function
#include "../header/execute.h"				// apply changes function

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
 * last modified: 2017/03/16
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
			apply_changes(NULL);
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