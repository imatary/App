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
// === includes ===========================================
#include <stdlib.h>
#include <string.h>
#include "../header/cmd.h"
#include "../header/at_commands.h"
#include "../header/rfmodul.h"
#include "../header/rfmodul.h"
#include "../header/helper.h"
#include "../../ATuracoli/stackrelated.h"


// === min / max values ===================================
#define  U8__SIZE 1
#define  U16_SIZE 2
#define  U32_SIZE 4
#define  U64_SIZE 8

#define NO_OFFSET 911

// === command table ======================================
static const CMD StdCmdTable[] =
{
	/* name ,  ID  ,  addr at offset                   ,  rwx option        , max size , min ,     max   , set functions   , validation     */
    /* AT */
	{ "AT%C", AT_pC,                         NO_OFFSET , READ               ,  U8__SIZE,													 }, // %
	{ "AT%V", AT_pV,                         NO_OFFSET , READ               ,  0,														     },			// TODO ZigBee

	{ "ATA1", AT_A1, offsetof( device_t, netCMD_a1    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x0F, SET_netCMD_a1   , max_u32val      }, // A
	{ "ATA2", AT_A2, offsetof( device_t, netCMD_a2    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x07, SET_netCMD_a1   , max_u32val      },
	{ "ATAC", AT_AC,                         NO_OFFSET ,			    EXEC,  0,				      		 						         },
	{ "ATAI", AT_AI, offsetof( device_t, netCMD_ai    ), READ               ,  U8__SIZE,		      		 								 },
	{ "ATAP", AT_AP, offsetof( device_t, serintCMD_ap ), READ | WRITE       ,  U8__SIZE, 0x00,       0x02, SET_atAP_tmp    , max_u32val      },

	{ "ATBD", AT_BD, offsetof( device_t, serintCMD_bd ), READ | WRITE       ,  U8__SIZE, 0x00,       0x07, SET_serintCMD_bd, max_u32val      }, // B

	{ "ATCA", AT_CA, offsetof( device_t, rfiCMD_ca    ), READ | WRITE       ,  U8__SIZE, 0x24,       0x50, SET_rfiCMD_ca   , max_u32val      }, // C
	{ "ATCC", AT_CC, offsetof( device_t, atcopCMD_cc  ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_atcopCMD_cc , max_u32val      },
	{ "ATCE", AT_CE, offsetof( device_t, netCMD_ce    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x01, SET_netCMD_ce   , max_u32val      },
	{ "ATCH", AT_CH, offsetof( device_t, netCMD_ch    ), READ | WRITE       ,  U8__SIZE, 0x0B,       0x1A, SET_netCMD_ch   , max_u32val      },
	{ "ATCN", AT_CN,                         NO_OFFSET ,			    EXEC,  0,														     },
	{ "ATCT", AT_CT, offsetof( device_t, atcopCMD_ct  ), READ | WRITE       ,  U16_SIZE, 0x02,     0x1770, SET_atCT_tmp    , max_u32val      },

	{ "ATD0", AT_D0, offsetof( device_t, ioserCMD_d0  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d0 , max_u32val      }, // D
	{ "ATD1", AT_D1, offsetof( device_t, ioserCMD_d1  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d1 , max_u32val      },
	{ "ATD2", AT_D2, offsetof( device_t, ioserCMD_d2  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d2 , max_u32val      },
	{ "ATD3", AT_D3, offsetof( device_t, ioserCMD_d3  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d3 , max_u32val      },
	{ "ATD4", AT_D4, offsetof( device_t, ioserCMD_d4  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d4 , max_u32val      },
	{ "ATD5", AT_D5, offsetof( device_t, ioserCMD_d5  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d5 , max_u32val      },
	{ "ATD6", AT_D6, offsetof( device_t, ioserCMD_d6  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d6 , max_u32val      },
	{ "ATD7", AT_D7, offsetof( device_t, ioserCMD_d7  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d7 , max_u32val      },
	{ "ATD8", AT_D8, offsetof( device_t, ioserCMD_d8  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d8 , max_u32val      },
	{ "ATDB", AT_DB, offsetof( device_t, diagCMD_db   ), READ               ,  U8__SIZE,													 },
	{ "ATDD", AT_DD, offsetof( device_t, diagCMD_dd   ), READ | WRITE       ,  U32_SIZE, 0x00, 0xFFFFFFFF, SET_diagCMD_dd  , max_u32val      },
	{ "ATDH", AT_DH, offsetof( device_t, netCMD_dh    ), READ | WRITE       ,  U32_SIZE, 0x00, 0xFFFFFFFF, SET_netCMD_dh   , max_u32val      },
	{ "ATDL", AT_DL, offsetof( device_t, netCMD_dl    ), READ | WRITE       ,  U32_SIZE, 0x00, 0xFFFFFFFF, SET_netCMD_dl   , max_u32val      },
	{ "ATDP", AT_DP, offsetof( device_t, sleepmCMD_dp ), READ | WRITE       ,  U16_SIZE, 0x01,     0x68B0, SET_sleepmCMD_dp, max_u32val      },

	{ "ATEA", AT_EA, offsetof( device_t, diagCMD_ea   ), READ               ,  U16_SIZE,													 }, // E
	{ "ATEC", AT_EC, offsetof( device_t, diagCMD_ec   ), READ               ,  U16_SIZE,													 },
	{ "ATEE", AT_EE, offsetof( device_t, secCMD_ee    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x01, SET_secCMD_ee   , max_u32val      },

	{ "ATGT", AT_GT, offsetof( device_t, atcopCMD_gt  ), READ | WRITE       ,  U16_SIZE, 0x02,      0xCE4, SET_atcopCMD_gt , max_u32val      }, // G

	{ "ATHV", AT_HV, offsetof( device_t, diagCMD_hv   ), READ | WRITE       ,  U16_SIZE, 0x00,     0xFFFF, SET_diagCMD_hv  , max_u32val      }, // H

	{ "ATIA", AT_IA, offsetof( device_t, iolpCMD_ia   ), READ | WRITE       ,  U64_SIZE, 0x00,       0x00, SET_iolpCMD_ia  , max_u64val      }, // I
	{ "ATIC", AT_IC, offsetof( device_t, ioserCMD_ic  ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_ioserCMD_ic , max_u32val      },
	{ "ATID", AT_ID, offsetof( device_t, netCMD_id    ), READ | WRITE       ,  U16_SIZE, 0x00,     0xFFFF, SET_netCMD_id   , max_u32val      },
	{ "ATIR", AT_IR, offsetof( device_t, ioserCMD_ir  ), READ | WRITE       ,  U16_SIZE, 0x00,     0xFFFF, SET_ioserCMD_ir , max_u32val      },
	{ "ATIT", AT_IT, offsetof( device_t, ioserCMD_it  ), READ | WRITE       ,  U8__SIZE, 0x01,       0xFF, SET_ioserCMD_it , max_u32val      },
	{ "ATIU", AT_IU, offsetof( device_t, ioserCMD_iu  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x01, SET_ioserCMD_iu , max_u32val      },

	{ "ATKY", AT_KY, offsetof( device_t, secCMD_ky    ),		WRITE | EXEC,        16, 0x00,       0xFF, SET_secCMD_ky   , ky_validator    }, // K

	{ "ATMM", AT_MM, offsetof( device_t, netCMD_mm    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x03, SET_netCMD_mm   , max_u32val      }, // M
	{ "ATMY", AT_MY, offsetof( device_t, netCMD_my    ), READ | WRITE       ,  U16_SIZE, 0x00,     0xFFFF, SET_netCMD_my   , max_u32val      },

	{ "ATNB", AT_NB, offsetof( device_t, serintCMD_nb ), READ | WRITE       ,  U8__SIZE, 0x00,       0x04, SET_serintCMD_nb, max_u32val      }, // N
	{ "ATNI", AT_NI, offsetof( device_t, netCMD_ni    ), READ | WRITE       ,        20, 0x00,       0x00, SET_netCMD_ni   , node_identifier },
	{ "ATNO", AT_NO, offsetof( device_t, netCMD_no    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x01, SET_netCMD_no   , max_u32val      },
	{ "ATNT", AT_NT, offsetof( device_t, netCMD_nt    ), READ | WRITE       ,  U8__SIZE, 0x01,       0xFC, SET_netCMD_nt   , max_u32val      },

	{ "ATP0", AT_P0, offsetof( device_t, ioserCMD_p0  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x02, SET_ioserCMD_p0 , max_u32val      }, // P
	{ "ATP1", AT_P1, offsetof( device_t, ioserCMD_p1  ), READ | WRITE       ,  U8__SIZE, 0x00,       0x02, SET_ioserCMD_p1 , max_u32val      },
	{ "ATPL", AT_PL, offsetof( device_t, rfiCMD_pl    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x03, SET_rfiCMD_pl   , max_u32val      },
	{ "ATPR", AT_PR, offsetof( device_t, ioserCMD_pr  ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_ioserCMD_pr , max_u32val      },
	{ "ATPT", AT_PT, offsetof( device_t, ioserCMD_pt  ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_ioserCMD_pt , max_u32val      },

	{ "ATR?", AT_Rq,                         NO_OFFSET , READ               ,  0,														     }, // R
	{ "ATRE", AT_RE,                         NO_OFFSET ,			    EXEC,  0,                                                            },
	{ "ATRN", AT_RN, offsetof( device_t, netCMD_rn    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x03, SET_netCMD_rn   , max_u32val      },
	{ "ATRO", AT_RO, offsetof( device_t, serintCMD_ro ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_serintCMD_ro, max_u32val      },
	{ "ATRP", AT_RP, offsetof( device_t, ioserCMD_rp  ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_ioserCMD_rp , max_u32val      },
	{ "ATRR", AT_RR, offsetof( device_t, netCMD_rr    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x06, SET_netCMD_rr   , max_u32val      },

	{ "ATSB", AT_SB,                         NO_OFFSET , READ               ,  0,														     }, // S
	{ "ATSC", AT_SC, offsetof( device_t, netCMD_sc    ), READ | WRITE       ,  U16_SIZE, 0x00,     0xFFFF, SET_netCMD_sc   , max_u32val      },
	{ "ATSD", AT_SD, offsetof( device_t, netCMD_sd    ), READ | WRITE       ,  U8__SIZE, 0x00,       0x0F, SET_netCMD_sd   , max_u32val      },
	{ "ATSH", AT_SH, offsetof( device_t, netCMD_sh    ), READ               ,  U32_SIZE,													 },
	{ "ATSL", AT_SL, offsetof( device_t, netCMD_sl    ), READ               ,  U32_SIZE,													 },
	{ "ATSM", AT_SM, offsetof( device_t, sleepmCMD_sm ), READ | WRITE       ,  U8__SIZE, 0x00,       0x06, SET_sleepmCMD_sm, max_u32val      },
	{ "ATSO", AT_SO, offsetof( device_t, sleepmCMD_so ), READ | WRITE       ,  U8__SIZE, 0x00,       0x06, SET_sleepmCMD_so, max_u32val      },
	{ "ATSP", AT_SP, offsetof( device_t, sleepmCMD_sp ), READ | WRITE       ,  U16_SIZE, 0x00,     0x68B0, SET_sleepmCMD_sp, max_u32val      },
	{ "ATSS", AT_SS,                         NO_OFFSET , READ               ,  0,														     },
	{ "ATST", AT_ST, offsetof( device_t, sleepmCMD_st ), READ | WRITE       ,  U16_SIZE, 0x01,     0xFFFF, SET_sleepmCMD_st, max_u32val      },

	{ "ATT0", AT_T0, offsetof( device_t, iolpCMD_t0   ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t0  , max_u32val      }, // T
	{ "ATT1", AT_T1, offsetof( device_t, iolpCMD_t1   ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t1  , max_u32val      },
	{ "ATT2", AT_T2, offsetof( device_t, iolpCMD_t2   ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t2  , max_u32val      },
	{ "ATT3", AT_T3, offsetof( device_t, iolpCMD_t3   ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t3  , max_u32val      },
	{ "ATT4", AT_T4, offsetof( device_t, iolpCMD_t4   ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t4  , max_u32val      },
	{ "ATT5", AT_T5, offsetof( device_t, iolpCMD_t5   ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t5  , max_u32val      },
	{ "ATT6", AT_T6, offsetof( device_t, iolpCMD_t6   ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t6  , max_u32val      },
	{ "ATT7", AT_T7, offsetof( device_t, iolpCMD_t7   ), READ | WRITE       ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t7  , max_u32val      },

	{ "ATVR", AT_VR, offsetof( device_t, diagCMD_vr   ), READ               ,  U16_SIZE, 0x00,     0xFFFF, SET_diagCMD_vr  , max_u32val      }, // V

	{ "ATWR", AT_WR,                         NO_OFFSET ,			    EXEC,  0,														     }, // W

    /* DE */
	{ "DEPT", DE_PT,                         NO_OFFSET , READ               ,  0,														     }, // P (Prototype Version full)
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



/*
 * Binary search function to find command in table
 * halved the table every time, runtime ~ O( log(n) )
 *
 * IMPORTANT! : table needs to be sorted alphabetically
 *
 * Received:
 *		cmdIDs		command id which is requested
 *
 * Returns:
 *		CMD			pointer to command position in the command table
 *		NO_AT_CMD	if there not a command available
 *
 * last modified: 2016/12/19
 */
CMD *CMD_findInTableByID(cmdIDs id)
{
	int right  = ( sizeof(StdCmdTable)/sizeof(CMD) ) - 1; // count of elements in CMD table - 1
	int middle;
	int left = 0;
	cmdIDs val;

	while( left <= right )
	{
		middle = left + ( ( right - left) /2 );
		val = StdCmdTable[middle].ID;
		if      ( id == val )   return (CMD*) &StdCmdTable[middle];
		else if ( id < val )    right = middle - 1;
		else /* ( 0 > val ) */ left  = middle + 1;

	}
	return NO_AT_CMD;
}