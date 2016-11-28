/*
 * apiframe.c
 *
 * Created: 25.11.2016 14:37:35
 *  Author: TOE
 */ 
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <avr/interrupt.h>

#include "../header/rfmodul.h"
#include "../atlocal.h"
#include "../header/apiframe.h"
#include "../header/circularBuffer.h"			// buffer
#include "../../ATuracoli/stackrelated.h"		// UART_print(f)

// === c-File structs =====================================
struct api_f 
{
	ATERROR  ret;
	uint8_t	 delimiter;
	uint16_t length;
	uint8_t  type;
	uint8_t  id;
	
};

// === Prototypes =========================================
static ATERROR API_0x08_atLocal(struct api_f *frame);
static ATERROR API_0x09_atLocal_queue(struct api_f *frame);

// === Functions ==========================================
/*
 *
 *
 *
 * Returns:
 *     final position in array
 *
 * last modified: 2016/11/28
 */
ATERROR API_frameHandle_uart(int *len)
{
	uint8_t  outchar[2]		= 0;
	struct api_f frame = {0,0,0,0,0};
	
	// Start delimiter	1 byte
	UART_print("Start delimiter\r");
	
	cli(); BufferOut( &UART_deBuf, outchar[0] ); sei();
	cli(); BufferOut( &UART_deBuf, outchar[1] ); sei();
	sprintf((char*)&frame.delimiter,"%c", (uint8_t) strtoul( (const char*) outchar, NULL, 16) );
	if ( frame.delimiter != STD_DELIMITER ) return API_NOT_AVAILABLE;
	
	UART_printf("%"PRIX8"\r", STD_DELIMITER );
	
	// length	2 byte
	cli(); BufferOut( &UART_deBuf, outchar[0] ); sei();
	cli(); BufferOut( &UART_deBuf, outchar[1] ); sei();
	sprintf((char*) outchar[0],"%c", (uint8_t) strtoul( (const char*) outchar, NULL, 16) );
		frame.length = (uint16_t) outchar[0] << 2;
		
	cli(); BufferOut( &UART_deBuf, outchar[0] ); sei();
	cli(); BufferOut( &UART_deBuf, outchar[1] ); sei();
	sprintf((char*) outchar[0],"%c", (uint8_t) strtoul( (const char*) outchar, NULL, 16) );
		frame.length = (uint16_t) outchar[0] & 0xFF;

	UART_print("Length\r"); 
	UART_printf("%"PRIX8" %"PRIX8"\r", (uint8_t) frame.length >> 2, frame.length & 0xFF );	
	
	UART_print("Frame type\r"); // frame type	1 byte
	cli(); BufferOut( &UART_deBuf, frame.type ); sei();
	switch ( frame.type )
	{
		case AT_COMMAND    : 
			UART_print("08 (AT Command)\r");
			frame.ret = API_0x08_atLocal(&frame);
		break;
		
		case AT_COMMAND_Q  : 
			UART_printf("09 (AT Command Queue)\r");
			frame.ret = API_0x09_atLocal_queue(&frame);
		break;
		
		case REMOTE_AT_CMD : 
			UART_print("17 (AT Remote Command)\r");
			frame.ret = TRX_atRemoteFrame(); 
		break;
		
		default : break;
	}
	
	
	
	// frame spez. handling 
	/*for (uint16_t i = 0x4; i < frame_length-1 && i < 0xFFFF; i++)
	{
		UART_printf("%"PRIX8" ", workbuf[i]);						   // parameter max 256 byte
	}
	UART_print("\r\r");
	
	UART_print("Checksum\r")
	UART_printf("%"PRIX8"\r\r", workbuf[frame_length-1] );				// checksum		1 byte
*/
	
	/*
	 * create the frame & calc checksum
	 * 0xFF - (API type + frame ID + target address + options + message) = checksum
	 */
/*
	for (int i = 0; i < frame.length; i++)
	{
		workbuf[frame.length-1] += workbuf[i];
	}
	workbuf[frame.length-1] = 0xFF - workbuf[frame.length-1];*/
	
	return OP_SUCCESS;
}

/*
 *
 *
 *
 * Returns:
 *		OP_SUCCESS
 *		INVALID_COMMAND
 *		INVALID_PARAMETER
 *
 * last modified: 2016/11/28
 */
static ATERROR API_0x08_atLocal(api_f *frame)
{
	// frame id		1 byte
	UART_print("Frame ID\r");
	cli(); BufferOut( &UART_deBuf, frame->id ); sei();
	UART_printf("%"PRIX8"\r\r", frame->id );
	
	// AT command 2 bytes
	uint8_t CmdString[5] = {'A', 'T', 0x0, 0x0, 0x0};
	cli(); BufferOut( &UART_deBuf, &CmdString[2] ); sei();
	cli(); BufferOut( &UART_deBuf, &CmdString[3] ); sei();
	
	CMD *workPointer = (CMD*) pStdCmdTable;
	for (int i = 0; i < command_count ; i++, workPointer++)
	{
		if( strncmp( (const char*) CmdString, workPointer->name, 4 ) == 0 )
		{
			return workPointer;
		}
	}
	
	// handel more content in buff
	// Handel rx or w
	
	UART_print("AT Command")
	UART_printf("%"PRIX8" %"PRIX8"\r\r", *(frame+2), *(frame+3) );		// AT CMD		2 byte

}

/*
 *
 *
 *
 * Returns:
 *		OP_SUCCESS
 *		INVALID_COMMAND
 *		INVALID_PARAMETER 
 *
 * last modified: 2016/11/28
 */
static ATERROR API_0x09_atLocal_queue(uint8_t *frame)
{
	UART_print("Frame ID\r");
	UART_printf("%"PRIX8"\r\r", *(frame+1) );							// frame id		1 byte
	
	UART_print("AT Command")
	UART_printf("%"PRIX8" %"PRIX8"\r\r", *(frame+2), *(frame+3) );		// AT CMD		2 byte
}

