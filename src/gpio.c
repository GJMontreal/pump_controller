#include "gpio.h"

#include "demand.h"

#include <stdio.h>


static char event_str[128];

static TimerHandle_t debounce_timer[NUM_DEMAND_INPUTS];
static void debounceCallback(TimerHandle_t timer);

void gpioIRQCallback(uint gpio, uint32_t events){
  gpio_acknowledge_irq(gpio, events);
  gpioEventString(event_str, events);
  uint gpio_state = gpio_get(gpio);
  printf("GPIO %d %s %d\n", gpio, event_str, gpio_state);

  uint index = gpio - DEMAND_1_GPIO;
  
  // TODO: we could make this a function which we pass in a timer,period and callback
  TimerHandle_t timer = debounce_timer[index];
  if( timer != NULL){
    printf("stopping timer %d \n", index);
    xTimerStop(timer, 0);
  }
  char timer_name[12];
  sprintf(timer_name, "timer %d", index);
  timer = xTimerCreate(timer_name, DEBOUNCE_TIMER_PERIOD, pdFALSE,
                       (void *)index, &debounceCallback);
  debounce_timer[index] = timer;
  xTimerStart(timer, 0);
}

static void debounceCallback(TimerHandle_t timer){
  uint index = (uint) pvTimerGetTimerID(timer);
  Demand_t state = demand[index];
  debounce_timer[index] = NULL;
  uint gpio_state = gpio_get(state.input_gpio);
  printf("debounce callback %d %d\n", index, gpio_state);

  if( gpio_state != state.desired_state){
    printf("queuing demand %d %d\n",index, gpio_state);
    demand[index].desired_state = gpio_state;
    xQueueSendToBackFromISR(demand_queue, &demand[index], 0);
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