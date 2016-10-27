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
#include "header/circularBuffer.h"
#include "stackrelated/stackrelated.h"


void main(void) 
{
	bool_t initErr = FALSE;
	int inchar;
	Buffer UART_Xbuf = {{}, 0, 0};
	Buffer RX_Xbuf	= {{}, 0, 0};
	
	/*
	 * init uart
	 *
	 * last modified: 2016/10/26
	 */
	UART_init();

	/*
	 * init transceiver base
	 *
	 * if		SEND    == XXX 
	 * else if	RECEIVE == XXX
	 *
	 * last modified: 2016/10/26
	 */
	if( !trxBase_init() ) initErr = TRUE;

	sei();

	while (TRUE)
	{
		if(initErr) { UART_print("Cannot initialize trx base!"); initErr = FALSE; }
		
		inchar = UART_getc();
		if ( EOF != inchar )
		{
			cli();
			BufferIn( &UART_Xbuf, inchar );
			sei();
			uint8_t buf;
			cli();
			BufferOut(&UART_Xbuf,&buf);
			UART_printf("%c", buf );
			sei();
		}
	}

}
