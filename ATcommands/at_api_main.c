/*
 * at_api_main.c
 *
 * Created: 11.10.2016 07:52:53
 *  Author: TOE
 *
 * This file contains the main function (root)
 */
#include <stdio.h>								// EOF
#include <ctype.h>

#include "header/_global.h"						// bool_t, AT_MODE_ACTIVE values
#include "header/at_commands.h"					// AT command parser
#include "header/ap_frames.h"					// API command parser
#include "header/rfmodul.h"						// GET_serintCMD_ap
#include "header/circularBuffer.h"				// buffer & buffer init
#include "../ATuracoli/stackrelated.h"			// trx init, uart init
#include "../ATuracoli/stackrelated_timer.h"	// timer init

// === globals ============================================
uint8_t dirtyBits = 0;

// === functions ==========================================
void main(void)
{
	int inchar = 0;

	/*
	 * (1.) initialize structure and peripheral devices
	 */
	GET_allFromEEPROM();
	UART_init();
	deTIMER_init();
	TRX_baseInit();

	sei();	// allow interrupts

	/*
	 * (3.) starting main loop
	 */
	while (TRUE)
	{
		/*
		 * Receiver operation
		 */
		if ( TRUE == GET_deBufferNewContent(RX) )
		{
			TRX_receive(RX);
		}

		/*
		 * UART operation
		 */
		cli(); inchar = UART_getc(); sei();
		if ( EOF != inchar )
		{
			if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) AT_parser( (uint8_t) inchar, UART );
			else										  AP_parser( (uint8_t) inchar, UART );
		}

	} /* end of while loop */

}

/* EOF */