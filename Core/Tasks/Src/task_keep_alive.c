#include "../Tasks/Inc/task_keep_alive.h"


extern osThreadId keepAliveHandle;
extern osThreadId getTempHandle;
extern osThreadId getEnergyHandle;
extern osThreadId loraHandle;
extern u8 isRxNewFirmware;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;

extern CircularBuffer circBufAllPckgs;
// extern CircularBuffer circBufPckgEnergy;
// extern u8 SZ_PCKGENERGY;

static PckgEnergy curPckgEnergy;

void taskKeepAlive(void const * argument){
    u16 timeout = 1;
    vTaskSuspend(keepAliveHandle);
    

    for(;;)
    {
        HAL_GPIO_TogglePin(LED1G_GPIO_Port, LED1G_Pin);
        if(!(timeout % 60) && !isRxNewFirmware){
            getNumFirmware();
        }
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
    D(printf("OK: PWR OFF START\r\n"));
    // cBufReset(&circBufPckgEnergy);

    bkte.pwrInfo.adcVoltBat = getAdcVoltBat();
    generateMsgDevOff();
    D(printf("OK: PWR OFF WAIT\r\n"));

    osDelay(100000);

    bkte.pwrInfo.adcVoltBat = getAdcVoltBat();
    generateMsgDevOff();

    bkte.isSentData = 0;
    updSpiFlash(&circBufAllPckgs);
    while((bkte.pwrInfo.adcVoltBat = getAdcVoltBat()) > 300 && !bkte.isSentData) osDelay(1000);
    if(!bkte.isSentData){
        sdWriteLog(SD_MSG_NOT_SENT, SD_LEN_NOT_SENT, NULL, 0, &sdSectorLogs);
        sdUpdLog(&sdSectorLogs);
    }

    D(printf("OK: PWR OFF SENT TELEMETRY\r\n"));

    /*cBufReset(&circBufPckgEnergy);
    memcpy(circBufPckgEnergy.buf, &spiFlash64.headNumPg, 4);
    spiFlashWrPg(circBufPckgEnergy.buf, 4, 0, BKTE_SAVE_NUM_PAGE);*/

    sprintf(strVolts, "%03d", bkte.pwrInfo.adcVoltBat);
    sdWriteLog(SD_MSG_OFF_BKTE, SD_LEN_OFF_BKTE, strVolts, 3, &sdSectorLogs);
    sdUpdLog(&sdSectorLogs);

    osDelay(10000);
    HAL_GPIO_WritePin(BAT_PWR_EN_GPIO_Port, BAT_PWR_EN_Pin, GPIO_PIN_RESET);  //OFF
    osDelay(5000);
    NVIC_SystemReset();
}

void updRTC(){

    getServerTime();
    // fillTelemetry(&curPckgEnergy, TEL_CHANGE_TIME, time);
}

void generateMsgKeepAlive(){
    PckgTelemetry pckgTel;
	pckgTel.group = TEL_GR_HARDWARE_STATUS;
	pckgTel.code = TEL_CD_HW_BKTE_ALIVE;
	pckgTel.data = 0;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

    sdWriteLog(SD_MSG_KEEP_ALIVE, SD_LEN_KEEP_ALIVE, NULL, 0, &sdSectorLogs);
}

void generateMsgDevOff(){
    PckgTelemetry pckgTel;
	pckgTel.group = TEL_GR_HARDWARE_STATUS;
	pckgTel.code = TEL_CD_HW_BATTERY;
	pckgTel.data = bkte.pwrInfo.adcVoltBat;
    saveTelemetry(&pckgTel, &circBufAllPckgs);
}