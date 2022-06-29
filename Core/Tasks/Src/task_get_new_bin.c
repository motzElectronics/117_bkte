#include "../Tasks/Inc/task_get_new_bin.h"

#include "../Tasks/Inc/task_iwdg.h"
#include "../Tasks/Inc/task_keep_alive.h"
#include "../Utils/Inc/utils_crc.h"
#include "../Utils/Inc/utils_pckgs_manager.h"

extern u16 iwdgTaskReg;

extern osThreadId getEnergyHandle;
extern osThreadId webExchangeHandle;
extern osThreadId getTempHandle;
extern osThreadId getNewBinHandle;
extern osThreadId keepAliveHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId wirelessSensHandle;
extern osTimerId  timerPowerOffHandle;
extern osMutexId  mutexWriteToEnergyBufHandle;
extern osMutexId  mutexWebHandle;
extern osMutexId  mutexRTCHandle;
extern osMutexId  mutexSDHandle;
extern osMutexId  mutexSpiFlashHandle;

extern CircularBuffer circBufAllPckgs;

u8                     isRxNewFirmware = 0;
static u8              bufNumBytesFirmware[8];
static PckgUpdFirmware pckgInfoFirmware;
static u8              partNewFW[SZ_PART_FIRMW + 1];
static u32             flashAddrFirmware = FLASH_ADDR_BUF_NEW_FIRMWARE;
static u32             szNewFW = 0;
static u32             crcNewFW;

void taskGetNewBin(void const* argument) {
    u32 curSzSoft = 0;
    u32 szPartSoft;
    u8  cntFailTCPReq = 0;

    FLASH_Erase_Sector(FLASH_SECTOR_3, VOLTAGE_RANGE_3);

    vTaskSuspend(getNewBinHandle);

    lockAllTasks();
    isRxNewFirmware = 1;
    bkte.isTCPOpen = 0;
    osTimerStop(timerPowerOffHandle);

    while (!(szNewFW = getSzFirmware()))
        ;
    bkte.szNewFirmware = szNewFW;
    if (sendMsgFWUpdateBegin() != SUCCESS) {
        D(printf("ERROR: Send FW UPDATED\r\n"));
    }

    flashClearPage(FLASH_SECTOR_11);
    clearAllWebPckgs();
    HAL_GPIO_WritePin(LED4G_GPIO_Port, LED4G_Pin, GPIO_PIN_SET);
    crcNewFW = 0xffffffff;

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_NEW_BIN;
        bkte.stat.new_bin++;
        if (szNewFW != curSzSoft) {
            if (szNewFW - curSzSoft > SZ_PART_FIRMW) {
                szPartSoft = SZ_PART_FIRMW;
            } else {
                szPartSoft = szNewFW - curSzSoft;
            }
            pckgInfoFirmware.fromByte = curSzSoft;
            pckgInfoFirmware.toByte = szPartSoft + curSzSoft;
            memcpy(bufNumBytesFirmware, &pckgInfoFirmware.fromByte, 4);
            memcpy(bufNumBytesFirmware + 4, &pckgInfoFirmware.toByte, 4);
            memset(partNewFW, 0xFF, SZ_PART_FIRMW + 1);

            if (!bkte.isTCPOpen) {
                while (openTcp() != TCP_OK) {}
                cntFailTCPReq = 0;
            }

            if (getPartFirmware(bufNumBytesFirmware, partNewFW, szPartSoft + 4, 8) == SUCCESS &&
                isCrcOk(partNewFW, szPartSoft)) {
                crc32_chank(&crcNewFW, partNewFW, szPartSoft);
                // D(printf("crcNewFW 0x%08x\r\n", crcNewFW));
                curSzSoft += szPartSoft;
                D(printf("OK: DOWNLOAD %d BYTES\r\n", (int)curSzSoft));
                HAL_GPIO_TogglePin(LED4G_GPIO_Port, LED4G_Pin);
                cntFailTCPReq = 0;
                flashWrite(partNewFW, szPartSoft, &flashAddrFirmware);
            } else {
                D(printf("ERROR: httpPost() DOWNLOAD\r\n"));
                cntFailTCPReq++;
                if (cntFailTCPReq > 10) {
                    cntFailTCPReq = 0;
                    simReset();
                }
            }
        } else {
            D(printf("DOWNLOAD COMPLETE\r\n"));
            updBootInfo();
            osTimerStart(timerPowerOffHandle, 300000);

            osDelay(100);
            spiFlashSaveInfo();
            osDelay(1000);
            bkte.isTCPOpen = 0;

            if (sendMsgFWUpdated() != SUCCESS) {
                D(printf("ERROR: Send FW UPDATED\r\n"));
            }
            osDelay(1000);
            NVIC_SystemReset();
        }
    }
    /* USER CODE END taskGetNewBin */
}

void updBootInfo() {
    u8 diff = 4 - (szNewFW % 4);
    szNewFW = szNewFW % 4 == 0 ? szNewFW : ((szNewFW / 4) + 1) * 4;
    if (diff > 0 && diff < 4) {
        memset(partNewFW, 0xFF, diff);
        crc32_chank(&crcNewFW, partNewFW, diff);
        // szNewFW += diff;
    }
    crcNewFW ^= 0xffffffff;
    while (HAL_FLASH_Unlock() != HAL_OK)
        D(printf("ERROR: HAL_FLASH_Unlock()\r\n"));

    FLASH_Erase_Sector(FLASH_SECTOR_3, VOLTAGE_RANGE_3);
    D(printf("FLASH_Erase_Sector\r\n"));

    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_ID_BOOT, BKTE_ID_BOOT))
        D(printf("ERROR: HAL_FLASH_Program(BOOT_ADDR_ID_LOADER)\r\n"));
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_IS_NEW_FIRMWARE, (u32)1))
        D(printf("ERROR: HAL_FLASH_Program(BOOT_ADDR_IS_NEW_FIRMWARE)\r\n"));
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_SZ_NEW_FIRMWARE, (u32)(szNewFW)))
        D(printf("ERROR: HAL_FLASH_Program(FLASH_ADDR_SZ_NEW_FIRMWARE)\r\n"));
    while (HAL_FLASH_Program(TYPEPROGRAM_WORD, FLASH_ADDR_CRC_NEW_FIRMWARE, (u32)(crcNewFW)))
        D(printf("ERROR: HAL_FLASH_Program(FLASH_ADDR_CRC_NEW_FIRMWARE)\r\n"));

    while (HAL_FLASH_Lock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Lock()\r\n"));
    D(printf("BOOT_ID: %d\r\n", (int)getFlashData(FLASH_ADDR_ID_BOOT)));
    D(printf("IS_NEW_FIRMARE: %d\r\n", (int)getFlashData(FLASH_ADDR_IS_NEW_FIRMWARE)));
}

void lockAllTasks() {
    osMutexWait(mutexWriteToEnergyBufHandle, osWaitForever);
    osMutexWait(mutexRTCHandle, osWaitForever);
    osMutexWait(mutexSpiFlashHandle, osWaitForever);
    osMutexWait(mutexSDHandle, osWaitForever);
    osMutexWait(mutexWebHandle, osWaitForever);

    vTaskSuspend(webExchangeHandle);
    vTaskSuspend(getEnergyHandle);
    vTaskSuspend(getTempHandle);
    vTaskSuspend(keepAliveHandle);
    // vTaskSuspend(loraHandle);
    vTaskSuspend(createWebPckgHandle);
    vTaskSuspend(wirelessSensHandle);

    osMutexRelease(mutexWriteToEnergyBufHandle);
    osMutexRelease(mutexRTCHandle);
    osMutexRelease(mutexSpiFlashHandle);
    osMutexRelease(mutexSDHandle);
    osMutexRelease(mutexWebHandle);
}

u32 getSzFirmware() {
    u8 bufSzFirmware[4];
    if (generateWebPckgReq(CMD_REQUEST_SZ_FIRMWARE, NULL, 0, SZ_REQUEST_GET_SZ_FIRMWARE, bufSzFirmware, 4) == ERROR) {
        sdWriteLog(SD_ER_SZ_FIRMWARE, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogs);
        D(printf("ERROR: sz firmware\r\n"));
        return 0;
    } else {
        u32 szFirmware = bufSzFirmware[0] << 24 | bufSzFirmware[1] << 16 | bufSzFirmware[2] << 8 | bufSzFirmware[3];
        D(printf("OK: sz firmware %d\r\n", szFirmware));
        return szFirmware;
    }
}

ErrorStatus getPartFirmware(u8* reqData, u8* answBuf, u16 szAnsw, u8 szReq) {
    WebPckg*    curPckg;
    ErrorStatus ret = SUCCESS;
    curPckg = createWebPckgReq(CMD_REQUEST_PART_FIRMWARE, reqData, szReq, SZ_REQUEST_GET_PART_FIRMWARE);
    osMutexWait(mutexWebHandle, osWaitForever);
    if (sendTcp(curPckg->buf, curPckg->shift) != TCP_OK) {
        sdWriteLog(SD_ER_PART_FIRMWARE, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogs);
        D(printf("ERROR: part Firmware\r\n"));
        HAL_GPIO_WritePin(LED4R_GPIO_Port, LED4R_Pin, GPIO_PIN_SET);
        ret = ERROR;
    } else {
        waitIdleCnt("wait IDLE part firmware", &(uInfoSim.irqFlags), szAnsw / SZ_TCP_PCKG + 1, 200, 20000);
        osDelay(100);
        HAL_GPIO_WritePin(LED4R_GPIO_Port, LED4R_Pin, GPIO_PIN_RESET);
        HAL_GPIO_TogglePin(LED4G_GPIO_Port, LED4G_Pin);
        memcpy(answBuf, &uInfoSim.pRxBuf[11], szAnsw);
    }
    osMutexRelease(mutexWebHandle);
    clearWebPckg(curPckg);
    return ret;
}
