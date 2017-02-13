/*
 * ap_trx.c
 *
 * Created: 19.01.2017 09:48:06
 *  Author: TOE
 */
// === includes ===========================================
#include <avr/interrupt.h>

#include "../header/ap_frames.h"
#include "../header/rfmodul.h"
#include "../../ATuracoli/stackrelated.h"
#include "../../ATuracoli/stackdefines.h"

// === defines ============================================
#define ACK_WITH_MAXSTREAM    0x0
#define NO_ACK_NO_MAXSTREAM   0x1
#define ACK_NO_MAXSTREAM	  0x2
#define NO_ACK_WITH_MAXSTREAM 0x3

// === globals ============================================
static device_t *RFmodul;
static uint8_t  workArray[PACKAGE_SIZE];
static uint8_t deviceAddr[10]; // device MAC addr 0-7 extended, 8-9 short

// === prototypes =========================================
static void prepareDeviceAddrArray(void);

// === functions (transreceiver) ==========================
/*
 * TRX_createAPframe
 * creates an AP frame and print it to UART
 *
 * Received:
 *		uint8_t		pointer to workArray
 *		uint8_t		length of the received frame
 *		uint8_t		position where the data payload starts
 *		uint8_t		length of source address
 *		uint8_t		MAC options -> foe decryption if necessary (not implementet)
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2017/01/27
 */
void TRX_createAPframe(bufType_n bufType, uint8_t flen, uint8_t dataStart, uint8_t srcAddrLen, uint8_t option)
{
	/*
	 * security handler not included
	 *  -- TODO add handler (uint8_t option field) --
	 */

	/*
	 * read option field (if included) and handle special frame
	 * else print a receive message
	 */
	switch ( GET_deBufferByteAt(bufType,dataStart+1) )
	{
		case 0x04 : AP_atRemoteFrame_localExec( bufType, flen, dataStart, srcAddrLen );  break;	// TX response only
		case 0x05 : AP_atRemote_response( bufType, flen, dataStart );					 break;	// UART response only
		default   : AP_rxReceive( bufType, flen, dataStart, srcAddrLen );                break;	// UART response only
	}

}

/*
 * AT Remote Command (remote request)
 * prepared frame for a remote AT command to control another device
 *
 * API frame sample:
 * 7E + length + frame type + frame ID + 64 bit dest. addr. + 16 bit dest addr. + option + cmd [+ parameter] + crc
 *             |            |-- data --|------- used in address creation -------|---------- data ------------|
 *             |--------------------------------- length 15 ------------------------------------|
 *
 * Received:
 *		uint8_t		pointer to send array
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2017/02/13
 */
int TRX_atRemoteFrame(bufType_n bufType, uint8_t *send)
{
	RFmodul = GET_device();
	size_t length = GET_apFrameLength();
	if ( 15 > length ) return 0;

	int	pos;

	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */
													   *(send)  = 0x01; // send data to _one_ device
	if ( TRUE == RFmodul->secCMD_ee )				   *(send) |= 0x08; // security active
	if ( ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm ||\
	     ACK_NO_MAXSTREAM   == RFmodul->netCMD_mm )    *(send) |= 0x20; // ACK on
													   *(send) |= 0x40; // PAN Compression on

	memcpy( send+3, &RFmodul->netCMD_id, 2);							// destination PAN_ID

	READ_deBufferData_atReadPosition(bufType, workArray, length-2);

	/*
	 * dest. address
	 *
	 * if 16bit address is deactivate
	 * - write destination ext. addr.
	 * else if byte 0 - 5 equal 0
	 * - write destination ext. addr. as short addr.
	 * else
	 * - write destination short addr.
	 */
	if ( workArray[8] != 0xFF || workArray[9] != 0xFE )
	{
		for(pos = 5; 5+2 > pos; pos++)
		{
			*(send+pos) = workArray[14-pos];
		}
		*(send+1) |= 0x08;								// MAC header second byte
	}
	else if ( workArray[0] == 0x00 &&\
	          workArray[1] == 0x00 &&\
			  workArray[2] == 0x00 &&\
			  workArray[3] == 0x00 &&\
			  workArray[4] == 0x00 &&\
			  workArray[5] == 0x00 )
	{
		for(pos = 5; 5+2 > pos; pos++)
		{
			*(send+pos) = workArray[12-pos];
		}
		 *(send+1) |= 0x08;								// MAC header second byte
	}
	else
	{
		for(pos = 5; 5+8 > pos; pos++)
		{
			*(send+pos) = workArray[12-pos];
		}
		*(send+1) |= 0x0C;								// MAC header second byte
	}


	/*
	 * src. address
	 */
	if ( 0x0 < RFmodul->netCMD_my )
	{
		memcpy( send+pos, &RFmodul->netCMD_my, 2);		// src. short address

		*(send+1) |= 0x80;								// MAC header second byte
		pos += 2;
	}
	else
	{
		memcpy( send+pos,   &RFmodul->netCMD_sl, 4 );
		memcpy( send+pos+4, &RFmodul->netCMD_sh, 4);	// src. ext. addr.

		*(send+1) |= 0xC0;								// MAC header second byte
		pos += 8;
	}

	if ( NO_ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm ||\
	     ACK_WITH_MAXSTREAM    == RFmodul->netCMD_mm )
	{
		*(send+pos)   = (uint8_t) ( *(send+2) + (RFmodul->netCMD_sl & 0xFF) );	// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x04;													    // this byte will be used as command, 0x04 AT remote command
		pos += 2;
	}


	/*
	 * data
	 */
	*(send+pos) = GET_apFrameID();
	pos += 1;

	if ( PACKAGE_SIZE-1 < pos+length-12 ) return 0;

	memcpy( send+pos, workArray, length-2 );

	return pos+length-2;
}



/*
 * AT Remote Command (response)
 * send result information of received and executed 0x17 Remote Command
 *
 * API frame sample:
 * 7E + length + frame type + frame ID + cmd + option [+ parameter] + crc
 *                          |--------------- data ------------------|
 *
 * Received:
 *		uint8_t		pointer to send array
 *		uint8_t		pointer to source address array
 *		uint8_t		source address length
 *
 * Returns:
 *		final position in array
 *
 * last modified: 2017/01/27
 */
int TRX_atRemote_response(bufType_n bufType, uint8_t *send, uint8_t *srcAddr, uint8_t srcAddrLen)
{
	RFmodul = GET_device();
	size_t length = GET_apFrameLength();
	if ( PACKAGE_SIZE-1 <= length ) return 0;

	int	pos	= 0;

	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */
													   *(send)  = 0x01; // send data to _one_ device
	if ( TRUE == RFmodul->secCMD_ee )				   *(send) |= 0x08; // security active
	if ( ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm ||\
	     ACK_NO_MAXSTREAM   == RFmodul->netCMD_mm )    *(send) |= 0x20; // ACK on
													   *(send) |= 0x40; // PAN Compression on

	memcpy( send+3, &RFmodul->netCMD_id, 2);							// destination PAN_ID

	/*
	 * dest. addr.
	 */
	for ( pos = 5; pos < srcAddrLen+5; pos++ )
	{
		*(send+pos) = *(srcAddr+pos-5);
	}
	*(send+1) |= (srcAddrLen == 8)? 0xC : 0x8;


	/*
	 * src. address
	 */
	if ( 0x0 < RFmodul->netCMD_my )
	{
		memcpy( send+pos, &RFmodul->netCMD_my, 2);							// src. short address

		*(send+1) |= 0x80;													// MAC header second byte
		pos += 2;
	}
	else
	{
		memcpy( send+pos,   &RFmodul->netCMD_sl, 4 );
		memcpy( send+pos+4, &RFmodul->netCMD_sh, 4 );					    // src. ext. addr.

		*(send+1) |= 0xC0;													// MAC header second byte
		pos += 8;
	}

	if ( NO_ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm ||\
	     ACK_WITH_MAXSTREAM    == RFmodul->netCMD_mm )
	{
		*(send+pos) = (uint8_t) ( *(send+2) + (RFmodul->netCMD_sl & 0xFF) );	// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x05;													// this byte will be used as command, 0x05 AT remote command response
		pos += 2;
	}


	/*
	 * data
	 */
	*(send+pos) = GET_apFrameID();											// AP Frame ID

	*(send+pos+1)  = RFmodul->netCMD_sh >> 24;
	*(send+pos+2)  = RFmodul->netCMD_sh >> 16;
	*(send+pos+3)  = RFmodul->netCMD_sh >>  8;
	*(send+pos+4)  = RFmodul->netCMD_sh >>  0;							    // src. ext. addr. high

	*(send+pos+5)  = RFmodul->netCMD_sl >> 24;
	*(send+pos+6)  = RFmodul->netCMD_sl >> 16;
	*(send+pos+7)  = RFmodul->netCMD_sl >>  8;
	*(send+pos+8)  = RFmodul->netCMD_sl >>  0;							    // src. ext. addr. low

	*(send+pos+9)  = RFmodul->netCMD_my >> 8;
	*(send+pos+10) = RFmodul->netCMD_my & 0xFF;							    // src. short address

	GET_apFrameATcmd( send, pos+11);										// + 2 pos for AT command
	*(send+pos+13) = GET_apFrameRet() * (-1);								// command status

	pos += 14;

	if ( PACKAGE_SIZE-1 < pos+length ) return 0;

	if ( 0 < length && WRITE != GET_apFrameRWXopt() )
	{
		GET_apFrameMsg( send, pos, length );
		pos += length;
	}

	return pos;
}



/*
 * Frame will be send as text message with a 64bit address
 *
 * API frame sample:
 * 7E + length + frame type + frame ID + 64 bit dest. addr. + option + payload + crc
 *             |                       |---- in address creation ----|- data --|
 *             |-------------------- length 11 ----------------------|
 *
 * Received:
 *		uint8_t		pointer to send array
 *
 * Returns:
 *		final position in array
 *
 * last modified: 2017/02/13
 */
int TRX_transmit64Frame(bufType_n bufType, uint8_t *send)
{
	RFmodul = GET_device();
	size_t length = GET_apFrameLength();
	if ( 11 >= length || PACKAGE_SIZE-1 < length ) return 0;

	int			pos		= 0;

	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write dest. address
	 * - write destination PANID
	 * - write src. address
	 */
										*(send)  = 0x01;	// send data to a device
	if ( TRUE == RFmodul->secCMD_ee )   *(send) |= 0x08;	// security active

	READ_deBufferData_atReadPosition(bufType, workArray, length-2);

	/*
	 * dest. address
	 */
	if ( workArray[0] == 0x00 &&\
		 workArray[1] == 0x00 &&\
		 workArray[2] == 0x00 &&\
		 workArray[3] == 0x00 &&\
		 workArray[4] == 0x00 &&\
		 workArray[5] == 0x00 )								// if command is a broadcast message
	{

		for (pos = 5; 5+2 > pos; pos++)
		{
			*(send+pos) = workArray[12-pos];
		}
		*(send+1) |= 0x08;									// MAC header second byte

	}
	else
	{
		for (pos = 5; 5+8 > pos; pos++)
		{
			*(send+pos) = workArray[12-pos];
		}
		*(send+1) |= 0x0C;									// MAC header second byte
	}


	/*
	 * handle of API Option field and  set PAN ID
	 */
	if ( ( ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm ||\
	       ACK_NO_MAXSTREAM   == RFmodul->netCMD_mm ) &&\
	       0x1 != workArray[8] )							// frame option "disable ACK"
	{
		*(send) |= 0x20;									// ACK on
	}

	if ( 0x4 == workArray[8] )								// frame option "PAN ID id Broadcast PAN ID
	{
		*(send+3) = 0xFF;
		*(send+4) = 0xFF;									// source PAN_ID

		memcpy( send+pos, &RFmodul->netCMD_id, 2);			// destination PAN_ID
		pos += 2;
	}
	else
	{
		*(send) |= 0x40;									// PAN Compression on
		memcpy( send+3, &RFmodul->netCMD_id, 2);			// destination PAN_ID = source PAN_ID
	}


	/*
	 * src. address
	 */
	if ( 0xFFFE != RFmodul->netCMD_my )
	{
		memcpy( send+pos, &RFmodul->netCMD_my, 2);			// src. short address

		*(send+1) |= 0x80;									// MAC header second byte
		pos += 2;
	}
	else
	{
		memcpy( send+pos, &RFmodul->netCMD_sl, 4);
		memcpy( send+pos+4, &RFmodul->netCMD_sh, 4);		// src. ext. addr.

		*(send+1) |= 0xC0;									// MAC header second byte
		pos += 8;
	}

	if ( NO_ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm ||\
	     ACK_WITH_MAXSTREAM    == RFmodul->netCMD_mm )
	{
		*(send+pos)   = (uint8_t) ( *(send+2) + (RFmodul->netCMD_sl & 0xFF));	// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x00;													// this byte will be used as command, 0x04 AT remote command
		pos += 2;
	}

	/*
	 * data
	 */
	if ( PACKAGE_SIZE-1 < pos+length-11 ) return 0;
	memcpy( send+pos, &workArray[9], length-11 );

	return pos+length-11 ;
}



/*
 * Frame will be send as text message with a 16bit address
 *
 * API frame smaple:
 * 7E + length + frame type + frame ID + 16 bit dest. addr. + option + payload + crc
 *             |                       |---- in address creation ----|- data --|
 *             |-------------------- length  5 ----------------------|
 *
 * Received:
 *		uint8_t		pointer to send array
 *
 * Returns:
 *		final position in array
 *
 * last modified: 2017/02/13
 */
int TRX_transmit16Frame(bufType_n bufType, uint8_t *send)
{
	RFmodul = GET_device();
	size_t length = GET_apFrameLength();
	if ( 5 >= length || PACKAGE_SIZE-1 < length ) return 0;

	int	pos		= 0;

	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write dest. address
	 * - write destination PANID
	 * - write src. address
	 */
								      *(send)    = 0x01;		// send data to a device
	if ( TRUE == RFmodul->secCMD_ee ) *(send)   |= 0x08;		// security active
								      *(send+1) |= 0x08;		// dest. address type (16bit)

	 READ_deBufferData_atReadPosition(bufType, workArray, length-2);

	/*
	 * dest. address
	 */
	for (pos = 5; 5+2 > pos; pos++)
	{
		*(send+pos) = workArray[6-pos];
	}

	/*
	 * handle of API Option field and  set PAN ID
	 */
	if ( ( ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm ||\
	       ACK_NO_MAXSTREAM   == RFmodul->netCMD_mm ) &&\
		   0x1 != workArray[2] )							  // frame option "disable ACK"
	{
		*(send) |= 0x20; // ACK on
	}

	if ( 0x4 == workArray[2] )								  // frame option "PAN ID id Broadcast PAN ID
	{
		*(send+3) = 0xFF;
		*(send+4) = 0xFF;									  // source PAN_ID

		memcpy( send+7, &RFmodul->netCMD_id, 2 );			  // destination PAN_ID

		pos += 2;
	}
	else
	{
		*(send) |= 0x40;									  // PAN Compression on
		memcpy( send+3, &RFmodul->netCMD_id, 2);			  // destination PAN_ID = source PAN_ID
	}


	/*
	 * src. address
	 */
	if ( 0xFFFE != RFmodul->netCMD_my )
	{
		memcpy( send+pos, &RFmodul->netCMD_my, 2 );			   // src. short address
		*(send+1) |= 0x80;									   // MAC header second byte
		pos += 2;
	}
	else
	{
		memcpy( send+pos,   &RFmodul->netCMD_sl, 4 );          // src. ext. addr.
		memcpy( send+pos+4, &RFmodul->netCMD_sh, 4 );
		*(send+1) |= 0xC0;									   // MAC header second byte
		pos += 8;
	}

	if ( ACK_WITH_MAXSTREAM    == RFmodul->netCMD_mm ||\
	     NO_ACK_WITH_MAXSTREAM == RFmodul->netCMD_mm )
	{
		*(send+pos)   = (uint8_t) (*(send+2) + (RFmodul->netCMD_sl & 0xFF) );	// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x00;													// this byte will be used as command, 0x04 AT remote command
		pos += 2;
	}

	/*
	 * data
	 */
	if ( PACKAGE_SIZE-1 < pos+length-5 ) return 0;
	memcpy( send+pos, &workArray[3], length-5 );

	return pos+length-5;
}