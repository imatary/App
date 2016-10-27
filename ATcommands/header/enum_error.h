/*
 * enum_error.h
 *
 * Created: 11.10.2016 10:02:34
 *  Author: TOE
 */ 


#ifndef ENUM_ERROR_H_
#define ENUM_ERROR_H_

#include "_global.h"

static void ATERROR_print(ATERROR *value);

/**
 * Enumerated error codes,
 * starting with -1 and counted into negative direction.
 *
 * last modified: 2016/10/27
 */

typedef enum
{
	OP_SUCCESS				=  0,
	TRX_INIT_ERROR			= -1,
	BUFFER_IN_FAIL			= -2,
	BUFFER_OUT_FAIL			= -3,
	TRANSMIT_OUT_FAIL		= -4,
	TRANSMIT_IN_FAIL		= -5,
	NOT_ABLE_TO_WRITE		= -6,
	NOT_ABLE_TO_READ		= -7,
	COMMAND_MODE_FAIL		= -8,

	/* ADD NEW CODES HERE! */
}__attribute__((packed)) ATERROR;


#endif /* ENUM_ERROR_H_ */