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

#define  API_IDLE		0x0
#define  API_LENGTH_1	0x1
#define  API_LENGTH_2   0x2
#define  API_GET_DATA	0x3

device_t RFmodul;
static bool_t   expired = FALSE;
static uint8_t  apistate = 0;
static uint16_t apicounter = 0;

static uint32_t DE_timeHandle(uint32_t arg);

void main(void) 
{
	at_status_t	  ret = 0;
	uint32_t	   th = 0;
	int		   inchar = 0; // received the data of the UART input (byte by byte)
	short     counter = 0;
	
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
			 * handle timer according to AP mode value
			 */
			if ( RFmodul.serintCMD_ap > 0 )
			{
				/*
				 * State machine
				 * state 0 to 1
				 */
				if ( 0x7E == inchar && API_IDLE == apistate )
				{
					th = deTIMER_start(DE_timeHandle, deMSEC( 0x5 ), 0 ); // 5 MS
					AP_setFrameLength(0x0, FALSE);
					apistate = API_LENGTH_1;
				}
				/*
				 * state 1 to 2
				 */
				else if ( API_LENGTH_1 == apistate ) 
				{
					th = deTIMER_restart(th, deMSEC( 0x5 ) );
					AP_setFrameLength( (uint16_t) inchar << 8, TRUE );
					apistate = API_LENGTH_2;
				}
				/*
				 * state 2 to 3
				 */
				else if ( API_LENGTH_2 == apistate )
				{
					th = deTIMER_restart(th, deMSEC( 0x5 ) );
					ret = AP_setFrameLength( (uint16_t) inchar & 0xFF, TRUE );
					if ( ERROR == ret ) apistate = API_IDLE;
					else				apistate = API_GET_DATA;
				}
				/*
				 * state 3
				 */
				else if ( API_GET_DATA == apistate )
				{
					th = deTIMER_restart(th, deMSEC( 0x5 ) );
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
					apicounter++;
					
					if ( apicounter-1 == AP_getFrameLength() ) expired = TRUE;
				}
				
			
			} /* end API mode */
			else
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
			     * if a CC character received, start timer and count CC character signs 
			     * if the user hit three times the CC character button, he needs to wait 
				 * until the timer has expired to switch into local command mode
				 *
				 * else kill the timer and reset the counter
			     */
				
				if ((counter == 1 || counter == 2) && inchar != RFmodul.atcopCMD_cc ||\
					counter == 3)
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
			apistate = API_IDLE;
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
	if ( 0 == RFmodul.serintCMD_ap ) expired = ( FALSE == expired)? TRUE : FALSE;
	else
	{
		apicounter = 0;
		expired = FALSE;
		apistate = API_IDLE;
	}
	return 0;
}