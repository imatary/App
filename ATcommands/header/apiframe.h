/*
 * apiframe.h
 *
 * Created: 25.11.2016 14:34:16
 *  Author: TOE
 */ 


#ifndef APIFRAME_H_
#define APIFRAME_H_

#include <inttypes.h>
#include "../header/enum_error.h"

#define STD_DELIMITER	(0x7E)
#define TX_MSG_64		(0x00)
#define TX_MSG_16		(0x01)
#define AT_COMMAND		(0x08)
#define AT_COMMAND_Q	(0x09)
#define REMOTE_AT_CMD	(0x17)
#define RX_MSG_64		(0x80)
#define RX_MSG_16		(0x81)

ATERROR API_frameHandle_uart(void);


#endif /* APIFRAME_H_ */