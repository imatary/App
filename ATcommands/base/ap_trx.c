/*
 * ap_trx.c
 *
 * Created: 19.01.2017 09:48:06
 *  Author: TOE
 */ 
// === functions (transreceiver) ==========================
/*
 * TRX_createAPframe
 * creates an AP frame and print it to UART
 *
 * Received:
 *		uint8_t		length of the received frame
 *		uint8_t		position where the data payload starts
 *		uint8_t		length of source address
 *		uint8_t		MAC options
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/12/15
 */
void TRX_createAPframe( uint8_t flen, uint8_t dataStart, uint8_t srcAddrLen, uint8_t option )
{
	/* AES decryption not supported -- if security decryption functions are added delete this upper part */
	if ( option == 0x48 || option == 0x08 ) return;
	/**********************************************************************/
	
	uint8_t outchar, DigiHeader = 0, srcAddrAndOption[10] = {0x0}; // size = max 8 bytes for src address and 2 bytes for Digi data header
	
	/*
	 * - delete  dest. PANID, dest. addr., [src. PANID] (max 12 bytes)
	 * - read src addr (2 or 8 bytes)
	 * - read Digi data header 2 Bytes
	 * - check type of data header and handle data
	 */
	if ( RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x3 ) DigiHeader = 2; // if Digi header included add temporary two bytes for data header 

	for (uint8_t counter = 0, pos = 0; pos < srcAddrLen + DigiHeader && counter < flen; counter++ )
	{
		cli(); BufferOut( &RX_deBuf, &outchar ); sei();
		if ( counter >= dataStart-srcAddrLen )
		{
			srcAddrAndOption[pos] = outchar;
			pos++;
		}
	}
	
	switch ( srcAddrAndOption[srcAddrLen+1] )
	{
		case 0x04 : AP_0x17_atRemoteFrame( (uint16_t) flen-dataStart-4 , srcAddrAndOption, srcAddrLen );  break;	// TX response only
		case 0x05 : AP_0x97_atRemote_response( (uint16_t) flen-dataStart-4 );							  break;	// UART response only
		default   : AP_0x80_0x81_rxReceive( (uint16_t) flen-dataStart-4 , srcAddrAndOption, srcAddrLen ); break;	// UART response only
	}					
	
}

/* 
 * AT Remote Command (remote request)
 * prepared frame for a remote AT command to control another device
 *
 * Received:
 *		uint8_t		pointer to send array
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/12/15
 */
int TRX_0x17_atRemoteFrame(uint8_t *send)
{
	int			pos		= 0;
	at_status_t ret		= 0;
	uint8_t		tmp[11] = {0x0};
	bool_t		flag	= FALSE;
	 
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */	
																 *(send) |= 0x01; // send data to _one_ device
	if ( RFmodul.secCMD_ee == TRUE )							 *(send) |= 0x08; // security active
	if ( RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x2 )  *(send) |= 0x20; // ACK on
																 *(send) |= 0x40; // PAN Compression on
	
	*(send+ 3) = (uint8_t) (RFmodul.netCMD_id & 0xff);
	*(send+ 4) = (uint8_t) (RFmodul.netCMD_id >>  8);							// destination PAN_ID
	pos = 5;
	
	/*
	 * dest. address
	 */;
	for (char i = 0; i < 11; i++ )
	{
		cli(); ret = BufferOut( &UART_deBuf, &tmp[i] ); sei();
		if ( ret == BUFFER_OUT_FAIL ) return 0;
		frame.crc += tmp[i];
	}
	
	
	if ( tmp[8] == 0xFF && tmp[9] == 0xFE )										// if 16bit address is deactivate 
	{
		*(send+ 5) = tmp[7];
		*(send+ 6) = tmp[6];
		*(send+ 7) = tmp[5];
		*(send+ 8) = tmp[4];													// destination ext. addr. low

		*(send+ 9) = tmp[3];
		*(send+10) = tmp[2];
		*(send+11) = tmp[1];
		*(send+12) = tmp[0];													// destination ext. addr. high
		
		*(send+1) |= 0x0C;														// MAC header second byte
		pos = 13;
	}
	else if ( tmp[0] == 0x00 &&\
	          tmp[1] == 0x00 &&\
			  tmp[2] == 0x00 &&\
			  tmp[3] == 0x00 &&\
			  tmp[4] == 0x00 &&\
			  tmp[5] == 0x00 &&\
			  tmp[6] == 0xFF &&\
			  tmp[7] == 0xFF ||
			  tmp[8] == 0xFF &&\
			  tmp[9] == 0xFF)													// if command is a broadcast message
	{
		 *(send+5) = 0xFF;
		 *(send+6) = 0xFF;
		 
		 *(send+1) |= 0x08;														// MAC header second byte
		 pos = 7;
	}
	else																		// if message is send with 16bit address
	{
		*(send+5) = tmp[9];
		*(send+6) = tmp[8];
		
		*(send+1) |= 0x08;														// MAC header second byte
		pos = 7;
	}
	
	/*
	 * src. address
	 */
	if ( RFmodul.netCMD_my > 0x0 )
	{
		*(send+pos)   = (uint8_t) (RFmodul.netCMD_my & 0xff);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_my >> 8);						// src. short address
		
		*(send+1) |= 0x80;														// MAC header second byte
		pos += 2;
	}
	else
	{
		*(send+pos)   = (uint8_t) (RFmodul.netCMD_sl >>  0);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_sl >>  8);
		*(send+pos+2) = (uint8_t) (RFmodul.netCMD_sl >> 16);
		*(send+pos+3) = (uint8_t) (RFmodul.netCMD_sl >> 24);					// src. ext. addr. low

		*(send+pos+4) = (uint8_t) (RFmodul.netCMD_sh >>  0);
		*(send+pos+5) = (uint8_t) (RFmodul.netCMD_sh >>  8);
		*(send+pos+6) = (uint8_t) (RFmodul.netCMD_sh >> 16);
		*(send+pos+7) = (uint8_t) (RFmodul.netCMD_sh >> 24);					// src. ext. addr. high
		
		*(send+1) |= 0xC0;														// MAC header second byte
		pos += 8;
	}
	
	if ( RFmodul.netCMD_mm != 0x1 || RFmodul.netCMD_mm != 0x2 )
	{
		*(send+pos)   = (uint8_t) ( *(send+2) + (RFmodul.netCMD_sl & 0xFF) );	// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x04;													// this byte will be used as command, 0x04 AT remote command
		pos += 2;
	}
		
	*(send+pos) = frame.id;
	pos += 1;
	
	/*
	 * data
	 */
	for ( char i = 0; i<11; i++, pos++ )
	{
		*(send+pos) = tmp[i];
	}
	
	frame.length -= 12;
	do
	{
		cli(); ret = BufferOut( &UART_deBuf, send+pos ); sei();
		if ( ret == BUFFER_OUT_FAIL ) break;
		
		frame.crc += *(send+pos);
		pos++;
		
	} while ( frame.length-- || pos < PACKAGE_SIZE-1 );

	if ( AP_compareCRC() == FALSE ) return 0;
	
	return pos-1;
}

/*
 * AT Remote Command (response)
 * send result information of received and executed 0x17 Remote Command 
 *
 * Received:
 *		uint8_t		pointer to send array
 *		uint8_t		pointer to source address array
 *		uint8_t		source address length
 *
 * Returns:
 *		final position in array
 *
 * last modified: 2017/01/05
 */
int TRX_0x97_atRemote_response(uint8_t *send, uint8_t *srcAddr, uint8_t srcAddrLen)
{
	int	pos	= 0;
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write destination PANID
	 * - write dest. address
	 * - write src. address
	 */
																*(send) |= 0x01; // send data to _one_ device
	if ( RFmodul.secCMD_ee == TRUE )							*(send) |= 0x08; // security active
	if ( RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x2 ) *(send) |= 0x20; // ACK on
																*(send) |= 0x40; // PAN Compression on
	
	*(send+ 3) = (uint8_t) (RFmodul.netCMD_id & 0xff);
	*(send+ 4) = (uint8_t) (RFmodul.netCMD_id >>  8);							// destination PAN_ID
	pos = 5;
	/*
	 * dest. addr.
	 */
	for ( char i = 0; i < srcAddrLen; i++, pos++ )
	{
		*(send+pos) = *(srcAddr+i);
	}
	*(send+1) |= (srcAddrLen == 8)? 0xC : 0x8;

	/*
	 * src. address
	 */
	if ( RFmodul.netCMD_my > 0x0 )
	{
		*(send+pos)	  = (uint8_t) (RFmodul.netCMD_my & 0xff);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_my >> 8);						// src. short address
		
		*(send+1) |= 0x80;														// MAC header second byte
		pos += 2;
	}
	else
	{
		*(send+pos)   = (uint8_t) (RFmodul.netCMD_sl >>  0);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_sl >>  8);
		*(send+pos+2) = (uint8_t) (RFmodul.netCMD_sl >> 16);
		*(send+pos+3) = (uint8_t) (RFmodul.netCMD_sl >> 24);					// src. ext. addr. low
		
		*(send+pos+4) = (uint8_t) (RFmodul.netCMD_sh >>  0);
		*(send+pos+5) = (uint8_t) (RFmodul.netCMD_sh >>  8);
		*(send+pos+6) = (uint8_t) (RFmodul.netCMD_sh >> 16);
		*(send+pos+7) = (uint8_t) (RFmodul.netCMD_sh >> 24);					// src. ext. addr. high
		
		*(send+1) |= 0xC0;														// MAC header second byte
		pos += 8;
	}
	
	if ( RFmodul.netCMD_mm != 0x1 || RFmodul.netCMD_mm != 0x2 )
	{
		*(send+pos) = (uint8_t) ( *(send+2) + (RFmodul.netCMD_sl & 0xFF) );		// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x05;													// this byte will be used as command, 0x05 AT remote command response
		pos += 2;
	}
	
	/*
	 * data
	 */
	*(send+pos) = frame.id;														// AP Frame ID
	
	*(send+pos+1) = (uint8_t) (RFmodul.netCMD_sh >> 24);
	*(send+pos+2) = (uint8_t) (RFmodul.netCMD_sh >> 16);
	*(send+pos+3) = (uint8_t) (RFmodul.netCMD_sh >>  8);
	*(send+pos+4) = (uint8_t) (RFmodul.netCMD_sh >>  0);						// src. ext. addr. high
	
	*(send+pos+5) = (uint8_t) (RFmodul.netCMD_sl >> 24);
	*(send+pos+6) = (uint8_t) (RFmodul.netCMD_sl >> 16);
	*(send+pos+7) = (uint8_t) (RFmodul.netCMD_sl >>  8);
	*(send+pos+8) = (uint8_t) (RFmodul.netCMD_sl >>  0);						// src. ext. addr. low
	
	*(send+pos+9)  = (uint8_t) (RFmodul.netCMD_my >> 8);
	*(send+pos+10) = (uint8_t) (RFmodul.netCMD_my & 0xFF);						// src. short address
	
	*(send+pos+11) = frame.cmd[0];
	*(send+pos+12) = frame.cmd[1];												// command
	*(send+pos+13) = frame.ret * (-1);											// command status
	
	pos +=14;
	if ( READ == frame.rwx )
	{
		for ( char i = 0; i < frame.length; i++, pos++ )						// content [optional]
		{
			*(send+pos) = frame.msg[i];
		}
	}
	
	
	return pos;
}

/*
 * Frame will be send as text message with a 64bit address
 *
 * Received:
 *		uint8_t		pointer to send array
 *
 * Returns:
 *		final position in array
 *
 * last modified: 2017/01/09
 */
int TRX_0x00_transmit64Frame(uint8_t *send)
{
	int			pos		= 0;
	at_status_t ret		= 0;
	uint8_t		tmp[8]  = {0x0};
	
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write dest. address
	 * - write destination PANID
	 * - write src. address
	 */	
									 *(send) |= 0x01;							// send data to a device
	if ( RFmodul.secCMD_ee == TRUE ) *(send) |= 0x08;							// security active

	pos = 5;	
	/*
	 * dest. address
	 */
	for (char i = 0; i < 8; i++)
	{
		cli(); ret = BufferOut( &UART_deBuf, &tmp[i] ); sei();
		if ( ret == BUFFER_OUT_FAIL ) return 0;
		frame.crc += tmp[i];
	}
	
	if ( tmp[0] == 0x00 &&\
		 tmp[1] == 0x00 &&\
		 tmp[2] == 0x00 &&\
		 tmp[3] == 0x00 &&\
		 tmp[4] == 0x00 &&\
		 tmp[5] == 0x00 &&\
		 tmp[6] == 0xFF &&\
		 tmp[7] == 0xFF )														// if command is a broadcast message
	{
		*(send+5) = 0xFF;
		*(send+6) = 0xFF;
		
		*(send+1) |= 0x08;														// MAC header second byte
		pos += 2;
	}
	else
	{
		for (char i = 0; i < 8; i++)
		{
			*(send+pos+i) = tmp[7-i];
		}
		*(send+1) |= 0x0C;														// MAC header second byte
		pos += 8;
	}

	/*
	 * handle of API Option field and  set PAN ID
	 */
	cli(); ret = BufferOut( &UART_deBuf, &tmp[0] ); sei();
	if ( ret == BUFFER_OUT_FAIL ) return 0;
	frame.crc += tmp[0];
	
	if ( (RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x2) && tmp[0] != 0x1 )  
	{
		*(send) |= 0x20;														// ACK on
	}
	
	if ( tmp[0] == 0x4 )
	{
		*(send+3) = 0xFF;
		*(send+4) = 0xFF;														// source PAN_ID
		
		*(send+pos)   = RFmodul.netCMD_id & 0xff;
		*(send+pos+1) = RFmodul.netCMD_id >> 8;									// destination PAN_ID
		
		pos += 2;
	}
	else
	{
		*(send) |= 0x40;														// PAN Compression on

		*(send+ 3) = RFmodul.netCMD_id & 0xff;
		*(send+ 4) = RFmodul.netCMD_id >> 8;									// destination PAN_ID = source PAN_ID
	}
		
	/*
	 * src. address
	 */
	if ( RFmodul.netCMD_my != 0xFFFE )
	{
		*(send+pos)   = RFmodul.netCMD_my & 0xff;
		*(send+pos+1) = RFmodul.netCMD_my >> 8;									// src. short address
		
		*(send+1) |= 0x80;														// MAC header second byte
		pos += 2;
	}
	else
	{
		*(send+pos)   = (uint8_t) (RFmodul.netCMD_sl >>  0);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_sl >>  8);
		*(send+pos+2) = (uint8_t) (RFmodul.netCMD_sl >> 16);
		*(send+pos+3) = (uint8_t) (RFmodul.netCMD_sl >> 24);					// src. ext. addr. low
		
		*(send+pos+4) = (uint8_t) (RFmodul.netCMD_sh >>  0);
		*(send+pos+5) = (uint8_t) (RFmodul.netCMD_sh >>  8);
		*(send+pos+6) = (uint8_t) (RFmodul.netCMD_sh >> 16);
		*(send+pos+7) = (uint8_t) (RFmodul.netCMD_sh >> 24);					// src. ext. addr. high
		
		*(send+1) |= 0xC0;														// MAC header second byte
		pos += 8;
	}
	
	if ( RFmodul.netCMD_mm != 0x1 || RFmodul.netCMD_mm != 0x2 )
	{
		*(send+pos)   = (uint8_t) ( *(send+2) + (RFmodul.netCMD_sl & 0xFF));	// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x00;													// this byte will be used as command, 0x04 AT remote command
		pos += 2;
	}
	
	/*
	 * data
	 */
	frame.length -= 11;
	do
	{
		cli(); ret = BufferOut( &UART_deBuf, send+pos ); sei();
		if ( ret == BUFFER_OUT_FAIL ) break;
		
		frame.crc += *(send+pos);
		pos++;
		
	} while ( frame.length-- || pos < PACKAGE_SIZE-1 );

	if ( AP_compareCRC() == FALSE ) return 0;
	
	return pos-1;
}

/*
 * Frame will be send as text message with a 16bit address
 *
 * Received:
 *		uint8_t		pointer to send array
 *
 * Returns:
 *		final position in array
 *
 * last modified: 2017/01/09
 */
int TRX_0x01_transmit16Frame(uint8_t *send)
{
	int			pos		= 0;
	at_status_t ret		= 0;
	uint8_t		tmp		= 0x0;
	
	/* Step 1: prepare packed
	 * - prepare MAC header, first byte
	 * - write dest. address
	 * - write destination PANID
	 * - write src. address
	 */	
									 *(send)   |= 0x01;							// send data to a device
	if ( RFmodul.secCMD_ee == TRUE ) *(send)   |= 0x08;							// security active
									 *(send+1) |= 0x08;							// dest. address type (16bit)
	pos = 5;	
	/*
	 * dest. address
	 */
	cli(); ret = BufferOut( &UART_deBuf, send+6 ); sei();
	cli(); ret = BufferOut( &UART_deBuf, send+5 ); sei();
	if ( ret == BUFFER_OUT_FAIL ) return 0;
	frame.crc += *(send+5) + *(send+6);
	
	/*
	 * handle of API Option field and  set PAN ID
	 */
	cli(); ret = BufferOut( &UART_deBuf, &tmp ); sei();
	if ( ret == BUFFER_OUT_FAIL ) return 0;
	frame.crc += tmp;
	
	if ( (RFmodul.netCMD_mm == 0x0 || RFmodul.netCMD_mm == 0x2) && tmp != 0x1 )  
	{
		*(send) |= 0x20; // ACK on
	}
	
	if ( tmp == 0x4 )
	{
		*(send+3) = 0xFF;
		*(send+4) = 0xFF;														// source PAN_ID
		
		*(send+ 7) = (uint8_t) (RFmodul.netCMD_id & 0xff);
		*(send+ 8) = (uint8_t) (RFmodul.netCMD_id >>  8);						// destination PAN_ID
		
		pos += 4;
	}
	else
	{
		*(send) |= 0x40; // PAN Compression on

		*(send+ 3) = (uint8_t) (RFmodul.netCMD_id & 0xff);
		*(send+ 4) = (uint8_t) (RFmodul.netCMD_id >>  8);						// destination PAN_ID = source PAN_ID
		
		pos += 2;
	}
		
	
	/*
	 * src. address
	 */
	if ( RFmodul.netCMD_my != 0xFFFE )
	{
		*(send+pos)   = (uint8_t) (RFmodul.netCMD_my & 0xff);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_my >> 8);						// src. short address
		
		*(send+1) |= 0x80;														// MAC header second byte
		pos += 2;
	}
	else
	{
		*(send+pos)   = (uint8_t) (RFmodul.netCMD_sl >>  0);
		*(send+pos+1) = (uint8_t) (RFmodul.netCMD_sl >>  8);
		*(send+pos+2) = (uint8_t) (RFmodul.netCMD_sl >> 16);
		*(send+pos+3) = (uint8_t) (RFmodul.netCMD_sl >> 24);					// src. ext. addr. low

		*(send+pos+4) = (uint8_t) (RFmodul.netCMD_sh >>  0);
		*(send+pos+5) = (uint8_t) (RFmodul.netCMD_sh >>  8);
		*(send+pos+6) = (uint8_t) (RFmodul.netCMD_sh >> 16);
		*(send+pos+7) = (uint8_t) (RFmodul.netCMD_sh >> 24);					// src. ext. addr. high
		
		*(send+1) |= 0xC0;														// MAC header second byte
		pos += 8;
	}
	
	if ( RFmodul.netCMD_mm != 0x1 || RFmodul.netCMD_mm != 0x2 )
	{
		*(send+pos)   = (uint8_t) (*(send+2) + (RFmodul.netCMD_sl & 0xFF) );	// second counter, distance to frame counter is last byte of src extended addr.
		*(send+pos+1) = 0x00;													// this byte will be used as command, 0x04 AT remote command
		pos += 2;
	}
	
	/*
	 * data
	 */
	frame.length -= 5;
	do
	{
		cli(); ret = BufferOut( &UART_deBuf, send+pos ); sei();
		if ( ret == BUFFER_OUT_FAIL ) break;
		
		frame.crc += *(send+pos);
		pos++;
		
	} while ( frame.length-- || pos < PACKAGE_SIZE-1 );
	
	if ( AP_compareCRC() == FALSE ) return 0;
	
	return pos-1;
}