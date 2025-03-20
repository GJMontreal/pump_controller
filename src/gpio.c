#include "gpio.h"

const char *gpio_irq_str[] = {
  "LEVEL_LOW",  // 0x1
  "LEVEL_HIGH", // 0x2
  "EDGE_FALL",  // 0x4
  "EDGE_RISE"   // 0x8
};

void gpioEventString(char *buf, uint32_t events) {
  for (uint i = 0; i < 4; i++) {
    uint mask = (1 << i);
    if (events & mask) {
        // Copy this event string into the user string
        const char *event_str = gpio_irq_str[i];
        while (*event_str != '\0') {
            *buf++ = *event_str++;
        }
        events &= ~mask;

        // If more events add ", "
        if (events) {
            *buf++ = ',';
            *buf++ = ' ';
        }
    }
  }
  *buf++ = '\0';
}