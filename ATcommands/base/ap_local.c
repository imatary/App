/*
 * ap_local.c
 *
 * Created: 25.11.2016 14:37:35
 *  Author: TOE
 */
// === includes ===========================================
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <avr/interrupt.h>

#include "../header/rfmodul.h"					// RF module information
#include "../header/ap_frames.h"				// AP frame struct + AP TRX functions
#include "../header/circularBuffer.h"			// UART & RX buffer
#include "../header/execute.h"
#include "../header/rfinterfacing.h"
#include "../../ATuracoli/stackrelated.h"		// UART_print(f), TRX_send(f)
#include "../../ATuracoli/stackdefines.h"		// defined register addresses
#include "../../ATuracoli/stackrelated_timer.h"

// === std. defines & frame types =========================
#define STD_DELIMITER	      0x7E
#define TX_MSG_64		      0x00
#define TX_MSG_16		      0x01
#define AT_COMMAND		      0x08
#define AT_COMMAND_Q	      0x09
#define REMOTE_AT_CMD	      0x17
#define DEVICE_AT_CMD	      0x18
#define RX_MSG_64		      0x80
#define RX_MSG_16		      0x81
#define REMOTE_RESPONSE       0x97

#define ACK_WITH_MAXSTREAM    0x0
#define NO_ACK_NO_MAXSTREAM   0x1
#define ACK_NO_MAXSTREAM	  0x2
#define NO_ACK_WITH_MAXSTREAM 0x3

// === globals ============================================
static uint8_t outchar[256];
static at_status_t ret;
static CMD *pCommand  = NULL;
static uint32_t writetimer = 0;

// === Prototypes =========================================
static at_status_t AP_getCommand( bufType_n bufType, CMD **cmd );
static void		   AP_atLocal_response(void);

static at_status_t AP_localDevice(bufType_n bufType);
static uint32_t    AP_write_timedEEPROM(uint32_t arg);
// === functions (local handling shared) ==================
/*
 * AP_frameHandle_uart
 * is called when the AP mode is active and the standard delimiter was recognized
 *
 * Received:
 *		size_t the length of input
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/02/08
 */
void AP_frameHandle_uart(bufType_n bufType)
{
	if ( 4 > GET_apFrameLength() )
	{
		deBufferReadReset(bufType, '+', GET_apFrameLength() );
		return;
	}

	// frame type	1 byte
	ret = deBufferOut( bufType, &outchar[0] );
	if ( BUFFER_OUT_FAIL == ret )
	{
		deBufferReset( bufType );
		return;
	}
	SET_apFrameType(outchar[0]);

	// frame id		1 byte
	ret = deBufferOut( bufType, &outchar[0] );
	if ( BUFFER_OUT_FAIL == ret )
	{
		deBufferReset( bufType );
		return;
	}
	SET_apFrameID(outchar[0]);

	switch ( GET_apFrameType() )
	{
		case AT_COMMAND    :
		case AT_COMMAND_Q  :
			{
				ret	= AP_getCommand( bufType, &pCommand );
				if ( INVALID_COMMAND == ret )
				{
					SET_apFrameRet(INVALID_COMMAND);
					AP_atLocal_response();
					deBufferReadReset(bufType, '+', GET_apFrameLength() );
					return;
				}
				if ( 4 == GET_apFrameLength() && EXEC & pCommand->rwxAttrib )
				{
					ret = AP_exec( pCommand->ID );
				}
				else if ( 4 == GET_apFrameLength() && READ & pCommand->rwxAttrib )
				{
					ret = AP_read( pCommand );
				}
				else
				{
					ret = AP_write( bufType, pCommand );
					if ( OP_SUCCESS == ret && AT_COMMAND == GET_apFrameType() )
					{
						writetimer = deTIMER_start(AP_write_timedEEPROM, 0x10, 0 );
						apply_changes(NULL);
					}
				}
				SET_apFrameRet(ret);
				AP_atLocal_response();
				pCommand = NULL;
			}
			break;

		case REMOTE_AT_CMD :
			{
				TRX_send( bufType, REMOTE_AT_CMD, NULL, 0);
			}
			break;

		case DEVICE_AT_CMD :
			{
				ret = AP_localDevice( bufType );
				SET_apFrameRet(ret);
				AP_atLocal_response();
			}
			break;

		case TX_MSG_64 :
			{
				TRX_send( bufType, TX_MSG_64, NULL, 0);
			}
			break;

		case TX_MSG_16 :
			{
				TRX_send( bufType, TX_MSG_16, NULL, 0);
			}
			break;

		default :
			{
				SET_apFrameRet(INVALID_COMMAND);
				AP_atLocal_response();
			}
			break;
	}
}



/*
 * Get command
 * - reads a string from buffer
 * - searched for the command in the command table
 * - store it into pCommand pointer
 *
 * Received:
 *		bufType_n	number of buffer type
 *		CMD			pointer for address in command table
 *
 * Returns:
 *     OP_SUCCESS		on success
 *	   INVALID_COMMAND	if command is not in command table
 *
 * last modified: 2017/01/26
 */
static at_status_t AP_getCommand( bufType_n bufType, CMD **cmd )
{
	outchar[0] = 'A';
	outchar[1] = 'T';
	deBufferOut( bufType, &outchar[2] );
	deBufferOut( bufType, &outchar[3] );
	if ( 'a' <= outchar[2] && 'z' >= outchar[2] ) outchar[2] -= 0x20;
	if ( 'a' <= outchar[3] && 'z' >= outchar[3] ) outchar[3] -= 0x20;
	SET_apFrameATcmd( &outchar[2] );

	*cmd = CMD_findInTable(outchar);
	return ( NO_AT_CMD == (*cmd)->ID || NULL == *cmd )? INVALID_COMMAND : OP_SUCCESS;
}



/*
 * Specific device commands (AP)
 * interpret the received frame and execute the specific task
 *
 * Received:
 *		api_f		pointer to the frame struct
 *		uint8_t		pointer to an array which is already initialized and available for processing (just to save a little mem)
 *		size_t		the length of the whole input, which is stored in buffer
 *
 * Returns:
 *		OP_SUCCESS			no error has occurred
 *		ERROR				calculated crc does not match with received crc
 *		INVALID_COMMAND		command is not in the table
 *		INVALID_PARAMETER	parameter is not in a valid range
 *
 * last modified: 2017/01/26
 */
static at_status_t AP_localDevice(bufType_n bufType)
{
	// AT command 2 bytes
	outchar[0] = 'D';
	outchar[1] = 'E';
	deBufferOut( bufType, &outchar[2] );
	deBufferOut( bufType, &outchar[3] );
	if ( 'a' <= outchar[2] && 'z' >= outchar[2] ) outchar[2] -= 0x20;
	if ( 'a' <= outchar[3] && 'z' >= outchar[3] ) outchar[3] -= 0x20;
	SET_apFrameATcmd( &outchar[2] );
	SET_apFrameCRC( outchar[2] + outchar[3], true );

	pCommand = CMD_findInTable(outchar);
	if ( NO_AT_CMD == pCommand->ID || NULL == pCommand ) return INVALID_COMMAND;

	/*
	 * handle CMD
	 *
	 * frame length
	 * EXEC is allowed
	 */
	if ( 4 == GET_apFrameLength() && EXEC & pCommand->rwxAttrib )
	{
		SET_apFrameRWXopt(EXEC);

		switch( pCommand->rwxAttrib )
		{
			/* no exec functions right now */
			default: return INVALID_COMMAND;
		}
	}
	/*
	 * frame length
	 * READ is allowed
	 */
	else if ( 4 == GET_apFrameLength() && READ & pCommand->rwxAttrib )
	{
		SET_apFrameRWXopt(READ);

		switch ( pCommand->ID )
		{
/* FV */	case DE_PT :
					SET_apFrameLength( strlen(AT_VERSION), FALSE );
					uint8_t *val = AT_VERSION;
					SET_apFrameMsg( val, strlen(AT_VERSION)+1, DE_PT);
				break;

			default:
				return INVALID_COMMAND;
		}
	}
	/*
	 * WRITE is allowed, store the value of the string into RFmodel struct and register
	 * frame length
	 * string length of input
	 * writing to RFmodul struct [and to EEPROM] is allowed
	 */
	else if ( 4 < GET_apFrameLength() && WRITE & pCommand->rwxAttrib )
	{
		size_t cmdSize = GET_apFrameLength() - 4;
		SET_apFrameRWXopt(WRITE);

		switch( pCommand->ID )
		{
			/* no write functions right now */
			default :  return INVALID_COMMAND;
		}
	}

	return OP_SUCCESS;
}



/*
 * AP AT local response
 * generated the output AP frame with the return value
 * of the specific function in AP_frameHandle_uart function
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/26
 */
static void AP_atLocal_response(void)
{
	/*
	 * if the read option set and the return value OP_SUCCESS
	 * add 5 to length for frame type, id, command and return value
	 *
	 * else set frame length to 5
	 */
	uint16_t length = GET_apFrameLength();
	if ( READ       == GET_apFrameRWXopt() &&\
	     OP_SUCCESS == GET_apFrameRet() )      length += 5;
	else									   length  = 5;

	UART_putc( STD_DELIMITER );					// start delimiter
	UART_putc( (uint8_t) (length >> 8) );		// frame length
	UART_putc( (uint8_t) (length & 0xFF) );
	UART_putc( 0x88 );							// frame type
	UART_putc( GET_apFrameID() );

	GET_apFrameATcmd(outchar, 0);
	UART_putc( outchar[0] );					// AT cmd
	UART_putc( outchar[1] );
	UART_putc( GET_apFrameRet() * (-1) );	    // cmd option (successful/ not successful etc.)

	//			  Type +  Frame ID       +  AT Command             +   Return Value              //
	uint8_t crc = 0x88 + GET_apFrameID() + outchar[0] + outchar[1] + ( GET_apFrameRet() * (-1) );

	if ( READ       == GET_apFrameRWXopt() &&\
	     OP_SUCCESS == GET_apFrameRet() )
	{
		length -= 5;
		GET_apFrameMsg(outchar, 0, length);
		for (uint8_t x = 0; x < length; x++)
		{
			UART_putc( outchar[x] );
			crc += outchar[x];
		}
	}
	UART_putc( 0xFF - crc);					// checksum
}



/*
 * The tx status frame,
 * generated the output AP frame with the status return value
 * of the transmit 16/64 frame
 *
 * received:
 *		uint8_t		transmit status value
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/02/10
 */
void AP_txStatus(at_status_t status)
{
	/*
	 * std. delimiter
	 */
	UART_putc( STD_DELIMITER );

	/*
	 * frame length, constant 3 bytes
	 */
	UART_putc( 0x0 );
	UART_putc( 0x3 );

	/*
	 * frame  type
	 */
	UART_putc( 0x89 );

	/*
	 * frame ID
	 */
	UART_putc( GET_apFrameID() );

	uint8_t crc = 0x89 + GET_apFrameID();

	/*
	 * status value
	 */
	switch (status)
	{
		case OP_SUCCESS			: UART_putc( 0x00 ); crc += 0x00; break;	// success
		case TRANSMIT_OUT_FAIL	: UART_putc( 0x01 ); crc += 0x01; break;	// No acknowledgment received

		/* TODO other cases need to be added */
		default					: UART_putc( 0x85 ); crc += 0x85; break;	// Unknown ERROR
	}

	UART_putc(0xFF - crc);
}



/*
 * RX Receive Packet Frame,
 * prints the received package as an AP frame
 *
 * RX frame sample:
 * |<----------------- dataStart length -------------->|
 *  61 8C xx 16 DE 27 C3 00 FF FF 2E 21 00 | 0E BE | 0B 00 | 00 4D 65 73 73 61 67 65 | xx xx |
 *                                         |< src >|<- * ->|<----- data payload ---->|< crc >|
 *                                                     * MaxStream header (optional)
 *
 * API frame:
 *         |<---------------- frame length ----------------->|
 * 7E 00 0C 81 | 0E BE |  18   |  01  | 4D 65 73 73 61 67 65 |  02
 *             |< src >|< RSSI + opt >|<--- data payload --->|< crc >|
 *
 * Received:
 *		uint8_t		pointer to the srcAddrAndOtion array
 *		uint8_t		length of src. addr.
 *
 * Returns:
 *     nothing
 *
 * last modified: 2017/02/13
 */
void AP_rxReceive( bufType_n bufType, uint16_t length, uint8_t dataStart, uint8_t srcAddrLen )
{
	/*
	 * Frame length calculation:
	 * length = total received frame length - (payload start position - 1) - 2 bytes crc + 1 byte frame type + 1 (RSSI) + src addr length
	 */
	uint8_t tmp;
	uint16_t frLength = length - dataStart + srcAddrLen - 1;

	/*
	 * std. delimiter
	 */
	UART_putc( STD_DELIMITER );

	/*
	 * frame length
	 */
	UART_putc( (uint8_t) (frLength >> 8) );
	UART_putc( (uint8_t) (frLength & 0xFF) );

	/*
	 * frame type
	 */
	if ( srcAddrLen == 8 )
	{
		UART_putc( 0x08 );				// 64 bit src addr
		SET_apFrameCRC( 0x80, FALSE );
	}
	else
	{
		UART_putc( 0x81 );				// 16 bit src addr
		SET_apFrameCRC( 0x81, FALSE );
	}

	/*
	 * source address
	 */
	for ( uint8_t i = 0; i < srcAddrLen ; i++ )
	{
		tmp = GET_deBufferByteAt( bufType, dataStart-i-1);
		UART_putc( tmp );
		SET_apFrameCRC( tmp, TRUE );
	}

	/*
	 * RSSI value
	 */
	tmp = (90 - 3 * ((TRX_readReg(PHY_RSSI) & 0x1F)-1));
	UART_putc( tmp );
	SET_apFrameCRC( tmp, TRUE );

	/*
	 * option [0x1] Address broadcast / [0x2] PAN broadcast
	 */
	tmp = GET_deBufferByteAt( bufType, dataStart+1);
	if ( 0x4 == tmp )
	{
		UART_putc( 0x2 );
		SET_apFrameCRC( 0x2, TRUE);
	}
	else
	{
		UART_putc( 0x1 );
		SET_apFrameCRC( 0x1, TRUE);
	}

	/*
	 * if Mac Mode (MM) equal to ACK_WITH_MAXSTREAM or NO_ACK_WITH_MAXSTREAM, + 2 bytes for MaxStream header
	 */
	if ( ACK_WITH_MAXSTREAM == GET_netCMD_mm() || NO_ACK_WITH_MAXSTREAM == GET_netCMD_mm() ) dataStart += 2;

	/*
	 * data payload
	 */
	for ( uint16_t i = 0; i < frLength-srcAddrLen-3; i++ )
	{
		tmp = GET_deBufferByteAt( bufType, dataStart+i);
		UART_putc( tmp );
		SET_apFrameCRC( tmp, TRUE);
	}

	/*
	 * checksum
	 */
	UART_putc( GET_apFrameCRC() );
	deBufferReset( bufType );
}



/*
 * AT Remote Command (local execution),
 * reads from RX buffer, prepared the response frame struct and send response
 *
 * RX frame sample:
 * |<------------------ dataStart length ------------->|   |<----------------- 14 bytes -------------------->|
 *  61 8C 6C 16 DE C0 40 46 41 00 A2 13 00 | E0 BE | 0E 04 | 01 | 00 13 A2 00 41 46 40 C0 FF FE | 02 | 53 4C | 99 C6
 *                                         |< src >|       |<ID>|<-------- dest, addr --------->|<* >|< cmd >|
 *																	                              * option
 *
 * Received:
 *		uint8_t		pointer to the workArray
 *		uint16_t	length of the received AP Frame
 *		uint8_t		length of src. addr.
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/02/13
 */
void AP_atRemoteFrame_localExec(bufType_n bufType, uint16_t length, uint8_t dataStart, uint8_t srcAddrLen)
{
	uint8_t srcAddr[8];
	uint8_t mm_option = GET_netCMD_mm();
	/*
	 * - Frame consists a minimum of 14 payload bytes
	 * - read src address for response
	 * - read AP Frame ID (1 byte)
	 * - read dest. addr. (10 bytes) (not important) but need to be deleted
	 * - read remote option and store it temporary in frame.type
	 * - copy command information into UART buffer
	 * - delete crc checksum
	 * - call rx | w function
	 * - if remote option is 0x2 reinitialize module config
	 * - call response function
	 */

	deBufferReadReset( bufType, '+', dataStart - srcAddrLen );
	READ_deBufferData_atReadPosition( bufType, srcAddr, srcAddrLen );
	if ( ACK_WITH_MAXSTREAM == mm_option || NO_ACK_WITH_MAXSTREAM == mm_option )
	{
		deBufferReadReset(bufType, '+', 2 );
		dataStart += 2;
	}

	SET_apFrameID  ( GET_deBufferByteAt(bufType,  0 ) );
	SET_apFrameType( GET_deBufferByteAt(bufType, 11 ) ); // the frame type variable will be used for option

	deBufferReadReset(bufType, '+', 12 );

	length -= (dataStart+12); // length = 2 dummy bytes + 2 bytes AT cmd [+ n-bytes payload] (min 4 Bytes)

	ret	= AP_getCommand( bufType, &pCommand );
	if ( INVALID_COMMAND == ret )
	{
		SET_apFrameRet(ret);
		TRX_send( NONE, REMOTE_RESPONSE, srcAddr, srcAddrLen );
		return;
	}

	if ( 4 == length && EXEC & pCommand->rwxAttrib )
	{
		ret = AP_exec( pCommand->ID );
	}
	else if ( 4 == length && READ & pCommand->rwxAttrib )
	{
		ret = AP_read( pCommand );
	}
	else
	{
		SET_apFrameLength(length, FALSE);
		ret = AP_write( bufType, pCommand );
	}
	SET_apFrameRet(ret);

	/*
	 * if the apply option and some parameter are received, reconfigure the device
	 */
	if ( OP_SUCCESS == ret &&\
	     0x2 == GET_apFrameType() &&\
		 WRITE == GET_apFrameRWXopt() &&\
		 AT_AP != pCommand->ID )
	{
		/*
		 * reinitialize user interface
		 */
		if ( (DIRTYB_BD & dirtyBits) != FALSE )
		{
			UART_baudInit();
			dirtyBits ^= DIRTYB_BD;
		}

		/*
		 * reinitialize transceiver
		 */
		if ( (DIRTYB_CH & dirtyBits) != FALSE ||\
		     (DIRTYB_ID & dirtyBits) != FALSE )
		{
			TRX_baseInit();
			dirtyBits ^= (DIRTYB_CH | DIRTYB_ID);

			if ( (DIRTYB_PL & dirtyBits) != FALSE ) dirtyBits ^= DIRTYB_PL;
		}

		/*
		 * reinitialize transceiver power level
		 */
		if ( (DIRTYB_PL & dirtyBits) != FALSE )
		{
			CMD *cmd = CMD_findInTableByID(AT_PL);
			uint8_t pwl;
			GET_deviceValue( (uint8_t*) &pwl, cmd);

			config_powerlevel(pwl);
			dirtyBits ^= DIRTYB_PL;
		}

		/*
		 * recalculate at next call Packetization Timeout
		 */
		if ( (dirtyBits & DIRTYB_RO) == 0 )
		{
			dirtyBits ^= DIRTYB_RO;
		}

	}

	TRX_send( NONE, REMOTE_RESPONSE, srcAddr, srcAddrLen );
	pCommand = NULL;
}



/*
 * AT Remote Response Frame (read frame)
 * prints the response of an AT Remote Command
 *
 * RX frame sample:
 * |<----- dataStart length ---->|   |<------------- API frame payload length + 1 ------------------>|
 *  61 88 5B 16 DE E0 BE E1 BE 09 05 | 01 | 00 13 A2 00 41 46 40 C0 BE E1 | 53 4C | 00 | 41 46 40 C0 | 2C BF
 *                                   |<ID>|<--------- src addr ---------->|< cmd >|<* >|<- [param] ->|< crc >|
 *                                                                                  * return value
 *
 * received:
 *		uint16_t	length of data content
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/02/13
 */
void AP_atRemote_response(bufType_n bufType, uint16_t length, uint8_t dataStart)
{
	uint8_t  outchar;
	/*
	 * Frame length calculation:
	 * total received frame length - payload start position - 2 bytes crc - 2 bytes MaxStream header + 1 byte frame type
	 */
	uint16_t frLength = length - dataStart - 3;

	UART_putc( STD_DELIMITER );						// start delimiter
	UART_putc( (uint8_t) (frLength >> 8) );			// frame length
	UART_putc( (uint8_t) (frLength & 0xFF) );
	UART_putc( 0x97 );								// frame type
	uint8_t crc = 0x97;

	deBufferReadReset( bufType, '+', dataStart + 2 );
	for (uint16_t i = 0; i < frLength-1; i++)
	{
		deBufferOut( bufType, &outchar );
		UART_putc( outchar );
		crc += outchar;
	}
	UART_putc(0xFF - crc);							// checksum

	deBufferReset( bufType );
}



/*
 * timer for writing operation
 *
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2017/03/03
 */
static uint32_t AP_write_timedEEPROM(uint32_t arg)
{
	SET_userValInEEPROM();
	return FALSE;
}