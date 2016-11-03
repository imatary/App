/*
 * stackrelated.h
 *
 * Created: 26.10.2016 08:50:30
 *  Author: TOE
 */ 


#ifndef STACKRELATED_H_
#define STACKRELATED_H_

#include "../ATcommands/header/_global.h"
#include "../ATcommands/header/enum_error.h"

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
 * TRX_baseInit();		pointer to trx base initializer function
 * TRX_setup();			setup the transceiver for sending or receiving data
 * TRX_send();			function to sent data over air
 * TRX_receive();		function to receive data over the air
 * TRX_msgFrame();		prepare the buffer to send a simple text message
 * TRX_atRemoteFrame();	prepare the buffer to send a AT Remote command
 * TRX_txHandler();
 * TRX_rxHandler();
 */

#define PACKAGE_SIZE 127						// size in bytes

static uint8_t (*TRX_baseInit)    (void)				= trx_init;
static    void (*TRX_setPanId)    (uint16_t panid)		= trx_set_panid;
static    void (*TRX_setShortAddr)(uint16_t shortaddr)	= trx_set_shortaddr;
static    void (*TRX_setLongAddr) (uint64_t longaddr)	= trx_set_longaddr;

void	TRX_setup(void);
void	TRX_ack(void);
ATERROR TRX_send(void);

int TRX_msgFrame		(uint8_t *send);
int TRX_atRemoteFrame	(uint8_t *send);

static void TRX_txHandler(void);
static void TRX_rxHandler(void);

typedef struct {
	volatile uint8_t cnt;
	volatile uint8_t fail;
	volatile bool_t in_progress;
}txStatus_t;

typedef struct {
	volatile uint8_t cnt;
	volatile uint8_t seq;
	volatile bool_t fail;
	volatile bool_t done;
}rxStatus_t;


#endif /* STACKRELATED_H_ */