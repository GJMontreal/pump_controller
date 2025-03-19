// A very simple two zone pump controller
// An input on either zone will start the corresponding pump and generate a boiler demand signal
// Either pump will continue to run for a preset purge time after the zone demand disappears

/* Library includes. */
#include "FreeRTOS.h"
#include "timers.h"   /* Software timer related API prototypes. */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define DEMAND_1_GPIO  2
#define DEMAND_2_GPIO  3

#define LED_TIMER_PERIOD  pdMS_TO_TICKS( 500UL ) 
#define STATUS_LED  ( PICO_DEFAULT_LED_PIN )

#define DEBOUNCE_TIMER_PERIOD pdMS_TO_TICKS( 50UL)

typedef struct {
  uint gpio_pin;
  uint pin_state;
} Demand_State_t;

static char event_str[128];

static void setupHardware(void);
static void createTimer(TimerHandle_t* timer);
static void timerCallback( TimerHandle_t timer );
static void debounceCallback(TimerHandle_t timer);
static void gpioIRQCallback(uint gpio, uint32_t events);
static void gpioEventString(char *buf, uint32_t events);

#define NUM_DEMAND_INPUTS 2
static TimerHandle_t debounce_timer[NUM_DEMAND_INPUTS];
static Demand_State_t demand_state[NUM_DEMAND_INPUTS];

int main(){
  setupHardware();
  printf(" Starting pump_controller.\n");
  
  // TODO: move this
  for(uint i = 0; i < NUM_DEMAND_INPUTS; i++){
    debounce_timer[i]= NULL;
    demand_state[i].gpio_pin = DEMAND_1_GPIO + i;
    demand_state[i].pin_state = gpio_get(DEMAND_1_GPIO + i);
  }

  TimerHandle_t LEDTimer = NULL;
  createTimer(&LEDTimer);
  xTimerStart(LEDTimer, 0);
// create tasks for each input
  vTaskStartScheduler();
  for( ;; ){
      
  };
  return 0;
}

static void setupHardware( void )
{
    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);
    gpio_put(PICO_DEFAULT_LED_PIN, !PICO_DEFAULT_LED_PIN_INVERTED);

    gpio_init(DEMAND_1_GPIO);
    gpio_set_dir(DEMAND_1_GPIO, false);
    gpio_set_irq_enabled_with_callback(DEMAND_1_GPIO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpioIRQCallback);
}

static void createTimer(TimerHandle_t *timer)
{
  *timer = xTimerCreate(     /* A text name, purely to help
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
}

static void timerCallback( TimerHandle_t timer )
{
    ( void )timer;
    gpio_xor_mask( 1u << STATUS_LED );
}

static void gpioIRQCallback(uint gpio, uint32_t events){
  // TODO: debounce interrupts
  // only put something in the queue if the state has changed and remains stable for some amount of time
  gpio_acknowledge_irq(gpio, events);
  gpioEventString(event_str, events);
  uint gpio_state = gpio_get(gpio);
  printf("GPIO %d %s %d\n", gpio, event_str, gpio_state);

  uint index = gpio - DEMAND_1_GPIO;
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
  (void)timer;
  uint index = (uint) pvTimerGetTimerID(timer);
  Demand_State_t state = demand_state[index];
  uint gpio_state = gpio_get(state.gpio_pin);
  printf("debounce callback %d %d\n", index, gpio_state);

  if( gpio_state != state.pin_state){
    printf("do something for demand %d %d\n",index, gpio_state);
    demand_state[index].pin_state = gpio_state;
  }
}
  

static const char *gpio_irq_str[] = {
  "LEVEL_LOW",  // 0x1
  "LEVEL_HIGH", // 0x2
  "EDGE_FALL",  // 0x4
  "EDGE_RISE"   // 0x8
};

static void gpioEventString(char *buf, uint32_t events) {
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