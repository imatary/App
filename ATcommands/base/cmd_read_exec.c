/*
 * cmd_read_exec.c
 *
 * Created: CMD_print( 18.01.2017 13: CMD_print(07: CMD_print(13
 *  Author: CMD_print( TOE
 */ 
// === includes ===========================================
#include <inttypes.h>

#include "../header/_global.h"
#include "../header/cmd.h"						// AT command search parser
#include "../header/apiframe.h"					// AP set functions
#include "../header/rfmodul.h"					// RFmodul struct
#include "../../ATuracoli/stackrelated.h"		// uart functions
#include "../../ATuracoli/stackrelated_timer.h"	// timer function

// === prototypes =========================================
static void CMD_print(uint8_t len, uint64_t value, bool_t swap);

// === functions ==========================================
/*
 * CMD_readOrExec() 
 * - read the values of the memory
 * - or execute commands
 *
 * Received: CMD_print(
 *		if in  AT Mode, a time handler pointer else a NULL pointer
 *
 * Returns: CMD_print(
 *		OP_SUCCESS			if command successful accomplished
 *		INVALID_COMMAND		if the command is not in the command table
 *		ERROR				if an process error occurred
 *
 * last modified: CMD_print( 2017/01/18
 */
at_status_t CMD_readOrExec(uint32_t *th, bufType_n bufType) 
{
	CMD *pCommand  = NULL;
	uint8_t pCmdString[5] = {'A','T',0,0,0};
		
	if ( AT_MODE_ACTIVE != GET_serintCMD_ap() ) // AP frame
	{ 
		cli(); deBufferOut( bufType, &pCmdString[2] ); sei();
		cli(); deBufferOut( bufType, &pCmdString[3] ); sei();
		if ( 'a' <= pCmdString[2] && 'z' >= pCmdString[2] ) pCmdString[2] -= 0x20;
		if ( 'a' <= pCmdString[3] && 'z' >= pCmdString[3] ) pCmdString[3] -= 0x20;
		AP_setATcmd(pCmdString);
	}
	else // AT CMD
	{
		for (int i = 0; i < 4 ; i++)
		{
			cli(); deBufferOut( bufType, &pCmdString[i] ); sei();
			if ( 'a' <= pCmdString[2] && 'z' >= pCmdString[i] ) pCmdString[i] -= 0x20;
		}
		// remove the '\0' from the buffer if in AT cmd mode
		deBufferReadReset( bufType, '+', 1);
	}
	
	/*
	 * if there not a valid command, leave function
	 */
	pCommand = CMD_findInTable(pCmdString);
	if ( NO_AT_CMD == pCommand->ID || NULL == pCommand ) return INVALID_COMMAND;
		
	/*
	 * handle CMD
	 * 
	 * exec is allowed
	 */
	if ( pCommand->rwxAttrib & EXEC )
	{
		if ( AT_MODE_ACTIVE != GET_serintCMD_ap() ) AP_setRWXopt(EXEC);	
		switch( pCommand->ID )
		{
			// leave command mode command (only in AT mode)
/* CN */    case AT_CN :
				if ( AT_MODE_ACTIVE == GET_serintCMD_ap() )
				{
					if ( th != NULL) *th = deTIMER_restart(*th, deMSEC( 0x10 ));
					UART_print_status(OP_SUCCESS);
				} 
				else { return INVALID_COMMAND; }
				break;
			
			// write config to firmware
/* WR */    case AT_WR : 
				if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
				SET_userValInEEPROM();
			break;
			
			// apply changes - currently only a dummy
/* AC */    case AT_AC : 
			{
				UART_init();
				TRX_baseInit();	
				SET_serintCMD_ap( GET_atAP_tmp() );
				SET_atcopCMD_ct ( GET_atCT_tmp() );
	
				if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
			}
			break;
			
			// reset all parameter
/* RE */    case AT_RE :
				if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_status(OP_SUCCESS); }
				SET_allDefault();
			break;
				
/* KY */	case AT_KY : if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print("\r"); } break;
			
			default: return INVALID_COMMAND;
		}
	}
	/*
	 * frame length
	 * string length of input
	 * reading the command is allowed
	 */		
	else if ( pCommand->rwxAttrib & READ )
	{
		if ( AT_MODE_ACTIVE != GET_serintCMD_ap() ) AP_setRWXopt(READ);	

		switch( pCommand->ID )
		{
/* CH */	case AT_CH : CMD_print( 1,    GET_netCMD_ch(), FALSE ); break;
/* ID */	case AT_ID : CMD_print( 2,    GET_netCMD_id(),  TRUE ); break;
/* DH */	case AT_DH : CMD_print( 4,    GET_netCMD_dh(),  TRUE ); break;
/* DL */	case AT_DL : CMD_print( 4,    GET_netCMD_dl(),  TRUE ); break;
/* MY */	case AT_MY : CMD_print( 2,    GET_netCMD_my(),  TRUE ); break;
/* SH */	case AT_SH : CMD_print( 4,    GET_netCMD_sh(),  TRUE ); break;
/* SL */	case AT_SL : CMD_print( 4,    GET_netCMD_sl(),  TRUE ); break;			
/* SC */	case AT_SC : CMD_print( 2,    GET_netCMD_sc(),  TRUE ); break;			
/* MM */	case AT_MM : CMD_print( 1,    GET_netCMD_mm(), FALSE ); break;
/* RR */	case AT_RR : CMD_print( 1,    GET_netCMD_rr(), FALSE ); break;
/* RN */	case AT_RN : CMD_print( 1,    GET_netCMD_rn(), FALSE ); break;
/* NT */	case AT_NT : CMD_print( 1,    GET_netCMD_nt(), FALSE ); break;
/* SD */	case AT_SD : CMD_print( 1,    GET_netCMD_sd(), FALSE ); break;
/* A1 */	case AT_A1 : CMD_print( 1,    GET_netCMD_a1(), FALSE ); break;
/* A2 */	case AT_A2 : CMD_print( 1,    GET_netCMD_a2(), FALSE ); break;
/* AI */	case AT_AI : CMD_print( 1,    GET_netCMD_ai(), FALSE ); break;
/* PL */	case AT_PL : CMD_print( 1,    GET_rfiCMD_pl(), FALSE ); break;
/* CA */	case AT_CA : CMD_print( 1,    GET_rfiCMD_ca(), FALSE ); break;
/* SM */	case AT_SM : CMD_print( 1, GET_sleepmCMD_sm(), FALSE ); break;
/* ST */	case AT_ST : CMD_print( 2, GET_sleepmCMD_st(),  TRUE ); break;
/* SP */	case AT_SP : CMD_print( 2, GET_sleepmCMD_sp(),  TRUE ); break;
/* DP */	case AT_DP : CMD_print( 2, GET_sleepmCMD_dp(),  TRUE ); break;
/* SO */	case AT_SO : CMD_print( 1, GET_sleepmCMD_so(), FALSE ); break;
/* BD */    case AT_BD : CMD_print( 1, GET_serintCMD_bd(), FALSE ); break;
/* NB */    case AT_NB : CMD_print( 1, GET_serintCMD_nb(), FALSE ); break;
/* RO */	case AT_RO : CMD_print( 1, GET_serintCMD_ro(), FALSE ); break;
/* D8 */	case AT_D8 : CMD_print( 1,  GET_ioserCMD_d8(), FALSE ); break;
/* D7 */	case AT_D7 : CMD_print( 1,  GET_ioserCMD_d7(), FALSE ); break;
/* D6 */	case AT_D6 : CMD_print( 1,  GET_ioserCMD_d6(), FALSE ); break;
/* D5 */	case AT_D5 : CMD_print( 1,  GET_ioserCMD_d5(), FALSE ); break;
/* D4 */	case AT_D4 : CMD_print( 1,  GET_ioserCMD_d4(), FALSE ); break;
/* D3 */	case AT_D3 : CMD_print( 1,  GET_ioserCMD_d3(), FALSE ); break;
/* D2 */	case AT_D2 : CMD_print( 1,  GET_ioserCMD_d2(), FALSE ); break;
/* D1 */	case AT_D1 : CMD_print( 1,  GET_ioserCMD_d1(), FALSE ); break;
/* D0 */	case AT_D0 : CMD_print( 1,  GET_ioserCMD_d0(), FALSE ); break;
/* PR */	case AT_PR : CMD_print( 1,  GET_ioserCMD_pr(), FALSE ); break;			
/* IT */	case AT_IT : CMD_print( 1,  GET_ioserCMD_it(), FALSE ); break;
/* IC */	case AT_IC : CMD_print( 1,  GET_ioserCMD_ic(), FALSE ); break;
/* IR */	case AT_IR : CMD_print( 2,  GET_ioserCMD_ir(),  TRUE ); break;
/* P0 */	case AT_P0 : CMD_print( 1,  GET_ioserCMD_p0(), FALSE ); break;
/* P1 */	case AT_P1 : CMD_print( 1,  GET_ioserCMD_p1(), FALSE ); break;
/* PT */	case AT_PT : CMD_print( 1,  GET_ioserCMD_pt(), FALSE ); break;
/* RP */	case AT_RP : CMD_print( 1,  GET_ioserCMD_rp(), FALSE ); break;
/* IA */	case AT_IA : CMD_print( 8,   GET_iolpCMD_ia(),  TRUE ); break;
/* T0 */	case AT_T0 : CMD_print( 1,   GET_iolpCMD_T0(), FALSE ); break;
/* T1 */	case AT_T1 : CMD_print( 1,   GET_iolpCMD_T1(), FALSE ); break;
/* T2 */	case AT_T2 : CMD_print( 1,   GET_iolpCMD_T2(), FALSE ); break;
/* T3 */	case AT_T3 : CMD_print( 1,   GET_iolpCMD_T3(), FALSE ); break;			
/* T4 */	case AT_T4 : CMD_print( 1,   GET_iolpCMD_T4(), FALSE ); break;			
/* T5 */	case AT_T5 : CMD_print( 1,   GET_iolpCMD_T5(), FALSE ); break;			
/* T6 */	case AT_T6 : CMD_print( 1,   GET_iolpCMD_T6(), FALSE ); break;			
/* T7 */	case AT_T7 : CMD_print( 1,   GET_iolpCMD_T7(), FALSE ); break;
/* VR */	case AT_VR : CMD_print( 2,   GET_diagCMD_vr(),  TRUE ); break;		
/* HV */	case AT_HV : CMD_print( 2,   GET_diagCMD_hv(),  TRUE ); break;
/* DB */	case AT_DB : CMD_print( 1,   GET_diagCMD_db(), FALSE ); break;
/* EC */	case AT_EC : CMD_print( 2,   GET_diagCMD_ec(),  TRUE ); break;
/* EA */	case AT_EA : CMD_print( 2,   GET_diagCMD_ea(),  TRUE ); break;
/* DD */	case AT_DD : CMD_print( 4,   GET_diagCMD_dd(),  TRUE ); break;
/* GT */	case AT_GT : CMD_print( 2,  GET_atcopCMD_gt(),  TRUE ); break;
/* CC */	case AT_CC : CMD_print( 1,  GET_atcopCMD_cc(), FALSE ); break;		
/* pC*/		case AT_pC : CMD_print( 1,                0x1, FALSE ); break;	

/* SS */	case AT_SS :
/* Rq */	case AT_Rq : 	
/* SB */	case AT_SB : return ERROR;

/* CE */	case AT_CE :
					if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_decimal( GET_netCMD_ce() ); }
					else
					{
						uint8_t val = GET_netCMD_ce();
						AP_setMSG( &val, 1, FALSE );
					}
				break;

/* NO */	case AT_NO :
					if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_decimal( GET_netCMD_no() ); }
					else
					{
						uint8_t val = GET_netCMD_no();
						AP_setMSG( &val, 1, FALSE );
					}
				break;

/* EE */	case AT_EE :
					if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_decimal( GET_secCMD_ee() ); }
					else
					{
						uint8_t val = GET_secCMD_ee();
						AP_setMSG( &val, 1, FALSE );
					}
				break;

/* IU */	case AT_IU :
					if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_print_decimal( GET_ioserCMD_iu() ); }
					else
					{
						uint8_t val = GET_ioserCMD_iu();
						AP_setMSG( &val, 1, FALSE );
					}
				break;

/* AP */	case AT_AP :
					CMD_print( 1, GET_serintCMD_ap(), FALSE );
					SET_atAP_tmp( GET_serintCMD_ap() );
				break;

/* CT */	case AT_CT : 
					CMD_print( 2, GET_atcopCMD_ct(),  TRUE );
					SET_atCT_tmp( GET_atcopCMD_ct() );
				break;

/* NI */	case AT_NI :
					if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_printf("%s\r", GET_netCMD_ni() ); }
					else          { AP_setMSG( GET_netCMD_ni(), strlen( (const char*) GET_netCMD_ni() ), FALSE ); }
				break;

/* FV */	case DE_FV : 
					if ( AT_MODE_ACTIVE == GET_serintCMD_ap() ) { UART_printf("%s\r", AT_VERSION); }
				         /* No AP handle at this place -> check AP_localDevice function */
				break;
			
			default : return INVALID_COMMAND;
		}
	} 
	else
	{
		return ERROR;
	}
	return OP_SUCCESS;
}

/*
 * CMD print send data to
 * - UART if AT mode is active
 * - API frame message buffer
 *
 * Received:
 *		uint8_t		length of transmitted data (in bytes)
 *		uint64_t	data value
 *		bool_t		boolean value whether the transmitted data need to be swapped before printed
 *
 * Returns:
 *		nothing
 *
 * last modified: 2017/01/19
 */
static void CMD_print(uint8_t len, uint64_t value, bool_t swap)
{
	if ( AT_MODE_ACTIVE == GET_serintCMD_ap() )
	{
		UART_print_data( len, value );
	}
	else
	{
		AP_setMSG( &value, len, swap );
	}
}