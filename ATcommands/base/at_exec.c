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
#include "../../ATuracoli/stackrelated.h"		// init functions
#include "../../ATuracoli/stackrelated_timer.h"	// timer function

// === globals ============================================
static uint32_t writetimer;

// === prototypes =========================================
static uint32_t AT_writeEEPROM(uint32_t arg);

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
at_status_t AT_exec(uint32_t *th, cmdIDs cmdID)
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
				writetimer = deTIMER_start(AT_writeEEPROM, 0x10, 0);
			}
			break;

	/* AC - apply changes */
		case AT_AC :
			{
				if ( (DIRTYB_BD & dirtyBits) != FALSE ) { UART_init(); dirtyBits ^= DIRTYB_BD; }
				if ( (DIRTYB_CH & dirtyBits) != FALSE ||\
				     (DIRTYB_ID & dirtyBits) != FALSE ) { TRX_baseInit(); dirtyBits ^= (DIRTYB_CH | DIRTYB_ID); }
				if ( (DIRTYB_AP & dirtyBits) != FALSE ) { SET_serintCMD_ap( GET_atAP_tmp() ); dirtyBits ^= DIRTYB_AP; deBufferReset(UART);}
				if ( (DIRTYB_CT_AC & dirtyBits) != FALSE ) { SET_atcopCMD_ct ( GET_atCT_tmp() ); dirtyBits ^= DIRTYB_CT_AC; }
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
static uint32_t AT_writeEEPROM(uint32_t arg)
{
	SET_userValInEEPROM();
	return FALSE;
}