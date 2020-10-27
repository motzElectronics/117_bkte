#include "../Tasks/Inc/task_get_new_bin.h"
extern osThreadId getEnergyHandle;
extern osThreadId webExchangeHandle;
extern osThreadId getTempHandle;
extern osThreadId getNewBinHandle;
extern osThreadId keepAliveHandle;
extern osThreadId loraHandle;

static char tmpBufPage[256];

extern HttpUrl urls;

u8 isRxNewFirmware = 0;

void taskGetNewBin(void const * argument){
	u32 szSoft = 100;
	u32 curSzSoft = 0;
	u32	szPartSoft;
	u8	resCode;
	u8 	cntHttpPostFail = 0;
	u32	flashAddrFirmware = FLASH_ADDR_BUF_NEW_FIRMWARE;
	PckgUpdFirmware pckgInfoFirmware;
	char*	pRxData;
	u8 csq = 0;
	vTaskSuspend(getNewBinHandle);
	vTaskSuspend(webExchangeHandle);
	vTaskSuspend(getEnergyHandle);
	vTaskSuspend(getTempHandle);
	vTaskSuspend(keepAliveHandle);
	vTaskSuspend(loraHandle);
	isRxNewFirmware = 1;
	while(simGetSzSoft(&szSoft) == SIM_FAIL){
		sdWriteLog(SD_ER_MSG_GET_SZ_NEW_BIN_MYFUN, SD_LEN_MYFUN, NULL, 0, &sdSectorLogError);
		D(printf("ERROR: simGetSzSoft"));
		osDelay(1000);
	}
	flashClearPage(FLASH_SECTOR_11);
	simHttpInit(urls.getPartFirmware);
	HAL_GPIO_WritePin(LED4G_GPIO_Port, LED4G_Pin, GPIO_PIN_SET);

	for(;;)
	{
		if((csq = simCheckCSQ()) > 12 && csq < 99){
				if(szSoft != curSzSoft){
					if(szSoft - curSzSoft > SZ_PART_NEW_SOFTWARE) szPartSoft = SZ_PART_NEW_SOFTWARE;
					else szPartSoft = szSoft - curSzSoft;
					pckgInfoFirmware.fromByte = curSzSoft;
					pckgInfoFirmware.toByte = szPartSoft + curSzSoft;
					pckgUpdFirmwareToJson(&pckgInfoFirmware, (char*)tmpBufPage, 256);
					if((resCode = httpPost((char*)tmpBufPage, strlen((char*)tmpBufPage), &pRxData, 10, 10000)) == SIM_SUCCESS
							&& isCrcOk(pRxData, szPartSoft)){
						cntHttpPostFail = 0;
						flashWrite(pRxData + 20,  szPartSoft / 2, &flashAddrFirmware);
						curSzSoft += szPartSoft;
						D(printf("OK: DOWNLOAD %d BYTES\r\n", (int)curSzSoft));
						HAL_GPIO_TogglePin(LED4G_GPIO_Port, LED4G_Pin);
					} else{
						D(printf("ERROR: httpPost() DOWNLOAD\r\n"));
						cntHttpPostFail++;
						if(cntHttpPostFail > 10) simReset();
						simHttpInit(urls.getPartFirmware);
					}
				} else{ 
					while(HAL_FLASH_Unlock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Unlock()\r\n"));
					FLASH_Erase_Sector(FLASH_SECTOR_3, VOLTAGE_RANGE_3);
					D(printf("FLASH_Erase_Sector\r\n"));
					while(HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_ID_BOOT, (u32)bkte.idBoot))
						D(printf("ERROR: HAL_FLASH_Program(BOOT_ADDR_ID_LOADER)\r\n"));
					while(HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_IS_NEW_FIRWARE, (u32)1))
						D(printf("ERROR: HAL_FLASH_Program(BOOT_ADDR_IS_NEW_FIRWARE)\r\n"));
					while(HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_SZ_NEW_FIRMWARE, (u32)(szSoft / 2)))
						D(printf("ERROR: HAL_FLASH_Program(FLASH_ADDR_SZ_NEW_FIRMWARE)\r\n"));
					while(HAL_FLASH_Lock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Lock()\r\n"));
					D(printf("BOOT_ID: %d\r\n", (int)getFlashData(FLASH_ADDR_ID_BOOT)));
					D(printf("IS_NEW_FIRMARE: %d\r\n", (int)getFlashData(FLASH_ADDR_IS_NEW_FIRWARE)));
					NVIC_SystemReset();
				}
			osDelay(1);
		} else {
			osDelay(1000);
		}
	}
  /* USER CODE END taskGetNewBin */
}