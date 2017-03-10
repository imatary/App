/*
 * execute.c
 *
 * Created: 10.03.2017 12:56:11
 *  Author: TOE
 */
// === includes ===========================================
#include "../header/_global.h"
#include "../header/rfInterfacing.h"
#include "../header/rfmodul.h"
#include "../../ATuracoli/stackrelated.h"
#include "../../ATuracoli/stackrelated_timer.h"

// === functions ==========================================
/*
 * this function apply changed parameter values
 *
 * Received:
 *		uint16_t	if in  AT Mode, a time handler else NULL pointer
 *
 * Returns:
 *     nothing
 *
 * last modified: 2017/03/10
 */
void apply_changes(uint16_t *th)
{
	/*
	 * reinitialize user interface
	 */
	if ( (DIRTYB_BD & dirtyBits) != FALSE )
	{
		UART_init();
		dirtyBits ^= DIRTYB_BD;
	}

	/*
	 * reinitialize transceiver
	 */
	if ( (DIRTYB_CH & dirtyBits) != FALSE ||\
	     (DIRTYB_ID & dirtyBits) != FALSE )
	{
		TRX_baseInit();
		dirtyBits ^= (DIRTYB_CH | DIRTYB_ID);

		if ( (DIRTYB_PL & dirtyBits) != FALSE ) dirtyBits ^= DIRTYB_PL;
	}

	/*
	 * reinitialize transceiver power level
	 */
	if ( (DIRTYB_PL & dirtyBits) != FALSE )
	{
		CMD *cmd = CMD_findInTableByID(AT_PL);
		uint8_t pwl;
		GET_deviceValue( (uint8_t*) &pwl, cmd);

		config_powerlevel(pwl);
		dirtyBits ^= DIRTYB_PL;
	}

	/*
	 * set command timeout
	 */
	if ( (DIRTYB_CT_AC & dirtyBits) == DIRTYB_CT_AC )
	{
		SET_atcopCMD_ct ( GET_atCT_tmp() );
		dirtyBits ^= DIRTYB_CT_AC;
	}

	/*
	 * - change configuration mode (AT <--> API)
	 * - set configuration mode
	 * - reset UART buffer
	 * - set and reset dirty bits
	 *   - AP -> 0
	 *   - RO -> 1
	 * - if AP value not equal to zero, leave AT command mode immediately
	 */
	if ( (DIRTYB_AP & dirtyBits) == DIRTYB_AP )
	{
		SET_serintCMD_ap( GET_atAP_tmp() );
		deBufferReset(UART);

		dirtyBits ^= DIRTYB_AP;
		if ( (dirtyBits & DIRTYB_RO) == 0 ) dirtyBits ^= DIRTYB_RO;

		if ( GET_atAP_tmp() != 0 && th != NULL) *th = deTIMER_restart(*th, deMSEC( 0x5 ));
	}
}
