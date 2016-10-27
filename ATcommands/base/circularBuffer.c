/*
 * circularBuffer.c
 *
 * Created: 26.10.2016 07:54:25
 *  Author: TOE
 *
 * File related to the sample code of mikrocontroller.net
 * http://www.mikrocontroller.net/articles/FIFO
 */ 

#include "../header/circularBuffer.h"

/*
 * Set one byte in to circular buffer
 *
 * Returns:
 *     BUFFER_FAIL       buffer is full. No more space for more bytes.
 *     BUFFER_SUCCESS    the byte was saved
 *
 * last modified: 2016/10/26
 */
uint8_t BufferIn(Buffer *bufType, uint8_t inByte)
{
	uint8_t next = ((bufType->write + 1) & BUFFER_MASK);

	if (bufType->read == next)
	return BUFFER_FAIL; // full

	//buffer.data[buffer.write] = byte;
	bufType->data[bufType->write & BUFFER_MASK] = inByte; // absolutely secure (related to the author)
	bufType->write = next;

	return BUFFER_SUCCESS;
}

/*
 * Get 1 byte of the buffer, if the byte is ready
 *
 * Returns:
 *     BUFFER_FAIL       buffer is empty. he cannot send a byte.
 *     BUFFER_SUCCESS    1 Byte was delivered
 *
 * last modified: 2016/10/26
 */
uint8_t BufferOut(Buffer *bufType, uint8_t *pByte)
{
	if (bufType->read == bufType->write)
	return BUFFER_FAIL;

	*pByte = bufType->data[bufType->read];

	bufType->read = (bufType->read+1) & BUFFER_MASK;

	return BUFFER_SUCCESS;
}
