#ifndef SERIAL_H_
#define SERIAL_H_

#include <saml21.h>

class Serial{
public:
	Serial();
	void write(uint8_t byte);
	char read();
	
	void print(const char* str, const char* endl = "\n");

	void (*line_received_cb)(char*) = nullptr;
	void (*receive_buffer_full_cb)(char*) = nullptr;
};

extern Serial serial;

#endif