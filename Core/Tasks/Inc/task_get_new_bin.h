#ifndef __TASK_GET_NEW_BIN_H
#define __TASK_GET_NEW_BIN_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/utils_flash.h"
#include "../Utils/Inc/utils_json.h"

#define SZ_PART_FIRMW   7000

void updBootInfo();
void lockAllTasks();
u32 getSzFirmware();

ErrorStatus getPartFirmware(u8* reqData, u8 sz, u8* answ, u16 szAnsw);
ErrorStatus getServerTime_test(u8* answ, u16 szAnsw);
#endif