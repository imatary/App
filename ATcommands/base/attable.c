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
const uint8_t  val_00     = 0x0;
const uint8_t  val_01     = 0x1;
const uint8_t  val_02     = 0x2;
const uint8_t  val_03     = 0x3;
const uint8_t  val_04     = 0x4;
const uint8_t  val_05     = 0x5;
const uint8_t  val_06     = 0x6;
const uint8_t  val_07     = 0x7;
const uint8_t  val_0B     = 0xB;
const uint8_t  val_0F     = 0xF;
const uint8_t  val_14     = 0x14;
const uint8_t  val_1A     = 0x1A;
const uint8_t  val_24     = 0x24;
const uint8_t  val_50     = 0x50;
const uint8_t  val_FC     = 0xFC;
const uint8_t  val_FF     = 0xFF;
const uint16_t val_CE4    = 0xCE4;
const uint16_t val_1770   = 0x1770;
const uint16_t val_68B0   = 0x68B0;
const uint16_t val_u16_FF = 0xFF;
const uint32_t val_u32_FF = 0xFFFFFFFF;
const uint64_t val_u64_FF = 0xFFFFFFFFFFFFFFFF;


// === command table ======================================
static const CMD StdCmdTable[] =
{
	/* AT */
	{ "AT%C", AT_pC, READ               ,  0,																				  }, // %
	{ "AT%V", AT_pV, READ               ,  0,																				  },			// TODO

	{ "ATA1", AT_A1, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_0F    , SET_netCMD_a1   , max_u32val      }, // A
	{ "ATA2", AT_A2, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_07    , SET_netCMD_a1   , max_u32val      },
	{ "ATAC", AT_AC,			    EXEC,  0,																				  },
	{ "ATAI", AT_AI, READ               ,  0,																				  },
	{ "ATAP", AT_AP, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_02    , SET_atAP_tmp    , max_u32val      },
		
	{ "ATBD", AT_BD, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_07    , SET_serintCMD_bd, max_u32val      }, // B

	{ "ATCA", AT_CA, READ | WRITE       ,  1, (uint64_t*) &val_24, (uint64_t*) &val_50    , SET_rfiCMD_ca   ,                 }, // C
	{ "ATCC", AT_CC, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_00    , SET_atcopCMD_cc , max_u32val      },
	{ "ATCE", AT_CE, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_01    , SET_netCMD_ce   , max_u32val      },
	{ "ATCH", AT_CH, READ | WRITE       ,  1, (uint64_t*) &val_0B, (uint64_t*) &val_1A    , SET_netCMD_ch   , max_u32val      },
	{ "ATCN", AT_CN,			    EXEC,  0,																				  },
	{ "ATCT", AT_CT, READ | WRITE       ,  2, (uint64_t*) &val_02, (uint64_t*) &val_1770  , SET_atCT_tmp    , max_u32val      },

	{ "ATD0", AT_D0, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d0 , max_u32val      }, // D
	{ "ATD1", AT_D1, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d1 , max_u32val      },
	{ "ATD2", AT_D2, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d2 , max_u32val      },
	{ "ATD3", AT_D3, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d3 , max_u32val      },
	{ "ATD4", AT_D4, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d4 , max_u32val      },
	{ "ATD5", AT_D5, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d5 , max_u32val      },
	{ "ATD6", AT_D6, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d6 , max_u32val      },	
	{ "ATD7", AT_D7, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d7 , max_u32val      },
	{ "ATD8", AT_D8, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_05    , SET_ioserCMD_d8 , max_u32val      },
	{ "ATDB", AT_DB, READ               ,  0,																				  }, 
	{ "ATDD", AT_DD, READ | WRITE       ,  4, (uint64_t*) &val_00, (uint64_t*) &val_u32_FF, SET_diagCMD_dd  , max_u32val      },
	{ "ATDH", AT_DH, READ | WRITE       ,  4, (uint64_t*) &val_00, (uint64_t*) &val_u32_FF, SET_netCMD_dh   , max_u32val      },
	{ "ATDL", AT_DL, READ | WRITE       ,  4, (uint64_t*) &val_00, (uint64_t*) &val_u32_FF, SET_netCMD_dl   , max_u32val      },
	{ "ATDP", AT_DP, READ | WRITE       ,  2, (uint64_t*) &val_01, (uint64_t*) &val_68B0  , SET_sleepmCMD_dp, max_u32val      },

	{ "ATEA", AT_EA, READ               ,  0,																				  }, // E
	{ "ATEC", AT_EC, READ               ,  0,																				  },
	{ "ATEE", AT_EE, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_01    , SET_secCMD_ee   , max_u32val      },

	{ "ATGT", AT_GT, READ | WRITE       ,  2, (uint64_t*) &val_02, (uint64_t*) &val_CE4   , SET_atcopCMD_gt , max_u32val      }, // G

	{ "ATHV", AT_HV, READ | WRITE       ,  2, (uint64_t*) &val_00, (uint64_t*) &val_u16_FF, SET_diagCMD_hv  , max_u32val      }, // H

	{ "ATIA", AT_IA, READ | WRITE       ,  8, (uint64_t*) &val_00, (uint64_t*) &val_u64_FF, SET_iolpCMD_ia  , max_u64val      }, // I
	{ "ATIC", AT_IC, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_ioserCMD_ic , max_u32val      },
	{ "ATID", AT_ID, READ | WRITE       ,  2, (uint64_t*) &val_00, (uint64_t*) &val_u16_FF, SET_netCMD_id   , max_u32val      },
	{ "ATIR", AT_IR, READ | WRITE       ,  2, (uint64_t*) &val_00, (uint64_t*) &val_u16_FF, SET_ioserCMD_ir , max_u32val      },
	{ "ATIT", AT_IT, READ | WRITE       ,  2, (uint64_t*) &val_01, (uint64_t*) &val_u16_FF, SET_ioserCMD_it , max_u32val      },
	{ "ATIU", AT_IU, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_01    , SET_ioserCMD_iu , max_u32val      },

	{ "ATKY", AT_KY,		WRITE | EXEC, 16, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_secCMD_ky   , ky_validator    }, // K

	{ "ATMM", AT_MM, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_03    , SET_netCMD_mm   , max_u32val      }, // M
	{ "ATMY", AT_MY, READ | WRITE       ,  2, (uint64_t*) &val_00, (uint64_t*) &val_u16_FF, SET_netCMD_my   , max_u32val      },

	{ "ATNB", AT_NB, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_04    , SET_serintCMD_nb, max_u32val      }, // N
	{ "ATNI", AT_NI, READ | WRITE       , 20, NULL               , (uint64_t*) &val_14    , SET_netCMD_ni   , node_identifier },
	{ "ATNO", AT_NO, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_01    , SET_netCMD_no   , max_u32val      },
	{ "ATNT", AT_NT, READ | WRITE       ,  1, (uint64_t*) &val_01, (uint64_t*) &val_FC    , SET_netCMD_nt   , max_u32val      },

	{ "ATP0", AT_P0, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_02    , SET_ioserCMD_p0 , max_u32val      }, // P
	{ "ATP1", AT_P1, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_02    , SET_ioserCMD_p1 , max_u32val      },
	{ "ATPL", AT_PL, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_04    , SET_rfiCMD_pl   , max_u32val      },
	{ "ATPR", AT_PR, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_ioserCMD_pr , max_u32val      },
	{ "ATPT", AT_PT, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_ioserCMD_pt , max_u32val      },

	{ "ATR?", AT_Rq, READ               ,  0,																				  }, // R
	{ "ATRE", AT_RE,			    EXEC,  0,																				  },
	{ "ATRN", AT_RN, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_03    , SET_netCMD_rn   , max_u32val      },
	{ "ATRO", AT_RO, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_serintCMD_ro, max_u32val      },
	{ "ATRP", AT_RP, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_ioserCMD_rp , max_u32val      },
	{ "ATRR", AT_RR, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_06    , SET_netCMD_rr   , max_u32val      },

	{ "ATSB", AT_SB, READ               ,  0,																				  }, // S
	{ "ATSC", AT_SC, READ | WRITE       ,  2, (uint64_t*) &val_00, (uint64_t*) &val_u16_FF, SET_netCMD_sc   , max_u32val      },
	{ "ATSD", AT_SD, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_0F    , SET_netCMD_sd   , max_u32val      },
	{ "ATSH", AT_SH, READ               ,  4,																				  },
	{ "ATSL", AT_SL, READ               ,  4,																				  },
	{ "ATSM", AT_SM, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_06    , SET_sleepmCMD_sm, max_u32val      },
	{ "ATSO", AT_SO, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_06    , SET_sleepmCMD_so, max_u32val      },
	{ "ATSP", AT_SP, READ | WRITE       ,  2, (uint64_t*) &val_00, (uint64_t*) &val_68B0  , SET_sleepmCMD_sp, max_u32val      },
	{ "ATSS", AT_SS, READ               ,  0                                                          },
	{ "ATST", AT_ST, READ | WRITE       ,  2, (uint64_t*) &val_01, (uint64_t*) &val_u16_FF, SET_sleepmCMD_st, max_u32val      },

	{ "ATT0", AT_T0, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_iolpCMD_T0  , max_u32val      }, // T
	{ "ATT1", AT_T1, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_iolpCMD_T1  , max_u32val      },
	{ "ATT2", AT_T2, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_iolpCMD_T2  , max_u32val      },
	{ "ATT3", AT_T3, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_iolpCMD_T3  , max_u32val      },
	{ "ATT4", AT_T4, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_iolpCMD_T4  , max_u32val      },
	{ "ATT5", AT_T5, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_iolpCMD_T5  , max_u32val      },
	{ "ATT6", AT_T6, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_iolpCMD_T6  , max_u32val      },
	{ "ATT7", AT_T7, READ | WRITE       ,  1, (uint64_t*) &val_00, (uint64_t*) &val_FF    , SET_iolpCMD_T7  , max_u32val      },

	{ "ATVR", AT_VR, READ | WRITE       ,  2, (uint64_t*) &val_00, (uint64_t*) &val_u16_FF, SET_diagCMD_vr  , max_u32val      }, // V

	{ "ATWR", AT_WR,			    EXEC,  0,																				  }, // W

	/* DE */
	{ "DEFV", DE_FV, READ               ,  0,																				  }, // F

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