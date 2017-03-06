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

void AT_parser( uint8_t inchar, bufType_n bufType );

at_status_t AT_read ( CMD *cmd );
at_status_t AT_exec ( uint16_t *th, cmdIDs cmdID);
at_status_t AT_write( size_t len,   bufType_n bufType, CMD *pCommand );

void TRX_printContent( bufType_n bufType, uint8_t flen, uint8_t dataStart );
int  TRX_msgFrame    ( bufType_n bufType, uint8_t *package );

#endif /* ATLOCAL_H_ */