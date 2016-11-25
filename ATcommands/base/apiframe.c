/*
 * apiframe.c
 *
 * Created: 25.11.2016 14:37:35
 *  Author: TOE
 */ 

#include "../header/apiframe.h"

static uint8_t API_calc_crc(uint8_t *frame);

ATERROR API_frameHandle(void)
{
	return API_NOT_AVAILABLE;
	uint16_t frame_length = 0;
	
	{
		uint8_t *outchar = NULL;
		cli(); BufferOut( &UART_deBuf, outchar ); sei();
		if ( *outchar != STD_DELIMITER ) return API_NOT_AVAILABLE;
		
		cli(); BufferOut( &UART_deBuf, outchar ); sei();
		frame_length = (uint16_t) *outchar << 2;
		cli(); BufferOut( &UART_deBuf, outchar ); sei();
		frame_length = (uint16_t) *outchar;
	}
	
	uint8_t workbuf[frame_length] = {0};
		
	for (int i = 0; i < frame_length; i++)
	{
		cli(); BufferOut( &UART_deBuf, workbuf[i] ) sei();
	}
	
	if( workbuf[frame_length-1] == API_calc_crc( workbuf ) ) return API_FRAMESIZE_ERROR;
	
	switch ( &workbuf[0] )
	{
		case AT_COMMAND : break;
		case REMOTE_AT_CMD : TRX_atRemoteFrame(workbuf); break;
		default : break;
	}
	
	return OP_SUCCESS;
}