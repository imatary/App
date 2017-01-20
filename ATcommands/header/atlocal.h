/*
 * atlocal.h
 *
 * Created: 15.11.2016 11:33:30
 *  Author: TOE
 */ 


#ifndef ATLOCAL_H_
#define ATLOCAL_H_

#include <inttypes.h>
#include "_global.h"
#include "circularBuffer.h"

// === prototypes ==========================================

void AT_parser( uint8_t inchar, bufType_n bufType );

#endif /* ATLOCAL_H_ */