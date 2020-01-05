#include "Timer.h"
#include "System.h"


// void timer_init(){
// //	Configure clock for TC0 in 16 bit (ties TC0 & TC1 together): GCLK1@1khz -> TC0
// 	GCLK_PCHCTRL_Type tc_peripheral_clk_config{.reg = 0};
// 	tc_peripheral_clk_config.bit.CHEN = true; // enable the peripheral
// 	tc_peripheral_clk_config.bit.GEN = GCLK_PCHCTRL_GEN_GCLK1_Val;	// use gclk0/mclk as source for tc0
// 	GCLK->PCHCTRL[TC0_GCLK_ID].reg = tc_peripheral_clk_config.reg;
	
// //	disable timer to enable configuration
// 	((TcCount8 *) TC0)->CTRLA.bit.ENABLE = false;
// 	while(((TcCount8 *) TC0)->SYNCBUSY.bit.ENABLE);

// 	// enable interrupt
// 	((TcCount8 *) TC0)->INTENSET.bit.OVF = true; // enable overflow interrupt
// 	NVIC_ClearPendingIRQ(TC0_IRQn);
// 	NVIC_SetPriority(TC0_IRQn, 2); // prio
// 	NVIC_EnableIRQ(TC0_IRQn);

// 	TC_CTRLA_Type tc_ctrla_config = {.reg = 0};
// 	tc_ctrla_config.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV8_Val;
// 	tc_ctrla_config.bit.MODE = TC_CTRLA_MODE_COUNT8_Val;

// 	// periode is not configurable in 16 or 32 bit mode Top = 2^16-1
	
// 	((TcCount8 *) TC0)->CTRLA.reg = tc_ctrla_config.reg; 
// 	((TcCount8 *) TC0)->PER.bit.PER = 29; // 48Mhz / 30 / 8 / 29 = 145us , Periode reg can only be written if 8bit mode is setup
// }
/**
 * periode: [0:255]
 * prescaler: TC_CTRLA_PRESCALER_DIVxxx_Val
 * tc: timer instance
 * info: clock needs to be configured in advance!
 */
void timer_init(TcCount8 *tc, timer_prescaler_t prescaler, uint8_t periode){
	// disable clock before configuring
	// disabling sets stop flag in status -> needs to be
	tc->CTRLA.bit.ENABLE = false;
	while(tc->SYNCBUSY.bit.ENABLE);

	TC_CTRLA_Type tc_ctrla_config = {.reg = 0};
	tc_ctrla_config.bit.PRESCALER = static_cast<uint8_t>(prescaler);
	tc_ctrla_config.bit.MODE = TC_CTRLA_MODE_COUNT8_Val;

	tc->CTRLA.reg = tc_ctrla_config.reg; 
	tc->PER.bit.PER = periode; // 48Mhz / 30 / 8 / 29 = 145us , Periode reg can only be written if 8bit mode is setup
	
}

void timer_enableInterrupts(TcCount8 *tc, IRQn_Type irqn, uint8_t prio, timer_interrupt_t type){
	if(type & timer_interrupt_t::OVF){
		tc->INTENSET.bit.OVF = true; // enable overflow interrupt
	}
	if(type & timer_interrupt_t::ERR){
		tc->INTENSET.bit.ERR = true; // enable overflow interrupt
	}
	if(type & timer_interrupt_t::MC0){
		tc->INTENSET.bit.MC0 = true; // enable overflow interrupt
	}
	if(type & timer_interrupt_t::MC1){
		tc->INTENSET.bit.MC1 = true; // enable overflow interrupt
	}
	NVIC_ClearPendingIRQ(irqn);
	NVIC_SetPriority(irqn, prio); 
	NVIC_EnableIRQ(irqn);
}

void timer_triggerInterrupt(TcCount8 *tc, timer_interrupt_t type){
	if(type & timer_interrupt_t::OVF){
		tc->INTFLAG.bit.OVF = true; // enable overflow interrupt
	}
	if(type & timer_interrupt_t::ERR){
		tc->INTFLAG.bit.ERR = true; // enable overflow interrupt
	}
	if(type & timer_interrupt_t::MC0){
		tc->INTFLAG.bit.MC0 = true; // enable overflow interrupt
	}
	if(type & timer_interrupt_t::MC1){
		tc->INTFLAG.bit.MC1 = true; // enable overflow interrupt
	}
}

void timer_enable(TcCount8 *tc){

	tc->CTRLA.bit.ENABLE = true;
	while(tc->SYNCBUSY.bit.ENABLE);
}
void timer_start(TcCount8 *tc){
	tc->CTRLBSET.bit.CMD = TC_CTRLBSET_CMD_RETRIGGER_Val;
}
void timer_stop(TcCount8 *tc){
	tc->CTRLBSET.bit.CMD = TC_CTRLBSET_CMD_STOP_Val;
}

void timer_disable(TcCount8 *tc){
	tc->CTRLA.bit.ENABLE = false;
	while(tc->SYNCBUSY.bit.ENABLE);
}

void timer_setPeriode(TcCount8 *tc, uint8_t newVal){
	tc->PER.bit.PER = newVal;
}

void timer_setCount(TcCount8 *tc, uint8_t newVal){
	tc->COUNT.reg = newVal;
}

void timer_setOneShot(TcCount8 *tc){
	tc->CTRLBSET.bit.ONESHOT = true;
}
void timer_setRepeating(TcCount8 *tc){
	tc->CTRLBCLR.bit.ONESHOT = true;
}