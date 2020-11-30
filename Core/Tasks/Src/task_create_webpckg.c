#include "../Tasks/Inc/task_create_webpckg.h"
extern osThreadId webExchangeHandle;
extern osThreadId keepAliveHandle;
extern osThreadId getNewBinHandle;
extern osThreadId createWebPckgHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;
extern osMessageQId queueWebPckgHandle;


extern CircularBuffer circBufPckgEnergy;
extern u8 SZ_PCKGENERGY;
static char tmpBufPage[256];
extern HttpUrl urls;

extern PckgJsonEn pckgJsonEn;
static Page pgEnergy = {.type = CMD_DATA_ENERGY, .szType = SZ_CMD_ENERGY};
static Page pgTemp = {.type = CMD_DATA_TEMP, .szType = SZ_CMD_TEMP};
static Page pgVoltAmper = {.type = CMD_DATA_VOLTAMPER, .szType = SZ_CMD_VOLTAMPER};
static Page* allPages[] = {&pgVoltAmper, &pgEnergy, &pgTemp}; 
static WebPckg* curPckg;

void taskCreateWebPckg(void const * argument){
// vTaskSuspend(webExchangeHandle);    

	u32 notTxCntPckg = 0;
	u32 allTxPckg = 0;
	u32 tmpTimeStamp;
	s16 delayPages;
	u16 tmpCntPckg = 0;
	u16 szAllPages = 0;
	u8	amntPages;
	u8 len;
	u8 test = 0;
	
	offAllLeds();
	vTaskSuspend(createWebPckgHandle);
	// simOn();
	
	/*simHttpInit(urls.addMeasure);*///!use in http
	// vTaskResume(keepAliveHandle);
//	vTaskSuspend(getNewBinHandle);
//	vTaskSuspend(webExchangeHandle);

//	HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);

	for(;;){
		delayPages = spiFlash64.headNumPg >= spiFlash64.tailNumPg ? spiFlash64.headNumPg - spiFlash64.tailNumPg : 
			spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
		while(delayPages > 0 && isNotFullPckg()){
			curPckg = getFreePckg();
			clearAllPages();
			amntPages = delayPages > AMOUNT_MAX_PAGES ? AMOUNT_MAX_PAGES : delayPages;
			for(u8 i = 0; i < amntPages; i++){
				spiFlashRdPg((u8*)tmpBufPage, 256, 0, spiFlash64.tailNumPg);

				/*if(!spiFlash64.tailNumPg){
					fillTelemetry(&tmpPckgEnergy, TEL_SERV_FLASH_CIRC_BUF_END_TAIL, 0);
					cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
				}*/

				if((len = isDataFromFlashOk(tmpBufPage, 256))){
					D(printf("OK: good crc\r\n"));
					parceData(tmpBufPage, len);
				}

				/*while(getGnssPckg((u8*)tmpBufPage, 256, &tmpPckgEnergy, SZ_PCKGENERGY)){
					tmpTimeStamp = addEnPckgToJson(&tmpPckgEnergy);
					if(tmpTimeStamp > BKTE_BAD_TIMESTAMP){
						fillTelemetry(&tmpPckgEnergy, TEL_BAD_RTC_TIME, 0);
						cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
					}
				}*/
			}
			szAllPages = getSzAllPages();
			initWebPckg(curPckg, szAllPages);
			addPagesToWebPckg(curPckg);
			xQueueSendToBack(queueWebPckgHandle, &curPckg, portMAX_DELAY);
			test++;
			if(test == 3){
				D(printf("UNLOCK vTaskResume(webExchangeHandle)\r\n"));
				vTaskResume(webExchangeHandle);
			}
			delayPages = spiFlash64.headNumPg >= spiFlash64.tailNumPg ? spiFlash64.headNumPg - spiFlash64.tailNumPg : 
				spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);

		}
		D(printf("no pckg in spiflash\r\n"));
		osDelay(3000);
	}
}

void clearAllPages(){
	for(u8 i = 0; i < 3; i++){
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
	for(u8 i = 0; i < 3; i++){
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
	for(u8 i = 0; i < 3; i++){
		if(allPages[i]->iter){
			addInfoToWebPckg(pckg, allPages[i]->buf, allPages[i]->iter, allPages[i]->iter / allPages[i]-> szType, allPages[i]->type);
		}
	}
	closeWebPckg(pckg);

}