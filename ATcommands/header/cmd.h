/*
 * cmd.h
 *
 * Created: 26.10.2016 08:46:58
 *  Author: TOE
 */ 


#ifndef CMD_H_
#define CMD_H_

#include <inttypes.h>
#include "_global.h"
#include "enum_cmd.h"

#define READ	 (0x8)
#define WRITE	 (0x4)
#define EXEC	 (0x2)

typedef struct {
	const char	name;
	cmdIDs		ID;			// AT ID of enum type cmdIDs
	uint8_t		rwxAttrib;	// 8 Bit for rwx or rwx Attrib bit field
	uint8_t		Param;		// Pointer to parameter start position in UART_inBuf
	//uint8_t :1;			// reserves one byte, is it really necessary? If yes we will loose 1 byte with each AT command, with 50 commands we loose 50 byte for nothing.
}__attribute__((packed)) CMD;

static const CMD StdCmdTable[] =
{
	{ "ATCH", AT_CH, READ | WRITE 		},
	{ "ATDH", AT_DH, READ | WRITE		},
	{ "ATDL", AT_DL, READ | WRITE		},
	{ "ATCN", AT_CN, EXEC         		},
};

const CMD FLASH_ATTR* pStdCmdTable = StdCmdTable;
const size_t command_count = sizeof(StdCmdTable)/sizeof(CMD);

#endif /* CMD_H_ */