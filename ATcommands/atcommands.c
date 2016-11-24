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
#include "header/rfmodul.h"							// RFmodul struct
#include "header/circularBuffer.h"					// buffer
#include "../ATuracoli/stackrelated.h"				// trx init, uart init
#include "../ATuracoli/stackrelated_timer.h"		// timer init

device_t RFmodul;

static void ATERROR_print(ATERROR *value);


void main(void) 
{
	ATERROR ret     = 0;
	int		inchar  = 0; // received the data of the UART input (byte by byte)
	int     counter = 0;
	/*
	 * initialize structs and buffer
	 */
	GET_allFromEEPROM();
	BufferInit(&UART_deBuf, &RX_deBuf, NULL);
	
	/*
	 * init peripheral devices
	 */
	UART_init();
	deTIMER_init();									
	if( !TRX_baseInit() ) ret =  TRX_INIT_ERROR;
	
	sei();	// allow interrupts

	/*
	 * starting main loop
	 */
	while (TRUE)
	{
		if( ret ) { ATERROR_print(&ret);  ret = 0; }
		/*
		 * Receiver operation
		 *
		 * last modified: 2016/11/04
		 */
		if (RX_deBuf.newContent)
		{
			ret = TRX_receive();
			if ( ret )	{ ATERROR_print(&ret);  ret = 0; }
		}

		
		/*
		 * uart operation
		 *
		 * last modified: 2016/10/27
		 */
		inchar = UART_getc();
		if ( EOF != inchar )
		{
			//UART_printf("%c", inchar );				// return character immediately

			/*
			 * push the character into the buffer
			 * neither interrupts allowed
			 */
			cli(); ret = BufferIn( &UART_deBuf, inchar ); sei();
			if( ret ) { ATERROR_print(&ret); ret = 0; continue; }
			
			
			/*
			 * if a carriage return (0xD) received, send the buffer content 
			 */
			if( '\r' == inchar || '\n' == inchar ) 
			{ 
				ret = TRX_send(); 
				if ( ret )	{ ATERROR_print(&ret); ret = 0; }
				counter = 0;
				
			}
			
			/*
			 * if a plus (0x2B) received, count it 
			 * if the user hit three times the plus button switch to local command mode
			 */
			if ( RFmodul.atcopCMD_cc == inchar )
			{
				counter += 1;
				if ( 3 == counter )
				{
					/*
					 * if a API frame is received reset the read counter
					 */
					BufferOut( &UART_deBuf, (uint8_t*) &inchar);
					
					if ( 0x7e == inchar ) deBufferReadReset( &UART_deBuf, '+', 10 );
					else				  deBufferReadReset( &UART_deBuf, '+', 2 );
					ret = AT_localMode();
					if ( ret )	{ ATERROR_print(&ret); ret = 0; }
					counter = 0;
					
				}
				
			} /* end of 0x2B condition */
			
		} /* end of uart condition */
		
	} /* end of while loop */ 

}

static void ATERROR_print(ATERROR *value)
{
	switch(*value)
	{
		case TRX_INIT_ERROR		: UART_print("Cannot initialize trx base!\r\n");							break;
		case BUFFER_IN_FAIL		: UART_print("BufferIn error!\r\n"); 										break;
		case BUFFER_OUT_FAIL	: UART_print("BufferOut error!\r\n"); 										break;
		case TRANSMIT_OUT_FAIL	: UART_print("Transmitter send error! Data can't transmitted.\r\n");		break;
		case TRANSMIT_IN_FAIL	: UART_print("Receiver error! Can't receive or translate the data.\r\n");	break;
		case TRANSMIT_CRC_FAIL  : UART_print("Receiver error! CRC code does not match.\r\n");				break;
		case COMMAND_MODE_FAIL	: UART_print("AT command mode error! Quit command mode.\r\n");				break;
		default					: 																			break;
	}
}