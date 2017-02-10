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
#include "../ATcommands/header/ap_frames.h"
#include "../ATcommands/header/at_commands.h"
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

// === structs & var init =================================
/*
 * transceiver (trx) status structs
 */
typedef struct {
	uint8_t  cnt;
	uint16_t fail;
	uint8_t  senderInfo;
	bool_t   in_progress;
}txStatus_t;

typedef struct {
	uint8_t cnt;
	uint8_t seq;
	bool_t fail;
	bool_t done;
}rxStatus_t;

static txStatus_t tx_stat = {1,0,0,FALSE};
static rxStatus_t rx_stat = {0,0,FALSE,FALSE};
static uint8_t    send[PACKAGE_SIZE] = {0};

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
uint8_t ret;
uint8_t channel;
uint16_t panid, shortaddr;
uint32_t extaddrhigh, extaddrlow;
uint8_t TRX_baseInit(void)
{

	CMD *cmd;

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

	/* catch all needed data */
	cmd = CMD_findInTableByID(AT_CH);
	GET_deviceValue( (uint8_t*) &channel, cmd);

	cmd = CMD_findInTableByID(AT_ID);
	GET_deviceValue( (uint16_t*) &panid, cmd);

	cmd = CMD_findInTableByID(AT_SH);
	GET_deviceValue( (uint32_t*) &extaddrhigh, cmd);

	cmd = CMD_findInTableByID(AT_SL);
	GET_deviceValue( (uint32_t*) &extaddrlow, cmd);

	cmd = CMD_findInTableByID(AT_MY);
	GET_deviceValue( (uint16_t*) &shortaddr, cmd);

	/* set bits and bytes */
	TRX_writeBit(deSR_CHANNEL, channel );
	TRX_writeBit(deSR_TX_AUTO_CRC_ON, TRUE);

	TRX_setPanId( panid );								            // target PAN ID
	TRX_setLongAddr( (uint64_t) extaddrhigh << 32 | extaddrlow );	// device long address
	TRX_setShortAddr( shortaddr );							        // short address

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
	int pos = 0;

	send[2] = tx_stat.cnt;	// set frame counter

	/*
	 * Handle buffer dependent on AP mode on or off and return pointer position in the array
	 */
	if ( TRANSPARENT_MODE == GET_serintCMD_ap() )
	{
		pos = TRX_msgFrame( bufType, send );		// AT TX Transmit Request
	}
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
		if ( TRANSPARENT_MODE != GET_serintCMD_ap() ) AP_txStatus(TRANSMIT_OUT_FAIL);
		return;
	}

	/*
	 * Step 2: setup and send workArray
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
 * TRX_receive()
 * translated received workArrays
 *
 * Returns:
 *     OP_SCCESS			frame is received successfully without error
 *	   TRANSMIT_IN_FAIL		frame is received and an error has occurred
 *
 * last modified: 2017/01/03
 */
at_status_t TRX_receive(bufType_n bufType)
{
	static uint8_t flen	= 0;					// total length of the frame which is stored in the buffer
	static uint8_t dataStart  = 0;				// start position of data payload
	static uint8_t srcAddrLen = 0;				// source address length

	rx_stat.done = FALSE;
	SET_deBufferNewContent( bufType, FALSE);
	SET_diagCMD_db( TRX_readReg(PHY_RSSI) & 0x1F );

	/*
	 * read the len out of the buffer
	 * copy RX buffer into work buffer
	 */
	deBufferOut( bufType, &flen );
	CPY_deBufferData( RX_WORK_BUF, bufType, flen);

	/*
	 * - get the frame type
	 * - set the data start pointer depending on frame type
	 * - print the data
	 *
	 *  61 8C 6C 16 DE C0 40 46 41 00 A2 13 00 E0 BE 0E 04 01 00 13 A2 00 41 46 40 C0 FF FE 02 53 4C 99 C6
	 * | MAC |                                | src |---------------- data -------------------------| crc |
	 *       |-------------- 13 bytes --------------|
	 */
	switch ( GET_deBufferByteAt(RX_WORK_BUF, 1) ) // address fields length
	{
		case 0x88 : dataStart = 0x09; srcAddrLen = 2; break; // dest 16 & src 16 -> 4  bytes + (dest PAN ID + Frame counter) 3 bytes + 2 bytes MAC header =  9
		case 0x8C : dataStart = 0x0F; srcAddrLen = 2; break; // dest 64 & src 16 -> 10 bytes + (dest PAN ID + Frame counter) 3 bytes + 2 bytes MAC header = 15
		case 0xC8 : dataStart = 0x0F; srcAddrLen = 8; break; // dest 16 & src 64 -> 10 bytes + (dest PAN ID + Frame counter) 3 bytes + 2 bytes MAC header = 15
		case 0xCC : dataStart = 0x15; srcAddrLen = 8; break; // dest 64 & src 64 -> 16 bytes + (dest PAN ID + Frame counter) 3 bytes + 2 bytes MAC header = 21
		default:
			rx_stat.fail++;
			return TRANSMIT_IN_FAIL;
	}

	if ( TRANSPARENT_MODE == GET_serintCMD_ap() )
	{
		TRX_printContent( RX_WORK_BUF, flen, dataStart );
	}
	else
	{
		switch ( 0x48 & GET_deBufferByteAt(RX_WORK_BUF, 0) ) // define MAC options
		{
			case 0x40 : TRX_createAPframe( RX_WORK_BUF, flen, dataStart,   srcAddrLen, 0x40 ); break;	// security disabled, PAN compression  enabled = add nothing
			case 0x00 : TRX_createAPframe( RX_WORK_BUF, flen, dataStart+2, srcAddrLen, 0x00 ); break;	// security disabled, PAN compression disabled = add 2 bytes for src PAN ID
			case 0x08 : TRX_createAPframe( RX_WORK_BUF, flen, dataStart+5, srcAddrLen, 0x08 ); break;	// security  enabled, PAN compression disabled = add 5 bytes for Auxiliary Sec. Header
			case 0x48 : TRX_createAPframe( RX_WORK_BUF, flen, dataStart+7, srcAddrLen, 0x48 ); break;	// security  enabled, PAN compression  enabled = add 7 bytes for src PAN ID and Auxiliary Sec. header
			default:
				rx_stat.fail++;
				return TRANSMIT_IN_FAIL;
		}
	}

	rx_stat.cnt += 1;
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
			SET_diagCMD_ea(tx_stat.fail);
			if ( TRANSPARENT_MODE > GET_serintCMD_ap() ) AP_txStatus(TRANSMIT_OUT_FAIL);
			else UART_print_status(TRANSMIT_OUT_FAIL);
			break;

			default:
			tx_stat.fail++;
			SET_diagCMD_ea(tx_stat.fail);
			if ( TRANSPARENT_MODE < GET_serintCMD_ap() ) AP_txStatus(TRANSMIT_OUT_FAIL);
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
	deBufferIn( bufType, flen);

	/*
	 * upload the4 frame into RX buffer
	 */
	for (uint8_t i = 0; i < flen; i++)
	{
		deBufferIn( bufType, receive[i] );
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