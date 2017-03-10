/*
 * stackrelated.h
 *
 * Created: 26.10.2016 08:50:30
 *  Author: TOE
 */


#ifndef STACKRELATED_H_
#define STACKRELATED_H_

#include <stdio.h>
#include <stdlib.h>

#include "../ATcommands/header/_global.h"
#include "../ATcommands/header/enum_status.h"
#include "../ATcommands/header/ap_frames.h"

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
int  (*UART_getc) (void);
int  (*UART_putc) (int);
void (*UART_puts) (const char *);
void UART_print_status(at_status_t value);
void UART_print_data(uint8_t size, uint64_t val);
void UART_print_decimal(uint8_t number);

/*
 * transceiver (trx) pointer to functions
 *
 * TRX_setPanId();		pointer to function which set the PANID
 * TRX_setShortAddr();	pointer to function which set the 16-bit short address
 * TRX_setLongAddr();	pointer to function which set the 64-bit long address
 * TRX_init();			pointer to function which set up the basic radio functions
 * TRX_spiInit();		pointer to function which set the Serial Peripheral Interface rate
 * TRX_writeReg();		pointer to function which writes into the register
 * TRX_readReg();		pointer to function which reads the register
 * TRX_writeBit();		pointer to function which writes into the subregister
 * TRX_readBit();		pointer to function which reads the subregister value
 * TRX_writeTX();		pointer to function which write a frame into the frame buffer
 * TRX_readRX();		pointer to function which read the received frame out of the frame buffer
 * TRX_getRxLength();	pointer to function which get the length of a received frame
 */
static void		(*TRX_setPanId)		(uint16_t);
static void		(*TRX_setShortAddr)	(uint16_t);
static void		(*TRX_setLongAddr)	(uint64_t);
static uint8_t	(*TRX_init)			(void);
	   void		(*TRX_spiInit)		(uint8_t);
	   void		(*TRX_writeReg)		(uint8_t, uint8_t);
	   uint8_t	(*TRX_readReg)		(uint8_t);
	   void		(*TRX_writeBit)		(uint8_t, uint8_t, uint8_t, uint8_t);
	   uint8_t	(*TRX_readBit)		(uint8_t, uint8_t, uint8_t);
	   void		(*TRX_writeTX)		(uint8_t, uint8_t*);
	   uint8_t	(*TRX_readRX)		(uint8_t*, uint8_t, uint8_t*);
	   uint8_t	(*TRX_getRxLength)	(void);
	   uint8_t  (*TRX_initDatarate) (uint8_t);

/*
 * transceiver (trx) functions
 *
 * TRX_baseInit();		base initializer and setup the transceiver for sending or receiving data
 * TRX_send();			function to sent data over air
 * TRX_receive();		function to receive data over the air
 * TRX_get_TXfail();	returns the send failures (ACK failures)
 */
uint8_t		TRX_baseInit  (void);
void		TRX_send      (bufType_n bufType, uint8_t senderInfo, uint8_t *srcAddr, uint8_t srcAddrLen);
at_status_t TRX_receive   (bufType_n bufType);
uint8_t		TRX_get_TXfail(void);

#endif /* STACKRELATED_H_ */