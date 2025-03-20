#ifndef __startup_h_
#define __startup_h_

#include "FreeRTOS.h"

#define STARTUP_TASK_PRIORITY ( tskIDLE_PRIORITY + 1 )

void startupTask(void *params);

#endif
