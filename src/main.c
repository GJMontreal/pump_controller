// A very simple two zone pump and boiler sequencer
// An input on either zone will start the corresponding pump and generate a
// boiler demand signal Either pump will continue to run for a preset purge time
// after the zone demand disappears

#include "FreeRTOS.h"

#include "timers.h" /* Software timer related API prototypes. */

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdio.h>

#include "demand.h"
#include "gpio.h"
#include "status_led.h"

static void setupHardware(void);

int main() {
  setupHardware();
  printf("initializing pump_controller.\n");
  initDemands(&demand[0]);
  initStatusLED();

  vTaskStartScheduler();
  for (;;) {
  };
  return 0;
}

static void setupHardware(void) {
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
