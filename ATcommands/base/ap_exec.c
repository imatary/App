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
#include "../header/ap_frames.h"			// AP set function
#include "../header/execute.h"
#include "../../ATuracoli/stackrelated.h"	// init functions
#include "../../ATuracoli/stackrelated_timer.h"

// === globals ============================================
static uint32_t writetimer = 0;

// === prototypes =========================================
static uint32_t AP_write_timedEEPROM(uint32_t arg);

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
			writetimer = deTIMER_start(AP_write_timedEEPROM, 0x05, 0);
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



/*
 * timer for writing operation
 *
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2017/03/03
 */
static uint32_t AP_write_timedEEPROM(uint32_t arg)
{
	SET_userValInEEPROM();
	return FALSE;
}
