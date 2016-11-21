/*
 * atlocal.h
 *
 * Created: 15.11.2016 11:33:30
 *  Author: TOE
 */ 


#ifndef ATLOCAL_H_
#define ATLOCAL_H_

#include "_global.h"	// bool_t
#include "enum_error.h"	// ATERROR
#include "cmd.h"		// CMD

// === functions ==========================================
ATERROR		AT_localMode(void);

static CMD*	CMD_findInTable(void);
static void CMD_readOrExec(uint32_t *th);
static void CMD_write(unsigned int *len);
uint32_t    CMD_timeHandle(uint32_t arg);
static bool_t charToUint8(uint8_t *pCmdString, int *len);

#endif /* ATLOCAL_H_ */