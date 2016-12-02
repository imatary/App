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
static ATERROR API_0x18_localDevice(struct api_f *frame, uint8_t *array, size_t *len);
static void  API_0x88_atLocal_response(struct api_f *frame);

// === Functions ==========================================

/*
 *
 *
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/11/28
 */
ATERROR API_frameHandle_uart(size_t *len)
{
	uint8_t  outchar[5]	= {0x0};
	struct api_f frame  = {0,0,0,0,0,{0},0,{0},0xFF};
	
	// Start delimiter	1 byte	
	cli(); BufferOut( &UART_deBuf, &frame.delimiter ); sei();
	if ( frame.delimiter != STD_DELIMITER ) return API_NOT_AVAILABLE;
	if (RFmodul.deCMD_ru) UART_printf("Start delimiter\r%02"PRIX8"\r\r", STD_DELIMITER );
	
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
			if ( frame->length == 4)
			{
				frame.ret = CMD_readOrExec(&frame, outchar, NULL);
			}
			else
			{
				frame.ret = CMD_write(&frame, outchar, *len);
			}
		break;
		
		case AT_COMMAND_Q  : 
			if (RFmodul.deCMD_ru) UART_print("Frame type\r09 (AT Command Queue)\r\r");
			frame.crc -= 0x09;
			if ( frame->length == 4)
			{
				frame.ret = CMD_readOrExec(&frame, outchar, NULL);
			}
			else
			{
				frame.ret = CMD_write(&frame, outchar, *len);
			}
		break;
		
		case REMOTE_AT_CMD : 
			if (RFmodul.deCMD_ru) UART_print("Frame type\r17 (AT Remote Command)\r\r");
			frame.crc -= 0x17;
			frame.ret = TRX_send(); 
		break;
		
		default : UART_print("Not a valid command type!\r\r"); return INVALID_COMMAND;
	}
		
	API_0x88_atLocal_response( frame );
}

/*
 * Specific device commands (API)
 *
 * Returns:
 *		OP_SUCCESS
 *		
 */
static ATERROR API_0x18_localDevice(struct api_f *frame, uint8_t *array, size_t *len)
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
	frame->cmd[0] = array+2;
	frame->cmd[1] = array+3;
	
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
		uint8_t userCRC;
		if ( API_compareCRC(&frame->crc, &userCRC) == FALSE )
		{
			UART_printf("Calculated CRC: %"PRIX8" vs read CRC:  %"PRIX8"\r\r",  frame->crc, userCRC );
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
		uint8_t userCRC;
		if ( API_compareCRC( &frame->crc, &userCRC ) == FALSE )
		{
			UART_printf("Calculated CRC: %"PRIX8" vs read CRC:  %"PRIX8"\r\r", frame->crc, userCRC );
			return ERROR;
		}
		
		switch (workPointer->ID)
		{
/* RU */	case DE_RU :
				if ( RFmodul.serintCMD_ap == 0 )
				{
					UART_printf("%"PRIX8"\r", RFmodul.deCMD_ru);
				}
				else
				{
					uint8_t val = RFmodul.deCMD_ru;
					API_0x88_atLocal_response( frame );
				}
				break;
				
			default: break;
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
		size_t cmdSize = (((*len-15) % 2) + (*len-15))/2;
		
		switch( workPointer->ID )
		{
/* RU */   case DE_RU : {
				/*
				 * get the parameter
				 */
				cli(); BufferOut( &UART_deBuf, &frame->msg[0] ); sei()
				frame->crc -= frame->msg[0];
					
				if ( API_compareCRC(frame) == FALSE )
				{
					if ( RFmodul.deCMD_ru ) UART_printf("Expected CRC: %"PRIX8"\r\r",  frame->crc );
					return ERROR;
				}
								
				/*
				 * compare the parameter
				 */
				if ( cmdSize <= 1 && frame->msg[0] >= FALSE && frame->msg[0] <= TRUE )
				{
					RFmodul.deCMD_ru = frame->msg[0];
					if( frame->type == 0x8 ) SET_userValInEEPROM();
				}
				else 
				{ 
					return INVALID_PARAMETER;
				}
			}
			default : return INVALID_COMMAND;
		}
	}
	else
	{
		return INVALID_COMMAND;
	}
	
}

/*
 *
 *
 *
 * Returns:
 *		nothing
 *
 * last modified: 2016/11/30
 */
static void API_0x88_atLocal_response(struct api_f *frame)
{
	frame->crc = 0xFF;
	frame->length += 5;
	
	UART_putc( frame->delimiter );					// start delimiter
	UART_putc( (uint8_t) (frame->length >> 8) );	// frame length
	UART_putc( (uint8_t) (frame->length & 0xFF) );
	UART_putc( 0x88 );								// frame type
	frame->crc -=   0x88;
	UART_putc(      frame->id );
	frame->crc -=   frame->id  ;
	UART_putc(      frame->cmd[0] );				// AT cmd
	frame->crc -=   frame->cmd[0]  ;
	UART_putc(      frame->cmd[1] );
	frame->crc -=   frame->cmd[1]  ;
	UART_putc(      frame->ret * (-1) );			// cmd option (successful/ not successful etc.)
	frame->crc -= ( frame->ret * (-1) );
	
	for (uint16_t x; x < frame->length ; x++ )
	{
		UART_putc(    frame->msg[x] );
		frame->crc -= frame->msg[x];
	}

	UART_putc(frame->crc);									// checksum
}


/*
 * The API compare function red the user crc value from the buffer
 * and compared it with the calculated crc sum
 *
 * Received:
 *		the API frame
 *
 * Returned:
 *		TRUE	if calculated crc equal to user crc
 *		FALSE	if calculated crc is not equal to user crc
 *
 * last modified: 2016/11/29
 */
bool_t API_compareCRC(struct api_f *frame)
{
	uint8_t userCRC;
	BufferOut( &UART_deBuf, userCRC);
	
	return ( frame->crc == *userCRC )? TRUE : FALSE;	
}


/*
 * API_findInTable() 
 * - searched in the command table for the command id
 *
 * Returns:
 *	   CMD struct		on success
 *     INVALID_COMMAND	on fail
 *
 * last modified: 2016/11/18
 */
CMD* API_findInTable(struct api_f *frame, uint8_t *array)
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
	frame->cmd[0] = array+2;
	frame->cmd[1] = array+3;
	
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