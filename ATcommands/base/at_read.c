/*
 * cmd_read.c
 *
 * Created: 18.01.2017 13:07:13
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>

#include "../header/_global.h"
#include "../header/cmd.h"						// AT command search parser
#include "../header/rfmodul.h"					// get device value
#include "../../ATuracoli/stackrelated.h"		// uart functions

// === globals ============================================
static uint8_t workArray[MAX_PARAMETER_LENGHT];
static uint32_t u32val;

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
 * last modified: 2017/01/25
 */
at_status_t AT_read( CMD *cmd )
{
	switch( cmd->ID )
	{
		case AT_IA :
			{
				uint64_t value;
				GET_deviceValue( &value, cmd );
				UART_printf("%"PRIX32,      value >> 32 );			// need to be divided in two peaces because PRIX64 has an bug
				UART_printf("%"PRIX32"\r",  value & 0xFFFFFFFF );
			}
		break;

		case AT_pC :
			{
				UART_printf("%X\r", 0x1 );
			}
		break;

		case AT_SS :
		case AT_Rq :
		case AT_SB : return ERROR;

		case AT_NI :
			{
				GET_deviceValue( workArray, cmd );
				UART_printf("%s\r", workArray );
			}
		break;

		case DE_FV :
			{
				UART_printf("%s\r", AT_VERSION);
			}
		break;

		default :
			{
				u32val = 0;
				GET_deviceValue( &u32val, cmd );
				UART_printf("%"PRIX32"\r", u32val );
			}
		break;
	}

	if ( AT_AP == cmd->ID && cmd->max >= u32val )
	{
		SET_atAP_tmp(&u32val, cmd->cmdSize);
	}

	if ( AT_CT == cmd->ID && cmd->max >= u32val && cmd->min <= u32val )
	{
		SET_atCT_tmp(&u32val, cmd->cmdSize);
	}

	return OP_SUCCESS;
}