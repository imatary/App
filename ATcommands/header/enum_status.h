/*
 * enum_status.h
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

	TIMER_START_FAIL	= -15,
	TRX_INIT_ERROR		= -14,
	BUFFER_IN_FAIL		= -13,
	BUFFER_OUT_FAIL		= -12,
	NOT_ABLE_TO_WRITE	= -11,
	NOT_ABLE_TO_READ	= -10,
	COMMAND_MODE_FAIL	= -9,
	TRANSMIT_CRC_FAIL	= -8,
	AP_NOT_AVAILABLE	= -7,
	AP_FRAMESIZE_ERROR	= -6,
	TRANSMIT_IN_FAIL	= -5,
	TRANSMIT_OUT_FAIL	= -4,
	INVALID_PARAMETER	= -3,
	INVALID_COMMAND		= -2,
	ERROR				= -1,
	OP_SUCCESS			=  0,
	QUIT_CMD_MODE		=  1,

}__attribute__((packed)) at_status_t;

#endif /* ENUM_ERROR_H_ */