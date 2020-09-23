#include "../Tasks/Inc/task_get_energy.h"
extern osThreadId getEnergyHandle;
extern osThreadId webExchangeHandle;
extern osThreadId getTempHandle;

extern osMutexId mutexWriteToEnergyBufHandle;

extern CircularBuffer circBufPckgEnergy;
extern CircularBuffer rxUart1CircBuf;

static u16 testBufUart1[SZ_RX_UART1];
static EnergyData lastData = {.current = 0, .enAct = 0, .enReact = 0, .volt = 0};

void taskGetEnergy(void const * argument){
  	vTaskSuspend(getEnergyHandle);
	PckgEnergy curPckgEnergy = {.preambule=BKTE_PREAMBLE_EN};
	u8 SZ_PCKGENERGY = sizeof(curPckgEnergy);

	u8 numIteration = 0;
	u16 retLen;
	u32 time = 0;
	spiFlashInit(circBufPckgEnergy.buf);
	cBufReset(&circBufPckgEnergy);
	bkteInit(&time);

	fillTelemetry(&curPckgEnergy, TEL_ON_DEV, 0);
	cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);

	fillTelemetry(&curPckgEnergy, TEL_CHANGE_TIME, time);
	cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);

	fillTelemetry(&curPckgEnergy, TEL_ID_FIRMWARE, bkte.idFirmware);
	cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);

	vTaskResume(webExchangeHandle);
	vTaskResume(getTempHandle);

	rxUart1_IT();

    for(;;){
        memset(testBufUart1, '\0', sizeof(testBufUart1));
        retLen = cBufRead(&rxUart1CircBuf, (u8*)testBufUart1, CIRC_TYPE_ENERGY_UART, 0);
        if(retLen == BKTE_SZ_UART_MSG){
            numIteration = (numIteration + 1) % BKTE_ENERGY_FULL_LOOP;
            fillPckgEnergy(&curPckgEnergy, testBufUart1);
            // if(getDeviation(&curPckgEnergy.energyData, &lastData) || !numIteration){
                xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
                cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
                xSemaphoreGive(mutexWriteToEnergyBufHandle);
            // }			

        }
        checkBufForWritingToFlash();
    }
}