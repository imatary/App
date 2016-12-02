/*
 * apiframe.h
 *
 * Created: 25.11.2016 14:34:16
 *  Author: TOE
 */ 


#ifndef APIFRAME_H_
#define APIFRAME_H_

#include <inttypes.h>
#include "enum_error.h"
#include "cmd.h"

#define STD_DELIMITER	(0x7E)
#define TX_MSG_64		(0x00)
#define TX_MSG_16		(0x01)
#define AT_COMMAND		(0x08)
#define AT_COMMAND_Q	(0x09)
#define REMOTE_AT_CMD	(0x17)
#define DEVICE_AT_CMD	(0x18)
#define RX_MSG_64		(0x80)
#define RX_MSG_16		(0x81)

struct api_f 
{
	ATERROR  ret;			// 1 Byte
	uint8_t  rwx;			// 1 Byte
	uint8_t	 delimiter;		// 1 Byte
	uint16_t length;		// 2 Byte
	uint8_t  type;			// 1 Byte
	uint8_t  cmd[3];		// 3 Byte
	uint8_t  id;			// 1 Byte
	uint8_t  msg[256];		// 256 Byte
	/*
	 * create the frame & calc checksum
	 * 0xFF - (API type + frame ID [+ target address] [+ options] + main content [+ parameter]) = checksum
	 *        |<---------------------------------- frame frame->bufLength ------------------->|
	 */
	uint8_t  crc;			// 1 Byte
};

ATERROR   API_frameHandle_uart(size_t *len);
CMD*   API_findInTable(struct api_f *frame, uint8_t *array);
bool_t API_compareCRC(struct api_f *frame);


#endif /* APIFRAME_H_ */