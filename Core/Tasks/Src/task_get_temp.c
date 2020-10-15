#include "../Tasks/Inc/task_get_temp.h"

extern osThreadId getTempHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
extern CircularBuffer circBufPckgEnergy;

extern u8 SZ_PCKGENERGY;

void taskGetTemp(void const * argument){

	u8 tempBytes[2];
	s8 temps[BKTE_MAX_CNT_1WIRE];
	PckgEnergy curPckgTemp = {.preambule=BKTE_PREAMBLE_EN};
	vTaskSuspend(getTempHandle);
    HAL_GPIO_WritePin(LED3G_GPIO_Port, LED3G_Pin, GPIO_PIN_RESET);
    s8 tmpTemp;

    for(;;){
        for(u8 num1Wire = 0; num1Wire < BKTE_MAX_CNT_1WIRE; num1Wire++){
            setTempLine(num1Wire);
            osDelay(500);
            ds2482OneWireReset();
            osDelay(200);
            ds2482WriteByte(0xCC); // Skip ROM command
            osDelay(200);
                // Test reading the temperature from the DS18S20
            //	  ds2482WriteConfig(false, true, true);  // Enable strong pullup for temp conversion
            ds2482WriteByte(0x44);  // Get the temp
            osDelay(750); // Wait for the temp conversion
            ds2482OneWireReset();
            osDelay(200);
            ds2482WriteByte(0xCC); // Skip ROM command
            osDelay(200);
            ds2482WriteByte(0xbe); 	// Read scratchpad command
            osDelay(200);
            memset(tempBytes, '\0', 2);

            ds2482ReadByte(&tempBytes[0]); 	// LSB temp byte
            ds2482ReadByte(&tempBytes[1]); 	// MSB temp byte

            if((tmpTemp = ds2482ConvTemp(tempBytes[0], tempBytes[1])) > BKTE_MAX_TEMP ||
            tmpTemp < BKTE_MIN_TEMP){
                tmpTemp = BKTE_NO_TEMP;
            }
            temps[num1Wire] = tmpTemp;
            D(printf("TEMP %d:%d\r\n", num1Wire, tmpTemp));
            HAL_GPIO_TogglePin(LED3G_GPIO_Port, LED3G_Pin);
            resetTempLine(num1Wire);
        }

        fillTempPckgEnergy(&curPckgTemp, temps);
        xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
        cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgTemp, SZ_PCKGENERGY);
        xSemaphoreGive(mutexWriteToEnergyBufHandle);
        checkBufForWritingToFlash();

        osDelay(1000);
    }
}