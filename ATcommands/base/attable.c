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
#include "../../ATuracoli/stackrelated.h" 


static const CMD StdCmdTable[] =
{
	/* AT */
	{ "AT%C", AT_pC, READ               }, // %
	{ "AT%V", AT_pV, READ               },
	
	{ "ATA1", AT_A1, READ | WRITE       }, // A
	{ "ATA2", AT_A2, READ | WRITE       },
	{ "ATAC", AT_AC,			   EXEC },
	{ "ATAI", AT_AI, READ               },
	{ "ATAP", AT_AP, READ | WRITE       },
	
	{ "ATBD", AT_BD, READ | WRITE       }, // B
	
	{ "ATCA", AT_CA, READ | WRITE       }, // C
	{ "ATCC", AT_CC, READ | WRITE       },
	{ "ATCE", AT_CE, READ | WRITE       },
	{ "ATCH", AT_CH, READ | WRITE       },
	{ "ATCN", AT_CN,			   EXEC },
	{ "ATCT", AT_CT, READ | WRITE       },

	{ "ATD0", AT_D0, READ | WRITE       }, // D
	{ "ATD1", AT_D1, READ | WRITE       },
	{ "ATD2", AT_D2, READ | WRITE       },
	{ "ATD3", AT_D3, READ | WRITE       },
	{ "ATD4", AT_D4, READ | WRITE       },
	{ "ATD5", AT_D5, READ | WRITE       },
	{ "ATD6", AT_D6, READ | WRITE       },	
	{ "ATD7", AT_D7, READ | WRITE       },
	{ "ATD8", AT_D8, READ | WRITE       },
	{ "ATDB", AT_DB, READ               }, 
	{ "ATDD", AT_DD, READ | WRITE       },
	{ "ATDH", AT_DH, READ | WRITE       },
	{ "ATDL", AT_DL, READ | WRITE       },
	{ "ATDP", AT_DP, READ | WRITE       },
	
	{ "ATEA", AT_EA, READ               }, // E
	{ "ATEC", AT_EC, READ               },
	{ "ATEE", AT_EE, READ | WRITE       },
	
	{ "ATGT", AT_GT, READ | WRITE       }, // G
	
	{ "ATHV", AT_HV, READ | WRITE       }, // H
	
	{ "ATIA", AT_IA, READ | WRITE       }, // I
	{ "ATIC", AT_IC, READ | WRITE       },
	{ "ATID", AT_ID, READ | WRITE       },
	{ "ATIR", AT_IR, READ | WRITE       },
	{ "ATIT", AT_IT, READ | WRITE       },
	{ "ATIU", AT_IU, READ | WRITE       },
	
	{ "ATKY", AT_KY, READ | WRITE       }, // K
	
	{ "ATMM", AT_MM, READ | WRITE       }, // M
	{ "ATMY", AT_MY, READ | WRITE       },
	
	{ "ATNB", AT_NB, READ | WRITE       }, // N
	{ "ATNI", AT_NI, READ | WRITE       },
	{ "ATNO", AT_NO, READ | WRITE       },
	{ "ATNT", AT_NT, READ | WRITE       },
	 
	{ "ATP0", AT_P0, READ | WRITE       }, // P
	{ "ATP1", AT_P1, READ | WRITE       },
	{ "ATPL", AT_PL, READ | WRITE       },
	{ "ATPR", AT_PR, READ | WRITE       },
	{ "ATPT", AT_PT, READ | WRITE       },
	
	{ "ATR?", AT_Rq, READ               }, // R
	{ "ATRE", AT_RE,			   EXEC },
	{ "ATRN", AT_RN, READ | WRITE       },
	{ "ATRO", AT_RO, READ | WRITE       },
	{ "ATRP", AT_RP, READ | WRITE       },
	{ "ATRR", AT_RR, READ | WRITE       },
	
	{ "ATSB", AT_SB, READ               }, // S
	{ "ATSC", AT_SC, READ | WRITE       },
	{ "ATSD", AT_SD, READ | WRITE       },
	{ "ATSH", AT_SH, READ               },
	{ "ATSL", AT_SL, READ               },
	{ "ATSM", AT_SM, READ | WRITE       },
	{ "ATSO", AT_SO, READ | WRITE       },
	{ "ATSP", AT_SP, READ | WRITE       },
	{ "ATSS", AT_SS, READ               },
	{ "ATST", AT_ST, READ | WRITE       },
		
	{ "ATT0", AT_T0, READ | WRITE       }, // T
	{ "ATT1", AT_T1, READ | WRITE       },
	{ "ATT2", AT_T2, READ | WRITE       },
	{ "ATT3", AT_T3, READ | WRITE       },
	{ "ATT4", AT_T4, READ | WRITE       },
	{ "ATT5", AT_T5, READ | WRITE       },
	{ "ATT6", AT_T6, READ | WRITE       },
	{ "ATT7", AT_T7, READ | WRITE       },

	{ "ATVR", AT_VR, READ | WRITE       }, // V
	
	{ "ATWR", AT_WR,			   EXEC }, // W
	
	/* DE */
	{ "DERU", DE_RU, READ | WRITE       },
	{ "DEFV", DE_RU, READ               },

};

const CMD *pStdCmdTable = StdCmdTable;

/*
 * command table size
 * - the number of commands which are stored in the command table 
 */
const size_t command_count = sizeof(StdCmdTable)/sizeof(CMD);

/*
 * Binary search function to find command in table
 * halved the table every time, runtime ~ O( log(n) )
 *
 * IMPORTANT! : table needs to be sorted alphabetically
 *
 * Received:
 *		uint8_t pointer to the array which hold the AT command line
 *
 * Returns:
 *		CMD			pointer to command position in the command table
 *		NO_AT_CMD	if there not a command available
 *
 * last modified: 2016/12/19
 */
CMD *CMD_findInTable(uint8_t *cmd)
{
	int right  = (int) command_count-1;
	int middle;
	int left = 0;
	int val  = 0;
	
	while( left <= right )
	{
		middle = left + ( ( right - left) /2 );
		val = strncmp( (const char*) cmd, (pStdCmdTable+middle)->name, 4 );
		if ( 0 == val ) 
		{
			return (CMD*) (pStdCmdTable+middle);
		}
		
		else if ( 0 > val )    right = middle - 1;
		else /* ( 0 < val ) */ left  = middle + 1;
		
	}
	return NO_AT_CMD;
}