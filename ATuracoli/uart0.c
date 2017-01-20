/*
 * uart0.c
 *
 * Created: 26.10.2016 09:07:14
 *  Author: TOE
 */ 
#include <inttypes.h>

#include "../ATcommands/header/rfmodul.h"
#include "../ATcommands/header/enum_status.h"
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
		case 0x5 : baud =  38400; break;
		case 0x6 : baud =  57600; break;
		case 0x7 : baud = 115200; break;
		default : baud = deHIF_DEFAULT_BAUDRATE; break;
	}
	hif_init(baud);
	
	UART_getc	= hif_getc;
	UART_putc	= hif_putc;
	UART_puts	= hif_puts;
}

/*
 * UART_print_data()
 * print uint8/16/32 to the uart
 * 
 * Received:
 *		uint8_t		real size of the receiving object
 *		uint64_t	pointer to data
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/04
 */
void UART_print_data(uint8_t size, uint64_t val)
{
	switch(size)
	{
		case 1 : UART_printf("%"PRIX8"\r",  (uint8_t)  val & 0xFF ); break;
		case 2 : UART_printf("%"PRIX16"\r", (uint16_t) val & 0xFFFF ); break;
		case 4 : UART_printf("%"PRIX32"\r", (uint32_t) val & 0xFFFFFFFF ); break;
		case 8 : UART_printf("%"PRIX64"\r", val ); break;
		default: break;
	}
}

/*
 * UART_print_decimal()
 * print a decimal number to the uart
 * 
 * Received:
 *		at_status_t	value with the return information, which error occurred
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/04
 */
void UART_print_decimal(uint8_t number)
{
	UART_printf("%d\r", number);
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
		case ERROR				: UART_print("ERROR!\r");									break;	
		case INVALID_COMMAND	: UART_print("Invalid command!\r");							break;
		case INVALID_PARAMETER	: UART_print("Invalid parameter!\r");						break;
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



