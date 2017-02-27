/*
 * ap_read.c
 *
 * Created: 26.01.2017 11:33:07
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>

#include "../header/_global.h"
#include "../header/cmd.h"						// AT command search parser
#include "../header/ap_frames.h"				// AP set functions
#include "../header/rfmodul.h"					// RFmodul struct
#include "../../ATuracoli/stackrelated.h"		// uart functions

// === globals ============================================
static uint8_t workArray[MAX_PARAMETER_LENGHT];

// === functions ==========================================
/*
 * Command read function reads the values of the memory
 *
 * Received:
 *		CMD		pointer to AT command in command table
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		ERROR				if one of these commands SS, R?, SB
 *
 * last modified: 2017/01/26
 */
at_status_t AP_read( const CMD *cmd )
{
	uint16_t length;

	SET_apFrameRWXopt(READ);

	switch( cmd->ID )
	{
		case AT_pC :
			{
				workArray[0] = 0x1;
				SET_apFrameMsg ( workArray, 1, AT_pC );
			}
			break;

		case AT_SS :
		case AT_Rq :
		case AT_SB : return ERROR;

		default :
			{
				 length = GET_deviceValue( workArray, cmd );
				 SET_apFrameLength( length, FALSE);
				 SET_apFrameMsg ( workArray, length, cmd->ID );
			}
			break;
	}

	return OP_SUCCESS;
}

