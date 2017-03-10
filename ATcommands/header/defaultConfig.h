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

/*
 * AT commands: network
 *
 * last modified: 2016/10/11
 */
#define CH_CHANNEL					(0xC)			// 0x0B-0x1A (XBee default 'C')
#define ID_PANID					(0x3332)		// 0x0-0xFFFF (XBee default '3332')
#define DH_DEST_HIGH				(0x0)			// 0x0-0xFFFFFFFF (XBee default '0')
#define DL_DEST_LOW					(0x0)			// 0x0-0xFFFFFFFF (XBee default '0')
#define MY_SHORT_ADDR				(0xDE)			// 0x0-0xFFFF (XBee default '0')
#define CE_COORDINATOR_ENABLE		(FALSE)			// 0x0-0x1 (XBee default '0')
#define SC_SCAN_CHANNELS			(0x1FFE)		// 0x0-0xFFFF (XBee default '1FFE')
#define NI_NODE_IDENTIFY			"deConBee"		// 0-20 ASCII characters (XBee default '')
#define MM_MAC_MODE					(0x0)			// 0x0-0x3 (XBee default '0')
#define RR_XBEE_RETRIES				(0x0)			// 0x0-0x6 (XBee default '0')
#define RN_RANDOM_DELAY_SLOTS		(0x0)			// 0x0-0x3 (XBee default '0')
#define NT_NODE_DISCOVER_TIME		(0x19)			// 0x1-0xFC (XBee default '19')
#define NO_NODE_DISCOVER_OPTION		(FALSE)			// 0x0-0x1 (XBee default '0')
#define SD_SCAN_DURATION			(0x4)			// 0x0-0xF (XBee default '4')
#define A1_END_DEVICE_ASSOCIATION	(0x0)			// 0x0-0xF (XBee default '0')
#define A2_COORDINATOR_ASSOCIATION	(0x0)			// 0x0-0xF (XBee default '0')
#define AI_ASSOCIATION_INDICATION	(0x0)			// dummy value -> read only

/*
 * AT commands: security
 *
 * last modified: 2016/11/10
 */
#define EE_AES_ECRYPTION_ENABLE		(FALSE)			// 0x0-0x1 (XBee default '0')

/*
 * AT commands: RF interface
 *
 * last modified: 2016/11/10
 */
#define PL_POWER_LEVEL				(0x3)			// 0x0-0x3 (XBee default '4')
#define CA_CCA_TRESHOLD				(0x2C)			// 0x24-0x50 (XBee default '2C')

/*
 * AT commands: sleep modes
 *
 * last modified: 2016/11/10
 */
#define SM_SLEEP_MODE				(0x0)			// 0x0-0x6 (XBee default '0')
#define ST_TIME_BEFORE_SLEEP		(0x1388)		// 0x1-0xFFFF (XBee default '1388')
#define SP_CYCLIC_SLEEP_PERIOD		(0x0)			// 0x0-0x68B0 (XBee default '0')
#define DP_DISASSOCIATED_SP			(0x3E8)			// 0x1-0x68B0 (XBee default '3E8')
#define SO_SLEEP_OPTION				(0x0)			// 0x0-0x6 (XBee default '0')

/*
 * AT commands: serial interfacing
 *
 * last modified: 2016/11/10
 */
#define AP_AP_ENABLE				(0x0)			// 0x0-0x2 (XBee default '0')
#define BD_INTERFACE_DATA_RATE		(0x5)			// 0x0-0x7 (XBee default '3')
#define NB_PARITY					(0x0)			// 0x0-0x4 (XBee default '0')
#define RO_PACKETIZATION_TIMEOUT	(0x3)			// 0x0-0xFF (XBee default '3')

/*
 * AT commands: IO settings
 *
 * last modified: 2016/11/10
 */
#define D8_DI8_CONFIGURATION		(0x0)			// 0x0 & 0x5 (XBee default '0')
#define D7_DIO7_CONFIGURATION		(0x0)			// 0x0-0x5 (XBee default '1')
#define D6_DIO6_CONFIGURATION		(0x0)			// 0x0-0x5 (XBee default '0')
#define D5_DIO5_CONFIGURATION		(0x0)			// 0x0-0x5 (XBee default '1')
#define D4_DIO4_CONFIGURATION		(0x0)			// 0x0-0x5 (XBee default '0')
#define D3_DIO3_CONFIGURATION		(0x0)			// 0x0-0x5 (XBee default '0')
#define D2_DIO2_CONFIGURATION		(0x0)			// 0x0-0x5 (XBee default '0')
#define D1_DIO1_CONFIGURATION		(0x0)			// 0x0-0x5 (XBee default '0')
#define D0_DIO0_CONFIGURATION		(0x0)			// 0x0-0x5 (XBee default '0')
#define PR_PULLUP_RESISTOR_ENABLE	(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define IU_IO_OUTPUT_ENABLE			(TRUE)			// 0x0-0x1 (XBee default '1')
#define IT_SAMPLES_BEFORE_TX		(0x1)			// 0x1-0xFF (XBee default '1')
#define IC_DIO_CHANGE_DETECT		(0x0)			// 0x0-0xFF (XBee default '0')
#define IR_SAPLE_RATE				(0x0)			// 0x0-0xFFFF (XBee default '0')
#define P0_PWM0_CONFIGURATION		(0x1)			// 0x0-0x2 (XBee default '1')
#define P1_PWM1_CONFIGURATION		(0x0)			// 0x0-0x2 (XBee default '0')
#define PT_PWM_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define RP_RSSI_PWM_TIMER			(0x28)			// 0x0-0xFF (XBee default '28')

/*
 * AT commands: IO line passing
 *
 * last modified: 2016/11/10
 */
#define IA_IO_INPUT_ADDRESS			(0xFFFFFFFFFFFFFFFF) // 0x0-0xFFFFFFFFFFFFFFFF (XBee default 'FFFFFFFFFFFFFFFF')
#define T0_D0_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define T1_D1_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define T2_D2_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define T3_D3_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define T4_D4_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define T5_D5_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define T6_D6_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')
#define T7_D7_OUTPUT_TIMEOUT		(0xFF)			// 0x0-0xFF (XBee default 'FF')

/*
 * AT commands: diagnostics
 *
 * last modified: 2016/11/10
 */
#define VR_FIRMWARE_VERS			(0xDE10)		// ConBee Firmware Version	-> read only
#define HV_HARDWARE_VERS			(0x0)		    // ConBee Hardware Version	-> cheat value, read only
#define DB_RECEIVED_SIGNAL_STRENGTH (0x0)			// dummy value				-> read only
#define EC_CCA_FAILURES				(0x0)			// dummy value				-> read only
#define EA_ACK_FAILURES				(0x0)			// dummy value				-> read only
#define DD_DEVICE_TYPE_IDENTIFIER	(0x10000)		// 0x0-0xFFFFFFFF (XBee default '10000')

/*
 * AT commands: AT command options
 *
 * last modified: 2016/11/10
 */
#define CT_AT_CMD_TIMEOUT			(0x64)			// 0x2-0x1770 (XBee default '64')
#define GT_GUART_TIMES				(0x3E8)			// 0x2-0xCE4 (XBee default '3E8')
#define CC_COMMAND_SEQUENCE_CHAR	(0x2B)			// 0x0-0xFF (XBee default '2B')

/*
 * AT commands: device related
 *
 * last modified: 20--/--/--
 */



#endif /* DEFAULTCONFIG_H_ */