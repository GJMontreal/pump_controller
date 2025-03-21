#include "demand.h"

#include "gpio.h"

#include "startup.h"

#include <stdio.h>

QueueHandle_t demand_queue = NULL;

Demand_Queue_Params_t queue_params;
Demand_t demands[NUM_DEMAND_INPUTS];

static void debounceCallback(TimerHandle_t timer) {
  Demand_t* demand = (Demand_t*)pvTimerGetTimerID(timer);
  
  uint gpio_state = (gpio_get(demand->input_gpio) == INPUT_ACTIVE_HIGH);
  
  if (gpio_state == demand->desired_state) {
    xQueueSendToBackFromISR(demand_queue, demand, 0);
  }
}

Demand_t* demandForInput(uint gpio){
  Demand_t* demand = NULL;
  for(uint i = 0; i < NUM_DEMAND_INPUTS; i++)
  {
    if(demands[i].input_gpio == gpio){
      demand = &demands[i];
      break;
    }
  }
  return demand;
}


void initDemands() {
  char timer_name[12];
  for (uint i = 0; i < NUM_DEMAND_INPUTS; i++) {
    sprintf(timer_name, "timer %d", i);
    demands[i].shutoff_timer = xTimerCreate(
        timer_name, PURGE_TIME, pdFALSE, (void *)&demands[i], &shutdownCallback);
    demands[i].debounce_timer = xTimerCreate(timer_name, DEBOUNCE_TIMER_PERIOD, pdFALSE,
      NULL, &debounceCallback);
    demands[i].input_gpio = DEMAND_1_GPIO + i;
    demands[i].desired_state = (gpio_get(DEMAND_1_GPIO + i) == INPUT_ACTIVE_HIGH);
    demands[i].output_gpio = PUMP_1_GPIO + i;
  }

  demand_queue = xQueueCreate(NUM_DEMAND_INPUTS, sizeof(Demand_t));
  if (demand_queue != NULL) {

    queue_params.demand = demands;
    queue_params.queue = &demand_queue;
    xTaskCreate(demandQueueRXTask, "DemandRx", configMINIMAL_STACK_SIZE,
                &queue_params, DEMAND_QUEUE_RX_TASK_PRIORITY, NULL);

    xTaskCreate(startupTask, "Startup", configMINIMAL_STACK_SIZE, &queue_params,
                STARTUP_TASK_PRIORITY, NULL);
  }
}

void demandQueueRXTask(void *params) {
  Demand_t pump_demand;
  QueueHandle_t *queue = ((Demand_Queue_Params_t *)params)->queue;
  Demand_t *all_demands = ((Demand_Queue_Params_t *)params)->demand;

  for (;;) {
    xQueueReceive(*queue, &pump_demand, portMAX_DELAY);

    printf("set pump %d state %d, \n", pump_demand.output_gpio,
           pump_demand.desired_state);
    if (pump_demand.desired_state == 0) {
      xTimerStart(pump_demand.shutoff_timer, 0);
    } else {
      xTimerStop(pump_demand.shutoff_timer, 0);
      gpio_put(pump_demand.output_gpio,
               (pump_demand.desired_state == OUTPUT_ACTIVE_HIGH));
    }

    uint desired_boiler_state = 0;
    for (uint i = 0; i < NUM_DEMAND_INPUTS; i++) {
      if ((all_demands + i)->desired_state == 1) {
        desired_boiler_state = 1;
        break;
      }
    }
    gpio_put(BOILER_GPIO, (desired_boiler_state == OUTPUT_ACTIVE_HIGH));
  }
}

void shutdownCallback(TimerHandle_t timer) {
  Demand_t *demand = (Demand_t *)pvTimerGetTimerID(timer);
  gpio_put(demand->output_gpio, (demand->desired_state == OUTPUT_ACTIVE_HIGH));
}