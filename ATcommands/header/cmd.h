/*
 * cmd.h
 *
 * Created: 26.10.2016 08:46:58
 *  Author: TOE
 */ 


#ifndef CMD_H_
#define CMD_H_

#include <inttypes.h>		// uint8_t type

#include "circularBuffer.h"	// buffer functions
#include "enum_status.h"	// at_status_t
#include "enum_cmd.h"		// enumerated commands

// === defined values =====================================
#define READ	 (0x8)
#define WRITE	 (0x4)
#define EXEC	 (0x2)

// === struct and struct table ============================
typedef struct command {
	const char		*name;
	const cmdIDs	ID;			// AT ID of enum type cmdIDs
	const uint8_t	rwxAttrib;	// 8 Bit for rwx or rwx Attrib bit field
	const uint8_t   cmdSize;
	const uint64_t	*min;
	const uint64_t  *max;
	void (*set) ( void*, size_t ); // data, length
	at_status_t (*valid_and_set) (bufType_n, size_t, struct command*); // buffer type, payload length, command
}__attribute__((packed)) CMD;

CMD *CMD_findInTable      (uint8_t  *cmd);
at_status_t CMD_readOrExec(uint32_t *th,  bufType_n bufType);
at_status_t CMD_write     (size_t   len,   bufType_n bufType);

#endif /* CMD_H_ */