/*
 * cmd.h
 *
 * Created: 26.10.2016 08:46:58
 *  Author: TOE
 */


#ifndef CMD_H_
#define CMD_H_

// === includes ===========================================
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
	const char		*name;		                        // command name
	const cmdIDs	ID;			                        // AT ID of enum type cmdIDs
	const size_t	addr_offset;                        // offset of RFmodul struct
	const uint8_t	rwxAttrib;	                        // 8 Bit for rwx or rwx Attrib bit field
	const uint8_t   cmdSize;	                        // max command size depending on data type or array size
	const uint32_t	min;		                        // allowed minimum value
	const uint32_t  max;                                // allowed maximum value

	void        (*mySet) ( void*, size_t );				// set function (data, length)
	at_status_t (*valid) (size_t len,					// command parameter length
	                      const uint8_t *workArray,		// array pointer to workArray which contained command param
						  const struct command *cmd,	// pointer to command information in command table
						  const device_mode devMode		// information from which mode this function was called
						 );								// validation function, if data valid it call the set function

}__attribute__((packed)) CMD;

// === prototypes =========================================
CMD *CMD_findInTable(uint8_t  *cmd);
CMD *CMD_findInTableByID(cmdIDs id);

#endif /* CMD_H_ */