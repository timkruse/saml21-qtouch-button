#include "Serial.h"

Serial serial;

void SERCOM4_Handler(){
	static char recv_buffer[64];
	static uint8_t recv_buffer_len = 0;
	char c = ((SercomUsart *)SERCOM4)->DATA.bit.DATA;
	if(c == '\n') {
		recv_buffer[recv_buffer_len] = 0; // overwrites newline
		if(serial.line_received_cb != nullptr) serial.line_received_cb((recv_buffer)); // copy buffer to a string and call dedicated function
		recv_buffer_len = 0;
	} else {
		if (recv_buffer_len < 63) recv_buffer[recv_buffer_len++] = c;
		if (recv_buffer_len >= 63){
			recv_buffer_len = 0;
			if(serial.receive_buffer_full_cb != nullptr) serial.receive_buffer_full_cb((recv_buffer));
		}	
	}
}

Serial::Serial(){
//	Configuration of SERCOM4 as UART. RX/PAD[3] on PB11 and TX/PAD[2] on PB10, peripheral D
	// Config PB10 as output of SERCOM4.PAD[2] (USART.TX) -> pin func D
	PORT_PINCFG_Type pb10_conf{.reg = 0};
	pb10_conf.bit.PMUXEN = true;
	PORT->Group[1].PMUX[5].bit.PMUXE = 0x03; // 0x03 := D
	PORT->Group[1].PINCFG[10].reg = pb10_conf.reg;

	// Config PB11 as output of SERCOM4.PAD[3] (USART.RX) -> pin func D
	PORT_PINCFG_Type pb11_conf{.reg = 0};
	pb11_conf.bit.PMUXEN = true;
	PORT->Group[1].PMUX[5].bit.PMUXO = 0x03; // 0x03 := D
	PORT->Group[1].PINCFG[11].reg = pb11_conf.reg;

	// Config SERCOM4 peripheral clock
	GCLK_PCHCTRL_Type usart_peripheral_clk_config{.reg = 0};
	usart_peripheral_clk_config.bit.CHEN = true; // enable the peripheral
	usart_peripheral_clk_config.bit.GEN = GCLK_PCHCTRL_GEN_GCLK0_Val;	// use gclk0/mclk as source for sercom4
	GCLK->PCHCTRL[SERCOM4_GCLK_ID_CORE].reg = usart_peripheral_clk_config.reg;

//	USART Config:

	// enable receive complete interrupt
	((SercomUsart *)SERCOM4)->INTENSET.bit.RXC = true;
	NVIC_ClearPendingIRQ(SERCOM4_IRQn);
	NVIC_SetPriority(SERCOM4_IRQn, 3); // lowest prio
	NVIC_EnableIRQ(SERCOM4_IRQn);

	((SercomUsart *)SERCOM4)->CTRLA.bit.ENABLE = false; // disable to be able to write to ctrl regs
	// Configure SERCOM4 as USART for 115200/8N1
	SercomUsart uart;

	uart.CTRLB.reg = 0; // create config
	uart.CTRLB.bit.RXEN = true;
	uart.CTRLB.bit.TXEN = true;

	uart.BAUD.reg = 0; // create config
	uart.BAUD.bit.BAUD = 59944; // 63019->115200baud, 59944 -> 256000, Table 31-2 Asynchronous Arithmetic

	uart.CTRLA.reg = 0; // create config
	uart.CTRLA.bit.RUNSTDBY = true;
	uart.CTRLA.bit.RXPO = MUX_PB11D_SERCOM4_PAD3;
	uart.CTRLA.bit.TXPO = 1; // MUX_PB10D_SERCOM4_PAD2==3 and WRONG!!!
	uart.CTRLA.bit.MODE = 1; // using internal clock
	uart.CTRLA.bit.DORD = 1; // lsb first
	uart.CTRLA.bit.ENABLE = true; // enable peripheral

	// install config
	((SercomUsart *)SERCOM4)->CTRLB.reg = uart.CTRLB.reg;
	((SercomUsart *)SERCOM4)->BAUD.reg = uart.BAUD.reg;
	((SercomUsart *)SERCOM4)->CTRLA.reg = uart.CTRLA.reg; // this enables the uart
}
void Serial::write(uint8_t byte){
	while(!((SercomUsart *)SERCOM4)->INTFLAG.bit.DRE);
	((SercomUsart *)SERCOM4)->DATA.bit.DATA = byte; // write byte
}
char Serial::read(){
	while(!((SercomUsart *)SERCOM4)->INTFLAG.bit.RXC);
	return ((SercomUsart *)SERCOM4)->DATA.bit.DATA; // read byte
}
void Serial::print(const char* str, const char* endl){
	const char* ptr = str;
	while(*ptr != 0){
		this->write(*ptr++);
	}
	ptr = endl;
	while(*ptr != 0){
		this->write(*ptr++);
	}
}
