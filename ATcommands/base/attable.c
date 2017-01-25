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
#include "../header/atlocal.h"
#include "../header/rfmodul.h"
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
	/* name ,  ID  ,  addr at offset  ,  rwx option       , max size , min ,     max   , set functions   , validation    , print function */     
	/* AT */
	{ "AT%C", AT_pC,        NO_OFFSET , READ              ,  U8__SIZE,													   }, // %
	{ "AT%V", AT_pV,        NO_OFFSET , READ              ,  0,														       },			// TODO ZigBee

	{ "ATA1", AT_A1, GET_offsetof_a1(), READ | WRITE      ,  U8__SIZE, 0x00,       0x0F, SET_netCMD_a1   , max_u32val      }, // A
	{ "ATA2", AT_A2, GET_offsetof_a2(), READ | WRITE      ,  U8__SIZE, 0x00,       0x07, SET_netCMD_a1   , max_u32val      },
	{ "ATAC", AT_AC,        NO_OFFSET ,			      EXEC,  0,				      		 						           },
	{ "ATAI", AT_AI, GET_offsetof_ai(), READ              ,  U8__SIZE,		      		 								   },
	{ "ATAP", AT_AP, GET_offsetof_ap(), READ | WRITE      ,  U8__SIZE, 0x00,       0x02, SET_atAP_tmp    , max_u32val      },
																	         	   
	{ "ATBD", AT_BD, GET_offsetof_bd(), READ | WRITE      ,  U8__SIZE, 0x00,       0x07, SET_serintCMD_bd, max_u32val      }, // B
																	         	   
	{ "ATCA", AT_CA, GET_offsetof_ca(), READ | WRITE      ,  U8__SIZE, 0x24,       0x50, SET_rfiCMD_ca   ,                 }, // C
	{ "ATCC", AT_CC, GET_offsetof_cc(), READ | WRITE      ,  U8__SIZE, 0x00,       0x00, SET_atcopCMD_cc , max_u32val      },
	{ "ATCE", AT_CE, GET_offsetof_ce(), READ | WRITE      ,  U8__SIZE, 0x00,       0x01, SET_netCMD_ce   , max_u32val      },
	{ "ATCH", AT_CH, GET_offsetof_ch(), READ | WRITE      ,  U8__SIZE, 0x0B,       0x1A, SET_netCMD_ch   , max_u32val      },
	{ "ATCN", AT_CN,        NO_OFFSET ,			      EXEC,  0,														       },
	{ "ATCT", AT_CT, GET_offsetof_ct(), READ | WRITE      ,  U16_SIZE, 0x02,     0x1770, SET_atCT_tmp    , max_u32val      },

	{ "ATD0", AT_D0, GET_offsetof_d0(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d0 , max_u32val      }, // D
	{ "ATD1", AT_D1, GET_offsetof_d1(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d1 , max_u32val      },
	{ "ATD2", AT_D2, GET_offsetof_d2(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d2 , max_u32val      },
	{ "ATD3", AT_D3, GET_offsetof_d3(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d3 , max_u32val      },
	{ "ATD4", AT_D4, GET_offsetof_d4(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d4 , max_u32val      },
	{ "ATD5", AT_D5, GET_offsetof_d5(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d5 , max_u32val      },
	{ "ATD6", AT_D6, GET_offsetof_d6(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d6 , max_u32val      },
	{ "ATD7", AT_D7, GET_offsetof_d7(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d7 , max_u32val      },
	{ "ATD8", AT_D8, GET_offsetof_d8(), READ | WRITE      ,  U8__SIZE, 0x00,       0x05, SET_ioserCMD_d8 , max_u32val      },
	{ "ATDB", AT_DB, GET_offsetof_db(), READ              ,  U8__SIZE,													   },
	{ "ATDD", AT_DD, GET_offsetof_dd(), READ | WRITE      ,  U32_SIZE, 0x00, 0xFFFFFFFF, SET_diagCMD_dd  , max_u32val      },
	{ "ATDH", AT_DH, GET_offsetof_dh(), READ | WRITE      ,  U32_SIZE, 0x00, 0xFFFFFFFF, SET_netCMD_dh   , max_u32val      },
	{ "ATDL", AT_DL, GET_offsetof_dl(), READ | WRITE      ,  U32_SIZE, 0x00, 0xFFFFFFFF, SET_netCMD_dl   , max_u32val      },
	{ "ATDP", AT_DP, GET_offsetof_dp(), READ | WRITE      ,  U16_SIZE, 0x01, 0x68B0    , SET_sleepmCMD_dp, max_u32val      },

	{ "ATEA", AT_EA, GET_offsetof_ea(), READ              ,  U16_SIZE,													   }, // E
	{ "ATEC", AT_EC, GET_offsetof_ec(), READ              ,  U16_SIZE,													   },
	{ "ATEE", AT_EE, GET_offsetof_ee(), READ | WRITE      ,  U8__SIZE, 0x00,       0x01, SET_secCMD_ee   , max_u32val      },

	{ "ATGT", AT_GT, GET_offsetof_gt(), READ | WRITE      ,  U16_SIZE, 0x02,      0xCE4, SET_atcopCMD_gt , max_u32val      }, // G

	{ "ATHV", AT_HV, GET_offsetof_hv(), READ | WRITE      ,  U16_SIZE, 0x00,     0xFFFF, SET_diagCMD_hv  , max_u32val      }, // H

	{ "ATIA", AT_IA, GET_offsetof_ia(), READ | WRITE      ,  U64_SIZE, 0x00,       0x00, SET_iolpCMD_ia  , max_u64val      }, // I
	{ "ATIC", AT_IC, GET_offsetof_ic(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_ioserCMD_ic , max_u32val      },
	{ "ATID", AT_ID, GET_offsetof_id(), READ | WRITE      ,  U16_SIZE, 0x00,     0xFFFF, SET_netCMD_id   , max_u32val      },
	{ "ATIR", AT_IR, GET_offsetof_ir(), READ | WRITE      ,  U16_SIZE, 0x00,     0xFFFF, SET_ioserCMD_ir , max_u32val      },
	{ "ATIT", AT_IT, GET_offsetof_it(), READ | WRITE      ,  U16_SIZE, 0x01,     0xFFFF, SET_ioserCMD_it , max_u32val      },
	{ "ATIU", AT_IU, GET_offsetof_iu(), READ | WRITE      ,  U8__SIZE, 0x00,       0x01, SET_ioserCMD_iu , max_u32val      },

	{ "ATKY", AT_KY,        NO_OFFSET ,		WRITE |   EXEC,        16, 0x00,       0xFF, SET_secCMD_ky   , ky_validator    }, // K

	{ "ATMM", AT_MM, GET_offsetof_mm(), READ | WRITE      ,  U8__SIZE, 0x00,       0x03, SET_netCMD_mm   , max_u32val      }, // M
	{ "ATMY", AT_MY, GET_offsetof_my(), READ | WRITE      ,  U16_SIZE, 0x00,     0xFFFF, SET_netCMD_my   , max_u32val      },

	{ "ATNB", AT_NB, GET_offsetof_nb(), READ | WRITE      ,  U8__SIZE, 0x00,       0x04, SET_serintCMD_nb, max_u32val      }, // N
	{ "ATNI", AT_NI, GET_offsetof_ni(), READ | WRITE      ,        20, 0x00,       0x00, SET_netCMD_ni   , node_identifier },
	{ "ATNO", AT_NO, GET_offsetof_no(), READ | WRITE      ,  U8__SIZE, 0x00,       0x01, SET_netCMD_no   , max_u32val      },
	{ "ATNT", AT_NT, GET_offsetof_nt(), READ | WRITE      ,  U8__SIZE, 0x01,       0xFC, SET_netCMD_nt   , max_u32val      },

	{ "ATP0", AT_P0, GET_offsetof_p0(), READ | WRITE      ,  U8__SIZE, 0x00,       0x02, SET_ioserCMD_p0 , max_u32val      }, // P
	{ "ATP1", AT_P1, GET_offsetof_p1(), READ | WRITE      ,  U8__SIZE, 0x00,       0x02, SET_ioserCMD_p1 , max_u32val      },
	{ "ATPL", AT_PL, GET_offsetof_pl(), READ | WRITE      ,  U8__SIZE, 0x00,       0x04, SET_rfiCMD_pl   , max_u32val      },
	{ "ATPR", AT_PR, GET_offsetof_pr(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_ioserCMD_pr , max_u32val      },
	{ "ATPT", AT_PT, GET_offsetof_pt(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_ioserCMD_pt , max_u32val      },

	{ "ATR?", AT_Rq,        NO_OFFSET , READ              ,  0,														       }, // R
	{ "ATRE", AT_RE,        NO_OFFSET ,			      EXEC,  0,},
	{ "ATRN", AT_RN, GET_offsetof_rn(), READ | WRITE      ,  U8__SIZE, 0x00,       0x03, SET_netCMD_rn   , max_u32val      },
	{ "ATRO", AT_RO, GET_offsetof_ro(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_serintCMD_ro, max_u32val      },
	{ "ATRP", AT_RP, GET_offsetof_rp(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_ioserCMD_rp , max_u32val      },
	{ "ATRR", AT_RR, GET_offsetof_rr(), READ | WRITE      ,  U8__SIZE, 0x00,       0x06, SET_netCMD_rr   , max_u32val      },

	{ "ATSB", AT_SB,        NO_OFFSET , READ              ,  0,														       }, // S
	{ "ATSC", AT_SC, GET_offsetof_sc(), READ | WRITE      ,  U16_SIZE, 0x00,     0xFFFF, SET_netCMD_sc   , max_u32val      },
	{ "ATSD", AT_SD, GET_offsetof_sd(), READ | WRITE      ,  U8__SIZE, 0x00,       0x0F, SET_netCMD_sd   , max_u32val      },
	{ "ATSH", AT_SH, GET_offsetof_sh(), READ              ,  U32_SIZE,													   },
	{ "ATSL", AT_SL, GET_offsetof_sl(), READ              ,  U32_SIZE,													   },
	{ "ATSM", AT_SM, GET_offsetof_sm(), READ | WRITE      ,  U8__SIZE, 0x00,       0x06, SET_sleepmCMD_sm, max_u32val      },
	{ "ATSO", AT_SO, GET_offsetof_so(), READ | WRITE      ,  U8__SIZE, 0x00,       0x06, SET_sleepmCMD_so, max_u32val      },
	{ "ATSP", AT_SP, GET_offsetof_sp(), READ | WRITE      ,  U16_SIZE, 0x00,     0x68B0, SET_sleepmCMD_sp, max_u32val      },
	{ "ATSS", AT_SS,        NO_OFFSET , READ              ,  0,														       },
	{ "ATST", AT_ST, GET_offsetof_st(), READ | WRITE      ,  U16_SIZE, 0x01,     0xFFFF, SET_sleepmCMD_st, max_u32val      },

	{ "ATT0", AT_T0, GET_offsetof_t0(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t0  , max_u32val      }, // T
	{ "ATT1", AT_T1, GET_offsetof_t1(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t1  , max_u32val      },
	{ "ATT2", AT_T2, GET_offsetof_t2(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t2  , max_u32val      },
	{ "ATT3", AT_T3, GET_offsetof_t3(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t3  , max_u32val      },
	{ "ATT4", AT_T4, GET_offsetof_t4(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t4  , max_u32val      },
	{ "ATT5", AT_T5, GET_offsetof_t5(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t5  , max_u32val      },
	{ "ATT6", AT_T6, GET_offsetof_t6(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t6  , max_u32val      },
	{ "ATT7", AT_T7, GET_offsetof_t7(), READ | WRITE      ,  U8__SIZE, 0x00,       0xFF, SET_iolpCMD_t7  , max_u32val      },

	{ "ATVR", AT_VR, GET_offsetof_iu(), READ | WRITE      ,  U16_SIZE, 0x00,     0xFFFF, SET_diagCMD_vr  , max_u32val      }, // V

	{ "ATWR", AT_WR,      NO_OFFSET ,			      EXEC,  0,														       }, // W

	/* DE */
	{ "DEFV", DE_FV,      NO_OFFSET , READ                ,  0,														       }, // F

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
 * Get command 
 * - reads a string from buffer
 * - searched for the command in the command table
 * - store it into pCommand pointer
 * 
 * Received:
 *		bufType_n	number of buffer type
 *		CMD			pointer for address in command table
 *		
 * Returns:
 *     OP_SUCCESS			on success
 *	   INVALID_PARAMETER	if parameter is not valid or error has occurred during transforming to hex
 *
 * last modified: 2017/01/25
 */
at_status_t CMD_getCommand( bufType_n bufType, CMD *pCommand, const device_mode devMode )
{
	static uint8_t cmdString[5];
	 
	if ( devMode != GET_serintCMD_ap() ) // AP frame
	{
		cmdString[0] = 'A';
		cmdString[1] = 'T';
		deBufferOut( bufType, &cmdString[2] );
		deBufferOut( bufType, &cmdString[3] );
		if ( 'a' <= cmdString[2] && 'z' >= cmdString[2] ) cmdString[2] -= 0x20;
		if ( 'a' <= cmdString[3] && 'z' >= cmdString[3] ) cmdString[3] -= 0x20;
		AP_setATcmd(cmdString);
	}
	else // AT CMD
	{
		for (int i = 0; i < 4 ; i++)
		{
			deBufferOut( bufType, &cmdString[i] );
			if ( 'a' <= cmdString[i] && 'z' >= cmdString[i] ) cmdString[i] -= 0x20;
		}
	}
	
	pCommand = CMD_findInTable(cmdString);
	return ( NO_AT_CMD == pCommand->ID || NULL == pCommand )? INVALID_COMMAND : OP_SUCCESS;
}