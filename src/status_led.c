#include "status_led.h"

#include "gpio.h"
#include "timers.h"

#include "hardware/gpio.h"

#include <stdio.h>

#define LED_TIMER_PERIOD pdMS_TO_TICKS(500UL)
#define STATUS_LED (PICO_DEFAULT_LED_PIN)

static TimerHandle_t led_timer = NULL;

void timerCallback(TimerHandle_t timer) {
  (void)timer;
  gpio_xor_mask(1u << STATUS_LED);
}

void initStatusLED() {
  led_timer = xTimerCreate((const char *)"LEDTimer", LED_TIMER_PERIOD, pdTRUE,
                           (void *)0, timerCallback);

  if (led_timer != NULL) {
    printf("starting heartbeat timer \n");
    xTimerStart(led_timer, 0);
  }
}
