/*
 * atlocal.h
 *
 * Created: 15.11.2016 11:33:30
 *  Author: TOE
 */ 


#ifndef ATLOCAL_H_
#define ATLOCAL_H_

#include "_global.h"
#include "enum_error.h"	// ATERROR

// === functions ==========================================
ATERROR	 AT_localMode(void);
bool_t   charToUint8(uint8_t *cmdString, size_t *strlength, size_t *cmdSize ,size_t maxCmdSize);

#endif /* ATLOCAL_H_ */