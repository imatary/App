/*
 * nvm_eeprom1.c
 *
 * Created: 23.12.2016 09:46:12
 *  Author: TOE
 *
 * contained the order of EEPROM bytes for AT command parameter
 * the default parameter will be written at first startup
 * and every time when the calculated checksum is not equal to the stored checksum in EEPROM
 *
 */
#include <stdint.h>							// uintX_t
#include <string.h>							// srncpy, memcpy, memset
#include <avr/eeprom.h>						// eeprom write & read

#include "../header/_global.h"
#include "../header/defaultConfig.h"		// default values
#include "../header/rfmodul.h"				// RFmodul struct
#include "../../ATuracoli/stackrelated.h"


// === defines ============================================
#define START_POS		0x1DE0	// start position in EEPROM
#define ADDR_SH			0x1FE8	// position of MAC address  (dresden-elektronik ConBee modules)
#define ADDR_SL			0x1FE4

// === local prototypes ===================================
static inline uint16_t crc_16_ccitt(uint16_t crc, uint8_t data);
static        uint16_t calc_crc(uint8_t* pBuffer, uint16_t size);

// === globals ============================================
static uint8_t lary[1024];
static device_t *RFmodul;
static size_t       size;

// === functions ==========================================
/*
 * Get all AT command parameter which are stored in the EEPROM
 *
 * (1) get block of mem
 * (2) check whether it is valid
 * (3) if not, reset to default
 *     if yes, init RFmodul struct and store it
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2017/03/16
 */
void SET_defaultInEEPROM(void)
{
	RFmodul = GET_device();
	size = GET_device_tSize();
	memset(lary, 0xFF, size + 8);
	lary[0] = 0x80;
	lary[1] = 0xDE;
	lary[2] = 0x02;
	lary[3] = (uint8_t) size;

	SET_allDefault();
	RFmodul->netCMD_sh = eeprom_read_dword( (uint32_t*) ADDR_SH );
	RFmodul->netCMD_sl = eeprom_read_dword( (uint32_t*) ADDR_SL );
	RFmodul->netCMD_ai  = 0x0;
	RFmodul->diagCMD_db = 0x0;
	RFmodul->diagCMD_ec = 0x0;
	RFmodul->diagCMD_ea = 0x0;

	memcpy(&lary[4], RFmodul, size );

	uint16_t crc_val = calc_crc( lary, size + 4 );
	memcpy( &lary[size + 6] , &crc_val, 2);

	eeprom_update_block(lary, (void*) START_POS, size + 8 );
}



/*
 * Get all AT command parameter which are stored in the EEPROM
 *
 * (1) get block of mem
 * (2) check whether it is valid
 * (3) if not, reset to default
 *     if yes, init RFmodul struct and store it
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2017/01/13
 */
void GET_allFromEEPROM(void)
{
	RFmodul = GET_device();
	size = GET_device_tSize();
	eeprom_read_block(lary, (void*) START_POS, size + 8);				// (1)
	uint16_t calcCrc = calc_crc(lary, size + 4 );						// (2)
	uint16_t readCrc = (uint16_t) lary[size + 7] << 8 | lary[size + 6];

	if ( readCrc != calcCrc )	 SET_defaultInEEPROM();					// (3)
	else						 memcpy( RFmodul, &lary[4], size );

	if ( 0x10 > RFmodul->atcopCMD_ct ) RFmodul->atcopCMD_ct = 0x64;
	dirtyBits = DIRTYB_MAC_UPDATE | DIRTYB_RO | DIRTYB_CC | DIRTYB_GT | DIRTYB_CT_AT;
}



/*
 * Write changeable values of AT command parameter into EEPROM
 *
 * copy data into mem struct
 * calc checksum
 * write block to EEPROM
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2017/03/16
 */
void SET_userValInEEPROM(void)
{
	RFmodul = GET_device();
	size = GET_device_tSize();
	lary[0] = 0x80;
	lary[1] = 0xDE;
	lary[2] = 0x02;
	lary[3] = (uint8_t) size;

	uint8_t  _tmp  = RFmodul ->serintCMD_ap;
	uint16_t _tmp2 = RFmodul ->atcopCMD_ct;

	RFmodul ->serintCMD_ap = GET_atAP_tmp();
	RFmodul ->atcopCMD_ct  = GET_atCT_tmp();

	memcpy( &lary[4], RFmodul, size );

	RFmodul ->serintCMD_ap = _tmp;
	RFmodul ->atcopCMD_ct  = _tmp2;

	uint16_t crc_val = calc_crc(lary, size + 4 );
	memcpy( &lary[size + 6], &crc_val, 2);

	eeprom_update_block( lary, (void*) START_POS, size + 8 );
}



// === helper functions ===================================
/*
 * Optimized CRC-CCITT calculation.
 *
 * Polynomial: x^16 + x^12 + x^5 + 1 (0x8408)
 * Initial value: 0xffff
 *
 * This is the CRC used by PPP and IrDA.
 * See RFC1171 (PPP protocol) and IrDA IrLAP 1.1
 *
 * Adopted from WinAVR library code
 *
 * Received:
 *		crc The initial/previous CRC.
 *		data The data byte for which checksum must be created
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/24
 */
static inline uint16_t crc_16_ccitt(uint16_t crc, uint8_t data)
{
	data ^= (crc & 0xFF); data ^= data << 4;

	return ((((uint16_t)data << 8) | (uint8_t)(crc >> 8)) ^ \
	(uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}



/*
 * Calculate a CRC-16-CCITT over the given buffer.
 *
 * Received:
 *		pBuffer Pointer to buffer where to calculate CRC.
 *		size Size of buffer.
 *
 * Returns:
 *     final calculated crc checksum
 *
 * Last modified: 2016/11/24
 */
static uint16_t calc_crc(uint8_t* pBuffer, uint16_t size)
{
	uint16_t crc = 0xFFFF; // initialize value
	for(uint16_t i = 0; i < size; i++)
	{
		crc = crc_16_ccitt(crc, *pBuffer++);
	}
	return crc;
}