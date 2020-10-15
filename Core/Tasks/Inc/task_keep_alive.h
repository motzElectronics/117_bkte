#ifndef __TASK_KEEP_ALIVE_H
#define __TASK_KEEP_ALIVE_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/circularBuffer.h"
#include "adc.h"
#include "../Utils/Inc/utils_sd.h"

u16 getAdcVoltBat();
void pwrOffBkte();
void updRTC();
void generateMsgKeepAlive();
void generateMsgDevOff();
#endif