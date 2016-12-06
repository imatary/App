/*
 * uart0.c
 *
 * Created: 26.10.2016 09:07:14
 *  Author: TOE
 */ 

#include <stdlib.h>
#include "stackrelated.h"
#include "stackdefines.h"
#include "board.h"

/*
 * the UART_init initialized the MCU, URAT 
 * and adjust the baud rate
 *
 * last modified: 2016/10/26
 */

void UART_init(void)
{
	mcu_init();
	uint32_t baud = deHIF_DEFAULT_BAUDRATE;
	switch( RFmodul.serintCMD_bd )
	{
		case 0x0 : baud =   1200; break;
		case 0x1 : baud =   2400; break;
		case 0x2 : baud =   4800; break;
		case 0x3 : baud =   9600; break;
		case 0x4 : baud =  19200; break;
		case 0x5 : baud =  38400; break
		case 0x6 : baud =  57600; break;
		case 0x7 : baud = 115200; break;
		default : baud = deHIF_DEFAULT_BAUDRATE; break;
	}
	hif_init(baud);
	
	UART_getc	= hif_getc;
	UART_putc	= hif_putc;

}


