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
#define AT_COMMAND		(0x08)
#define REMOTE_AT_CMD	(0x17)

ATERROR API_frameHandle(void);


#endif /* APIFRAME_H_ */