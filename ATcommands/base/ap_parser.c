/*
 * ap_parser.c
 *
 * Created: 18.01.2017 13:08:52
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>
#include <stddef.h>
#include <avr/interrupt.h>

#include "../header/_global.h"					// bool_t
#include "../header/ap_frames.h"				// prototypes
#include "../header/rfmodul.h"					// get baud and packetization timeout
#include "../header/circularBuffer.h"			// buffer
#include "../../ATuracoli/stackrelated_timer.h"
#include "../../ATuracoli/stackdefines.h"
#include "../../ATuracoli/stackrelated.h"

// === defines ============================================
#define STATEM_IDLE	 0x00
#define AP_LENGTH_1	 0x01
#define AP_LENGTH_2	 0x02
#define AP_GET_DATA	 0x03
#define AP_CHECK_CRC 0x04
#define AP_HANDLE	 0x05

// === globals ============================================
static uint8_t      state = 0;
static uint16_t   counter = 0;
static uint16_t	       th = 0;
static uint32_t aptimeout = 0;
static bufType_n     ubuf = 0;

// === prototypes =========================================
static uint32_t AP_expired_timeHandle(uint32_t arg);

// === functions ==========================================
/*
 * Parser function (state machine)
 * reads the received character and switched the states
 * - if calculated CRC equal with received CRC, handle the frame
 * - if the timer expired or the frame not valid, reset the state machine
 *
 * Received:
 *		uint8_t		received character from UART
 *		deBuffer_t	buffer pointer in which the received characters should be stored
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/26
 */
void AP_parser( uint8_t inchar, bufType_n bufType )
{
	uint8_t ret;

	if ( (DIRTYB_RO & dirtyBits) == DIRTYB_RO )
	{
		dirtyBits ^= DIRTYB_RO;
		aptimeout = GET_serintCMD_ro() * 10000000;

		switch( GET_serintCMD_bd() )
		{
			case 0x0 : aptimeout /=   1200; break;
			case 0x1 : aptimeout /=   2400; break;
			case 0x2 : aptimeout /=   4800; break;
			case 0x3 : aptimeout /=   9600; break;
			case 0x4 : aptimeout /=  19200; break;
			case 0x5 : aptimeout /=  38400; break;
			case 0x6 : aptimeout /=  57600; break;
			case 0x7 : aptimeout /= 115200; break;
			default  : aptimeout /= deHIF_DEFAULT_BAUDRATE; break;
		}

		aptimeout = deMSEC( aptimeout );

	}
	ubuf = bufType;

	switch ( state )
	{
	case STATEM_IDLE :
		{
			if ( 0x7E == inchar )
			{
				th = deTIMER_start(AP_expired_timeHandle, aptimeout, 0 );
				state = AP_LENGTH_1;
			}
		}
		break;

	case AP_LENGTH_1 :
		{
			th = deTIMER_restart(th, aptimeout );
			SET_apFrameLength( (uint16_t) inchar << 8, FALSE ); // init new value
			state = AP_LENGTH_2;
		}
		break;

	case AP_LENGTH_2 :
		{
			th = deTIMER_restart(th, aptimeout );
			SET_apFrameLength( (uint16_t) inchar & 0xFF, TRUE ); // update value
			state = AP_GET_DATA;
		}
		break;

	case AP_GET_DATA :
		{
			th = deTIMER_restart(th, aptimeout );

			/*
			 * push the character into the buffer
			 * neither interrupts allowed
			 * if an buffer error occurred, reset the buffer
			 */
			ret = deBufferIn( bufType, inchar );
			if( BUFFER_IN_FAIL == ret )
			{
				deBufferReadReset( bufType, '+', counter);
				ret = 0;
			}

			if ( 0 == counter ) SET_apFrameCRC( inchar, FALSE ); // init new value
			else                SET_apFrameCRC( inchar, TRUE  ); // update value

			counter += 1;

			if ( counter == GET_apFrameLength() ) state = AP_HANDLE;
		}
		break;

	case AP_HANDLE :
		{
			th = deTIMER_stop(th);

			if ( OP_SUCCESS == COMPARE_apFrameCRC( inchar ) )
			{
				AP_frameHandle_uart( bufType );
			}
			counter = 0;
			state   = STATEM_IDLE;
		}
		break;

	default: /* do nothing */ break;

	}/* end state machine */
}


/*
 * Time handler for state machine
 * - if no other sign received and the timer is expired reset Buffer and state machine
 *
 * Received:
 *		uint32_t arg	this argument can be used in this function
 *
 * Returns:
 *		FALSE	to stop the timer
 *
 * last modified: 2017/01/18
 */
static uint32_t AP_expired_timeHandle(uint32_t arg)
{
	deBufferReset(ubuf);
	state   = STATEM_IDLE;
	counter = 0;

	return 0;
}