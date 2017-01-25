/*
 * setter.c
 *
 * Created: 26.10.2016 13:36:35
 *  Author: TOE
 */ 
#include <string.h>
#include <stddef.h>
#include <avr/eeprom.h>

#include "../header/_global.h"			// RFmodul struct
#include "../header/defaultConfig.h"	// defines for default configuration
#include "../header/rfmodul.h"			// prototypes

// === struct =============================================
typedef struct dev {
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

	uint8_t  iolpCMD_T0;        // max 0xFF                                            
	uint8_t  iolpCMD_T1;        // max 0xFF                                            
	uint8_t  iolpCMD_T2;        // max 0xFF                                            
	uint8_t  iolpCMD_T3;        // max 0xFF                                            
	uint8_t  iolpCMD_T4;        // max 0xFF                                            
	uint8_t  iolpCMD_T5;        // max 0xFF                                            
	uint8_t  iolpCMD_T6;        // max 0xFF                                            
	uint8_t  iolpCMD_T7;        // max 0xFF                                            

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

// === globals ============================================
static device_t RFmodul;
static size_t   device_tSize = sizeof(device_t);

static uint8_t atAP_tmp = 0;
static uint16_t atCT_tmp = 0;

// === set functions ======================================
/*
 * Set all AT command parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/30
 */
 void SET_allDefault()
 {
	 uint16_t tmp;
	 RFmodul.netCMD_ch = CH_CHANNEL;
	 RFmodul.netCMD_id = ID_PANID;
	 RFmodul.netCMD_dh = DH_DEST_HIGH;
	 RFmodul.netCMD_dl = DL_DEST_LOW;
	 RFmodul.netCMD_my = MY_SHORT_ADDR;
	 RFmodul.netCMD_ce = CE_COORDINATOR_ENABLE;
	 RFmodul.netCMD_sc = SC_SCAN_CHANNELS;
	 strcpy(RFmodul.netCMD_ni,  NI_NODE_IDENTIFY);
	 RFmodul.netCMD_mm = MM_MAC_MODE;
	 RFmodul.netCMD_rr = RR_XBEE_RETRIES;
	 RFmodul.netCMD_rn = RN_RANDOM_DELAY_SLOTS;
	 RFmodul.netCMD_nt = NT_NODE_DISCOVER_TIME;
	 RFmodul.netCMD_no = NO_NODE_DISCOVER_OPTION;
	 RFmodul.netCMD_sd = SD_SCAN_DURATION;
	 RFmodul.netCMD_a1 = A1_END_DEVICE_ASSOCIATION;
	 RFmodul.netCMD_a2 = A2_COORDINATOR_ASSOCIATION;
	 RFmodul.netCMD_ai = AI_ASSOCIATION_INDICATION;
	 
	 RFmodul.secCMD_ee = EE_AES_ECRYPTION_ENABLE;
	 
	 RFmodul.rfiCMD_pl = PL_POWER_LEVEL;
	 RFmodul.rfiCMD_ca = CA_CCA_TRESHOLD;
	 
	 RFmodul.sleepmCMD_sm = SM_SLEEP_MODE;
	 RFmodul.sleepmCMD_st = ST_TIME_BEFORE_SLEEP;
	 RFmodul.sleepmCMD_sp = SP_CYCLIC_SLEEP_PERIOD;
	 RFmodul.sleepmCMD_dp = DP_DISASSOCIATED_SP;
	 RFmodul.sleepmCMD_so = SO_SLEEP_OPTION;
	 
	 RFmodul.serintCMD_bd = BD_INTERFACE_DATA_RATE;
	 RFmodul.serintCMD_nb = NB_PARITY;
	 RFmodul.serintCMD_ro = RO_PACKETIZATION_TIMEOUT;
	 tmp = AP_AP_ENABLE;
	 SET_atAP_tmp( &tmp, 1);
	 
	 RFmodul.ioserCMD_d8 = D8_DI8_CONFIGURATION;
	 RFmodul.ioserCMD_d7 = D7_DIO7_CONFIGURATION;
	 RFmodul.ioserCMD_d6 = D6_DIO6_CONFIGURATION;
	 RFmodul.ioserCMD_d5 = D5_DIO5_CONFIGURATION;
	 RFmodul.ioserCMD_d4 = D4_DIO4_CONFIGURATION;
	 RFmodul.ioserCMD_d3 = D3_DIO3_CONFIGURATION;
	 RFmodul.ioserCMD_d2 = D2_DIO2_CONFIGURATION;
	 RFmodul.ioserCMD_d1 = D1_DIO1_CONFIGURATION;
	 RFmodul.ioserCMD_d0 = D0_DIO0_CONFIGURATION;
	 RFmodul.ioserCMD_pr = PR_PULLUP_RESISTOR_ENABLE;
	 RFmodul.ioserCMD_iu = IU_IO_OUTPUT_ENABLE;
	 RFmodul.ioserCMD_it = IT_SAMPLES_BEFORE_TX;
	 RFmodul.ioserCMD_ic = IC_DIO_CHANGE_DETECT;
	 RFmodul.ioserCMD_ir = IR_SAPLE_RATE;
	 RFmodul.ioserCMD_p0 = P0_PWM0_CONFIGURATION;
	 RFmodul.ioserCMD_p1 = P1_PWM1_CONFIGURATION;
	 RFmodul.ioserCMD_pt = PT_PWM_OUTPUT_TIMEOUT;
	 RFmodul.ioserCMD_rp = RP_RSSI_PWM_TIMER;			
	 
	 RFmodul.iolpCMD_ia = IA_IO_INPUT_ADDRESS;
	 RFmodul.iolpCMD_T0 = T0_D0_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_T1 = T1_D1_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_T2 = T2_D2_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_T3 = T3_D3_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_T4 = T4_D4_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_T5 = T5_D5_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_T6 = T6_D6_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_T7 = T7_D7_OUTPUT_TIMEOUT;

	 RFmodul.diagCMD_vr = VR_FIRMWARE_VERS;
	 RFmodul.diagCMD_hv = HV_HARDWARE_VERS;
	 RFmodul.diagCMD_db = DB_RECEIVED_SIGNAL_STRENGTH;
	 RFmodul.diagCMD_ec = EC_CCA_FAILURES;
	 RFmodul.diagCMD_ea = EA_ACK_FAILURES;
	 RFmodul.diagCMD_dd = DD_DEVICE_TYPE_IDENTIFIER;
	 
	 tmp =	CT_AT_CMD_TIMEOUT;		    
	 SET_atCT_tmp(&tmp, 2);
	 RFmodul.atcopCMD_gt = GT_GUART_TIMES;
	 RFmodul.atcopCMD_cc = CC_COMMAND_SEQUENCE_CHAR;
 }
 
 /*
 * Set parameter values
 *
 * Received:
 *		void		pointer to data
 *		uint8_t		length of data
 *
 * Returns:
 *		nothing
 *
 * Last modified: 2016/01/19
 */
void SET_netCMD_ni   (void *val, size_t len) { memcpy(RFmodul.netCMD_ni   , val, len); }
void SET_secCMD_ky   (void *val, size_t len) { memcpy(RFmodul.secCMD_ky   , val, len); }
void SET_serintCMD_ro(void *val, size_t len) { memcpy(&RFmodul.serintCMD_ro, val, len); }
void SET_atcopCMD_cc (void *val, size_t len) { memcpy(&RFmodul.atcopCMD_cc , val, len); }
void SET_iolpCMD_ia  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_ia  , val, len); }
void SET_netCMD_id   (void *val, size_t len) { memcpy(&RFmodul.netCMD_id   , val, len); }
void SET_netCMD_my   (void *val, size_t len) { memcpy(&RFmodul.netCMD_my   , val, len); }
void SET_netCMD_nt   (void *val, size_t len) { memcpy(&RFmodul.netCMD_nt   , val, len); }
void SET_netCMD_sc   (void *val, size_t len) { memcpy(&RFmodul.netCMD_sc   , val, len); }
void SET_netCMD_dh   (void *val, size_t len) { memcpy(&RFmodul.netCMD_dh   , val, len); }
void SET_netCMD_dl   (void *val, size_t len) { memcpy(&RFmodul.netCMD_dl   , val, len); }
void SET_ioserCMD_ir (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_ir , val, len); }
void SET_ioserCMD_pr (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_pr , val, len); }
void SET_ioserCMD_it (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_it , val, len); }
void SET_ioserCMD_ic (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_ic , val, len); }
void SET_ioserCMD_pt (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_pt , val, len); }
void SET_ioserCMD_rp (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_rp , val, len); }
void SET_iolpCMD_t0  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_T0  , val, len); }
void SET_iolpCMD_t1  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_T1  , val, len); }
void SET_iolpCMD_t2  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_T2  , val, len); }
void SET_iolpCMD_t3  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_T3  , val, len); }
void SET_iolpCMD_t4  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_T4  , val, len); }
void SET_iolpCMD_t5  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_T5  , val, len); }
void SET_iolpCMD_t6  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_T6  , val, len); }
void SET_iolpCMD_t7  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_T7  , val, len); }
void SET_diagCMD_vr  (void *val, size_t len) { memcpy(&RFmodul.diagCMD_vr  , val, len); }
void SET_diagCMD_hv  (void *val, size_t len) { memcpy(&RFmodul.diagCMD_hv  , val, len); }
void SET_sleepmCMD_st(void *val, size_t len) { memcpy(&RFmodul.sleepmCMD_st, val, len); }
void SET_sleepmCMD_sp(void *val, size_t len) { memcpy(&RFmodul.sleepmCMD_sp, val, len); }
void SET_sleepmCMD_dp(void *val, size_t len) { memcpy(&RFmodul.sleepmCMD_dp, val, len); }
void SET_atcopCMD_gt (void *val, size_t len) { memcpy(&RFmodul.atcopCMD_gt , val, len); }
void SET_rfiCMD_ca   (void *val, size_t len) { memcpy(&RFmodul.rfiCMD_ca   , val, len); }
void SET_netCMD_ch   (void *val, size_t len) { memcpy(&RFmodul.netCMD_ch   , val, len); }
void SET_netCMD_a1   (void *val, size_t len) { memcpy(&RFmodul.netCMD_a1   , val, len); }
void SET_netCMD_a2   (void *val, size_t len) { memcpy(&RFmodul.netCMD_a2   , val, len); }
void SET_netCMD_mm   (void *val, size_t len) { memcpy(&RFmodul.netCMD_mm   , val, len); }
void SET_ioserCMD_d8 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d8 , val, len); }
void SET_ioserCMD_d7 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d7 , val, len); }
void SET_ioserCMD_d6 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d6 , val, len); }
void SET_ioserCMD_d5 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d5 , val, len); }
void SET_ioserCMD_d4 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d4 , val, len); }
void SET_ioserCMD_d3 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d3 , val, len); }
void SET_ioserCMD_d2 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d2 , val, len); }
void SET_ioserCMD_d1 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d1 , val, len); }
void SET_ioserCMD_d0 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_d0 , val, len); }
void SET_serintCMD_bd(void *val, size_t len) { memcpy(&RFmodul.serintCMD_bd, val, len); }
void SET_serintCMD_nb(void *val, size_t len) { memcpy(&RFmodul.serintCMD_nb, val, len); }
void SET_sleepmCMD_so(void *val, size_t len) { memcpy(&RFmodul.sleepmCMD_so, val, len); }
void SET_sleepmCMD_sm(void *val, size_t len) { memcpy(&RFmodul.sleepmCMD_sm, val, len); }
void SET_rfiCMD_pl   (void *val, size_t len) { memcpy(&RFmodul.rfiCMD_pl   , val, len); }
void SET_netCMD_sd   (void *val, size_t len) { memcpy(&RFmodul.netCMD_sd   , val, len); }
void SET_netCMD_rr   (void *val, size_t len) { memcpy(&RFmodul.netCMD_rr   , val, len); }
void SET_netCMD_rn   (void *val, size_t len) { memcpy(&RFmodul.netCMD_rn   , val, len); }
void SET_ioserCMD_p0 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_p0 , val, len); }
void SET_ioserCMD_p1 (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_p1 , val, len); }
void SET_ioserCMD_iu (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_iu , val, len); }
void SET_netCMD_ce   (void *val, size_t len) { memcpy(&RFmodul.netCMD_ce   , val, len); }
void SET_netCMD_no   (void *val, size_t len) { memcpy(&RFmodul.netCMD_no   , val, len); }
void SET_secCMD_ee   (void *val, size_t len) { memcpy(&RFmodul.secCMD_ee   , val, len); }
void SET_diagCMD_dd  (void *val, size_t len) { memcpy(&RFmodul.diagCMD_dd  , val, len); } 
void SET_atCT_tmp    (void *val, size_t len) { memcpy(&atCT_tmp            , val, len); }
void SET_atAP_tmp    (void *val, size_t len) { memcpy(&atAP_tmp            , val, len); }

void SET_netCMD_sh   (uint32_t val) { RFmodul.netCMD_sh    = val; } // only for EEPROM
void SET_netCMD_sl   (uint32_t val) { RFmodul.netCMD_sl    = val; } // only for EEPROM
void SET_diagCMD_ec  (uint16_t val) { RFmodul.diagCMD_ec   = val; }
void SET_diagCMD_ea  (uint16_t val) { RFmodul.diagCMD_ea   = val; }
void SET_atcopCMD_ct (uint16_t val) { RFmodul.atcopCMD_ct  = val; }
void SET_serintCMD_ap(uint8_t  val) { RFmodul.serintCMD_ap = val; }
void SET_diagCMD_db  (uint8_t  val) { RFmodul.diagCMD_db   = val; }
void SET_netCMD_ai   (uint8_t  val) { RFmodul.netCMD_ai    = val; }
	
// === get functions ======================================
/*
 * Get AT command parameter
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/01/19
 */

size_t GET_device_tSize(void) { return device_tSize; }
	
size_t GET_offsetof_ni(void) { return offsetof( device_t, netCMD_ni    ); }
size_t GET_offsetof_ky(void) { return offsetof( device_t, secCMD_ky    ); }
size_t GET_offsetof_ro(void) { return offsetof( device_t, serintCMD_ro ); }
size_t GET_offsetof_cc(void) { return offsetof( device_t, atcopCMD_cc  ); }
size_t GET_offsetof_ia(void) { return offsetof( device_t, iolpCMD_ia   ); }
size_t GET_offsetof_id(void) { return offsetof( device_t, netCMD_id    ); }
size_t GET_offsetof_my(void) { return offsetof( device_t, netCMD_my    ); }
size_t GET_offsetof_nt(void) { return offsetof( device_t, netCMD_nt    ); }
size_t GET_offsetof_sc(void) { return offsetof( device_t, netCMD_sc    ); }
size_t GET_offsetof_dh(void) { return offsetof( device_t, netCMD_dh    ); }
size_t GET_offsetof_dl(void) { return offsetof( device_t, netCMD_dl    ); }
size_t GET_offsetof_sh(void) { return offsetof( device_t, netCMD_sh    ); }
size_t GET_offsetof_sl(void) { return offsetof( device_t, netCMD_sl    ); }
size_t GET_offsetof_ir(void) { return offsetof( device_t, ioserCMD_ir  ); }
size_t GET_offsetof_pr(void) { return offsetof( device_t, ioserCMD_pr  ); }
size_t GET_offsetof_it(void) { return offsetof( device_t, ioserCMD_it  ); }
size_t GET_offsetof_ic(void) { return offsetof( device_t, ioserCMD_ic  ); }
size_t GET_offsetof_pt(void) { return offsetof( device_t, ioserCMD_pt  ); }
size_t GET_offsetof_rp(void) { return offsetof( device_t, ioserCMD_rp  ); }
size_t GET_offsetof_t0(void) { return offsetof( device_t, iolpCMD_T0   ); }
size_t GET_offsetof_t1(void) { return offsetof( device_t, iolpCMD_T1   ); }
size_t GET_offsetof_t2(void) { return offsetof( device_t, iolpCMD_T2   ); }
size_t GET_offsetof_t3(void) { return offsetof( device_t, iolpCMD_T3   ); }
size_t GET_offsetof_t4(void) { return offsetof( device_t, iolpCMD_T4   ); }
size_t GET_offsetof_t5(void) { return offsetof( device_t, iolpCMD_T5   ); }
size_t GET_offsetof_t6(void) { return offsetof( device_t, iolpCMD_T6   ); }
size_t GET_offsetof_t7(void) { return offsetof( device_t, iolpCMD_T7   ); }
size_t GET_offsetof_vr(void) { return offsetof( device_t, diagCMD_vr   ); }
size_t GET_offsetof_hv(void) { return offsetof( device_t, diagCMD_hv   ); }
size_t GET_offsetof_ec(void) { return offsetof( device_t, diagCMD_ec   ); }
size_t GET_offsetof_ea(void) { return offsetof( device_t, diagCMD_ea   ); }
size_t GET_offsetof_dd(void) { return offsetof( device_t, diagCMD_dd   ); }
size_t GET_offsetof_db(void) { return offsetof( device_t, diagCMD_db   ); }
size_t GET_offsetof_st(void) { return offsetof( device_t, sleepmCMD_st ); }
size_t GET_offsetof_sp(void) { return offsetof( device_t, sleepmCMD_sp ); }
size_t GET_offsetof_dp(void) { return offsetof( device_t, sleepmCMD_dp ); }
size_t GET_offsetof_ct(void) { return offsetof( device_t, atcopCMD_ct  ); }
size_t GET_offsetof_gt(void) { return offsetof( device_t, atcopCMD_gt  ); }
size_t GET_offsetof_ca(void) { return offsetof( device_t, rfiCMD_ca    ); }
size_t GET_offsetof_ch(void) { return offsetof( device_t, netCMD_ch    ); }
size_t GET_offsetof_ai(void) { return offsetof( device_t, netCMD_ai    ); }
size_t GET_offsetof_a1(void) { return offsetof( device_t, netCMD_a1    ); }
size_t GET_offsetof_a2(void) { return offsetof( device_t, netCMD_a2    ); }
size_t GET_offsetof_mm(void) { return offsetof( device_t, netCMD_mm    ); }
size_t GET_offsetof_d8(void) { return offsetof( device_t, ioserCMD_d8  ); }
size_t GET_offsetof_d7(void) { return offsetof( device_t, ioserCMD_d7  ); }
size_t GET_offsetof_d6(void) { return offsetof( device_t, ioserCMD_d6  ); }
size_t GET_offsetof_d5(void) { return offsetof( device_t, ioserCMD_d5  ); }
size_t GET_offsetof_d4(void) { return offsetof( device_t, ioserCMD_d4  ); }
size_t GET_offsetof_d3(void) { return offsetof( device_t, ioserCMD_d3  ); }
size_t GET_offsetof_d2(void) { return offsetof( device_t, ioserCMD_d2  ); }
size_t GET_offsetof_d1(void) { return offsetof( device_t, ioserCMD_d1  ); }
size_t GET_offsetof_d0(void) { return offsetof( device_t, ioserCMD_d0  ); }
size_t GET_offsetof_bd(void) { return offsetof( device_t, serintCMD_bd ); }
size_t GET_offsetof_nb(void) { return offsetof( device_t, serintCMD_nb ); }
size_t GET_offsetof_so(void) { return offsetof( device_t, sleepmCMD_so ); }
size_t GET_offsetof_sm(void) { return offsetof( device_t, sleepmCMD_sm ); }
size_t GET_offsetof_pl(void) { return offsetof( device_t, rfiCMD_pl    ); }
size_t GET_offsetof_sd(void) { return offsetof( device_t, netCMD_sd    ); }
size_t GET_offsetof_rr(void) { return offsetof( device_t, netCMD_rr    ); }
size_t GET_offsetof_rn(void) { return offsetof( device_t, netCMD_rn    ); }
size_t GET_offsetof_p0(void) { return offsetof( device_t, ioserCMD_p0  ); }
size_t GET_offsetof_p1(void) { return offsetof( device_t, ioserCMD_p1  ); }
size_t GET_offsetof_ap(void) { return offsetof( device_t, serintCMD_ap ); }
size_t GET_offsetof_iu(void) { return offsetof( device_t, ioserCMD_iu  ); }
size_t GET_offsetof_ce(void) { return offsetof( device_t, netCMD_ce    ); }
size_t GET_offsetof_no(void) { return offsetof( device_t, netCMD_no    ); }
size_t GET_offsetof_ee(void) { return offsetof( device_t, secCMD_ee    ); }

// TODO -> insert get ai = RFmodul.netCMD_a1 & RFmodul.netCMD_a2;

uint8_t  GET_atAP_tmp(void) { return atAP_tmp; }
uint16_t GET_atCT_tmp(void) { return atCT_tmp; }
// === general functions ==================================
void deviceMemcpy(uint8_t *arrayStart, bool_t from_device_t)
{
	if ( TRUE == from_device_t ) memcpy( arrayStart, &RFmodul, sizeof(device_t) );
	else                         memcpy( &RFmodul, arrayStart, sizeof(device_t) );
}