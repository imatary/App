/*
 * apiframe.h
 *
 * Created: 25.11.2016 14:34:16
 *  Author: TOE
 */ 


#ifndef APIFRAME_H_
#define APIFRAME_H_

#include <inttypes.h>		// uint8_t
#include "enum_status.h"	// at_status_t
#include "cmd.h"			// CMD

// === std. defines & frame types =========================
#define STD_DELIMITER	(0x7E)
#define TX_MSG_64		(0x00)
#define TX_MSG_16		(0x01)
#define AT_COMMAND		(0x08)
#define AT_COMMAND_Q	(0x09)
#define REMOTE_AT_CMD	(0x17)
#define DEVICE_AT_CMD	(0x18)
#define RX_MSG_64		(0x80)
#define RX_MSG_16		(0x81)

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
	uint16_t  crc;			// normal it should be 1 Byte but in some cases it cause a calculation error
}__attribute__((packed));

// === prototypes =========================================
/*
 * Main function to handle AP frames 
 */
void AP_frameHandle_uart(void);

/*
 * Set-functions
 * setATcmd		writes the command into the frame struct array
 * setRWXopt	writes the READ, WRITE or EXEC flag into the frame struct
 * setMSG		writes uint8/16/32/64_t value into the uint8_t frame struct array
 */
void AP_setATcmd(uint8_t *array);
void AP_setRWXopt(uint8_t opt);
void AP_setMSG(void *val, short length, uint8_t swapp);

/*
 * update or compared the calculated crc with user crc
 */
void	AP_updateCRC(uint8_t *val);
bool_t  AP_compareCRC(void);

/*
 * created a AP frame if data was received over air
 */
void TRX_createAPframe( uint8_t flen, uint8_t dataStart, uint8_t srcAddrLen, uint8_t option);

/*
 * pack a package for a remote AT command and AT command response
 */
int TRX_0x17_atRemoteFrame(uint8_t *send);
int TRX_0x97_atRemote_response(uint8_t *send, uint8_t *srcAddr, uint8_t srcAddrLen);

/*
 * pack a package for a message which will transmitted
 */
int TRX_0x01_transmit64Frame(uint8_t *send);
int TRX_0x02_transmit16Frame(uint8_t *send);

#endif /* APIFRAME_H_ */