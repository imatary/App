/*
 * cmd_read.c
 *
 * Created: 18.01.2017 13:07:13
 *  Author: TOE
 */ 
// === includes ===========================================
#include <inttypes.h>

#include "../header/_global.h"
#include "../header/cmd.h"						// AT command search parser
#include "../header/apiframe.h"					// AP set functions
#include "../header/rfmodul.h"					// RFmodul struct
#include "../../ATuracoli/stackrelated.h"		// uart functions

#define DECIMAL_SIZE 0
// === prototypes =========================================
static void CMD_print(size_t len, uint64_t value);

// === functions ==========================================


/*
 * Command read function reads the values of the memory
 *
 * Received:
 *		CMD		pointer to AT command in command table
 *
 * Returns:
 *		OP_SUCCESS			if command successful accomplished
 *		ERROR				if one of these commands SS, R?, SB 
 *
 * last modified: 2017/01/25
 */
at_status_t CMD_read( CMD *pCommand )
{
	if ( TRANSPARENT_MODE != GET_serintCMD_ap() ) AP_setRWXopt(READ);	

	switch( pCommand->ID )
	{
/* CH */	case AT_CH : CMD_print( pCommand->cmdSize, GET_netCMD_ch() );     break;
/* ID */	case AT_ID : CMD_print( pCommand->cmdSize, GET_netCMD_id() );     break;
/* DH */	case AT_DH : CMD_print( pCommand->cmdSize, GET_netCMD_dh() );	 break;
/* DL */	case AT_DL : CMD_print( pCommand->cmdSize, GET_netCMD_dl() );	 break;
/* MY */	case AT_MY : CMD_print( pCommand->cmdSize, GET_netCMD_my() );	 break;
/* SH */	case AT_SH : CMD_print( pCommand->cmdSize, GET_netCMD_sh() );	 break;
/* SL */	case AT_SL : CMD_print( pCommand->cmdSize, GET_netCMD_sl() );	 break;
/* CE */	case AT_CE : CMD_print(      DECIMAL_SIZE, GET_netCMD_ce() );	 break;
/* SC */	case AT_SC : CMD_print( pCommand->cmdSize, GET_netCMD_sc() );	 break;
/* MM */	case AT_MM : CMD_print( pCommand->cmdSize, GET_netCMD_mm() );	 break;
/* RR */	case AT_RR : CMD_print( pCommand->cmdSize, GET_netCMD_rr() );	 break;
/* RN */	case AT_RN : CMD_print( pCommand->cmdSize, GET_netCMD_rn() );	 break;
/* NT */	case AT_NT : CMD_print( pCommand->cmdSize, GET_netCMD_nt() );	 break;
/* NO */	case AT_NO : CMD_print(      DECIMAL_SIZE, GET_netCMD_no() );	 break;
/* SD */	case AT_SD : CMD_print( pCommand->cmdSize, GET_netCMD_sd() );	 break;
/* A1 */	case AT_A1 : CMD_print( pCommand->cmdSize, GET_netCMD_a1() );	 break;
/* A2 */	case AT_A2 : CMD_print( pCommand->cmdSize, GET_netCMD_a2() );	 break;
/* AI */	case AT_AI : CMD_print( pCommand->cmdSize, GET_netCMD_ai() );	 break;
/* EE */	case AT_EE : CMD_print(      DECIMAL_SIZE, GET_secCMD_ee() );	 break;
/* PL */	case AT_PL : CMD_print( pCommand->cmdSize, GET_rfiCMD_pl() );	 break;
/* CA */	case AT_CA : CMD_print( pCommand->cmdSize, GET_rfiCMD_ca() );	 break;
/* SM */	case AT_SM : CMD_print( pCommand->cmdSize, GET_sleepmCMD_sm() ); break;
/* ST */	case AT_ST : CMD_print( pCommand->cmdSize, GET_sleepmCMD_st() ); break;
/* SP */	case AT_SP : CMD_print( pCommand->cmdSize, GET_sleepmCMD_sp() ); break;
/* DP */	case AT_DP : CMD_print( pCommand->cmdSize, GET_sleepmCMD_dp() ); break;
/* SO */	case AT_SO : CMD_print( pCommand->cmdSize, GET_sleepmCMD_so() ); break;
/* BD */    case AT_BD : CMD_print( pCommand->cmdSize, GET_serintCMD_bd() ); break;
/* NB */    case AT_NB : CMD_print( pCommand->cmdSize, GET_serintCMD_nb() ); break;
/* RO */	case AT_RO : CMD_print( pCommand->cmdSize, GET_serintCMD_ro() ); break;
/* D8 */	case AT_D8 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d8() );	 break;
/* D7 */	case AT_D7 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d7() );	 break;
/* D6 */	case AT_D6 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d6() );	 break;
/* D5 */	case AT_D5 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d5() );	 break;
/* D4 */	case AT_D4 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d4() );	 break;
/* D3 */	case AT_D3 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d3() );	 break;
/* D2 */	case AT_D2 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d2() );	 break;
/* D1 */	case AT_D1 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d1() );	 break;
/* D0 */	case AT_D0 : CMD_print( pCommand->cmdSize, GET_ioserCMD_d0() );	 break;
/* PR */	case AT_PR : CMD_print( pCommand->cmdSize, GET_ioserCMD_pr() );	 break;
/* IU */	case AT_IU : CMD_print(      DECIMAL_SIZE, GET_ioserCMD_iu() );	 break;
/* IT */	case AT_IT : CMD_print( pCommand->cmdSize, GET_ioserCMD_it() );	 break;
/* IC */	case AT_IC : CMD_print( pCommand->cmdSize, GET_ioserCMD_ic() );	 break;
/* IR */	case AT_IR : CMD_print( pCommand->cmdSize, GET_ioserCMD_ir() );	 break;
/* P0 */	case AT_P0 : CMD_print( pCommand->cmdSize, GET_ioserCMD_p0() );	 break;
/* P1 */	case AT_P1 : CMD_print( pCommand->cmdSize, GET_ioserCMD_p1() );	 break;
/* PT */	case AT_PT : CMD_print( pCommand->cmdSize, GET_ioserCMD_pt() );	 break;
/* RP */	case AT_RP : CMD_print( pCommand->cmdSize, GET_ioserCMD_rp() );	 break;
/* IA */	case AT_IA : CMD_print( pCommand->cmdSize, GET_iolpCMD_ia() );	 break;
/* T0 */	case AT_T0 : CMD_print( pCommand->cmdSize, GET_iolpCMD_T0() );	 break;
/* T1 */	case AT_T1 : CMD_print( pCommand->cmdSize, GET_iolpCMD_T1() );	 break;
/* T2 */	case AT_T2 : CMD_print( pCommand->cmdSize, GET_iolpCMD_T2() );	 break;
/* T3 */	case AT_T3 : CMD_print( pCommand->cmdSize, GET_iolpCMD_T3() );	 break;
/* T4 */	case AT_T4 : CMD_print( pCommand->cmdSize, GET_iolpCMD_T4() );	 break;
/* T5 */	case AT_T5 : CMD_print( pCommand->cmdSize, GET_iolpCMD_T5() );	 break;
/* T6 */	case AT_T6 : CMD_print( pCommand->cmdSize, GET_iolpCMD_T6() );	 break;
/* T7 */	case AT_T7 : CMD_print( pCommand->cmdSize, GET_iolpCMD_T7() );	 break;
/* VR */	case AT_VR : CMD_print( pCommand->cmdSize, GET_diagCMD_vr() );	 break;
/* HV */	case AT_HV : CMD_print( pCommand->cmdSize, GET_diagCMD_hv() );	 break;
/* DB */	case AT_DB : CMD_print( pCommand->cmdSize, GET_diagCMD_db() );	 break;
/* EC */	case AT_EC : CMD_print( pCommand->cmdSize, GET_diagCMD_ec() );	 break;
/* EA */	case AT_EA : CMD_print( pCommand->cmdSize, GET_diagCMD_ea() );	 break;
/* DD */	case AT_DD : CMD_print( pCommand->cmdSize, GET_diagCMD_dd() );	 break;
/* GT */	case AT_GT : CMD_print( pCommand->cmdSize, GET_atcopCMD_gt() );	 break;
/* CC */	case AT_CC : CMD_print( pCommand->cmdSize, GET_atcopCMD_cc() );  break;

/* AP */	case AT_AP :
				{
					CMD_print( pCommand->cmdSize, GET_serintCMD_ap() );
					uint8_t val = GET_serintCMD_ap();
					SET_atAP_tmp( &val, pCommand->cmdSize );
				}
				break;	
/* CT */	case AT_CT :
				{
					CMD_print( pCommand->cmdSize, GET_atcopCMD_ct() );
					uint16_t val = GET_atcopCMD_ct();
					SET_atAP_tmp( &val, pCommand->cmdSize );
				}
				break;	

/* pC*/		case AT_pC : CMD_print( 1, 0x1 ); break;	

/* SS */	case AT_SS :
/* Rq */	case AT_Rq : 	
/* SB */	case AT_SB : return ERROR;

/* NI */	case AT_NI :
					if ( devMode == GET_serintCMD_ap() ) { UART_printf("%s\r", GET_netCMD_ni() ); }
					else          { AP_setMSG( GET_netCMD_ni(), strlen( (const char*) GET_netCMD_ni() ) ); }
				break;

/* FV */	case DE_FV : 
					if ( devMode == GET_serintCMD_ap() ) { UART_printf("%s\r", AT_VERSION); }
				         /* No AP handle at this place -> check AP_localDevice function */
				break;
				
			default : return INVALID_COMMAND;
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
static void CMD_print( size_t len, uint64_t value)
{
	if ( devMode == GET_serintCMD_ap() )
	{
		CMD_print( len, value );
	}
	else
	{
		AP_setMSG( &value, len );
	}
}