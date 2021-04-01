#include "../Tasks/Inc/task_keep_alive.h"

#include "../Utils/Inc/utils_pckgs_manager.h"

extern osThreadId keepAliveHandle;
extern osThreadId getTempHandle;
extern osThreadId getEnergyHandle;
extern osThreadId loraHandle;
extern osThreadId wirelessSensHandle;
extern u8 isRxNewFirmware;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;
extern osSemaphoreId semCreateWebPckgHandle;

extern CircularBuffer circBufAllPckgs;
// extern CircularBuffer circBufPckgEnergy;
// extern u8 SZ_PCKGENERGY;

// static PckgEnergy curPckgEnergy;

void taskKeepAlive(void const* argument) {
    u16 timeout = 1;
    vTaskSuspend(keepAliveHandle);

    for (;;) {
        HAL_GPIO_TogglePin(LED1G_GPIO_Port, LED1G_Pin);
        if (!(timeout % 60) && !isRxNewFirmware) {
            D(printf("\r\ngetNumFirmware\r\n\r\n"));
            getNumFirmware();
        }
        if (!(timeout % 600) && !isRxNewFirmware) {
            D(printf("\r\ngenerateMsgKeepAlive\r\n\r\n"));
            generateMsgKeepAlive();
        }
        if (!(timeout % 3600) && !isRxNewFirmware) {
            D(printf("\r\nupdRTC\r\n\r\n"));
            updRTC();
        }
        if (bkte.pwrInfo.isPwrState) {
            pwrOffBkte();
        }
        
        bkte.pwrInfo.isPwrState = HAL_GPIO_ReadPin(PWR_STATE_GPIO_Port, PWR_STATE_Pin);
        timeout++;
        osDelay(2000);
    }
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
    static u32 delayPages = 1;
    u8 cnt;
    vTaskSuspend(getEnergyHandle);
    vTaskSuspend(getTempHandle);
    // vTaskSuspend(loraHandle);
    vTaskSuspend(wirelessSensHandle);

    osDelay(2000);
    D(printf("OK: PWR OFF START\r\n"));
    // cBufReset(&circBufPckgEnergy);

    bkte.pwrInfo.adcVoltBat = getAdcVoltBat();
    generateMsgDevOff();
    D(printf("OK: PWR OFF WAIT: %d\r\n", getUnixTimeStamp()));

    while (delayPages > BKTE_THRESHOLD_CNT_PAGES &&
           (bkte.pwrInfo.adcVoltBat = getAdcVoltBat()) > 360) {
        osDelay(5000);
        printf("delayPages %d\r\n", delayPages);
        delayPages = spiFlash64.headNumPg >= spiFlash64.tailNumPg ? 
                    spiFlash64.headNumPg - spiFlash64.tailNumPg : 
                    spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
    }

    while ((cnt = getCntFreePckg()) != CNT_WEBPCKGS &&
                 (bkte.pwrInfo.adcVoltBat = getAdcVoltBat()) > 300) {
        osDelay(5000);
        printf("getCntFreePckg %d\r\n", cnt);
    }

    bkte.pwrInfo.adcVoltBat = getAdcVoltBat();
    generateMsgDevOff();
    D(printf("OFF  VOLT: %d\r\n", bkte.pwrInfo.adcVoltBat));
    bkte.isSentData = 0;
    updSpiFlash(&circBufAllPckgs);
    xSemaphoreGive(semCreateWebPckgHandle);
    while ((bkte.pwrInfo.adcVoltBat = getAdcVoltBat()) > 360 &&
           !bkte.isSentData) {
        osDelay(1000);
        printf("wait isSentData\r\n");
    }
    if (!bkte.isSentData) {
        sdWriteLog(SD_MSG_NOT_SENT, SD_LEN_NOT_SENT, NULL, 0, &sdSectorLogs);
        sdUpdLog(&sdSectorLogs);
    }

    D(printf("OK: PWR OFF SENT TELEMETRY: %d\r\n", getUnixTimeStamp()));

    /*cBufReset(&circBufPckgEnergy);
    memcpy(circBufPckgEnergy.buf, &spiFlash64.headNumPg, 4);
    spiFlashWrPg(circBufPckgEnergy.buf, 4, 0, BKTE_SAVE_NUM_PAGE);*/

    sprintf(strVolts, "%03d", bkte.pwrInfo.adcVoltBat);
    sdWriteLog(SD_MSG_OFF_BKTE, SD_LEN_OFF_BKTE, strVolts, 3, &sdSectorLogs);
    sdUpdLog(&sdSectorLogs);

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

void generateMsgDevOff() {
    PckgTelemetry pckgTel;
    pckgTel.group = TEL_GR_HARDWARE_STATUS;
    pckgTel.code = TEL_CD_HW_BATTERY;
    pckgTel.data = bkte.pwrInfo.adcVoltBat;
    saveTelemetry(&pckgTel, &circBufAllPckgs);
}