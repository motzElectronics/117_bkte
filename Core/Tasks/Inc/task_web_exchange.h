#ifndef __TASK_WEB_EXCHANGE_H
#define __TASK_WEB_EXCHANGE_H

#include "main.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/circularBuffer.h"
#include "../Drivers/Inc/spiflash.h"
#include "../Drivers/Inc/simcom.h"
#include "../Utils/Inc/utils_json.h"
#include "../Utils/Inc/utils_pckgs_manager.h"

#define PCKG_WAS_lOST   1
#define PCKG_WAS_SENT   0



typedef struct{
    u8 iter;
    u8 type;
    u8 szType;
    u8 buf[255]; 
}Page;

u8 sendDataToServer();
void saveCsq(u8 csq);

void clearPage(Page* pg);
void clearAllPages();
void parceData(u8* tmpBufPage, u8 len);
u16 getSzAllPages();
void addToPage(Page* pg, u8* src, u8 sz);
void addPagesToWebPckg();





#endif