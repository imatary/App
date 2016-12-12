/*
 * enum_cmd.h
 *
 * Created: 26.10.2016 13:11:43
 *  Author: TOE
 */ 


#ifndef ENUM_CMD_H_
#define ENUM_CMD_H_

/*
 * AT Command IDs.
 *
 * Commands are enumerated, starting with zero and packed to save memory.
 */

typedef enum
{
	NO_AT_CMD = 0,
	// network
	AT_CH,					// CHANNEL
	AT_ID,					// PANID
	AT_DH,					// DEST HIGH
	AT_DL,					// DEST LOW
	AT_MY,					// SHORT ADDR
	AT_SH,					// SERIAL HIGH
	AT_SL,					// SERIAL LOW
	AT_CE,					// COORDINATOR ENABLE
	AT_SC,					// SCAN CHANNELS
	AT_NI,					// NODE IDENTIFY
	AT_MM,					// MAC MODE
	AT_RR,					// XBEE RETRIES
	AT_RN,					// RANDOM DELAY SLOTS
	AT_NT,					// NODE DISCOVER TIME
	AT_NO,					// NODE DISCOVER OPTION
	AT_SD,					// SCAN DURATION
	AT_A1,					// END DEVICE ASSOCIATIO
	AT_A2,					// COORDINATOR ASSOCIATI
	AT_AI,					// ASSOCIATION INDICATIO
	// security
	AT_KY,					// AES ECRYPTION KEY
	AT_EE,					// AES ECRYPTION ENABLE
	// RF interface
	AT_PL,					// POWER LEVEL
	AT_CA,					// CCA TRESHOLD
	// sleep modes										
	AT_SM,					// SLEEP MODE
	AT_ST,					// TIME BEFORE SLEEP
	AT_SP,					// CYCLIC SLEEP PERIOD
	AT_DP,					// DISASSOCIATED SP
	AT_SO,					// SLEEP OPTION
	AT_SS,					// SLEEP STATUS
	// serial interfacing								
	AT_AP,					// API ENABLE
	AT_BD,					// INTERFACE DATA RATE	
	AT_NB,					// PARITY
	AT_RO,					// PACKETIZATION TIMEOUT
	// IO settings			
	AT_D8,					// DI8 CONFIGURATION
	AT_D7,					// DIO7 CONFIGURATION
	AT_D6,					// DIO6 CONFIGURATION
	AT_D5,					// DIO5 CONFIGURATION
	AT_D4,					// DIO4 CONFIGURATION
	AT_D3,					// DIO3 CONFIGURATION
	AT_D2,					// DIO2 CONFIGURATION
	AT_D1,					// DIO1 CONFIGURATION
	AT_D0,					// DIO0 CONFIGURATION
	AT_PR,					// PULLUP RESISTOR ENABLE
	AT_IU,					// IO OUTPUT ENABLE
	AT_IT,					// SAMPLES BEFORE TX
	AT_IC,					// DIO CHANGE DETECT
	AT_IR,					// SAPLE RATE
	AT_P0,					// PWM0 CONFIGURATION
	AT_P1,					// PWM1 CONFIGURATION
	AT_PT,					// PWM OUTPUT TIMEOUT
	AT_RP,					// RSSI PWM TIMER
	// IO line passing		 
	AT_IA,					// IO INPUT ADDRESS
	AT_T0,					// D0 OUTPUT TIMEOUT
	AT_T1,					// D1 OUTPUT TIMEOUT
	AT_T2,					// D2 OUTPUT TIMEOUT
	AT_T3,					// D3 OUTPUT TIMEOUT
	AT_T4,					// D4 OUTPUT TIMEOUT
	AT_T5,					// D5 OUTPUT TIMEOUT
	AT_T6,					// D6 OUTPUT TIMEOUT
	AT_T7,					// D7 OUTPUT TIMEOUT
	// diagnostics			 							
	AT_VR,					// FIRMWARE VERS
	AT_HV,					// HARDWARE VERS
	AT_DB,					// RECEIVED SIGNAL STRENGTH
	AT_EC,					// CCA FAILURES
	AT_EA,					// ACK FAILURES
	AT_DD,					// DEVICE TYPE IDENTIFIER
	AT_pV,					// PERCENT VOLTAGE
	// AT command options							
	AT_CT,					// AT CMD TIMEOUT
	AT_GT,					// GUART TIMES
	AT_CC,					// COMMAND SEQUENCE CHAR
	//exec commands
	AT_CN,					// COMMAND NULL
	AT_WR,					// WRITE
	AT_RE,					// RESET
	AT_AC,					// APPLY CHANGES
	// unknown commands
	AT_Rq,					// R?
	AT_pC,					// %C
	// no assignment
	AT_SB,					// STOP BITS
	
	// special device commands
	DE_RU,					// RETURN TO UART
	DE_FV					// AT FIRMWARE VERSION
																		
}__attribute__((packed)) cmdIDs;										
																		
#endif /* ENUM_CMD_H_ */