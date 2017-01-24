/*
 * at_parser.c
 *
 * Created: 18.01.2017 13:08:21
 *  Author: TOE
 */ 
// === includes ===========================================
#include <inttypes.h>							// uint8/16/32

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

// === prototypes =========================================
static uint32_t AT_GT_timeHandle(uint32_t arg);
uint32_t AT_CT_timeHandle(uint32_t arg);

// === functions ==========================================

void AT_parser( uint8_t inchar, bufType_n bufType )
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
	switch(state)
	{
	case STATEM_IDLE :
		if ( inchar == commandSequenceChar )
		{
			th = deTIMER_start(AT_GT_timeHandle, deMSEC( guartTimes ), 0 );
			counter += 1;
			state = AT_CC_COUNT;
		}
		break;	
	
	case AT_CC_COUNT :
		if ((counter == 1 || counter == 2) && inchar != commandSequenceChar || counter == 3)
		{
			th = deTIMER_stop(th);
			counter = 0;
			state = STATEM_IDLE;
		}
		else
			th = deTIMER_restart(th, deMSEC( guartTimes ) );
			counter += 1;
		break;
		
	case AT_MODE :
		if ( 0 == th ) 
		{
		UART_puts("TIMER START");
			th = deTIMER_start(AT_CT_timeHandle,  ATcmdTimeOut , 0 );
			deBufferReset( bufType );
		}
		counter += 1;
		
		if( '\r' == inchar ) 
		{
			th = deTIMER_restart(th, ATcmdTimeOut );
			state = AT_HANDLE;
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
	if ( AT_MODE == state && ' ' == inchar && counter < 4 );
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
	if ( STATEM_IDLE == state && 0 == counter )
	{ 
		TRX_send( bufType, 0, NULL, 0);
	}
	
	/*
	 * if counter greater than 0 and state machine in AT handle mode
	 * - delete buffer if command length less then 4 signs and carriage return
	 * - if command length equal 4 signs and carriage return cal read/exec function
	 * - if command length greater then 4 signs and carriage return cal write function
	 */
	if ( AT_HANDLE == state && counter <  5 )
	{
		ret = INVALID_COMMAND;
	}
	else if( AT_HANDLE == state && counter == 5 )
	{
		state = AT_MODE;
		ret = CMD_readOrExec(&th, bufType);
		counter = 0;
	}
	else if( AT_HANDLE == state && counter >  5 ) 
	{
		state = AT_MODE;
		ret = CMD_write( counter, bufType);
		counter = 0;
	}
	
	if (ret)
	{
		UART_print_status(ret);
		deBufferReset( bufType );
		ret = 0;
		counter = 0;
	}
	
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
static uint32_t AT_GT_timeHandle(uint32_t arg)
{
	if ( counter == 3 ) 
	{
		UART_print_status(OP_SUCCESS);
		state = AT_MODE;
	}
	else
	{
		state = STATEM_IDLE;
	}
	counter = 0;
	th      = 0;
	
	return 0;
}

uint32_t AT_CT_timeHandle(uint32_t arg)
{
	state   = STATEM_IDLE;
	counter = 0;
	th      = 0;
	UART_print_status(QUIT_CMD_MODE);
	
	return 0;
}