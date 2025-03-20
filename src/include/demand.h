#ifndef __demand_h_
#define __demand_h_

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

#include "queue.h"
#include "timers.h"
#include "pico/stdlib.h"

#define PURGE_TIME pdMS_TO_TICKS( 5000UL )
#define NUM_DEMAND_INPUTS 2

#define DEMAND_QUEUE_RX_TASK_PRIORITY	( tskIDLE_PRIORITY + 2 )

typedef struct {
  uint input_gpio;
  uint output_gpio;
  uint desired_state;
  TimerHandle_t debounce_timer;
  TimerHandle_t shutoff_timer;
} Demand_t;

typedef struct {
  Demand_t *demand;
  QueueHandle_t *queue;
} Demand_Queue_Params_t;

extern QueueHandle_t demand_queue;
extern Demand_t demand[];


void initDemands(Demand_t* demand);
void demandQueueRXTask(void *params);
void shutdownCallback(TimerHandle_t timer);

#endif
