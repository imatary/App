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
#include "header/enum_error.h"
#include "header/circularBuffer.h"
#include "../ATuracoli/stackrelated.h"


void main(void) 
{
	int inchar = 0, counter = 0;
	ATERROR ret;
	
	UART_Xbuf = {{}, 0, 0};
	  RX_Xbuf = {{}, 0, 0};
	
	/*
	 * init peripheral devices
	 */
	UART_init();			// init uart
	ret = TRX_baseInit();	// init transceiver base
	// TODO trx read mode - nur die Einstellungen für die Antenne ändern

	sei();					// allow interrupts

	/*
	 * starting main loop
	 */
	while (TRUE)
	{
		if( ret ) { ATERROR_print(ret); }
		
		/*
		 * uart operation
		 *
		 * last modified: 2016/10/27
		 */
		inchar = UART_getc();
		if ( EOF != inchar )
		{
			UART_printf("%c", buf ); // return character immediately
			/*
			 * push the character into the buffer
			 * neither interrupts allowed
			 */
			cli();
				ret = BufferIn( &UART_Xbuf, inchar );
			sei();
			if( ret ) { ATERROR_print(ret); continue; }
			
			
			/*
			 * if a carriage return (0xD) received, send the buffer content 
			 */
			if( 0xD == inchar ) 
			{ 
				ret = TRX_sendAction(); 
				if ( ret )	{ ATERROR_print(ret); }
				counter = 0;
				
			}
			
			/*
			 * if a plus (0x2B) received, count it 
			 * if the user hit three times the plus button switch to local command mode
			 */
			if ( 0x2B == inchar )
			{
				counter += 1;
				if ( 3 == counter )
				{
					ret = AT_localMode();
					if ( ret )	{ ATERROR_print(ret); }
					counter = 0;
					
				}
				
			} /* end of 0x2B condition */
			
		} /* end of uart condition */
		
		/*
		 * receiver operation
		 * 
		 * last modified:
		 */
	}

}

static void ATERROR_print(ATERROR *value)
{
	switch(*value)
	{
		case TRX_INIT_ERROR		: UART_print("Cannot initialize trx base!"); break;
		case BUFFER_IN_FAIL		: UART_print("BufferIn error!"); break;
		case BUFFER_OUT_FAIL	: UART_print("error!"); break;
		case TRANSMIT_OUT_FAIL	: UART_print("Transmitter send error! Data can't transmitted."); break;
		case TRANSMIT_IN_FAIL	:
		case NOT_ABLE_TO_WRITE	:
		case NOT_ABLE_TO_READ	: UART_print("error!"); break;
		case COMMAND_MODE_FAIL	: UART_print("AT command mode error! Quit command mode."); break;
	}
}