#include "../Tasks/Inc/task_get_new_bin.h"
extern osThreadId getEnergyHandle;
extern osThreadId webExchangeHandle;
extern osThreadId getTempHandle;
extern osThreadId getNewBinHandle;
extern osThreadId keepAliveHandle;
extern osThreadId loraHandle;

u8 isRxNewFirmware = 0;
static u8 bufNumBytesFirmware[8];
static PckgUpdFirmware pckgInfoFirmware;
static u8 partFirmware[SZ_WEB_PCKG];
static u32 flashAddrFirmware = FLASH_ADDR_BUF_NEW_FIRMWARE;
static u32 szSoft = 0;

void taskGetNewBin(void const * argument){
	u32 curSzSoft = 0;
	u32	szPartSoft;
	u8 cntFailTCPReq = 0;

	lockAllTasks();
	isRxNewFirmware = 1;
	while(!(szSoft = getSzFirmware())){
	}
	flashClearPage(FLASH_SECTOR_11);
	HAL_GPIO_WritePin(LED4G_GPIO_Port, LED4G_Pin, GPIO_PIN_SET);

	for(;;){
		if(szSoft != curSzSoft){
			if(szSoft - curSzSoft > SZ_PART_NEW_SOFTWARE) szPartSoft = SZ_PART_NEW_SOFTWARE;
			else szPartSoft = szSoft - curSzSoft;
			pckgInfoFirmware.fromByte = curSzSoft;
			pckgInfoFirmware.toByte = szPartSoft + curSzSoft;
			memcpy(bufNumBytesFirmware, &pckgInfoFirmware.fromByte, 4);
			memcpy(bufNumBytesFirmware + 4, &pckgInfoFirmware.toByte, 4);
			if(getPartFirmware(bufNumBytesFirmware, 8, partFirmware, SZ_WEB_PCKG) == SUCCESS && isCrcOk(partFirmware, SZ_PART_NEW_SOFTWARE)){
				curSzSoft += szPartSoft;
				D(printf("OK: DOWNLOAD %d BYTES\r\n", (int)curSzSoft));
				HAL_GPIO_TogglePin(LED4G_GPIO_Port, LED4G_Pin);
				cntFailTCPReq = 0;
				// flashWrite(pRxData + 20,  szPartSoft / 2, &flashAddrFirmware);
			} else {
				D(printf("ERROR: httpPost() DOWNLOAD\r\n"));
				cntFailTCPReq++;
				if(cntFailTCPReq > 10) simReset();
			}
		} else{ 
			updBootInfo();
			NVIC_SystemReset();
		}
	}
  /* USER CODE END taskGetNewBin */
}


void updBootInfo(){
	while(HAL_FLASH_Unlock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Unlock()\r\n"));
	FLASH_Erase_Sector(FLASH_SECTOR_3, VOLTAGE_RANGE_3);
	D(printf("FLASH_Erase_Sector\r\n"));
	while(HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_ID_BOOT, BKTE_ID_BOOT))
		D(printf("ERROR: HAL_FLASH_Program(BOOT_ADDR_ID_LOADER)\r\n"));
	while(HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_IS_NEW_FIRWARE, (u32)1))
		D(printf("ERROR: HAL_FLASH_Program(BOOT_ADDR_IS_NEW_FIRWARE)\r\n"));
	while(HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_SZ_NEW_FIRMWARE, (u32)(szSoft)))
		D(printf("ERROR: HAL_FLASH_Program(FLASH_ADDR_SZ_NEW_FIRMWARE)\r\n"));
	while(HAL_FLASH_Lock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Lock()\r\n"));
	D(printf("BOOT_ID: %d\r\n", (int)getFlashData(FLASH_ADDR_ID_BOOT)));
	D(printf("IS_NEW_FIRMARE: %d\r\n", (int)getFlashData(FLASH_ADDR_IS_NEW_FIRWARE)));
}


void lockAllTasks(){
	vTaskSuspend(getNewBinHandle);
	vTaskSuspend(webExchangeHandle);
	vTaskSuspend(getEnergyHandle);
	vTaskSuspend(getTempHandle);
	vTaskSuspend(keepAliveHandle);
	vTaskSuspend(loraHandle);
}