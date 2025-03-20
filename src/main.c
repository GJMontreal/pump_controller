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

#include "demand.h"
#include "gpio.h"
#include "startup.h"
#include "status_led.h"

static char event_str[128];

#define DEBOUNCE_TIMER_PERIOD pdMS_TO_TICKS( 1000UL)
#define PURGE_TIME pdMS_TO_TICKS( 5000UL )

static void setupHardware(void);
static void setupDemands(Demand_t* demand);

// FreeRTOS callbacks
static void debounceCallback(TimerHandle_t timer);

static void gpioIRQCallback(uint gpio, uint32_t events);

static TimerHandle_t debounce_timer[NUM_DEMAND_INPUTS];

QueueHandle_t demand_queue = NULL;
Demand_t demand[NUM_DEMAND_INPUTS];

#define DEMAND_QUEUE_RX_TASK_PRIORITY	( tskIDLE_PRIORITY + 2 )
#define STARTUP_TASK_PRIORITY ( tskIDLE_PRIORITY + 1 )

int main(){
  Demand_Queue_Params_t queue_params; 

  setupHardware();
  printf("initializing pump_controller.\n");
  
  setupDemands(&demand[0]);

  demand_queue =
      xQueueCreate(NUM_DEMAND_INPUTS,
                   sizeof(Demand_t));
  if (demand_queue != NULL) {
    
    queue_params.demand = &demand[0];
    queue_params.queue = &demand_queue;
    xTaskCreate(demandQueueRXTask, "DemandRx", configMINIMAL_STACK_SIZE, &queue_params,
                DEMAND_QUEUE_RX_TASK_PRIORITY, NULL);

    xTaskCreate(startupTask, "Startup", configMINIMAL_STACK_SIZE, &queue_params,
                STARTUP_TASK_PRIORITY, NULL);
  }

  TimerHandle_t LEDTimer = setupStatusLED();
  if(LEDTimer != NULL){
    printf("starting heartbeat timer \n");
    xTimerStart(LEDTimer, 0);
  }
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

static void setupDemands(Demand_t *demand){
  char timer_name[12];
  for(uint i=0; i < NUM_DEMAND_INPUTS; i++){
    sprintf(timer_name, "timer %d", i);
    demand[i].shutoff_timer = xTimerCreate(timer_name, PURGE_TIME, pdFALSE, (void*)&demand[i], &shutdownCallback);
    debounce_timer[i]= NULL;
    demand[i].input_gpio = DEMAND_1_GPIO + i;
    demand[i].desired_state = gpio_get(DEMAND_1_GPIO + i);
    demand[i].output_gpio = PUMP_1_GPIO + i;
  }
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





