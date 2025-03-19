// A very simple two zone pump controller
// An input on either zone will start the corresponding pump and generate a boiler demand signal
// Either pump will continue to run for a preset purge time after the zone demand disappears

/* Library includes. */
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "projdefs.h"
#include "timers.h"   /* Software timer related API prototypes. */
#include"queue.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define OUTPUT_ACTIVE_HIGH true
#define INPUT_ACTIVE_HIGH true

#define DEMAND_1_GPIO  2
#define DEMAND_2_GPIO  ( DEMAND_2_GPIO + 1 )

#define PUMP_1_GPIO 18
#define PUMP_2_GPIO ( PUMP_1_GPIO +1 )

#define BOILER_GPIO 16

#define LED_TIMER_PERIOD  pdMS_TO_TICKS( 500UL ) 
#define STATUS_LED  ( PICO_DEFAULT_LED_PIN )

#define DEBOUNCE_TIMER_PERIOD pdMS_TO_TICKS( 1000UL)
#define PURGE_TIME pdMS_TO_TICKS( 5000UL )
typedef struct {
  uint input_gpio;
  uint output_gpio;
  uint desired_state;
  TimerHandle_t shutoff_timer;
} Demand_t;

static char event_str[128];

static void setupHardware(void);
static void createTimer(TimerHandle_t* timer);

static void setupShutdownTimers();

// FreeRTOS callbacks
static void timerCallback( TimerHandle_t timer );
static void debounceCallback(TimerHandle_t timer);
static void shutdownCallback(TimerHandle_t timer);

static void gpioIRQCallback(uint gpio, uint32_t events);
static void gpioEventString(char *buf, uint32_t events);

static void demandQueueRXTask(void *params);
static void startupTask(void *params);
static TaskHandle_t startup_task;

#define NUM_DEMAND_INPUTS 2
static TimerHandle_t debounce_timer[NUM_DEMAND_INPUTS];
static Demand_t demand[NUM_DEMAND_INPUTS];

static QueueHandle_t demand_queue = NULL;
#define DEMAND_QUEUE_RX_TASK_PRIORITY	( tskIDLE_PRIORITY + 2 )
#define STARTUP_TASK_PRIORITY ( tskIDLE_PRIORITY + 1 )

int main(){
  setupHardware();
  printf("initializing pump_controller.\n");
  
  // TODO: move this
  for(uint i = 0; i < NUM_DEMAND_INPUTS; i++){
    debounce_timer[i]= NULL;
    demand[i].input_gpio = DEMAND_1_GPIO + i;
    demand[i].desired_state = gpio_get(DEMAND_1_GPIO + i);
    demand[i].output_gpio = PUMP_1_GPIO + i;
  }

  setupShutdownTimers();

  demand_queue =
      xQueueCreate(NUM_DEMAND_INPUTS,
                   sizeof(Demand_t));
  if (demand_queue != NULL) {
    xTaskCreate(demandQueueRXTask, "DemandRx", configMINIMAL_STACK_SIZE, &demand,
                DEMAND_QUEUE_RX_TASK_PRIORITY, NULL);

    xTaskCreate(startupTask, "Startup", configMINIMAL_STACK_SIZE, NULL,
                STARTUP_TASK_PRIORITY, &startup_task);
  }

  TimerHandle_t LEDTimer = NULL;
  createTimer(&LEDTimer);
  xTimerStart(LEDTimer, 0);

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

    for (uint i = 0; i < NUM_DEMAND_INPUTS; i++) {
      gpio_init(DEMAND_1_GPIO + i);
      gpio_init(PUMP_1_GPIO + i);
      gpio_set_dir(DEMAND_1_GPIO + i, false);
      gpio_set_dir(PUMP_1_GPIO + i, true);
      gpio_set_irq_enabled_with_callback(
          DEMAND_1_GPIO + i, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true,
          &gpioIRQCallback);
      gpio_put(PUMP_1_GPIO + i, 0);
    }

    gpio_put(BOILER_GPIO, (false == OUTPUT_ACTIVE_HIGH));
    gpio_init(BOILER_GPIO);
    gpio_set_dir(BOILER_GPIO, true);
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

static void setupShutdownTimers(){
  char timer_name[12];
  for(uint i=0; i < NUM_DEMAND_INPUTS; i++){
    sprintf(timer_name, "timer %d", i);
    demand[i].shutoff_timer = xTimerCreate(timer_name, PURGE_TIME, pdFALSE, (void*)&demand[i], &shutdownCallback);
  }
}

static void timerCallback( TimerHandle_t timer )
{
    ( void )timer;
    gpio_xor_mask( 1u << STATUS_LED );
}

static void gpioIRQCallback(uint gpio, uint32_t events){
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

// TODO: how could you pass in the array of demands to the callback
static void shutdownCallback(TimerHandle_t timer){
  Demand_t* demand = (Demand_t*) pvTimerGetTimerID(timer);
  gpio_put(demand->output_gpio, (demand->desired_state == OUTPUT_ACTIVE_HIGH)); 
}

//how do you pass in parameters
static void demandQueueRXTask(void *params){
  Demand_t pump_demand;
  (void)params;
  Demand_t *demands = (Demand_t*)params;

  for(;;){
    xQueueReceive(demand_queue, &pump_demand, portMAX_DELAY);
  
    printf("set pump %d state %d, \n",pump_demand.output_gpio, pump_demand.desired_state);
    if(pump_demand.desired_state == 0 ){
      xTimerStart(pump_demand.shutoff_timer, 0);    
    }else{
      xTimerStop(pump_demand.shutoff_timer, 0);
      gpio_put( pump_demand.output_gpio,( pump_demand.desired_state == OUTPUT_ACTIVE_HIGH));
    }
  
    uint desired_boiler_state = 0;
    for(uint i = 0; i < NUM_DEMAND_INPUTS; i++){
      if((demands+i)->desired_state == 1){
        desired_boiler_state = 1;
        break;
      }
    }
    gpio_put(BOILER_GPIO,(desired_boiler_state == OUTPUT_ACTIVE_HIGH));
  }
}

static void startupTask(void *params){
  (void)params;
  printf("startup task \n");
  for(uint i =0 ; i < NUM_DEMAND_INPUTS ; i++){
    xQueueSendToBack(demand_queue,
      &demand[i],
      0);
  }
  while(1);
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