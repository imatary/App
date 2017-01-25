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
#include "cmd.h"

// === prototypes ==========================================

void AT_parser( uint8_t inchar, bufType_n bufType, const device_mode devMode );

at_status_t AT_read ( CMD *pCommand );
at_status_t AT_exec ( uint32_t *th, cmdIDs cmdID);
at_status_t AT_write( size_t len,   bufType_n bufType, CMD *pCommand );

#endif /* ATLOCAL_H_ */