/*
 * at_local.c
 *
 * Created: 10.11.2016 13:14:32
 *  Author: TOE
 */
#include <inttypes.h>							// PRIX8/16/32
#include <stdlib.h>								// size_t, strtol
#include <string.h>

#include "../header/_global.h"					// boolean values
#include "../header/atlocal.h"					// prototypes
#include "../header/rfmodul.h"
#include "../header/cmd.h"						// CMD
#include "../header/circularBuffer.h"			// buffer
#include "../../ATuracoli/stackrelated.h"		// UART_print(f)
#include "../../ATuracoli/stackrelated_timer.h"	// UART_print(f)
#include "../../ATuracoli/stackdefines.h"		// defined register addresses


// === c-File Globals =====================================
static bool_t noTimeout = TRUE;
static uint8_t atAP_tmp = 0;
static uint16_t atCT_tmp = 0;

// === Functions ==========================================
/*
 * Get and set function for temporary AP value.
 * This variable is needed for API mode to write first to eeprom ans secondly apply the changes.
 * - Prototypes are declared in rfmodul.h
 *
 * Received:
 *		uint8_t		parameter value for atAP_tmp (set function)
 *
 * Returns:
 *		uint8_t		parameter value of atAP_tmp (get function)
 *
 * last modified: 2017/01/13
 */
uint8_t GET_atAP_tmp(void)            { return atAP_tmp; }
void    SET_atAP_tmp(uint8_t APvalue) {	atAP_tmp = APvalue; }

/*
 * Get and set function for temporary CT value.
 * This variable is needed for API mode to write first to eeprom ans secondly apply the changes.
 * - Prototypes are declared in rfmodul.h
 *
 * Received:
 *		uint16_t		parameter value for atCT_tmp (set function)
 *
 * Returns:
 *		uint16_t		parameter value of atCT_tmp (get function)
 *
 * last modified: 2017/01/17
 */
uint16_t GET_atCT_tmp(void)             { return atCT_tmp; }
void     SET_atCT_tmp(uint16_t CTvalue) { atCT_tmp = CTvalue; }