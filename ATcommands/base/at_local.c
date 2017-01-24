/*
 * at_local.c
 *
 * Created: 10.11.2016 13:14:32
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>
#include <string.h>

#include "../header/rfmodul.h"
#include "../header/enum_status.h"
#include "../header/circularBuffer.h"
#include "../../ATuracoli/stackrelated.h"

// === prototypes =========================================
static inline void swap_u32(uint32_t *inVal);
static inline void swap_u64(uint64_t *inVal);

// === functions ==========================================
/*
 * Validation an write function
 * - received the buffer content and converted content to uint32 hex values
 * if - the command size smaller or equal then unit of the tmp buffer 
 *    - the buffer value greater or equal than min value
 *    - the buffer value smaller or equal than max value
 * write to RFmodul struct
 * 
 * Received:
 *		bufType_n	number of buffer type
 *		size_t  	string length
 *		CMD			pointer to command in command table
 *		
 * Returns:
 *     OP_SUCCESS			on success
 *	   INVALID_PARAMETER	if parameter is not valid or error has occurred during transforming to hex
 *
 * last modified: 2016/12/02
 */
at_status_t max_u32val( bufType_n bufType, size_t len, CMD *cmd )
{
	uint8_t  workArray[9] = {0x0};
	uint32_t val = 0x0;
	char *endptr;
	
	GET_deBufferData_atReadPosition( bufType, workArray, len);
	
	if( TRANSPARENT_MODE == GET_serintCMD_ap() )
	{
		val = strtoul( (const char*) workArray, &endptr, 16);
		if ( *endptr != workArray[len-1]) return INVALID_PARAMETER;
	}
	else
	{
		memcpy( &val, workArray, len);
		if ( val & 0xFF != workArray[len-2] ) swap_u32(&val);
	}
	
	UART_printf("DEBUG val >> %"PRIX32"\r", val); // DEBUG
	UART_printf("DEBUG min >> %"PRIX32"\r", *cmd->min);
	UART_printf("DEBUG max >> %"PRIX32"\r", *cmd->max);
	
	if ( val >= *cmd->min && val <= *cmd->max )
	{
		cmd->set( &val, cmd->cmdSize);
		
		if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
		return OP_SUCCESS;
	}
	else return INVALID_PARAMETER;
}

at_status_t max_u64val( bufType_n bufType, size_t len, CMD *cmd )
{
	uint8_t  workArray_A[9] = {0x0}; 
	size_t len_A = (len > 9)? 8 : len;
	uint64_t val = 0;
	char *endptr;
	
	GET_deBufferData_atReadPosition( bufType, workArray_A, len_A);
	
	if( TRANSPARENT_MODE == GET_serintCMD_ap )
	{
		val = (uint64_t) strtoul( (const char*) workArray_A, &endptr, 16) << ( len > 9 )? 32 : 0;
		if ( *endptr != workArray_A[len-1]) return INVALID_PARAMETER;
		
		if ( len > 9 )
		{
			uint8_t workArray_B[9] = {0x0};
			uint16_t len_B = len-8;
				
			GET_deBufferData_atReadPosition( bufType, workArray_B, len_B);
			val |= strtoul( (const char*) workArray_B, &endptr, 16);
			if ( *endptr != workArray_B[len-1]) return INVALID_PARAMETER;
		}
	}
	else
	{
		memcpy( &val, workArray_A, len);
		if ( val & 0xFF != workArray_A[len-2] ); 
	}
	
	deBufferReadReset( bufType, '+', len);
	
	if ( val >= *cmd->min && val <= *cmd->max )
	{
		cmd->set( &val, len);
		
		if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
		return OP_SUCCESS;
	}
	else return INVALID_PARAMETER;
}


/* 
 * special handle if
 * - network identifier string command
 * - buffer content <= 20 characters
 */
at_status_t node_identifier( bufType_n bufType, size_t len, CMD *cmd )
{
	if ( len <= *cmd->max )
	{
		uint8_t workArray[21] = {0x0};
			
		GET_deBufferData_atReadPosition(bufType, workArray, len);
		cmd->set( workArray, len);
		
		if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) UART_print_status(OP_SUCCESS);
		return OP_SUCCESS;
	}
	else
	{
		return INVALID_PARAMETER;
	}
}

at_status_t ky_validator(bufType_n bufType, size_t len, CMD *cmd)
{
	/* TODO */
	if (FALSE)
	{
		if ( TRANSPARENT_MODE == GET_serintCMD_ap() ) UART_print_status(OP_SUCCESS);
		return OP_SUCCESS;
	}
	else
	{
		return INVALID_PARAMETER;
	}
}

/*
 * swap helper functions
 * 
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/23
 */

static inline void swap_u32(uint32_t *inVal)
{
	uint8_t *bytes = (uint8_t*)inVal;
	bytes[0] ^= bytes[3];
	bytes[3] ^= bytes[0];
	bytes[0] ^= bytes[3];
	bytes[1] ^= bytes[2];
	bytes[2] ^= bytes[1];
	bytes[1] ^= bytes[2];
}

static inline void swap_u64(uint64_t *inVal)
{
	uint8_t *bytes = (uint8_t*)inVal;

	for(char i = 0; i < 4; i++)
	{
		bytes[i]   ^= bytes[7-i];
		bytes[7-i] ^= bytes[i];
		bytes[i]   ^= bytes[7-i];
	}

}