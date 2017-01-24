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

// === defines ============================================
#define STATEM_IDLE		0x00
#define AT_CC_COUNT		0x10
#define AT_MODE			0x20
#define AT_HANDLE		0x30
#define TIMER_EXPIRED	0x40

// === prototypes ==========================================

void AT_parser( uint8_t inchar, bufType_n bufType, timeStat_t *th );

at_status_t max_u32val     ( bufType_n bufType, size_t len, CMD *cmd );
at_status_t max_u64val     ( bufType_n bufType, size_t len, CMD *cmd );
at_status_t node_identifier( bufType_n bufType, size_t len, CMD *cmd );
at_status_t ky_validator   ( bufType_n bufType, size_t len, CMD *cmd );


uint32_t AT_GT_timeHandle(uint32_t arg);
uint32_t AT_CT_timeHandle(uint32_t arg);

#endif /* ATLOCAL_H_ */