/*
 * timer0.c
 *
 * Created: 21.11.2016 07:25:32
 *  Author: TOE
 */ 

#include "board.h"
#include "timer.h"
#include "stackrelated_timer.h"

/*
 * deTIMER_init
 * prepared the module for timer related functions
 *
 * last modified: 2016/11/21
 */
void deTIMER_init(void)
{
	timer_init();
	
	deTIMER_start   = timer_start;
	deTIMER_stop    = timer_stop;
	deTIMER_restart = timer_restart;
	
}

/*
 * deMSEC
 * Returned:
 *		device related value in milliseconds 
 *
 * last modified: 2016/11/21
 */
uint32_t deMSEC(uint32_t arg)
{
	return MSEC(arg);
}