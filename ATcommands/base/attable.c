/*
 * attable.c
 *
 * Created: 15.11.2016 14:29:40
 *  Author: TOE
 *
 * Implemented the AT command table with informations about valid operations
 *
 * !!! ORDERED ALPHABETICALLY !!! for cmd search function
 */
#include <stdlib.h>
#include <string.h>
#include "../header/cmd.h"
#include "../header/atlocal.h"
#include "../header/rfmodul.h"
#include "../../ATuracoli/stackrelated.h" 

// === min / max values ===================================
const uint64_t  val_00    = 0x0;
const uint64_t  val_01    = 0x1;
const uint64_t  val_02    = 0x2;
const uint64_t  val_03    = 0x3;
const uint64_t  val_04    = 0x4;
const uint64_t  val_05    = 0x5;
const uint64_t  val_06    = 0x6;
const uint64_t  val_07    = 0x7;
const uint64_t  val_0B    = 0xB;
const uint64_t  val_0F    = 0xF;
const uint64_t  val_14    = 0x14;
const uint64_t  val_1A    = 0x1A;
const uint64_t  val_24    = 0x24;
const uint64_t  val_50    = 0x50;
const uint64_t  val_FC    = 0xFC;
const uint64_t  val_FF    = 0xFF;
const uint64_t val_CE4    = 0xCE4;
const uint64_t val_1770   = 0x1770;
const uint64_t val_68B0   = 0x68B0;
const uint64_t val_u16_FF = 0xFF;
const uint64_t val_u32_FF = 0xFFFFFFFF;
const uint64_t val_u64_FF = 0xFFFFFFFFFFFFFFFF;


// === command table ======================================
static const CMD StdCmdTable[] =
{
	/* AT */
	{ "AT%C", AT_pC, READ               ,  0,														                    }, // %
	{ "AT%V", AT_pV, READ               ,  0,														                    },			// TODO ZigBee

	{ "ATA1", AT_A1, READ | WRITE       ,  1, &val_00, &val_0F    , SET_netCMD_a1   , max_u32val     , GET_netCMD_a1    }, // A
	{ "ATA2", AT_A2, READ | WRITE       ,  1, &val_00, &val_07    , SET_netCMD_a1   , max_u32val     , GET_netCMD_a1    },
	{ "ATAC", AT_AC,			    EXEC,  0,														   				    },
	{ "ATAI", AT_AI, READ               ,  0,														 , GET_netCMD_ai    },
	{ "ATAP", AT_AP, READ | WRITE       ,  1, &val_00, &val_02    , SET_atAP_tmp    , max_u32val     , GET_atAP_tmp     },
	
	{ "ATBD", AT_BD, READ | WRITE       ,  1, &val_00, &val_07    , SET_serintCMD_bd, max_u32val     , GET_serintCMD_bd }, // B
	
	{ "ATCA", AT_CA, READ | WRITE       ,  1, &val_24, &val_50    , SET_rfiCMD_ca   ,                , GET_rfiCMD_ca    }, // C
	{ "ATCC", AT_CC, READ | WRITE       ,  1, &val_00, &val_00    , SET_atcopCMD_cc , max_u32val     , GET_atcopCMD_cc  },
	{ "ATCE", AT_CE, READ | WRITE       ,  1, &val_00, &val_01    , SET_netCMD_ce   , max_u32val     , GET_netCMD_ce    },
	{ "ATCH", AT_CH, READ | WRITE       ,  1, &val_0B, &val_1A    , SET_netCMD_ch   , max_u32val     , GET_netCMD_ch    },
	{ "ATCN", AT_CN,			    EXEC,  0,														   				    },
	{ "ATCT", AT_CT, READ | WRITE       ,  2, &val_02, &val_1770  , SET_atCT_tmp    , max_u32val     , GET_atCT_tmp     },
	,
	{ "ATD0", AT_D0, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d0 , max_u32val     , GET_ioserCMD_d0  }, // D
	{ "ATD1", AT_D1, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d1 , max_u32val     , GET_ioserCMD_d1  },
	{ "ATD2", AT_D2, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d2 , max_u32val     , GET_ioserCMD_d2  },
	{ "ATD3", AT_D3, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d3 , max_u32val     , GET_ioserCMD_d3  },
	{ "ATD4", AT_D4, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d4 , max_u32val     , GET_ioserCMD_d4  },
	{ "ATD5", AT_D5, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d5 , max_u32val     , GET_ioserCMD_d5  },
	{ "ATD6", AT_D6, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d6 , max_u32val     , GET_ioserCMD_d6  },
	{ "ATD7", AT_D7, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d7 , max_u32val     , GET_ioserCMD_d7  },
	{ "ATD8", AT_D8, READ | WRITE       ,  1, &val_00, &val_05    , SET_ioserCMD_d8 , max_u32val     , GET_ioserCMD_d8  },
	{ "ATDB", AT_DB, READ               ,  0,														 , GET_diagCMD_db   },
	{ "ATDD", AT_DD, READ | WRITE       ,  4, &val_00, &val_u32_FF, SET_diagCMD_dd  , max_u32val     , GET_diagCMD_dd   },
	{ "ATDH", AT_DH, READ | WRITE       ,  4, &val_00, &val_u32_FF, SET_netCMD_dh   , max_u32val     , GET_netCMD_dh    },
	{ "ATDL", AT_DL, READ | WRITE       ,  4, &val_00, &val_u32_FF, SET_netCMD_dl   , max_u32val     , GET_netCMD_dl    },
	{ "ATDP", AT_DP, READ | WRITE       ,  2, &val_01, &val_68B0  , SET_sleepmCMD_dp, max_u32val     , GET_sleepmCMD_dp },
	,
	{ "ATEA", AT_EA, READ               ,  0,														 , GET_diagCMD_ea   }, // E
	{ "ATEC", AT_EC, READ               ,  0,														 , GET_diagCMD_ec   },
	{ "ATEE", AT_EE, READ | WRITE       ,  1, &val_00, &val_01    , SET_secCMD_ee   , max_u32val     , GET_secCMD_ee    },
	,
	{ "ATGT", AT_GT, READ | WRITE       ,  2, &val_02, &val_CE4   , SET_atcopCMD_gt , max_u32val     , GET_atcopCMD_gt  }, // G
	,
	{ "ATHV", AT_HV, READ | WRITE       ,  2, &val_00, &val_u16_FF, SET_diagCMD_hv  , max_u32val     , GET_diagCMD_hv   }, // H
	,
	{ "ATIA", AT_IA, READ | WRITE       ,  8, &val_00, &val_u64_FF, SET_iolpCMD_ia  , max_u64val     , GET_iolpCMD_ia   }, // I
	{ "ATIC", AT_IC, READ | WRITE       ,  1, &val_00, &val_FF    , SET_ioserCMD_ic , max_u32val     , GET_ioserCMD_ic  },
	{ "ATID", AT_ID, READ | WRITE       ,  2, &val_00, &val_u16_FF, SET_netCMD_id   , max_u32val     , GET_netCMD_id    },
	{ "ATIR", AT_IR, READ | WRITE       ,  2, &val_00, &val_u16_FF, SET_ioserCMD_ir , max_u32val     , GET_ioserCMD_ir  },
	{ "ATIT", AT_IT, READ | WRITE       ,  2, &val_01, &val_u16_FF, SET_ioserCMD_it , max_u32val     , GET_ioserCMD_it  },
	{ "ATIU", AT_IU, READ | WRITE       ,  1, &val_00, &val_01    , SET_ioserCMD_iu , max_u32val     , GET_ioserCMD_iu  },
	,
	{ "ATKY", AT_KY,		WRITE | EXEC, 16, &val_00, &val_FF    , SET_secCMD_ky   , ky_validator   , GET_secCMD_ky    }, // K
	,
	{ "ATMM", AT_MM, READ | WRITE       ,  1, &val_00, &val_03    , SET_netCMD_mm   , max_u32val     , GET_netCMD_mm    }, // M
	{ "ATMY", AT_MY, READ | WRITE       ,  2, &val_00, &val_u16_FF, SET_netCMD_my   , max_u32val     , GET_netCMD_my    },
	,
	{ "ATNB", AT_NB, READ | WRITE       ,  1, &val_00, &val_04    , SET_serintCMD_nb, max_u32val     , GET_serintCMD_nb }, // N
	{ "ATNI", AT_NI, READ | WRITE       , 20, NULL   , &val_14    , SET_netCMD_ni   , node_identifier, GET_netCMD_ni    },
	{ "ATNO", AT_NO, READ | WRITE       ,  1, &val_00, &val_01    , SET_netCMD_no   , max_u32val     , GET_netCMD_no    },
	{ "ATNT", AT_NT, READ | WRITE       ,  1, &val_01, &val_FC    , SET_netCMD_nt   , max_u32val     , GET_netCMD_nt    },
	,
	{ "ATP0", AT_P0, READ | WRITE       ,  1, &val_00, &val_02    , SET_ioserCMD_p0 , max_u32val     , GET_ioserCMD_p0  }, // P
	{ "ATP1", AT_P1, READ | WRITE       ,  1, &val_00, &val_02    , SET_ioserCMD_p1 , max_u32val     , GET_ioserCMD_p1  },
	{ "ATPL", AT_PL, READ | WRITE       ,  1, &val_00, &val_04    , SET_rfiCMD_pl   , max_u32val     , GET_rfiCMD_pl    },
	{ "ATPR", AT_PR, READ | WRITE       ,  1, &val_00, &val_FF    , SET_ioserCMD_pr , max_u32val     , GET_ioserCMD_pr  },
	{ "ATPT", AT_PT, READ | WRITE       ,  1, &val_00, &val_FF    , SET_ioserCMD_pt , max_u32val     , GET_ioserCMD_pt  },
	,
	{ "ATR?", AT_Rq, READ               ,  0,														 , 				    }, // R
	{ "ATRE", AT_RE,			    EXEC,  0,														 , 				    },
	{ "ATRN", AT_RN, READ | WRITE       ,  1, &val_00, &val_03    , SET_netCMD_rn   , max_u32val     , GET_netCMD_rn    },
	{ "ATRO", AT_RO, READ | WRITE       ,  1, &val_00, &val_FF    , SET_serintCMD_ro, max_u32val     , GET_serintCMD_ro },
	{ "ATRP", AT_RP, READ | WRITE       ,  1, &val_00, &val_FF    , SET_ioserCMD_rp , max_u32val     , GET_ioserCMD_rp  },
	{ "ATRR", AT_RR, READ | WRITE       ,  1, &val_00, &val_06    , SET_netCMD_rr   , max_u32val     , GET_netCMD_rr    },
	,
	{ "ATSB", AT_SB, READ               ,  0,														 , 				    }, // S
	{ "ATSC", AT_SC, READ | WRITE       ,  2, &val_00, &val_u16_FF, SET_netCMD_sc   , max_u32val     , GET_netCMD_sc    },
	{ "ATSD", AT_SD, READ | WRITE       ,  1, &val_00, &val_0F    , SET_netCMD_sd   , max_u32val     , GET_netCMD_sd    },
	{ "ATSH", AT_SH, READ               ,  4,														 , GET_netCMD_sh    },
	{ "ATSL", AT_SL, READ               ,  4,														 , GET_netCMD_sl    },
	{ "ATSM", AT_SM, READ | WRITE       ,  1, &val_00, &val_06    , SET_sleepmCMD_sm, max_u32val     , GET_sleepmCMD_sm },
	{ "ATSO", AT_SO, READ | WRITE       ,  1, &val_00, &val_06    , SET_sleepmCMD_so, max_u32val     , GET_sleepmCMD_so },
	{ "ATSP", AT_SP, READ | WRITE       ,  2, &val_00, &val_68B0  , SET_sleepmCMD_sp, max_u32val     , GET_sleepmCMD_sp },
	{ "ATSS", AT_SS, READ               ,  0														 , 				    },
	{ "ATST", AT_ST, READ | WRITE       ,  2, &val_01, &val_u16_FF, SET_sleepmCMD_st, max_u32val     , GET_sleepmCMD_st },
	,
	{ "ATT0", AT_T0, READ | WRITE       ,  1, &val_00, &val_FF    , SET_iolpCMD_T0  , max_u32val     , GET_iolpCMD_T0   }, // T
	{ "ATT1", AT_T1, READ | WRITE       ,  1, &val_00, &val_FF    , SET_iolpCMD_T1  , max_u32val     , GET_iolpCMD_T1   },
	{ "ATT2", AT_T2, READ | WRITE       ,  1, &val_00, &val_FF    , SET_iolpCMD_T2  , max_u32val     , GET_iolpCMD_T2   },
	{ "ATT3", AT_T3, READ | WRITE       ,  1, &val_00, &val_FF    , SET_iolpCMD_T3  , max_u32val     , GET_iolpCMD_T3   },
	{ "ATT4", AT_T4, READ | WRITE       ,  1, &val_00, &val_FF    , SET_iolpCMD_T4  , max_u32val     , GET_iolpCMD_T4   },
	{ "ATT5", AT_T5, READ | WRITE       ,  1, &val_00, &val_FF    , SET_iolpCMD_T5  , max_u32val     , GET_iolpCMD_T5   },
	{ "ATT6", AT_T6, READ | WRITE       ,  1, &val_00, &val_FF    , SET_iolpCMD_T6  , max_u32val     , GET_iolpCMD_T6   },
	{ "ATT7", AT_T7, READ | WRITE       ,  1, &val_00, &val_FF    , SET_iolpCMD_T7  , max_u32val     , GET_iolpCMD_T7   },
	,
	{ "ATVR", AT_VR, READ | WRITE       ,  2, &val_00, &val_u16_FF, SET_diagCMD_vr  , max_u32val     , GET_diagCMD_vr   }, // V

	{ "ATWR", AT_WR,			    EXEC,  0,														  }, // W

	/* DE */
	{ "DEFV", DE_FV, READ               ,  0,														  }, // F

};

const CMD *pStdCmdTable = StdCmdTable;

/*
 * Binary search function to find command in table
 * halved the table every time, runtime ~ O( log(n) )
 *
 * IMPORTANT! : table needs to be sorted alphabetically
 *
 * Received:
 *		uint8_t		pointer to the array which hold the AT command line
 *
 * Returns:
 *		CMD			pointer to command position in the command table
 *		NO_AT_CMD	if there not a command available
 *
 * last modified: 2016/12/19
 */
CMD *CMD_findInTable(uint8_t *cmd)
{
	int right  = ( sizeof(StdCmdTable)/sizeof(CMD) ) - 1; // count of elements in CMD table - 1
	int middle;
	int left = 0;
	int val  = 0;
	
	while( left <= right )
	{
		middle = left + ( ( right - left) /2 );
		val = strncmp( (const char*) cmd, (pStdCmdTable+middle)->name, 4 );
		
		if      ( 0 == val )   return (CMD*) (pStdCmdTable+middle);		
		else if ( 0 > val )    right = middle - 1;
		else /* ( 0 < val ) */ left  = middle + 1;
		
	}
	return NO_AT_CMD;
}