/*
 * setter.c
 *
 * Created: 26.10.2016 13:36:35
 *  Author: TOE
 */ 

#include <string.h>
#include <avr/eeprom.h>

#include "../header/_global.h"			// RFmodul struct
#include "../header/defaultConfig.h"	// defines for default configuration
#include "../header/rfmodul.h"			// prototypes

device_t RFmodul;
/*
 * Set network parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_netDefault()
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
}

/*
 * Set security parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_secDefault()
{
	RFmodul.secCMD_ee = EE_AES_ECRYPTION_ENABLE;
}

/*
 * Set RF interface parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_rfiDefault()
{
	RFmodul.rfiCMD_pl = PL_POWER_LEVEL;
	RFmodul.rfiCMD_ca = CA_CCA_TRESHOLD;
}

/*
 * Set sleep mode parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_sleepDafault()
{
	RFmodul.sleepmCMD_sm = SM_SLEEP_MODE;
	RFmodul.sleepmCMD_st = ST_TIME_BEFORE_SLEEP;
	RFmodul.sleepmCMD_sp = SP_CYCLIC_SLEEP_PERIOD;
	RFmodul.sleepmCMD_dp = DP_DISASSOCIATED_SP;
	RFmodul.sleepmCMD_so = SO_SLEEP_OPTION;
}

/*
 * Set serial interfacing parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_siDefault()
{
	RFmodul.serintCMD_bd = BD_INTERFACE_DATA_RATE;
	RFmodul.serintCMD_nb = NB_PARITY;
	RFmodul.serintCMD_ro = RO_PACKETIZATION_TIMEOUT;
	RFmodul.serintCMD_ap = AP_API_ENABLE;
}

/*
 * Set IO settings parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_iosDefault()
{
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
}

/*
 * Set IO line passing parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_iolpDefault()
{
	RFmodul.iolpCMD_ia = IA_IO_INPUT_ADDRESS;
	RFmodul.iolpCMD_T0 = T0_D0_OUTPUT_TIMEOUT;
	RFmodul.iolpCMD_T1 = T1_D1_OUTPUT_TIMEOUT;
	RFmodul.iolpCMD_T2 = T2_D2_OUTPUT_TIMEOUT;
	RFmodul.iolpCMD_T3 = T3_D3_OUTPUT_TIMEOUT;
	RFmodul.iolpCMD_T4 = T4_D4_OUTPUT_TIMEOUT;
	RFmodul.iolpCMD_T5 = T5_D5_OUTPUT_TIMEOUT;
	RFmodul.iolpCMD_T6 = T6_D6_OUTPUT_TIMEOUT;
	RFmodul.iolpCMD_T7 = T7_D7_OUTPUT_TIMEOUT;
}

/* Set diagnostics parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_diagDefault()
{
	RFmodul.diagCMD_vr = VR_FIRMWARE_VERS;
	RFmodul.diagCMD_hv = HV_HARDWARE_VERS;
	RFmodul.diagCMD_db = DB_RECEIVED_SIGNAL_STRENGTH;
	RFmodul.diagCMD_ec = EC_CCA_FAILURES;
	RFmodul.diagCMD_ea = EA_ACK_FAILURES;
	RFmodul.diagCMD_dd = DD_DEVICE_TYPE_IDENTIFIER;
}

/*
 * Set AT command option parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
 */
void SET_cmdoDefault()
{
	RFmodul.atcopCMD_ct = CT_AT_CMD_TIMEOUT;
	RFmodul.atcopCMD_gt = GT_GUART_TIMES;
	RFmodul.atcopCMD_cc = CC_COMMAND_SEQUENCE_CHAR;
}

/*
 * Set all AT command parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified: 2016/11/10
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
	 RFmodul.serintCMD_ap = AP_API_ENABLE;
	 
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
	 				    
	 RFmodul.atcopCMD_ct = CT_AT_CMD_TIMEOUT;
	 RFmodul.atcopCMD_gt = GT_GUART_TIMES;
	 RFmodul.atcopCMD_cc = CC_COMMAND_SEQUENCE_CHAR;
 }