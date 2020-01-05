
#include <saml21.h>


/**
 * id: xx_GCLK_ID
 * gclk_source: GCLK_PCHCTRL_GEN_xx_VAL
 */
void gclk_enable_clock(uint8_t id, uint8_t gclk_source){
	GCLK_PCHCTRL_Type clk_config{.reg = 0};
	clk_config.bit.CHEN = true; // enable the peripheral
	clk_config.bit.GEN = gclk_source;
	GCLK->PCHCTRL[id].reg = clk_config.reg;
}

void SystemInit(){
//	Set PowerLevel to 2 for all peripherals to work
	PM->PLCFG.bit.PLSEL = 2;
//	Set NVM flash wait states to 2
	NVMCTRL->CTRLB.bit.RWS = 2;
}

/** Configure MCLK to be at 48MHz, fed by XTAL@16MHz */
void SystemCoreClockUpdate(){
	// config XOSC
	OSCCTRL_XOSCCTRL_Type xosc_config{.reg = 0};
	xosc_config.bit.ENABLE = true;
	xosc_config.bit.XTALEN = true;
	xosc_config.bit.STARTUP = 0xB; // 62.5ms
	xosc_config.bit.AMPGC = true;
	xosc_config.bit.GAIN = 0x3; // 16MHz
	xosc_config.bit.ONDEMAND = false;
	OSCCTRL->XOSCCTRL.reg = xosc_config.reg;
	while(!OSCCTRL->STATUS.bit.XOSCRDY); // wait for xosc to be ready

	// Config PA16 as output of GCLK_IO[2] -> pin func H
	// PORT_PINCFG_Type pa16_conf{.reg = 0};
	// pa16_conf.bit.PMUXEN = true;
	// PORT->Group[0].PMUX[8].bit.PMUXE = 0x07; // 0x07 := H
	// PORT->Group[0].PINCFG[16].reg = pa16_conf.reg;

	// GCLK_GENCTRL_Type gclk2_config{
	// 	/*.bit.SRC =*/ GCLK_GENCTRL_SRC_XOSC_Val,
	// 	/*.bit.GENEN =*/ true,
	// 	/*.bit.IDC =*/ true,
	// 	/*.bit.OOV =*/ 0,
	// 	/*.bit.OE =*/ true,
	// 	/*.bit.DIVSEL =*/ false,
	// 	/*.bit.RUNSTDBY =*/ true,
	// 	/*.bit.DIV =*/ 16
	// };
	// GCLK->GENCTRL[2].reg = gclk2_config.reg;

	// gclk0 -> dpll
	// GCLK_PCHCTRL_Type fdpll_peripheral_clk_config{.reg = 0};
	// fdpll_peripheral_clk_config.bit.CHEN = true; // enable the peripheral
	// fdpll_peripheral_clk_config.bit.GEN = GCLK_PCHCTRL_GEN_GCLK2_Val;	// use output of gclk2
	// GCLK->PCHCTRL[OSCCTRL_GCLK_ID_FDPLL].reg = fdpll_peripheral_clk_config.reg;

//	Configure PLL
	OSCCTRL_DPLLRATIO_Type pllratio_config{.reg = 0};
	pllratio_config.bit.LDR = 47; // 47 -> 48MHz which is the least possible
	OSCCTRL->DPLLRATIO.reg = pllratio_config.reg;
	while(OSCCTRL->DPLLSYNCBUSY.bit.DPLLRATIO); // wait until register is written

	OSCCTRL_DPLLCTRLB_Type pllb_config{.reg = 0};
	pllb_config.bit.DIV = 7; // 7 = Divide XOSC@16MHz to 1MHz to meet dpll reqirements. (fdiv = fxosc/(2*(div + 1)) @ p. 246)
	pllb_config.bit.REFCLK = 1; // 1 = XOSC as reference, 2 = gclk input
	OSCCTRL->DPLLCTRLB.reg = pllb_config.reg;

	OSCCTRL_DPLLCTRLA_Type plla_config{.reg = 0};
	plla_config.bit.RUNSTDBY = true;
	plla_config.bit.ENABLE = true;
	OSCCTRL->DPLLCTRLA.reg = plla_config.reg; // this enables the dpll
	while(OSCCTRL->DPLLSYNCBUSY.bit.ENABLE); // wait until register is written



	//while(!OSCCTRL->DPLLSTATUS.bit.LOCK); // wait until dpll is ready
	//while(!OSCCTRL->DPLLSTATUS.bit.CLKRDY); // wait for xosc to be ready

	
	// Config PB23 as output of GCLK_IO[2] -> pin func H
	// PORT_PINCFG_Type pb23_conf{.reg = 0};
	// pb23_conf.bit.PMUXEN = true;
	// PORT->Group[1].PMUX[11].bit.PMUXO = 0x07; // 0x07 := H
	// PORT->Group[1].PINCFG[23].reg = pb23_conf.reg;


	GCLK_GENCTRL_Type gclk1_config{
		/*.bit.SRC =*/ GCLK_GENCTRL_SRC_DPLL96M_Val,
		/*.bit.GENEN =*/ true,
		/*.bit.IDC =*/ true,
		/*.bit.OOV =*/ 0,
		/*.bit.OE =*/ false, // pb23
		/*.bit.DIVSEL =*/ false,
		/*.bit.RUNSTDBY =*/ true,
		/*.bit.DIV =*/ 48
	};
	GCLK->GENCTRL[1].reg = gclk1_config.reg;

//	Setup GCLK0 (GCLK_MAIN) to be driven by FDPLL@48MHz
	GCLK_GENCTRL_Type gclk0_config{
		/*.bit.SRC =*/ GCLK_GENCTRL_SRC_DPLL96M_Val,
		/*.bit.GENEN =*/ true,
		/*.bit.IDC =*/ true,
		/*.bit.OOV =*/ 0,
		/*.bit.OE =*/ false, 
		/*.bit.DIVSEL =*/ false,
		/*.bit.RUNSTDBY =*/ true,
		/*.bit.DIV =*/ 1
	};
	GCLK->GENCTRL[0].reg = gclk0_config.reg;

	SystemCoreClock = 48000000;

}