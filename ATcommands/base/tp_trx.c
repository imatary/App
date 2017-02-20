/*
 * tp_trx.c
 *
 * Created: 26.01.2017 14:04:38
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>

#include "../header/_global.h"
#include "../header/rfmodul.h"
#include "../header/cmd.h"
#include "../../ATuracoli/stackrelated.h"

// === defines ============================================
#define ACK_WITH_MAXSTREAM    0x0
#define NO_ACK_NO_MAXSTREAM   0x1
#define ACK_NO_MAXSTREAM	  0x2
#define NO_ACK_WITH_MAXSTREAM 0x3

// === globals ============================================
static CMD *pCommand = NULL;
static uint8_t macHeader[24];
static int macHeaderSize;
static device_t *RFmodul;

// === prototypes =========================================
static void SET_macHeader(void);

// === functions ==========================================
/*
 * Get functions for status informations
 *
 * Received:
 *		uint8_t		pointer to rx work buffer
 *		uint8_t		length of the frame
 *		uint8_t		start position of data payload
 *		uint16_t	pointer to mac header value
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/26
 */
void TRX_printContent( bufType_n bufType, uint8_t flen, uint8_t dataStart )
{
	RFmodul = GET_device();
	dataStart += ( NO_ACK_NO_MAXSTREAM == RFmodul->netCMD_mm ||\
	               ACK_NO_MAXSTREAM    == RFmodul->netCMD_mm )? 0 : 2;

	if ( 0x08 & GET_deBufferByteAt(RX_WORK_BUF, 0) )	// security enabled
	{
		UART_print("Cant read data now!"); /* TODO */
	}
	else					    // security disabled
	{
		for (uint8_t i = dataStart; i < flen-2 ; i++)
		{
			UART_putc( GET_deBufferByteAt(RX_WORK_BUF, i) );

		}/* end for loop */
	}
}



/*
 * Simple text frame
 * prepared frame for simple text message
 *
 * Received:
 *		uint8_t		pointer to dest array
 *		uint8_t		pointer to src array
 *		size_t		length of input
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2017/01/26
 */
int TRX_msgFrame( bufType_n bufType, uint8_t *package )
{
	        RFmodul = GET_device();
	uint8_t   u8tmp = *(package+2);
	at_status_t ret = 0;

	/*
	 * if the value of pan id, dest. addr or src. short addr changed
	 * create new mac header
	 */
	if ( dirtyBits & 0x68 )
	{
		SET_macHeader();
		dirtyBits ^= (dirtyBits & 0x68);
	}

	memcpy( (uint8_t*) package, (uint8_t*) macHeader, macHeaderSize);
	*(package+2) = u8tmp;

	int pos = macHeaderSize;

	if ( NO_ACK_NO_MAXSTREAM != RFmodul->netCMD_mm ||\
	     ACK_NO_MAXSTREAM    != RFmodul->netCMD_mm )
	{
		*( package + pos     ) = (uint8_t) ( *( package + 2 ) + RFmodul->netCMD_sl & 0xFF );	// second counter, distance to frame counter is last byte of src extended addr.
		*( package + pos + 2 ) = 0x00;													        // this byte will be used as command, 0x00 = no command is given
		pos += 2;
	}

	while ( 0x00 != u8tmp )
	{
		ret = deBufferOut(bufType, &u8tmp);
		if ( BUFFER_OUT_FAIL == ret ) break;
		*( package + pos) = u8tmp;
		pos++;
	}

	/*
	 *	if no data has been read, return 0
	 */
	if ( pos <= macHeaderSize ) return 0;
	else                        return pos-1;
}



/*
 *
 */
static void SET_macHeader(void)
{
	RFmodul = GET_device();
	uint64_t u64tmp;

	/*
	 * data frame			(bit 0)
	 */
	macHeader[0] = 0x01;

	/*
	 * security active		(bit 3)
	 */
	if ( TRUE == RFmodul->secCMD_ee ) macHeader[0] |= 0x08;

	/*
	 * ACK on				(bit 5)
	 */
	if ( ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm ||\
	     ACK_NO_MAXSTREAM   == RFmodul->netCMD_mm    ) macHeader[0] |= 0x20;

	/*
	 * PAN Compression		(bit 6)
	 */
	macHeader[0] |= 0x40;

	/*
	 * destination PAN_ID
	 */
	memcpy( &macHeader[3], &RFmodul->netCMD_id, 2);

	/*
	 * destination addr.
	 * extended or short
	 */
	u64tmp = (uint64_t) RFmodul->netCMD_dh << 32 | RFmodul->netCMD_dl;

	if ( 0xFFFF < u64tmp )
	{
		memcpy( &macHeader[5], &u64tmp, 8 );
		macHeader[1] = 0x0C; // MAC header second byte (bit 2/3)
		macHeaderSize = 5+8;
	}
	else
	{
		memcpy( &macHeader[5], &u64tmp, 2 );
		macHeader[1] = 0x08; // MAC header second byte (bit 2/3)
		macHeaderSize = 5+2;
	}

	/*
	 * source addr.
	 * short or extended
	 */
	if ( 0xFFFE != RFmodul->netCMD_my )
	{
		memcpy( &macHeader[ macHeaderSize ], &RFmodul->netCMD_my, 2 );
		macHeader[1] |= 0x80; // MAC header second byte (bit 6/7)
		macHeaderSize += 2;
	}
	else
	{
		u64tmp = (uint64_t) RFmodul->netCMD_dh << 32 | RFmodul->netCMD_dl;

		memcpy( &macHeader[5], &u64tmp, 8 );
		macHeader[1] |= 0xC0; // MAC header second byte (bit 6/7)
		macHeaderSize += 8;
	}
}