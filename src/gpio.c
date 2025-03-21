#include "gpio.h"

#include "demand.h"

#include <stdio.h>

void initGPIO(void) {
  stdio_init_all();
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);
  gpio_put(PICO_DEFAULT_LED_PIN, !PICO_DEFAULT_LED_PIN_INVERTED);

  for (uint i = 0; i < NUM_DEMAND_INPUTS; i++) {
    gpio_init(DEMAND_1_GPIO + i);
    gpio_init(PUMP_1_GPIO + i);
    gpio_set_dir(DEMAND_1_GPIO + i, false);
    gpio_set_dir(PUMP_1_GPIO + i, true);
    gpio_set_irq_enabled_with_callback(DEMAND_1_GPIO + i,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true, &gpioIRQCallback);
    gpio_put(PUMP_1_GPIO + i, 0);
  }

  gpio_put(BOILER_GPIO, (false == OUTPUT_ACTIVE_HIGH));
  gpio_init(BOILER_GPIO);
  gpio_set_dir(BOILER_GPIO, true);
}

void gpioIRQCallback(uint gpio, uint32_t events) {
  gpio_acknowledge_irq(gpio, events);
  Demand_t *demand = demandForInput(gpio);
  if(demand != NULL){
    demand->desired_state = (gpio_get(gpio) == INPUT_ACTIVE_HIGH);
    vTimerSetTimerID(demand->debounce_timer, (void*)demand);
    xTimerStart(demand->debounce_timer, 0);
  }
}

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