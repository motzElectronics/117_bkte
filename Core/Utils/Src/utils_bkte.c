/*
 * utils_bkte.c
 *
 *  Created on: Mar 16, 2020
 *      Author: bls
 */

#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/utils_sd.h"
#include "../Utils/Inc/utils_pckgs_manager.h"
#include "../Utils/Inc/utils_crc.h"
GPIO_TypeDef* oneWirePorts[BKTE_MAX_CNT_1WIRE] = {ONEWIRE_1_EN_GPIO_Port, ONEWIRE_2_EN_GPIO_Port, ONEWIRE_3_EN_GPIO_Port, ONEWIRE_4_EN_GPIO_Port};
u16 oneWirePins[BKTE_MAX_CNT_1WIRE] = {ONEWIRE_1_EN_Pin, ONEWIRE_2_EN_Pin, ONEWIRE_3_EN_Pin, ONEWIRE_4_EN_Pin};
//extern osMessageQId queue1WireHandle;
extern UART_HandleTypeDef huart3;
extern RTC_HandleTypeDef hrtc;
extern osMutexId mutexRTCHandle;
extern osMutexId mutexWriteToEnergyBufHandle;
extern osMutexId mutexWebHandle;
extern osThreadId getNewBinHandle;
extern osSemaphoreId semCreateWebPckgHandle;
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
	if(bkte.pwrInfo.isPwrState){
		HAL_GPIO_WritePin(BAT_PWR_EN_GPIO_Port, BAT_PWR_EN_Pin, GPIO_PIN_RESET);
		HAL_Delay(5000);
		NVIC_SystemReset();
	}

	for(u8 i = 0; i < 3; i++)
		bkte.idMCU[i] = getFlashData(BKTE_ADDR_ID_MCU + (i * 4));
	D(printf("%08x%08x%08x\r\n",
               (uint)bkte.idMCU[0], (uint)bkte.idMCU[1], (uint)bkte.idMCU[2]));
	bkte.hwStat.regHardWareStat = 0;
	bkte.erFlags.errReg = 0;
}


void getServerTime(){
	u8 bufTime[4];
	if(generateWebPckgReq(CMD_REQUEST_SERVER_TIME, NULL, 0, SZ_REQUEST_GET_SERVER_TIME, bufTime, 4) == ERROR){
		sdWriteLog(SD_ER_BAD_SERVERTIME, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogs);
		D(printf("ERROR: bad server time\r\n"));
	} else{
		time_t t = bufTime[0] << 24 | bufTime[1] << 16 | bufTime[2] << 8 | bufTime[3];
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
	}
}

void getNumFirmware(){
	u8 bufFirmware[4];
	if(generateWebPckgReq(CMD_REQUEST_NUM_FIRMWARE, NULL, 0, SZ_REQUEST_GET_NUM_FIRMWARE, bufFirmware, 4) == ERROR){
		sdWriteLog(SD_ER_NUM_FIRMWARE, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogs);
		D(printf("ERROR: getNumFirmware()\r\n"));
	} else{
		u32 numFirmware = bufFirmware[0] << 24 | bufFirmware[1] << 16 | bufFirmware[2] << 8 | bufFirmware[3];
		if(numFirmware != BKTE_ID_FIRMWARE && numFirmware > 0){
			D(printf("New FIRMWARE v.:%d\r\n", (int)numFirmware));
			vTaskResume(getNewBinHandle);
		}

	}
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
	u32 crcCalc = crc32_byte(pData, len);
	u32 crcRecv = pData[len] << 24 | pData[len + 1] << 16 | pData[len + 2] << 8 | pData[len + 3];
	if(crcCalc != crcRecv){
		D(printf("ERROR: crc \r\n"));
	}
	for(u8 i = 0; i < sizeof(u32); i++){
		pData[len + i] = 0xFF; //! clear crc32
	}

	return crcCalc == crcRecv;
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

void updSpiFlash(CircularBuffer* cbuf){
	u16 bufEnd[2] = {0, BKTE_PREAMBLE};
	// xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);

	bufEnd[0] = calcCrc16(cbuf->buf, cbuf->readAvailable);
	cBufWriteToBuf(cbuf, (u8*)bufEnd, 4);
	spiFlash64.lock = 0;
	spiFlashWrPg(cbuf->buf, cbuf->readAvailable, 0, spiFlash64.headNumPg);
	cBufReset(cbuf);

	// xSemaphoreGive(mutexWriteToEnergyBufHandle);
	D(printf("updSpiFlash()\r\n"));
}


void waitGoodCsq(){
	u8 csq = 0;
	u16 cntNOCsq = 0;
	while((csq = simCheckCSQ()) < 5 || csq > 99){
		osDelay(3000);
		// saveCsq(csq);
		cntNOCsq++;
		if(cntNOCsq == 1800){
			bkte.erFlags.simCSQINF = 1;
			sdWriteLog(SD_ER_CSQINF, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
			cntNOCsq = 0;
		}
		D(printf("ER: CSQ %d\r\n", csq));
	}
	bkte.erFlags.simCSQINF = 0;
	D(printf("OK: CSQ %d\r\n", csq));
}

void saveData(u8* data, u8 sz, u8 cmdData, CircularBuffer* cbuf){
	u16 bufEnd[2] = {0, BKTE_PREAMBLE};
	xSemaphoreTake(mutexWriteToEnergyBufHandle, portMAX_DELAY);
	if(cbuf->writeAvailable < sz + 2 + 4){
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

void copyTelemetry(u8* buf, PckgTelemetry* pckgTel){
	memcpy(buf, &pckgTel->unixTimeStamp, 4);
	memcpy(buf + 4, &pckgTel->group, 1);
	memcpy(buf + 5, &pckgTel->code, 1);
	memcpy(buf + 6, &pckgTel->data, 4);
}

void saveTelemetry(PckgTelemetry* pckg, CircularBuffer* cbuf){
	u8 buf[10];
	pckg->unixTimeStamp = getUnixTimeStamp();
	copyTelemetry(buf, pckg);
	saveData(buf, SZ_CMD_TELEMETRY, CMD_DATA_TELEMETRY, cbuf);
}