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
#include "../header/cmd.h"

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
	 atAP_tmp             = AP_AP_ENABLE;

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
	 RFmodul.iolpCMD_t0 = T0_D0_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_t1 = T1_D1_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_t2 = T2_D2_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_t3 = T3_D3_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_t4 = T4_D4_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_t5 = T5_D5_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_t6 = T6_D6_OUTPUT_TIMEOUT;
	 RFmodul.iolpCMD_t7 = T7_D7_OUTPUT_TIMEOUT;

	 RFmodul.diagCMD_vr = VR_FIRMWARE_VERS;
	 RFmodul.diagCMD_hv = HV_HARDWARE_VERS;
	 RFmodul.diagCMD_db = DB_RECEIVED_SIGNAL_STRENGTH;
	 RFmodul.diagCMD_ec = EC_CCA_FAILURES;
	 RFmodul.diagCMD_ea = EA_ACK_FAILURES;
	 RFmodul.diagCMD_dd = DD_DEVICE_TYPE_IDENTIFIER;

	 atCT_tmp            = CT_AT_CMD_TIMEOUT;
	 RFmodul.atcopCMD_gt = GT_GUART_TIMES;
	 RFmodul.atcopCMD_cc = CC_COMMAND_SEQUENCE_CHAR;

	 dirtyBits = 0x68;
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
void SET_netCMD_ni   (void *val, size_t len) { memcpy( RFmodul.netCMD_ni   , val, len); }
void SET_secCMD_ky   (void *val, size_t len) { memcpy( RFmodul.secCMD_ky   , val, len); }
void SET_serintCMD_ro(void *val, size_t len) { memcpy(&RFmodul.serintCMD_ro, val, len); }
void SET_atcopCMD_cc (void *val, size_t len) { memcpy(&RFmodul.atcopCMD_cc , val, len); }
void SET_iolpCMD_ia  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_ia  , val, len); }
void SET_netCMD_nt   (void *val, size_t len) { memcpy(&RFmodul.netCMD_nt   , val, len); }
void SET_netCMD_sc   (void *val, size_t len) { memcpy(&RFmodul.netCMD_sc   , val, len); }
void SET_ioserCMD_ir (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_ir , val, len); }
void SET_ioserCMD_pr (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_pr , val, len); }
void SET_ioserCMD_it (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_it , val, len); }
void SET_ioserCMD_ic (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_ic , val, len); }
void SET_ioserCMD_pt (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_pt , val, len); }
void SET_ioserCMD_rp (void *val, size_t len) { memcpy(&RFmodul.ioserCMD_rp , val, len); }
void SET_iolpCMD_t0  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_t0  , val, len); }
void SET_iolpCMD_t1  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_t1  , val, len); }
void SET_iolpCMD_t2  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_t2  , val, len); }
void SET_iolpCMD_t3  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_t3  , val, len); }
void SET_iolpCMD_t4  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_t4  , val, len); }
void SET_iolpCMD_t5  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_t5  , val, len); }
void SET_iolpCMD_t6  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_t6  , val, len); }
void SET_iolpCMD_t7  (void *val, size_t len) { memcpy(&RFmodul.iolpCMD_t7  , val, len); }
void SET_diagCMD_vr  (void *val, size_t len) { memcpy(&RFmodul.diagCMD_vr  , val, len); }
void SET_diagCMD_hv  (void *val, size_t len) { memcpy(&RFmodul.diagCMD_hv  , val, len); }
void SET_sleepmCMD_st(void *val, size_t len) { memcpy(&RFmodul.sleepmCMD_st, val, len); }
void SET_sleepmCMD_sp(void *val, size_t len) { memcpy(&RFmodul.sleepmCMD_sp, val, len); }
void SET_sleepmCMD_dp(void *val, size_t len) { memcpy(&RFmodul.sleepmCMD_dp, val, len); }
void SET_atcopCMD_gt (void *val, size_t len) { memcpy(&RFmodul.atcopCMD_gt , val, len); }
void SET_rfiCMD_ca   (void *val, size_t len) { memcpy(&RFmodul.rfiCMD_ca   , val, len); }
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

void SET_netCMD_id   (void *val, size_t len)
{
	memcpy(&RFmodul.netCMD_id   , val, len);
	dirtyBits ^= DIRTYB_ID;
}

void SET_netCMD_my   (void *val, size_t len)
{
	memcpy(&RFmodul.netCMD_my   , val, len);
	dirtyBits ^= DIRTYB_MY;
}

void SET_netCMD_dh   (void *val, size_t len)
{
	memcpy(&RFmodul.netCMD_dh   , val, len);
	dirtyBits ^= DIRTYB_DH_DL;
}

void SET_netCMD_dl   (void *val, size_t len)
{
	memcpy(&RFmodul.netCMD_dl   , val, len);
	dirtyBits ^= DIRTYB_DH_DL;
}

void SET_netCMD_ch   (void *val, size_t len)
{
	memcpy(&RFmodul.netCMD_ch, val, len);
	dirtyBits ^= DIRTYB_CH;
}

void SET_netCMD_a1   (void *val, size_t len)
{
	memcpy(&RFmodul.netCMD_a1, val, len);
	RFmodul.netCMD_ai = RFmodul.netCMD_a1 & RFmodul.netCMD_a2;
}

void SET_netCMD_a2   (void *val, size_t len)
{
	memcpy(&RFmodul.netCMD_a2, val, len);
	RFmodul.netCMD_ai = RFmodul.netCMD_a1 & RFmodul.netCMD_a2;
}

void SET_atCT_tmp    (void *val, size_t len)
{
	memcpy(&atCT_tmp, val, len);
	dirtyBits ^= DIRTYB_CT;
}

void SET_atAP_tmp    (void *val, size_t len)
{
	memcpy(&atAP_tmp, val, len);
	dirtyBits ^= DIRTYB_AP;
}

void SET_diagCMD_ec  (uint16_t val) { RFmodul.diagCMD_ec   = val; } // not in use right now
void SET_diagCMD_ea  (uint16_t val) { RFmodul.diagCMD_ea   = val; } // used in tx irq
void SET_atcopCMD_ct (uint16_t val) { RFmodul.atcopCMD_ct  = val; } // used in AC command
void SET_serintCMD_ap(uint8_t  val) { RFmodul.serintCMD_ap = val; } // used in AC command
void SET_diagCMD_db  (uint8_t  val) { RFmodul.diagCMD_db   = val; }	// used in receive function

// === get functions ======================================
/*
 * Get AT command parameter
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/01/19
 */

size_t    GET_device_tSize(void) { return device_tSize; }
device_t *GET_device(void)       { return &RFmodul; }

uint8_t  GET_atAP_tmp(void) { return atAP_tmp; }
uint16_t GET_atCT_tmp(void) { return atCT_tmp; }

uint8_t GET_serintCMD_ap(void) { return RFmodul.serintCMD_ap; }
uint8_t GET_serintCMD_bd(void) { return RFmodul.serintCMD_bd; }

uint8_t  GET_atcopCMD_cc(void) { return RFmodul.atcopCMD_cc; }
uint16_t GET_atcopCMD_gt(void) { return RFmodul.atcopCMD_gt; }
uint16_t GET_atcopCMD_ct(void) { return RFmodul.atcopCMD_ct; }

// === general functions ==================================
/*
 * Get device value returned the value from offset position
 * into destination pointer.
 *
 * Received:
 *		void	pointer to target
 *		CMD		pointer to command in command table
 *
 * Returns:
 *		uint16_t	length of copied value
 *
 * last modified: 2017/01/26
 */
uint16_t GET_deviceValue( void *dest, const CMD *cmd )
{
	void *s = &RFmodul;
	void *value = s+cmd->addr_offset;

	memcpy( dest, value, cmd->cmdSize );

	if ( AT_NI != cmd->ID )
	{
		return cmd->cmdSize;
	}
	else
	{
		return strlen( (const char*) RFmodul.netCMD_ni) + 1;
	}
}