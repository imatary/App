/*
 * rfmodul.h
 *
 * Created: 15.11.2016 11:37:54
 *  Author: TOE
 */ 


#ifndef RFMODUL_H_
#define RFMODUL_H_

#include <inttypes.h>
#include "_global.h"

// === struct =============================================
typedef struct  {
	uint8_t	 netCMD_ni[21];     // max 20* 0xFF                = 168 bit
	uint8_t  netCMD_ch : 6;     // max 0x1A                    =   6 bit
	uint16_t netCMD_id;         // max 0xFFFF                  =  16 bit
	uint32_t netCMD_dh;         // max 0xFFFFFFFF              =  32 bit
	uint32_t netCMD_dl;         // max 0xFFFFFFFF              =  32 bit
	uint16_t netCMD_my;         // max 0xFFFF                  =  16 bit
	uint32_t netCMD_sh;         // max 0xFFFFFFFF              =  32 bit
	uint32_t netCMD_sl;         // max 0xFFFFFFFF              =  32 bit
	bool_t	 netCMD_ce;	        // max 0x1                     =   1 bit
	uint16_t netCMD_sc;         // max 0xFFFF                  =  16 bit
	uint8_t	 netCMD_mm : 3;     // max 0x4                     =   3 bit
	uint8_t  netCMD_rr : 2;     // max 0x3                     =   2 bit
	uint8_t  netCMD_rn : 2;     // max 0x3                     =   2 bit
	uint8_t  netCMD_nt;         // max 0xFC                    =   8 bit
	bool_t   netCMD_no : 1;     // max 0x1                     =   1 bit
	uint8_t  netCMD_sd : 3;     // max 0x4                     =   3 bit
	uint8_t  netCMD_a1 : 4;     // max 0xF                     =   4 bit
	uint8_t  netCMD_a2 : 4;     // max 0xF                     =   4 bit
	uint8_t  netCMD_ai : 6;     // a1(0xF) & a2(0xF) = ai 0x13 =   6 bit
	
	bool_t   secCMD_ee : 1;     // max 0x1                     =   1 bit
	uint8_t  secCMD_ky[16];		// max FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF (32 characters) 128 Bit
	
	uint8_t  rfiCMD_pl : 3;     // max 0x4                     =   3 bit
	uint8_t  rfiCMD_ca : 7;     // max 0x50                    =   7 bit
	
	uint8_t  sleepmCMD_sm : 3;  // max 0x6                     =   3 bit
	uint16_t sleepmCMD_st : 16; // max 0xFFFF                  =  16 bit
	uint16_t sleepmCMD_sp : 15; // max 0x68B0                  =  15 bit
	uint16_t sleepmCMD_dp : 15; // max 0x68B0                  =  15 bit
	uint8_t  sleepmCMD_so : 3;  // max 0x6                     =   3 bit

	uint8_t  serintCMD_bd : 3;  // max 0x7                     =   3 bit
	uint8_t  serintCMD_nb : 3;  // max 0x4                     =   3 bit
	uint8_t  serintCMD_ro;      // max 0xFF                    =   8 bit
	uint8_t  serintCMD_ap : 2;  // max 0x2                     =   2 bit
	
	uint8_t  ioserCMD_d8 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_d7 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_d6 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_d5 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_d4 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_d3 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_d2 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_d1 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_d0 : 3;  // max 0x5                      =   3 bit
	uint8_t  ioserCMD_pr;      // max 0xFF                     =   8 bit
	bool_t   ioserCMD_iu;      // max 0x1                      =   1 bit
	uint8_t  ioserCMD_it;      // max 0xFF                     =   8 bit
	uint8_t  ioserCMD_ic;      // max 0xFF                     =   8 bit
	uint16_t ioserCMD_ir;      // max 0xFFFF                   =  16 bit
	uint8_t  ioserCMD_p0 : 2;  // max 0x2                      =   2 bit
	uint8_t  ioserCMD_p1 : 2;  // max 0x2                      =   2 bit
	uint8_t  ioserCMD_pt;      // max 0xFF                     =   8 bit
	uint8_t  ioserCMD_rp;      // max 0xFF                     =   8 bit
	
	uint64_t iolpCMD_ia;       // 0xFFFFFFFFFFFFFFFF           =  64 bit
	uint8_t  iolpCMD_T0;       // max 0xFF                     =   8 bit
	uint8_t  iolpCMD_T1;       // max 0xFF                     =   8 bit
	uint8_t  iolpCMD_T2;       // max 0xFF                     =   8 bit
	uint8_t  iolpCMD_T3;       // max 0xFF                     =   8 bit
	uint8_t  iolpCMD_T4;       // max 0xFF                     =   8 bit
	uint8_t  iolpCMD_T5;       // max 0xFF                     =   8 bit
	uint8_t  iolpCMD_T6;       // max 0xFF                     =   8 bit
	uint8_t  iolpCMD_T7;       // max 0xFF                     =   8 bit
	
	uint16_t diagCMD_vr;       // max 0xFFFF                   =  16 bit
	uint16_t diagCMD_hv;       // max 0xFFFF                   =  16 bit
	uint8_t  diagCMD_db;       // max 0xFF                     =   8 bit
	uint16_t diagCMD_ec;       // max 0xFFFF                   =  16 bit
	uint16_t diagCMD_ea;       // max 0xFFFF                   =  16 bit
	uint32_t diagCMD_dd;       // max 0xFFFFFFFF               =  32 bit
	
	uint16_t atcopCMD_ct : 13; // max 0x1770                   =  13 bit
	uint16_t atcopCMD_gt : 12; // max 0xCE4                    =  12 bit
	uint8_t  atcopCMD_cc;      // max 0xFF                     =   8 bit
	
	bool_t	 deCMD_ru;		   // max 0x1				       =   1 bit
	// ------------------------------------------------------------------
	//                            Total:                        1077 bit // if compiler pack the struct
}__attribute__((packed)) device_t;

extern device_t RFmodul;

// === set functions ==========================================
void SET_netDefault(void);
void SET_secDefault(void);
void SET_rfiDefault(void);
void SET_sleepDafault(void);
void SET_siDefault(void);
void SET_iosDefault(void);
void SET_iolpDefault(void);
void SET_diagDefault(void);
void SET_cmdoDefault(void);
void SET_allDefault(void);
void SET_deDefault(void);

// === EEPROM functions =======================================
void GET_allFromEEPROM(void);
void SET_defaultInEEPROM(void);
void SET_userValInEEPROM(void);

#endif /* RFMODUL_H_ */