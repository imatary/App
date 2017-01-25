/*
 * ap_local.c
 *
 * Created: 25.11.2016 14:37:35
 *  Author: TOE
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <avr/interrupt.h>

#include "../header/rfmodul.h"					// RF module information
#include "../header/atlocal.h"					// AT command functions
#include "../header/apiframe.h"					// AP frame struct + AP TRX functions
#include "../header/circularBuffer.h"			// UART & RX buffer
#include "../../ATuracoli/stackrelated.h"		// UART_print(f), TRX_send(f)
#include "../../ATuracoli/stackdefines.h"		// defined register addresses

// === std. defines & frame types =========================
#define STD_DELIMITER	(0x7E)
#define TX_MSG_64		(0x00)
#define TX_MSG_16		(0x01)
#define AT_COMMAND		(0x08)
#define AT_COMMAND_Q	(0x09)
#define REMOTE_AT_CMD	(0x17)
#define DEVICE_AT_CMD	(0x18)
#define RX_MSG_64		(0x80)
#define RX_MSG_16		(0x81)
#define REMOTE_RESPONSE (0x97)

// === Prototypes =========================================
static at_status_t AP_localDevice(bufType_n bufType);
static void		   AP_atLocal_response(void);
static void        swap_msg(size_t length);

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
 * last modified: 2017/01/19
 */
void AP_frameHandle_uart(bufType_n bufType)
{
	// frame type	1 byte
	cli(); frame.ret = deBufferOut( bufType, &frame.type ); sei();
	if (frame.ret == BUFFER_OUT_FAIL) return;
	
	// frame id		1 byte
	cli(); frame.ret = deBufferOut( bufType, &frame.id ); sei();
	if (frame.ret == BUFFER_OUT_FAIL) return;
	
	switch ( frame.type )
	{
		case AT_COMMAND    :
		case AT_COMMAND_Q  :
			{
				static CMD *pCommand  = NULL;
				frame.ret	= CMD_getCommand( bufType, pCommand );
				if ( INVALID_COMMAND == frame.ret ) 
				{
					AP_atLocal_response(); 
					return;
				}
				
				if ( 4 == frame.length && EXEC == pCommand->rwxAttrib )
				{
					frame.ret = CMD_exec( NULL, pCommand->ID );
				}
				else if ( 4 == frame.length && READ == pCommand->rwxAttrib )
				{
					
					frame.ret = CMD_read( pCommand );
				}
				else
				{
					frame.rwx = WRITE;
					frame.ret = CMD_write( frame.length, bufType, pCommand );
					if ( AT_COMMAND == frame.type )
					{
						SET_userValInEEPROM();
						UART_init();
						TRX_baseInit();
						SET_serintCMD_ap( GET_atAP_tmp() );
						SET_atcopCMD_ct ( GET_atCT_tmp() );
					}
				}
			}
			break;
		
		case REMOTE_AT_CMD : 
			{
				TRX_send( bufType, REMOTE_AT_CMD, NULL, 0); 
			}
			break;
		
		case DEVICE_AT_CMD :
			{
				frame.ret = AP_localDevice( bufType );
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
				frame.ret = INVALID_COMMAND;
			}
			break;
			
	}
	
	AP_atLocal_response();
}


/*
 * AT Remote Command (local execution)
 * reads from RX buffer, prepared the response frame struct and send response
 *
 * Received:
 *		uint16_t	length of the received AP Frame
 *		uint8_t		pointer to the srcAddrAndOtion array
 *		uint8_t		length of src. addr.
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/04
 */
void AP_atRemoteFrame_localExec(bufType_n bufType, uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen)
{
	/* 
	 * - Frame consist a minimum of 14 Bytes
	 * - read AP Frame ID (1 byte)
	 * - read dest. addr. (10 bytes) (not important) but need to be deleted
	 * - read remote option and store it temporary in frame.type
	 * - copy command information into UART buffer
	 * - delete crc checksum
	 * - call rx | w function
	 * - if remote option is 0x2 reinitialize module config
	 * - call response function 
	 */
	uint8_t outchar; 

	for ( uint16_t i = 0; i < 0xC; i++ )
	{
		cli(); deBufferOut( bufType, &outchar ); sei();
		if		( 0x0 == i ) { frame.id = outchar; }
		else if ( 0xB == i ) { frame.type = outchar; } // the frame type variable will be used for option
	}
	
	length -= 0xA; // length = 2 dummy bytes + 2 bytes AT cmd [+ n-bytes payload]
	
	static CMD *pCommand  = NULL;
	frame.ret	= CMD_getCommand( bufType, pCommand );
	if ( INVALID_COMMAND == frame.ret )
	{
		TRX_send( NONE, REMOTE_RESPONSE, srcAddr, srcAddrLen );
		return;
	}
	
	if ( 4 == frame.length && EXEC == pCommand->rwxAttrib )
	{
		frame.ret = AP_exec( NULL, pCommand->ID );
	}
	else if ( 4 == frame.length && READ == pCommand->rwxAttrib )
	{
		frame.ret = AP_read( pCommand );
	}
	else
	{
		frame.ret = AP_write( frame.length, bufType, pCommand );
	}
	
	/*
	 * if the apply option and some parameter are received, reconfigure the device
	 */
	if ( 0x2 == frame.type && WRITE == frame.rwx )
	{
		SET_userValInEEPROM();
		UART_init();
		TRX_baseInit();
		SET_serintCMD_ap( GET_atAP_tmp() );
		SET_atcopCMD_ct ( GET_atCT_tmp() );
	}

	TRX_send( NONE, REMOTE_RESPONSE, srcAddr, srcAddrLen );
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
 * last modified: 2017/01/19
 */
static at_status_t AP_localDevice(bufType_n bufType)
{	
	// AT command 2 bytes
	uint8_t pCmdString[5] = {'D','E',0,0,0};

	cli(); deBufferOut( bufType, &pCmdString[2] ); sei();
	cli(); deBufferOut( bufType, &pCmdString[3] ); sei();
	
	frame.cmd[0] = pCmdString[2];
	frame.cmd[1] = pCmdString[3];
	
	// search for CMD in table
	CMD *workPointer = CMD_findInTable(pCmdString);
	
	/*
	 * handle CMD
	 *
	 * frame length
	 * EXEC is allowed
	 */
	if ( frame.length == 4 && workPointer->rwxAttrib & EXEC )
	{
		frame.rwx = EXEC;
		
		switch( workPointer->rwxAttrib )
		{
			/* no exec functions right now */
			default: return INVALID_COMMAND;
		}
	}
	/*
	 * frame length
	 * READ is allowed
	 */
	else if ( frame.length == 4 && workPointer->rwxAttrib & READ )
	{
		frame.rwx = READ;
	
		switch ( workPointer->ID )
		{
/* FV */	case DE_FV :
					frame.length = 1;
					memcpy( frame.msg, AT_VERSION, strlen(AT_VERSION)+1 );
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
	else if ( frame.length > 4 && workPointer->rwxAttrib & WRITE )
	{
		size_t cmdSize = frame.length - 4;
		frame.rwx = WRITE;
		
		switch( workPointer->ID )
		{
			/* no write functions right now */
			default :  return INVALID_COMMAND;
		}
	}
	else
	{
		return INVALID_COMMAND;
	}
	return OP_SUCCESS;
	
}

/*
 * RX Receive Packet Frame
 * prints the received package as an AP frame
 *
 * Received:
 *		uint8_t		pointer to the srcAddrAndOtion array
 *		uint8_t		length of src. addr.
 *
 * Returns:
 *     nothing
 *
 * last modified: 2017/01/03
 */
void AP_rxReceive( bufType_n bufType, uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen )
{
	uint8_t  outchar;
	uint16_t frLength = length + 3 + srcAddrLen;			// +2 (frame type) +1 (RSSI) +1 (Option) + src addr length
	
	UART_putc( STD_DELIMITER );								// start delimiter
	UART_putc( (uint8_t) (frLength >> 8) );					// frame length
	UART_putc( (uint8_t) (frLength & 0xFF) );
	if ( srcAddrLen == 8 )									// frame type
	{
		UART_putc( 0x80 );
		frame.crc = 0x80;
	} 
	else
	{
		UART_putc( 0x81 );
		frame.crc = 0x81;
	}

	for ( short i = srcAddrLen-1; i >= 0 ; i-- )			// Addr.
	{
		UART_putc( *(srcAddr+i) );
		frame.crc += *(srcAddr+i);
	}
	
	uint8_t x = TRX_readReg(PHY_RSSI) & 0x1F;
	UART_putc( x );	
	frame.crc += x;
	
	UART_putc( *(srcAddr+srcAddrLen+1) );					// Option
	frame.crc +=     *(srcAddr+srcAddrLen+1);
				
	for (uint16_t i; i < length; i++ )
	{
		cli(); deBufferOut( bufType, &outchar ); sei();
		UART_putc( outchar );
		frame.crc += outchar;
	}

	UART_putc( 0xFF - frame.crc );							// checksum

}

/*
 * AP AT local response
 * generated the output AP frame with the return value of the specific function in AP_frameHandle_uart function
 * if the specific device value of return to uart (DERU) true, it will be printed a detailed message
 * else the the frame will be written to the uart
 *
 * Received:
 *		api_f	the AP information frame struct
 *
 * Returns:
 *		nothing
 *
 * last modified: 2016/11/30
 */
static void AP_atLocal_response(void)
{
	if ( READ == frame.rwx && 0 == frame.ret ) frame.length += 5;
	else									   frame.length  = 5;
		
	UART_putc( STD_DELIMITER );						// start delimiter
	UART_putc( (uint8_t) (frame.length >> 8) );		// frame length
	UART_putc( (uint8_t) (frame.length & 0xFF) );
	UART_putc( 0x88 );								// frame type
	UART_putc( frame.id );
	UART_putc( frame.cmd[0] );						// AT cmd
	UART_putc( frame.cmd[1] );
	UART_putc( frame.ret * (-1) );					// cmd option (successful/ not successful etc.)
		
	//			 Type +  Frame ID +  AT Command                   +   Return Value      //
	frame.crc = 0x88 + frame.id + frame.cmd[0] + frame.cmd[1] + (frame.ret * (-1));
	
	if ( READ == frame.rwx )
	{
		frame.length -= 5;
		for (uint8_t x = 0; x < frame.length; x++)
		{
			UART_putc( frame.msg[x] );
			frame.crc += frame.msg[x];
		}
	}
	UART_putc( 0xFF - frame.crc);					// checksum
}

/*
 *
 * received:
 *		uint8_t		transmit status value
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/09
 */
void AP_txStatus(at_status_t status)
{
	UART_putc( STD_DELIMITER );						// start delimiter
	UART_putc( 0x0 );								// frame length
	UART_putc( 0x3 );
	UART_putc( 0x89 );
	UART_putc( frame.id );
	
	frame.crc = 0x89 + frame.id;
	
	switch (status)
	{
		case OP_SUCCESS			: UART_putc( 0x00 ); frame.crc += 0x00; break;	// success
		case TRANSMIT_OUT_FAIL	: UART_putc( 0x01 ); frame.crc += 0x01; break;	// No acknowledgment received
		
		/* TODO other cases need to be added */
		default					: UART_putc( 0x85 ); frame.crc += 0x85; break;	// Unknown ERROR
	}
	
	UART_putc(0xFF - frame.crc);
}

/*
 * AT Remote Response Frame (read frame)
 * prints the response of an AT Remote Command
 *
 * received:
 *		uint16_t	length of data content
 *
 * Returns:
 *		nothing
 *
 * last modified: 2016/12/14
 */
void AP_atRemote_response(bufType_n bufType, uint16_t length)
{
	uint8_t  outchar;
	uint16_t frLength = length + 1;							// +1 for frame type
	
	UART_putc( STD_DELIMITER );								// start delimiter
	UART_putc( (uint8_t) (frLength >> 8) );					// frame length
	UART_putc( (uint8_t) (frLength & 0xFF) );
	UART_putc( 0x97 );										// frame type
	frame.crc = 0x97;
	
	for (uint16_t i = 0; i < length; i++)
	{
		cli(); deBufferOut( bufType, &outchar ); sei();
		UART_putc( outchar );
		frame.crc += outchar;
	}
	UART_putc(0xFF - frame.crc);							// checksum

}