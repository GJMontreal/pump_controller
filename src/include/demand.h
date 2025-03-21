#ifndef __demand_h_
#define __demand_h_

#include "FreeRTOS.h"

#include "queue.h"
#include "timers.h"

#define PURGE_TIME pdMS_TO_TICKS(5 * 1000UL)
#define DEBOUNCE_TIMER_PERIOD pdMS_TO_TICKS(1000UL)

#define NUM_DEMAND_INPUTS 2

#define DEMAND_QUEUE_RX_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

typedef struct {
  uint input_gpio;
  uint output_gpio;
  uint desired_state;
  TimerHandle_t debounce_timer;
  TimerHandle_t shutoff_timer;
} Demand_t;

typedef struct {
  Demand_t *demand; // first demand in array
  QueueHandle_t *queue;
} Demand_Queue_Params_t;

extern QueueHandle_t demand_queue;

void initDemands();
void demandQueueRXTask(void *params);
void shutdownCallback(TimerHandle_t timer);

Demand_t *demandForInput(uint gpio);

#endif
