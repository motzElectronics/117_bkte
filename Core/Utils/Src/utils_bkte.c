/*
 * utils_bkte.c
 *
 *  Created on: Mar 16, 2020
 *      Author: bls
 */

#include "../Utils/Inc/utils_bkte.h"

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
extern CircularBuffer circBufPckgEnergy;
//extern FIL fAddr;
/*static u8 lenLog[] = {SD_LEN_LOG_ENERGY, SD_LEN_LOG_TEMP, SD_LEN_LOG_RSSI};
static char* fNamesLog[NUM_READ_FILES] = {FILE_LOG_ENERGY, FILE_LOG_TEMP, FILE_LOG_RSSI};
static char* fNamesAddr[NUM_READ_FILES] = {FILE_ADDR_ENERGY, FILE_ADDR_TEMP, FILE_ADDR_RSSI};*/
void bkteInit(){
	char* retMsg;

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
}

u32 getServerTime(){

	u8 tmpSimBadResponse = 0;
	char timestamp[LEN_TIMESTAMP + 1];
	memset(timestamp, '\0', sizeof(timestamp));
	D(printf("getServerTime()\r\n"));
	while(simGetDateTime(timestamp) != SIM_SUCCESS){
		memset(timestamp, '\0', sizeof(timestamp));
		D(printf("ERROR: bad time\r\n"));
//		HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_RESET);
		tmpSimBadResponse = (tmpSimBadResponse + 1) % 10;
		if(!tmpSimBadResponse){
			simReset();
			return 0;
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

		if(tmpDate.Year < 30){  //sometimes timestamp is wrong and has value like 2066 year
			HAL_RTC_SetTime(&hrtc, &tmpTime, RTC_FORMAT_BIN);
			HAL_RTC_SetDate(&hrtc, &tmpDate, RTC_FORMAT_BIN);
		}
	}
	return t;
}

u8 getGnssPckg(u8* pBuf, u16 szBuf, PckgEnergy* pPckgGnss, u8 szPckg){
	static u16 head = 0;
	u8 crc;
	while(head < szBuf){
		if(pBuf[head] == BKTE_PREAMBLE_EN2 && pBuf[(head + 1) % szBuf] == BKTE_PREAMBLE_EN1){
			crc = crc8(pBuf + 2 + head, 20);
			if(crc == pBuf[head + szPckg - 2]){
				D(printf("ok crc GNSSPCKG\r\n"));
				memcpy(pPckgGnss, pBuf + head, szPckg);
				memset(pBuf + head, '\0', szPckg);
				head += szPckg;
				return 1;
			} else {
				D(printf("bad crc GNSSPCKG\r\n"));
			}
		}
		head++;
	}
	head = 0;
	return 0;
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

void fillPckgEnergy(PckgEnergy* pckg, u16* data){

	pckg->energyData.current = data[BKTE_NUM_CURRENT + 1] << 8 | data[BKTE_NUM_CURRENT];
	pckg->energyData.volt = data[BKTE_NUM_VOLT + 1] << 8 | data[BKTE_NUM_VOLT];

	pckg->energyData.enAct = data[BKTE_NUM_ACT_ENERGY + 3] << 24 | data[BKTE_NUM_ACT_ENERGY + 2] << 16 |
			data[BKTE_NUM_ACT_ENERGY + 1] << 8 | data[BKTE_NUM_ACT_ENERGY];
	pckg->energyData.enReact = data[BKTE_NUM_REACT_ENERGY + 3] << 24 | data[BKTE_NUM_REACT_ENERGY + 2] << 16 |
			data[BKTE_NUM_REACT_ENERGY + 1] << 8 | data[BKTE_NUM_REACT_ENERGY];
	setDateTime(&pckg->dateTime);
	pckg->crc = crc8((char*)pckg + 2, 20);

}

void fillTempPckgEnergy(PckgEnergy* pckg, s8* data){
	static u32 lastTime = 0;
	u32 curTime = HAL_GetTick();
	pckg->energyData.current = ((curTime - lastTime) / 1000) % 32767;
	lastTime = curTime;

	pckg->energyData.volt = 0;

	pckg->energyData.enAct = MSG_TEMP;
	pckg->energyData.enReact = ((data[0] & 0xFF) << 24)  | ((data[1] & 0xFF) << 16)  | ((data[2] & 0xFF) << 8)  | data[3] & 0xFF ;

	setDateTime(&pckg->dateTime);
	pckg->crc = crc8((char*)pckg + 2, 20);

}

void fillTelemetry(PckgEnergy* pckg, TYPE_TELEMETRY typeTel, u32 value){
	pckg->energyData.enAct = MSG_TELEMETRY;
	pckg->energyData.enReact = value;
    pckg->energyData.current = typeTel;
	setDateTime(&pckg->dateTime);
	pckg->crc = crc8((char*)pckg + 2, 20);
}



void setDateTime(DateTime* dt){
//	RTC_TimeTypeDef tmpTime;
//	RTC_DateTypeDef tmpDate;
	xSemaphoreTake(mutexRTCHandle, portMAX_DELAY);
	HAL_RTC_GetTime(&hrtc, &tmpTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &tmpDate, RTC_FORMAT_BIN);
	dt->day = tmpDate.Date;
	dt->month = tmpDate.Month;
	dt->year = tmpDate.Year;
	dt->sec = tmpTime.Seconds;
	dt->min = tmpTime.Minutes;
	dt->hour = tmpTime.Hours;
	xSemaphoreGive(mutexRTCHandle);
}



void setTM(time_t* pTimeStamp, DateTime* dt){
	static struct tm curTime;
	curTime.tm_year = dt->year + 100;  // In fact: 2000 + 18 - 1900
	curTime.tm_mday = dt->day;
	curTime.tm_mon  = dt->month;

	curTime.tm_hour = dt->hour;
	curTime.tm_min  = dt->min;
	curTime.tm_sec  = dt->sec;

	curTime.tm_isdst = 0;

	*pTimeStamp = mktime(&curTime);
}

time_t getTimeStamp(){
	DateTime tmpDateTime;
	time_t t;
	setDateTime(&tmpDateTime);
	setTM(&t, &tmpDateTime);
	return t;
}


u32 getFlashData(u32 ADDR){
	return (*(__IO u32*) ADDR);
}

u8 getDeviation(EnergyData* pCurData, EnergyData* pLastData){
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
}

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

void checkBufForWritingToFlash(){
	u8 isFullSpiFlash = 0;
	xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
	if((circBufPckgEnergy.numPckgInBuf + 1) * SZ_PCKGENERGY + 1 > spiFlash64.pgSz){
		PckgEnergy curPckgEnergy = {.preambule=BKTE_PREAMBLE_EN};
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

	}
	xSemaphoreGive(mutexWriteToEnergyBufHandle);
}
