/*
 * stackdefines.h
 *
 * Created: 10.11.2016 10:10:51
 *  Author: TOE
 * 
 * To keep the compatibility and reduce searching times
 * the original defines are redefined 
 * not nice to read, but it could save a lot of time
 */ 

#include "board.h"


#ifndef STACKDEFINES_H_
#define STACKDEFINES_H_

#define deHIF_DEFAULT_BAUDRATE	HIF_DEFAULT_BAUDRATE

#define deSPI_RATE_1_2			SPI_RATE_1_2

#define deSR_CHANNEL			SR_CHANNEL
#define deSR_TRAC_STATUS		SR_TRAC_STATUS
#define deSR_TX_AUTO_CRC_ON		SR_TX_AUTO_CRC_ON

#define deRG_TRX_STATE			RG_TRX_STATE
#define deRG_IRQ_MASK			RG_IRQ_MASK
#define deRG_IRQ_STATUS			RG_IRQ_STATUS

#define deCMD_RX_AACK_ON		CMD_RX_AACK_ON
#define deCMD_PLL_ON			CMD_PLL_ON
#define deCMD_TX_ARET_ON		CMD_TX_ARET_ON

#if defined(TRX_IRQ_TRX_END)
	#define deTRX_IRQ_TRX_END	TRX_IRQ_TRX_END
#endif

#if defined(TRX_IRQ_RX_END)
	#define deTRX_IRQ_RX_END	TRX_IRQ_RX_END
	#define deTRX_IRQ_TX_END	TRX_IRQ_TX_END
#endif

#define deTRAC_SUCCESS			TRAC_SUCCESS

#if defined(TRX_IF_RFA1)
	#define deTRX_IF_RFA1		TRX_IF_RFA1
#endif

#define deTRX24_TX_END_vect		TRX24_TX_END_vect
#define deTRX24_RX_END_vect		TRX24_RX_END_vect
#define deTRX_IRQ_vect			TRX_IRQ_vect

#endif /* STACKDEFINES_H_ */