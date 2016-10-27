/*
 * trx0.c
 *
 * Created: 26.10.2016 09:07:31
 *  Author: TOE
 */ 

#include "../ATcommands/header/_global.h"
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
 * last modified: 2016/10/27
 */

static ATERROR TRX_sendAction()
{
	uint8_t send[PACKAGE_SIZE];

	/* Step 1: prepare packed
	 * - set IEEE data frames
	 * - set sequence counter
	 * - write short destination PANID
	 * - write short source ID
	 * - set TX fail counter
	*/	
	send[0] = 0x21;						/* IEEE 802.15.4 FCF: */
	send[1] = 0x08;						/* data frame with ack request */
	send[2] =   42;						/* sequence counter */
	send[3] = (netCMD.id & 0xff);
	send[4] = (netCMD.id >> 8);			/* destination PAN_ID */
	send[5] = (netCMD.my & 0xff);
	send[6] = (netCMD.my >> 8),			/* src. short address */
	send[7] =   42;						/* TX fail counter */
	
	for ( int pos = 8; 0xD != send[pos]; pos++)
	{
		BufferOut(UART_Xbuf, &send[pos]);
		#if DEBUG
			UART_printf("%c",send[pos]);
		#endif
	}
	
	/* Step 2: send package
	 */

	if (tx_stat.in_progress == FALSE)
	{
		UART_printf(">TX FRAME tx: %4d, fail: %3d, tx_seq: %3d\n", tx_stat.cnt, tx_stat.fail, send[2]);
		/* some older SPI transceivers require this coming from RX_AACK*/
		trx_reg_write(RG_TRX_STATE, CMD_PLL_ON);
		trx_reg_write(RG_TRX_STATE, CMD_TX_ARET_ON);

		send[2] = tx_stat.cnt;
		send[7] = tx_stat.fail;
		send[8 + i] = 0;
		UART_printf("\rSend: %s\n\r", send);
		trx_frame_write(10 + i, send);
		tx_stat.in_progress = TRUE;
		TRX_SLPTR_HIGH();
		TRX_SLPTR_LOW();
	}
	if (rx_stat.done)
	{
		rx_stat.done = FALSE;
		UART_printf("<RX FRAME rx: %4d, fail: %3d, rx_seq: %3d\n", rx_stat.cnt, rx_stat.fail, rx_stat.seq);
	}
	
	return OP_SUCCESS;
	
}

/* setup transmitter 
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
static void TRX_setup()
{
#if DEBUG
	UART_printf("Set channel:  0x%x\n\r",netCMD.ch);
	UART_printf("Set PAN ID:   0x%x\n\r",netCMD.id);
	UART_printf("Set short ID: 0x%x\n\r",netCMD.my);
	UART_print("RX_AACK_ON ... ok\n\r");				// TODO muss umstellbar sein
#endif
	
	trx_bit_write(SR_CHANNEL,netCMD.ch);
	trx_bit_write(SR_TX_AUTO_CRC_ON,1);
		
	trx_reg_write(RG_PAN_ID_0,(netCMD.id & 0xff));
	trx_reg_write(RG_PAN_ID_1,(netCMD.id >> 8  ));

	trx_reg_write(RG_SHORT_ADDR_0,(netCMD.my & 0xff));
	trx_reg_write(RG_SHORT_ADDR_1,(netCMD.my >> 8  ));

	trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON);
	
	
	#if defined(TRX_IRQ_TRX_END)
		trx_reg_write(RG_IRQ_MASK,TRX_IRQ_TRX_END);
	#elif defined(TRX_IRQ_RX_END)
		trx_reg_write(RG_IRQ_MASK,TRX_IRQ_TX_END | TRX_IRQ_RX_END);
	#else
	#  error "Unknown IRQ bits"
	#endif
}

/*
 * IRQ functions
 *
 *
 */
#if defined(TRX_IF_RFA1)
ISR(TRX24_TX_END_vect)
{

}

ISR(TRX24_RX_END_vect)
{

}
#else  /* !RFA1 */
ISR(TRX_IRQ_vect)
{
	tx_stat.in_progress = FALSE;
	rx_stat.done = TRUE;
}
#endif  /* RFA1 */