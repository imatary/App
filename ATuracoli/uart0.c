/*
 * uart0.c
 *
 * Created: 26.10.2016 09:07:14
 *  Author: TOE
 */ 

#include "stackrelated.h"
#include "board.h"

/*
 * the UART_init initialized the MCU, URAT 
 * and adjust the baud rate
 *
 * last modified: 2016/10/26
 */

static inline void UART_init(void)
{
	mcu_init();
	hif_init(HIF_DEFAULT_BAUDRATE);
	
	UART_getc	= hif_getc;

}


