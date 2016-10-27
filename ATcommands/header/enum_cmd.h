/*
 * enum_cmd.h
 *
 * Created: 26.10.2016 13:11:43
 *  Author: TOE
 */ 


#ifndef ENUM_CMD_H_
#define ENUM_CMD_H_

/*
 * AT Command IDs.
 *
 * Commands are enumerated, starting with zero and packed to save memory.
 */

typedef enum
{
	AT_CMD1 = 0,
	AT_CMD2,
	AT_CMD3,
	AT_CMD4,
	AT_CMD5,
	AT_CMD6,
	AT_CMD7,
	AT_CMDn,
	
}__attribute__((packed)) cmdIDs;



#endif /* ENUM_CMD_H_ */