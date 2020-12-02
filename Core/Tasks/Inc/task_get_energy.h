#ifndef __TASK_GET_ENERGY_H
#define __TASK_GET_ENERGY_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_sd.h"

void unLockTasks();
void generateInitTelemetry();

#endif