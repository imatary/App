/*
 * circularBuffer.c
 *
 * Created: 26.10.2016 07:54:25
 *  Author: TOE
 *
 * File related to the sample code of mikrocontroller.net
 * http://www.mikrocontroller.net/articles/FIFO
 */ 

#include <stdlib.h>
#include <stdarg.h>						// variable argument list for buffer init
#include "../header/circularBuffer.h"	// struct, prototypes, defines

deBuffer_t UART_deBuf;
deBuffer_t   RX_deBuf;

/*
 * Set one byte in to circular buffer
 *
 * Returns:
 *     BUFFER_FAIL      buffer is full. No more space for more bytes.
 *     OP_SUCCESS		the byte was saved
 *
 * last modified: 2016/10/26
 */
ATERROR BufferIn(deBuffer_t *bufType, uint8_t inByte)
{
	uint8_t next = ((bufType->write + 1) & DE_BUFFER_MASK);

	if (bufType->read == next)
	return BUFFER_IN_FAIL; // full

	bufType->data[bufType->write] = inByte;

	//bufType->data[bufType->write & DE_BUFFER_MASK] = inByte; // absolutely secure (related to the author)
	bufType->write = next;
	
	return OP_SUCCESS;
}

/*
 * Get 1 byte of the buffer, if the byte is ready
 *
 * Returns:
 *     BUFFER_FAIL      buffer is empty. he cannot send a byte.
 *     OP_SUCCESS		1 Byte was delivered
 *
 * last modified: 2016/10/26
 */
ATERROR BufferOut(deBuffer_t *bufType, uint8_t *pByte)
{
	if (bufType->read == bufType->write)
	return BUFFER_OUT_FAIL;

	*pByte = bufType->data[bufType->read];

	bufType->read = (bufType->read+1) & DE_BUFFER_MASK;

	return OP_SUCCESS;
}

/*
 * Initialize the UART and the RX buffer
 * it takes a variable list of buffer
 * the last value need to be the NULL argument
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/10/28
 */
void BufferInit(deBuffer_t *bufType, ...)
{
	static va_list arg;
	deBuffer_t *bufout = bufType;
	
	va_start(arg,bufType);
	do 
	{
		for( int i = 0; DE_BUFFER_SIZE > i; i++ )
		{
			bufout->data[i] = 0;
		}
		bufout->read = 0;
		bufout->write = 0;
		bufout->newContent	= FALSE;
		bufout = va_arg(arg, deBuffer_t*);
		
	} while (bufout != NULL);
	va_end(arg);
}

/*
 * Helper functions
 *
 * Returns:
 *     nothing
 *
 * last modified: 2016/11/17
 */
void BufferNewContent(deBuffer_t *bufType, bool_t val)
{
	bufType->newContent = val;
}

void deBufferReadReset(deBuffer_t* bufType,char operand ,uint8_t len)
{
	if ( '-' == operand )
	{
		bufType->read -= len;
	}
	
	if ( '+' == operand )
	{
		bufType->read += len;
	}
	
}