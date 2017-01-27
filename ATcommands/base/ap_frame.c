/*
 * ap_frame.c
 *
 * Created: 25.01.2017 16:41:48
 *  Author: TOE
 */
// === includes ===========================================
#include <inttypes.h>		// uint8/16_t
#include <stddef.h>			// size_t

// === struct =============================================
struct api_f
{
	at_status_t ret;		// 1 Byte
	uint8_t		rwx;		// 1 Byte
	uint16_t	length;		// 2 Byte
	uint8_t		type;		// 1 Byte
	uint8_t		cmd[3];		// 3 Byte
	uint8_t		id;			// 1 Byte
	uint8_t		msg[256];	// 256 Byte
	/*
	 * create the frame & calc checksum
	 * 0xFF - (AP type + frame ID [+ target address] [+ options] + main content [+ parameter]) = checksum
	 *        |<---------------------------------- frame frame->bufLength ------------------->|
	 */
	uint8_t  crc;

}__attribute__((packed));

// === globals ============================================
static struct api_f frame  = {0,0,0,0,{0},0,{0},0};

// === prototypes ==========================================
static void SWAP_apFrameMsg(size_t length);

// === functions ===========================================
/*
 * Set and get frame return value. (Command Status)
 *
 * Received:
 *		at_status_t		received return value
 *
 * Returns:
 *		at_status_t		stored return value
 *
 * last modified: 2017/01/26
 */
void SET_apFrameRet(at_status_t ret)
{
	frame.ret = ret;
}

at_status_t GET_apFrameRet(void)
{
	return frame.ret;
}



/*
 * Set and get frame read, write and execute option value.
 *
 * Received:
 *		uint8_t		received value depending on read write execute function
 *					and error (will be set as EXEC) return value
 *
 * Returns:
 *	   uint8_t		stored return value
 *
 * last modified: 2017/01/26
 */
void SET_apFrameRWXopt(uint8_t rwx)
{
	frame.rwx = rwx;
}

uint8_t GET_apFrameRWXopt(void)
{
	return frame.rwx;
}



/*
 * Set and get frame id.
 * The frame id is not equal to frame type,
 * the frame id is a frame counter.
 *
 * Received:
 *		uint8_t		received id value
 *
 * Returned:
 *		uint8_t		stored id value
 *
 * last modified: 2017/01/26
 */
void SET_apFrameID(uint8_t id)
{
	frame.id = id;
}

uint8_t GET_apFrameID(void)
{
	return frame.id;
}



/*
 * Set and compare the calculated frame crc.
 * The set function can used to store initial value
 * or to update the stored value.
 *
 * The compare function compared
 * the calculated CRC value with the received CRC
 *
 * Received:
 *		uint8_t		received value for set/ update	(SET_apFrameCRC)
 *		uint8_t		received user crc				(COMPARE_apFrameCRC)
 *		bool_t		boolean value, if its TRUE the stored value will be updated
 *
 * Returned:
 *		uint8_t				calculated crc sum
 *		OP_SUCCESS			if calculated crc equal to user crc			(COMPARE_apFrameCRC)
 *		INVALID_COMMAND		if calculated crc is not equal to user crc	(COMPARE_apFrameCRC)
 *
 * last modified: 2017/01/26
 */
void SET_apFrameCRC(uint8_t crc, bool_t update)
{
	if ( TRUE == update )
	{
		frame.crc  += crc;
	}
	else
	{
		frame.crc = crc;
	}
}

uint8_t GET_apFrameCRC(void)
{
	return 0xFF - frame.crc;
}

at_status_t COMPARE_apFrameCRC(uint8_t userCrc)
{
	if ( (0xFF - frame.crc) == userCrc )
	{
		return OP_SUCCESS;
	}
	else
	{
		return INVALID_COMMAND;
	}
}



/*
 * Set and get AT command letters.
 *
 * Received:
 *		uint8_t		pointer to the array which hold the AT command line		(SET_apFrameATcmd)
 *		uint8_t		pointer which shall receive the AT command letters		(GET_apFrameATcmd)
 *		int			position of the array which shall receive the letters	(GET_apFrameATcmd)
 *
 * Returns:
 *	   nothing
 *
 * last modified: 2017/01/26
 */
void SET_apFrameATcmd(uint8_t *array)
{
	frame.crc += *(array) + *(array+1);
	memcpy( frame.cmd, array,2);
}

void GET_apFrameATcmd(uint8_t *array, int pos)
{
	memcpy( array+pos, frame.cmd, 2);
}



/*
 * Set and get frame length.
 * The set function can set set a new value or
 * update the current value with left shifting.
 *
 * Received:
 *		uint16_t	length value
 *		bool_t		boolean value whether the length shall updated with or operator
 *
 * Returns:
 *		uint16_t	stored length value				(GET_apFrameLength)
 *
 * last modified: 2017/01/26
 */
void SET_apFrameLength(uint16_t length, bool_t or)
{
	if ( TRUE == or )
	{
		frame.length |= length;
	}
	else
	{
		frame.length  = length;
	}
}

uint16_t GET_apFrameLength(void)
{
	return frame.length;
}



/*
 * Set, get and swap message payload
 *
 * Received:
 *		void		pointer to the variable which hold the parameter value	(SET_apFrameMsg)
 *		size_t		size of the parameter
 *		cmdIDs		if known the command ID									(SET_apFrameMsg)
 *		uint8_t		pointer which shall receive the AT command letters		(GET_apFrameMsg)
 *		int			position of the array which shall receive the letters	(GET_apFrameMsg)
 *
 * Returns:
 *		nothing
 *
 * last modified: 2016/12/19
 */
void SET_apFrameMsg(void *val, size_t len, const cmdIDs id)
{
	frame.msg[len] = 0x0;
 	memcpy( frame.msg, (uint8_t*) val, len);

	// swap only if not NI command
 	if ( AT_NI != id && 1 < len ) SWAP_apFrameMsg(len);
}

static void SWAP_apFrameMsg(size_t length)
{
	for (short i = 0; i < length/2; i++)
	{
		frame.msg[i]          ^= frame.msg[length-1-i];
		frame.msg[length-1-i] ^= frame.msg[i];
		frame.msg[i]          ^= frame.msg[length-1-i];
	}
}

void GET_apFrameMsg(uint8_t *array, int pos, size_t len)
{
	memcpy( array+pos, frame.msg, len);
}

/*
 * Set and get frame type
 *
 * received:
 *		uint8_t		received type value
 *
 * Returns:
 *		uint8_t		stored type value
 *
 * last modified: 2017/01/26
 */
void SET_apFrameType(uint8_t type)
{
	frame.type = type;
}

uint8_t GET_apFrameType(void)
{
	return frame.type;
}