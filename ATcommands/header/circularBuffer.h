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

// === includes ===========================================
#include <inttypes.h>
#include <string.h>

#include "enum_status.h"
#include "_global.h"

// === defines ============================================
#define DE_BUFFER_SIZE 256					// need to be 2^n (8, 16, 32, 64, 128, 256), for each buffer (Bytes)
#define DE_BUFFER_MASK (DE_BUFFER_SIZE-1)	// do not forget the brackets

// === typedefs ===========================================
typedef enum
{
	NONE = -1,
	UART,
	RX,
	RX_WORK_BUF,

}__attribute__((packed)) bufType_n;

// === prototypes =========================================
bool_t GET_deBufferNewContent(bufType_n bufType);
void   SET_deBufferNewContent(bufType_n bufType, bool_t val);
uint8_t GET_deBufferByteAt( bufType_n bufType, uint8_t pos);

at_status_t deBufferIn	(bufType_n bufType, uint8_t inByte);
at_status_t deBufferOut	(bufType_n bufType, uint8_t *pByte);

void READ_deBufferData_atReadPosition(bufType_n bufType, uint8_t *workArray, size_t len);
void CPY_deBufferData( bufType_n dest, bufType_n src, size_t len);
/*
 * careful with this functions
 * to manipulate the buffer can omit a crash or overwrite new data
 */
void deBufferReset	   (bufType_n bufType);
void deBufferReadReset (bufType_n bufType, char operand , size_t len);
void deBufferWriteReset(bufType_n bufType, char operand , size_t len);

#endif /* CIRCULARBUFFER_H_ */