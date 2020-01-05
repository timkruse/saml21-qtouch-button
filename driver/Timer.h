#ifndef TIMER_H_
#define TIMER_H_

#include <saml21.h>

typedef enum{
	OVF = 1 << 0,
	ERR = 1 << 1,
	MC0 = 1 << 4,
	MC1 = 1 << 5
}timer_interrupt_t;

typedef enum{
	None = 0,
	Div2 = 1,
	Div4 = 2,
	Div8 = 3,
	Div16 = 4,
	Div64 = 5,
	Div256 = 6,
	Div1024 = 7
}timer_prescaler_t;

/**
 * @brief Initializes the timer
 * Requires the timer to be disabled !!!
 * Timer init flow: init, config, config interrupts, enable
 * @param tc 
 * @param prescaler 
 * @param periode 
 */
extern void timer_init(TcCount8 *tc, timer_prescaler_t prescaler, uint8_t periode);

/**
 * @brief Enables and starts the timer
 * 
 * @param tc 
 */
extern void timer_enable(TcCount8 *tc);
extern void timer_disable(TcCount8 *tc);

/**
 * @brief Sends the start command to the timer
 * Effective if timer was paused/stopped
 * @param tc 
 */
extern void timer_start(TcCount8 *tc);
extern void timer_stop(TcCount8 *tc);
extern void timer_setPeriode(TcCount8 *tc, uint8_t newVal);
extern void timer_setCount(TcCount8 *tc, uint8_t newVal);
extern void timer_setOneShot(TcCount8 *tc);
extern void timer_setRepeating(TcCount8 *tc);
extern void timer_enableInterrupts(TcCount8 *tc, IRQn_Type irqn, uint8_t prio, timer_interrupt_t type);
extern void timer_triggerInterrupt(TcCount8 *tc, timer_interrupt_t type);


#endif