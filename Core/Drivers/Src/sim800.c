/*
 * sim800.c
 *
 *  Created on: Feb 20, 2020
 *      Author: bls
 */

#include "../Drivers/Inc/sim800.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"
//#include "utils_sd.h"
//#define PEER_CLOSED 1 // (u8*)"http://188.242.176.25:8080/api/add/measures";
#define PEER_CLOSED 1 // (u8*)"http://ptsv2.com/t/rt98e-1582717549/post";

extern osSemaphoreId semRxUartSIMHandle;
extern osMessageQId queueRxSIMHandle;
//extern char logError[LOG_SZ_ERROR];
/*extern FIL fError;
extern char logError[LOG_SZ_ERROR];*/

char responseSIMbuf[UART_SZ_RX_RESPONSE];
char simBufCommand[COMMAND_BUF_SZ];
char bufResponse[RESPONSE_BUF_SZ];
//u8* HTTP_URL;
HttpUrl urls;
UartInfo gnssUartInfo;
UartInfo gsmUartInfo;
char* headUrlFileSz = "http://188.242.176.25:8080/api/filesize?uid=";
static char arrUrlFileSz[70];
extern BKTE bkte;
//u8* HTTP_URL = (u8*)"http://ptsv2.com/t/rt98e-1582717549/post";
//#define PEER_CLOSED 0; // (u8*)"http://ptsv2.com/t/rt98e-1582717549/post";
//u8* HTTP_URL = (u8*)"http://188.242.176.25:8080/api/add/measures";


u8 gpsInit(u8 retriesCount){
	char param[50];
	memset(param, '\0', 50);
	sprintf(param,"%d,%d,\"%s\",\"%s\"", (int)3, (int)1, "Contype", "GPRS");
	if(httpWriteCommand(SIM_SAPBR, param, retriesCount, SIM_OK) == SIM_FAIL){
		return SIM_FAIL;
	}

	memset(param, '\0', 50);
	sprintf(param,"%d,%d,\"%s\",\"%s\"", (int)3, (int)1, "APN", "www");
	if(httpWriteCommand(SIM_SAPBR, param, retriesCount, SIM_OK) == SIM_FAIL){
		return SIM_FAIL;
	}

	memset(param, '\0', 50);
	sprintf(param,"%d,%d", (int)1, (int)1); // Enable GPRS
	if(httpWriteCommand(SIM_SAPBR, param, retriesCount, SIM_OK) == SIM_FAIL){
		return SIM_FAIL;
	}

	return SIM_SUCCESS;
}

u8 httpInit(char* httpAddr, u8 retriesCount){

	char temp[100];
	char* retMsg = simExecCommand(INIT_HTTP_SERVICE, &gsmUartInfo);
	char* token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
	while(strcmp((const char*)token, (const char*)SIM_OK_TEXT)){
		sprintf(bufResponse, "INIT_HTTP_SERVICE: %s\r\n", token);
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
		copyStr(bufResponse, token, RESPONSE_BUF_SZ);
		if(strcmp((const char*)token, "AT+HTTPINIT") == 0 ||
				strcmp((const char*)token, SIM_NO_RESPONSE_TEXT) == 0){
			return SIM_RESTART;
		}
		printf("ERROR: INIT_HTTP_SERVICE %s\r\n", token);
		if (retriesCount-- == 0)
			return SIM_FAIL;
		osDelay(300);
		httpDeInit();
		retMsg = simExecCommand(INIT_HTTP_SERVICE, &gsmUartInfo);
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	}
	printf("OK: INIT_HTTP_SERVICE %s\r\n", token);

	sprintf((char*)temp,"\"%s\",\"%s\"", URL_HTTP_PARAMETER, httpAddr);
	if(httpWriteCommand(SET_HTTP_PARAMETERS_VALUE, temp, retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}

	sprintf((char*)temp,"\"%s\",\"application/json\"", CONTENT_HTTP_PARAMETER);
	if(httpWriteCommand(SET_HTTP_PARAMETERS_VALUE, temp, retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	return SIM_SUCCESS;
}

char* httpDeInit(){
	char* retMsg =  simExecCommand(TERM_HTTP_SERVICE, &gsmUartInfo);
	printf("DEINIT: %s\r\n", retMsg);
	return retMsg;
}

char* simExecCommand(char* httpCommand, UartInfo* pUInf){
	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
	sprintf((char*)simBufCommand, "AT+%s\r\n", httpCommand);
	return simTxATCommand(simBufCommand, strlen(simBufCommand), pUInf);
}

//char* gnssExecCommand(char* httpCommand){
//	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
//	sprintf((char*)simBufCommand, "AT+%s\r\n", httpCommand);
//	return simTxATCommand(simBufCommand, strlen(simBufCommand));
//}

u8 simGetDateTime(char* timestamp){
	simHttpInit(urls.getTime);
	char* retMsg;
	u8 retStatus = httpGet(&retMsg, 10, 10000);
	memcpy(timestamp, retMsg + 13, 10);
	printf("%s\r\n", retMsg);
	return retStatus;
}

u8 httpWriteCommand(char* commandCode, char* params, u8 retriesCount, char* SUCCES_RETURN){
 	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
	sprintf((char*)simBufCommand, "AT+%s=%s\r\n", commandCode, params);
	char* retMsg = simTxATCommand(simBufCommand, strlen(simBufCommand), &gsmUartInfo);
	char* token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
	while(strcmp((const char*)token, (const char*)SUCCES_RETURN)){
		sprintf(bufResponse, "%s %s return: %s\r\n", commandCode, params, token);
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
		copyStr(bufResponse, token, RESPONSE_BUF_SZ);
		sprintf((char*)bufResponse + strlen(token), "\r\n");
		if(strcmp((const char*)bufResponse, (const char*)simBufCommand) == 0){
			return SIM_RESTART;
		}
		printf("ERROR:   %s %s return: %s\r\n", commandCode, params, token);
		retriesCount--;
		if (!retriesCount)
			return SIM_FAIL;
		retMsg = simTxATCommand(simBufCommand, strlen(simBufCommand), &gsmUartInfo);
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	}
	printf("OK:   %s %s return: %s\r\n", commandCode, params, token);
	return SIM_SUCCESS;
}

char* httpWriteData(char* data, u16 sz){
	return simTxATCommand(data, sz, &gsmUartInfo);
}

char* simTxATCommand(char* command, u16 sz, UartInfo* pUInf){
//	pTxUart(command, sz);
	txUart(command, sz, pUInf);
	return getStatusSim(pUInf);
}


void simInit(){
//	HAL_UART_Transmit(gnssUartInfo.pHuart,
//			(u8*)"$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0*35\r\n",
//			sizeof("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0*35\r\n"), 1000);
//	txUart("AT+CGNSSEQ=\"RMC\"", sizeof("AT+CGNSSEQ=\"RMC\""), &gnssUartInfo);
//	rxUart(&gnssUartInfo);

	gsmUartInfo.pHuart = &huart6;
	rxUart(&gsmUartInfo);


	simWriteCommand("ATE0");
	osDelay(500);
	memset(arrUrlFileSz, '\0', sizeof(arrUrlFileSz));
	sprintf(arrUrlFileSz, "%s%08x%08x%08x", headUrlFileSz, bkte.idMCU[0], bkte.idMCU[1], bkte.idMCU[2]);
	urls.getTime = "http://188.242.176.25:8080/api/time";
	urls.getSzSoft = arrUrlFileSz;
	urls.getPartFirmware = "http://188.242.176.25:8080/api/getFile";
#if ( PEER_CLOSED == 1 )
	urls.addMeasure = "http://188.242.176.25:8080/api/add/measures";
#elif (PEER_CLOSED == 0)
	urls.addMeasure = "http://ptsv2.com/t/p76ko-1587044533/post";
#endif
	printf("simINIT()\r\n");
//	simWriteCommand(simBufCommand, COMMAND_BUF_SZ, (u8*)"AT+IPR=115200");
//	HAL_Delay(500);
}

void simWriteCommand(char* command){
	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
	sprintf(simBufCommand, "%s\r\n", command);
	txUart(simBufCommand, strlen(simBufCommand), &gsmUartInfo);
}

u8 httpPost(char* txData, u16 szTx, char** pRxData, u8 retriesCount, u32 httpTimeout){
	char param[15];
//	u8 cnt = 0;

	sprintf(param,"%d,%d", (int)szTx, (int)httpTimeout);
	if(httpWriteCommand(INPUT_HTTP_DATA, param, retriesCount, SIM_DOWNLOAD) == SIM_FAIL){
		return SIM_FAIL;
	}

	char* retMsg = httpWriteData(txData, szTx);
	char* token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
	if(strcmp((const char*)token, (const char*)SIM_OK_TEXT)){
		printf("ERROR: txData %s\r\n", retMsg);
		sprintf(bufResponse, "httpWriteData(): %s\r\n", retMsg);
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
		return SIM_FAIL;
	}
	printf("OK: txData %s return : %s\r\n", txData, retMsg);

	if(httpWriteCommand(HTTP_METHOD_ACTION, "1", 1, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	if(gsmUartInfo.rxBuffer[6] == '\0')
		waitIdle("+HTTPACTION", &gsmUartInfo.irqFlags, 100, WAIT_TIMEOUT);
	if(gsmUartInfo.rxBuffer[6] == '\0'){
		printf("ERROR: +HTTPACTION no resp\r\n");
		sprintf(bufResponse, "+HTTPACTION no resp\r\n");
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
	}else if(atoi((char*)gsmUartInfo.rxBuffer + 23) == HTTP_CODE_OK){
		printf("OK: HTTP_CODE_OK\r\n");
		return httpRead(pRxData);
	}else{
		printf("%s\r\n", gsmUartInfo.rxBuffer + 8);
		sprintf(bufResponse, "HTTP_CODE_OK: %d\r\n", atoi((char*)gsmUartInfo.rxBuffer + 23));
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
	}
	return SIM_FAIL;
}

//void waitSim(char* waitStr){
//	u16 timeout = 0;
//	while(!simUartInfo.isIrqIdle && timeout < SIM_TIMEOUT_RESPONSE){
//		osDelay(200);
//		timeout += 200;
//		printf("%s timeout: %d\r\n", waitStr, timeout);
//	}
//}


char* getStatusSim(UartInfo* pUInf){
	waitIdle("wait getSattusSim()", &(pUInf->irqFlags), 100, WAIT_TIMEOUT);
	if(pUInf->irqFlags.isIrqIdle){
//		copyStr(bufResponse, (char*)simUartInfo.rxBuffer, sizeof(bufResponse));
		pUInf->irqFlags.isIrqIdle = 0;
		return (char*)pUInf->rxBuffer;
	} else{
		return SIM_NO_RESPONSE_TEXT;
	}

}

u8 simGetSzSoft(u32* szSoft){
	char*	retMsg;
	httpDeInit();
	if(httpInit(urls.getSzSoft, 2) == SIM_SUCCESS){
		printf("OK: httpInit(getSoft)\r\n");
//		HAL_GPIO_TogglePin(LED2R_GPIO_Port, LED2R_Pin);
		httpGet(&retMsg, 10, 10000);
		if(atoi(retMsg) > 1000){
			*szSoft = atoi(retMsg);
			return SIM_SUCCESS;
		}
	}
	return SIM_FAIL;
}

void copyStr(char* dist, char* source, u16 distSz){
	memset(dist, '\0', distSz);
	sprintf(dist, "%s", source);
}

void copyStrFromTo(char* dist, char* source, u16 numFrom, u16 numTo, u16 distSz){
	memset(dist, '\0', distSz);
	memcpy(dist, source + numTo, numTo - numFrom);
}

u8 httpGet(char** pRxData, u8 retriesCount, u32 httpTimeout){
	if(httpWriteCommand(HTTP_METHOD_ACTION, "0", retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	if(gsmUartInfo.rxBuffer[6] == '\0')
		waitIdle("+HTTPACTION", &gsmUartInfo.irqFlags, 100, WAIT_TIMEOUT);
	if(gsmUartInfo.rxBuffer[6] == '\0'){
		printf("ERROR: +HTTPACTION no resp\r\n");
		sprintf(bufResponse, "+HTTPACTION no resp\r\n");
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
	}else if(atoi((char*)gsmUartInfo.rxBuffer + 23) == HTTP_CODE_OK){
		printf("OK: HTTP_CODE_OK\r\n");
		return httpRead(pRxData);
	}else{
		printf("%s\r\n", gsmUartInfo.rxBuffer);
		sprintf(bufResponse, "HTTP_CODE_OK: %d\r\n", atoi((char*)gsmUartInfo.rxBuffer + 23));
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
	}
	return SIM_FAIL;
}

u8 httpRead(char** pRxData){
	char* token;
	u16 httpCode;
	simExecCommand(READ_THE_HTTP_SERVER_RESPONSE, &gsmUartInfo);
	osDelay(100);
//	waitIdle("waitIdle status httpRead", &gsmUartInfo.irqFlags);
	if(gsmUartInfo.rxBuffer[0] == '\0'){
		printf("ERROR: HTTPREAD sim no resp\r\n");
//		createLog(logError, LOG_SZ_ERROR, "HTTPREAD sim no resp\r\n");
		return SIM_FAIL;
	} else
		token = strtok((char*)gsmUartInfo.rxBuffer, SIM_SEPARATOR_TEXT);

//	waitSim("wait HTTPREAD resp server");
	if(token == NULL || token[0] == '\0')
		return SIM_HTTP_BAD_CODE_REQUEST;
	else if(gsmUartInfo.rxBuffer[strlen(token) + 4] == '\0'){
		printf("ERROR: HTTPREAD server no response\r\n");
//		createLog(logError, LOG_SZ_ERROR, "HTTPREAD server no resp\r\n");
		return SIM_HTTP_BAD_CODE_REQUEST;
	} else
		token = strtok((char*)gsmUartInfo.rxBuffer + strlen(token) + 4, SIM_SEPARATOR_TEXT);

		//	httpWriteCommand(READ_THE_HTTP_SERVER_RESPONSE, "0,39", 3, SIM_OK);
	if(token == NULL || token[0] == '\0'){
		printf("httpRead() token NULL\r\n");
//		token = SIM_NO_RESPONSE_TEXT;
		return SIM_FAIL;
	}
	copyStr(responseSIMbuf, token, sizeof(responseSIMbuf));
	*pRxData = responseSIMbuf;
//	*pRxData = token;
	if((httpCode = atoi(token + strlen(token) - 4)) == HTTP_CODE_OK){
		printf("status POST: 200\r\n");
		return SIM_SUCCESS;
	}else{
		printf("ERROR status POST: %d\r\n", httpCode);
//		char tmpBuf[50];
//		sprintf(tmpBuf, "HTTPREAD server code: %s\r\n", token);
//		createLog(logError, LOG_SZ_ERROR, tmpBuf);
	}
	return SIM_FAIL;
}

u8 httpReadFirmwareSize(char** pRxData){
	char* token;
//	u16 httpCode;
	if(httpWriteCommand(READ_THE_HTTP_SERVER_RESPONSE, "0,26", 3, SIM_OK_TEXT) == SIM_FAIL){
			return SIM_FAIL;
	} else{
		token = strtok((char*)(gsmUartInfo.rxBuffer + 26), SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
		printf("status POST: %d\r\n", atoi(token + strlen(token) - 4));  //data
		if(atoi(token) > 1000){
			copyStr(responseSIMbuf, token, sizeof(responseSIMbuf));
			*pRxData = responseSIMbuf;
//			*pRxData = token;
			return SIM_SUCCESS;
		}else{
			char tmpBuf[50];
			sprintf(tmpBuf, "HTTPREAD server code: %s\r\n", token);
//			createLog(logError, LOG_SZ_ERROR, tmpBuf);
		}
		return SIM_FAIL;

	}
}

void simHttpInit(char* url){
	u8 resCode;
	httpDeInit();
	for(u8 i = 0; i < 3; i++){
		if((resCode = httpInit(url, 2)) != SIM_SUCCESS){
			printf("ERROR: httpInit()\r\n");
			httpDeInit();
			if(resCode == SIM_RESTART) resetSim();
		} else{
			printf("OK: simHttpInit()\r\n");
			break;
		}
	}
}

u8 simGetSignalLevel(){
	char* retMsg;
	char* token;
	u8 csq;
	retMsg = simExecCommand(SIM_CSQ, &gsmUartInfo);  // check signal level
	token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token != NULL && strlen(token) > 8)
		csq = atoi(token + 6);
	else csq = 0;
	return csq;
}

bool simIsGoodCSQ(u8* pCsq){
	u8 csq = 0;
	char* retMsg;
	char* token;
	retMsg = simExecCommand(SIM_CSQ, &gsmUartInfo);  // check signal level
	token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token != NULL && strlen(token) > 8)
		csq = atoi(token + 6);
	else csq = 0;
	*pCsq = csq;
	if(csq > 10 && csq < 99)
		return true;
	else
		return false;
}

void simOn(){
	HAL_GPIO_WritePin(SIM_PWR_EN_GPIO_Port, SIM_PWR_EN_Pin, GPIO_PIN_SET);
	osDelay(1500);
	HAL_GPIO_WritePin(GSM_PWRKEY_GPIO_Port, GSM_PWRKEY_Pin, GPIO_PIN_RESET);
	osDelay(1000);
	HAL_GPIO_WritePin(GSM_PWRKEY_GPIO_Port, GSM_PWRKEY_Pin, GPIO_PIN_SET);
	osDelay(1500);
	HAL_GPIO_WritePin(GSM_PWRKEY_GPIO_Port, GSM_PWRKEY_Pin, GPIO_PIN_RESET);
	osDelay(5000);
}

void simOff(){
	HAL_GPIO_WritePin(SIM_PWR_EN_GPIO_Port, SIM_PWR_EN_Pin, GPIO_PIN_RESET);
	osDelay(1000);
}

void resetSim(){
	u8 tmpSimBadResponse = 0;
	char* retMsg;
	char* token;
	D(printf("WARINIG!: R E S E T !\r\n"));
	simOff();
	simOn();
//	  createLog(logError, LOG_SZ_ERROR, "R E S E T !\r\n");
//	  simWriteCommand("AT+CRESET");
	//TODO: ????? ??????? ??????????? ????????? ?????????? gsm ? ?????? ????? ????????, ???? ?? ???????
//	__HAL_DMA_DISABLE(gnssUartInfo.pHuart->hdmarx);
//	__HAL_DMA_ENABLE(gnssUartInfo.pHuart->hdmarx);
	while(1){
		simInit();
		retMsg = simTxATCommand("AT\r\n", strlen("AT\r\n"), &gsmUartInfo);
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
		printf("simInit AT: %s\r\n", token);
		if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
		if((strcmp(token, SIM_OK_TEXT)) != 0){
	//				sdWriteError("simInit() ISSUE:BAD_RESPONSE_AT_COMMAND\r\n");
			tmpSimBadResponse = (tmpSimBadResponse + 1) % 10;
			if(!tmpSimBadResponse){
			  D(printf("WARINTING!: T O T A L  R E S E T\r\n"));
	//				  createLog(logError, LOG_SZ_ERROR, "ERROR: TOTAL RESET \r\n");
			  osDelay(3000);
			  HAL_NVIC_SystemReset();
			}
			osDelay(5000);
		} else {
			httpWriteCommand(SIM_SAPBR, "1,1", 1, SIM_OK);
			break;
		}
	}
}
