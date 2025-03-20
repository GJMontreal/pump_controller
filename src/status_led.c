#include "status_led.h"

#include "gpio.h"

#include "hardware/gpio.h"

#define LED_TIMER_PERIOD  pdMS_TO_TICKS( 500UL ) 
#define STATUS_LED  ( PICO_DEFAULT_LED_PIN )

void timerCallback( TimerHandle_t timer )
{
    ( void )timer;
    gpio_xor_mask( 1u << STATUS_LED );
}

TimerHandle_t setupStatusLED()
{
  TimerHandle_t timer =
  timer = xTimerCreate(     /* A text name, purely to help
    debugging. */
(const char *) "LEDTimer",
/* The timer period, in this case
1000ms (1s). */
LED_TIMER_PERIOD,
/* This is a periodic timer, so
xAutoReload is set to pdTRUE. */
pdTRUE,
/* The ID is not used, so can be set
to anything. */
(void *) 0,
/* The callback function that switches
the LED off. */
timerCallback
);
  return timer;
}
