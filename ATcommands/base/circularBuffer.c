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
#include <string.h>
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
	uint8_t next = ( (buffer[bufType].write + 1) & DE_BUFFER_MASK );

	if ( buffer[bufType].read == next )
		 return BUFFER_IN_FAIL; // full
	

	buffer.data[ buffer[bufType].read ] = inByte;

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
	if (buffer[bufType].read == buffer[bufType].write)
	return BUFFER_OUT_FAIL;

	*pByte = buffer[bufType].data[ buffer[bufType].read ];

	buffer[bufType].read = (buffer[bufType].read+1) & DE_BUFFER_MASK;

	return OP_SUCCESS;
}

// === Helper functions ===================================
 
/*
 * BufferNewContent
 * will set the newContent variable to true if the module received some data over the air
 *
 * Received:
 *		deBuffer_t	pointer to the buffer
 *		bool_t		FALSE or TRUE
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/11/17
 */
void   SET_deBufferNewContent(bufType_n bufType, bool_t val) { buffer[bufType].newContent = val; }
bool_t GET_deBufferNewContent(bufType_n bufType)             { return buffer[bufType].newContent; }

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
void deBufferReadReset(bufType_n bufType, char operand , uint8_t len)
{
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
void deBufferWriteReset(bufType_n bufType,char operand ,uint8_t len)
{
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
void deBufferReset(bufType_n bufType) { memset( &buffer[bufType_n], 0, sizeof(deBuffer_t) ); }
	
/*
 * Validation an write function
 * - received the buffer content and converted content to uint32 hex values
 * if - the command size smaller or equal then unit of the tmp buffer 
 *    - the buffer value greater or equal than min value
 *    - the buffer value smaller or equal than max value
 * write to RFmodul struct
 * 
 * Received:
 *		bufType_n	number of buffer type
 *		uint16_t  	complete string length
 *		CMD			pointer to command in command table
 *		
 * Returns:
 *     OP_SUCCESS			on success
 *	   INVALID_PARAMETER	if parameter is not valid or error has occurred during transforming to hex
 *
 * last modified: 2016/12/02
 */
at_status_t max_u32val( bufType_n bufType, uint16_t len, CMD *cmd )
{
	uint32_t val;
	char *endptr;
	
	if( AT_MODE_ACTIVE == GET_serintCMD_ap )
	{
		val = strtoul( (const char*) buffer[bufType].data[ buffer[bufType].read ], &endptr, 16);
		if ( *endptr != buffer[bufType].data[len-1]) return INVALID_PARAMETER;
	}
	else
	{
		memcpy( &val, &buffer[bufType].data[ buffer[bufType].read ], cmd->cmdSize);
	}
	
	deBufferReadReset( bufType, '+', len);
	
	if ( val >= cmd->min && val <= cmd->max )
	{
		cmd->set( &val, len);
		
		if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
		return OP_SUCCESS;
	}
	else return INVALID_PARAMETER;
}

at_status_t max_u64val( bufType_n bufType, uint16_t len, CMD *cmd )
{
	uint64_t val;
	char *endptr;
	
	if( AT_MODE_ACTIVE == GET_serintCMD_ap )
	{
		val = strtoul( (const char*) buffer[bufType].data[ buffer[bufType].read ], &endptr, 16);
		if ( *endptr != buffer[bufType].data[len-1]) return INVALID_PARAMETER;
	}
	else
	{
		memcpy( &val, &buffer[bufType].data[ buffer[bufType].read ], len);
	}
	
	deBufferReadReset( bufType, '+', len);
	
	if ( val >= cmd->min && val <= cmd->max )
	{
		cmd->set( &val, len);
		
		if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
		return OP_SUCCESS;
	}
	else return INVALID_PARAMETER;
}


/* 
 * special handle if
 * - network identifier string command
 * - buffer content <= 20 characters
 */
at_status_t node_identifier( bufType_n bufType, uint16_t len, CMD *cmd )
{
	if ( len <= cmd->max )
	{
		cmd->set( &buffer[bufType].data[ buffer[bufType].read ], len);
		
		if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) UART_print_status(OP_SUCCESS);
		return OP_SUCCESS;
	}
	else
	{
		return INVALID_PARAMETER;
	}
}

at_status_t ky_validator(bufType_n bufType, uint16_t len, CMD *cmd)
{
	/* TODO */
	if (FALSE)
	{
		if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) UART_print_status(OP_SUCCESS);
		return OP_SUCCESS;
	}
	else
	{
		return INVALID_PARAMETER;
	}
}