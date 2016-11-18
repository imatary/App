/*
 * attable.c
 *
 * Created: 15.11.2016 14:29:40
 *  Author: TOE
 */
#include "../header/cmd.h" 

static const CMD StdCmdTable[] =
{
	{ "ATCH", AT_CH, READ | WRITE       },
	{ "ATID", AT_ID, READ | WRITE       },
	{ "ATDH", AT_DH, READ | WRITE       },
	{ "ATDL", AT_DL, READ | WRITE       },
	{ "ATMY", AT_MY, READ | WRITE       },
	{ "ATSH", AT_SH, READ               },
	{ "ATSL", AT_SL, READ               },
	{ "ATCE", AT_CE, READ | WRITE       },
	{ "ATSC", AT_SC, READ | WRITE       },
	{ "ATNI", AT_NI, READ | WRITE       },
	{ "ATMM", AT_MM, READ | WRITE       },
	{ "ATRR", AT_RR, READ | WRITE       },
	{ "ATRN", AT_RN, READ | WRITE       },
	{ "ATNT", AT_NT, READ | WRITE       },
	{ "ATNO", AT_NO, READ | WRITE       },
	{ "ATSD", AT_SD, READ | WRITE       },
	{ "ATA1", AT_A1, READ | WRITE       },
	{ "ATA2", AT_A2, READ | WRITE       },
	{ "ATAI", AT_AI, READ               },

	{ "ATKY", AT_KY, READ | WRITE       },
	{ "ATEE", AT_EE, READ | WRITE       },

	{ "ATPL", AT_PL, READ | WRITE       },
	{ "ATCA", AT_CA, READ | WRITE       },

	{ "ATSM", AT_SM, READ | WRITE       },
	{ "ATST", AT_ST, READ | WRITE       },
	{ "ATSP", AT_SP, READ | WRITE       },
	{ "ATDP", AT_DP, READ | WRITE       },
	{ "ATSO", AT_SO, READ | WRITE       },
	{ "ATSS", AT_SO, READ               },

	{ "ATAP", AT_AP, READ | WRITE       },
	{ "ATBD", AT_BD, READ | WRITE       },
	{ "ATNB", AT_NB, READ | WRITE       },
	{ "ATRO", AT_RO, READ | WRITE       },

	{ "ATD8", AT_D8, READ | WRITE       },
	{ "ATD7", AT_D7, READ | WRITE       },
	{ "ATD6", AT_D6, READ | WRITE       },
	{ "ATD5", AT_D5, READ | WRITE       },
	{ "ATD4", AT_D4, READ | WRITE       },
	{ "ATD3", AT_D3, READ | WRITE       },
	{ "ATD2", AT_D2, READ | WRITE       },
	{ "ATD1", AT_D1, READ | WRITE       },
	{ "ATD0", AT_D0, READ | WRITE       },
	{ "ATPR", AT_PR, READ | WRITE       },
	{ "ATIU", AT_IU, READ | WRITE       },
	{ "ATIT", AT_IT, READ | WRITE       },
	{ "ATIC", AT_IC, READ | WRITE       },
	{ "ATIR", AT_IR, READ | WRITE       },
	{ "ATP0", AT_P0, READ | WRITE       },
	{ "ATP1", AT_P1, READ | WRITE       },
	{ "ATPT", AT_PT, READ | WRITE       },
	{ "ATRP", AT_RP, READ | WRITE       },

	{ "ATIA", AT_IA, READ | WRITE       },
	{ "ATT0", AT_T0, READ | WRITE       },
	{ "ATT1", AT_T1, READ | WRITE       },
	{ "ATT2", AT_T2, READ | WRITE       },
	{ "ATT3", AT_T3, READ | WRITE       },
	{ "ATT4", AT_T4, READ | WRITE       },
	{ "ATT5", AT_T5, READ | WRITE       },
	{ "ATT6", AT_T6, READ | WRITE       },
	{ "ATT7", AT_T7, READ | WRITE       },

	{ "ATVR", AT_VR, READ               },
	{ "ATHV", AT_HV, READ               },
	{ "ATDB", AT_DB, READ               },
	{ "ATEC", AT_EC, READ               },
	{ "ATEA", AT_EA, READ               },
	{ "ATDD", AT_DD, READ | WRITE       },
	{ "AT%V", AT_pV, READ               },

	{ "ATCT", AT_CT, READ | WRITE       },
	{ "ATGT", AT_GT, READ | WRITE       },
	{ "ATCC", AT_CC, READ | WRITE       },

	{ "ATCN", AT_CN,			   EXEC },
	{ "ATWR", AT_WR,			   EXEC },
	{ "ATWR", AT_RE,			   EXEC },
	{ "ATAC", AT_AC,			   EXEC },
		
	{ "ATR?", AT_Rq, READ               },
	{ "AT%C", AT_pC, READ               },
		
	{ "ATSB", AT_SB, READ               },

};

const CMD *pStdCmdTable = StdCmdTable;
const size_t command_count = sizeof(StdCmdTable)/sizeof(CMD);