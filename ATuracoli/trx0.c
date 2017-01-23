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
#include "../ATcommands/header/apiframe.h"
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
int TRX_msgFrame	 (bufType_n bufType, uint8_t *send);
int TRX_atRemoteFrame(bufType_n bufType, uint8_t *send);

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

static txStatus_t tx_stat = {1,0,0,FALSE};
static rxStatus_t rx_stat = {0,0,FALSE,FALSE};

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
 * last modified: 2017/01/19
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
	
	TRX_writeBit(deSR_CHANNEL, GET_netCMD_ch() );
	TRX_writeBit(deSR_TX_AUTO_CRC_ON, TRUE);
		
	TRX_setPanId( GET_netCMD_id() );											// target PAN ID
	TRX_setLongAddr( (uint64_t) GET_netCMD_sh() << 32 | GET_netCMD_sl() );		// device long address
	TRX_setShortAddr( GET_netCMD_my() );										// short address

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
 * last modified: 2017/01/19
 */
void TRX_send(bufType_n bufType, uint8_t senderInfo, uint8_t *srcAddr, uint8_t srcAddrLen)
{
	tx_stat.senderInfo = senderInfo;
	uint8_t send[PACKAGE_SIZE] = {0};
	int pos;

	send[2] = tx_stat.cnt;	// set frame counter
	
	/*
	 * Handle buffer dependent on AP mode on or off and return pointer position in the array
	 */	 
	if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { pos = TRX_msgFrame( bufType, send );		}			// AT TX Transmit Request
	else
	{
		switch(senderInfo)
		{
			case 0x17 : { pos = TRX_atRemoteFrame    ( bufType, send );						} break;	// AP Remote Command	
		    case 0x00 : { pos = TRX_transmit64Frame  ( bufType, send ); 					} break;	// AP TX Transmit Request 64-bit addr.
	        case 0x01 : { pos = TRX_transmit16Frame  ( bufType, send ); 					} break;	// AP TX Transmit Request 16-bit addr.
		    case 0x97 : { pos = TRX_atRemote_response( bufType, send, srcAddr, srcAddrLen);	} break;	// AP Remote Response
		}
	}
													       
	if (pos == 0) 
	{
		if ( AT_MODE_ACTIVE != GET_serintCMD_ap() ) AP_txStatus(TRANSMIT_OUT_FAIL);
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
 * last modified: 2017/01/19
 */
int TRX_msgFrame(bufType_n bufType, uint8_t *send)
{    
	at_status_t ret;
	uint16_t u16tmp = 0;
	uint64_t u64tmp = 0;
	uint8_t   shift = 0; 
	int         pos = 3;
	                                       
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */	
	if ( TRUE )												  *send  = 0x01; // data frame			(bit 0)
	if ( TRUE == GET_secCMD_ee() )							  *send |= 0x08; // security active		(bit 3)
	if ( 0x00 == GET_netCMD_mm() || 0x02 == GET_netCMD_mm() ) *send |= 0x20; // ACK on				(bit 5)
															  *send |= 0x40; // PAN Compression		(bit 6)
	/*
	 * destination PAN_ID
	 */
	u16tmp = GET_netCMD_id();
	
	for (shift = 0; 9 > shift; shift += 8, pos++ )
	{
		*(send+pos) = (uint8_t) u16tmp >> shift;
	}					
	
	/*
	 * destination addr.
	 * extended or short
	 */
	u64tmp = (uint64_t) GET_netCMD_dh() << 32 | GET_netCMD_dl();
	
	if ( 0xFFFF < u64tmp || 0x00 <u64tmp )
	{
		for (shift = 0; 25 > shift; shift += 8, pos++ )
		{
			*(send+pos) = (uint8_t) u64tmp >>  shift;
		}
				
		*(send+1) |= 0x0C; // MAC header second byte (bit 2/3)
	} 
	else
	{
		u16tmp = (uint16_t) GET_netCMD_dl();
		
		for (shift = 0; 9 > shift; shift += 8, pos++ )
		{
			*(send+pos) = (uint8_t) u16tmp >> shift;
		}				
		
		*(send+1) |= 0x08; // MAC header second byte (bit 2/3)
	}
	
	/*
	 * source addr.
	 * short or extended
	 */
	u16tmp = GET_netCMD_my();
	if ( 0xFFFE != u16tmp )
	{
		for (shift = 0; 9 > shift; shift += 8, pos++ )
		{
			*(send+pos) = (uint8_t) u16tmp >> shift;
		}

		*(send+1) |= 0x80; // MAC header second byte (bit 6/7)
	} 
	else
	{
		u64tmp = (uint64_t) GET_netCMD_sh() << 32 || GET_netCMD_sl();
		
		for (shift = 0; 25 > shift; shift += 8, pos++ )
		{
			*(send+pos) = (uint8_t) u64tmp >>  shift;
		}
		
		*(send+1) |= 0xC0; // MAC header second byte (bit 6/7)
	}
	
	u16tmp = GET_netCMD_mm();
	if ( 0x1 != u16tmp || 0x2 != u16tmp )
	{
		*(send+pos)   = (uint8_t) ( *(send+2) + (GET_netCMD_sl() & 0xFF) );		// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x00;													// this byte will be used as command, 0x00 = no command is given
		pos += 2;
	}
	
	do
	{
		cli(); ret = deBufferOut( bufType, send + pos ); sei();
		pos +=1;

	} while ( BUFFER_OUT_FAIL != ret && pos < PACKAGE_SIZE-1 );

	return pos-1;
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
at_status_t TRX_receive(bufType_n bufType)
{
	uint8_t flen		= 0;	// total length of the frame which is stored in the buffer
	uint8_t outchar		= 0;	// received the data of the buffer (byte by byte)
	uint8_t dataStart	= 0;	// start position of data payload
	uint8_t srcAddrLen  = 0;	// source address length
	uint16_t macHeader	= 0;	// received the frame type for frame handling
	
	rx_stat.done = FALSE;
	SET_deBufferNewContent( bufType, FALSE);
	
	/*
	 * read the len out of the buffer
	 */
	cli(); deBufferOut( bufType, &flen); sei();
	
	/* 
	 * - get the frame type
	 * - set the data start pointer depending on frame type
	 * - print the data
	 */
	for (int i = 0; i < 2; i++)
	{
		cli(); deBufferOut( bufType, &outchar); sei();
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
			rx_stat.fail++;
			deBufferReadReset(bufType, '+', flen);
			return TRANSMIT_IN_FAIL;
	}
	
	if ( 0x0 == GET_serintCMD_ap() )
	{
		
	} 
	else
	{
		switch ( 0x48 & macHeader ) // define MAC options
		{
			case 0x40 : TRX_createAPframe( bufType, flen-2, dataStart,   srcAddrLen, 0x40 ); break;	// security disabled, PAN compression  enabled = add nothing
			case 0x00 : TRX_createAPframe( bufType, flen-2, dataStart+2, srcAddrLen, 0x00 ); break;	// security disabled, PAN compression disabled = add 2 bytes for src PAN ID
			case 0x08 : TRX_createAPframe( bufType, flen-2, dataStart+5, srcAddrLen, 0x08 ); break;	// security  enabled, PAN compression disabled = add 5 bytes for Auxiliary Sec. Header
			case 0x48 : TRX_createAPframe( bufType, flen-2, dataStart+7, srcAddrLen, 0x48 ); break;	// security  enabled, PAN compression  enabled = add 7 bytes for src PAN ID and Auxiliary Sec. header
			default:
				rx_stat.fail++;
				deBufferReadReset( bufType, '+', flen);
				return TRANSMIT_IN_FAIL;
		}
	}
	
	rx_stat.cnt += 1;
	return OP_SUCCESS;
}

/*
 * Get functions for status informations
 *
 * Received:
 *		deBuffer_t	pointer to receiver buffer
 *		uint8_t		length of the frame
 *		uint8_t		start position of data payload
 *		uint16_t	pointer to mac header value
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/19
 */
static void TRX_printContent(bufType_n bufType, uint8_t flen, uint8_t dataStart, uint16_t *macHeader )
{
	uint8_t outchar;
	
	if ( 0x08 & *macHeader )	// security enabled
	{
		/* TODO */
	}
	else					    // security disabled
	{
		dataStart += ( 0x1 == GET_netCMD_mm() || 0x2 == GET_netCMD_mm() )? 0 : 2;
		for (uint8_t i = 0; i < flen-0x4; i++)
		{
			cli(); deBufferOut( bufType, &outchar); sei();
			if ( i >= dataStart ) UART_putc(outchar);

		}/* end for loop */
	}
}


/*
 * Get functions for status informations
 *
 * Returns:
 *		uint8_t		value for send failures (TRX_get_TXfail)
 *
 * last modified: 2016/11/01
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
			if ( 0x0 == GET_serintCMD_ap() || ( 0x00 < GET_serintCMD_ap() && ( 0x97 == tx_stat.senderInfo || 0x17 == tx_stat.senderInfo )) ) /* do nothing */;
			else  AP_txStatus(OP_SUCCESS);
			break;

			case TRAC_CHANNEL_ACCESS_FAILURE:
			/* TODO TX_CCA_FAIL; */
			break;

			case TRAC_NO_ACK:
			tx_stat.fail++;
			if ( 0 > GET_serintCMD_ap() ) AP_txStatus(TRANSMIT_OUT_FAIL);
			else UART_print_status(TRANSMIT_OUT_FAIL);
			break;

			default:
			tx_stat.fail++;
			if ( 0 < GET_serintCMD_ap() ) AP_txStatus(TRANSMIT_OUT_FAIL);
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
	bufType_n bufType = RX;
	
	flen = TRX_readRX(&receive[0], sizeof(receive), NULL);
	if ( 0x7F < flen ) // 0x7f == 127(dez)
	{
		UART_print("Buffer overflow\r");
		return;
	}

	/*
	 * write the length of the frame in the first field of the buffer
	 */
	cli(); deBufferIn( bufType, flen-2); sei();
	
	/*
	 * upload the4 frame into RX buffer
	 */	
	for (uint8_t i = 0; i < flen; i++)
	{
		cli(); deBufferIn( bufType, receive[i] ); sei();
	}

	SET_deBufferNewContent( bufType, TRUE);
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