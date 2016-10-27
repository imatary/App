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

typedef struct {
	cmdIDs  ID;				// AT ID of enum type cmdIDs
	uint8_t rwxAttrib;		// 8 Bit for rwx or rwx Attrib bit field
	uint8_t Param;			// Pointer to parameter start position in UART_inBuf
	//uint8_t :1;			// reserves one byte, is it really necessary? If yes we will loose 1 byte with each AT command, with 50 commands we loose 50 byte for nothing.
}__attribute__((packed)) CMD;

/*
 * get methods
 *
 * get a pointer to the command struct and
 * returned the requested value and type
 *
 * last modified: 2016/10/11
 */
	cmdIDs 		  get_cmdID	( CMD *cmdStruct );	// returned the enum cmdID
	uint8_t		  get_Param	( CMD *cmdStruct );	// returned uint8_t pointer position
	uint8_t	 	  get_rwx	( CMD *cmdStruct );	// returned the rwx struct
	unsigned char get_r		( CMD *cmdStruct );	// returned unsigned (int)
	unsigned char get_w		( CMD *cmdStruct );	// returned unsigned (int)
	unsigned char get_x		( CMD *cmdStruct );	// returned unsigned (int)
/*
 * DBG + print methods
 *
 * get a pointer to the command struct and printed the values to the screen
 * returned nothing
 *
 * last modified: 2016/10/11
 */
#if DEBUG
	void print_cmd	( CMD *cmdStruct );
	void print_ID	( CMD *cmdStruct );
	void print_rwx	( CMD *cmdStruct );
	void print_r	( CMD *cmdStruct );
	void print_w	( CMD *cmdStruct );
	void print_x	( CMD *cmdStruct );
	void print_parm ( CMD *cmdStruct );
#endif



#endif /* CMD_H_ */