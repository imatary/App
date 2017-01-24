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
#include "../header/apiframe.h"					// prototypes
#include "../header/circularBuffer.h"			// buffer
#include "../../ATuracoli/stackrelated_timer.h"	// timer

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
 * last modified: 2017/01/18
 */
void AP_parser( uint8_t inchar, bufType_n bufType, timeStat_t *th )
{
	uint8_t ret;
	
	switch ( th->state )
	{
	case STATEM_IDLE :
		{
			if ( 0x7E == inchar )
			{
				th->watch = deTIMER_start(AP_expired_timeHandle, deMSEC( AP_TIMEOUT ), 0 );
				AP_setFrameLength( 0x0, FALSE);
				th->state = AP_LENGTH_1;
			}
		}
		break;
		
	case AP_LENGTH_1 :
		{
			th->watch = deTIMER_restart(th->watch, deMSEC( AP_TIMEOUT ) );
			AP_setFrameLength( (uint16_t) inchar << 8, TRUE );
			th->state = AP_LENGTH_2;
		}
		break;
		
	case AP_LENGTH_2 :
		{
			th->watch = deTIMER_restart(th->watch, deMSEC( AP_TIMEOUT ) );
			ret = AP_setFrameLength( (uint16_t) inchar & 0xFF, TRUE );
			if ( ret )	th->state = STATEM_IDLE;
			else		th->state = AP_GET_DATA;
		}
		break;
		
	case AP_GET_DATA :
		{
			th->watch = deTIMER_restart(th->watch, deMSEC( AP_TIMEOUT ) );
			
			/*
			 * push the character into the buffer
			 * neither interrupts allowed
			 * if an buffer error occurred, reset the buffer
			 */
			cli(); ret = deBufferIn( bufType, inchar ); sei();
			if( ret ) 
			{ 
				deBufferReadReset( bufType, '+', th->counter);
				ret = 0;
			}
			
			if ( 0 == th->counter ) AP_setCRC( inchar );
			else                    AP_updateCRC( inchar );
			
			th->counter += 1;
			
			if ( th->counter == AP_getFrameLength() ) th->state = AP_HANDLE;
		}
		break;
		
	case AP_HANDLE :
		{
			th->watch = deTIMER_stop(th->watch);
			
			if ( TRUE == AP_compareCRC( inchar ) )
			{
				AP_frameHandle_uart( bufType );
			}
			th->counter = 0;
			th->state   = STATEM_IDLE;
		}
		break;
						
	default: /* do nothing */ break;
		
	}/* end state machine */	
}