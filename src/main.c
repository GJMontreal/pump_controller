/* 
A very simple zone pump and boiler sequencer.
 
A demand will start the corresponding pump and generate a boiler demand signal.

Pumps will continue to run for a preset purge time after the zone demand disappears.

Outputs and inputs are configurable to be either active high or low.
*/

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
