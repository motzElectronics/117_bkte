/*
 * utils_bkte.c
 *
 *  Created on: Mar 16, 2020
 *      Author: bls
 */

#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/utils_sd.h"
GPIO_TypeDef* oneWirePorts[BKTE_MAX_CNT_1WIRE] = {ONEWIRE_1_EN_GPIO_Port, ONEWIRE_2_EN_GPIO_Port, ONEWIRE_3_EN_GPIO_Port, ONEWIRE_4_EN_GPIO_Port};
u16 oneWirePins[BKTE_MAX_CNT_1WIRE] = {ONEWIRE_1_EN_Pin, ONEWIRE_2_EN_Pin, ONEWIRE_3_EN_Pin, ONEWIRE_4_EN_Pin};
//extern osMessageQId queue1WireHandle;
extern UART_HandleTypeDef huart3;
extern RTC_HandleTypeDef hrtc;
extern osMutexId mutexRTCHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
//extern osMutexId mutexRTCHandle;
static RTC_TimeTypeDef tmpTime;
static RTC_DateTypeDef tmpDate;
extern u8 SZ_PCKGENERGY;
//extern FIL fAddr;
/*static u8 lenLog[] = {SD_LEN_LOG_ENERGY, SD_LEN_LOG_TEMP, SD_LEN_LOG_RSSI};
static char* fNamesLog[NUM_READ_FILES] = {FILE_LOG_ENERGY, FILE_LOG_TEMP, FILE_LOG_RSSI};
static char* fNamesAddr[NUM_READ_FILES] = {FILE_ADDR_ENERGY, FILE_ADDR_TEMP, FILE_ADDR_RSSI};*/
void bkteInit(){

	HAL_GPIO_WritePin(SD_PWR_EN_GPIO_Port, SD_PWR_EN_Pin, GPIO_PIN_RESET);
	
	HAL_GPIO_WritePin(BAT_PWR_EN_GPIO_Port, BAT_PWR_EN_Pin, GPIO_PIN_SET);
	bkte.pwrInfo.isPwrState = HAL_GPIO_ReadPin(PWR_STATE_GPIO_Port, PWR_STATE_Pin);

	for(u8 i = 0; i < 3; i++)
		bkte.idMCU[i] = getFlashData(BKTE_ADDR_ID_MCU + (i * 4));
	D(printf("%08x%08x%08x\r\n",
               (uint)bkte.idMCU[0], (uint)bkte.idMCU[1], (uint)bkte.idMCU[2]));
	bkte.idTrain = BKTE_ID_TRAIN;
	bkte.idTrainCar = BKTE_ID_TRAINCAR;
	bkte.idReceiver = 1;
	bkte.idDev = BKTE_ID_DEV_BKTE;
	bkte.idFirmware = BKTE_ID_FIRMWARE;
	bkte.idBoot = BKTE_ID_BOOT;
	bkte.isFatMount = 0;

	bkte.erFlags.errReg = 0;
}



u32 getServerTime(){

	u8 tmpSimBadResponse = 0;
	char timestamp[LEN_TIMESTAMP + 1];
	memset(timestamp, '\0', sizeof(timestamp));
	D(printf("getServerTime()\r\n"));
	while(simGetDateTime(timestamp) != SIM_SUCCESS){
		memset(timestamp, '\0', sizeof(timestamp));
		sdWriteLog(SD_ER_MSG_HTTPINIT_MYFUN, SD_LEN_MYFUN, NULL, 0, &sdSectorLogError);
		D(printf("ERROR: bad time\r\n"));
//		HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_RESET);
		tmpSimBadResponse = (tmpSimBadResponse + 1) % 10;
		if(tmpSimBadResponse % 4 == 0){
			simReset();
			return 0;
		} else if(!tmpSimBadResponse){
			HAL_NVIC_SystemReset();
		}
		osDelay(1000);
//		rxUartSIM_IT();
	}
	HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_RESET);
	timestamp[LEN_TIMESTAMP] = '\0';
	time_t t = strtoull(timestamp, NULL, 0);
    struct tm* pTm;
	pTm = gmtime(&t);
	if(pTm != NULL){
		tmpTime.Hours = pTm->tm_hour;
		tmpTime.Minutes = pTm->tm_min;
		tmpTime.Seconds = pTm->tm_sec;

		tmpDate.Date = pTm->tm_mday;
		tmpDate.Month = pTm->tm_mon;
		tmpDate.Year = pTm->tm_year - 100;

		if(tmpDate.Year < 30 && tmpDate.Year > 19){  //sometimes timestamp is wrong and has value like 2066 year
			HAL_RTC_SetTime(&hrtc, &tmpTime, RTC_FORMAT_BIN);
			HAL_RTC_SetDate(&hrtc, &tmpDate, RTC_FORMAT_BIN);
		}
	}
	return t;
}

/*void getMaxNumDS1820(BKTE* pBkte){
	u8 numReInit = 0;
	for(u8 i = 0; i < BKTE_MAX_CNT_1WIRE; i++){
		setTempLine(i);
		printf("TERM %d: \r\n", i);
		while(owReset(&huart3) != OW_OK && numReInit != OW_MAX_REINIT_ATTEMPT) numReInit++;
		pBkte->isOwActive[i] = numReInit != OW_MAX_REINIT_ATTEMPT ? 1 : 0;
		numReInit = 0;
		resetTempLine(i);
	}
}

void setTempLine(u8 numLine){
	HAL_GPIO_WritePin(oneWirePorts[numLine], oneWirePins[numLine], GPIO_PIN_SET);
}

void resetTempLine(u8 numLine){
	HAL_GPIO_WritePin(oneWirePorts[numLine], oneWirePins[numLine], GPIO_PIN_RESET);
}*/

void fillPckgVoltAmper(PckgVoltAmper* pckg, u16* data){
	pckg->unixTimeStamp = getUnixTimeStamp();
	pckg->amper = data[BKTE_NUM_CURRENT + 1] << 8 | data[BKTE_NUM_CURRENT];
	pckg->volt = data[BKTE_NUM_VOLT + 1] << 8 | data[BKTE_NUM_VOLT];
}

void fillPckgEnergy(PckgEnergy* pckg, u16* data){
	pckg->unixTimeStamp = getUnixTimeStamp();

	pckg->enAct = data[BKTE_NUM_ACT_ENERGY + 3] << 24 | data[BKTE_NUM_ACT_ENERGY + 2] << 16 |
	data[BKTE_NUM_ACT_ENERGY + 1] << 8 | data[BKTE_NUM_ACT_ENERGY];
	
	pckg->enReact = data[BKTE_NUM_REACT_ENERGY + 3] << 24 | data[BKTE_NUM_REACT_ENERGY + 2] << 16 |
	data[BKTE_NUM_REACT_ENERGY + 1] << 8 | data[BKTE_NUM_REACT_ENERGY];
}

void fillPckgTemp(PckgTemp* pckg, s8* data){
	pckg->unixTimeStamp = getUnixTimeStamp();
	memcpy(pckg->temp, data, 4);
}

void fillTelemetry(PckgEnergy* pckg, TYPE_TELEMETRY typeTel, u32 value){
	/*pckg->energyData.enAct = MSG_TELEMETRY;
	pckg->energyData.enReact = value;
    pckg->energyData.current = typeTel;*/
	D(printf("OK: fillTelemetry\r\n"));
}

u32 getUnixTimeStamp(){
	time_t t;
	static struct tm curTime;

	xSemaphoreTake(mutexRTCHandle, portMAX_DELAY);
	HAL_RTC_GetTime(&hrtc, &tmpTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &tmpDate, RTC_FORMAT_BIN);
	curTime.tm_year = tmpDate.Year + 100;
	curTime.tm_mday = tmpDate.Date;
	curTime.tm_mon  = tmpDate.Month;

	curTime.tm_hour = tmpTime.Hours;
	curTime.tm_min  = tmpTime.Minutes;
	curTime.tm_sec  = tmpTime.Seconds;
	xSemaphoreGive(mutexRTCHandle);
	curTime.tm_isdst = 0;

	t = mktime(&curTime);
	return (u32)t;
}

u32 getFlashData(u32 ADDR){
	return (*(__IO u32*) ADDR);
}

/*u8 getDeviation(EnergyData* pCurData, EnergyData* pLastData){
	float low = (1 - BKTE_PERCENT_DEVIATION_ENERGY_DATA);
	float high = (1 + BKTE_PERCENT_DEVIATION_ENERGY_DATA);
	if(pLastData->current * low > pCurData->current || pLastData->current * high < pCurData->current ||
			pLastData->enAct * low > pCurData -> enAct || pLastData->enAct * high < pCurData->enAct ||
			pLastData->enReact * low > pCurData -> enReact || pLastData->enReact * high < pCurData->enReact ||
			pLastData->volt * low > pCurData -> volt || pLastData->volt * high < pCurData->volt){
		*pLastData = *pCurData;
		return 1;
	}
	return 0;
}*/

u8 isCrcOk(char* pData, int len){
	char tmp[] = {'\0', '\0', '\0'};
	memcpy(tmp, pData + 8, 2);
	u8 crcCalc = crc8(pData + 20, len);
	u8 crcRecv = strtoul(tmp, NULL, 16);
	if(crcCalc == crcRecv){
		return 1;
	} else
		return 0;
}

u8 crc8(char *pcBlock, int len){
    u8 crc = 0xFF;
    while (len--){
        crc ^= *pcBlock++;
        for (u8 i = 0; i < 8; i++)
            crc = crc & 0x80 ? (crc << 1) ^ 0x31 : crc << 1;
    }
    return crc;
}

void setTempLine(u8 numLine){
	HAL_GPIO_WritePin(oneWirePorts[numLine], oneWirePins[numLine], GPIO_PIN_SET);
}

void resetTempLine(u8 numLine){
	HAL_GPIO_WritePin(oneWirePorts[numLine], oneWirePins[numLine], GPIO_PIN_RESET);
}



//void getSzNewSoft(){
//	u8 timestamp[LEN_TIMESTAMP + 1];
//	memset(timestamp, '\0', LEN_TIMESTAMP);
//	rxUartSIM_IT();
//	while(simGetDateTime(timestamp) != SIM_SUCCESS){
//		memset(timestamp, '\0', LEN_TIMESTAMP);
//		printf("ERROR: bad time\r\n");
//		sdWriteError("simGetDateTime() ISSUE:BAD_TIME\r\n");
////		rxUartSIM_IT();
//	}
//}


void offAllLeds(){
	HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED2G_GPIO_Port, LED2G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED2R_GPIO_Port, LED2R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED3G_GPIO_Port, LED3G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED4G_GPIO_Port, LED4G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED4R_GPIO_Port, LED4R_Pin, GPIO_PIN_SET);
}



/*void offAllRedLeds(){
	HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED2R_GPIO_Port, LED2R_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED4R_GPIO_Port, LED4R_Pin, GPIO_PIN_SET);
}

void toggleGreenLeds(){
	HAL_GPIO_TogglePin(LED1G_GPIO_Port, LED1G_Pin);
	HAL_GPIO_TogglePin(LED2G_GPIO_Port, LED2G_Pin);
	HAL_GPIO_TogglePin(LED4G_GPIO_Port, LED4G_Pin);
}

void toggleRedLeds(){
	  HAL_GPIO_TogglePin(LED1R_GPIO_Port, LED1R_Pin);
	  HAL_GPIO_TogglePin(LED2R_GPIO_Port, LED2R_Pin);
	  HAL_GPIO_TogglePin(LED4R_GPIO_Port, LED4R_Pin);
}*/

void checkBufForWritingToFlash(){  //!need to delete
	
	D(printf("checkBufForWritingToFlash()\r\n"));
	/*xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
	if((circBufPckgEnergy.numPckgInBuf + 1) * SZ_PCKGENERGY + 1 > spiFlash64.pgSz){
		updSpiFlash();
	}
	xSemaphoreGive(mutexWriteToEnergyBufHandle);*/
}

void updSpiFlash(){ //!need to delete
		/*u8 isFullSpiFlash = 0;
		PckgEnergy curPckgEnergy;
		isFullSpiFlash =  spiFlashWrPg(circBufPckgEnergy.buf, circBufPckgEnergy.numPckgInBuf * SZ_PCKGENERGY, 0, spiFlash64.headNumPg);
		cBufReset(&circBufPckgEnergy);
		if(isFullSpiFlash){
		  	fillTelemetry(&curPckgEnergy, TEL_SERV_FLASH_CIRC_BUF_FULL, 0);
			cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
		}	

	  	if(!spiFlash64.headNumPg){
			fillTelemetry(&curPckgEnergy, TEL_SERV_FLASH_CIRC_BUF_END_HEAD, 0);
			cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
	  	} else if(spiFlash64.headNumPg == SPIFLASH_NUM_PG_GNSS / 2){
			fillTelemetry(&curPckgEnergy, TEL_SERV_FLASH_CIRC_BUF_HALF_HEAD, 0);
			cBufWriteToBuf(&circBufPckgEnergy, (u8*)&curPckgEnergy, SZ_PCKGENERGY);
		}
		*/
	D(printf("updSpiFlash()\r\n"));
}


void waitGoodCsq(){
	u8 csq = 0;
	u16 cntNOCsq = 0;
	while((csq = simCheckCSQ()) < 15 && csq > 99){
		osDelay(2000);
		// saveCsq(csq);
		cntNOCsq++;
		if(cntNOCsq == 1800){
			bkte.erFlags.simCSQINF = 1;
			sdWriteLog(SD_ER_CSQINF, SD_LEN_HTTP, NULL, 0, &sdSectorLogError);
			cntNOCsq = 0;
		}
	}
	bkte.erFlags.simCSQINF = 0;
}

void saveData(u8* data, u8 sz, u8 cmdData, CircularBuffer* cbuf){
	u16 bufEnd[2] = {0, BKTE_PREAMBLE};
	xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
	if(cbuf->writeAvailable < sz + 1 + 4){
		bufEnd[0] = calcCrc16(cbuf->buf, cbuf->readAvailable);
		cBufWriteToBuf(cbuf, (u8*)bufEnd, 4);
		spiFlashWrPg(cbuf->buf, cbuf->readAvailable, 0, spiFlash64.headNumPg);
		cBufReset(cbuf);
	} else {
		cBufWriteToBuf(cbuf, &cmdData, 1);
		cBufWriteToBuf(cbuf, data, sz);
	}
	xSemaphoreGive(mutexWriteToEnergyBufHandle);
}



u8 isDataFromFlashOk(char* pData, u8 len){
    u16 crc;
    for(u8 i = len - 1; i; --i){
        if(pData[i] == BKTE_PREAMBLE_LSB && pData[i - 1] == BKTE_PREAMBLE_MSB){
            crc = pData[i - 3] | pData[i - 2] << 8;
            if(calcCrc16(pData, i - 3) == crc){
				len = (i + 1) - 4;
                return len;
            }
        }
    }
    return 0;
}