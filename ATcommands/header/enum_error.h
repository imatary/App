/*
 * enum_error.h
 *
 * Created: 11.10.2016 10:02:34
 *  Author: TOE
 */ 


#ifndef ENUM_ERROR_H_
#define ENUM_ERROR_H_

/*
 * Enumerated error codes,
 * starting with -1 and counted into negative direction.
 *
 * last modified: 2016/10/27
 */

typedef enum
{
	/* ADD NEW CODES ON TOP! */
					
	TRX_INIT_ERROR		= -14,
	BUFFER_IN_FAIL,
	BUFFER_OUT_FAIL,
	NOT_ABLE_TO_WRITE,
	NOT_ABLE_TO_READ,
	COMMAND_MODE_FAIL,
	TRANSMIT_CRC_FAIL,
	API_NOT_AVAILABLE,
	API_FRAMESIZE_ERROR,
	TRANSMIT_IN_FAIL,
	TRANSMIT_OUT_FAIL,
	INVALID_PARAMETER,
	INVALID_COMMAND,
	ERROR,
	OP_SUCCESS			=  0,

	
}__attribute__((packed)) ATERROR;

#endif /* ENUM_ERROR_H_ */