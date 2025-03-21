// A very simple two zone pump and boiler sequencer
// An input on either zone will start the corresponding pump and generate a
// boiler demand signal Either pump will continue to run for a preset purge time
// after the zone demand disappears

#include "FreeRTOS.h"

#include <stdio.h>

#include "demand.h"
#include "gpio.h"
#include "status_led.h"

int main() {
  initGPIO();
  printf("initializing pump_controller.\n");

  initDemands();
  initStatusLED();

  vTaskStartScheduler();
  for (;;) {
  };
  return 0;
}
