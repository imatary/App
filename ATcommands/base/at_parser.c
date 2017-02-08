/*
 * at_parser.c
 *
 * Created: 18.01.2017 13:08:21
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>							// uint8/16/32
#include <ctype.h>

#include "../header/_global.h"						// bool_t
#include "../header/rfmodul.h"						// device get and set functions
#include "../header/cmd.h"							// prototypes for rwx functions
#include "../header/at_commands.h"
#include "../header/circularBuffer.h"				// buffer
#include "../../ATuracoli/stackrelated.h"			// UART functions
#include "../../ATuracoli/stackrelated_timer.h"	// timer

// === defines ============================================
#define STATEM_IDLE		0x00
#define AT_CC_COUNT		0x10
#define AT_MODE			0x20
#define AT_HANDLE		0x30
#define TIMER_EXPIRED	0x40

// === globals ============================================
static uint8_t    state = STATEM_IDLE;
static uint16_t counter = 0;
static uint32_t	 GT_th;
static uint32_t  CT_th;
static uint32_t SEND_th;

static bufType_n        ubuf;
static uint16_t   guardTimes;
static uint16_t ATcmdTimeOut;
static uint8_t commandSequenceChar;

static CMD *pCommand  = NULL;

// === prototypes =========================================
static uint32_t		AT_guardTime_timeHandle(uint32_t arg);
static uint32_t		AT_commandTime_timeHandle(uint32_t arg);
static uint32_t		AT_sendTX_timeHandle(uint32_t arg);
static at_status_t	AT_getCommand( bufType_n bufType, CMD **cmd );

// === functions ==========================================
/*
 * AT parser analyzed inchar
 * 1. whether it is a command sequence character
 *    - yes: it set state machine to AT_CC_COUNT state and start the guard times timer
 *	  - no : the state machine remained in STATEM_IDLE state
 *
 * 2. whether it is a command sequence character and state machine in AT_CC_COUNT state
 *    - yes: restart the guard times timer
 *	  - no : stop the guard times timer and set state machine to STATEM_IDLE state
 *
 * 3. whether it is an carriage return character and state machine in AT_MODE state
 *    - yes: restart command times timer, set inchar to 0x0 and state machine to AT_HANDLE
 *	  - no : do nothing at this point
 *
 * and pushed the character into the buffer.
 * If state machine in STATEM_IDLE state and counter not equal 0 start or restart the send timer.
 * If counter greater than 0 and state machine in AT_HANDLE state, check counter size and start
 * read, write or exec function.
 *
 * Receives:
 *		uint8_t		copy of the incoming character
 *		bufType_n	the buffer type where to store the character
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/02/08
 */
void AT_parser( uint8_t inchar, bufType_n bufType )
{
	static at_status_t ret = 0;
	commandSequenceChar = GET_atcopCMD_cc();
	guardTimes			= GET_atcopCMD_gt();
	ATcmdTimeOut		= deMSEC( (uint32_t) GET_atcopCMD_ct() * 0x64UL );
	ubuf                = bufType;

	/*
	 * if a CC character received, start timer and count CC character signs
	 * if the user hit three times the CC character button, he needs to wait
	 * until the timer has expired to switch into local command mode
	 *
	 * else kill the timer and reset the counter
	 */
	switch(state)
	{
	case STATEM_IDLE :
		{
			if ( inchar == commandSequenceChar )
			{
				GT_th = deTIMER_start(AT_guardTime_timeHandle, deMSEC( guardTimes ), 0 );
				state = AT_CC_COUNT;
			}
		}
		break;

	case AT_CC_COUNT :
		{
			if ((counter == 1 || counter == 2) && inchar != commandSequenceChar || counter == 3)
			{
				GT_th = deTIMER_stop(GT_th);
				counter = 0;
				state = STATEM_IDLE;
			}
			else { GT_th = deTIMER_restart(GT_th, deMSEC( guardTimes ) ); }
		}
		break;

	case AT_MODE :
		{
			if( '\r' == inchar )
			{
				CT_th = deTIMER_restart(CT_th, ATcmdTimeOut );
				state = AT_HANDLE;
				inchar = 0;
			}
		}
		break;

	default : break;
	}

	/*
	 * push the character into the buffer
	 * neither interrupts allowed
	 *
	 * if state machine in AT mode and within the first 5 characters a space character,
	 * don't store it into the buffer and don't count,
	 */
	if ( AT_MODE == state && ' ' == inchar && counter < 4 );
	else
	{
		ret = deBufferIn( bufType, inchar );
		counter += 1;

		if( OP_SUCCESS != ret )
		{
			UART_print_status(ret);
			deBufferReset( bufType );
			ret = 0;
		}
	}


	/*
	 * if counter != 0 and state machine in idle mode start send timer
	 */
	if ( STATEM_IDLE == state && 0 != counter )
	{
		if ( 0 == SEND_th )
		{
			SEND_th = deTIMER_start(AT_sendTX_timeHandle, deMSEC(0x10), 0);
		}
		else
		{
			SEND_th = deTIMER_restart( SEND_th, deMSEC(0x10) );
		}
	}

	/*
	 * if counter greater than 0 and state machine in AT handle mode
	 * - delete buffer if command length less then 4 signs and carriage return
	 * - if command length equal 4 signs and carriage return cal read/exec function
	 * - if command length greater then 4 signs and carriage return cal write function
	 */

	if ( AT_HANDLE == state && 5 > counter )
	{
		UART_print_status(INVALID_COMMAND);
		deBufferReset( bufType );
		counter = 0;
	}
	else if ( AT_HANDLE == state && 4 < counter )
	{
		ret	= AT_getCommand( bufType, &pCommand );

		if      ( INVALID_COMMAND == ret || NULL == pCommand ) /* do nothing */;
		else if ( 5 == counter && EXEC & pCommand->rwxAttrib )
		{
			deBufferReadReset( bufType, '+', 1); // remove the '\0' from the buffer
			ret = AT_exec( &CT_th, pCommand->ID );
			UART_print_status(ret);
		}
		else if ( 5 == counter && READ & pCommand->rwxAttrib )
		{
			deBufferReadReset( bufType, '+', 1); // remove the '\0' from the buffer
			ret = AT_read( pCommand );
		}
		else
		{
			ret = AT_write( counter, bufType, pCommand );
			UART_print_status(ret);
		}
		state = AT_MODE;
		ret = counter = 0;
	}
}

/*
 * Get command
 * - reads a string from buffer
 * - searched for the command in the command table
 * - store it into pCommand pointer
 *
 * Received:
 *		bufType_n	number of buffer type
 *		CMD			pointer for address in command table
 *
 * Returns:
 *     OP_SUCCESS		on success
 *	   INVALID_COMMAND	if command is not in command table
 *
 * last modified: 2017/01/26
 */
static at_status_t AT_getCommand( bufType_n bufType, CMD **cmd )
{
	static uint8_t cmdString[5];

	for (int i = 0; i < 4 ; i++)
	{
		deBufferOut( bufType, &cmdString[i] );
		if ( 'a' <= cmdString[i] && 'z' >= cmdString[i] ) cmdString[i] -= 0x20;
	}
	cmdString[4] = 0x0;

	*cmd = CMD_findInTable(cmdString);

	return ( NO_AT_CMD == (*cmd)->ID || NULL == *cmd )? INVALID_COMMAND : OP_SUCCESS;
}



/*
 * Time handler for state machine
 * - if 3x CC type is received set state machine to AT handle
 *   else reset counter and state machine
 * - if the GT timer expired reset state machine (quit At mode) and counter
 *
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2017/02/08
 */
static uint32_t AT_guardTime_timeHandle(uint32_t arg)
{
	if ( counter == 3 )
	{
		state = AT_MODE;
		UART_print_status(OP_SUCCESS);
		deBufferReset( ubuf );
		CT_th = deTIMER_start(AT_commandTime_timeHandle, ATcmdTimeOut, 0 );
	}
	else
	{
		state = STATEM_IDLE;
	}
	counter = 0;
	return 0;
}

uint32_t AT_commandTime_timeHandle(uint32_t arg)
{
	state   = STATEM_IDLE;
	counter = 0;
	UART_print_status(QUIT_CMD_MODE);

	return 0;
}

uint32_t AT_sendTX_timeHandle(uint32_t arg)
{
	deBufferIn( ubuf, 0x00 );
	TRX_send( ubuf, 0, NULL, 0);
	SEND_th = 0;
	counter = 0;
	return 0;
}