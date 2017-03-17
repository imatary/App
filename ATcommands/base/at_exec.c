/*
 * cmd_exec.c
 *
 * Created: 25.01.2017 08:11:34
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>

#include "../header/enum_cmd.h"					// cmd ID definition
#include "../header/rfmodul.h"					// RFmodul GET_ and SET_ functions
#include "../header/execute.h"					// apply changes function
#include "../../ATuracoli/stackrelated.h"		// UART print function
#include "../../ATuracoli/stackrelated_timer.h"	// timer function

// === function ===========================================
/*
 * Command exec function executes commands
 *
 * Received:
 *		uint16_t	if in  AT Mode, a time handler else NULL pointer
 *		cmdIDs		command ID
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *
 * last modified: 2017/03/16
 */
at_status_t AT_exec(uint16_t *th, cmdIDs cmdID)
{
	switch ( cmdID )
	{
	/* CN - leave command mode command (only in AT mode) */
		case AT_CN :
			{
				if ( th != NULL) *th = deTIMER_restart(*th, deMSEC( 0x5 ));
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
				apply_changes(th);
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
				UART_print("\r");
			}
			break;

		default: return INVALID_COMMAND;
	}

	return OP_SUCCESS;
}