#include "../Tasks/Inc/task_get_energy.h"
extern osThreadId getEnergyHandle;
extern osThreadId webExchangeHandle;
extern osThreadId getTempHandle;
extern osThreadId keepAliveHandle;
extern osThreadId loraHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId getNewBinHandle;
extern osThreadId wirelessSensHandle;
extern osSemaphoreId semCreateWebPckgHandle;

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
	sdInit();
	simInit();
	getServerTime();

	generateInitTelemetry();
	unLockTasks();
	rxUart1_IT();

    for(;;){
        memset(testBufUart1, '\0', sizeof(testBufUart1));
        retLen = cBufRead(&rxUart1CircBuf, (u8*)testBufUart1, 0);
        if(retLen == BKTE_SZ_UART_MSG){
            numIteration = (numIteration + 1) % BKTE_ENERGY_FULL_LOOP;
            fillPckgVoltAmper(&pckgVoltAmper, testBufUart1);
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
				// D(printf("OK: enAct: %08x, enReact: %08x", pckgEnergy.enAct, pckgEnergy.enReact));
			}			

        }
		osDelay(400);
    }
}

void unLockTasks(){
	// vTaskResume(getNewBinHandle); //! debug download firmware
	vTaskResume(webExchangeHandle);
	vTaskResume(getTempHandle);
	vTaskResume(keepAliveHandle);
	vTaskResume(loraHandle);
	vTaskResume(createWebPckgHandle);
	vTaskResume(wirelessSensHandle);
}

void generateInitTelemetry(){
	PckgTelemetry pckgTel;
	long long phoneNum;
	u32 tmp;
	pckgTel.group = TEL_GR_GENINF;
	pckgTel.code = TEL_CD_GENINF_NUM_FIRMWARE;
	pckgTel.data = BKTE_ID_FIRMWARE;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

	pckgTel.code = TEL_CD_GENINF_NUM_BOOT;
	pckgTel.data = BKTE_ID_BOOT;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

	pckgTel.code = TEL_CD_GENINF_NUM_PCB;
	pckgTel.data = BKTE_ID_PCB;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

	phoneNum = simGetPhoneNum();
	if(phoneNum > 0){
		tmp = phoneNum % 1000000000;
		pckgTel.code = TEL_CD_GENINF_PHONE_NUM1;
		pckgTel.data = tmp;
		saveTelemetry(&pckgTel, &circBufAllPckgs);
	}

	pckgTel.group = TEL_GR_HARDWARE_STATUS;
	pckgTel.code = TEL_CD_HW_BKTE;
	pckgTel.data = 1;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

	pckgTel.code = TEL_CD_HW_SD;
	pckgTel.data = bkte.hwStat.isFatMount;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

	pckgTel.code = TEL_CD_HW_DS2482;
	pckgTel.data = bkte.hwStat.isDS2482;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

	pckgTel.code = TEL_CD_HW_SPI_FLASH;
	pckgTel.data = bkte.hwStat.isSPIFlash;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

	pckgTel.code = TEL_CD_HW_LORA;
	pckgTel.data = bkte.hwStat.isLoraOk;
	saveTelemetry(&pckgTel, &circBufAllPckgs);

	updSpiFlash(&circBufAllPckgs);
	xSemaphoreGive(semCreateWebPckgHandle);
	
}

