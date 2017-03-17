/*
 * rfmodul.h
 *
 * Created: 15.11.2016 11:37:54
 *  Author: TOE
 */


#ifndef RFMODUL_H_
#define RFMODUL_H_

#include <inttypes.h>
#include <stddef.h>
#include "_global.h"
#include "cmd.h"

// === struct =============================================
typedef struct {
	uint8_t	 netCMD_ni[21];     // max 20* 0xFF
	uint8_t  secCMD_ky[16];	    // max FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF (32 characters)
	uint8_t  serintCMD_ro;      // max 0xFF
	uint8_t  atcopCMD_cc;       // max 0xFF
	uint64_t iolpCMD_ia;        // 0xFFFFFFFFFFFFFFFF

	uint16_t netCMD_id;         // max 0xFFFF
	uint16_t netCMD_my;         // max 0xFFFF
	uint8_t  netCMD_nt;         // max 0xFC
	uint16_t netCMD_sc;         // max 0xFFFF
	uint32_t netCMD_dh;         // max 0xFFFFFFFF
	uint32_t netCMD_dl;         // max 0xFFFFFFFF
	uint32_t netCMD_sh;         // max 0xFFFFFFFF
	uint32_t netCMD_sl;         // max 0xFFFFFFFF

	uint16_t ioserCMD_ir;       // max 0xFFFF
	uint8_t  ioserCMD_pr;       // max 0xFF
	uint8_t  ioserCMD_it;       // max 0xFF
	uint8_t  ioserCMD_ic;       // max 0xFF
	uint8_t  ioserCMD_pt;       // max 0xFF
	uint8_t  ioserCMD_rp;       // max 0xFF

	uint8_t  iolpCMD_t0;        // max 0xFF
	uint8_t  iolpCMD_t1;        // max 0xFF
	uint8_t  iolpCMD_t2;        // max 0xFF
	uint8_t  iolpCMD_t3;        // max 0xFF
	uint8_t  iolpCMD_t4;        // max 0xFF
	uint8_t  iolpCMD_t5;        // max 0xFF
	uint8_t  iolpCMD_t6;        // max 0xFF
	uint8_t  iolpCMD_t7;        // max 0xFF

	uint16_t diagCMD_vr;        // max 0xFFFF
	uint16_t diagCMD_hv;        // max 0xFFFF
	uint16_t diagCMD_ec;        // max 0xFFFF
	uint16_t diagCMD_ea;        // max 0xFFFF
	uint32_t diagCMD_dd;        // max 0xFFFFFFFF
	uint8_t  diagCMD_db;        // max 0xFF

	uint16_t sleepmCMD_st;      // max 0xFFFF
	uint16_t sleepmCMD_sp;		// max 0x68B0
	uint16_t sleepmCMD_dp;		// max 0x68B0
	uint16_t atcopCMD_ct;		// max 0x1770
	uint16_t atcopCMD_gt;		// max 0xCE4
	uint8_t  rfiCMD_ca;			// max 0x50
	uint8_t  netCMD_ch;			// max 0x1A
	uint8_t  netCMD_ai;			// a1(0xF) & a2(0xF) = ai 0x13
	uint8_t  netCMD_a1;			// max 0xF
	uint8_t  netCMD_a2;			// max 0xF
	uint8_t	 netCMD_mm;			// max 0x4
	uint8_t  ioserCMD_d8;		// max 0x5
	uint8_t  ioserCMD_d7;		// max 0x5
	uint8_t  ioserCMD_d6;		// max 0x5
	uint8_t  ioserCMD_d5;		// max 0x5
	uint8_t  ioserCMD_d4;		// max 0x5
	uint8_t  ioserCMD_d3;		// max 0x5
	uint8_t  ioserCMD_d2;		// max 0x5
	uint8_t  ioserCMD_d1;		// max 0x5
	uint8_t  ioserCMD_d0;		// max 0x5
	uint8_t  serintCMD_bd;		// max 0x7
	uint8_t  serintCMD_nb;		// max 0x4
	uint8_t  sleepmCMD_so;		// max 0x6
	uint8_t  sleepmCMD_sm;		// max 0x6
	uint8_t  rfiCMD_pl;			// max 0x4
	uint8_t  netCMD_sd;			// max 0x4
	uint8_t  netCMD_rr;			// max 0x3
	uint8_t  netCMD_rn;			// max 0x3
	uint8_t  ioserCMD_p0;		// max 0x2
	uint8_t  ioserCMD_p1;		// max 0x2
	uint8_t  serintCMD_ap;		// max 0x2
	bool_t   ioserCMD_iu;		// max 0x1
	bool_t	 netCMD_ce;			// max 0x1
	bool_t   netCMD_no;			// max 0x1
	bool_t   secCMD_ee;			// max 0x1
	// --------------------------------------------------------------------------------- //
	//													   Total:      142               //
}__attribute__((packed)) device_t;

// === set functions ==========================================
void SET_allDefault  (void);

void SET_netCMD_id   (void *val, size_t len);
void SET_netCMD_my   (void *val, size_t len);
void SET_netCMD_nt   (void *val, size_t len);
void SET_netCMD_sc   (void *val, size_t len);
void SET_netCMD_dh   (void *val, size_t len);
void SET_netCMD_dl   (void *val, size_t len);
void SET_netCMD_ni   (void *val, size_t len);
void SET_netCMD_sd   (void *val, size_t len);
void SET_netCMD_rr   (void *val, size_t len);
void SET_netCMD_rn   (void *val, size_t len);
void SET_netCMD_ch   (void *val, size_t len);
void SET_netCMD_a1   (void *val, size_t len);
void SET_netCMD_a2   (void *val, size_t len);
void SET_netCMD_mm   (void *val, size_t len);
void SET_netCMD_ce   (void *val, size_t len);
void SET_netCMD_no   (void *val, size_t len);

void SET_secCMD_ee   (void *val, size_t len);
void SET_secCMD_ky   (void *val, size_t len);

void SET_serintCMD_ro(void *val, size_t len);
void SET_serintCMD_bd(void *val, size_t len);
void SET_serintCMD_nb(void *val, size_t len);

void SET_atcopCMD_cc (void *val, size_t len);
void SET_atcopCMD_gt (void *val, size_t len);

void SET_iolpCMD_ia  (void *val, size_t len);
void SET_iolpCMD_t0  (void *val, size_t len);
void SET_iolpCMD_t1  (void *val, size_t len);
void SET_iolpCMD_t2  (void *val, size_t len);
void SET_iolpCMD_t3  (void *val, size_t len);
void SET_iolpCMD_t4  (void *val, size_t len);
void SET_iolpCMD_t5  (void *val, size_t len);
void SET_iolpCMD_t6  (void *val, size_t len);
void SET_iolpCMD_t7  (void *val, size_t len);

void SET_ioserCMD_ir (void *val, size_t len);
void SET_ioserCMD_pr (void *val, size_t len);
void SET_ioserCMD_it (void *val, size_t len);
void SET_ioserCMD_ic (void *val, size_t len);
void SET_ioserCMD_pt (void *val, size_t len);
void SET_ioserCMD_rp (void *val, size_t len);
void SET_ioserCMD_d8 (void *val, size_t len);
void SET_ioserCMD_d7 (void *val, size_t len);
void SET_ioserCMD_d6 (void *val, size_t len);
void SET_ioserCMD_d5 (void *val, size_t len);
void SET_ioserCMD_d4 (void *val, size_t len);
void SET_ioserCMD_d3 (void *val, size_t len);
void SET_ioserCMD_d2 (void *val, size_t len);
void SET_ioserCMD_d1 (void *val, size_t len);
void SET_ioserCMD_d0 (void *val, size_t len);
void SET_ioserCMD_p0 (void *val, size_t len);
void SET_ioserCMD_p1 (void *val, size_t len);
void SET_ioserCMD_iu (void *val, size_t len);

void SET_diagCMD_vr  (void *val, size_t len);
void SET_diagCMD_hv  (void *val, size_t len);
void SET_diagCMD_dd  (void *val, size_t len);

void SET_sleepmCMD_st(void *val, size_t len);
void SET_sleepmCMD_sp(void *val, size_t len);
void SET_sleepmCMD_dp(void *val, size_t len);
void SET_sleepmCMD_so(void *val, size_t len);
void SET_sleepmCMD_sm(void *val, size_t len);

void SET_rfiCMD_ca   (void *val, size_t len);
void SET_rfiCMD_pl   (void *val, size_t len);

void SET_atCT_tmp    (void *val, size_t len);
void SET_atAP_tmp    (void *val, size_t len);


void SET_diagCMD_ec  (uint16_t val); // not in use right now
void SET_diagCMD_ea  (uint16_t val); // used in tx irq
void SET_atcopCMD_ct (uint16_t val); // used in AC command
void SET_serintCMD_ap(uint8_t  val); // used in AC command
void SET_diagCMD_db  (uint8_t  val); // used in receive function

// === get functions ==========================================
size_t   GET_device_tSize(void);
device_t *GET_device(void);

uint8_t  GET_atAP_tmp(void);
uint16_t GET_atCT_tmp(void);

uint8_t GET_netCMD_mm   (void);
uint8_t GET_serintCMD_ap(void);
uint8_t GET_serintCMD_bd(void);
uint8_t GET_serintCMD_ro(void);

uint8_t  GET_atcopCMD_cc(void);
uint16_t GET_atcopCMD_gt(void);
uint16_t GET_atcopCMD_ct(void);

uint16_t GET_deviceValue( void *dest, const CMD *cmd );

// === general functions ======================================
void deviceMemcpy(uint8_t *arrayStart, bool_t from_device_t);

// === EEPROM functions =======================================
void GET_allFromEEPROM(void);
void SET_defaultInEEPROM(void);
void SET_userValInEEPROM(void);

#endif /* RFMODUL_H_ */