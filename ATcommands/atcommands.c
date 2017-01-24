/*
 * atcommands.c
 *
 * Created: 11.10.2016 07:52:53
 *  Author: TOE
 *
 * This file contains the main function (root)
 */ 
#include <stdio.h>								// EOF

#include "header/_global.h"						// bool_t, AT_MODE_ACTIVE values
#include "header/atlocal.h"						// AT command parser
#include "header/apiframe.h"					// API command parser
#include "header/rfmodul.h"						// GET_serintCMD_ap
#include "header/circularBuffer.h"				// buffer & buffer init
#include "../ATuracoli/stackrelated.h"			// trx init, uart init
#include "../ATuracoli/stackrelated_timer.h"	// timer init

// === functions ==========================================
void main(void) 
{
	int	inchar = 0;
	
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
		inchar = UART_getc();
		if ( EOF != inchar )
		{
			if ( AT_MODE_ACTIVE == GET_serintCMD_ap() )	AT_parser(inchar, UART );
			else										AP_parser(inchar, UART );				
		}

	} /* end of while loop */ 

}

/* EOF */