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
static void		(*TRX_setPanId)		(uint16_t panid);
static void		(*TRX_setShortAddr)	(uint16_t shortaddr);
static void		(*TRX_setLongAddr)	(uint64_t longaddr);
static uint8_t	(*TRX_init)			(void);
	   void		(*TRX_spiInit)		(uint8_t spirate);
	   void		(*TRX_writeReg)		(uint8_t addr, uint8_t val);	  
	   uint8_t	(*TRX_readReg)		(uint8_t addr);
	   void		(*TRX_writeBit)		(uint8_t addr, uint8_t mask, uint8_t pos, uint8_t value);
	   uint8_t	(*TRX_readBit)		(uint8_t addr, uint8_t mask, uint8_t pos);      
	   void		(*TRX_writeTX)		(uint8_t length, uint8_t *data);
	   uint8_t	(*TRX_readRX)		(uint8_t *data, uint8_t datasz, uint8_t *lqi);
	   uint8_t	(*TRX_getRxLength)	(void);

/*
 * transceiver (trx) functions
 *
 * TRX_baseInit();		base initializer and setup the transceiver for sending or receiving data
 * TRX_send();			function to sent data over air
 * TRX_receive();		function to receive data over the air
 * TRX_msgFrame();		prepare the buffer to send a simple text message
 * TRX_atRemoteFrame();	prepare the buffer to send a AT Remote command
 * TRX_txHandler();
 * TRX_rxHandler();		writes the received frame into the deRX_buffer
 */
uint8_t TRX_baseInit(void);
ATERROR TRX_send(void);
ATERROR TRX_receive(void);

int TRX_msgFrame		(uint8_t *send);
int TRX_atRemoteFrame	(uint8_t *send);

static void TRX_txHandler(void);
static void TRX_rxHandler(void);

/*
 * transceiver (trx) status structs
 */
typedef struct {
	uint8_t cnt;
	uint8_t fail;
	bool_t in_progress;
}txStatus_t;

typedef struct {
	uint8_t cnt;
	uint8_t seq;
	bool_t fail;
	bool_t done;
}rxStatus_t;

#endif /* STACKRELATED_H_ */