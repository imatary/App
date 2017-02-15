/*
 * stackrelated_timer.h
 *
 * Created: 21.11.2016 11:04:04
 *  Author: TOE
 */


#ifndef STACKRELATED_TIMER_H_
#define STACKRELATED_TIMER_H_

/*
 * timer functions
 *
 * deTIMER_init		base initializer and set the pointer to µracoli functions
 * deTIMER_start	start a timer
 * deTIMER_stop		stopped a given timer
 * deTIMER_restart	restart a given timer
 */

void deTIMER_init(void);
uint16_t (*deTIMER_start)   (uint32_t (*thfunc)(uint32_t param), uint32_t duration, uint32_t arg);
uint16_t (*deTIMER_stop)    (uint16_t timehandler);
uint16_t (*deTIMER_restart) (uint16_t timehandler, uint32_t duration);

uint32_t deMSEC(uint32_t arg);
uint32_t deUSEC(uint32_t arg);

#endif /* STACKRELATED_TIMER_H_ */