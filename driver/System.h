#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <stdint.h>

void gclk_enable_clock(uint8_t id, uint8_t gclk_source);
void SystemCoreClockUpdate();
void SystemInit();

#endif
