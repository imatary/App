/*
 * trx0.c
 *
 * Created: 26.10.2016 09:07:31
 *  Author: TOE
 */ 

#include "../ATcommands/header/_global.h"
#include "../ATcommands/header/circularBuffer.h"
#include "stackrelated.h"

#include "board.h"

txStatus_t tx_stat = {1,FALSE,FALSE};
rxStatus_t rx_stat = {0,0,FALSE,FALSE};

/* 
 * Setup transmitter 
 * - configure radio channel
 * - enable transmitters automatic crc16 generation
 * - go into RX AACK state,
 * - configure address filter
 * - enable "receive end" IRQ
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/10/27
 */
void TRX_setup()
{	
	trx_io_init(SPI_RATE_1_2);
	trx_bit_write(SR_CHANNEL,netCMD.ch);
	trx_bit_write(SR_TX_AUTO_CRC_ON,TRUE);
		
	//TRX_setPanId(netCMD.id);										// target PAN ID
	//TRX_setLongAddr( (uint64_t) netCMD.dh<<32 | netCMD.dl );		// destination extended address

	trx_reg_write(RG_TRX_STATE, CMD_RX_ON); 

	#if defined(TRX_IRQ_TRX_END)
		trx_reg_write(RG_IRQ_MASK,TRX_IRQ_TRX_END);
	#elif defined(TRX_IRQ_RX_END)
		trx_reg_write(RG_IRQ_MASK,TRX_IRQ_RX_END | TRX_IRQ_TX_END);
	#else
	#  error "Unknown IRQ bits"
	#endif
	MCU_IRQ_ENABLE();
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

ATERROR TRX_send()
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
		UART_printf(">TX FRAME tx: %4d, fail: %3d, tx_seq: %3d\r\n", tx_stat.cnt, tx_stat.fail, send[2]);
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
		UART_printf("<RX FRAME rx: %4d, fail: %3d, rx_seq: %3d\r\n", rx_stat.cnt, rx_stat.fail, rx_stat.seq);
	}
	
	trx_reg_write(RG_TRX_STATE, CMD_RX_ON); 
	
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
	
	sprintf((char*)(send+ 0),"%c",0x61);				// IEEE 802.15.4 FCF: 
	sprintf((char*)(send+ 1),"%c",0x8c);				// data frame  
	sprintf((char*)(send+ 2),"%c",0x0);					// sequence counter 
	
	sprintf((char*)(send+ 3),"%c",netCMD.id & 0xff);
	sprintf((char*)(send+ 4),"%c",netCMD.id >>  8);		// destination PAN_ID
	
	sprintf((char*)(send+ 5),"%c",netCMD.dl >>  0);
	sprintf((char*)(send+ 6),"%c",netCMD.dl >>  8);
	sprintf((char*)(send+ 7),"%c",netCMD.dl >> 16);
	sprintf((char*)(send+ 8),"%c",netCMD.dl >> 24);		// destination ext. addr. low 
	
	sprintf((char*)(send+ 9),"%c",netCMD.dh >>  0);
	sprintf((char*)(send+10),"%c",netCMD.dh >>  8);
	sprintf((char*)(send+11),"%c",netCMD.dh >> 16);
	sprintf((char*)(send+12),"%c",netCMD.dh >> 24);		// destination ext. addr. high 
	
	sprintf((char*)(send+13),"%c",netCMD.my & 0xff);
	sprintf((char*)(send+14),"%c",netCMD.my >> 8);		// src. short address
	sprintf((char*)(send+15),"%c",0x0);					
	sprintf((char*)(send+16),"%c",0x0);					// I do not know in which relation this value stands, but it is in all test 04
	pos += 16;
	do
	{
		pos +=1;
		BufferOut(&UART_deBuf, send + pos );
		
#if DEBUG
		UART_printf("%c", *(send + pos) );
#endif

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
	
	sprintf((char*)(send+ 3),"%c",netCMD.id & 0xff);
	sprintf((char*)(send+ 4),"%c",netCMD.id >>  8);		// destination PAN_ID
	
	sprintf((char*)(send+ 5),"%c",netCMD.dl >>  0);
	sprintf((char*)(send+ 6),"%c",netCMD.dl >>  8);
	sprintf((char*)(send+ 7),"%c",netCMD.dl >> 16);
	sprintf((char*)(send+ 8),"%c",netCMD.dl >> 24);		// destination ext. addr. low
	
	sprintf((char*)(send+ 9),"%c",netCMD.dh >>  0);
	sprintf((char*)(send+10),"%c",netCMD.dh >>  8);
	sprintf((char*)(send+11),"%c",netCMD.dh >> 16);
	sprintf((char*)(send+12),"%c",netCMD.dh >> 24);		// destination ext. addr. high
	
	sprintf((char*)(send+13),"%c",netCMD.my & 0xff);
	sprintf((char*)(send+14),"%c",netCMD.my >> 8);		// src. short address
	
	// begin data
	sprintf((char*)(send+15),"%c",0xB5);				// tx counter - this value = 4C
	sprintf((char*)(send+16),"%c",0x04);				// I do not know in which relation this line stands, but it is in all test 04
	
	sprintf((char*)(send+17),"%c",0x01);				// Frame ID
	
	sprintf((char*)(send+18),"%c",netCMD.dh >> 24);
	sprintf((char*)(send+19),"%c",netCMD.dh >> 16);
	sprintf((char*)(send+20),"%c",netCMD.dh >>  8);
	sprintf((char*)(send+21),"%c",netCMD.dh >>  0);
	sprintf((char*)(send+22),"%c",netCMD.dl >> 24);
	sprintf((char*)(send+23),"%c",netCMD.dl >> 16);
	sprintf((char*)(send+24),"%c",netCMD.dl >>  8);
	sprintf((char*)(send+25),"%c",netCMD.dl >>  0);		// destination long addr.
	
	sprintf((char*)(send+26),"%c",0xff);
	sprintf((char*)(send+27),"%c",0xfe);				// destination short addr.
	
	sprintf((char*)(send+28),"%c",0x02);				// cmd option
	pos = 28;
	// content	
	do
	{
		pos +=1;
		BufferOut(&UART_deBuf, send + pos );
#if DEBUG
		UART_printf("%c", *(send + pos) );
#endif

	} while ( 0xD != *(send + pos) );
	
	return pos;
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
	
	tx_stat.in_progress = FALSE;
	
	if (TRAC_SUCCESS != trac_status)
	{
		tx_stat.fail++;
		UART_print("tx fail\r\n");
	}
	else
	{
		tx_stat.cnt++;
		UART_print("tx success\r\n");
	}
}

static void TRX_rxHandler()
{
	uint8_t counter, flen = 0, *pfrm;
	static uint8_t buf[PACKAGE_SIZE] = {0};
	uint16_t crc;
	pfrm = buf;
	rx_stat.done = TRUE;
	
	/*
	 * upload frame and check for CRC16 validity 
	 */
	counter = flen = trx_frame_read(pfrm, sizeof(buf), NULL);
	crc = 0;
	do
	{
		crc = _crc_ccitt_update(crc, *pfrm++);
	}
	while(flen--);
	
	/* 
	 * if crc is correct and frame larger then 5 bytes, update RX frame counter, send ACK and print to UART 
	 * if crc is correct and frame equal 5 bytes, packet is a ACK package
	 * else a error is occurred
	 */
	if ( crc == 0 && counter > 5 )
	{
		rx_stat.cnt += 1;
		rx_stat.seq  = buf[2];
UART_printf("<RX FRAME rx: %4d, fail: %3d, rx_seq: %3d\r\n", rx_stat.cnt, rx_stat.fail, rx_stat.seq);
		UART_printf("%c",buf[counter-3]);
		//TRX_ack();
	}
	else if ( crc == 0 && counter == 5 )
	{
		// ACK check
	}
	else
	{
		UART_print("Receiver error!\r\n");
		rx_stat.fail++;
	}	
	UART_print("\r\n");
}

void TRX_ack(void)
{
	uint8_t send[5];
	send[0] = 0x2;
	send[1] = 0x0;
	send[2] = rx_stat.seq;
	send[3] = 0x1;
	send[4] = 0x1;
	trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);

	trx_frame_write(5, send);
	trx_reg_write(RG_TRX_STATE, CMD_RX_ON); 
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