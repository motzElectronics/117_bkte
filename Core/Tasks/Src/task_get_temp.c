#include "../Tasks/Inc/task_get_temp.h"

extern osThreadId getTempHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
extern CircularBuffer circBufAllPckgs;
static PckgTemp pckgTemp;
void taskGetTemp(void const * argument){

	u8 tempBytes[2];
	s8 temps[BKTE_MAX_CNT_1WIRE];
	
	// vTaskSuspend(getTempHandle);
    ds2482Init();
    vTaskSuspend(getTempHandle);
    HAL_GPIO_WritePin(LED3G_GPIO_Port, LED3G_Pin, GPIO_PIN_RESET);
    s8 tmpTemp;

    for(;;){
        for(u8 num1Wire = 0; num1Wire < BKTE_MAX_CNT_1WIRE; num1Wire++){
            setTempLine(num1Wire);
            osDelay(500);
            if(ds2482OneWireReset() != DS2482_OK){
                saveErrorToTel();
                continue;
            }
            osDelay(200);
            if(ds2482WriteByte(0xCC) != DS2482_OK){ // Skip ROM command
                saveErrorToTel();
                continue;
            }
            osDelay(200);
                // Test reading the temperature from the DS18S20
            //	  ds2482WriteConfig(false, true, true);  // Enable strong pullup for temp conversion
            if(ds2482WriteByte(0x44) != DS2482_OK){  // Get the temp
                saveErrorToTel();
                continue;
            }
            osDelay(750); // Wait for the temp conversion
            if(ds2482OneWireReset() != DS2482_OK){
                saveErrorToTel();
                continue;
            }
            osDelay(200);
            if(ds2482WriteByte(0xCC) != DS2482_OK){ // Skip ROM command
                saveErrorToTel();
                continue;
            }
            osDelay(200);
            if(ds2482WriteByte(0xbe) != DS2482_OK){ 	// Read scratchpad command
                saveErrorToTel();
                continue;
            }
            osDelay(200);
            memset(tempBytes, '\0', 2);

            if(ds2482ReadByte(&tempBytes[0]) != DS2482_OK){ 	// LSB temp byte
                saveErrorToTel();
                continue;
            }
            if(ds2482ReadByte(&tempBytes[1]) != DS2482_OK){ 	// MSB temp byte
                saveErrorToTel();
                continue;
            }

            if((tmpTemp = ds2482ConvTemp(tempBytes[0], tempBytes[1])) > BKTE_MAX_TEMP ||
            tmpTemp < BKTE_MIN_TEMP){
                tmpTemp = BKTE_NO_TEMP;
            }
            temps[num1Wire] = tmpTemp;
            D(printf("TEMP %d:%d\r\n", num1Wire, tmpTemp));
            // HAL_GPIO_TogglePin(LED3G_GPIO_Port, LED3G_Pin);
            resetTempLine(num1Wire);
        }

        fillPckgTemp(&pckgTemp, temps);
        saveData((u8*)&pckgTemp, SZ_CMD_TEMP, CMD_DATA_TEMP, &circBufAllPckgs);
        // cBufSafeWrite(&circBufPckgEnergy, (u8*)&curPckgTemp, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
        /*xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
        cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgTemp, SZ_PCKGENERGY);
        xSemaphoreGive(mutexWriteToEnergyBufHandle);*/
        // checkBufForWritingToFlash();

        osDelay(1000);
    }
}

void saveErrorToTel(){
    /*fillTelemetry(&curPckgTemp, TEL_NO_DS2482, 0);
    cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgTemp, SZ_PCKGENERGY);*/
    osDelay(2000);
}