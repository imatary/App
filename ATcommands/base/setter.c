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