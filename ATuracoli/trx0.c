/*
 * trx0.c
 *
 * Created: 26.10.2016 09:07:31
 *  Author: TOE
 *
 */ 

#include <stdint-gcc.h>

#include "../ATcommands/header/_global.h"
#include "../ATcommands/header/circularBuffer.h"
#include "stackrelated.h"

#include "board.h"
#include "transceiver.h"

#define API 0

txStatus_t tx_stat = {1,FALSE,FALSE};
rxStatus_t rx_stat = {0,0,FALSE,FALSE};

/* 
 * Setup transmitter 
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
 * last modified: 2016/11/08
 */
uint8_t TRX_baseInit(void)
{	
	uint8_t ret;
	trx_io_init(SPI_RATE_1_2);
	ret = trx_init();
	
	trx_bit_write(SR_CHANNEL,netCMD.ch);
	trx_bit_write(SR_TX_AUTO_CRC_ON,TRUE);
		
	TRX_setPanId( RFmodul.netCMD_id );										// target PAN ID
	TRX_setLongAddr( (uint64_t) RFmodul.netCMD_sh<<32 | RFmodul.netCMD_sl );		// device long address
	TRX_setShortAddr( RFmodul.netCMD_my );									// short address

	trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON); 
	
	
	#if defined(TRX_IRQ_TRX_END)
		trx_reg_write(RG_IRQ_MASK,TRX_IRQ_TRX_END);
	#elif defined(TRX_IRQ_RX_END)
		trx_reg_write(RG_IRQ_MASK,TRX_IRQ_RX_END | TRX_IRQ_TX_END );
	#else
	#  error "Unknown IRQ bits"
	#endif
	
	return ret;
}

/*
 * TRX_send
 *
 * Returns:
 *     XX       buffer is empty. he cannot send a byte.
 *     SUCCESS			package was delivered
 * 
 * last modified: 2016/11/01
 */

ATERROR TRX_send(void)
{
	uint8_t send[PACKAGE_SIZE] = {0};
	int pos;

	/*
	 * Switch/case handling
	 * == not completely included in prototype ==
	 *
	 */
	frame._type = 0x0;
	switch (frame._type)
	{
		case 0x00 : pos = TRX_msgFrame(&send[0]);		break;	// TX Transmit Request 64-bit
		case 0x17 : pos = TRX_atRemoteFrame(&send[0]);	break;	// Remote AT Command
		
		case 0x01 : // TX Transmit Request 16-bit
		case 0x08 : // AT Command
		case 0x09 : // AT Command Queue Register Value
		case 0x80 : // RX Receive Packet: 64-bit
		case 0x81 : // RX Receive Packet: 16-bit
		case 0x82 : // RX Receive Packet: 64-bit IO
		case 0x83 : // RX Receive Packet: 16-bit IO
		case 0x88 : // AT Command Response
		case 0x89 : // TX Transmit Status
		case 0x8A : // Modem Status
		case 0x97 : // Remote AT Command Response

		default: // Error zum speichern setzten
			UART_print("Value not valid! Please chose one of them:\r\n");
			UART_print("00 : TX (Transmit) Request 64-bit address\r\n");
			UART_print("01 : not implemented\r\n");
			UART_print("08 : not implemented\r\n");
			UART_print("09 : not implemented\r\n");
			UART_print("17 : Remote AT Command\r\n");
			UART_print("80 : not implemented\r\n");
			UART_print("81 : not implemented\r\n");
			UART_print("82 : not implemented\r\n");
			UART_print("83 : not implemented\r\n");
			UART_print("88 : not implemented\r\n");
			UART_print("89 : not implemented\r\n");
			UART_print("8A : not implemented\r\n");
			UART_print("97 : not implemented\r\n");
		break;
	}
	
	
	/* Step 2: setup and send package
	 */
	trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON);
	if (tx_stat.in_progress == FALSE)
	{
#if DEBUG
		UART_printf(">TX FRAME tx: %4d, fail: %3d, tx_seq: %3d\r\n", tx_stat.cnt, tx_stat.fail, send[2]);
#endif
		/* some older SPI transceivers require this coming from RX_AACK*/
		trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);
		trx_reg_write(RG_TRX_STATE, CMD_TX_ARET_ON);

		send[2] = tx_stat.cnt;
		
#if DEBUG
	UART_print(">Send: ");
	for (int i=0; i<pos; i++)
	{
		UART_printf("%02x ", send[i], i);
	}
	UART_print("\r\n");
#endif

		trx_frame_write(pos + 2, send);
		tx_stat.in_progress = TRUE;
		TRX_SLPTR_HIGH();
		TRX_SLPTR_LOW();
		
	}
	if (rx_stat.done)
	{
		rx_stat.done = FALSE;
#if DEBUG
		UART_printf("<RX FRAME rx: %4d, fail: %3d, rx_seq: %3d\r\n", rx_stat.cnt, rx_stat.fail, rx_stat.seq);
#endif
	}
	
	//trx_reg_write(RG_TRX_STATE, CMD_RX_ON); 
	
	if		( tx_stat.fail ) { return TRANSMIT_OUT_FAIL; }
	else if ( rx_stat.fail ) { return TRANSMIT_IN_FAIL; }
	else					 { return OP_SUCCESS; }
}

/* 
 * Simple text frame 
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/11/01
 */

int TRX_msgFrame(uint8_t *send)
{
	int pos;
	/* Step 1: prepare packed
	 * - set IEEE data frames
	 * - set sequence counter
	 * - write destination PANID
	 * - write extended dest. address
	 * - write src. short address
	 */	
	
	sprintf((char*)(send+ 0),"%c",0x61);						// IEEE 802.15.4 FCF: 
	sprintf((char*)(send+ 1),"%c",0x8c);						// data frame  
	sprintf((char*)(send+ 2),"%c",0x0);							// sequence counter 
	
	sprintf((char*)(send+ 3),"%c",RFmodul.netCMD_id & 0xff);
	sprintf((char*)(send+ 4),"%c",RFmodul.netCMD_id >>  8);		// destination PAN_ID
	
	sprintf((char*)(send+ 5),"%c",RFmodul.netCMD_dl >>  0);
	sprintf((char*)(send+ 6),"%c",RFmodul.netCMD_dl >>  8);
	sprintf((char*)(send+ 7),"%c",RFmodul.netCMD_dl >> 16);
	sprintf((char*)(send+ 8),"%c",RFmodul.netCMD_dl >> 24);		// destination ext. addr. low 
	
	sprintf((char*)(send+ 9),"%c",RFmodul.netCMD_dh >>  0);
	sprintf((char*)(send+10),"%c",RFmodul.netCMD_dh >>  8);
	sprintf((char*)(send+11),"%c",RFmodul.netCMD_dh >> 16);
	sprintf((char*)(send+12),"%c",RFmodul.netCMD_dh >> 24);		// destination ext. addr. high 
	
	sprintf((char*)(send+13),"%c",RFmodul.netCMD_my & 0xff);
	sprintf((char*)(send+14),"%c",RFmodul.netCMD_my >> 8);		// src. short address
	sprintf((char*)(send+15),"%c",0x0);					
	sprintf((char*)(send+16),"%c",0x0);							// I do not know in which relation this value stands, but it is in all test 04
	pos += 16;
	do
	{
		pos +=1;
		BufferOut(&UART_deBuf, send + pos );

	} while ( 0xD != *(send + pos - 1) );
	
	return pos;
}

/* 
 * AT Remote Command request 
 * 
 * - TODO - sending ACK by receiving Remote Response
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/11/01
 */

int TRX_atRemoteFrame(uint8_t *send)
{
	int pos;
	/* Step 1: prepare packed
	 * - set IEEE data frames
	 * - set sequence counter
	 * - write destination PANID
	 * - write extended dest. address
	 * - write src. short address
	 */	
	
	sprintf((char*)(send+ 0),"%c",0x61);
	sprintf((char*)(send+ 1),"%c",0x8c);
	sprintf((char*)(send+ 2),"%c",0x0);
	
	sprintf((char*)(send+ 3),"%c",RFmodul.netCMD_id & 0xff);
	sprintf((char*)(send+ 4),"%c",RFmodul.netCMD_id >>  8);		// destination PAN_ID
	
	sprintf((char*)(send+ 5),"%c",RFmodul.netCMD_dl >>  0);
	sprintf((char*)(send+ 6),"%c",RFmodul.netCMD_dl >>  8);
	sprintf((char*)(send+ 7),"%c",RFmodul.netCMD_dl >> 16);
	sprintf((char*)(send+ 8),"%c",RFmodul.netCMD_dl >> 24);		// destination ext. addr. low
	
	sprintf((char*)(send+ 9),"%c",RFmodul.netCMD_dh >>  0);
	sprintf((char*)(send+10),"%c",RFmodul.netCMD_dh >>  8);
	sprintf((char*)(send+11),"%c",RFmodul.netCMD_dh >> 16);
	sprintf((char*)(send+12),"%c",RFmodul.netCMD_dh >> 24);		// destination ext. addr. high
	
	sprintf((char*)(send+13),"%c",RFmodul.netCMD_my & 0xff);
	sprintf((char*)(send+14),"%c",RFmodul.netCMD_my >> 8);		// src. short address
	
	sprintf((char*)(send+15),"%c",0xB5);				
	sprintf((char*)(send+16),"%c",0x04);						// I do not know in which relation this line stands, but it is in all test 04
	
	// begin data
	sprintf((char*)(send+17),"%c",0x01);						// Frame ID
	
	sprintf((char*)(send+18),"%c",RFmodul.netCMD_dh >> 24);
	sprintf((char*)(send+19),"%c",RFmodul.netCMD_dh >> 16);
	sprintf((char*)(send+20),"%c",RFmodul.netCMD_dh >>  8);
	sprintf((char*)(send+21),"%c",RFmodul.netCMD_dh >>  0);
	sprintf((char*)(send+22),"%c",RFmodul.netCMD_dl >> 24);
	sprintf((char*)(send+23),"%c",RFmodul.netCMD_dl >> 16);
	sprintf((char*)(send+24),"%c",RFmodul.netCMD_dl >>  8);
	sprintf((char*)(send+25),"%c",RFmodul.netCMD_dl >>  0);		// destination long addr.
	
	sprintf((char*)(send+26),"%c",0xff);
	sprintf((char*)(send+27),"%c",0xfe);						// destination short addr.
	
	sprintf((char*)(send+28),"%c",0x02);						// cmd option
	pos = 28;
	// content	
	do
	{
		pos +=1;
		BufferOut(&UART_deBuf, send + pos );

	} while ( 0xD != *(send + pos) );
	
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
 * last modified: 2016/11/08
 */


ATERROR TRX_receive(void)
{
	uint8_t	outchar		= 0;	// received the data of the buffer
	uint8_t flen		= 0;	// total length of the frame which is stored in the buffer
	uint8_t dataStart	= 0;	// start pointer to print the data through UART
	uint16_t frameType	= 0;	// received the frame type for frame handling
	
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
		if ( i == 0 ) { frameType  = (uint16_t) outchar << 8; }
		if ( i == 1 ) { frameType |= (uint16_t) outchar; }
	}
	
	switch (frameType)
	{
	case 0x618c : //UART_printf(">Frame Type: %"PRIx16"\r\n", frameType);
		dataStart = 0xF;
				// work in progress
		break;
		
	default :
		break;
	}
		
	for (uint8_t i = 0; i < flen-0x2; i++)
	{
		cli(); BufferOut(&RX_deBuf, &outchar); sei();
		
		if ( ( i == dataStart || (API && i <= dataStart) ) && 0xD != outchar )
		{
			UART_printf("%c", outchar);
		}
		if ( 0xD == outchar ) 
		{ 
			UART_print("\r\n");
		}
	}
	
	//if (API) { UART_print("\r\n"); }
	
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
 * Handler for tx and rx frames
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/11/01
 */
static void TRX_txHandler()
{	
	static volatile uint8_t trac_status;
	trac_status = trx_bit_read(SR_TRAC_STATUS);
	
	trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);
	trx_reg_write(RG_TRX_STATE, CMD_TX_ARET_ON);
	
	tx_stat.in_progress = FALSE;
	
	if (TRAC_SUCCESS != trac_status)
	{
		tx_stat.fail++;
	}
	else
	{
		tx_stat.cnt++;
	}
	
	trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON);
}

static void TRX_rxHandler()
{
	uint8_t flen = 0, receive[PACKAGE_SIZE];
	
	flen = trx_frame_read(&receive[0], sizeof(receive), NULL);
	if ( 0x7F < flen ) // 0x7f == 127(dez)
	{
		UART_print("buffer overflow\r\n");
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
#if defined(TRX_IF_RFA1)
ISR(TRX24_TX_END_vect)
{
	TRX_txHandler();
}

ISR(TRX24_RX_END_vect)
{
	TRX_rxHandler();
}
#else  /* !RFA1 */
ISR(TRX_IRQ_vect)
{
	static volatile uint8_t irq_cause;
	irq_cause = trx_reg_read(RG_IRQ_STATUS);	
	
	if (irq_cause & TRX_IRQ_TRX_END)
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