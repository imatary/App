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

txStatus_t tx_stat = {0,FALSE,FALSE};
rxStatus_t rx_stat = {0,0,FALSE,FALSE};

/*
 * TRX_sendAction
 *
 * Returns:
 *     XX       buffer is empty. he cannot send a byte.
 *     SUCCESS			package was delivered
 * 
 * last modified: 2016/10/28
 */

ATERROR TRX_send()
{
	uint8_t send[PACKAGE_SIZE] = {0};
	int pos = 14;

	/* Step 1: prepare packed
	 * - set IEEE data frames
	 * - set sequence counter
	 * - write destination PANID
	 * - write extended dest. address
	 * - write src. short address
	 */	
	
	send[ 0] = 0x41;						// IEEE 802.15.4 FCF: 
	send[ 1] = 0x8C;						// data frame  
	send[ 2] =   0;							// sequence counter 
	
	send[ 3] = (uint8_t)(netCMD.id & 0xff);
	send[ 4] = (uint8_t)(netCMD.id >>  8);	// destination PAN_ID 
	
	send[ 5] = (uint8_t)(netCMD.dl >>  0);
	send[ 6] = (uint8_t)(netCMD.dl >>  8);
	send[ 7] = (uint8_t)(netCMD.dl >> 16);
	send[ 8] = (uint8_t)(netCMD.dl >> 24);	// destination ext. addr. low 
	
	send[ 9] = (uint8_t)(netCMD.dh >>  0);
	send[10] = (uint8_t)(netCMD.dh >>  8);
	send[11] = (uint8_t)(netCMD.dh >> 16);
	send[12] = (uint8_t)(netCMD.dh >> 24);	// destination ext. addr. high 
	
	send[13] = (uint8_t)(netCMD.my & 0xff);
	send[14] = (uint8_t)(netCMD.my >> 8);	// src. short address
	//send[7] =   42;						// TX fail counter

	do 
	{
		pos +=1;
		BufferOut(&UART_deBuf, &send[pos]);
		
#if DEBUG
		UART_printf("%c", send[pos]);
#endif

	} while ( 0xD != send[pos-1] );
	
	/* Step 2: setup and send package
	 */
	if (tx_stat.in_progress == FALSE)
	{
		UART_printf(">TX FRAME tx: %4d, fail: %3d, tx_seq: %3d\n\r", tx_stat.cnt, tx_stat.fail, send[2]);
		/* some older SPI transceivers require this coming from RX_AACK*/
		trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);
		trx_reg_write(RG_TRX_STATE, CMD_TX_ARET_ON);

		send[2] = tx_stat.cnt;
		//send[7] = tx_stat.fail;
		
#if DEBUG
	UART_print(">Send: ");
	for (int i=0; i<pos; i++)
	{
		UART_printf("%02x ", send[i], i);
	}
	UART_print("\n\r");
#endif

		trx_frame_write(pos + 2, send);
		tx_stat.in_progress = TRUE;
		TRX_SLPTR_HIGH();
		TRX_SLPTR_LOW();
	}
	if (rx_stat.done)
	{
		rx_stat.done = FALSE;
		UART_printf("<RX FRAME rx: %4d, fail: %3d, rx_seq: %3d\n", rx_stat.cnt, rx_stat.fail, rx_stat.seq);
	}
	
	if		( tx_stat.fail ) { return TRANSMIT_OUT_FAIL; }
	else if ( rx_stat.fail ) { return TRANSMIT_IN_FAIL; }
	else					 { return OP_SUCCESS; }
}

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
	trx_bit_write(SR_CHANNEL,netCMD.ch);
	trx_bit_write(SR_TX_AUTO_CRC_ON,TRUE);
		
	TRX_setPanId(netCMD.id);										// target PAN ID
	TRX_setLongAddr( (uint64_t) netCMD.sh<<32 | netCMD.sl );		// destination extended address

	trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON);					// TODO should be changeable 
	trx_reg_write(RG_TRX_STATE,CMD_TX_ARET_ON);

	#if defined(TRX_IRQ_TRX_END)
		trx_reg_write(RG_IRQ_MASK,TRX_IRQ_TRX_END);
	#elif defined(TRX_IRQ_RX_END)
		trx_reg_write(RG_IRQ_MASK,TRX_IRQ_TX_END | TRX_IRQ_RX_END);
	#else
	#  error "Unknown IRQ bits"
	#endif
	MCU_IRQ_ENABLE();
}

/*
 * IRQ functions
 *
 *
 */
#if defined(TRX_IF_RFA1)
ISR(TRX24_TX_END_vect)
{
	tx_stat.in_progress = FALSE;
	rx_stat.done = TRUE;
}

ISR(TRX24_RX_END_vect)
{
	tx_stat.in_progress = FALSE;
	rx_stat.done = TRUE;
}
#else  /* !RFA1 */
ISR(TRX_IRQ_vect)
{
	tx_stat.in_progress = FALSE;
	rx_stat.done = TRUE;
}
#endif  /* RFA1 */