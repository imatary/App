/*
 * nvm_eeprom.c
 *
 * Created: 23.11.2016 08:54:26
 *  Author: TOE
 *
 * contained the order of EEPROM bytes for AT command parameter
 * the default parameter will be written at first startup 
 * and every time when the calculated checksum is not equal to the stored checksum in EEPROM 
 *
 * size:
 *		21 bytes | node identifier block (uint8_t array)
 *		16 bytes | AES 128 Key block (uint8_t array)
 *		 4 bytes |  4x bool_t
 *		40 bytes | 40x uint8_t
 *		24 bytes | 12x uint16_t 
 *		12 bytes |  3x uint32_t
 *		 8 bytes |  1x uint64_t
 *	-------------------------------
 *	   124 bytes | = 7C, used by AT command parameter
 *	+   30 bytes | reserved
 *  +    4 bytes | header
 *  +    2 bytes | crc checksum
 *	-------------------------------
 *	   160 bytes | total in use
 */ 

#include <stdint.h>							// uintX_t
#include <string.h>							// srncpy, memcpy, memset
#include <avr/eeprom.h>						// eeprom write & read
#include "../header/defaultConfig.h"		// default values
#include "../header/rfmodul.h"				// RFmodul struct

// === defines ============================================
#define START_POS		0x1DE0	// start position in EEPROM
#define PAYLOAD_LENGTH	0x7C	// see above
#define ADDR_SH			0x1FE8	// position of MAC address  (dresden-elektronik ConBee modules)
#define ADDR_SL			0x1FE4

// === local prototypes ===================================
static inline uint16_t crc_16_ccitt(uint16_t crc, uint8_t data);
static        uint16_t calc_crc(uint8_t* pBuffer, uint16_t size);

// === mem struct =========================================
typedef struct nvm {
	// === mem info =======================================
	uint16_t   magic;		// offset 0x00
	uint8_t  version;		// offset 0x02
	uint8_t      len;		// offset 0x03

	// === default values in mem order ====================
	uint8_t  netCMD_ch;		// offset 0x04
	uint8_t	 netCMD_mm;		// offset 0x05
	uint16_t netCMD_id;     // offset 0x06
	uint32_t netCMD_dl;		// offset 0x08
	uint32_t netCMD_dh;		// offset 0x0C
	uint16_t netCMD_my;		// offset 0x10
	uint8_t	 netCMD_ce;		// offset 0x12
	uint8_t  netCMD_rr;		// offset 0x13
	uint16_t netCMD_sc;		// offset 0x14
	uint8_t	 netCMD_ni[21];	// offset 0x16
	uint8_t  netCMD_rn;		// offset 0x2B
	uint8_t  netCMD_nt;		// offset 0x2C
	uint8_t  netCMD_no;		// offset 0x2D
	uint8_t  netCMD_sd;		// offset 0x2E
	uint8_t  netCMD_a1;		// offset 0x2F
	uint8_t  netCMD_a2;		// offset 0x30

	uint8_t  secCMD_ky[16];	// offset 0x31
	uint8_t  secCMD_ee;		// offset 0x41

	uint8_t  serintCMD_bd;	// offset 0x42
	uint8_t  serintCMD_nb;	// offset 0x43
	uint8_t  serintCMD_ap;	// offset 0x44
	uint8_t  serintCMD_ro;	// offset 0x45

	uint8_t  rfiCMD_pl;		// offset 0x46
	uint8_t  rfiCMD_ca;		// offset 0x47

	uint8_t  sleepmCMD_sm;	// offset 0x48
	uint8_t  sleepmCMD_so;	// offset 0x49
	uint16_t sleepmCMD_st;	// offset 0x4A
	uint16_t sleepmCMD_sp;	// offset 0x4C
	uint16_t sleepmCMD_dp;	// offset 0x4E

	uint8_t  ioserCMD_d8;	// offset 0x50
	uint8_t  ioserCMD_d7;	// offset 0x51
	uint8_t  ioserCMD_d6;	// offset 0x52
	uint8_t  ioserCMD_d5;	// offset 0x53
	uint8_t  ioserCMD_d4;	// offset 0x54
	uint8_t  ioserCMD_d3;	// offset 0x55
	uint8_t  ioserCMD_d2;	// offset 0x56
	uint8_t  ioserCMD_d1;	// offset 0x57
	uint8_t  ioserCMD_d0;	// offset 0x58
	uint8_t  ioserCMD_pr;	// offset 0x59
	uint8_t  ioserCMD_iu;	// offset 0x5A
	uint8_t  ioserCMD_it;	// offset 0x5B
	uint8_t  ioserCMD_ic;	// offset 0x5C
	uint8_t  ioserCMD_rp;	// offset 0x5D
	uint16_t ioserCMD_ir;	// offset 0x5E
	uint8_t  ioserCMD_p0;	// offset 0x60
	uint8_t  ioserCMD_p1;	// offset 0x61
	uint8_t  ioserCMD_pt;	// offset 0x62

	uint8_t  iolpCMD_T0;	// offset 0x63
	uint8_t  iolpCMD_T1;	// offset 0x64
	uint8_t  iolpCMD_T2;	// offset 0x65
	uint8_t  iolpCMD_T3;	// offset 0x66
	uint8_t  iolpCMD_T4;	// offset 0x67
	uint64_t iolpCMD_ia;	// offset 0x68
	uint8_t  iolpCMD_T5;	// offset 0x70
	uint8_t  iolpCMD_T6;	// offset 0x71
	uint8_t  iolpCMD_T7;	// offset 0x72

	uint8_t  atcopCMD_cc;	// offset 0x73
	uint16_t atcopCMD_ct;	// offset 0x74
	uint16_t atcopCMD_gt;	// offset 0x76

	uint32_t diagCMD_dd;	// offset 0x78
	uint16_t diagCMD_vr;	// offset 0x7C
	uint16_t diagCMD_hv;	// offset 0x7E
	
	uint8_t  deCMD_ru;		// offset 0x7F

	uint8_t reserved[29];	// offset 0x80
	uint16_t crc;			// offset 0x9E

} NVM;


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
 * Last modified: 2016/11/24
 */
void SET_defaultInEEPROM(void)
{
	NVM defaultValuesInEEPROM;
	memset(&defaultValuesInEEPROM, 0xFF,sizeof(NVM));
	eeprom_write_block(&defaultValuesInEEPROM, (void*) START_POS, sizeof(NVM)); // erase everything
	eeprom_write_dword((uint32_t*) 0x1E80 ,0xFFFFFFFF);
	
	strncpy(defaultValuesInEEPROM.netCMD_ni, NI_NODE_IDENTIFY,9);
	memset(&defaultValuesInEEPROM.secCMD_ky, 0x00, 16);
	defaultValuesInEEPROM.magic        = 0xDE80;
	defaultValuesInEEPROM.version      = 0x02;
	defaultValuesInEEPROM.len	       = PAYLOAD_LENGTH;
	defaultValuesInEEPROM.netCMD_ch    = CH_CHANNEL;
	defaultValuesInEEPROM.netCMD_id    = ID_PANID;
	defaultValuesInEEPROM.netCMD_dl    = DL_DEST_LOW;
	defaultValuesInEEPROM.netCMD_dh    = DH_DEST_HIGH;
	defaultValuesInEEPROM.netCMD_my    = MY_SHORT_ADDR;
	defaultValuesInEEPROM.netCMD_ce    = 0x00;
	defaultValuesInEEPROM.netCMD_sc    = SC_SCAN_CHANNELS;
	defaultValuesInEEPROM.netCMD_mm    = MM_MAC_MODE;
	defaultValuesInEEPROM.netCMD_rr    = RR_XBEE_RETRIES;
	defaultValuesInEEPROM.netCMD_rn    = RN_RANDOM_DELAY_SLOTS;
	defaultValuesInEEPROM.netCMD_nt    = NT_NODE_DISCOVER_TIME;
	defaultValuesInEEPROM.netCMD_no    = 0x00;
	defaultValuesInEEPROM.netCMD_sd    = SD_SCAN_DURATION;
	defaultValuesInEEPROM.netCMD_a1    = A1_END_DEVICE_ASSOCIATION;
	defaultValuesInEEPROM.netCMD_a2    = A2_COORDINATOR_ASSOCIATION;
	defaultValuesInEEPROM.secCMD_ee    = 0x00;
	defaultValuesInEEPROM.rfiCMD_pl    = PL_POWER_LEVEL;
	defaultValuesInEEPROM.rfiCMD_ca    = CA_CCA_TRESHOLD;
	defaultValuesInEEPROM.sleepmCMD_sm = SM_SLEEP_MODE;
	defaultValuesInEEPROM.sleepmCMD_st = ST_TIME_BEFORE_SLEEP;
	defaultValuesInEEPROM.sleepmCMD_sp = SP_CYCLIC_SLEEP_PERIOD;
	defaultValuesInEEPROM.sleepmCMD_dp = DP_DISASSOCIATED_SP;
	defaultValuesInEEPROM.sleepmCMD_so = SO_SLEEP_OPTION;
	defaultValuesInEEPROM.serintCMD_bd = BD_INTERFACE_DATA_RATE;
	defaultValuesInEEPROM.serintCMD_nb = NB_PARITY;
	defaultValuesInEEPROM.serintCMD_ro = RO_PACKETIZATION_TIMEOUT;
	defaultValuesInEEPROM.serintCMD_ap = AP_API_ENABLE;
	defaultValuesInEEPROM.ioserCMD_d8  = D8_DI8_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_d7  = D7_DIO7_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_d6  = D6_DIO6_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_d5  = D5_DIO5_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_d4  = D4_DIO4_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_d3  = D3_DIO3_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_d2  = D2_DIO2_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_d1  = D1_DIO1_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_d0  = D0_DIO0_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_pr  = PR_PULLUP_RESISTOR_ENABLE;
	defaultValuesInEEPROM.ioserCMD_iu  = 0x1;
	defaultValuesInEEPROM.ioserCMD_it  = IT_SAMPLES_BEFORE_TX;
	defaultValuesInEEPROM.ioserCMD_ic  = IC_DIO_CHANGE_DETECT;
	defaultValuesInEEPROM.ioserCMD_ir  = IR_SAPLE_RATE;
	defaultValuesInEEPROM.ioserCMD_p0  = P0_PWM0_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_p1  = P1_PWM1_CONFIGURATION;
	defaultValuesInEEPROM.ioserCMD_pt  = PT_PWM_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.ioserCMD_rp  = RP_RSSI_PWM_TIMER;
	defaultValuesInEEPROM.iolpCMD_ia   = IA_IO_INPUT_ADDRESS;
	defaultValuesInEEPROM.iolpCMD_T0   = T0_D0_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.iolpCMD_T1   = T1_D1_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.iolpCMD_T2   = T2_D2_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.iolpCMD_T3   = T3_D3_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.iolpCMD_T4   = T4_D4_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.iolpCMD_T5   = T5_D5_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.iolpCMD_T6   = T6_D6_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.iolpCMD_T7   = T7_D7_OUTPUT_TIMEOUT;
	defaultValuesInEEPROM.diagCMD_vr   = VR_FIRMWARE_VERS;
	defaultValuesInEEPROM.diagCMD_hv   = HV_HARDWARE_VERS;
	defaultValuesInEEPROM.diagCMD_dd   = DD_DEVICE_TYPE_IDENTIFIER;
	defaultValuesInEEPROM.atcopCMD_ct  = CT_AT_CMD_TIMEOUT;
	defaultValuesInEEPROM.atcopCMD_gt  = GT_GUART_TIMES;
	defaultValuesInEEPROM.atcopCMD_cc  = CC_COMMAND_SEQUENCE_CHAR;
	defaultValuesInEEPROM.deCMD_ru	   = RU_RETURN_TO_UART;
	
	uint8_t lary[sizeof(NVM)];
	memcpy(lary, &defaultValuesInEEPROM, sizeof(NVM)); 
	defaultValuesInEEPROM.crc	       = calc_crc(lary, PAYLOAD_LENGTH+4);

	eeprom_write_block(&defaultValuesInEEPROM, (void*) START_POS, sizeof(NVM));	
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
 * Last modified: 2016/11/23
 */
void GET_allFromEEPROM(void)
{
	NVM ValuesFromEEPROM;
	uint8_t workBuff[sizeof(NVM)];
			
	eeprom_read_block(&ValuesFromEEPROM, (void*) START_POS, sizeof(NVM));	// (1)
	memcpy(workBuff, &ValuesFromEEPROM, sizeof(NVM));						// (2)
	uint16_t calcCrc = calc_crc(workBuff, PAYLOAD_LENGTH+4 );
	
	if ( ValuesFromEEPROM.crc != calcCrc )									// (3)
	{
		SET_defaultInEEPROM();
		SET_allDefault();
		RFmodul.netCMD_sh = eeprom_read_dword( (uint32_t*) ADDR_SH );
		RFmodul.netCMD_sl = eeprom_read_dword( (uint32_t*) ADDR_SL );
		RFmodul.netCMD_ai = 0x0;
		RFmodul.diagCMD_db = 0x0;
		RFmodul.diagCMD_ec = 0x0;
		RFmodul.diagCMD_ea = 0x0;
	}
	else
	{
		 RFmodul.netCMD_ch = ValuesFromEEPROM.netCMD_ch;
		 RFmodul.netCMD_id = ValuesFromEEPROM.netCMD_id;
		 RFmodul.netCMD_dh = ValuesFromEEPROM.netCMD_dh;
		 RFmodul.netCMD_dl = ValuesFromEEPROM.netCMD_dl;
		 RFmodul.netCMD_my = ValuesFromEEPROM.netCMD_my;
		 RFmodul.netCMD_sh = eeprom_read_dword( (uint32_t*) ADDR_SH );
		 RFmodul.netCMD_sl = eeprom_read_dword( (uint32_t*) ADDR_SL );
		 RFmodul.netCMD_ce = ValuesFromEEPROM.netCMD_ce;
		 RFmodul.netCMD_sc = ValuesFromEEPROM.netCMD_sc;
		 strncpy( RFmodul.netCMD_ni, ValuesFromEEPROM.netCMD_ni, 21);
		 RFmodul.netCMD_mm = ValuesFromEEPROM.netCMD_mm;
		 RFmodul.netCMD_rr = ValuesFromEEPROM.netCMD_rr;
		 RFmodul.netCMD_rn = ValuesFromEEPROM.netCMD_rn;
		 RFmodul.netCMD_nt = ValuesFromEEPROM.netCMD_nt;
		 RFmodul.netCMD_no = ValuesFromEEPROM.netCMD_no;
		 RFmodul.netCMD_sd = ValuesFromEEPROM.netCMD_sd;
		 RFmodul.netCMD_a1 = ValuesFromEEPROM.netCMD_a1;
		 RFmodul.netCMD_a2 = ValuesFromEEPROM.netCMD_a2;
		 RFmodul.netCMD_ai = 0x0;

		 RFmodul.secCMD_ee = ValuesFromEEPROM.secCMD_ee;

		 RFmodul.rfiCMD_pl = ValuesFromEEPROM.rfiCMD_pl;
		 RFmodul.rfiCMD_ca = ValuesFromEEPROM.rfiCMD_ca;
		 
		 RFmodul.sleepmCMD_sm = ValuesFromEEPROM.sleepmCMD_sm;
		 RFmodul.sleepmCMD_st = ValuesFromEEPROM.sleepmCMD_st;
		 RFmodul.sleepmCMD_sp = ValuesFromEEPROM.sleepmCMD_sp;
		 RFmodul.sleepmCMD_dp = ValuesFromEEPROM.sleepmCMD_dp;
		 RFmodul.sleepmCMD_so = ValuesFromEEPROM.sleepmCMD_so;
		 
		 RFmodul.serintCMD_bd = ValuesFromEEPROM.serintCMD_bd;
		 RFmodul.serintCMD_nb = ValuesFromEEPROM.serintCMD_nb;
		 RFmodul.serintCMD_ro = ValuesFromEEPROM.serintCMD_ro;
		 RFmodul.serintCMD_ap = ValuesFromEEPROM.serintCMD_ap;
		 
		 RFmodul.ioserCMD_d8 = ValuesFromEEPROM.ioserCMD_d8;
		 RFmodul.ioserCMD_d7 = ValuesFromEEPROM.ioserCMD_d7;
		 RFmodul.ioserCMD_d6 = ValuesFromEEPROM.ioserCMD_d6;
		 RFmodul.ioserCMD_d5 = ValuesFromEEPROM.ioserCMD_d5;
		 RFmodul.ioserCMD_d4 = ValuesFromEEPROM.ioserCMD_d4;
		 RFmodul.ioserCMD_d3 = ValuesFromEEPROM.ioserCMD_d3;
		 RFmodul.ioserCMD_d2 = ValuesFromEEPROM.ioserCMD_d2;
		 RFmodul.ioserCMD_d1 = ValuesFromEEPROM.ioserCMD_d1;
		 RFmodul.ioserCMD_d0 = ValuesFromEEPROM.ioserCMD_d0;
		 RFmodul.ioserCMD_pr = ValuesFromEEPROM.ioserCMD_pr;
		 RFmodul.ioserCMD_iu = ValuesFromEEPROM.ioserCMD_iu;
		 RFmodul.ioserCMD_it = ValuesFromEEPROM.ioserCMD_it;
		 RFmodul.ioserCMD_ic = ValuesFromEEPROM.ioserCMD_ic;
		 RFmodul.ioserCMD_ir = ValuesFromEEPROM.ioserCMD_ir;
		 RFmodul.ioserCMD_p0 = ValuesFromEEPROM.ioserCMD_p0;
		 RFmodul.ioserCMD_p1 = ValuesFromEEPROM.ioserCMD_p1;
		 RFmodul.ioserCMD_pt = ValuesFromEEPROM.ioserCMD_pt;
		 RFmodul.ioserCMD_rp = ValuesFromEEPROM.ioserCMD_rp;
		 
		 RFmodul.iolpCMD_ia = ValuesFromEEPROM.iolpCMD_ia; 
		 RFmodul.iolpCMD_T0 = ValuesFromEEPROM.iolpCMD_T0; 
		 RFmodul.iolpCMD_T1 = ValuesFromEEPROM.iolpCMD_T1; 
		 RFmodul.iolpCMD_T2 = ValuesFromEEPROM.iolpCMD_T2; 
		 RFmodul.iolpCMD_T3 = ValuesFromEEPROM.iolpCMD_T3; 
		 RFmodul.iolpCMD_T4 = ValuesFromEEPROM.iolpCMD_T4; 
		 RFmodul.iolpCMD_T5 = ValuesFromEEPROM.iolpCMD_T5; 
		 RFmodul.iolpCMD_T6 = ValuesFromEEPROM.iolpCMD_T6; 
		 RFmodul.iolpCMD_T7 = ValuesFromEEPROM.iolpCMD_T7; 

		 RFmodul.diagCMD_vr = ValuesFromEEPROM.diagCMD_vr; 
		 RFmodul.diagCMD_hv = ValuesFromEEPROM.diagCMD_hv; 
		 RFmodul.diagCMD_db = 0x0;
		 RFmodul.diagCMD_ec = 0x0;
		 RFmodul.diagCMD_ea = 0x0;
		 RFmodul.diagCMD_dd = ValuesFromEEPROM.diagCMD_dd ;
		 					   
		 RFmodul.atcopCMD_ct = ValuesFromEEPROM.atcopCMD_ct;
		 RFmodul.atcopCMD_gt = ValuesFromEEPROM.atcopCMD_gt;
		 RFmodul.atcopCMD_cc = ValuesFromEEPROM.atcopCMD_cc;
		 
		 RFmodul.deCMD_ru = ValuesFromEEPROM.deCMD_ru;
		
	}
	
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
 * Last modified: 2016/11/24
 */
void SET_userValInEEPROM(void)
{
	NVM ValuesForEEPROM;
	memset(&ValuesForEEPROM, 0xFF,sizeof(NVM));
	
	ValuesForEEPROM.magic        = 0xDE80;
	ValuesForEEPROM.version      = 0x02;
	ValuesForEEPROM.len	         = PAYLOAD_LENGTH;
	
	strncpy( ValuesForEEPROM.netCMD_ni, RFmodul.netCMD_ni, 21);
	memcpy(ValuesForEEPROM.secCMD_ky, RFmodul.secCMD_ky, 16);
	ValuesForEEPROM.netCMD_ch    = RFmodul.netCMD_ch;
	ValuesForEEPROM.netCMD_id    = RFmodul.netCMD_id;
	ValuesForEEPROM.netCMD_dh    = RFmodul.netCMD_dh;
	ValuesForEEPROM.netCMD_dl    = RFmodul.netCMD_dl;
	ValuesForEEPROM.netCMD_my    = RFmodul.netCMD_my;
	ValuesForEEPROM.netCMD_ce    = RFmodul.netCMD_ce;
	ValuesForEEPROM.netCMD_sc    = RFmodul.netCMD_sc;
	ValuesForEEPROM.netCMD_mm    = RFmodul.netCMD_mm;
	ValuesForEEPROM.netCMD_rr    = RFmodul.netCMD_rr;
	ValuesForEEPROM.netCMD_rn    = RFmodul.netCMD_rn;
	ValuesForEEPROM.netCMD_nt    = RFmodul.netCMD_nt;
	ValuesForEEPROM.netCMD_no    = RFmodul.netCMD_no;
	ValuesForEEPROM.netCMD_sd    = RFmodul.netCMD_sd;
	ValuesForEEPROM.netCMD_a1    = RFmodul.netCMD_a1;
	ValuesForEEPROM.netCMD_a2    = RFmodul.netCMD_a2;
	ValuesForEEPROM.secCMD_ee    = RFmodul.secCMD_ee;
	ValuesForEEPROM.rfiCMD_pl    = RFmodul.rfiCMD_pl;
	ValuesForEEPROM.rfiCMD_ca    = RFmodul.rfiCMD_ca;
	ValuesForEEPROM.sleepmCMD_sm = RFmodul.sleepmCMD_sm;
	ValuesForEEPROM.sleepmCMD_st = RFmodul.sleepmCMD_st;
	ValuesForEEPROM.sleepmCMD_sp = RFmodul.sleepmCMD_sp;
	ValuesForEEPROM.sleepmCMD_dp = RFmodul.sleepmCMD_dp;
	ValuesForEEPROM.sleepmCMD_so = RFmodul.sleepmCMD_so;
	ValuesForEEPROM.serintCMD_bd = RFmodul.serintCMD_bd;
	ValuesForEEPROM.serintCMD_nb = RFmodul.serintCMD_nb;
	ValuesForEEPROM.serintCMD_ro = RFmodul.serintCMD_ro;
	ValuesForEEPROM.serintCMD_ap = RFmodul.serintCMD_ap;
	ValuesForEEPROM.ioserCMD_d8  = RFmodul.ioserCMD_d8;
	ValuesForEEPROM.ioserCMD_d7  = RFmodul.ioserCMD_d7;
	ValuesForEEPROM.ioserCMD_d6  = RFmodul.ioserCMD_d6;
	ValuesForEEPROM.ioserCMD_d5  = RFmodul.ioserCMD_d5;
	ValuesForEEPROM.ioserCMD_d4  = RFmodul.ioserCMD_d4;
	ValuesForEEPROM.ioserCMD_d3  = RFmodul.ioserCMD_d3;
	ValuesForEEPROM.ioserCMD_d2  = RFmodul.ioserCMD_d2;
	ValuesForEEPROM.ioserCMD_d1  = RFmodul.ioserCMD_d1;
	ValuesForEEPROM.ioserCMD_d0  = RFmodul.ioserCMD_d0;
	ValuesForEEPROM.ioserCMD_pr  = RFmodul.ioserCMD_pr;
	ValuesForEEPROM.ioserCMD_iu  = RFmodul.ioserCMD_iu;
	ValuesForEEPROM.ioserCMD_it  = RFmodul.ioserCMD_it;
	ValuesForEEPROM.ioserCMD_ic  = RFmodul.ioserCMD_ic;
	ValuesForEEPROM.ioserCMD_ir  = RFmodul.ioserCMD_ir;
	ValuesForEEPROM.ioserCMD_p0  = RFmodul.ioserCMD_p0;
	ValuesForEEPROM.ioserCMD_p1  = RFmodul.ioserCMD_p1;
	ValuesForEEPROM.ioserCMD_pt  = RFmodul.ioserCMD_pt;
	ValuesForEEPROM.ioserCMD_rp  = RFmodul.ioserCMD_rp;
	ValuesForEEPROM.iolpCMD_ia   = RFmodul.iolpCMD_ia;
	ValuesForEEPROM.iolpCMD_T0   = RFmodul.iolpCMD_T0;
	ValuesForEEPROM.iolpCMD_T1   = RFmodul.iolpCMD_T1;
	ValuesForEEPROM.iolpCMD_T2   = RFmodul.iolpCMD_T2;
	ValuesForEEPROM.iolpCMD_T3   = RFmodul.iolpCMD_T3;
	ValuesForEEPROM.iolpCMD_T4   = RFmodul.iolpCMD_T4;
	ValuesForEEPROM.iolpCMD_T5   = RFmodul.iolpCMD_T5;
	ValuesForEEPROM.iolpCMD_T6   = RFmodul.iolpCMD_T6;
	ValuesForEEPROM.iolpCMD_T7   = RFmodul.iolpCMD_T7;
	ValuesForEEPROM.diagCMD_vr   = RFmodul.diagCMD_vr;
	ValuesForEEPROM.diagCMD_hv   = RFmodul.diagCMD_hv;
	ValuesForEEPROM.diagCMD_dd   = RFmodul.diagCMD_dd;
	ValuesForEEPROM.atcopCMD_ct  = RFmodul.atcopCMD_ct;
	ValuesForEEPROM.atcopCMD_gt  = RFmodul.atcopCMD_gt;
	ValuesForEEPROM.atcopCMD_cc  = RFmodul.atcopCMD_cc;
	ValuesForEEPROM.deCMD_ru	 = RFmodul.deCMD_ru;
	
	
	uint8_t lary[sizeof(NVM)];
	memcpy(lary, &ValuesForEEPROM, sizeof(NVM));
	ValuesForEEPROM.crc = calc_crc(lary, PAYLOAD_LENGTH+4);
	
	eeprom_write_block(&ValuesForEEPROM, (void*) START_POS, sizeof(NVM));
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