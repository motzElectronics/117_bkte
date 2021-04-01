#include "../Tasks/Inc/task_get_new_bin.h"
#include "../Utils/Inc/utils_pckgs_manager.h"
extern osThreadId getEnergyHandle;
extern osThreadId webExchangeHandle;
extern osThreadId getTempHandle;
extern osThreadId getNewBinHandle;
extern osThreadId keepAliveHandle;
extern osThreadId loraHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId wirelessSensHandle;
extern osMutexId mutexWebHandle;

extern CircularBuffer circBufAllPckgs;

u8 isRxNewFirmware = 0;
static u8 bufNumBytesFirmware[8];
static PckgUpdFirmware pckgInfoFirmware;
static u8 partFirmware[SZ_PART_FIRMW + 1];
static u32 flashAddrFirmware = FLASH_ADDR_BUF_NEW_FIRMWARE;
static u32 szSoft = 0;

void taskGetNewBin(void const * argument) {
	u32 curSzSoft = 0;
	u32	szPartSoft;
	u8 cntFailTCPReq = 0;
        
        FLASH_Erase_Sector(FLASH_SECTOR_3, VOLTAGE_RANGE_3);
    
	vTaskSuspend(getNewBinHandle);
    
	lockAllTasks();
	isRxNewFirmware = 1;

    if (!bkte.isTCPOpen) {
	    while(openTcp() != TCP_OK);
    }

	while(!(szSoft = getSzFirmware()));
	flashClearPage(FLASH_SECTOR_11);
	clearAllWebPckgs();
	HAL_GPIO_WritePin(LED4G_GPIO_Port, LED4G_Pin, GPIO_PIN_SET);

	for(;;){
		if(szSoft != curSzSoft) {
			if (szSoft - curSzSoft > SZ_PART_FIRMW) {
                szPartSoft = SZ_PART_FIRMW;
			} else {
                szPartSoft = szSoft - curSzSoft;
            }
			pckgInfoFirmware.fromByte = curSzSoft;
			pckgInfoFirmware.toByte = szPartSoft + curSzSoft;
			memcpy(bufNumBytesFirmware, &pckgInfoFirmware.fromByte, 4);
			memcpy(bufNumBytesFirmware + 4, &pckgInfoFirmware.toByte, 4);
			memset(partFirmware, 0xFF, SZ_PART_FIRMW + 1);
            
            if (!bkte.isTCPOpen) {
                while(openTcp() != TCP_OK);
            }

			if(getPartFirmware(bufNumBytesFirmware, partFirmware, szPartSoft + 4, 8) == SUCCESS
                 && isCrcOk(partFirmware, szPartSoft)){
				curSzSoft += szPartSoft;
				D(printf("OK: DOWNLOAD %d BYTES\r\n", (int)curSzSoft));
				HAL_GPIO_TogglePin(LED4G_GPIO_Port, LED4G_Pin);
				cntFailTCPReq = 0;
				flashWrite(partFirmware,  szPartSoft, &flashAddrFirmware);
			} else {
				D(printf("ERROR: httpPost() DOWNLOAD\r\n"));
				cntFailTCPReq++;
				if(cntFailTCPReq > 10) {
                    simReset();
                }
			}
		} else { 
			updBootInfo();
			NVIC_SystemReset();
		}
	}
  /* USER CODE END taskGetNewBin */
}


void updBootInfo(){
	szSoft = szSoft % 4 == 0 ? szSoft : ((szSoft / 4) + 1) * 4;
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
	osMutexWait(mutexWebHandle, osWaitForever);
	vTaskSuspend(webExchangeHandle);
	vTaskSuspend(getEnergyHandle);
	vTaskSuspend(getTempHandle);
	vTaskSuspend(keepAliveHandle);
	// vTaskSuspend(loraHandle);
	vTaskSuspend(createWebPckgHandle);
    vTaskSuspend(wirelessSensHandle);
    osMutexRelease(mutexWebHandle);
	
}


u32 getSzFirmware(){
	u8 bufSzFirmware[4];
	if(generateWebPckgReq(CMD_REQUEST_SZ_FIRMWARE, NULL, 0, SZ_REQUEST_GET_SZ_FIRMWARE, bufSzFirmware, 4) == ERROR){
		sdWriteLog(SD_ER_SZ_FIRMWARE, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogs);
		D(printf("ERROR: sz firmware\r\n"));
		return 0;
	} else{
		u32 numFirmware = bufSzFirmware[0] << 24 | bufSzFirmware[1] << 16 | bufSzFirmware[2] << 8 | bufSzFirmware[3];
		D(printf("OK: sz firmware %d\r\n", numFirmware));
		return numFirmware;
	}
}


ErrorStatus getPartFirmware(u8* reqData, u8* answBuf, u16 szAnsw, u8 szReq){
	WebPckg* curPckg;
	ErrorStatus ret = SUCCESS;
	curPckg = createWebPckgReq(CMD_REQUEST_PART_FIRMWARE, reqData, szReq, SZ_REQUEST_GET_PART_FIRMWARE);
	osMutexWait(mutexWebHandle, osWaitForever);
	if(sendTcp(curPckg->buf, curPckg->shift) != TCP_OK) {
		sdWriteLog(SD_ER_PART_FIRMWARE, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogs);
		D(printf("ERROR: part Firmware\r\n"));
		HAL_GPIO_WritePin(LED4R_GPIO_Port, LED4R_Pin, GPIO_PIN_SET);
		ret = ERROR;
	} else {
		waitIdleCnt("wait IDLE part firmware", &(uInfoSim.irqFlags), szAnsw / SZ_TCP_PCKG + 1, 100, 6000);
		osDelay(100);
		HAL_GPIO_WritePin(LED4R_GPIO_Port, LED4R_Pin, GPIO_PIN_RESET);
		HAL_GPIO_TogglePin(LED4G_GPIO_Port, LED4G_Pin);
		memcpy(answBuf, &uInfoSim.pRxBuf[11], szAnsw);
	}
	osMutexRelease(mutexWebHandle);
    clearWebPckg(curPckg);
	return ret;
}


