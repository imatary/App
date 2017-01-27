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
static uint8_t    state = 0;
static uint16_t counter = 0;
static uint32_t	     th = 0;
static uint32_t send_th = 0;

static bufType_n        ubuf = 0;
static uint16_t   guartTimes = 0;
static uint16_t ATcmdTimeOut = 0;
static uint8_t commandSequenceChar;

// === prototypes =========================================
static uint32_t AT_guardTime_timeHandle(uint32_t arg);
static uint32_t AT_commandTime_timeHandle(uint32_t arg);
static uint32_t AT_sendTX_timeHandle(uint32_t arg);

// === functions ==========================================

void AT_parser( uint8_t inchar, bufType_n bufType )
{
	static at_status_t ret = 0;
	commandSequenceChar = GET_atcopCMD_cc();
	guartTimes			= GET_atcopCMD_gt();
	ATcmdTimeOut		= deMSEC( GET_atcopCMD_ct() ) * 0x64;
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
				th = deTIMER_start(AT_guardTime_timeHandle, deMSEC( guartTimes ), 0 );
				counter += 1;
				state = AT_CC_COUNT;
			}
		}
		break;

	case AT_CC_COUNT :
		{
			if ((counter == 1 || counter == 2) && inchar != commandSequenceChar || counter == 3)
			{
				th = deTIMER_stop(th);
				counter = 0;
				state = STATEM_IDLE;
			}
			else { th = deTIMER_restart(th, deMSEC( guartTimes ) ); }
			counter += 1;
		}
		break;

	case AT_MODE :
		{
			counter += 1;
			if( '\r' == inchar )
			{
				th = deTIMER_restart(th, ATcmdTimeOut );
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
		if( OP_SUCCESS != ret )
		{
			UART_print_status(ret);
			deBufferReset( bufType );
			ret = 0;
		}
	}


	/*
	 * if counter = 0 and state machine in idle mode send the buffer content
	 */
	if ( STATEM_IDLE == state )
	{
		if ( 0 == send_th )
		{
			send_th = deTIMER_start(AT_sendTX_timeHandle, deMSEC(0x10), 0);
		}
		else
		{
			send_th = deTIMER_restart( send_th, deMSEC(0x10) );
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
		ret = INVALID_COMMAND;
	}
	else if ( AT_HANDLE == state && 4 < counter )
	{
		static CMD *pCommand  = NULL;
		ret	= CMD_getCommand( bufType, pCommand );

		if      ( INVALID_COMMAND == ret ) /* do nothing */;
		else if ( 5 == counter && EXEC == pCommand->rwxAttrib )
		{
			ret = CMD_exec( &th, pCommand->ID );
		}
		else if ( 5 == counter && READ == pCommand->rwxAttrib )
		{
			deBufferReadReset( bufType, '+', 1); // remove the '\0' from the buffer if in AT cmd mode
			ret = CMD_read( pCommand );
		}
		else
		{
			ret = CMD_write( counter, bufType, pCommand );
		}

		state = AT_MODE;
		counter = 0;
	}

	if ( OP_SUCCESS != ret )
	{
		UART_print_status(ret);
		deBufferReset( bufType );
		ret = 0;
		counter = 0;
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
at_status_t AT_getCommand( bufType_n bufType, CMD *pCommand )
{
	static uint8_t cmdString[5];

	for (int i = 0; i < 4 ; i++)
	{
		deBufferOut( bufType, &cmdString[i] );
		if ( 'a' <= cmdString[i] && 'z' >= cmdString[i] ) cmdString[i] -= 0x20;
	}

	pCommand = CMD_findInTable(cmdString);
	return ( NO_AT_CMD == pCommand->ID || NULL == pCommand )? INVALID_COMMAND : OP_SUCCESS;
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
 * last modified: 2017/01/18
 */
static uint32_t AT_guardTime_timeHandle(uint32_t arg)
{
	if ( counter == 3 )
	{
		UART_print_status(OP_SUCCESS);
		deBufferReset( ubuf );
		state = AT_MODE;
		th = deTIMER_start(AT_commandTime_timeHandle, deMSEC( ATcmdTimeOut ), 0 );
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
	th      = 0;
	UART_print_status(QUIT_CMD_MODE);

	return 0;
}

uint32_t AT_sendTX_timeHandle(uint32_t arg)
{
	TRX_send( ubuf, 0, NULL, 0);
	send_th = 0;
	return 0;
}