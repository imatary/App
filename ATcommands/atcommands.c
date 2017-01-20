/*
 * atcommands.c
 *
 * Created: 11.10.2016 07:52:53
 *  Author: TOE
 *
 * This file contains the main function (root)
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "header/_global.h"
#include "header/atlocal.h"
#include "header/apiframe.h"
#include "header/rfmodul.h"							// RFmodul struct
#include "header/circularBuffer.h"					// buffer
#include "../ATuracoli/stackrelated.h"				// trx init, uart init
#include "../ATuracoli/stackrelated_timer.h"		// timer init

device_t RFmodul;
static bool_t   expired = FALSE;					// global use only in this file
static uint32_t DE_timeHandle(uint32_t arg);


void main(void) 
{
	at_status_t	  ret = 0;
	uint32_t	   th = 0;
	int		   inchar = 0; // received the data of the UART input (byte by byte)
	short     counter = 0;
	size_t apicounter = 0;
	/*
	 * (1.) initialize structs and buffer
	 */
	GET_allFromEEPROM();
	BufferInit(&UART_deBuf, &RX_deBuf, NULL);
	
	/*
	 * (2.) init peripheral devices
	 */
	UART_init();
	deTIMER_init();									
	if( !TRX_baseInit() ) ret =  TRX_INIT_ERROR;
	
	sei();	// allow interrupts

	/*
	 * (3.) starting main loop
	 */
	while (TRUE)
	{
		if( ret ) { UART_print_status(ret);  ret = 0; }
		/*
		 * Receiver operation
		 *
		 * last modified: 2016/11/04
		 */
		if (RX_deBuf.newContent)
		{
			ret = TRX_receive();
			if ( ret )	{ UART_print_status(ret);  ret = 0; }
		}

		/*
		 * uart operation
		 *
		 * last modified: 2016/11/29
		 */
		inchar = UART_getc();
		if ( EOF != inchar )
		{
			/*
			 * push the character into the buffer
			 * neither interrupts allowed
			 */
			cli(); ret = BufferIn( &UART_deBuf, inchar ); sei();
			if( ret ) 
			{ 
				UART_print_status(ret); 
				BufferInit(&UART_deBuf, NULL);
				ret = 0;
			}
			
			/*
			 * handle timer according to AP mode value
			 */
			if ( RFmodul.serintCMD_ap > 0 )
			{
				if ( 0x7E == inchar && 0 == apicounter )
				{
					th = deTIMER_start(DE_timeHandle, deMSEC( 0x18 ), 0 ); // 24 MS
					apicounter++;
				}
				apicounter = ( apicounter > 0 )? apicounter+1 : 0;
			
			} /* end API mode */
			else
			{
				/*
			     * if a plus (0x2B) received, start timer and count plus signs 
			     * if the user hit three times the plus button, he needs to wait 
				 * until the timer has expired to switch into local command mode
				 *
				 * else kill the timer and reset the counter
			     */
				
				if ((counter == 1 || counter == 2) && inchar != RFmodul.atcopCMD_cc)
				{
					th = deTIMER_stop(th);
					counter = 0;
					expired = FALSE;
					
				}
				else if ( inchar == RFmodul.atcopCMD_cc && 0 < counter && 3 > counter)
				{
					th = deTIMER_restart(th, deMSEC( RFmodul.atcopCMD_gt) );
					counter+=1;
				}
				else if ( inchar == RFmodul.atcopCMD_cc && 0 == counter )
				{
					th = deTIMER_start(DE_timeHandle, deMSEC( RFmodul.atcopCMD_gt ), 0 );
					counter+=1;
					
				} /* end of 0x2B condition */
				
				/*
				 * if plus counter = 0 send the buffer content 
				 */					
				if ( 0 == counter )
				{ 
					TRX_send(0, NULL, 0);
				}
			  
			}/* end AT mode */
						
		} /* end of uart condition */
		
		
		/*
		 * if AP Mode is enabled  and expired true handle AP Frame
		 */
		if( TRUE == expired && RFmodul.serintCMD_ap > 0 )
		{
			AP_frameHandle_uart();
			apicounter = 0;
			expired = FALSE;
			th = 0;
		}
		
		/*
		 * enter AT Mode
		 */
		if ( 3 == counter && TRUE == expired )
		{
			BufferInit(&UART_deBuf, NULL); // delete all content in the buffer
			AT_localMode();
			counter = 0;
			expired = FALSE;
		}
		
		
	} /* end of while loop */ 

}

/*
 * DE_timeHandle()
 * 
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2016/11/24
 */
static uint32_t DE_timeHandle(uint32_t arg)
{
	expired = TRUE;
	return 0;
}