/*
 * stackrelated.h
 *
 * Created: 26.10.2016 08:50:30
 *  Author: TOE
 */ 


#ifndef STACKRELATED_H_
#define STACKRELATED_H_

#include "../header/_global.h"
#include "../header/enum_error.h"

#include "hif.h"
#include "transceiver.h"

/*
 * UART functions
 *
 * UART_init();		initialisation of UART, MCU and set baud rate
 * UART_print();	pointer to UART echo out funktion
 * UART_printf();	pointer to UART formated out function
 * UART_getc();		pointer to UART read function
 */
void UART_init(void);
#define UART_print(fmt)			hif_echo(FLASH_STRING(fmt))
#define UART_printf(fmt, ...)	hif_printf(FLASH_STRING(fmt), __VA_ARGS__)
int (*UART_getc) (void);

/*
 * transceiver (trx) functions
 *
 * trxBase_init();		pointer to trx base initializer function
 * trx_send_action();	function to sent data over air
 */
static uint8_t (*trxBase_init) (void) = trx_init;
void trx_send_action(int i, int *data);


typedef struct {
	volatile uint8_t cnt;
	volatile uint8_t fail;
	volatile bool_t in_progress;
}txStatus;

typedef struct {
	volatile uint8_t cnt;
	volatile uint8_t seq;
	volatile bool_t fail;
	volatile bool_t done;
}rxStatus;


#endif /* STACKRELATED_H_ */