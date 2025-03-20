#include "startup.h"

#include "demand.h"
#include "queue.h"

#include <stdio.h>

void startupTask(void *params) {
  QueueHandle_t *queue = ((Demand_Queue_Params_t *)params)->queue;
  Demand_t *all_demands = ((Demand_Queue_Params_t *)params)->demand;

  printf("startup task \n");
  for (uint i = 0; i < NUM_DEMAND_INPUTS; i++) {
    xQueueSendToBack(*queue, &all_demands[i], 0);
  }
  while (1)
    ;
}
