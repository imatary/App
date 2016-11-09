/*
 * setter.c
 *
 * Created: 26.10.2016 13:36:35
 *  Author: TOE
 */ 

#include "../header/_global.h"

/*
 * Set network parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 */
void SET_netDefault()
{
	netCMD.ch = CH_CHANNEL;	
	netCMD.id = ID_PANID; 
	netCMD.dh = DH_DEST_HIGH; 
	netCMD.dl = DL_DEST_LOW; 
	netCMD.my = MY_SHORT_ADDR;
	netCMD.sh = SH_SERIAL_HIGH;
	netCMD.sl = SL_SERIAL_LOW; 
	netCMD.ce = CE_COORDINATOR_ENABLE; 
	netCMD.sc = SC_SCAN_CHANNELS;
	netCMD.ni = NI_NODE_IDENTIFY;
}

/*
 * Set security parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 *
void SET_secDefault()
{
	
}

 *
 * Set RF interface parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 *
void SET_rfiDefault()
{
	
}

 *
 * Set sleep mode parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 *
void SET_sleepDafault()
{
	
}

 *
 * Set serial interfacing parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 *
void SET_siDefault()
{
	
}

 *
 * Set IO settings parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 *
void SET_iosDefault()
{
	
}

 *
 * Set IO line passing parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 *
void SET_iolpDefault()
{
	
}

 * Set diagnostics parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 *
void SET_diagDefault()
{
	
}

 *
 * Set AT command option parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 *
void SET_cmdoDefault()
{
	
}
 */
/*
 * Set AT command option parameter to default
 *
 * Returns:
 *     nothing
 *
 * Last modified:
 */
 void SET_allDefault()
 {
	 RFmodul.netCMD_ch = CH_CHANNEL;
	 RFmodul.netCMD_id = ID_PANID;
	 RFmodul.netCMD_dh = DH_DEST_HIGH;
	 RFmodul.netCMD_dl = DL_DEST_LOW;
	 RFmodul.netCMD_my = MY_SHORT_ADDR;
	 RFmodul.netCMD_sh = SH_SERIAL_HIGH;
	 RFmodul.netCMD_sl = SL_SERIAL_LOW;
	 RFmodul.netCMD_ce = CE_COORDINATOR_ENABLE;
	 RFmodul.netCMD_sc = SC_SCAN_CHANNELS;
	 RFmodul.netCMD_ni = NI_NODE_IDENTIFY;
	 RFmodul.netCMD_mm
	 RFmodul.netCMD_rr
	 RFmodul.netCMD_rn
	 RFmodul.netCMD_nt
	 RFmodul.netCMD_no
	 RFmodul.netCMD_ce
	 RFmodul.netCMD_sc
	 RFmodul.netCMD_sd
	 RFmodul.netCMD_a1
	 RFmodul.netCMD_a2
	 RFmodul.netCMD_ai
	 
	 RFmodul.secCMD_ee
	 RFmodul.secCMD_ky
	 
	 RFmodul.rfiCMD_pl
	 RFmodul.rfiCMD_ce
	 
	 RFmodul.sleepmCMD_sm
	 RFmodul.sleepmCMD_st
	 RFmodul.sleepmCMD_sp
	 RFmodul.sleepmCMD_dp
	 RFmodul.sleepmCMD_so
	 
	 RFmodul.serintCMD_bd
	 RFmodul.serintCMD_nb
	 RFmodul.serintCMD_ro
	 RFmodul.serintCMD_ap
	 
	 RFmodul.ioserCMD_d8
	 RFmodul.ioserCMD_d7
	 RFmodul.ioserCMD_d6
	 RFmodul.ioserCMD_d5
	 RFmodul.ioserCMD_d4
	 RFmodul.ioserCMD_d3
	 RFmodul.ioserCMD_d2
	 RFmodul.ioserCMD_d1
	 RFmodul.ioserCMD_d0
	 RFmodul.ioserCMD_pr
	 RFmodul.ioserCMD_iu
	 RFmodul.ioserCMD_it
	 RFmodul.ioserCMD_ic
	 RFmodul.ioserCMD_ir
	 RFmodul.ioserCMD_p0
	 RFmodul.ioserCMD_p1
	 RFmodul.ioserCMD_pt
	 RFmodul.ioserCMD_rp
	 
	 RFmodul.iolpCMD_ia
	 RFmodul.iolpCMD_T0
	 RFmodul.iolpCMD_T1
	 RFmodul.iolpCMD_T2
	 RFmodul.iolpCMD_T3
	 RFmodul.iolpCMD_T4
	 RFmodul.iolpCMD_T5
	 RFmodul.iolpCMD_T6
	 RFmodul.iolpCMD_T7
	 
	 RFmodul.diagCMD_vr
	 RFmodul.diagCMD_hv
	 RFmodul.diagCMD_db
	 RFmodul.diagCMD_ec
	 RFmodul.diagCMD_ea
	 RFmodul.diagCMD_dd
	 
	 RFmodul.atcopCMD_ct
	 RFmodul.atcopCMD_gt
	 RFmodul.atcopCMD_cc
 }