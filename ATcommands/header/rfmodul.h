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

// === set functions ==========================================
void SET_allDefault(void);
void SET_netCMD_ni   (uint8_t *val, uint8_t len);
void SET_secCMD_ky   (uint8_t *val, uint8_t len);
void SET_serintCMD_ro(uint8_t  val);
void SET_atcopCMD_cc (uint8_t  val);
void SET_iolpCMD_ia  (uint64_t val);

void SET_netCMD_id   (uint16_t val);
void SET_netCMD_my   (uint16_t val);
void SET_netCMD_nt   (uint8_t  val);
void SET_netCMD_sc   (uint16_t val);
void SET_netCMD_dh   (uint32_t val);
void SET_netCMD_dl   (uint32_t val);
void SET_netCMD_sh   (uint32_t val);
void SET_netCMD_sl   (uint32_t val);

void SET_ioserCMD_ir (uint16_t val);
void SET_ioserCMD_pr (uint8_t  val);
void SET_ioserCMD_it (uint8_t  val);
void SET_ioserCMD_ic (uint8_t  val);
void SET_ioserCMD_pt (uint8_t  val);
void SET_ioserCMD_rp (uint8_t  val);

void SET_iolpCMD_T0  (uint8_t  val);
void SET_iolpCMD_T1  (uint8_t  val);
void SET_iolpCMD_T2  (uint8_t  val);
void SET_iolpCMD_T3  (uint8_t  val);
void SET_iolpCMD_T4  (uint8_t  val);
void SET_iolpCMD_T5  (uint8_t  val);
void SET_iolpCMD_T6  (uint8_t  val);
void SET_iolpCMD_T7  (uint8_t  val);

void SET_diagCMD_vr  (uint16_t val);
void SET_diagCMD_hv  (uint16_t val);
void SET_diagCMD_ec  (uint16_t val);
void SET_diagCMD_ea  (uint16_t val);
void SET_diagCMD_dd  (uint32_t val);
void SET_diagCMD_db  (uint8_t  val);

void SET_sleepmCMD_st(uint16_t val);
void SET_sleepmCMD_sp(uint16_t val);
void SET_sleepmCMD_dp(uint16_t val);
void SET_atcopCMD_ct (uint16_t val);
void SET_atcopCMD_gt (uint16_t val);
void SET_rfiCMD_ca   (uint8_t  val);
void SET_netCMD_ch   (uint8_t  val);
void SET_netCMD_ai   (uint8_t  val);
void SET_netCMD_a1   (uint8_t  val);
void SET_netCMD_a2   (uint8_t  val);
void SET_netCMD_mm   (uint8_t  val);
void SET_ioserCMD_d8 (uint8_t  val);
void SET_ioserCMD_d7 (uint8_t  val);
void SET_ioserCMD_d6 (uint8_t  val);
void SET_ioserCMD_d5 (uint8_t  val);
void SET_ioserCMD_d4 (uint8_t  val);
void SET_ioserCMD_d3 (uint8_t  val);
void SET_ioserCMD_d2 (uint8_t  val);
void SET_ioserCMD_d1 (uint8_t  val);
void SET_ioserCMD_d0 (uint8_t  val);
void SET_serintCMD_bd(uint8_t  val);
void SET_serintCMD_nb(uint8_t  val);
void SET_sleepmCMD_so(uint8_t  val);
void SET_sleepmCMD_sm(uint8_t  val);
void SET_rfiCMD_pl   (uint8_t  val);
void SET_netCMD_sd   (uint8_t  val);
void SET_netCMD_rr   (uint8_t  val);
void SET_netCMD_rn   (uint8_t  val);
void SET_ioserCMD_p0 (uint8_t  val);
void SET_ioserCMD_p1 (uint8_t  val);
void SET_serintCMD_ap(uint8_t  val);
void SET_ioserCMD_iu (bool_t   val);
void SET_netCMD_ce   (bool_t   val);
void SET_netCMD_no   (bool_t   val);
void SET_secCMD_ee   (bool_t   val);

void SET_atAP_tmp    (void* APvalue, uint8_t len);	// defined in atlocal.c
void SET_atCT_tmp    (void* CTvalue, uint8_t len);	// defined in atlocal.c

// === get functions ==========================================
size_t   GET_device_tSize(void);
uint8_t	 *GET_netCMD_ni(void);
uint8_t  *GET_secCMD_ky(void);
uint8_t  GET_serintCMD_ro(void);
uint8_t  GET_atcopCMD_cc(void); 
uint64_t GET_iolpCMD_ia(void);

uint16_t GET_netCMD_id(void);
uint16_t GET_netCMD_my(void);
uint8_t  GET_netCMD_nt(void);
uint16_t GET_netCMD_sc(void);
uint32_t GET_netCMD_dh(void);
uint32_t GET_netCMD_dl(void);
uint32_t GET_netCMD_sh(void);
uint32_t GET_netCMD_sl(void);

uint16_t GET_ioserCMD_ir(void);
uint8_t  GET_ioserCMD_pr(void);
uint8_t  GET_ioserCMD_it(void);
uint8_t  GET_ioserCMD_ic(void);
uint8_t  GET_ioserCMD_pt(void);
uint8_t  GET_ioserCMD_rp(void);

uint8_t  GET_iolpCMD_T0(void);
uint8_t  GET_iolpCMD_T1(void);
uint8_t  GET_iolpCMD_T2(void);
uint8_t  GET_iolpCMD_T3(void);
uint8_t  GET_iolpCMD_T4(void);
uint8_t  GET_iolpCMD_T5(void);
uint8_t  GET_iolpCMD_T6(void);
uint8_t  GET_iolpCMD_T7(void);

uint16_t GET_diagCMD_vr(void);
uint16_t GET_diagCMD_hv(void);
uint16_t GET_diagCMD_ec(void);
uint16_t GET_diagCMD_ea(void);
uint32_t GET_diagCMD_dd(void);
uint8_t  GET_diagCMD_db(void);

uint16_t GET_sleepmCMD_st(void);
uint16_t GET_sleepmCMD_sp(void);
uint16_t GET_sleepmCMD_dp(void);
uint16_t GET_atcopCMD_ct(void);
uint16_t GET_atcopCMD_gt(void);
uint8_t  GET_rfiCMD_ca(void);
uint8_t  GET_netCMD_ch(void);
uint8_t  GET_netCMD_ai(void);
uint8_t  GET_netCMD_a1(void);
uint8_t  GET_netCMD_a2(void);
uint8_t	 GET_netCMD_mm(void);
uint8_t  GET_ioserCMD_d8(void);
uint8_t  GET_ioserCMD_d7(void);
uint8_t  GET_ioserCMD_d6(void);
uint8_t  GET_ioserCMD_d5(void);
uint8_t  GET_ioserCMD_d4(void);
uint8_t  GET_ioserCMD_d3(void);
uint8_t  GET_ioserCMD_d2(void);
uint8_t  GET_ioserCMD_d1(void);
uint8_t  GET_ioserCMD_d0(void);
uint8_t  GET_serintCMD_bd(void);
uint8_t  GET_serintCMD_nb(void);
uint8_t  GET_sleepmCMD_so(void);
uint8_t  GET_sleepmCMD_sm(void);
uint8_t  GET_rfiCMD_pl(void);
uint8_t  GET_netCMD_sd(void);
uint8_t  GET_netCMD_rr(void);
uint8_t  GET_netCMD_rn(void);
uint8_t  GET_ioserCMD_p0(void);
uint8_t  GET_ioserCMD_p1(void);
uint8_t  GET_serintCMD_ap(void);
bool_t   GET_ioserCMD_iu(void); 
bool_t	 GET_netCMD_ce(void);
bool_t   GET_netCMD_no(void);
bool_t   GET_secCMD_ee(void);

uint8_t GET_atAP_tmp(void);			// defined in atlocal.c
uint16_t GET_atCT_tmp(void);		// defined in atlocal.c

// === general functions ======================================
void deviceMemcpy(uint8_t *arrayStart, bool_t from_device_t);

// === EEPROM functions =======================================
void GET_allFromEEPROM(void);
void SET_defaultInEEPROM(void);
void SET_userValInEEPROM(void);

#endif /* RFMODUL_H_ */