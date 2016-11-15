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
	AT_CH,
	AT_ID,
	AT_DH,
	AT_DL,
	AT_MY,
	AT_SH,
	AT_SL,
	AT_CE,
	AT_SC,
	AT_NI,
	AT_MM,
	AT_RR,
	AT_RN,
	AT_NT,
	AT_NO,
	AT_SD,
	AT_A1,
	AT_A2,
	AT_AI,
	// security
	AT_KY,							// = 20
	AT_EE,
	// RF interface
	AT_PL,
	AT_CA,
	// sleep modes
	AT_SM,
	AT_ST,
	AT_SP,
	AT_DP,
	AT_SO,
	// serial interfacing
	AT_AP,
	AT_BD,
	AT_NB,
	AT_RO,
	// IO settings
	AT_D8,
	AT_D7,
	AT_D6,
	AT_D5,
	AT_D4,
	AT_D3,
	AT_D2,
	AT_D1,							// = 40
	AT_D0,
	AT_PR,
	AT_IU,
	AT_IT,
	AT_IC,
	AT_IR,
	AT_P0,
	AT_P1,
	AT_PT,
	AT_RP,
	// IO line passing
	AT_IA,
	AT_T0,
	AT_T1,
	AT_T2,
	AT_T3,
	AT_T4,
	AT_T5,
	AT_T6,
	AT_T7,
	// diagnostics
	AT_VR,						   // = 60
	AT_HV,
	AT_DB,
	AT_EC,
	AT_EA,
	AT_DD,
	// AT command options
	AT_CT,
	AT_GT,
	AT_CC,
	//exec commands
	AT_CN,
	
}__attribute__((packed)) cmdIDs;

#endif /* ENUM_CMD_H_ */
