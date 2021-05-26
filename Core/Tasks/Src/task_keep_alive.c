#include "../Tasks/Inc/task_keep_alive.h"
#include "../Utils/Inc/utils_pckgs_manager.h"
#include "../Tasks/Inc/task_iwdg.h"
#include "../Utils/Inc/utils_flash.h"

extern u16 iwdgTaskReg;

extern osThreadId keepAliveHandle;
extern osThreadId getTempHandle;
extern osThreadId getEnergyHandle;
// extern osThreadId loraHandle;
extern osThreadId wirelessSensHandle;
extern osTimerId timerPowerOffHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;
extern osMutexId mutexRTCHandle;
extern osMutexId mutexSDHandle;
extern osMutexId mutexSpiFlashHandle;
extern osSemaphoreId semCreateWebPckgHandle;

extern CircularBuffer circBufAllPckgs;
extern u8 isRxNewFirmware;
// extern CircularBuffer circBufPckgEnergy;
// extern u8 SZ_PCKGENERGY;

// static PckgEnergy curPckgEnergy;

u8 bufTxData[256];

void taskKeepAlive(void const* argument) {
    u32 timeout = 1;
    vTaskSuspend(keepAliveHandle);

    for (;;) {
        HAL_GPIO_TogglePin(LED1G_GPIO_Port, LED1G_Pin);
        if (!(timeout % 600) && !isRxNewFirmware) {
            D(printf("\r\ngetNumFirmware\r\n\r\n"));
            osTimerStart(timerPowerOffHandle, 1100000);
            getNumFirmware();
            osTimerStop(timerPowerOffHandle);
        }
        if (!(timeout % 6000) && !isRxNewFirmware) {
            D(printf("\r\ngenerateMsgKeepAlive\r\n\r\n"));
            osTimerStart(timerPowerOffHandle, 1100000);
            generateMsgKeepAlive();
            osTimerStop(timerPowerOffHandle);
        }
        if (!(timeout % 36000) && !isRxNewFirmware) {
            D(printf("\r\nupdRTC\r\n\r\n"));
            osTimerStart(timerPowerOffHandle, 1100000);
            updRTC();
            osTimerStop(timerPowerOffHandle);
        }
        if (bkte.pwrInfo.isPwrState) {
            HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_RESET);
            osTimerStart(timerPowerOffHandle, 1100000);
            pwrOffBkte();
            osTimerStop(timerPowerOffHandle);
        }
        
        bkte.pwrInfo.isPwrState = HAL_GPIO_ReadPin(PWR_STATE_GPIO_Port, PWR_STATE_Pin);
        timeout++;
        osDelay(200);
        iwdgTaskReg |= IWDG_TASK_REG_ALIVE;
    }
}

void timerPowerOff_callback(void const * argument)
{
    D(printf("\r\nTIMER POWER OFF\r\n\r\n"));
    HAL_GPIO_WritePin(BAT_PWR_EN_GPIO_Port, BAT_PWR_EN_Pin, GPIO_PIN_RESET);  // OFF
    osDelay(1000);
    NVIC_SystemReset();
}

u16 getAdcVoltBat() {
    u16 adc;
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 250);
    adc = HAL_ADC_GetValue(&hadc1);
    return (u16)(adc * 3.3 * 2 / 4096 * 100);
}

void pwrOffBkte() {
    char strVolts[4];
    u32 delayPages = BKTE_THRESHOLD_CNT_PAGES + 1;
    u32 curTime = 0;
    u8 cnt;

    osMutexWait(mutexWriteToEnergyBufHandle, osWaitForever);
    osMutexWait(mutexRTCHandle, osWaitForever);
    osMutexWait(mutexSpiFlashHandle, osWaitForever);
    osMutexWait(mutexSDHandle, osWaitForever);
    osMutexWait(mutexWebHandle, osWaitForever);

    vTaskSuspend(getEnergyHandle);
    vTaskSuspend(getTempHandle);
    // vTaskSuspend(loraHandle);
    vTaskSuspend(wirelessSensHandle);

    osMutexRelease(mutexWriteToEnergyBufHandle);
    osMutexRelease(mutexRTCHandle);
    osMutexRelease(mutexSpiFlashHandle);
    osMutexRelease(mutexSDHandle);
    osMutexRelease(mutexWebHandle);

    osDelay(2000);
    D(printf("OK: PWR OFF START\r\n"));
    // cBufReset(&circBufPckgEnergy);

    bkte.pwrInfo.adcVoltBat = getAdcVoltBat();
    generateMsgBat();
    D(printf("OK: PWR OFF WAIT: %d\r\n", getUnixTimeStamp()));

    curTime = 0;
    delayPages = getDelayPages();
    while (delayPages > BKTE_THRESHOLD_CNT_PAGES && (bkte.pwrInfo.adcVoltBat = getAdcVoltBat()) > 390) {
        osDelay(5000);
        curTime += 5000;
        delayPages = getDelayPages();
        printf("wait dalaypages %d\r\n", delayPages);

        if (curTime > 300000) {
            bkte.isTCPOpen = 0;
            if (sendMsgDevOffValue(11) != SUCCESS) {
                D(printf("ERROR: Send dev off val\r\n"));
            }
            break;
        }
    }

    curTime = 0;
    while ((cnt = getCntFreePckg()) != CNT_WEBPCKGS && (bkte.pwrInfo.adcVoltBat = getAdcVoltBat()) > 385) {
        osDelay(5000);
        curTime += 5000;
        printf("wait free pckg %d\r\n", cnt);

        if (curTime > 300000) {
            bkte.isTCPOpen = 0;
            if (sendMsgDevOffValue(12) != SUCCESS) {
                D(printf("ERROR: Send dev off val\r\n"));
            }
            break;
        }
    }

    bkte.pwrInfo.adcVoltBat = getAdcVoltBat();
    generateMsgBat();
    generateMsgDevOff();
    D(printf("OFF  VOLT: %d\r\n", bkte.pwrInfo.adcVoltBat));
    
    updSpiFlash(&circBufAllPckgs);
    osSemaphoreRelease(semCreateWebPckgHandle);
    osDelay(3000);
    
    curTime = 0;
    bkte.isSentData = 0;
    while (!bkte.isSentData && (bkte.pwrInfo.adcVoltBat = getAdcVoltBat()) > 380) {
        osDelay(5000);
        curTime += 5000;
        printf("wait data send\r\n");

        if (curTime > 300000) {
            bkte.isTCPOpen = 0;
            if (sendMsgDevOffValue(13) != SUCCESS) {
                D(printf("ERROR: Send dev off val\r\n"));
            }
            break;
        }
    }
    if (!bkte.isSentData) {
        sdWriteLog(SD_MSG_NOT_SENT, SD_LEN_NOT_SENT, NULL, 0, &sdSectorLogs);
        sdUpdLog(&sdSectorLogs);
    }

    D(printf("OK: PWR OFF SENT TELEMETRY: %d\r\n", getUnixTimeStamp()));

    sprintf(strVolts, "%03d", bkte.pwrInfo.adcVoltBat);
    sdWriteLog(SD_MSG_OFF_BKTE, SD_LEN_OFF_BKTE, strVolts, 3, &sdSectorLogs);
    sdUpdLog(&sdSectorLogs);

    spiFlashSaveInfo();

    osDelay(10000);
    HAL_GPIO_WritePin(BAT_PWR_EN_GPIO_Port, BAT_PWR_EN_Pin, GPIO_PIN_RESET);  // OFF
    osDelay(5000);
    NVIC_SystemReset();
}

void updRTC() {
    getServerTime();
    // fillTelemetry(&curPckgEnergy, TEL_CHANGE_TIME, time);
}

void generateMsgKeepAlive() {
    PckgTelemetry pckgTel;
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_BKTE_ALIVE;
    pckgTel.data = 0;
    saveTelemetry(&pckgTel, &circBufAllPckgs);

    sdWriteLog(SD_MSG_KEEP_ALIVE, SD_LEN_KEEP_ALIVE, NULL, 0, &sdSectorLogs);
}

void generateMsgBat() {
    PckgTelemetry pckgTel;
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_BATTERY;
    pckgTel.data = bkte.pwrInfo.adcVoltBat;
    saveTelemetry(&pckgTel, &circBufAllPckgs);
}

void generateMsgDevOff() {
    PckgTelemetry pckgTel;
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
	pckgTel.code = TEL_CD_HW_BKTE;
	pckgTel.data = 0;
	saveTelemetry(&pckgTel, &circBufAllPckgs);
}

ErrorStatus sendMsgFWUpdated() {
    ErrorStatus ret = SUCCESS;
    PckgTelemetry pckgTel;

    D(printf("sendMsgFWUpdated\r\n"));

    memset(bufTxData, 0, 20);
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
	pckgTel.code = TEL_CD_HW_UPDATED;
	pckgTel.data = bkte.idNewFirmware;
    pckgTel.unixTimeStamp = getUnixTimeStamp();
    copyTelemetry(bufTxData, &pckgTel);
	pckgTel.code = TEL_CD_HW_BKTE;
	pckgTel.data = 0;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY], &pckgTel);

    ret = sendWebPckgData(CMD_DATA_TELEMETRY, bufTxData, SZ_CMD_TELEMETRY * 2, 2);
    
    return ret;
}

ErrorStatus sendMsgDevOff() {
    ErrorStatus ret = SUCCESS;
    PckgTelemetry pckgTel;

    memset(bufTxData, 0, 20);
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
	pckgTel.code = TEL_CD_HW_BKTE;
	pckgTel.data = 0;
    pckgTel.unixTimeStamp = getUnixTimeStamp();
    copyTelemetry(bufTxData, &pckgTel);

    ret = sendWebPckgData(CMD_DATA_TELEMETRY, bufTxData, SZ_CMD_TELEMETRY, 1);
    
    return ret;
}

ErrorStatus sendMsgDevOffValue(u32 val) {
    ErrorStatus ret = SUCCESS;
    PckgTelemetry pckgTel;

    memset(bufTxData, 0, 20);
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
	pckgTel.code = TEL_CD_HW_BKTE;
	pckgTel.data = val;
    pckgTel.unixTimeStamp = getUnixTimeStamp();
    copyTelemetry(bufTxData, &pckgTel);

    ret = sendWebPckgData(CMD_DATA_TELEMETRY, bufTxData, SZ_CMD_TELEMETRY, 1);
    
    return ret;
}

ErrorStatus sendInitTelemetry() {
    ErrorStatus ret = SUCCESS;
    PckgTelemetry pckgTel;
    long long phoneNum;
    u32 tmp;
    u8 ptr = 0;

    memset(bufTxData, 0, 256);

    pckgTel.unixTimeStamp = getUnixTimeStamp();
    pckgTel.group = TEL_GR_GENINF;
    pckgTel.code = TEL_CD_GENINF_NUM_FIRMWARE;
    pckgTel.data = BKTE_ID_FIRMWARE;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = TEL_CD_GENINF_NUM_BOOT;
    pckgTel.data = BKTE_ID_BOOT;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = TEL_CD_GENINF_NUM_PCB;
    pckgTel.data = BKTE_ID_PCB;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    phoneNum = simGetPhoneNum();
    if (phoneNum > 0) {
        tmp = phoneNum % 1000000000;
        pckgTel.code = TEL_CD_GENINF_PHONE_NUM1;
        pckgTel.data = tmp;
        copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);
    }

    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_BKTE;
    pckgTel.data = 1;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = TEL_CD_HW_SD;
    pckgTel.data = bkte.hwStat.isFatMount;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = TEL_CD_HW_DS2482;
    pckgTel.data = bkte.hwStat.isDS2482;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = TEL_CD_HW_SPI_FLASH;
    pckgTel.data = bkte.hwStat.isSPIFlash;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = TEL_CD_HW_LORA;
    pckgTel.data = bkte.hwStat.isLoraOk;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = TEL_CD_HW_UPDATE_ERR;
    tmp = getFlashData(FLASH_ADDR_ERR_NEW_FIRMWARE);
    if (tmp > 2) tmp = 0;
    pckgTel.data = tmp;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = (u8)12;
    pckgTel.data = bkte.lastData.enAct;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    pckgTel.code = (u8)13;
    pckgTel.data = bkte.lastData.enReact;
    copyTelemetry(&bufTxData[SZ_CMD_TELEMETRY * ptr++], &pckgTel);

    ret = sendWebPckgData(CMD_DATA_TELEMETRY, bufTxData, SZ_CMD_TELEMETRY * ptr, ptr);
    
    return ret;
}