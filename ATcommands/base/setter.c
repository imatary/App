/*
 * setter.c
 *
 * Created: 26.10.2016 13:36:35
 *  Author: TOE
 */ 

#include "../header/_global.h"

void set_default()
{
	netCMD.ch = CH_CHANNEL;	
	netCMD.id = ID_PANID; 
	netCMD.dh = DH_DEST_HIGH; 
	netCMD.dl = DL_DEST_LOW; 
	netCMD.my = MY_SHORT_ADDR; 
	netCMD.ce = CE_COORDINATOR_ENABLE; 
	netCMD.sc = SC_SCAN_CHANNELS;
	netCMD.ni = NI_NODE_IDENTIFY;
}