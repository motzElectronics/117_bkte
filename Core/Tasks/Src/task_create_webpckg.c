#include "../Tasks/Inc/task_create_webpckg.h"
extern osThreadId webExchangeHandle;
extern osThreadId keepAliveHandle;
extern osThreadId getNewBinHandle;
extern osThreadId createWebPckgHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;
extern osMessageQId queueWebPckgHandle;
extern osSemaphoreId semCreateWebPckgHandle;


extern CircularBuffer circBufPckgEnergy;
extern u8 SZ_PCKGENERGY;
static char tmpBufPage[256];

static Page pgEnergy = {.type = CMD_DATA_ENERGY, .szType = SZ_CMD_ENERGY};
static Page pgTemp = {.type = CMD_DATA_TEMP, .szType = SZ_CMD_TEMP};
static Page pgVoltAmper = {.type = CMD_DATA_VOLTAMPER, .szType = SZ_CMD_VOLTAMPER};
static Page pgTelemetry = {.type = CMD_DATA_TELEMETRY, .szType = SZ_CMD_TELEMETRY};
static Page* allPages[] = {&pgVoltAmper, &pgEnergy, &pgTemp, &pgTelemetry}; 
static WebPckg* curPckg;

void taskCreateWebPckg(void const * argument){
// vTaskSuspend(webExchangeHandle);    

	s16 delayPages;
	u16 szAllPages = 0;
	u8	amntPages;
	u8 len;
	xSemaphoreTake(semCreateWebPckgHandle, 1);
	
	offAllLeds();
	vTaskSuspend(createWebPckgHandle);

	for(;;){
		delayPages = spiFlash64.headNumPg >= spiFlash64.tailNumPg ? spiFlash64.headNumPg - spiFlash64.tailNumPg : 
			spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
		while((delayPages > BKTE_THRESHOLD_CNT_PAGES || (xSemaphoreTake(semCreateWebPckgHandle, 1) == pdTRUE)) && (curPckg = getFreePckg()) != NULL){
			clearAllPages();
			amntPages = delayPages > AMOUNT_MAX_PAGES ? AMOUNT_MAX_PAGES : delayPages;
			for(u8 i = 0; i < amntPages; i++){
				spiFlashRdPg((u8*)tmpBufPage, 256, 0, spiFlash64.tailNumPg);

				if((len = isDataFromFlashOk(tmpBufPage, 256))){
					parceData(tmpBufPage, len);
				} else{
                                  D(printf("ERROR: bad crc isDataFromFlashOk()\r\n"));
                                }
			}
			szAllPages = getSzAllPages();
			initWebPckg(curPckg, szAllPages, 0);
			addPagesToWebPckg(curPckg);
			xQueueSendToBack(queueWebPckgHandle, &curPckg, portMAX_DELAY);
			delayPages = spiFlash64.headNumPg >= spiFlash64.tailNumPg ? spiFlash64.headNumPg - spiFlash64.tailNumPg : 
				spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);

		}

		if(!delayPages){
			D(printf("no pckg in spiflash\r\n"));
			bkte.isSentData = 1;
		}
		osDelay(1000);
	}
}

void clearAllPages(){
	for(u8 i = 0; i < 4; i++){
		clearPage(allPages[i]);
	}
}


void parceData(u8* tmpBufPage, u8 len){
	u8 i = 0;
	while(i < len){
		switch(tmpBufPage[i]){
			case CMD_DATA_ENERGY:
				addToPage(&pgEnergy, &tmpBufPage[i + 1], SZ_CMD_ENERGY);
				i += (SZ_CMD_ENERGY + 1);
				break;
			case CMD_DATA_VOLTAMPER:
				addToPage(&pgVoltAmper, &tmpBufPage[i + 1], SZ_CMD_VOLTAMPER);
				i += (SZ_CMD_VOLTAMPER + 1);
				break;
			case CMD_DATA_TEMP:
				addToPage(&pgTemp, &tmpBufPage[i + 1], SZ_CMD_TEMP);
				i += (SZ_CMD_TEMP + 1);
				break;
			case CMD_DATA_TELEMETRY:
				addToPage(&pgTelemetry, &tmpBufPage[i + 1], SZ_CMD_TELEMETRY);
				i += (SZ_CMD_TELEMETRY + 1);
				break;
			default:
				D(printf("ER: CMD_DATA_X is wrong\r\n"));
				return;
				break;
			
		}
	}
}

void clearPage(Page* pg){
	if(pg->iter){
		pg->iter = 0;
		memset(pg->buf, SZ_PAGES, '\0');
	}
}

u16 getSzAllPages(){
	u16 sz = 0;
	for(u8 i = 0; i < 4; i++){
		if(allPages[i]->iter){
			sz += (allPages[i]->iter + 1 + 1); // sz + sizeof(type) + szeof(cnt)
		}
	}
	return sz;
}

void addToPage(Page* pg, u8* src, u8 sz){
	memcpy(&pg->buf[pg->iter], src, sz);
	pg->iter += sz;
}

void addPagesToWebPckg(WebPckg* pckg){
	for(u8 i = 0; i < 4; i++){
		if(allPages[i]->iter){
			addInfoToWebPckg(pckg, allPages[i]->buf, allPages[i]->iter, allPages[i]->iter / allPages[i]-> szType, allPages[i]->type);
		}
	}
	closeWebPckg(pckg);
	showWebPckg(pckg);

}