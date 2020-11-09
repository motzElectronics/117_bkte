#ifndef __TASK_WEB_EXCHANGE_H
#define __TASK_WEB_EXCHANGE_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../Drivers/Inc/spiflash.h"
#include "../Drivers/Inc/simcom.h"
#include "../Utils/Inc/utils_json.h"

#define PCKG_WAS_lOST   1
#define PCKG_WAS_SENT   0

u8 sendDataToServer();
void saveCsq(u8 csq);




#endif