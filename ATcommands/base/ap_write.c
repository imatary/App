/*
 * ap_write.c
 *
 * Created: 25.01.2017 16:18:56
 *  Author: TOE
 */
// === includes ===========================================
#include <stddef.h>				// size_t

#include "../header/_global.h"	// device mode parameter
#include "../header/rfmodul.h"	// RFmodul struct
#include "../header/cmd.h"		// AT command parser
#include "../header/ap_frames.h"
#include "../../ATuracoli/stackrelated.h"

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
at_status_t AP_write( bufType_n bufType, const CMD *cmd )
{
	if ( NULL == cmd ) return ERROR;

	SET_apFrameRWXopt(WRITE);

	READ_deBufferData_atReadPosition( bufType, workArray, GET_apFrameLength()-4 );
	workArray[GET_apFrameLength()-5] = 0x0;

	return cmd->valid( GET_apFrameLength()-4, workArray, cmd, AP_MODE);
}