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
#include <stdint.h>

#include "enum_status.h"
#include "_global.h"

#define DE_BUFFER_SIZE 256					// need to be 2^n (8, 16, 32, 64 ...), for each buffer
#define DE_BUFFER_MASK (DE_BUFFER_SIZE-1)	// do not forget the brackets

typedef struct {
	uint8_t data[DE_BUFFER_SIZE];
	uint8_t read;							// pointer to sector of oldest contend
	uint8_t write;							// pointer to empty sector
	bool_t	newContent;						// status if the buffer has new content received 
} deBuffer_t;

/*
 * two buffer are initialized to allow parallel work without conflicts
 * one Buffer for UART op
 * one Buffer for RX op
 *
 * remember both buffer have the size of BUFFER_SIZE
 */	
extern deBuffer_t UART_deBuf;
extern deBuffer_t   RX_deBuf;

void BufferInit			(deBuffer_t *bufType, ...);
void BufferNewContent	(deBuffer_t *bufType, bool_t val);
at_status_t BufferIn		(deBuffer_t *bufType, uint8_t inByte);
at_status_t BufferOut		(deBuffer_t *bufType, uint8_t *pByte);

/*
 * careful with this functions
 * to manipulate the buffer pointer can omit a crash or overwrite new data
 */
void deBufferReadReset	(deBuffer_t *bufType, char operand , uint8_t len);
void deBufferWriteReset	(deBuffer_t *bufType, char operand , uint8_t len);

#endif /* CIRCULARBUFFER_H_ */