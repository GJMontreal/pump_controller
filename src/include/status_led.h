#ifndef __status_led_h_
#define __status_led_h_

#include "FreeRTOS.h"
#include "timers.h"


TimerHandle_t setupStatusLED();
// void timerCallback( TimerHandle_t timer );

#endif
