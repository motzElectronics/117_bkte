#include "../Tasks/Inc/task_get_temp.h"

extern osThreadId getTempHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
extern CircularBuffer circBufAllPckgs;
static PckgTemp pckgTemp;
static s8 temps[BKTE_MAX_CNT_1WIRE];
u32 testTimestamp, tmpTime;

u8 readTemp();

void taskGetTemp(void const* argument) {
    // vTaskSuspend(getTempHandle);
    ds2482Init();
    vTaskSuspend(getTempHandle);
    HAL_GPIO_WritePin(LED3G_GPIO_Port, LED3G_Pin, GPIO_PIN_RESET);

    for (;;) {
        testTimestamp = getUnixTimeStamp();
        if (readTemp() == 0) {
            osDelay(1000);
            continue;
        }
        tmpTime = getUnixTimeStamp() - testTimestamp;
        // D(printf("OK: LOOP TEMP |%d %d %d %d| %d\r\n", temps[0], temps[1], temps[2], temps[3], tmpTime));
        if (tmpTime > 6) {
            D(printf("ERROR: too long time\r\n"));
        }
        osDelay(100);
        fillPckgTemp(&pckgTemp, temps);
        saveData((u8*)&pckgTemp, SZ_CMD_TEMP, CMD_DATA_TEMP, &circBufAllPckgs);
    }
}

u8 readTemp() {
    u8 tempBytes[2];
    s8 tmpTemp;

    for (u8 num1Wire = 3; num1Wire < BKTE_MAX_CNT_1WIRE; num1Wire++) {
        setTempLine(num1Wire);
        // osDelay(500);
        if (ds2482OneWireReset() != DS2482_OK) {
            D(printf("ERROR: ds2482OneWireReset()\r\n"));
            saveErrorToTel();
            return 0;
        }
        osDelay(10);
        if (ds2482WriteByte(0xCC) != DS2482_OK) {  // Skip ROM command
            D(printf("ERROR: ds2482WriteByte(0xCC)\r\n"));
            saveErrorToTel();
            return 0;
        }
        osDelay(10);
        // Test reading the temperature from the DS18S20
        //	  ds2482WriteConfig(false, true, true);  // Enable strong pullup
        //for temp conversion
        if (ds2482WriteByte(0x44) != DS2482_OK) {  // Get the temp
            D(printf("ERROR: ds2482WriteByte(0x44)\r\n"));
            saveErrorToTel();
            return 0;
        }
        osDelay(100);  // Wait for the temp conversion
        if (ds2482OneWireReset() != DS2482_OK) {
            D(printf("ERROR: ds2482OneWireReset()\r\n"));
            saveErrorToTel();
            return 0;
        }
        osDelay(10);
        if (ds2482WriteByte(0xCC) != DS2482_OK) {  // Skip ROM command
            D(printf("ERROR: ds2482WriteByte(0xCC)\r\n"));
            saveErrorToTel();
            return 0;
        }
        osDelay(10);
        if (ds2482WriteByte(0xbe) != DS2482_OK) {  // Read scratchpad command
            D(printf("ERROR: ds2482WriteByte(0xbe)\r\n"));
            saveErrorToTel();
            return 0;
        }
        // osDelay(200);
        memset(tempBytes, '\0', 2);

        if (ds2482ReadByte(&tempBytes[0]) != DS2482_OK) {  // LSB temp byte
            D(printf("ERROR: ds2482ReadByte(&tempBytes[0])\r\n"));
            saveErrorToTel();
            return 0;
        }
        if (ds2482ReadByte(&tempBytes[1]) != DS2482_OK) {  // MSB temp byte
            D(printf("ERROR: ds2482ReadByte(&tempBytes[1])\r\n"));
            saveErrorToTel();
            return 0;
        }

        if ((tmpTemp = ds2482ConvTemp(tempBytes[0], tempBytes[1])) >
                BKTE_MAX_TEMP ||
            tmpTemp < BKTE_MIN_TEMP) {
            tmpTemp = BKTE_NO_TEMP;
        }
        temps[num1Wire] = tmpTemp;
        // D(printf("TEMP %d:%d\r\n", num1Wire, tmpTemp));
        // HAL_GPIO_TogglePin(LED3G_GPIO_Port, LED3G_Pin);
        resetTempLine(num1Wire);

        return 1;
    }
}

void saveErrorToTel() {
    /*fillTelemetry(&curPckgTemp, TEL_NO_DS2482, 0);
    cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgTemp, SZ_PCKGENERGY);
    osDelay(2000);*/
}