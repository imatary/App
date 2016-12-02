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
#include "apiframe.h"

// === functions ==========================================
bool_t  charToUint8(uint8_t *cmdString, size_t *strlength, size_t *cmdSize ,size_t maxCmdSize);
void	AT_localMode(void);
ATERROR CMD_readOrExec(struct api_f *frame, uint8_t *array, uint32_t *th);
ATERROR CMD_write(struct api_f *frame, uint8_t *array, size_t *len);
#endif /* ATLOCAL_H_ */