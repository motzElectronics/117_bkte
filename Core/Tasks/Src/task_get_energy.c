#include "../Tasks/Inc/task_get_energy.h"
extern osThreadId getEnergyHandle;
extern osThreadId webExchangeHandle;
extern osThreadId getTempHandle;
extern osThreadId keepAliveHandle;
extern osThreadId loraHandle;

extern osMutexId mutexWriteToEnergyBufHandle;

extern CircularBuffer rxUart1CircBuf;

extern CircularBuffer circBufAllPckgs;

static u16 testBufUart1[SZ_RX_UART1];
static EnergyData lastData = {.current = 0, .enAct = 0, .enReact = 0, .volt = 0};

static PckgVoltAmper pckgVoltAmper;
static PckgEnergy pckgEnergy;
// u8 test = 0;

void taskGetEnergy(void const * argument){
  	//vTaskSuspend(getEnergyHandle);
	u8 numIteration = 0;
	u8 energyTime = 0;
	u16 retLen;
	
	spiFlashInit(circBufAllPckgs.buf);
	cBufReset(&circBufAllPckgs);

	/*if(sdInit() != FAT_OK){
		fillTelemetry(&circBufAllPckgs, TEL_NO_FATFS, 0);
		cBufWriteToBuf(&circBufAllPckgs, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
	}
	sdWriteLog(SD_MSG_START_BKTE, SD_LEN_START_BKTE, NULL, 0, &sdSectorLogs);
	sdUpdLog(&sdSectorLogs);*/
	simInit();
	if(!getServerTime()){
		/*sdWriteLog(SD_ER_BAD_SERVERTIME, SD_LEN_MYFUN, NULL, 0, &sdSectorLogs);
		sdUpdLog(&sdSectorLogs);*/
		D(printf("ERROR: BAD TIME\r\n"));
	}

	
	/*fillTelemetry(&curPckgEnergy, TEL_ON_DEV, 0);
	cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);

	fillTelemetry(&curPckgEnergy, TEL_ID_FIRMWARE, bkte.idFirmware);
	cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);*/

	unLockTasks();
	rxUart1_IT();

    for(;;){
        memset(testBufUart1, '\0', sizeof(testBufUart1));
        retLen = cBufRead(&rxUart1CircBuf, (u8*)testBufUart1, 0);
        if(retLen == BKTE_SZ_UART_MSG){
            numIteration = (numIteration + 1) % BKTE_ENERGY_FULL_LOOP;
            fillPckgVoltAmper(&pckgVoltAmper, testBufUart1);
			D(printf("OK: volt: %04x, amper: %04x\r\n", pckgVoltAmper.volt, pckgVoltAmper.amper));
            // if(getDeviation(&curPckgEnergy.energyData, &lastData) || !numIteration){
				
			saveData((u8*)&pckgVoltAmper, SZ_CMD_VOLTAMPER, CMD_DATA_VOLTAMPER, &circBufAllPckgs);

				/*xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
                cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
                xSemaphoreGive(mutexWriteToEnergyBufHandle);*/
            // }
			energyTime = (energyTime + 1) % 3; //! change time after debugging!
			if(!energyTime){
				fillPckgEnergy(&pckgEnergy, testBufUart1);
				saveData((u8*)&pckgEnergy, SZ_CMD_ENERGY, CMD_DATA_ENERGY, &circBufAllPckgs);
				D(printf("OK: enAct: %08x, enReact: %08x", pckgEnergy.enAct, pckgEnergy.enReact));
			}			

        }
		osDelay(400);
    }
}

void unLockTasks(){
	vTaskResume(webExchangeHandle);
	vTaskResume(getTempHandle);
	vTaskResume(keepAliveHandle);
	vTaskResume(loraHandle);
}

