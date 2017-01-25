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

void SET_netCMD_sh   (uint32_t val); // only for EEPROM
void SET_netCMD_sl   (uint32_t val); // only for EEPROM
void SET_diagCMD_ec  (uint16_t val);
void SET_diagCMD_ea  (uint16_t val);
void SET_atcopCMD_ct (uint16_t val);
void SET_serintCMD_ap(uint8_t  val);
void SET_diagCMD_db  (uint8_t  val);
void SET_netCMD_ai   (uint8_t  val);

// === get functions ==========================================
size_t   GET_device_tSize(void);

size_t GET_offsetof_ni(void);
size_t GET_offsetof_ky(void);
size_t GET_offsetof_ro(void);
size_t GET_offsetof_cc(void);
size_t GET_offsetof_ia(void);
size_t GET_offsetof_id(void);
size_t GET_offsetof_my(void);
size_t GET_offsetof_nt(void);
size_t GET_offsetof_sc(void);
size_t GET_offsetof_dh(void);
size_t GET_offsetof_dl(void);
size_t GET_offsetof_sh(void);
size_t GET_offsetof_sl(void);
size_t GET_offsetof_ir(void);
size_t GET_offsetof_pr(void);
size_t GET_offsetof_it(void);
size_t GET_offsetof_ic(void);
size_t GET_offsetof_pt(void);
size_t GET_offsetof_rp(void);
size_t GET_offsetof_t0(void);
size_t GET_offsetof_t1(void);
size_t GET_offsetof_t2(void);
size_t GET_offsetof_t3(void);
size_t GET_offsetof_t4(void);
size_t GET_offsetof_t5(void);
size_t GET_offsetof_t6(void);
size_t GET_offsetof_t7(void);
size_t GET_offsetof_vr(void);
size_t GET_offsetof_hv(void);
size_t GET_offsetof_ec(void);
size_t GET_offsetof_ea(void);
size_t GET_offsetof_dd(void);
size_t GET_offsetof_db(void);
size_t GET_offsetof_st(void);
size_t GET_offsetof_sp(void);
size_t GET_offsetof_dp(void);
size_t GET_offsetof_ct(void);
size_t GET_offsetof_gt(void);
size_t GET_offsetof_ca(void);
size_t GET_offsetof_ch(void);
size_t GET_offsetof_ai(void);
size_t GET_offsetof_a1(void);
size_t GET_offsetof_a2(void);
size_t GET_offsetof_mm(void);
size_t GET_offsetof_d8(void);
size_t GET_offsetof_d7(void);
size_t GET_offsetof_d6(void);
size_t GET_offsetof_d5(void);
size_t GET_offsetof_d4(void);
size_t GET_offsetof_d3(void);
size_t GET_offsetof_d2(void);
size_t GET_offsetof_d1(void);
size_t GET_offsetof_d0(void);
size_t GET_offsetof_bd(void);
size_t GET_offsetof_nb(void);
size_t GET_offsetof_so(void);
size_t GET_offsetof_sm(void);
size_t GET_offsetof_pl(void);
size_t GET_offsetof_sd(void);
size_t GET_offsetof_rr(void);
size_t GET_offsetof_rn(void);
size_t GET_offsetof_p0(void);
size_t GET_offsetof_p1(void);
size_t GET_offsetof_ap(void);
size_t GET_offsetof_iu(void);
size_t GET_offsetof_ce(void);
size_t GET_offsetof_no(void);
size_t GET_offsetof_ee(void);

uint8_t  GET_atAP_tmp(void);
uint16_t GET_atCT_tmp(void);

uint16_t GET_serintCMD_ap(void);

// === general functions ======================================
void deviceMemcpy(uint8_t *arrayStart, bool_t from_device_t);

// === EEPROM functions =======================================
void GET_allFromEEPROM(void);
void SET_defaultInEEPROM(void);
void SET_userValInEEPROM(void);

#endif /* RFMODUL_H_ */