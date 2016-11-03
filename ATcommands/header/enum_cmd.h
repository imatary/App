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
	AT_CH = 0,
	AT_DH,
	AT_DL,
	AT_ID,
	AT_CMD5,
	AT_CMD6,
	AT_CMD7,
	AT_CMDn,
	
}__attribute__((packed)) cmdIDs;



#endif /* ENUM_CMD_H_ */