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

// === globals ============================================
timeStat_t th = {STATEM_IDLE,0,0};

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
			if ( AT_MODE_ACTIVE == GET_serintCMD_ap() )	AT_parser(inchar, UART, &th );
			else										AP_parser(inchar, UART, &th );				
		}

	} /* end of while loop */ 

}

/*
 * Time handler for state machine
 * - if 3x CC type is received set state machine to AT handle
 *   else reset counter and state machine
 * - if the GT timer expired reset state machine (quit At mode) and counter
 * 
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2017/01/18
 */
uint32_t AT_GT_timeHandle(uint32_t arg)
{
	if ( th.counter == 3 ) 
	{
		UART_print_status(OP_SUCCESS);
		th.state = AT_MODE;
	}
	else
	{
		th.state = STATEM_IDLE;
	}
	th.counter = 0;
	th.watch   = 0;
	
	return 0;
}

uint32_t AT_CT_timeHandle(uint32_t arg)
{
	th.state   = STATEM_IDLE;
	th.counter = 0;
	th.watch   = 0;
	UART_print_status(QUIT_CMD_MODE);
	
	return 0;
}

/*
 * Time handler for state machine
 * - if no other sign received and the timer is expired reset Buffer and state machine
 * 
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2017/01/18
 */
uint32_t AP_expired_timeHandle(uint32_t arg)
{
	deBufferReadReset( UART, '+', th.counter);
	th.state   = STATEM_IDLE;
	th.watch   = 0;
	th.counter = 0;

	return 0;
}

/* EOF */