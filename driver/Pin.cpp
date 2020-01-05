#include "Pin.h"

Pin::Pin(){
	
}

Pin::Pin(Port port, uint8_t pin, Function f){
	PORT_PINCFG_Type conf = {.reg = 0};
	if(static_cast<int>(f) & (0xf)) { // pin mux enabled
		conf.bit.PMUXEN = true;
		// odd pin == % 2
		uint8_t index = pin >> 1;
		uint8_t value = (static_cast<int>(f) & 0x0f) - 1;
		if(pin & 0x1) {
			((PortGroup *) PORT)[static_cast<uint8_t>(port)].PMUX[index].bit.PMUXO = value;
		}else {
			((PortGroup *) PORT)[static_cast<uint8_t>(port)].PMUX[index].bit.PMUXE = value;
		}
	}else{
		// Set direction - input or output
		if(f & Function::GPIO_Output) ((PortGroup *) PORT)[static_cast<uint8_t>(port)].DIRSET.reg = 1 << pin;
		else {
			((PortGroup *) PORT)[static_cast<uint8_t>(port)].DIRCLR.reg = 1 << pin;
			conf.bit.INEN = true;
		}

		if(f & Function::Pullup) {
			conf.bit.PULLEN = true; // enable pull
			((PortGroup *) PORT)[static_cast<uint8_t>(port)].OUTSET.reg = 1 << pin; // set pullup
		}else if(f & Function::Pulldown) {
			conf.bit.PULLEN = true; // enable pull
			((PortGroup *) PORT)[static_cast<uint8_t>(port)].OUTCLR.reg = 1 << pin; // set pulldown
		}
		if(f & Function::StrongDrive) conf.bit.DRVSTR = true; // Set strong drive strength

		// Set Pin state - high or low
		if(f & Function::High) ((PortGroup *) PORT)[static_cast<uint8_t>(port)].OUTSET.reg = 1 << pin;
		else ((PortGroup *) PORT)[static_cast<uint8_t>(port)].OUTCLR.reg = 1 << pin;
		
	}
	((PortGroup *) PORT)[static_cast<uint8_t>(port)].PINCFG[pin].reg = conf.reg;
	this->pin = pin;
	this->port = port;
}

void Pin::setHigh(){ ((PortGroup *) PORT)[static_cast<uint8_t>(this->port)].OUTSET.reg = 1 << this->pin; }
void Pin::setLow(){ ((PortGroup *) PORT)[static_cast<uint8_t>(this->port)].OUTCLR.reg = 1 << this->pin; }
void Pin::toggle(){ ((PortGroup *) PORT)[static_cast<uint8_t>(this->port)].OUTTGL.reg = 1 << this->pin; }
uint8_t Pin::getValue(){return ((PortGroup *) PORT)[static_cast<uint8_t>(this->port)].IN.reg >> this->pin & 0x01;}
uint8_t Pin::getPin(){return this->pin;}
uint8_t Pin::getPort(){return static_cast<uint8_t>(this->port);}