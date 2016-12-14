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

#include "../header/rfmodul.h"
#include "../header/atlocal.h"
#include "../header/apiframe.h"
#include "../header/circularBuffer.h"			// buffer
#include "../../ATuracoli/stackrelated.h"		// UART_print(f)
#include "../../ATuracoli/stackdefines.h"

// === Prototypes =========================================
static at_status_t AP_0x18_localDevice(struct api_f *frame, uint8_t *array, size_t *len);
static void		   AP_0x88_atLocal_response(struct api_f *frame);
static void		   AP_0x97_atRemote_response(uint16_t length);
static void		   AP_0x80_0x81_rxReceive( uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen );
static void		   AP_0x17_atRemoteFrame(void);

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
 * last modified: 2016/12/07
 */
void AP_frameHandle_uart(size_t *len)
{
	uint8_t  outchar[5]	= {0x0};
	struct api_f frame  = {0,0,0,0,{0},0,{0},0xFF};
	
	// Start delimiter	1 byte	
	cli(); BufferOut( &UART_deBuf, &outchar[0] ); sei();
	if ( outchar[0] != STD_DELIMITER ) 
	{
		frame.ret = ERROR;
		AP_0x88_atLocal_response( &frame );
		return;
	}
	if (RFmodul.deCMD_ru) UART_printf("\rStart delimiter\r%02"PRIX8"\r\r", STD_DELIMITER );

	// frame->bufLength	2 byte
	cli(); BufferOut( &UART_deBuf, &outchar[0] ); sei();
	cli(); BufferOut( &UART_deBuf, &outchar[1] ); sei();
	frame.length = (uint16_t) outchar[0] << 2 | outchar[1] ;
	if (RFmodul.deCMD_ru) UART_printf("Length\r%02"PRIX8" %02"PRIX8" (%u)\r\r", outchar[0], outchar[1], frame.length );
	
	// frame type	1 byte
	cli(); BufferOut( &UART_deBuf, &frame.type ); sei();
	switch ( frame.type )
	{
		case AT_COMMAND    : 
			if (RFmodul.deCMD_ru) UART_print("Frame type\r08 (AT Command)\r\r");
			frame.crc -= 0x08;
			if ( frame.length == 4)
			{
				frame.ret = CMD_readOrExec(&frame, outchar, NULL);
			}
			else
			{
				frame.ret = CMD_write(&frame, outchar, len);
				SET_userValInEEPROM();
			}
		break;
		
		case AT_COMMAND_Q  : 
			if (RFmodul.deCMD_ru) UART_print("Frame type\r09 (AT Command Queue)\r\r");
			frame.crc -= 0x09;
			if ( frame.length == 4)
			{
				frame.ret = CMD_readOrExec(&frame, outchar, NULL);
			}
			else
			{
				frame.ret = CMD_write(&frame, outchar, len);
			}
		break;
		
		case REMOTE_AT_CMD : 
			if (RFmodul.deCMD_ru) UART_print("Frame type\r17 (AT Remote Command)\r\r");
			frame.crc -= 0x17;
			frame.ret = TRX_send(&frame.type); 
		break;
		
		case DEVICE_AT_CMD :
			if (RFmodul.deCMD_ru) UART_print("Frame type\r18 (AT Device Command)\r\r");
			frame.crc -= 0x18;
			frame.ret = AP_0x18_localDevice(&frame, outchar, len);
		break;
				
		default : frame.ret = INVALID_COMMAND;
	}
		
	AP_0x88_atLocal_response( &frame );
}

/*
 * AP_findInTable() 
 * - searched in the command table for the command id
 *
 * Received:
 *		api_f		the AP frame to store important informations for the response
 *		uint8_t		pointer to an array which is already initialized and available for processing (just to save a little mem)
 *
 * Returns:
 *	   CMD struct		on success
 *     INVALID_COMMAND	on fail
 *
 * last modified: 2016/11/18
 */
CMD* AP_findInTable(struct api_f *frame, uint8_t *array)
{
	// frame id		1 byte
	cli(); BufferOut( &UART_deBuf, &frame->id ); sei();
	if ( RFmodul.deCMD_ru ) UART_printf("Frame ID\r%02"PRIX8"\r\r", frame->id );
	frame->crc -= frame->id;
	
	// AT command 2 bytes
	/*
	 * Why this intricate way and not simply compare 2 characters at a specific start position?
	 *   The answer is quiet simple, now there are less commands but if the cmd table grows you'll find
	 *   double matches, that's why it's better to compare 4 letters.
	 *
	 * But what is with the special device commands (DE..)?
	 *   For this case the frame type 0x18 is added to the library.
	 */
	*(array+0) = 'A';
	*(array+1) = 'T';
	cli(); BufferOut( &UART_deBuf, array+2 ); sei();
	cli(); BufferOut( &UART_deBuf, array+3 ); sei();
	
	if (RFmodul.deCMD_ru) UART_printf("AT Command\r%02"PRIX8" %02"PRIX8" (%c%c)\r\r", *(array+2), *(array+3), *(array+2), *(array+3));
	frame->crc -= (*(array+2) + *(array+3));
	frame->cmd[0] = *(array+2);
	frame->cmd[1] = *(array+3);
	
	// search for CMD in table
	CMD *workPointer = (CMD*) pStdCmdTable;
	for (int i = 0; i < command_count ; i++, workPointer++)
	{
		if( strncmp( (const char*) array, workPointer->name, 4 ) == 0 ) 
		{
			return workPointer;
		}
	}
	return NO_AT_CMD;
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
bool_t AP_compareCRC(struct api_f *frame)
{
	uint8_t userCRC;
	cli(); BufferOut( &UART_deBuf, &userCRC); sei();
	
	return ( frame->crc == userCRC )? TRUE : FALSE;	
}





// === Functions (local handling static) ==================
// ===
// ========================================================
/*
 * Specific device commands (AP)
 * interpret the received frame and execute the specific task
 *
 * Received:
 *		api_f	pointer to the frame struct
 *		uint8_t	pointer to an array which is already initialized and available for processing (just to save a little mem)
 *		size_t	the length of the whole input, which is stored in buffer
 *
 * Returns:
 *		OP_SUCCESS			no error has occurred
 *		ERROR				calculated crc does not match with received crc
 *		INVALID_COMMAND		command is not in the table
 *		INVALID_PARAMETER	parameter is not in a valid range
 *		
 * last modified: 2016/12/07
 */
static at_status_t AP_0x18_localDevice(struct api_f *frame, uint8_t *array, size_t *len)
{
	// frame id		1 byte
	cli(); BufferOut( &UART_deBuf, &frame->id ); sei();
	if (RFmodul.deCMD_ru) UART_printf("Frame ID\r%02"PRIX8"\r\r", frame->id );
	frame->crc -= frame->id;
	
	// AT command 2 bytes
	/*
	 * Why this intricate way and not simply compare 2 characters at a specific start position?
	 *   The answer is quiet simple, now there are less commands but if the cmd table grows you'll find
	 *   double matches, that's why it's better to compare 4 letters.
	 *
	 * But what is with the special device commands (DE..)?
	 *   For this case the frame type 0x18 is added to the library.
	 */
	*(array+0) = 'D';
	*(array+1) = 'E';
	cli(); BufferOut( &UART_deBuf, array+2 ); sei();
	cli(); BufferOut( &UART_deBuf, array+3 ); sei();
	
	if (RFmodul.deCMD_ru) UART_printf("DE Command\r%02"PRIX8" %02"PRIX8" (%c%c)\r\r", *(array+2), *(array+3), *(array+2), *(array+3));
	frame->crc -= (*(array+2) + *(array+3));
	frame->cmd[0] = *(array+2);
	frame->cmd[1] = *(array+3);
	
	// search for CMD in table
	CMD *workPointer = (CMD*) pStdCmdTable;
	for (int i = 0; i < command_count ; i++, workPointer++)
	{
		if( strncmp( (const char*) array, workPointer->name, 4 ) == 0 ) break;
	}
	
	/*
	 * handle CMD
	 *
	 * frame length
	 * EXEC is allowed
	 */
	if ( frame->length == 4 && workPointer->rwxAttrib & EXEC )
	{
		frame->rwx = EXEC;
		if ( AP_compareCRC( frame ) == FALSE )
		{
			UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
			return ERROR;
		}
		
		switch( workPointer->rwxAttrib )
		{
			default: break;
		}
	}
	/*
	 * frame length
	 * READ is allowed
	 */
	else if ( frame->length == 4 && workPointer->rwxAttrib & READ )
	{
		frame->rwx = READ;
		uint8_t userCRC;
		if ( AP_compareCRC( frame ) == FALSE )
		{
			UART_printf("Expected CRC: %"PRIX8"\r\r", frame->crc );
			frame->length = 0;
			return ERROR;
		}
		
		switch ( workPointer->ID )
		{
/* RU */	case DE_RU :
					frame->length = 1;
					frame->msg[0] = RFmodul.deCMD_ru;
				break;
				
/* FV */	case DE_FV :
					frame->length = 1;
					memcpy( frame->msg, AT_VERSION, strlen(AT_VERSION)+1 );
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
	else if ( frame->length > 4 && workPointer->rwxAttrib & WRITE )
	{
		size_t cmdSize = frame->length - 4;
		frame->rwx = WRITE;
		
		switch( workPointer->ID )
		{
/* RU */   case DE_RU : {
				/*
				 * get the parameter
				 */
				cli(); BufferOut( &UART_deBuf, &frame->msg[0] ); sei();
				frame->crc -= frame->msg[0];
				
				/*
				 * compare the parameter
				 */
				if ( AP_compareCRC(frame) == FALSE )
				{
					if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
					return ERROR;
				}

				if ( cmdSize <= 1 && frame->msg[0] == 0x0 || frame->msg[0] == 0x1 )
				{
					RFmodul.deCMD_ru = frame->msg[0];
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
			}
			default : 
				return INVALID_COMMAND;
		}
	}
	else
	{
		return INVALID_COMMAND;
	}
	return OP_SUCCESS;
	
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
	frame->crc = 0xFF;
	if ( frame->ret < 0 ) frame->length = 0;
	frame->length += 5;
	
	if ( RFmodul.deCMD_ru )
	{
		UART_print("\r********** AT RESPONSE **********\r");
		UART_printf("Start delimiter\r%02"PRIX8"\r\r", STD_DELIMITER );
		UART_printf("Length\r%02"PRIX8" %02"PRIX8" (%u)\r\r", frame->length >> 8, frame->length & 0xFF, frame->length );
		UART_print("Frame type\r88 (AT Command Response)\r\r");
		UART_printf("Frame ID\r%02"PRIX8"\r\r", frame->id );
		UART_printf("AT Command\r%02"PRIX8" %02"PRIX8" (%c%c)\r\r", frame->cmd[0], frame->cmd[1], frame->cmd[0], frame->cmd[1]);
		UART_printf("Command status\r%02"PRIX8" (%s)\r\r", frame->ret * (-1), (frame->ret * (-1) == 0)? "OK" :\
																			  (frame->ret * (-1) == 1)? "ERROR" :\
																			  (frame->ret * (-1) == 2)? "Invalid Command" :\
																			  (frame->ret * (-1) == 3)? "Invalid Parameter" : "" );
		
		frame->crc -=  ( 0x88 + frame->id + frame->cmd[0] + frame->cmd[1] + (frame->ret * (-1)) );
		
		if ( frame->rwx == READ ) 
		{
			UART_print("Response\r");
					
			for (uint16_t x; x < frame->length-5 ; x++ )
			{
				frame->crc -= frame->msg[x];
				if ( RFmodul.deCMD_ru ) UART_printf("%02"PRIX8" ", frame->msg[x] );
			}
			UART_print("\r\r");
		}
		UART_printf("Checksum\r%02"PRIX8"\r\r",frame->crc);
	}
	else
	{
		UART_putc( STD_DELIMITER );						// start delimiter
		UART_putc( (uint8_t) (frame->length >> 8) );	// frame length
		UART_putc( (uint8_t) (frame->length & 0xFF) );
		UART_putc( 0x88 );								// frame type
		UART_putc( frame->id );
		UART_putc( frame->cmd[0] );						// AT cmd
		UART_putc( frame->cmd[1] );
		UART_putc( frame->ret * (-1) );					// cmd option (successful/ not successful etc.)
		
		//			   Type +  Frame ID +  AT Command                   +   Return Value      //
		frame->crc -= (0x88 + frame->id + frame->cmd[0] + frame->cmd[1] + (frame->ret * (-1)) );
		if ( frame->rwx == READ )
		{
			for (uint16_t x; x < frame->length-5 ; x++ )
			{
				UART_putc(    frame->msg[x] );
				frame->crc -= frame->msg[x];
			}
		}
		
		UART_putc(frame->crc);							// checksum
	}
	
}

/*
 * AT Remote Response Frame
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
	uint8_t  outchar, crc = 0xFF;
	length += 1; // +1 for frame type
	
	UART_putc( STD_DELIMITER );						// start delimiter
	UART_putc( (uint8_t) (length >> 8) );			// frame length
	UART_putc( (uint8_t) (length & 0xFF) );
	UART_putc( 0x97 );								// frame type
	crc -= 0x97;
	
	for (uint16_t i = 0; i < length-1; i++)
	{
		cli(); BufferOut( &RX_deBuf, &outchar ); sei();
		UART_putc( outchar );
		crc -= outchar;
	}
	UART_putc(crc);									// checksum
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
 * last modified: 2016/12/14
 */
static void AP_0x80_0x81_rxReceive( uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen )
{
	uint8_t  outchar, crc = 0xFF;
	length += 3; // +2 for frame type
	
	UART_putc( STD_DELIMITER );				// start delimiter
	UART_putc( (uint8_t) (length >> 8) );	// frame length
	UART_putc( (uint8_t) (length & 0xFF) );
	if ( srcAddrLen == 8 )					// frame type
	{
		UART_putc( 0x80 );
		crc -= 0x80;
	} 
	else
	{
		UART_putc( 0x81 );
		crc -= 0x80;
	}
	
	for ( uint8_t i; i < srcAddrLen ; i++ )	// Addr.
	{
		UART_putc( *(srcAddr+i) );
		crc -= *(srcAddr+i);
	}
	
	UART_putc( 0x00 );						// call RSSI function TODO
	crc -= 0x00;
	
	UART_putc( *(srcAddr+srcAddrLen+1) );  // Option
	crc -=     *(srcAddr+srcAddrLen+1);
					
	for (uint16_t i; i < length-3 ; i++ )
	{
		cli(); BufferOut( &RX_deBuf, &outchar ); sei();
		UART_putc( outchar );
		crc -= outchar;
	}

	UART_putc(crc);							// checksum
}

static void AP_0x17_atRemoteFrame(void)
{
	
}




// === Functions (transreceiver) ==========================
// ===
// ========================================================
/*
 *
 *
 *
 *
 */
void TRX_createAPframe( uint8_t flen, uint8_t dataStart, uint8_t srcAddrLen, uint8_t option )
{
	uint8_t outchar, srcAddrAndOption[10];
	
	/*
	 * - delete  dest. PANID, dest. addr., [src. PANID] (max 12 bytes)
	 * - read src addr (2 or 8 bytes)
	 * - read Data header 2 Bytes
	 * - check type of data header and handle data
	 */
	for (uint8_t counter = 0, pos = 0; pos < srcAddrLen+2 && counter < flen; counter++ )
	{
		cli(); BufferOut( &RX_deBuf, &outchar ); sei();
		if ( counter >= dataStart-srcAddrLen )
		{
			srcAddrAndOption[pos] = outchar;
			if () srcAddrAndOption[pos] = option;
			pos++;
		}
	}
	
	switch ( srcAddrAndOption[srcAddrLen+1] )
	{
		case 0x04 : AP_0x17_atRemoteFrame(); break;									// TX response only
		case 0x05 : AP_0x97_atRemote_response( (uint16_t) flen-dataStart); break;								// UART response only
		case 0x00 : AP_0x80_0x81_rxReceive( (uint16_t) flen-dataStart , srcAddrAndOption, srcAddrLen ); break;	// UART response only
	}					
	
}

/* 
 * AT Remote Command request
 * prepared frame for a remote AT command to control another device
 *
 * Received:
 *		uint8_t pointer to send array
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/12/13
 */
int TRX_0x17_atRemoteFrame(uint8_t *send)
{
#if DEBUG
	UART_print("== in AP Remote CMD\r\n");
#endif
	int			pos		= 0;
	at_status_t ret		= 0;
	uint8_t		crc		= 0xFF;
	uint8_t		tmp[10] = {0};
	bool_t		flag	= FALSE;
	
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */	
	if ( TRUE )													 *send |= 0x01; // if data send to _one_ device else Beacon /* TODO */
	if ( RFmodul.secCMD_ee == TRUE )							 *send |= 0x08; // security active
	if ( RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x2 )  *send |= 0x20; // ACK on
	*send |= 0x40; // PAN Compression
	
	sprintf((char*)(send+ 3),"%c",RFmodul.netCMD_id & 0xff);
	sprintf((char*)(send+ 4),"%c",RFmodul.netCMD_id >>  8);		// destination PAN_ID
	
	/*
	 * dest. address
	 */
	crc -= 0x17;
	for (char i = 0; i < 10; )
	{
		cli(); ret = BufferOut( &UART_deBuf, &tmp[i] ); sei();
		if ( ret == BUFFER_OUT_FAIL ) return 0;
		crc -= tmp[i];
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
		*(send+ 8) = tmp[4];		// destination ext. addr. low

		*(send+ 9) = tmp[3];
		*(send+10) = tmp[2];
		*(send+11) = tmp[1];
		*(send+12) = tmp[0];		// destination ext. addr. high
		
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
	
	sprintf((char*)(send+15),"%c",0x4D);								// it looks like an APS counter
	sprintf((char*)(send+16),"%c",0x04);								// Command type
	
	pos = 17;
	

	do
	{
		cli(); ret = BufferOut( &UART_deBuf, send+pos ); sei();
		if ( ret == BUFFER_OUT_FAIL ) break;
		
		crc -= *(send+pos);
		pos++;
		
	} while ( pos < PACKAGE_SIZE-1 );
	
	crc += *(send+pos);	// the user crc need to be deleted
	if ( crc =! *(send+pos))
	{
		UART_printf("%x vs %x", crc, *(send+pos) );
	}
	
	return pos-1;
}

int TRX_0x01_transmit64Frame(uint8_t *send)
{
	
}

int TRX_0x02_transmit16Frame(uint8_t *send)
{
	
}

int TRX_0x97_atRemote_response(uint8_t *send)
{
	
}