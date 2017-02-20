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
static inline void swap(uint8_t *array, size_t len);
static inline void swap_u32(uint32_t *inVal);

// === functions ==========================================
/*
 * Validation and write function
 * if AT command
 *    - if length greater than 8 return invalid parameter
 *    - else convert string to unsigned long
 *    - if end pointer is not equal to '\0' return invalid parameter
 *
 * if API command
 *	  - if length greater than 4 return invalid parameter
 *	  - else swap buffer content and copy the received values in to 32 bit variable
 *
 * compare variable with min and max values
 *	  - if value is OK, store it into RFmodul struct
 *	  - else return invalid parameter
 *
 * Received:
 *		size_t			string length
 *		uint8_t  		pointer to workArray which includes the data
 *		CMD				pointer to command in command table
 *		device_mode		type of command AT or API
 *
 * Returns:
 *     OP_SUCCESS			on success
 *	   INVALID_PARAMETER	if parameter is not valid
 *
 * last modified: 2017/02/10
 */
at_status_t max_u32val( size_t len, uint8_t *workArray, const CMD *cmd, const device_mode devMode )
{
	uint32_t val = 0x0;

	if( TRANSPARENT_MODE == devMode )
	{
		if ( 9 < len ) return INVALID_PARAMETER;

		char *endptr;
		val = strtoul( (const char*) workArray, &endptr, 16);
		if ( *endptr != workArray[len-1]) return INVALID_PARAMETER;
	}
	else
	{
		if ( 4 < len ) return INVALID_PARAMETER;

		if ( 1 < len ) swap(workArray, len);
		memcpy( &val, workArray, len);
	}

	if ( val >= cmd->min && val <= cmd->max )
	{
		cmd->mySet( &val, cmd->cmdSize);
		return OP_SUCCESS;
	}
	else return INVALID_PARAMETER;
}



/*
 * Validation and write function
 * if AT command
 *    - if length greater than 16 return invalid parameter
 *	  - if length greater than 8 move len-8 bytes one position onwards and
 *      set a space at position 8 in array
 *    - else convert string to unsigned long
 *    - if end pointer is not equal to '\0' return invalid parameter
 *
 * if API command
 *	  - if length greater than 8 return invalid parameter
 *	  - else swap buffer content and copy the received values in to 32 bit variable
 *
 * store value into RFmodul struct
 *
 * Received:
 *		size_t			string length
 *		uint8_t  		pointer to workArray which includes the data
 *		CMD				pointer to command in command table
 *		device_mode		type of command AT or API
 *
 * Returns:
 *     OP_SUCCESS			on success
 *	   INVALID_PARAMETER	if parameter is not valid
 *
 * last modified: 2017/02/10
 */
at_status_t max_u64val( size_t len, uint8_t *workArray, const CMD *cmd, const device_mode devMode )
{
	uint64_t val;

	if( TRANSPARENT_MODE == devMode )
	{
		if ( 16 < len ) return INVALID_PARAMETER;

		char *endptr;
		uint8_t shift = 0x0;

		if ( 8 < len )
		{
			memmove( &workArray[9], &workArray[8], len-8 );
			workArray[8] = 0x20;
			shift = 4 * (len-8);
		}

		val  = (uint64_t) strtoul( (const char*) workArray, &endptr, 16) << shift;
		if ( *endptr != workArray[len] && 8 >= len ) return INVALID_PARAMETER;

		val |= strtoul( (const char*) endptr, &endptr, 16);
		if ( *endptr != workArray[len] && 8 < len ) return INVALID_PARAMETER;
	}
	else
	{
		if ( 8 < len ) return INVALID_PARAMETER;
		swap(workArray, len);
		memcpy( &val, workArray, len);
	}

	cmd->mySet( &val, cmd->cmdSize);
	return OP_SUCCESS;
}



/*
 * node identifier string command function
 * if buffer content smaller than 20 characters
 * store value into RFmodul struct else return invalid parameter
 *
 * Received:
 *		size_t			string length
 *		uint8_t  		pointer to workArray which includes the data
 *		CMD				pointer to command in command table
 *		device_mode		type of command AT or API
 *
 * Returns:
 *     OP_SUCCESS			on success
 *	   INVALID_PARAMETER	if parameter is not valid
 *
 * last modified: 2017/02/10
 */
at_status_t node_identifier( size_t len, const uint8_t *workArray, const CMD *cmd, const device_mode devMode )
{
	if ( len <= cmd->max )
	{
		cmd->mySet( (uint8_t*) workArray, len);

		if ( TRANSPARENT_MODE == devMode ) UART_print_status(OP_SUCCESS);
		return OP_SUCCESS;
	}
	else
	{
		return INVALID_PARAMETER;
	}
}



/*
 * key validation for AES encryption
 * TODO
 *
 * store value into RFmodul struct else return invalid parameter
 *
 * Received:
 *		size_t			string length
 *		uint8_t  		pointer to workArray which includes the data
 *		CMD				pointer to command in command table
 *		device_mode		type of command AT or API
 *
 * Returns:
 *     OP_SUCCESS			on success
 *	   INVALID_PARAMETER	if parameter is not valid
 *
 * last modified: 20--/--/--
 */
at_status_t ky_validator( size_t len, const uint8_t *workArray, const CMD *cmd, const device_mode devMode )
{
	if ( 16 >= len )
	{
		//if ( TRANSPARENT_MODE == devMode ) UART_print_status(OP_SUCCESS);
		return OP_SUCCESS;
	}
	else
	{
		return INVALID_PARAMETER;
	}
}



/*
 * swap helper function swap
 *
 * Received:
 *		uint8_t		pointer to array which should swapped
 *		size_t		size of array
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/02/10
 */
static inline void swap(uint8_t *array, size_t len)
{
	for(size_t i = 0; i < len/2; i++)
	{
		array[i] ^= array[len-1-i];
		array[len-1-i] ^= array[i];
		array[i] ^= array[len-1-i];
	}
}