/*
 * trx0.c
 *
 * Created: 26.10.2016 09:07:31
 *  Author: TOE
 */ 

#include "../header/_global.h"
#include "stackrelated.h"

#include "board.h"

txStatus tx_stat = {0,FALSE,FALSE};
rxStatus rx_stat = {0,0,FALSE,FALSE};

void trx_send_action(int i, int *data)
{
	uint8_t send[256];

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
	
	for (int k=8;k < (i + 8) ;k++)
	{
		send[k] = *data;
		UART_printf("%c", *data++);
	}

#ifdef DEBUG
	UART_print("\rPrepare: ");
	for (int k=8;k < (i + 8) ;k++)
	{
		UART_printf("%c",send[k]);
	}
	UART_print("\r");
#endif

	/* Step 2: setup transmitter 
     * - configure radio channel
     * - enable transmitters automatic crc16 generation
     * - go into RX AACK state,
     * - configure address filter
     * - enable "receive end" IRQ
     */
	UART_printf("Set channel: 0x%x\n\r",netCMD.ch);
    trx_bit_write(SR_CHANNEL,netCMD.ch);
    trx_bit_write(SR_TX_AUTO_CRC_ON,1);
	
	UART_printf("Set PAN ID: 0x%x\n\r",netCMD.id);
    trx_reg_write(RG_PAN_ID_0,(netCMD.id & 0xff));
    trx_reg_write(RG_PAN_ID_1,(netCMD.id >> 8  ));

    trx_reg_write(RG_SHORT_ADDR_0,(netCMD.my & 0xff));
    trx_reg_write(RG_SHORT_ADDR_1,(netCMD.my >> 8  ));

    trx_reg_write(RG_TRX_STATE, CMD_RX_AACK_ON);
    UART_print("RX_AACK_ON ... ok\n\r");
	
	#if defined(TRX_IRQ_TRX_END)
	trx_reg_write(RG_IRQ_MASK,TRX_IRQ_TRX_END);
	#elif defined(TRX_IRQ_RX_END)
	trx_reg_write(RG_IRQ_MASK,TRX_IRQ_TX_END | TRX_IRQ_RX_END);
	#else
	#  error "Unknown IRQ bits"
	#endif

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
	
}

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