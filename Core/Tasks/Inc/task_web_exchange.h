#ifndef __TASK_WEB_EXCHANGE_H
#define __TASK_WEB_EXCHANGE_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../Drivers/Inc/spiflash.h"
#include "../Drivers/Inc/simcom.h"
#include "../Utils/Inc/utils_pckgs_manager.h"


u8 openSendTcp(u8* data, u16 sz);
u8 fastSendTcp(u8 statSend);
u8 procReturnStatus(u8 ret);





#endif