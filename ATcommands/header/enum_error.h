/*
 * enum_error.h
 *
 * Created: 11.10.2016 10:02:34
 *  Author: TOE
 */ 


#ifndef ENUM_ERROR_H_
#define ENUM_ERROR_H_

#include "_global.h"

/**
 * Enumerated error codes,
 * starting with -1 and counted into negative direction.
 *
 * last modified: 2016/10/26
 */

typedef enum
{
	NOT_ABLE_TO_WRITE		= -1,
	NOT_ABLE_TO_READ		= -2,

	/* ADD NEW CODES HERE! */
}__attribute__((packed)) ATERROR;


#endif /* ENUM_ERROR_H_ */