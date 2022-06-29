#ifndef __TASK_KEEP_ALIVE_H
#define __TASK_KEEP_ALIVE_H

#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/utils_sd.h"
#include "adc.h"
#include "cmsis_os.h"
#include "main.h"

u16  getAdcVoltBat();
void pwrOffBkte();
void updRTC();
void generateMsgKeepAlive();
void generateMsgBat();
void generateMsgDevOff();

ErrorStatus sendMsgFWUpdated();
ErrorStatus sendMsgTaskStat();
ErrorStatus sendMsgFWUpdateBegin();
ErrorStatus sendMsgDevOff();
ErrorStatus sendMsgDevOffValue(u32 val);
ErrorStatus sendInitTelemetry();

#endif