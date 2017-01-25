/*
 * ap_frame.c
 *
 * Created: 25.01.2017 16:41:48
 *  Author: TOE
 */ 
// === object =============================================
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
	
// === functions ===========================================
/*
 *
 */
void        SET_apFrameRet(at_status_t ret) { frame.ret = ret * (-1); }
at_status_t GET_apFrameRet(void)            { return frame.ret; }


/*
 * AP set AT read, write or execute value into frame struct for response 
 *
 * Received:
 *		uint8_t value of READ, WRITE or EXEC
 *
 * Returns:
 *	   nothing
 *
 * last modified: 2016/12/19
 */
void SET_apFrameRWXopt(uint8_t opt) { frame.rwx = opt; }
uint8_t GET_apFrameRWXopt(void) { return frame.rwx; }	
	
	
/*
 *
 */
void    SET_apFrameID(uint8_t id) { frame.id = id; }
uint8_t GET_apFrameID(void)       { return frame.id; }



	

/*
 * The AP CRC functions
 * - set CRC (start) value
 * - update CRC value
 * - compared calculated CRC value with the received CRC
 *
 * Received:
 *		uint8_t		received UART value
 *
 * Returned:
 *		TRUE		if calculated crc equal to user crc			(AP_compareCRC)
 *		FALSE		if calculated crc is not equal to user crc	(AP_compareCRC)
 *
 * last modified: 2017/01/18
 */
void AP_setCRC   (uint8_t val) { frame.crc  = val; }
void AP_updateCRC(uint8_t val) { frame.crc += val; }

bool_t AP_compareCRC(uint8_t val)
{	
	if ( (0xFF - frame.crc) == val )
	{
		return TRUE;
	}
	else 
	{
		frame.rwx = EXEC;
		frame.ret = ERROR;
		return FALSE;
	}
}



/*
 * AP set AT command stored the AT CMD into frame struct for response 
 *
 * Received:
 *		uint8_t pointer to the array which hold the AT command line
 *
 * Returns:
 *	   nothing
 *
 * last modified: 2016/12/19
 */
void SET_apFrameATcmd(uint8_t *array)
{
	frame.crc += *(array) + *(array+1);
	memcpy( frame.cmd, array,2);
}

void GET_apFrameATcmd(uint8_t *array, int pos)  { memcpy( array+pos, frame.cmd, 2);   }



/*
 * AP set frame length
 *
 * Received:
 *		uint8_t value of READ, WRITE or EXEC
 *
 * Returns:
 *	   OP_SUCCESS	if frame length greater then 3
 *	   ERROR		if frame length is below 4
 *
 * last modified: 2016/01/17
 */
at_status_t AP_setFrameLength(uint16_t val, bool_t shift)
{
	if ( TRUE == shift) { frame.length |= val; }
	else				{ frame.length  = val; }

	return ( frame.length < 4 )? ERROR : OP_SUCCESS;
}

uint16_t AP_getFrameLength(void) { return frame.length; }



/*
 * AP set AT command stored the AT CMD for the response in the frame struct
 *
 * Received:
 *		void	pointer to the variable which hold the parameter value
 *		short	size of the parameter
 *		uint8_t boolean value whether the message array should be swapped or not
 *
 * Returns:
 *	   nothing
 *
 * last modified: 2016/12/19
 */
void AP_setMSG(void *val, size_t len)
{
	frame.msg[len] = 0x0;
	
	frame.length = len;
 	memcpy(frame.msg,(uint8_t*) val, len);
	
	// swap only if not NI command
 	if ( *((uint8_t*) val) != frame.msg[len-1] ) swap_msg(len);
}

static void swap_msg(size_t length)
{
	for (short i = 0; i < frame.length/2; i++, length--)
	{
		frame.msg[i]        ^= frame.msg[length-1];
		frame.msg[length-1] ^= frame.msg[i];
		frame.msg[i]        ^= frame.msg[length-1];
	}
}

void GET_apFrameMsg(uint8_t *array, int pos, size_t len) {	memcpy( array+pos, frame.msg, len); }