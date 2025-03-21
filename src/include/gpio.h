#ifndef __gpio_h_
#define __gpio_h_

#include "pico/stdlib.h"


#define OUTPUT_ACTIVE_HIGH true
#define INPUT_ACTIVE_HIGH false
 
#define DEMAND_1_GPIO 18

#define PUMP_1_GPIO 10

#define BOILER_GPIO 6

void initGPIO();
void gpioEventString(char *buf, uint32_t events);
void gpioIRQCallback(uint gpio, uint32_t events);

#endif
