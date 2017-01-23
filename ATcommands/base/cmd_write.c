/*
 * cmd_write.c
 *
 * Created: 18.01.2017 13:07:31
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>

#include "../header/_global.h"
#include "../header/cmd.h"						// AT command parser
#include "../header/apiframe.h"					// AP set functions
#include "../header/rfmodul.h"					// RFmodul struct
#include "../../ATuracoli/stackrelated.h"		// uart

// === functions ==========================================
/*
 * CMD_write()
 * write values into the memory
 *
 * Received:
 *		if in AP Mode, a frame struct else a NULL pointer
 *		if in AP Mode, a pointer to an already allocated array for processing else  a NULL pointer
 *		if in  AT Mode, a time handler pointer else a NULL pointer
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *		INVALID_PARAMETER	if the delivered parameter is not valid
 *		ERROR				if an process error occurred
 *				 
 * last modified: 2017/01/04
 */				 
at_status_t CMD_write(size_t len, bufType_n bufType)
{	
	CMD *pCommand  = NULL;
	uint8_t pCmdString[5] = {'A','T',0,0,0};
		
	if ( AT_MODE_ACTIVE != GET_serintCMD_ap() ) // AP frame
	{ 
		cli(); deBufferOut( bufType, &pCmdString[2] ); sei();
		cli(); deBufferOut( bufType, &pCmdString[3] ); sei();
		if ( 'a' <= pCmdString[2] && 'z' >= pCmdString[2] ) pCmdString[2] -= 0x20;
		if ( 'a' <= pCmdString[3] && 'z' >= pCmdString[3] ) pCmdString[3] -= 0x20;
		
		AP_setATcmd(pCmdString);
		AP_setRWXopt(WRITE);
	}
	else // AT CMD
	{
		for (int i = 0; i < 4 ; i++)
		{
			cli(); deBufferOut( bufType, &pCmdString[i] ); sei();
			if ( 'a' <= pCmdString[i] && 'z' >= pCmdString[i] ) pCmdString[i] -= 0x20;
		}
	}
	
	/*
	 * search in command table
	 * if there not a valid command, leave function
	 * remove command length from total length
	 */
	pCommand = CMD_findInTable(pCmdString);
	if ( NO_AT_CMD == pCommand->ID || NULL == pCommand ) return INVALID_COMMAND;
		
	/*
	 * if writing is allowed store the value of the string into RFmodel struct
	 * else it is a invalid parameter
	 */	
	if (pCommand->rwxAttrib & WRITE )
	{
		return pCommand->valid_and_set( bufType, len-4, pCommand );
	}
	else
	{
		deBufferReadReset(bufType, '+', len+1); // delete parameter and crc sum
		return INVALID_COMMAND;
	}
}