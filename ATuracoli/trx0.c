/*
 * trx0.c
 *
 * Created: 26.10.2016 09:07:31
 *  Author: TOE
 *
 */ 

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "../ATcommands/header/_global.h"
#include "../ATcommands/header/rfmodul.h"
#include "../ATcommands/header/circularBuffer.h"
#include "stackrelated.h"
#include "stackdefines.h"

#include "board.h"
#include "transceiver.h"

// === prototypes =========================================
/*
 * TRX_txHandler	sender interrupt handler
 * TRX_rxHandler	receiver interrupt handler
 */
static void TRX_txHandler(void);
static void TRX_rxHandler(void);
/*
 * TRX_msgFrame			prepare the buffer to send a simple text message
 * TRX_atRemoteFrame	prepare the buffer to send a AT Remote command
 */
int TRX_msgFrame		(uint8_t *send);
int TRX_atRemoteFrame	(uint8_t *send);

// === structs & var init =================================
/*
 * transceiver (trx) status structs
 */
typedef struct {
	uint8_t cnt;
	uint8_t fail;
	uint8_t senderInfo;
	bool_t in_progress;
}txStatus_t;

typedef struct {
	uint8_t cnt;
	uint8_t seq;
	bool_t fail;
	bool_t done;
}rxStatus_t;

txStatus_t tx_stat = {1,0,0,FALSE};
rxStatus_t rx_stat = {0,0,FALSE,FALSE};

// === functions ==========================================
/* 
 * Setup transmitter 
 * - set the function pointer
 * - configure radio channel
 * - configure address filter 
 * - enable transmitters automatic crc16 generation
 * - go into RX AACK state,
 * - configure address filter
 * - enable "receive end" IRQ
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/11/10
 */
uint8_t TRX_baseInit(void)
{	
	uint8_t ret;
	
	TRX_setPanId	 = trx_set_panid;
	TRX_setShortAddr = trx_set_shortaddr;
	TRX_setLongAddr	 = trx_set_longaddr;
	TRX_init		 = trx_init;
	TRX_spiInit		 = trx_io_init;
	TRX_writeReg	 = trx_reg_write;
	TRX_readReg		 = trx_reg_read;
	TRX_writeBit	 = trx_bit_write;
	TRX_readBit		 = trx_bit_read;	
	TRX_writeTX		 = trx_frame_write;
	TRX_readRX		 = trx_frame_read;
	TRX_getRxLength  = trx_frame_get_length;
	
	
	TRX_spiInit(deSPI_RATE_1_2);
	ret = TRX_init();
	
	TRX_writeBit(deSR_CHANNEL, RFmodul.netCMD_ch);
	TRX_writeBit(deSR_TX_AUTO_CRC_ON, TRUE);
		
	TRX_setPanId( RFmodul.netCMD_id );											// target PAN ID
	TRX_setLongAddr( (uint64_t) RFmodul.netCMD_sh << 32 | RFmodul.netCMD_sl );	// device long address
	TRX_setShortAddr( RFmodul.netCMD_my );										// short address

	TRX_writeReg(deRG_TRX_STATE, deCMD_RX_AACK_ON);
	
	
	#if defined(deTRX_IRQ_TRX_END)
		TRX_writeReg(deRG_IRQ_MASK, deTRX_IRQ_TRX_END);
		
	#elif defined(deTRX_IRQ_RX_END)
		TRX_writeReg(deRG_IRQ_MASK, deTRX_IRQ_RX_END | deTRX_IRQ_TX_END );
		
	#else
	#  error "Unknown IRQ bits"
	#endif
	
	return ret;
}

/*
 * TRX_send
 * send a message over antenna to another device
 *
 * Received:
 *		uint8_t		frame type information
 *		uint8_t		pointer to frame type or send information array if AP 0x97 is called
 *		uint8_t		length of source address if AP 0x97 is called
 *
 * Returns:
 *     nothing
 * 
 * last modified: 2017/01/10
 */
void TRX_send(uint8_t senderInfo, uint8_t *srcAddr, uint8_t srcAddrLen)
{
	tx_stat.senderInfo = senderInfo;
	uint8_t send[PACKAGE_SIZE] = {0};
	int pos;

	send[2] = tx_stat.cnt;	// set frame counter
	
	/*
	 * Handle buffer dependent on AP mode on or off and return pointer position in the array
	 */	 
	if      ( RFmodul.serintCMD_ap > 0 && senderInfo == 0x17 ) { pos = TRX_0x17_atRemoteFrame(&send[0]);							}	// AP Remote Command	
	else if ( RFmodul.serintCMD_ap > 0 && senderInfo == 0x00 ) { pos = TRX_0x00_transmit64Frame(&send[0]); 							}	// AP TX Transmit Request 64-bit addr.
	else if ( RFmodul.serintCMD_ap > 0 && senderInfo == 0x01 ) { pos = TRX_0x01_transmit16Frame(&send[0]); 							}	// AP TX Transmit Request 16-bit addr.
	else if ( RFmodul.serintCMD_ap > 0 && senderInfo == 0x97 ) { pos = TRX_0x97_atRemote_response(&send[0], srcAddr, srcAddrLen);	}	// AP Remote Response
	else												       { pos = TRX_msgFrame(&send[0]);										}	// AT TX Transmit Request
	
	if (pos == 0) 
	{
		if ( RFmodul.serintCMD_ap > 0 ) AP_0x89_txStatus(TRANSMIT_OUT_FAIL);
		return;
	}

	/*
	 * Step 2: setup and send package
	 */
	TRX_writeReg(deRG_TRX_STATE, deCMD_RX_AACK_ON);
	if (tx_stat.in_progress == FALSE)
	{
#if DEBUG
		UART_printf(">TX FRAME tx: %4d, fail: %3d, tx_seq: %3d\r", tx_stat.cnt, tx_stat.fail, send[2]);
#endif
		TRX_writeBit(deMAX_FRAME_RETRIES, 3);
		/* some older SPI transceivers require this coming from RX_AACK*/
		TRX_writeReg(deRG_TRX_STATE, deCMD_PLL_ON);
		TRX_writeReg(deRG_TRX_STATE, deCMD_TX_ARET_ON);
		
#if DEBUG
	UART_print(">Send: ");
	for (int i=0; i<pos; i++)
	{
		UART_printf("%02x ", send[i], i);
	}
	UART_print("\r");
#endif

		TRX_writeTX(pos + 2, send);
		tx_stat.in_progress = TRUE;
		TRX_SLPTR_HIGH();
		TRX_SLPTR_LOW();	
	}
}

/* 
 * Simple text frame 
 * prepared frame for simple text message
 *
 * Received:
 *		uint8_t		pointer to send array
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/01/09
 */
int TRX_msgFrame(uint8_t *send)
{                                              
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */	
	if ( TRUE )													 *send  = 0x01; // if data send to _one_ device else Beacon /* TODO */
	if ( RFmodul.secCMD_ee == TRUE )							 *send |= 0x08; // security active
	if ( RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x2 )  *send |= 0x20; // ACK on
																 *send |= 0x40; // PAN Compression
		
	*(send+ 3) = (uint8_t) (RFmodul.netCMD_id & 0xff);
	*(send+ 4) = (uint8_t) (RFmodul.netCMD_id >>  8);							// destination PAN_ID
	
	int pos = 5;
	if ( RFmodul.netCMD_dl > 0xFFFF || RFmodul.netCMD_dh > 0x0 )
	{
		*(send+ 5) = (uint8_t) (RFmodul.netCMD_dl >>  0);
		*(send+ 6) = (uint8_t) (RFmodul.netCMD_dl >>  8);
		*(send+ 7) = (uint8_t) (RFmodul.netCMD_dl >> 16);
		*(send+ 8) = (uint8_t) (RFmodul.netCMD_dl >> 24);						// destination ext. addr. low

		*(send+ 9) = (uint8_t) (RFmodul.netCMD_dh >>  0);
		*(send+10) = (uint8_t) (RFmodul.netCMD_dh >>  8);
		*(send+11) = (uint8_t) (RFmodul.netCMD_dh >> 16);
		*(send+12) = (uint8_t) (RFmodul.netCMD_dh >> 24);						// destination ext. addr. high
		
		*(send+1) |= 0x0C;														// MAC header second byte
		pos += 8;
	} 
	else
	{
		*(send+ 5) = (uint8_t) (RFmodul.netCMD_dl >>  0);
		*(send+ 6) = (uint8_t) (RFmodul.netCMD_dl >>  8);						// destination short addr.
		
		*(send+1) |= 0x08;														// MAC header second byte
		pos += 2;
	}
	
	/*
	 * src address
	 */
	if ( RFmodul.netCMD_my != 0xFFFE )
	{
		*(send+pos)   = (uint8_t) (RFmodul.netCMD_my & 0xff);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_my >> 8);						// src. short address
		
		*(send+1) |= 0x80;														// MAC header second byte
		pos += 1;
	} 
	else
	{
		*(send+pos)   = (uint8_t) (RFmodul.netCMD_sl >>  0);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_sl >>  8);
		*(send+pos+2) = (uint8_t) (RFmodul.netCMD_sl >> 16);
		*(send+pos+3) = (uint8_t) (RFmodul.netCMD_sl >> 24);					// src. ext. addr. low
		
		*(send+pos+4) = (uint8_t) (RFmodul.netCMD_sh >>  0);
		*(send+pos+5) = (uint8_t) (RFmodul.netCMD_sh >>  8);
		*(send+pos+6) = (uint8_t) (RFmodul.netCMD_sh >> 16);
		*(send+pos+7) = (uint8_t) (RFmodul.netCMD_sh >> 24);					// src. ext. addr. high
		
		*(send+1) |= 0xC0;														// MAC header second byte
		pos += 7;
	}
	
	if ( RFmodul.netCMD_mm != 0x1 || RFmodul.netCMD_mm != 0x2 )
	{
		*(send+pos)   = (uint8_t) ( *(send+2) + (RFmodul.netCMD_sl & 0xFF) );	// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x00;													// this byte will be used as command, 0x00 = no command is given
		pos += 2;
	}
	
	do
	{
		pos +=1;
		cli(); BufferOut(&UART_deBuf, send + pos ); sei();

	} while ( 0xD != *(send + pos - 1) && pos < PACKAGE_SIZE-1 );
	
	return pos;
}

/*
 * TRX_receive()
 * translated received packages 
 *
 * Returns:
 *     OP_SCCESS			frame is received successfully without error
 *	   TRANSMIT_IN_FAIL		frame is received and an error has occurred
 *
 * last modified: 2017/01/03
 */
at_status_t TRX_receive(void)
{
	uint8_t flen		= 0;	// total length of the frame which is stored in the buffer
	uint8_t outchar		= 0;	// received the data of the buffer (byte by byte)
	uint8_t dataStart	= 0;	// start position of data payload
	uint8_t srcAddrLen  = 0;	// source address length
	uint16_t macHeader	= 0;	// received the frame type for frame handling
	
	rx_stat.done = FALSE;
	BufferNewContent(&RX_deBuf, FALSE);
	
	/*
	 * read the len out of the buffer
	 */
	cli(); BufferOut(&RX_deBuf, &flen); sei();
	
	/* 
	 * - get the frame type
	 * - set the data start pointer depending on frame type
	 * - print the data
	 */
	for (int i = 0; i < 2; i++)
	{
		cli(); BufferOut(&RX_deBuf, &outchar); sei();
		if ( i == 0 ) { macHeader  = (uint16_t) outchar & 0xFF; }
		if ( i == 1 ) { macHeader |= (uint16_t) outchar << 8;   }
	}
	
	switch ( 0xCC00 & macHeader ) // address fields length
	{
		case 0x8800 : dataStart = 0x07; srcAddrLen = 2; break; // dest 16 & src 16 -> 4  bytes + (dest PAN ID + Frame counter) 3 bytes =  7
		case 0x8C00 : dataStart = 0x0D; srcAddrLen = 2; break; // dest 64 & src 16 -> 10 bytes + (dest PAN ID + Frame counter) 3 bytes = 13
		case 0xC800 : dataStart = 0x0D; srcAddrLen = 8; break; // dest 16 & src 64 -> 10 bytes + (dest PAN ID + Frame counter) 3 bytes = 13
		case 0xCC00 : dataStart = 0x13; srcAddrLen = 8; break; // dest 64 & src 64 -> 16 bytes + (dest PAN ID + Frame counter) 3 bytes = 19
		default: 
			BufferInit(&RX_deBuf, NULL);
			return TRANSMIT_IN_FAIL;
	}
	
	if ( 0x0 == RFmodul.serintCMD_ap )
	{
		if ( 0x08 & macHeader )	// security enabled
		{
		}
		else					// security disabled
		{
			dataStart += (0x1 == RFmodul.netCMD_mm || 0x2 == RFmodul.netCMD_mm)? 0 : 2;
			for (uint8_t i = 0; i < flen-0x4; i++)
			{
				cli(); BufferOut(&RX_deBuf, &outchar); sei();
				if ( i >= dataStart ) UART_putc(outchar);
			}/* end for loop */
		}
		deBufferReadReset(&RX_deBuf, '+', 2);
	} 
	else
	{
		switch ( 0x48 & macHeader ) // define MAC options
		{
			case 0x40 : TRX_createAPframe( flen-2, dataStart,   srcAddrLen, 0x40 ); break;	// security disabled, PAN compression  enabled = add nothing
			case 0x00 : TRX_createAPframe( flen-2, dataStart+2, srcAddrLen, 0x00 ); break;	// security disabled, PAN compression disabled = add 2 bytes for src PAN ID
			case 0x08 : TRX_createAPframe( flen-2, dataStart+5, srcAddrLen, 0x08 ); break;	// security  enabled, PAN compression disabled = add 5 bytes for Auxiliary Sec. Header
			case 0x48 : TRX_createAPframe( flen-2, dataStart+7, srcAddrLen, 0x48 ); break;	// security  enabled, PAN compression  enabled = add 7 bytes for src PAN ID and Auxiliary Sec. header
			default:
			BufferInit(&RX_deBuf, NULL);
			return TRANSMIT_IN_FAIL;
		}
	}
	
	rx_stat.cnt += 1;
	rx_stat.done = FALSE;
	if(FALSE)
	{
		rx_stat.fail++;
		return TRANSMIT_IN_FAIL;
	}	
	
	return OP_SUCCESS;
}

/*
 * Get functions for status informations
 *
 * Returns:
 *		uint8_t		value for send failures (TRX_get_TXfail)
 */
uint8_t TRX_get_TXfail(void)
{
	return tx_stat.fail;
}

/*
 * Handler for tx and rx frames
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/11/01
 */
static void TRX_txHandler(void)
{	
	static volatile uint8_t trac_status;
	trac_status = TRX_readBit(deSR_TRAC_STATUS);
	
	TRX_writeReg(deRG_TRX_STATE, deCMD_PLL_ON);
	TRX_writeReg(deRG_TRX_STATE, deCMD_TX_ARET_ON);
	
	if (TRUE == tx_stat.in_progress)
	{
		switch (trac_status)
		{
			case TRAC_SUCCESS:
			#if defined TRAC_SUCCESS_DATA_PENDING
			case TRAC_SUCCESS_DATA_PENDING:
			#endif
			
			#if defined TRAC_SUCCESS_WAIT_FOR_ACK
			case TRAC_SUCCESS_WAIT_FOR_ACK:
			#endif
			tx_stat.cnt++;
			if ( 0x0 == RFmodul.serintCMD_ap || (RFmodul.serintCMD_ap > 0x0 && (tx_stat.senderInfo == 0x97 || tx_stat.senderInfo == 0x17)) ) /* do nothing */;
			else  AP_0x89_txStatus(OP_SUCCESS);
			break;

			case TRAC_CHANNEL_ACCESS_FAILURE:
			/* TODO TX_CCA_FAIL; */
			break;

			case TRAC_NO_ACK:
			tx_stat.fail++;
			if ( RFmodul.serintCMD_ap > 0 ) AP_0x89_txStatus(TRANSMIT_OUT_FAIL);
			else UART_print_status(TRANSMIT_OUT_FAIL);
			break;

			default:
			tx_stat.fail++;
			if ( RFmodul.serintCMD_ap > 0 ) AP_0x89_txStatus(TRANSMIT_OUT_FAIL);
			else UART_print_status(TRANSMIT_OUT_FAIL);
			break;
		}
	}
	
	tx_stat.in_progress = FALSE;
	
	TRX_writeReg(deRG_TRX_STATE, deCMD_RX_AACK_ON);
}

static void TRX_rxHandler(void)
{
	uint8_t flen = 0, receive[PACKAGE_SIZE];
	
	flen = TRX_readRX(&receive[0], sizeof(receive), NULL);
	if ( 0x7F < flen ) // 0x7f == 127(dez)
	{
		UART_print("Buffer overflow\r");
		return;
	}

	/*
	 * write the length of the frame in the first field of the buffer
	 */
	cli(); BufferIn(&RX_deBuf, flen); sei();
	
	/*
	 * upload the4 frame into RX buffer
	 */	
	for (uint8_t i = 0; i < flen; i++)
	{
		cli(); BufferIn( &RX_deBuf, receive[i] ); sei();
	}

	BufferNewContent(&RX_deBuf, TRUE);
	rx_stat.done = TRUE;
}

/*
 * IRQ functions
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/11/01
 */
#if defined(deTRX_IF_RFA1)
ISR(deTRX24_TX_END_vect)
{
	TRX_txHandler();
}

ISR(deTRX24_RX_END_vect)
{
	TRX_rxHandler();
}
#else  /* !RFA1 */
ISR(deTRX_IRQ_vect)
{
	static volatile uint8_t irq_cause;
	irq_cause = TRX_readReg(deRG_IRQ_STATUS);	
	
	if (irq_cause & deTRX_IRQ_TRX_END)
	{
		if (tx_stat.in_progress)
		{
			TRX_txHandler();
		} 
		else
		{
			TRX_rxHandler();
		}
		
	}
		
}
#endif  /* RFA1 */