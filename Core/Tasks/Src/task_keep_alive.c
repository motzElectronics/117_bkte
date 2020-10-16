#include "../Tasks/Inc/task_keep_alive.h"

extern osThreadId keepAliveHandle;
extern osThreadId getTempHandle;
extern osThreadId getEnergyHandle;
extern osThreadId loraHandle;
extern u8 isRxNewFirmware;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;
extern CircularBuffer circBufPckgEnergy;
extern u8 SZ_PCKGENERGY;

extern HttpUrl urls;

static PckgEnergy curPckgEnergy = {.preambule=BKTE_PREAMBLE_EN};

void taskKeepAlive(void const * argument){
    u16 timeout = 1;
    vTaskSuspend(keepAliveHandle);

    for(;;)
    {
        if(!(timeout % 600) && !isRxNewFirmware){
            generateMsgKeepAlive();
        }
        if(!(timeout % 5400) && !isRxNewFirmware){
            updRTC();
        }
        if(bkte.pwrInfo.isPwrState){
            pwrOffBkte();
        }
        timeout++;
        osDelay(2000);
    }
}


u16 getAdcVoltBat(){
    u16 adc;
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 250);
    adc = HAL_ADC_GetValue(&hadc1);
    return (u16)(adc * 3.3 * 2 / 4096 * 100);
}

void pwrOffBkte(){
    char strVolts[4];
    vTaskSuspend(getEnergyHandle);
    vTaskSuspend(getTempHandle);
    vTaskSuspend(loraHandle);

    osDelay(2000);
    cBufReset(&circBufPckgEnergy);

    bkte.pwrInfo.adcVoltBat = getAdcVoltBat();
    generateMsgDevOff();

    osDelay(100000);

    bkte.pwrInfo.adcVoltBat = getAdcVoltBat();
    generateMsgDevOff();

    updSpiFlash();

    cBufReset(&circBufPckgEnergy);
    memcpy(circBufPckgEnergy.buf, &spiFlash64.headNumPg, 4);
    spiFlashWrPg(circBufPckgEnergy.buf, 4, 0, BKTE_SAVE_NUM_PAGE);

    sprintf(strVolts, "%03d", bkte.pwrInfo.adcVoltBat);
    sdWriteLog(SD_MSG_OFF_BKTE, SD_LEN_OFF_BKTE, strVolts, 3, &sdSectorLogs);
    sdUpdLog(&sdSectorLogs);

    osDelay(10000);
    HAL_GPIO_WritePin(BAT_PWR_EN_GPIO_Port, BAT_PWR_EN_Pin, GPIO_PIN_RESET);  //OFF
    osDelay(5000);
    NVIC_SystemReset();
}

void updRTC(){
    u8 csq = 0;
    time_t prevRtcTime;
    u32 time = 0;
    xSemaphoreTake(mutexWebHandle, portMAX_DELAY);
    while((csq = simCheckCSQ()) < 10 && csq > 99){
        osDelay(2000);
    }
    prevRtcTime = getTimeStamp();
    time = getServerTime();

    if(time - prevRtcTime > BKTE_BIG_DIF_RTC_SERVTIME){
        fillTelemetry(&curPckgEnergy, TEL_BIG_DIFFER_RTC_SERVERTIME, time - prevRtcTime);
        cBufSafeWrite(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
    }

    fillTelemetry(&curPckgEnergy, TEL_CHANGE_TIME, time);
    cBufSafeWrite(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);

    simHttpInit(urls.addMeasure);
    xSemaphoreGive(mutexWebHandle);
}

void generateMsgKeepAlive(){
    fillTelemetry(&curPckgEnergy, TEL_KEEP_ALIVE, 0);
    cBufSafeWrite(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
    sdWriteLog(SD_MSG_KEEP_ALIVE, SD_LEN_KEEP_ALIVE, NULL, 0, &sdSectorLogs);
}

void generateMsgDevOff(){
    fillTelemetry(&curPckgEnergy, TEL_OFF_DEV, bkte.pwrInfo.adcVoltBat);
    cBufSafeWrite(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
}