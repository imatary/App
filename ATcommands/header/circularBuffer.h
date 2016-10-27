/*
 * circularBuffer.h
 *
 * Created: 26.10.2016 08:09:07
 *  Author: TOE
 *
 * File related to the sample code of mikrocontroller.net
 * http://www.mikrocontroller.net/articles/FIFO
 */ 


#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_

#include <inttypes.h>

#define BUFFER_FAIL     0
#define BUFFER_SUCCESS  1

#define BUFFER_SIZE 256				// need to be 2^n (8, 16, 32, 64 ...)
#define BUFFER_MASK (BUFFER_SIZE-1) // do not forget the brackets

typedef struct {
	uint8_t data[BUFFER_SIZE];
	uint8_t read;					// pointer to sector of oldest contend
	uint8_t write;					// pointer to empty sector
} Buffer;

uint8_t BufferIn(Buffer *bufType, uint8_t inByte);
uint8_t BufferOut(Buffer *bufType, uint8_t *pByte);

#endif /* CIRCULARBUFFER_H_ */