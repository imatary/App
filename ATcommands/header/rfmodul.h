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
void SET_iolpCMD_T0  (void *val, size_t len);
void SET_iolpCMD_T1  (void *val, size_t len);
void SET_iolpCMD_T2  (void *val, size_t len);
void SET_iolpCMD_T3  (void *val, size_t len);
void SET_iolpCMD_T4  (void *val, size_t len);
void SET_iolpCMD_T5  (void *val, size_t len);
void SET_iolpCMD_T6  (void *val, size_t len);
void SET_iolpCMD_T7  (void *val, size_t len);

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

uint8_t	 *GET_netCMD_ni  (void);
uint8_t  *GET_secCMD_ky  (void);

bool_t   GET_secCMD_ee   (void);

uint16_t GET_netCMD_id   (void);
uint16_t GET_netCMD_my   (void);
uint8_t  GET_netCMD_nt   (void);
uint16_t GET_netCMD_sc   (void);
uint32_t GET_netCMD_dh   (void);
uint32_t GET_netCMD_dl   (void);
uint32_t GET_netCMD_sh   (void);
uint32_t GET_netCMD_sl   (void);
uint8_t  GET_netCMD_ch   (void);
uint8_t  GET_netCMD_ai   (void);
uint8_t  GET_netCMD_a1   (void);
uint8_t  GET_netCMD_a2   (void);
uint8_t	 GET_netCMD_mm   (void);
bool_t	 GET_netCMD_ce   (void);
bool_t   GET_netCMD_no   (void);
uint8_t  GET_netCMD_sd   (void);
uint8_t  GET_netCMD_rr   (void);
uint8_t  GET_netCMD_rn   (void);

uint8_t  GET_serintCMD_ro(void);
uint8_t  GET_serintCMD_ap(void);
uint8_t  GET_serintCMD_bd(void);
uint8_t  GET_serintCMD_nb(void);
 
uint16_t GET_ioserCMD_ir (void);
uint8_t  GET_ioserCMD_pr (void);
uint8_t  GET_ioserCMD_it (void);
uint8_t  GET_ioserCMD_ic (void);
uint8_t  GET_ioserCMD_pt (void);
uint8_t  GET_ioserCMD_rp (void);
uint8_t  GET_ioserCMD_d8 (void);
uint8_t  GET_ioserCMD_d7 (void);
uint8_t  GET_ioserCMD_d6 (void);
uint8_t  GET_ioserCMD_d5 (void);
uint8_t  GET_ioserCMD_d4 (void);
uint8_t  GET_ioserCMD_d3 (void);
uint8_t  GET_ioserCMD_d2 (void);
uint8_t  GET_ioserCMD_d1 (void);
uint8_t  GET_ioserCMD_d0 (void);
uint8_t  GET_ioserCMD_p0 (void);
uint8_t  GET_ioserCMD_p1 (void);
bool_t   GET_ioserCMD_iu (void); 

uint64_t GET_iolpCMD_ia  (void);
uint8_t  GET_iolpCMD_T0  (void);
uint8_t  GET_iolpCMD_T1  (void);
uint8_t  GET_iolpCMD_T2  (void);
uint8_t  GET_iolpCMD_T3  (void);
uint8_t  GET_iolpCMD_T4  (void);
uint8_t  GET_iolpCMD_T5  (void);
uint8_t  GET_iolpCMD_T6  (void);
uint8_t  GET_iolpCMD_T7  (void);

uint16_t GET_diagCMD_vr  (void);
uint16_t GET_diagCMD_hv  (void);
uint16_t GET_diagCMD_ec  (void);
uint16_t GET_diagCMD_ea  (void);
uint32_t GET_diagCMD_dd  (void);
uint8_t  GET_diagCMD_db  (void);

uint16_t GET_sleepmCMD_st(void);
uint16_t GET_sleepmCMD_sp(void);
uint16_t GET_sleepmCMD_dp(void);
uint8_t  GET_sleepmCMD_so(void);
uint8_t  GET_sleepmCMD_sm(void);

uint8_t  GET_atcopCMD_cc (void);
uint16_t GET_atcopCMD_ct (void);
uint16_t GET_atcopCMD_gt (void);

uint8_t  GET_rfiCMD_ca   (void);
uint8_t  GET_rfiCMD_pl   (void);

uint8_t  GET_atAP_tmp    (void);
uint16_t GET_atCT_tmp    (void);

// === general functions ======================================
void deviceMemcpy(uint8_t *arrayStart, bool_t from_device_t);

// === EEPROM functions =======================================
void GET_allFromEEPROM(void);
void SET_defaultInEEPROM(void);
void SET_userValInEEPROM(void);

#endif /* RFMODUL_H_ */