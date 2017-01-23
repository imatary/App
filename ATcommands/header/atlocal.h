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

at_status_t max_u32val     ( bufType_n bufType, size_t len, CMD *cmd );
at_status_t max_u64val     ( bufType_n bufType, size_t len, CMD *cmd );
at_status_t node_identifier( bufType_n bufType, size_t len, CMD *cmd );
at_status_t ky_validator   ( bufType_n bufType, size_t len, CMD *cmd );

#endif /* ATLOCAL_H_ */