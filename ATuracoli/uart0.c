/*
 * uart0.c
 *
 * Created: 26.10.2016 09:07:14
 *  Author: TOE
 */
#include <inttypes.h>

#include "../ATcommands/header/_global.h"
#include "../ATcommands/header/rfmodul.h"
#include "../ATcommands/header/enum_status.h"
#include "../ATcommands/header/defaultConfig.h"
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
	UART_baudInit();
	UART_getc	= hif_getc;
	UART_putc	= hif_putc;
	UART_puts	= hif_puts;
}

void UART_baudInit(void)
{
	switch( GET_serintCMD_bd() )
	{
		//case 0x0 : hif_init(   1200); break; // not supported
		case 0x1 : hif_init(   2400); break;
		case 0x2 : hif_init(   4800); break;
		case 0x3 : hif_init(   9600); break;
		case 0x4 : hif_init(  19200); break;
		case 0x5 : hif_init(  38400); break;
		//case 0x6 : hif_init(  57600); break; // not supported
		//case 0x7 : hif_init( 115200); break; // not supported
		default  :
		{
			uint8_t val = BD_INTERFACE_DATA_RATE;
			hif_init( deHIF_DEFAULT_BAUDRATE );
			SET_serintCMD_bd( &val, 1 );
			dirtyBits ^= DIRTYB_BD;
		}
	    break;
	}
}



/*
 * UART_print_status()
 * print a error message to the uart
 *
 * Received:
 *		at_status_t	value with the return information, which error occurred
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/04
 */
void UART_print_status(at_status_t value)
{
	switch(value)
	{
		case QUIT_CMD_MODE		: UART_print("Leave Command Mode.\r");						break;
		case OP_SUCCESS			: UART_print("OK\r");										break;
		case INVALID_COMMAND	: // UART_print("Invalid command!\r");							break; // not available on XBee
		case INVALID_PARAMETER	: // UART_print("Invalid parameter!\r");						break; // not available on XBee
		case ERROR				: UART_print("ERROR!\r");									break;
		case BUFFER_IN_FAIL		: UART_print("BufferIn error!\r"); 							break;
		case BUFFER_OUT_FAIL	: UART_print("BufferOut error!\r"); 						break;
		case TRANSMIT_OUT_FAIL	: UART_print("TX send fail!\r");							break;
		case TRANSMIT_IN_FAIL	: UART_print("RX receive fail!\r");							break;
		case TRANSMIT_CRC_FAIL  : UART_print("CRC code does not match.\r");					break;
		case COMMAND_MODE_FAIL	: UART_print("AT command mode error! Quit command mode.\r");break;
		case TRX_INIT_ERROR		: UART_print("Cannot initialize trx!\r");					break;
		case TIMER_START_FAIL	: UART_print("Timer could not start!\r");					break;
		default					: 															break;
	}
}



