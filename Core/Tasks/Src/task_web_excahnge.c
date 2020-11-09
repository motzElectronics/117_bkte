#include "../Tasks/Inc/task_web_exchange.h"
extern osThreadId webExchangeHandle;
extern osThreadId keepAliveHandle;
extern osThreadId getNewBinHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;

extern CircularBuffer circBufPckgEnergy;
extern u8 SZ_PCKGENERGY;
static char tmpBufPage[256];
static u8 CNT_MAX_PAGES_TX;
extern HttpUrl urls;

extern PckgJsonEn pckgJsonEn;

void taskWebExchange(void const * argument){
    // vTaskSuspend(webExchangeHandle);    
	PckgEnergy tmpPckgEnergy = {.preambule=BKTE_PREAMBLE_EN};
	s16 delayPages;
	u16 CNT_CUR_PAGES_TX;
	u16 tmpCntPckg = 0;
	u32 notTxCntPckg = 0;
	u32 allTxPckg = 0;
	u32 tmpTimeStamp;
	offAllLeds();
	vTaskSuspend(webExchangeHandle);
	// simOn();

	CNT_MAX_PAGES_TX = SZ_MAX_TX_DATA / spiFlash64.pgSz / 2 - 1;
	simHttpInit(urls.addMeasure);
	// vTaskResume(keepAliveHandle);
//	vTaskSuspend(getNewBinHandle);
//	vTaskSuspend(webExchangeHandle);

//	HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);

	for(;;){
		while(spiFlash64.headNumPg != spiFlash64.tailNumPg){
			if(spiFlash64.headNumPg > spiFlash64.tailNumPg)
				delayPages = spiFlash64.headNumPg - spiFlash64.tailNumPg;
			else
				delayPages = spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
			CNT_CUR_PAGES_TX = delayPages > CNT_MAX_PAGES_TX ? CNT_MAX_PAGES_TX : delayPages;
			for(u8 i = 0; i < CNT_CUR_PAGES_TX; i++){
				
				spiFlashRdPg((u8*)tmpBufPage, 256, 0, spiFlash64.tailNumPg);

				if(!spiFlash64.tailNumPg){
					fillTelemetry(&tmpPckgEnergy, TEL_SERV_FLASH_CIRC_BUF_END_TAIL, 0);
					cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
					/*xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
					cBufWriteToBuf(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY);
					xSemaphoreGive(mutexWriteToEnergyBufHandle);*/
				}

				while(getGnssPckg((u8*)tmpBufPage, 256, &tmpPckgEnergy, SZ_PCKGENERGY)){
					tmpTimeStamp = addEnPckgToJson(&tmpPckgEnergy);
					if(tmpTimeStamp > BKTE_BAD_TIMESTAMP){
						fillTelemetry(&tmpPckgEnergy, TEL_BAD_RTC_TIME, 0);
						cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
						/*xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
						cBufWriteToBuf(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY);
						xSemaphoreGive(mutexWriteToEnergyBufHandle);*/
					}
				}
			}
			if(pckgJsonEn.numPckg){
				tmpCntPckg = closeGnssJson();
				allTxPckg += tmpCntPckg;
				if(sendDataToServer() == PCKG_WAS_lOST){
					notTxCntPckg += tmpCntPckg;
					fillTelemetry(&tmpPckgEnergy, TEL_BAD_RESPONSE_SERVER, (u32)((float)notTxCntPckg / allTxPckg * 1000));
					cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
					/*xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
					cBufWriteToBuf(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY);
					xSemaphoreGive(mutexWriteToEnergyBufHandle);*/
				}
			} else {
				fillTelemetry(&tmpPckgEnergy, TEL_BAD_ALL_CRC, 0);
				cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
				/*xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
				cBufWriteToBuf(&circBufPckgEnergy, (u8*)&tmpPckgEnergy, SZ_PCKGENERGY);
				xSemaphoreGive(mutexWriteToEnergyBufHandle);*/
			}
		}
		D(printf("no pckg in spiflash\r\n"));
		osDelay(3000);
	}
}

u8 sendDataToServer(){
	u8 resCode;
	char* pRxData;
	u8 cntHttpPostFail = 1;
	u8 tmpIdFirmware;
	u8 ret = PCKG_WAS_SENT;

	xSemaphoreTake(mutexWebHandle, portMAX_DELAY);
	while(cntHttpPostFail){
		waitGoodCsq();
		
		if((resCode = httpPost(pckgJsonEn.jsonEnTxBuf, strlen(pckgJsonEn.jsonEnTxBuf), &pRxData, 
		10, 10000)) != SIM_SUCCESS){
			sdWriteLog(SD_ER_MSG_HTTPPOST_MYFUN, SD_LEN_MYFUN, NULL, 0, &sdSectorLogError);
			D(printf("ERROR: httpPost()\r\n"));
			
			cntHttpPostFail++;
//			HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
			if(cntHttpPostFail == 3){
				simReset();
			}else if(cntHttpPostFail == 4){
				cntHttpPostFail = 0;
				simReset();
				ret = PCKG_WAS_lOST;
			}
			simHttpInit(urls.addMeasure);	
		} else{
			cntHttpPostFail = 0;
			D(printf("OK: httpPost()\r\n"));
			HAL_GPIO_TogglePin(LED2G_GPIO_Port, LED2G_Pin);
			if((tmpIdFirmware = atoi(pRxData + 11)) != bkte.idFirmware && tmpIdFirmware > 0){
				sdWriteLog(SD_MSG_NEW_BIN, SD_LEN_MYFUN, NULL, 0, &sdSectorLogs);
				D(printf("New FIRMWARE v.:%d\r\n", (int)tmpIdFirmware));
				vTaskResume(getNewBinHandle);
			}
		}

	}
	xSemaphoreGive(mutexWebHandle);
	return ret;
}

void saveCsq(u8 csq){
	PckgEnergy pckgEnergy = {.preambule=BKTE_PREAMBLE_EN};
	fillTelemetry(&pckgEnergy, TEL_LVL_CSQ, csq);
	cBufSafeWrite(&circBufPckgEnergy, (u8*)&pckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
	/*xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
	cBufWriteToBuf(&circBufPckgEnergy, (u8*)&pckgEnergy, SZ_PCKGENERGY);
	xSemaphoreGive(mutexWriteToEnergyBufHandle);*/
}



