/*
 * apiframe.h
 *
 * Created: 25.11.2016 14:34:16
 *  Author: TOE
 */ 


#ifndef APIFRAME_H_
#define APIFRAME_H_

#include <inttypes.h>		// uint8_t
#include "_global.h"
#include "enum_status.h"	// at_status_t
#include "cmd.h"			// CMD

// === defines ============================================
#define AP_TIMEOUT	 0x25
#define STATEM_IDLE	 0x00
#define AP_LENGTH_1	 0x01
#define AP_LENGTH_2	 0x02
#define AP_GET_DATA	 0x03
#define AP_CHECK_CRC 0x04
#define AP_HANDLE	 0x05

// === prototypes =========================================
uint32_t AP_expired_timeHandle(uint32_t arg);

/*
 * Special TRX functions
 * only used in at_trx.c
 */
uint8_t TRX_getFrameID(void);
uint8_t TRX_getFrameRet(void);
void    TRX_getFrameATcmd(uint8_t *array, int pos);
void    TRX_getFrameMsg(uint8_t * array, int pos, size_t len);

/*
 * Set-functions
 * setATcmd		writes the command into the frame struct array
 * setRWXopt	writes the READ, WRITE or EXEC flag into the frame struct
 * setMSG		writes uint8/16/32/64_t value into the uint8_t frame struct array
 */
void AP_setATcmd (uint8_t *array);
void AP_setRWXopt(uint8_t opt);
void AP_setMSG   (void *val, size_t length);


/*
 * API frame length functions
 * set the length of a frame
 * get the length of a frame
 */
at_status_t AP_setFrameLength(uint16_t val, bool_t shift);
uint16_t    AP_getFrameLength(void);

/*
 * API frame CRC functions:
 * set, update and compared the calculated crc with user crc
 */
void   AP_setCRC    (uint8_t val);
void   AP_updateCRC (uint8_t val);
bool_t AP_compareCRC(uint8_t val);

/*
 * Main functions: 
 *
 * - parse API frame
 * - handle API frame if from UART
 * - handle API frame if from transceiver
 * - create an API status frame if a TRX_transmit16/64Frame was send
 * - create an API remote response frame if another devices responded to a remote frame
 * - create receiver message frame if data received without option parameter or the option parameter was set to 0
 */
void AP_parser					( uint8_t inchar, bufType_n bufType, timeStat_t *th );
void AP_frameHandle_uart		( bufType_n bufType );
void AP_atRemoteFrame_localExec	( bufType_n bufType, uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen);
void AP_atRemote_response		( bufType_n bufType, uint16_t length);
void AP_rxReceive				( bufType_n bufType, uint16_t length, uint8_t *srcAddr, uint8_t srcAddrLen );
void AP_txStatus				(at_status_t status);

/*
 * Transceiver functions:
 *
 * - created a API frame if data was received over air
 * - create a remote frame to control another device
 * - create a remote response frame if this device has received a remote frame
 * - create a package for a message which will transmitted with 16/64 bit dest. address
 */
void TRX_createAPframe    (bufType_n bufType, uint8_t flen, uint8_t dataStart, uint8_t srcAddrLen, uint8_t option);
int  TRX_atRemoteFrame    (bufType_n bufType, uint8_t *send);
int  TRX_atRemote_response(bufType_n bufType, uint8_t *send, uint8_t *srcAddr, uint8_t srcAddrLen);
int  TRX_transmit64Frame  (bufType_n bufType, uint8_t *send);
int  TRX_transmit16Frame  (bufType_n bufType, uint8_t *send);

#endif /* APIFRAME_H_ */