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

// === structures =========================================
typedef struct {
	uint8_t data[DE_BUFFER_SIZE];
	uint8_t read;						// pointer to sector of oldest contend
	uint8_t write;						// pointer to empty sector
	bool_t	newContent;					// status if the buffer has new content received
} deBuffer_t;

// === globals ============================================
static deBuffer_t buffer[] = { 
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
	
	uint8_t next = ( (buffer[bufType].write + 1) & DE_BUFFER_MASK );

	if ( buffer[bufType].read == next )
		 return BUFFER_IN_FAIL; // full
	

	buffer[bufType].data[ buffer[bufType].read ] = inByte;

	//buf->data[buffer[bufType].write & DE_BUFFER_MASK] = inByte; // absolutely secure (related to the author)
	buffer[bufType].write = next;
	
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
		
	if (buffer[bufType].read == buffer[bufType].write)
	return BUFFER_OUT_FAIL;

	*pByte = buffer[bufType].data[ buffer[bufType].read ];

	buffer[bufType].read = (buffer[bufType].read+1) & DE_BUFFER_MASK;

	return OP_SUCCESS;
}

// === Helper functions ===================================
/*
 * Get data of deBuffer
 * filled the received array pointer with data  of the buffer.
 * Caution! Buffer direct access.
 *
 * Received:
 *		bufType_n	number of buffer type
 *		uint8_t		pointer to work buffer which should be filled
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
	
	memcpy( workArray, &buffer[bufType].data[ buffer[bufType].read ], len); 
	deBufferReadReset( bufType, '+', len);
}

/*
 * BufferNewContent
 * will set the newContent variable to true if the module received some data over the air
 *
 * Received:
 *		bufType_n	number of buffer type
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
	buffer[bufType].newContent = val; 
}

bool_t GET_deBufferNewContent(bufType_n bufType)
{ 
	if ( NONE == bufType ) { return FALSE; }
	return buffer[bufType].newContent; 
}

/*
 * Buffer read pointer reset function,
 * reset the pointer position of a number of len
 * if the pointer hit the writing pointer, set the read pointer for
 *		'+' operation to buffer[bufType].write     (buffer can not read until new data was written into the buffer)
 *      '-' operation to buffer[bufType].write + 1 (buffer need to be read until new data can written into the buffer)
 * and return
 *
 * Received:
 *		bufType_n	number of buffer type
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
			buffer[bufType].read = (buffer[bufType].read - 1) & DE_BUFFER_MASK;
		}
		if ( '-' == operand && buffer[bufType].read == buffer[bufType].write)
		{
			buffer[bufType].read == buffer[bufType].write+1;
			return;
		}
		if ( '+' == operand ) 
		{
			buffer[bufType].read = (buffer[bufType].read + 1) & DE_BUFFER_MASK;
		}
		if ( '+' == operand && buffer[bufType].read == buffer[bufType].write)
		{
			return;
		}
	}
	
}

/*
 * Buffer write pointer reset function,
 * reset the pointer position of a number of len
 * if the pointer hit the read pointer, set the write pointer for
 *		'+' operation to buffer[bufType].read - 1 (no further writing is posible until the buffer was read)
 *      '-' operation to buffer[bufType].read + 1 (no further reading is posible until new data was written into the buffer)
 * and return
 *
 * Received:
 *		bufType_n	number of buffer type
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
			buffer[bufType].write = (buffer[bufType].write - 1) & DE_BUFFER_MASK;
		}
		if ( '-' == operand && buffer[bufType].read == buffer[bufType].write)
		{
			buffer[bufType].read = buffer[bufType].read + 1;
			return;
		}
		if ( '+' == operand )
		{
			buffer[bufType].read = (buffer[bufType].read + 1) & DE_BUFFER_MASK;
		}
		if ( '+' == operand && buffer[bufType].read == buffer[bufType].write)
		{
			buffer[bufType].read = buffer[bufType].read + 1;
			return;
		}
	}
	
}

/*
 * Reset the whole buffer
 *
 * Returns:
 *     nothing
 *
 * last modified: 2017/01/20
 */
void deBufferReset(bufType_n bufType) 
{ 
	if ( NONE == bufType ) { return; }
	memset( &buffer[bufType], 0, sizeof(deBuffer_t) ); 
}