/*
 * cmd.h
 *
 * Created: 26.10.2016 08:46:58
 *  Author: TOE
 */ 


#ifndef CMD_H_
#define CMD_H_

#include <stdint.h>		// uint8_t type
#include <stdio.h>		// sizeof(), size_t
#include "enum_cmd.h"	// enumerated commands

// === defined values =====================================
#define READ	 (0x8)
#define WRITE	 (0x4)
#define EXEC	 (0x2)

// === struct and struct table ============================
typedef struct {
	const char *name;
	cmdIDs		ID;			// AT ID of enum type cmdIDs
	uint8_t		rwxAttrib;	// 8 Bit for rwx or rwx Attrib bit field
}__attribute__((packed)) CMD;

CMD *CMD_findInTable(uint8_t *cmd);

#endif /* CMD_H_ */