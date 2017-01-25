/*
 * helper.h
 *
 * Created: 25.01.2017 16:07:06
 *  Author: TOE
 */ 


#ifndef HELPER_H_
#define HELPER_H_

#include <inttypes.h>
#include <stddef.h>

#include "_global.h"
#include "circularBuffer.h"
#include "cmd.h"

at_status_t max_u32val     ( bufType_n bufType, size_t len, CMD *cmd, const device_mode devMode );
at_status_t max_u64val     ( bufType_n bufType, size_t len, CMD *cmd, const device_mode devMode );
at_status_t node_identifier( bufType_n bufType, size_t len, CMD *cmd, const device_mode devMode );
at_status_t ky_validator   ( bufType_n bufType, size_t len, CMD *cmd, const device_mode devMode );

#endif /* HELPER_H_ */