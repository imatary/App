/*
 * defaultConfig.h
 *
 * Created: 26.10.2016 08:43:02
 *  Author: TOE
 *
 * This defines are default values of the XBee modules.
 * Caution width modifications!
 */ 


#ifndef DEFAULTCONFIG_H_
#define DEFAULTCONFIG_H_

#define STD_DELIMITER			(0x7E)

/*
 * AT commands: network
 *
 * last modified: 2016/10/11
 */
#define CH_CHANNEL				(0x17)			// 0x0B-0x1A (XBee default 'C')
#define ID_PANID				(0x3332)		// 0x0-0xFFFF (XBee default '3332')
#define DH_DEST_HIGH			(0x0013A200)	// 0x0-0xFFFFFFFF (XBee default '0')	/* Current value for testing only reset to 0 when tests are done. */
#define DL_DEST_LOW				(0x414640C2)	// 0x0-0xFFFFFFFF (XBee default '0')	/* Current value for testing only reset to 0 when tests are done. */
#define MY_SHORT_ADDR			(0xDE)			// 0x0-0xFFFF (XBee default '0')		/* For testing only reset to 0 if you want. */
#define SH_SERIAL_HIGH			(0x00212eff)	// modems unique IEEE 64-bit source address (high 32-bit) -> read only
#define SL_SERIAL_LOW			(0xff00c327)	// modems unique IEEE 64-bit source address (low 32-bit)  -> read only
#define CE_COORDINATOR_ENABLE	(0x0)			// 0x0-0x1 (XBee default '0')
#define SC_SCAN_CHANNELS		(0x1FFE)		// 0x0-0xFFFF (XBee default '1FFE')
#define NI_NODE_IDENTIFY		"deConBee"		// 0-20 ASCII characters (XBee default '')

/*
 * AT commands: security
 *
 * last modified: 
 */

/*
 * AT commands: RF interface
 *
 * last modified: 
 */

/*
 * AT commands: sleep modes
 *
 * last modified: 
 */

/*
 * AT commands: serial interfacing
 *
 * last modified: 2016/10/11
 */
#define AP_API_ENABLE			(0x0)			// 0x0-0x2 (XBee default '0')

/*
 * AT commands: IO settings
 *
 * last modified: 
 */

/*
 * AT commands: IO line passing
 *
 * last modified: 
 */

/*
 * AT commands: diagnostics
 *
 * last modified: 2016/10/11
 */
#define VS_FIRMWARE_VERS		(0x10EF)		// cheat XBee Firmware
#define HV_HARDWARE_VERS		(0x1746)		// cheat XBee Hardware Version

/*
 * AT commands: AT command options
 *
 * last modified: 
 */



#endif /* DEFAULTCONFIG_H_ */