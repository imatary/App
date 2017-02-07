/*
 * apiframe.h
 *
 * Created: 25.11.2016 14:34:16
 *  Author: TOE
 */


#ifndef APIFRAME_H_
#define APIFRAME_H_

// === includes ===========================================
#include <inttypes.h>			// uint8_t
#include <stddef.h>

#include "_global.h"
#include "enum_status.h"		// at_status_t
#include "cmd.h"				// Command IDs
#include "circularBuffer.h"

// === prototypes =========================================
/*
 * frame struct access functions
 */
void		SET_apFrameRet		(at_status_t ret);
void		SET_apFrameRWXopt	(uint8_t rwx);
void		SET_apFrameID		(uint8_t id);
void		SET_apFrameCRC		(uint8_t crc, bool_t update);
void		SET_apFrameATcmd	(uint8_t *array);
void		SET_apFrameMsg		(void *val, size_t len, const cmdIDs id);
void		SET_apFrameType		(uint8_t type);
void        SET_apFrameLength	(uint16_t length, bool_t or);

void		GET_apFrameMsg		(uint8_t *array, int pos, size_t len);
void		GET_apFrameATcmd	(uint8_t *array, int pos);
uint8_t		GET_apFrameRWXopt	(void);
uint8_t		GET_apFrameID		(void);
uint8_t		GET_apFrameType		(void);
uint8_t		GET_apFrameCRC      (void);
uint16_t	GET_apFrameLength	(void);
at_status_t GET_apFrameRet		(void);
at_status_t COMPARE_apFrameCRC	(uint8_t userCrc);

/*
 * read, write and execute functions
 */
at_status_t AP_read	 ( const CMD *cmd );
at_status_t AP_write ( bufType_n bufType, const CMD *cmd );
at_status_t AP_exec  ( cmdIDs cmdID );


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
void AP_parser					( uint8_t inchar, bufType_n bufType );
void AP_frameHandle_uart		( bufType_n bufType );
void AP_atRemoteFrame_localExec ( bufType_n bufType, uint16_t length, uint8_t dataStart, uint8_t srcAddrLen);
void AP_atRemote_response		( bufType_n bufType, uint16_t length, uint8_t dataStart);
void AP_rxReceive				( bufType_n bufType, uint16_t length, uint8_t dataStart, uint8_t srcAddrLen );
void AP_txStatus				( at_status_t status);

/*
 * Transceiver functions:
 *
 * - created a API frame if data was received over air
 * - create a remote frame to control another device
 * - create a remote response frame if this device has received a remote frame
 * - create a package for a message which will transmitted with 16/64 bit dest. address
 */
void TRX_createAPframe     ( bufType_n bufType, uint8_t flen, uint8_t dataStart, uint8_t srcAddrLen, uint8_t option);
int  TRX_atRemoteFrame     ( bufType_n bufType, uint8_t *send);
int  TRX_atRemote_response ( bufType_n bufType, uint8_t *send, uint8_t *srcAddr, uint8_t srcAddrLen);
int  TRX_transmit64Frame   ( bufType_n bufType, uint8_t *send);
int  TRX_transmit16Frame   ( bufType_n bufType, uint8_t *send);

#endif /* APIFRAME_H_ */