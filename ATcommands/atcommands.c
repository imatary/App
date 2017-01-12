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
bool_t   APIframe = FALSE;							// global use only in this file
static uint32_t AP_timeHandle(uint32_t arg);


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
			 * if AP mode active
			 */
			if ( RFmodul.serintCMD_ap > 0 )
			{
				if ( 0x7E == inchar && 0 == apicounter )
				{
					th = deTIMER_start(AP_timeHandle, deMSEC( 0x17 ), 0 ); // 23 MS
					apicounter++;
				}
				apicounter = ( apicounter > 0 )? apicounter+1 : 0;
				
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
					continue; 
				}
			
			}
			else
			{
				if ( 0x7E == inchar && 0 == apicounter )
				{
					th = deTIMER_start(AP_timeHandle, deMSEC( RFmodul.atcopCMD_gt ), 0 );
					apicounter++;
				}
				apicounter = ( apicounter > 0 )? apicounter+1 : 0;
				
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
					continue; 
				}

			    /*
			     * if a plus (0x2B) received, count it 
			     * if the user hit three times the plus button switch to local command mode
			     */
				if ((counter == 1 || counter == 2) && inchar != RFmodul.atcopCMD_cc)
				{
					counter = 0;
				}
			    if ( inchar == RFmodul.atcopCMD_cc  ) // && RFmodul.serintCMD_ap == 0
			    {
			    	counter += 1;
			    	if ( 3 == counter )
			    	{
			    		BufferInit(&UART_deBuf, NULL); // delete all content in the buffer
			    		AT_localMode();
			    		counter = 0;
			    		
			    	}
			    	
			    } /* end of 0x2B condition */
			}
						
		} /* end of uart condition */
		
		/*
		 * if AP Mode is enabled  and APIframe true handle AP Frame
		 */
		if( TRUE == APIframe && RFmodul.serintCMD_ap > 0 )
		{
			AP_frameHandle_uart();
			apicounter = 0;
			APIframe = FALSE;
			th = 0;
		}
		/*
		 * if a carriage return (0xD) received and the AP Mode is disabled send the buffer content 
		 */					
		if ( (('\r' == inchar || '\n' == inchar) || TRUE == APIframe ) && RFmodul.serintCMD_ap == 0 )
		{ 
			TRX_send(0, NULL, 0);
			counter = 0;
		}
		
	} /* end of while loop */ 

}

/*
 * CMD_timeHandle()
 * 
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2016/11/24
 */
static uint32_t AP_timeHandle(uint32_t arg)
{
	APIframe = TRUE;
	return 0;
}