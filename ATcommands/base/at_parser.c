/*
 * at_parser.c
 *
 * Created: 18.01.2017 13:08:21
 *  Author: TOE
 */ 
// === includes ===========================================
#include <inttypes.h>								// uint8/16/32

#include "../header/_global.h"						// bool_t
#include "../header/atlocal.h"						// prototypes
#include "../header/rfmodul.h"						// device get and set functions
#include "../header/cmd.h"							// prototypes for rwx functions
#include "../header/circularBuffer.h"				// buffer
#include "../../ATuracoli/stackrelated.h"			// UART functions
#include "../../ATuracoli/stackrelated_timer.h"		// timer

// === functions ==========================================

void AT_parser( uint8_t inchar, bufType_n bufType, timeStat_t *th )
{
	at_status_t ret = 0;
	uint8_t commandSequenceChar = GET_atcopCMD_cc();
	uint16_t guartTimes			= GET_atcopCMD_gt();
	uint16_t ATcmdTimeOut		= deMSEC( GET_atcopCMD_ct() ) * 0x64;
	
	/*
	 * if a CC character received, start timer and count CC character signs 
	 * if the user hit three times the CC character button, he needs to wait 
	 * until the timer has expired to switch into local command mode
	 *
	 * else kill the timer and reset the counter
	 */
	switch(th->state)
	{
	case STATEM_IDLE :
		if ( inchar == commandSequenceChar )
		{
			th->watch = deTIMER_start(AT_GT_timeHandle, deMSEC( guartTimes ), 0 );
			th->counter += 1;
			th->state = AT_CC_COUNT;
		}
		break;	
	
	case AT_CC_COUNT :
		if ((th->counter == 1 || th->counter == 2) && inchar != commandSequenceChar || th->counter == 3)
		{
			th->watch = deTIMER_stop(th->watch);
			th->counter = 0;
			th->state = STATEM_IDLE;
		}
		else
			th->watch = deTIMER_restart(th->watch, deMSEC( guartTimes ) );
			th->counter += 1;
		break;
		
	case AT_MODE :
		if ( 0 == th->watch ) 
		{
		UART_puts("TIMER START");
			th->watch = deTIMER_start(AT_CT_timeHandle,  ATcmdTimeOut , 0 );
			deBufferReset( bufType );
		}
		th->counter += 1;
		
		if( '\r' == inchar ) 
		{
			th->watch = deTIMER_restart(th->watch, ATcmdTimeOut );
			th->state = AT_HANDLE;
			inchar = '\0';
		}
		break;
	}
	
	/*
	 * push the character into the buffer
	 * neither interrupts allowed
	 *
	 * if state machine in AT mode and within the first 5 characters a space character, 
	 * don't store it into the buffer and don't count,
	 */
	if ( AT_MODE == th->state && ' ' == inchar && th->counter < 4 );
	else
	{
		cli(); ret = deBufferIn( bufType, inchar ); sei();
		if( ret )
		{
			UART_print_status(ret);
			deBufferReset( bufType );
			ret = 0;
		}
	}
	
								
	/*
	 * if counter = 0 and state machine in idle mode send the buffer content 
	 */					
	if ( STATEM_IDLE == th->state && 0 == th->counter )
	{ 
		TRX_send( bufType, 0, NULL, 0);
	}
	
	/*
	 * if counter greater than 0 and state machine in AT handle mode
	 * - delete buffer if command length less then 4 signs and carriage return
	 * - if command length equal 4 signs and carriage return cal read/exec function
	 * - if command length greater then 4 signs and carriage return cal write function
	 */
	if ( AT_HANDLE == th->state && th->counter <  5 )
	{
		ret = INVALID_COMMAND;
	}
	else if( AT_HANDLE == th->state && th->counter == 5 )
	{
		th->state = AT_MODE;
		ret = CMD_readOrExec( &th->watch, bufType);
		th->counter = 0;
	}
	else if( AT_HANDLE == th->state && th->counter >  5 ) 
	{
		th->state = AT_MODE;
		ret = CMD_write( th->counter, bufType);
		th->counter = 0;
	}
	
	if (ret)
	{
		UART_print_status(ret);
		deBufferReset( bufType );
		ret = 0;
		th->counter = 0;
	}
	
}