/*
 * helper.c
 *
 * Created: 10.11.2016 13:14:32
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>
#include <string.h>

#include "../header/rfmodul.h"
#include "../header/cmd.h"
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
at_status_t max_u32val( size_t len, const uint8_t *workArray, const CMD *cmd, const device_mode devMode )
{
	uint32_t val = 0x0;
	char *endptr;

	if( devMode == GET_serintCMD_ap() )
	{
		val = strtoul( (const char*) workArray, &endptr, 16);
		if ( *endptr != workArray[len-1]) return INVALID_PARAMETER;
	}
	else
	{
		memcpy( &val, workArray, len);
		swap_u32(&val);
	}

	if ( val >= cmd->min && val <= cmd->max )
	{
		cmd->mySet( &val, cmd->cmdSize);
		return OP_SUCCESS;
	}
	else return INVALID_PARAMETER;
}


at_status_t max_u64val( size_t len, const uint8_t *workArray, const CMD *cmd, const device_mode devMode )
{
/*	uint8_t  workArray_A[9] = {0x0};
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
		cmd->mySet( &val, cmd->cmdSize);

		if ( devMode == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
		return OP_SUCCESS;
	}
	else return INVALID_PARAMETER;*/
}


/*
 * special handle if
 * - network identifier string command
 * - buffer content <= 20 characters
 */
at_status_t node_identifier( size_t len, const uint8_t *workArray, const CMD *cmd, const device_mode devMode )
{
	if ( len <= cmd->max )
	{
		cmd->mySet( workArray, len);

		if ( devMode == GET_serintCMD_ap() ) UART_print_status(OP_SUCCESS);
		return OP_SUCCESS;
	}
	else
	{
		return INVALID_PARAMETER;
	}
}

at_status_t ky_validator( bufType_n bufType, size_t len, CMD *cmd, const device_t devMode )
{
	/* TODO */
	if (FALSE)
	{
		if ( devMode == GET_serintCMD_ap() ) UART_print_status(OP_SUCCESS);
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