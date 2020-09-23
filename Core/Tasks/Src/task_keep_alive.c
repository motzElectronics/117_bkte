#include "../Tasks/Inc/task_keep_alive.h"

extern osThreadId keepAliveHandle;
extern osThreadId getTempHandle;
extern osThreadId getEnergyHandle;
extern u8 isRxNewFirmware;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;
extern CircularBuffer circBufPckgEnergy;
extern u8 SZ_PCKGENERGY;

extern HttpUrl urls;

void taskKeepAlive(void const * argument){
    vTaskSuspend(keepAliveHandle);
    u16 timeout = 1;
    u16 adc;
    u8 csq = 0;
    u32 time = 0;
    time_t prevRtcTime;
    PckgEnergy curPckgEnergy = {.preambule=BKTE_PREAMBLE_EN};
    vTaskSuspend(keepAliveHandle);
    //	HAL_GPIO_WritePin(LED_ALIVE_GPIO_Port, LED_ALIVE_Pin, GPIO_PIN_SET);

    for(;;)
    {
        if(!(timeout % 300) && !isRxNewFirmware){
            fillTelemetry(&curPckgEnergy, TEL_KEEP_ALIVE, 0);
            xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
            cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
            xSemaphoreGive(mutexWriteToEnergyBufHandle);
        }
        if(!(timeout % 5400) && !isRxNewFirmware){
            xSemaphoreTake(mutexWebHandle, portMAX_DELAY);
            while(!simIsGoodCSQ(&csq)){
                osDelay(2000);
            }
            prevRtcTime = getTimeStamp();
            time = getServerTime();

            if(time - prevRtcTime > BKTE_BIG_DIF_RTC_SERVTIME){
                fillTelemetry(&curPckgEnergy, TEL_BIG_DIFFER_RTC_SERVERTIME, time - prevRtcTime);
                xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
                cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
                xSemaphoreGive(mutexWriteToEnergyBufHandle);
            }

            fillTelemetry(&curPckgEnergy, TEL_CHANGE_TIME, time);
            xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
            cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
            xSemaphoreGive(mutexWriteToEnergyBufHandle);

            simHttpInit(urls.addMeasure);
            xSemaphoreGive(mutexWebHandle);
        }
        if(bkte.pwrInfo.isPwrState){
            vTaskSuspend(getEnergyHandle);
            vTaskSuspend(getTempHandle);

            osDelay(2000);
            cBufReset(&circBufPckgEnergy);

            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 250);
            adc = HAL_ADC_GetValue(&hadc1);
            bkte.pwrInfo.adcVoltBat = (u16)(adc * 3.3 * 2 / 4096 * 100);
            fillTelemetry(&curPckgEnergy, TEL_OFF_DEV, bkte.pwrInfo.adcVoltBat);
            xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
            cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
            xSemaphoreGive(mutexWriteToEnergyBufHandle);

            osDelay(100000);

            HAL_ADC_Start(&hadc1);
            HAL_ADC_PollForConversion(&hadc1, 250);
            adc = HAL_ADC_GetValue(&hadc1);
            bkte.pwrInfo.adcVoltBat = (u16)(adc * 3.3 * 2 / 4096 * 100);
            fillTelemetry(&curPckgEnergy, TEL_OFF_DEV, bkte.pwrInfo.adcVoltBat);
            xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
            cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
            xSemaphoreGive(mutexWriteToEnergyBufHandle);

            circBufPckgEnergy.numPckgInBuf = 10; // need for checkBufForWritingToFlash()

            checkBufForWritingToFlash();

            cBufReset(&circBufPckgEnergy);
            memcpy(circBufPckgEnergy.buf, &spiFlash64.headNumPg, 4);
            spiFlashWrPg(circBufPckgEnergy.buf, 4, 0, BKTE_SAVE_NUM_PAGE);

            osDelay(100000);
            HAL_GPIO_WritePin(BAT_PWR_EN_GPIO_Port, BAT_PWR_EN_Pin, GPIO_PIN_RESET);  //OFF
            NVIC_SystemReset();

        }
        timeout++;
    //		HAL_GPIO_TogglePin(LED2R_GPIO_Port, LED2R_Pin);
    //		HAL_GPIO_TogglePin(LED_ALIVE_GPIO_Port, LED_ALIVE_Pin);
        osDelay(2000);
    }
}