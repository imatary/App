/*
 * at_write.c
 *
 * Created: 18.01.2017 13:07:31
 *  Author: TOE
 */
// === includes ===========================================
#include <stddef.h>				// size_t

#include "../header/_global.h"	// device mode parameter
#include "../header/rfmodul.h"	// RFmodul struct
#include "../header/cmd.h"		// AT command parser

// === globals ============================================
static uint8_t  workArray[MAX_PARAMETER_LENGHT];

// === functions ==========================================
/*
 * Command write function write values into the memory
 *
 * Received:
 *		size_t		length of the parameter string
 *		bufType_n	buffer type which held the data
 *		CMD			pointer to AT command in command table
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_PARAMETER	if the delivered parameter is not valid
 *		ERROR				if no command was delivered
 *
 * last modified: 2017/01/25
 */
at_status_t AT_write(size_t len, bufType_n bufType, const CMD *cmd )
{
	if ( NULL == cmd ) return ERROR;

	READ_deBufferData_atReadPosition( bufType, workArray, len);

	return cmd->valid(len, workArray, cmd, TRANSPARENT_MODE);
}