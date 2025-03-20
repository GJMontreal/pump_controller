#ifndef __gpio_h_
#define __gpio_h_

#include "pico/stdlib.h"

#define DEBOUNCE_TIMER_PERIOD pdMS_TO_TICKS(1000UL)

#define OUTPUT_ACTIVE_HIGH true
#define INPUT_ACTIVE_HIGH true

#define DEMAND_1_GPIO 2
#define DEMAND_2_GPIO (DEMAND_2_GPIO + 1)

#define PUMP_1_GPIO 18
#define PUMP_2_GPIO (PUMP_1_GPIO + 1)

#define BOILER_GPIO 16

void gpioEventString(char *buf, uint32_t events);
void gpioIRQCallback(uint gpio, uint32_t events);

#endif
