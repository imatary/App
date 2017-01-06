/*
 * apiframe.c
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

// === Prototypes =========================================
static at_status_t AP_0x18_localDevice(size_t *len);
static void		   AP_0x88_atLocal_response(struct api_f *frame);
static void		   AP_0x97_atRemote_response(uint16_t length);
static void		   AP_0x80_0x81_rxReceive( uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen );
static void		   AP_0x17_atRemoteFrame(uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen);

// === globals ============================================
struct api_f frame  = {0,0,0,0,{0},0,{0},0};

// === Functions (local handling shared) ==================
// ===
// ========================================================
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
 * last modified: 2016/12/19
 */
void AP_frameHandle_uart(void)
{
	uint8_t  outchar[5]	= {0x0};
	
	// Start delimiter	1 byte	
	cli(); BufferOut( &UART_deBuf, &outchar[0] ); sei();
	if ( outchar[0] != STD_DELIMITER ) 
	{
		BufferInit(&UART_deBuf, NULL);
		return;
	}
	
	// frame->bufLength	2 byte
	cli(); BufferOut( &UART_deBuf, &outchar[0] ); sei();
	cli(); BufferOut( &UART_deBuf, &outchar[1] ); sei();
	frame.length = (uint16_t) outchar[0] << 2 | outchar[1] ;
	
	// frame type	1 byte
	cli(); BufferOut( &UART_deBuf, &frame.type ); sei();
	
	// frame id		1 byte
	cli(); BufferOut( &UART_deBuf, &frame.id ); sei();
	frame.crc = frame.id;
	
	switch ( frame.type )
	{
		case AT_COMMAND    :
			frame.crc += 0x08;
			if ( frame.length == 4)
			{
				frame.ret = CMD_readOrExec(NULL, TRUE);
			}
			else
			{
				frame.ret = CMD_write( (size_t*) &frame.length, TRUE);
				SET_userValInEEPROM();
			}
			AP_0x88_atLocal_response( &frame );
		break;
		
		case AT_COMMAND_Q  : 
			frame.crc += 0x09;
			if ( frame.length == 4)
			{
				frame.ret = CMD_readOrExec(NULL, TRUE);
			}
			else
			{
				frame.ret = CMD_write((size_t*) &frame.length, TRUE);
			}
			AP_0x88_atLocal_response( &frame );
		break;
		
		case REMOTE_AT_CMD : 
			frame.crc += 0x17;
			frame.ret = TRX_send(0x17, NULL, 0); 
		break;
		
		case DEVICE_AT_CMD :
			frame.crc += 0x18;
			frame.ret = AP_0x18_localDevice((size_t*) &frame.length);
			AP_0x88_atLocal_response( &frame );
		break;
				
		default : frame.ret = INVALID_COMMAND;
	}
}

/*
 * AP set AT command stored the AT CMD into frame struct for response 
 *
 * Received:
 *		uint8_t pointer to the array which hold the AT command line
 *
 * Returns:
 *	   nothing
 *
 * last modified: 2016/12/19
 */
void AP_setATcmd(uint8_t *array)
{
	frame.crc	+= (*(array+2) + *(array+3));
	frame.cmd[0] =  *(array+2);
	frame.cmd[1] =  *(array+3);
}

/*
 * AP set AT read, write or execute value into frame struct for response 
 *
 * Received:
 *		uint8_t value of READ, WRITE or EXEC
 *
 * Returns:
 *	   nothing
 *
 * last modified: 2016/12/19
 */
void AP_setRWXopt(uint8_t opt)
{
	frame.rwx = opt;
}

/*
 * AP set AT command stored the AT CMD for the response in the frame struct
 *
 * Received:
 *		void	pointer to the variable which hold the parameter value
 *		short	size of the parameter
 *		uint8_t boolean value whether the message array should be swapped or not
 *
 * Returns:
 *	   nothing
 *
 * last modified: 2016/12/19
 */
void AP_setMSG(void *val, short length, uint8_t swapp)
{
	frame.msg[length+1] = 0x0;
	frame.length = length;
	memcpy(frame.msg, val, length);
	if ( length > 1 && swapp == 1 )
	{
		for (short i = 0; i < frame.length/2; i++, length--)
		{
			frame.msg[i]        ^= frame.msg[length-1];
			frame.msg[length-1] ^= frame.msg[i];
			frame.msg[i]        ^= frame.msg[length-1];
		}
	}	
}

/*
 * The AP compare function red the user crc value from the buffer
 * and compared it with the calculated crc sum
 *
 * Received:
 *		api_f	the AP frame to store important informations for the response
 *
 * Returned:
 *		TRUE	if calculated crc equal to user crc
 *		FALSE	if calculated crc is not equal to user crc
 *
 * last modified: 2016/11/29
 */
bool_t AP_compareCRC(void)
{	
	uint8_t userCRC;
	cli(); BufferOut( &UART_deBuf, &userCRC); sei();
	return ( (uint8_t) (0xFF - frame.crc) == userCRC )? TRUE : FALSE;	
}

/*
 * The AP update CRC function adds the value of val to the crc checksum
 *
 * Received:
 *		uint8_t	pointer to an unit8_t value
 *
 * Returned:
 *		nothing
 *
 * last modified: 2016/12/19
 */
void AP_updateCRC(uint8_t *val)
{
	frame.crc += *val;
}





// === Functions (local handling static) ==================
// ===
// ========================================================
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
static void AP_0x17_atRemoteFrame(uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen)
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
	for ( uint16_t i = 0; i < length; i++ )
	{
		cli(); BufferOut( &RX_deBuf, &outchar ); sei();
		if		( 0x0 == i ) { frame.id = outchar; }
		else if ( 0xB == i ) { frame.type = outchar; }										// the frame type variable will be used for option
		else if ( 0xB <  i ) { cli(); BufferIn( &UART_deBuf, outchar ); sei(); }
	}
	deBufferReadReset(&RX_deBuf, '+', 2);													// delete Frame Checksum

	length -= 12;
	if ( length > 2 ) { frame.ret = CMD_write( (size_t*) &length, frame.type ); } 
	else              { frame.ret = CMD_readOrExec( NULL, frame.type ); }

	if ( 0x2 == frame.type && frame.rwx == WRITE )
	{
		UART_init();
		TRX_baseInit();
	}

	TRX_send( 0x97, srcAddr, srcAddrLen );
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
 * last modified: 2016/12/07
 */
static at_status_t AP_0x18_localDevice(size_t *len)
{	
	// AT command 2 bytes
	uint8_t pCmdString[5] = {'D','E',0,0,0};

	cli(); BufferOut( &UART_deBuf, &pCmdString[2] ); sei();
	cli(); BufferOut( &UART_deBuf, &pCmdString[3] ); sei();
	
	frame.crc += (pCmdString[2] + pCmdString[3]);
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
		if ( AP_compareCRC() == FALSE ) return ERROR; 
		
		switch( workPointer->rwxAttrib )
		{
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
		uint8_t userCRC;
		if ( AP_compareCRC() == FALSE )
		{
			frame.length = 0;
			return ERROR;
		}
		
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
static void AP_0x80_0x81_rxReceive( uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen )
{
	uint8_t  outchar;
	uint16_t crc, frLength = length + 3 + srcAddrLen;		// +2 (frame type) +1 (RSSI) +1 (Option) + src addr length
	
	UART_putc( STD_DELIMITER );								// start delimiter
	UART_putc( (uint8_t) (frLength >> 8) );					// frame length
	UART_putc( (uint8_t) (frLength & 0xFF) );
	if ( srcAddrLen == 8 )									// frame type
	{
		UART_putc( 0x80 );
		crc = 0x80;
	} 
	else
	{
		UART_putc( 0x81 );
		crc = 0x81;
	}

	for ( short i = srcAddrLen-1; i >= 0 ; i-- )			// Addr.
	{
		UART_putc( *(srcAddr+i) );
		crc += *(srcAddr+i);
	}
	
	uint8_t x = TRX_readReg(PHY_RSSI) & 0x1F;
	UART_putc( x );	
	crc += x;
	
	UART_putc( *(srcAddr+srcAddrLen+1) );					// Option
	crc +=     *(srcAddr+srcAddrLen+1);
				
	for (uint16_t i; i < length; i++ )
	{
		cli(); BufferOut( &RX_deBuf, &outchar ); sei();
		UART_putc( outchar );
		crc += outchar;
	}

	UART_putc( 0xFF - crc );								// checksum
	deBufferReadReset(&RX_deBuf, '+', 2);					// delete Frame Checksum
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
static void AP_0x88_atLocal_response(struct api_f *frame)
{
	if (frame->rwx == EXEC || frame->rwx == WRITE) frame->length = 0;
	frame->length += 5;
	
	UART_putc( STD_DELIMITER );						// start delimiter
	UART_putc( (uint8_t) (frame->length >> 8) );	// frame length
	UART_putc( (uint8_t) (frame->length & 0xFF) );
	UART_putc( 0x88 );								// frame type
	UART_putc( frame->id );
	UART_putc( frame->cmd[0] );						// AT cmd
	UART_putc( frame->cmd[1] );
	UART_putc( frame->ret * (-1) );					// cmd option (successful/ not successful etc.)
		
	//			 Type +  Frame ID +  AT Command                   +   Return Value      //
	frame->crc = 0x88 + frame->id + frame->cmd[0] + frame->cmd[1] + (frame->ret * (-1));
	if ( frame->rwx == READ )
	{
		for (uint16_t x; x < frame->length-5 ; x++ )
		{
			UART_putc(    frame->msg[x] );
			frame->crc += frame->msg[x];
		}
	}
		
	UART_putc( 0xFF - frame->crc);							// checksum
	
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
static void AP_0x97_atRemote_response(uint16_t length)
{
	uint8_t  outchar;
	uint16_t crc, frLength = length + 1;					// +1 for frame type
	
	UART_putc( STD_DELIMITER );								// start delimiter
	UART_putc( (uint8_t) (frLength >> 8) );					// frame length
	UART_putc( (uint8_t) (frLength & 0xFF) );
	UART_putc( 0x97 );										// frame type
	crc = 0x97;
	
	for (uint16_t i = 0; i < length; i++)
	{
		cli(); BufferOut( &RX_deBuf, &outchar ); sei();
		UART_putc( outchar );
		crc += outchar;
	}
	UART_putc(0xFF-crc);									// checksum
	deBufferReadReset(&RX_deBuf, '+', 2);					// delete Frame Checksum
}




// === Functions (transreceiver) ==========================
// ===
// ========================================================
/*
 * TRX_createAPframe
 * creates an AP frame and print it to UART
 *
 * Received:
 *		uint8_t		length of the received frame
 *		uint8_t		position where the data payload starts
 *		uint8_t		length of source address
 *		uint8_t		MAC options
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/12/15
 */
void TRX_createAPframe( uint8_t flen, uint8_t dataStart, uint8_t srcAddrLen, uint8_t option )
{
	/* AES decryption not supported -- if security decryption functions are added delete this upper part */
	if ( option == 0x48 || option == 0x08 ) return;
	/**********************************************************************/
	
	uint8_t outchar, DigiHeader = 0, srcAddrAndOption[10] = {0x0}; // size = max 8 bytes for src address and 2 bytes for Digi data header
	
	/*
	 * - delete  dest. PANID, dest. addr., [src. PANID] (max 12 bytes)
	 * - read src addr (2 or 8 bytes)
	 * - read Digi data header 2 Bytes
	 * - check type of data header and handle data
	 */
	if ( RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x3 ) DigiHeader = 2; // if Digi header included add temporary two bytes for data header 

	for (uint8_t counter = 0, pos = 0; pos < srcAddrLen + DigiHeader && counter < flen; counter++ )
	{
		cli(); BufferOut( &RX_deBuf, &outchar ); sei();
		if ( counter >= dataStart-srcAddrLen )
		{
			srcAddrAndOption[pos] = outchar;
			pos++;
		}
	}
	
	switch ( srcAddrAndOption[srcAddrLen+1] )
	{
		case 0x04 : AP_0x17_atRemoteFrame( (uint16_t) flen-dataStart-4 , srcAddrAndOption, srcAddrLen );  break;	// TX response only
		case 0x05 : AP_0x97_atRemote_response( (uint16_t) flen-dataStart-4 );							  break;	// UART response only
		default   : AP_0x80_0x81_rxReceive( (uint16_t) flen-dataStart-4 , srcAddrAndOption, srcAddrLen ); break;	// UART response only
	}					
	
}

/* 
 * AT Remote Command (remote request)
 * prepared frame for a remote AT command to control another device
 *
 * Received:
 *		uint8_t		pointer to send array
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/12/15
 */
int TRX_0x17_atRemoteFrame(uint8_t *send)
{
	int			pos		= 0;
	at_status_t ret		= 0;
	uint8_t		crc		= 0x17;
	uint8_t		tmp[11] = {0};
	bool_t		flag	= FALSE;
	
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */	
																 *(send) |= 0x01; // send data to _one_ device
	if ( RFmodul.secCMD_ee == TRUE )							 *(send) |= 0x08; // security active
	if ( RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x2 )  *(send) |= 0x20; // ACK on
																 *(send) |= 0x40; // PAN Compression on
	
	sprintf((char*)(send+ 3),"%c",RFmodul.netCMD_id & 0xff);
	sprintf((char*)(send+ 4),"%c",RFmodul.netCMD_id >>  8);				// destination PAN_ID
	pos = 5;
	
	/*
	 * dest. address
	 */;
	for (char i = 0; i < 11; i++ )
	{
		cli(); ret = BufferOut( &UART_deBuf, &tmp[i] ); sei();
		if ( ret == BUFFER_OUT_FAIL ) return 0;
		crc += tmp[i];
	}
	
	if ( tmp[8] < 0xFF || tmp[9] < 0xFE )
	{
		*(send+5) = tmp[9];
		*(send+6) = tmp[8];
		
		*(send+1) |= 0x08;												// MAC header second byte
		pos = 7;
	} 
	else
	{
		*(send+ 5) = tmp[7];
		*(send+ 6) = tmp[6];
		*(send+ 7) = tmp[5];
		*(send+ 8) = tmp[4];											// destination ext. addr. low

		*(send+ 9) = tmp[3];
		*(send+10) = tmp[2];
		*(send+11) = tmp[1];
		*(send+12) = tmp[0];											// destination ext. addr. high
		
		*(send+1) |= 0x0C;												// MAC header second byte
		pos = 13;
	}
	
	/*
	 * src. address
	 */
	if ( RFmodul.netCMD_my > 0x0 )
	{
		sprintf((char*)(send+pos),  "%c",RFmodul.netCMD_my & 0xff);
		sprintf((char*)(send+pos+1),"%c",RFmodul.netCMD_my >> 8);		// src. short address
		
		*(send+1) |= 0x80;												// MAC header second byte
		pos += 2;
	}
	else
	{
		sprintf((char*)(send+pos),  "%c",RFmodul.netCMD_sl >>  0);
		sprintf((char*)(send+pos+1),"%c",RFmodul.netCMD_sl >>  8);
		sprintf((char*)(send+pos+2),"%c",RFmodul.netCMD_sl >> 16);
		sprintf((char*)(send+pos+3),"%c",RFmodul.netCMD_sl >> 24);		// src. ext. addr. low
		
		sprintf((char*)(send+pos+4),"%c",RFmodul.netCMD_sh >>  0);
		sprintf((char*)(send+pos+5),"%c",RFmodul.netCMD_sh >>  8);
		sprintf((char*)(send+pos+6),"%c",RFmodul.netCMD_sh >> 16);
		sprintf((char*)(send+pos+7),"%c",RFmodul.netCMD_sh >> 24);		// src. ext. addr. high
		
		*(send+1) |= 0xC0;												// MAC header second byte
		pos += 8;
	}
	
	sprintf((char*)(send+pos),   "%c", *(send+2) + (RFmodul.netCMD_sl & 0xFF) );	// second counter, distance to frame counter is last byte of src extended addr. 
	sprintf((char*)(send+pos+1), "%c", 0x04 );										// this byte will be used as command, 0x04 AT remote command
	sprintf((char*)(send+pos+2), "%c", frame.id );
	
	/*
	 * data
	 */
	pos += 3;
	for ( char i = 0; i<11; i++, pos++ )
	{
		*(send+pos) = tmp[i];
	}
	
	do
	{
		cli(); ret = BufferOut( &UART_deBuf, send+pos ); sei();
		if ( ret == BUFFER_OUT_FAIL ) break;
		
		crc += *(send+pos);
		pos++;
		
	} while ( pos < PACKAGE_SIZE-1 );
	
	crc = crc - *(send+pos);	// the user crc need to be deleted
	crc = 0xFF - crc;
	
	return pos-1;
}

/*
 * AT Remote Command (response)
 * send result information of received and executed 0x17 Remote Command 
 *
 * Received:
 *		uint8_t		pointer to send array
 *		uint8_t		pointer to source address array
 *		uint8_t		source address length
 *
 * Returns:
 *		final position in array
 *
 * last modified: 2017/01/05
 */
int TRX_0x97_atRemote_response(uint8_t *send, uint8_t *srcAddr, uint8_t srcAddrLen)
{
	int	pos	= 0;
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */
																*(send) |= 0x01; // send data to _one_ device
	if ( RFmodul.secCMD_ee == TRUE )							*(send) |= 0x08; // security active
	if ( RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x2 ) *(send) |= 0x20; // ACK on
																*(send) |= 0x40; // PAN Compression on
	
	sprintf((char*)(send+ 3),"%c",RFmodul.netCMD_id & 0xff);
	sprintf((char*)(send+ 4),"%c",RFmodul.netCMD_id >>  8);						// destination PAN_ID
	pos = 5;
	/*
	 * dest. addr.
	 */
	for ( char i = 0; i < srcAddrLen; i++, pos++ )
	{
		*(send+pos) = *(srcAddr+i);
	}
	*(send+1) |= (srcAddrLen == 8)? 0xC : 0x8;

	/*
	 * src. address
	 */
	if ( RFmodul.netCMD_my > 0x0 )
	{
		sprintf((char*)(send+pos),  "%c",RFmodul.netCMD_my & 0xff);
		sprintf((char*)(send+pos+1),"%c",RFmodul.netCMD_my >> 8);				// src. short address
		
		*(send+1) |= 0x80;														// MAC header second byte
		pos += 2;
	}
	else
	{
		sprintf((char*)(send+pos),  "%c",RFmodul.netCMD_sl >>  0);
		sprintf((char*)(send+pos+1),"%c",RFmodul.netCMD_sl >>  8);
		sprintf((char*)(send+pos+2),"%c",RFmodul.netCMD_sl >> 16);
		sprintf((char*)(send+pos+3),"%c",RFmodul.netCMD_sl >> 24);				// src. ext. addr. low
		
		sprintf((char*)(send+pos+4),"%c",RFmodul.netCMD_sh >>  0);
		sprintf((char*)(send+pos+5),"%c",RFmodul.netCMD_sh >>  8);
		sprintf((char*)(send+pos+6),"%c",RFmodul.netCMD_sh >> 16);
		sprintf((char*)(send+pos+7),"%c",RFmodul.netCMD_sh >> 24);				// src. ext. addr. high
		
		*(send+1) |= 0xC0;														// MAC header second byte
		pos += 8;
	}
	
	sprintf((char*)(send+pos),   "%c", *(send+2) + (RFmodul.netCMD_sl & 0xFF) );// second counter, distance to frame counter is last byte of src extended addr.
	sprintf((char*)(send+pos+1), "%c", 0x05 );									// this byte will be used as command, 0x05 AT remote command response
	pos += 2;
	
	/*
	 * data
	 */
	*(send+pos) = frame.id;														// AP Frame ID
	
	sprintf((char*)(send+pos+1),"%c",RFmodul.netCMD_sh >> 24);
	sprintf((char*)(send+pos+2),"%c",RFmodul.netCMD_sh >> 16);
	sprintf((char*)(send+pos+3),"%c",RFmodul.netCMD_sh >>  8);
	sprintf((char*)(send+pos+4),"%c",RFmodul.netCMD_sh >>  0);					// src. ext. addr. high
	
	sprintf((char*)(send+pos+5),"%c",RFmodul.netCMD_sl >> 24);
	sprintf((char*)(send+pos+6),"%c",RFmodul.netCMD_sl >> 16);
	sprintf((char*)(send+pos+7),"%c",RFmodul.netCMD_sl >>  8);
	sprintf((char*)(send+pos+8),"%c",RFmodul.netCMD_sl >>  0);					// src. ext. addr. low
	
	sprintf((char*)(send+pos+9), "%c",RFmodul.netCMD_my >> 8);
	sprintf((char*)(send+pos+10),"%c",RFmodul.netCMD_my & 0xFF);				// src. short address
	
	*(send+pos+11) = frame.cmd[0];
	*(send+pos+12) = frame.cmd[1];												// command
	*(send+pos+13) = frame.ret * (-1);											// command status
	
	pos +=14;
	for ( char i = 0; i < frame.length; i++, pos++ )							// content [optional]
	{
		*(send+pos) = frame.msg[i];
	}
	
	return pos;
}

/*
 * explanation
 *
 * Received:
 *		values
 *
 * Returns:
 *		values
 *
 * last modified: 20--/--/--
 */
int TRX_0x01_transmit64Frame(uint8_t *send)
{
	/* TODO */
}

/*
 * explanation
 *
 * Received:
 *		values
 *
 * Returns:
 *		values
 *
 * last modified: 20--/--/--
 */
int TRX_0x02_transmit16Frame(uint8_t *send)
{
	/* TODO */
}