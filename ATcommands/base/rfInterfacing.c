/*
 * rfInterfacing.c
 *
 * Created: 10.03.2017 08:00:24
 *  Author: TOE
 */
// === includes ===========================================
#include "../../ATuracoli/stackrelated.h"

// === defines ============================================
// Register
#define PHY_TX_PWR_SUB 0x5,0xf,0

// Values
#define MIN_TX_OUTPUT_POWER 0xF
#define MIN_OQPSK250   (0x33)
#define LOW_OQPSK500   (0x94)
#define HIGH_OQPSK1000 (0x34)
#define MAX_OQPSK2000  (0x54)

// === functions ==========================================
/*
 * this function configures the TX output power level and data rate
 *
 * Returns:
 *     nothing
 *
 * last modified: 2017/03/10
 */
void config_powerlevel(uint8_t option)
{
	TRX_writeBit(PHY_TX_PWR_SUB, (-5) * option + MIN_TX_OUTPUT_POWER); // TX output power

	switch( option )
	{
		case 0x0 :  TRX_initDatarate( MIN_OQPSK250   ); break;
		case 0x1 :  TRX_initDatarate( LOW_OQPSK500   ); break;
		case 0x2 :  TRX_initDatarate( HIGH_OQPSK1000 ); break;
		case 0x3 :  TRX_initDatarate( MAX_OQPSK2000  ); break;
	}
}