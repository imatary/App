/*
 * circularBuffer.c
 *
 * Created: 26.10.2016 07:54:25
 *  Author: TOE
 *
 * File related to the sample code of mikrocontroller.net
 * http://www.mikrocontroller.net/articles/FIFO
 */ 
// === includes ===========================================
#include <stdlib.h>
#include "../header/circularBuffer.h"	// struct, prototypes, defines
#include "../../ATuracoli/stackrelated.h"

// === structures =========================================
typedef struct {
	uint8_t data[DE_BUFFER_SIZE];
	uint8_t read;						// pointer to sector of oldest contend
	uint8_t write;						// pointer to empty sector
	bool_t	newContent;					// status if the buffer has new content received
} deBuffer_t;

// === globals ============================================
static deBuffer_t xbuffer[] = { 
	{ {0x0}, 0x0, 0x0, FALSE },	// UART
	{ {0x0}, 0x0, 0x0, FALSE }, // RX
};

// === functions ==========================================
/*
 * Set one byte in to circular buffer
 *
 * Received:
 *		bufType_n	number of buffer type
 *		uint8_t		the incoming value, which should be stored into the buffer
 *
 * Returns:
 *     BUFFER_FAIL      buffer is full. No more space for more bytes.
 *     OP_SUCCESS		the byte was saved
 *
 * last modified: 2017/01/20
 */
at_status_t deBufferIn(bufType_n bufType, uint8_t inByte)
{
	if ( NONE == bufType ) { return BUFFER_IN_FAIL; }
	
	uint8_t next =  (xbuffer[bufType].write + 1) & DE_BUFFER_MASK;

	if ( xbuffer[bufType].read == next )
		 return BUFFER_IN_FAIL; // full
	
	xbuffer[bufType].data[ xbuffer[bufType].write ] = inByte;
	xbuffer[bufType].write = next;
	
	return OP_SUCCESS;
}

/*
 * Get 1 byte of the buffer, if the byte is ready
 *
 * Received:
 *		bufType_n	number of buffer type
 *		uint8_t		pointer to the element which should receive the byte
 *
 * Returns:
 *     BUFFER_FAIL      buffer is empty. he cannot send a byte.
 *     OP_SUCCESS		1 Byte was delivered
 *
 * last modified: 2017/01/20
 */
at_status_t deBufferOut(bufType_n bufType, uint8_t *pByte)
{
	if ( NONE == bufType ) { return BUFFER_OUT_FAIL; }
	
	if ( xbuffer[bufType].read == xbuffer[bufType].write)
	return BUFFER_OUT_FAIL;

	*pByte = xbuffer[bufType].data[ xbuffer[bufType].read ];

	xbuffer[bufType].read = xbuffer[bufType].read+1 & DE_BUFFER_MASK;

	return OP_SUCCESS;
}

// === Helper functions ===================================
/*
 * Get data of deBuffer
 * filled the received array pointer with data  of the xbuffer.
 * Caution! Buffer direct access.
 *
 * Received:
 *		bufType_n	number of xbuffer type
 *		uint8_t		pointer to work xbuffer which should be filled
 *		uint16_t	pointer to length which should copied
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/11/17
 */
void GET_deBufferData_atReadPosition(bufType_n bufType, uint8_t *workArray, size_t len) 
{ 
	if ( NONE == bufType ) { return; }
	
	memcpy( workArray, &xbuffer[bufType].data[ xbuffer[bufType].read ], len); 
	deBufferReadReset( bufType, '+', len);
}

/*
 * BufferNewContent
 * will set the newContent variable to true if the module received some data over the air
 *
 * Received:
 *		bufType_n	number of xbuffer type
 *		bool_t		FALSE or TRUE	(SET)
 *
 * Returns:
 *     value of .newContent else FALSE (GET)
 *
 * last modified: 2016/11/17
 */
void   SET_deBufferNewContent(bufType_n bufType, bool_t val) 
{ 
	if ( NONE == bufType ) { return; }
	xbuffer[bufType].newContent = val; 
}

bool_t GET_deBufferNewContent(bufType_n bufType)
{ 
	if ( NONE == bufType ) { return FALSE; }
	return xbuffer[bufType].newContent; 
}

/*
 * Buffer read pointer reset function,
 * reset the pointer position of a number of len
 * if the pointer hit the writing pointer, set the read pointer for
 *		'+' operation to xbuffer[bufType].write     (xbuffer can not read until new data was written into the xbuffer)
 *      '-' operation to xbuffer[bufType].write + 1 (xbuffer need to be read until new data can written into the xbuffer)
 * and return
 *
 * Received:
 *		bufType_n	number of xbuffer type
 *		char		'+' or '-' for the direction
 *		uint8_t		the number of elements which should skipped
 *
 * Returns:
 *		nothing
 *
 * last modified: 2016/11/28
 */
void deBufferReadReset(bufType_n bufType, char operand , size_t len)
{
	if ( NONE == bufType ) { return; }

	for (uint8_t i = 0; i < len; i++)
	{
		if ( '-' == operand ) 
		{
			xbuffer[bufType].read = (xbuffer[bufType].read - 1) & DE_BUFFER_MASK;
		}
		if ( '-' == operand && xbuffer[bufType].read == xbuffer[bufType].write)
		{
			xbuffer[bufType].read == xbuffer[bufType].write+1;
			return;
		}
		if ( '+' == operand ) 
		{
			xbuffer[bufType].read = (xbuffer[bufType].read + 1) & DE_BUFFER_MASK;
		}
		if ( '+' == operand && xbuffer[bufType].read == xbuffer[bufType].write)
		{
			return;
		}
	}
	
}

/*
 * Buffer write pointer reset function,
 * reset the pointer position of a number of len
 * if the pointer hit the read pointer, set the write pointer for
 *		'+' operation to xbuffer[bufType].read - 1 (no further writing is posible until the xbuffer was read)
 *      '-' operation to xbuffer[bufType].read + 1 (no further reading is posible until new data was written into the xbuffer)
 * and return
 *
 * Received:
 *		bufType_n	number of xbuffer type
 *		char		'+' or '-' for the direction
 *		uint8_t		the number of elements which should skipped
 *
 * Returns:
 *		nothing
 *
 * last modified: 2016/11/28
 */
void deBufferWriteReset(bufType_n bufType,char operand , size_t len)
{
	if ( NONE == bufType ) { return; }
	
	for (uint8_t i = 0; i < len; i++)
	{
		if ( '-' == operand )
		{
			xbuffer[bufType].write = (xbuffer[bufType].write - 1) & DE_BUFFER_MASK;
		}
		if ( '-' == operand && xbuffer[bufType].read == xbuffer[bufType].write)
		{
			xbuffer[bufType].read = xbuffer[bufType].read + 1;
			return;
		}
		if ( '+' == operand )
		{
			xbuffer[bufType].read = (xbuffer[bufType].read + 1) & DE_BUFFER_MASK;
		}
		if ( '+' == operand && xbuffer[bufType].read == xbuffer[bufType].write)
		{
			xbuffer[bufType].read = xbuffer[bufType].read + 1;
			return;
		}
	}
	
}

/*
 * Reset the whole xbuffer
 *
 * Returns:
 *     nothing
 *
 * last modified: 2017/01/20
 */
void deBufferReset(bufType_n bufType) 
{ 
	if ( NONE == bufType ) { return; }
	memset( &xbuffer[bufType], 0, sizeof(deBuffer_t) );
}