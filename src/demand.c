#include "demand.h"

#include "gpio.h"

#include <stdio.h>


void demandQueueRXTask(void *params){
  Demand_t pump_demand;
  QueueHandle_t *queue = (( Demand_Queue_Params_t* )params)->queue;
  Demand_t *all_demands = (( Demand_Queue_Params_t* )params)->demand;

  for(;;){
    xQueueReceive(*queue, &pump_demand, portMAX_DELAY);
  
    printf("set pump %d state %d, \n",pump_demand.output_gpio, pump_demand.desired_state);
    if(pump_demand.desired_state == 0 ){
      xTimerStart(pump_demand.shutoff_timer, 0);    
    }else{
      xTimerStop(pump_demand.shutoff_timer, 0);
      gpio_put( pump_demand.output_gpio,( pump_demand.desired_state == OUTPUT_ACTIVE_HIGH));
    }
  
    uint desired_boiler_state = 0;
    for(uint i = 0; i < NUM_DEMAND_INPUTS; i++){
      if((all_demands+i)->desired_state == 1){
        desired_boiler_state = 1;
        break;
      }
    }
    gpio_put(BOILER_GPIO,(desired_boiler_state == OUTPUT_ACTIVE_HIGH));
  }
}

void shutdownCallback(TimerHandle_t timer){
  Demand_t* demand = (Demand_t*) pvTimerGetTimerID(timer);
  gpio_put(demand->output_gpio, (demand->desired_state == OUTPUT_ACTIVE_HIGH)); 
}