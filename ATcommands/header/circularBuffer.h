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

#include "enum_error.h"

#define DE_BUFFER_SIZE 256				// need to be 2^n (8, 16, 32, 64 ...), for each buffer
#define DE_BUFFER_MASK (DE_BUFFER_SIZE-1) // do not forget the brackets

typedef struct {
	uint8_t data[DE_BUFFER_SIZE];
	uint8_t read;					// pointer to sector of oldest contend
	uint8_t write;					// pointer to empty sector
} deBuffer_t;

/*
 * two buffer are initialized to allow parallel work without conflicts
 * one Buffer for UART op
 * one Buffer for RX op
 *
 * remember both buffer have the size of BUFFER_SIZE
 */	
deBuffer_t UART_deBuf;
deBuffer_t   RX_deBuf;

void BufferInit();
ATERROR BufferIn(deBuffer_t *bufType, uint8_t inByte);
ATERROR BufferOut(deBuffer_t *bufType, uint8_t *pByte);

#endif /* CIRCULARBUFFER_H_ */