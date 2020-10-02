/*
 * sim800.c
 *
 *  Created on: Feb 20, 2020
 *      Author: bls
 */

#include "../Drivers/Inc/simcom.h"
// //#define PEER_CLOSED 1 // (u8*)"http://188.242.176.25:8080/api/add/measures";
// #define PEER_CLOSED 1 // (u8*)"http://ptsv2.com/t/rt98e-1582717549/post";

static char simBufCommand[COMMAND_BUF_SZ];
static char simBufError[COMMAND_BUF_SZ];
static char tmpSimBuf[COMMAND_BUF_SZ];

u8 isJson = 1;

void simInit(){
	char* retMsg;
	char* token;
	u8 simBadAnsw;
	while(1){
		simWriteCommand("ATE0");
		retMsg = simTxATCommand("AT\r\n", strlen("AT\r\n"));
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0')
			token = SIM_NO_RESPONSE_TEXT;
		if((strcmp(token, SIM_OK_TEXT)) != 0){
			D(printf("ERROR: simInit() AT: %s\r\n", token));
			simBadAnsw = (simBadAnsw + 1) % 10;
			if(!simBadAnsw){
				D(printf("ERROR: need reset\r\n"));
				simReset();
			}
			continue;
		}
		if(SIM_GPS_INIT() != SIM_SUCCESS){        
			D(printf("ERROR: NOT CONNECT GPS\r\n"));
        	simReset();
			continue;
		} else {
			break;
		}
	}

	// SIM_GPS_INIT();
	D(printf("OK: simInit()\r\n"));
}

char* simGetStatusAnsw(){
	waitIdle("wait simGetStatusAnsw()", &(uInfoSim.irqFlags), 200, USART_TIMEOUT);
	if(uInfoSim.irqFlags.isIrqIdle){
		uInfoSim.irqFlags.isIrqIdle = 0;
		return (char*)uInfoSim.pRxBuf;
	} else{
		return SIM_NO_RESPONSE_TEXT;
	}

}

void simWriteCommand(char* command){
	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
	sprintf(simBufCommand, "%s\r\n", command);
	uartTx(simBufCommand, strlen(simBufCommand), &uInfoSim);
}

char* simTxATCommand(char* command, u16 sz){
	uartTx(command, sz, &uInfoSim);
	return simGetStatusAnsw();
}

u8 httpWriteCommand(char* commandCode, char* params, u8 retriesCount, char* SUCCES_RETURN){
	char* retMsg;
	char* token;
 	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
	sprintf((char*)simBufCommand, "AT+%s=%s\r\n", commandCode, params);
	for(; retriesCount > 0; --retriesCount){
		retMsg = simTxATCommand(simBufCommand, strlen(simBufCommand));
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
		if(strcmp((const char*)token, (const char*)SUCCES_RETURN)){
			copyStr(simBufError, token, COMMAND_BUF_SZ);
			sprintf((char*)simBufError + strlen(token), "\r\n");
			if(strcmp((const char*)simBufError, (const char*)simBufCommand) == 0){
				return SIM_RESTART;
			}
			D(printf("ERROR:   %s %s return: %s\r\n", commandCode, params, token));
		} else{
			D(printf("OK:   %s %s return: %s\r\n", commandCode, params, token));
			return SIM_SUCCESS;
		}
	}
	return SIM_FAIL;
}

void copyStr(char* dist, char* source, u16 distSz){
	memset(dist, '\0', distSz);
	sprintf(dist, "%s", source);
}

u8 simGetDateTime(char* timestamp){
	char* retMsg;
	simHttpInit(urls.getTime);
	u8 retStatus = httpGet(&retMsg, 10, 10000);
	memcpy(timestamp, retMsg + 13, 10);
	D(printf("%s\r\n", retMsg));
	return retStatus;
}

void simHttpInit(char* url){
	u8 resCode;
	httpDeInit();
	for(u8 i = 0; i < 3; i++){
		if((resCode = httpInit(url, 2)) != SIM_SUCCESS){
			D(printf("ERROR: httpInit()\r\n"));
			httpDeInit();
			if(resCode == SIM_RESTART) 
				simReset();
		} else{
			D(printf("OK: simHttpInit()\r\n"));
			break;
		}
	}
}

u8 httpInit(char* httpAddr, u8 retriesCount){
	char* retMsg;
	char* token;
	for(; retriesCount > 0; --retriesCount){
		retMsg = simExecCommand(INIT_HTTP_SERVICE);
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') 
			token = SIM_NO_RESPONSE_TEXT;
		if(strcmp((const char*)token, (const char*)SIM_OK_TEXT)){
			copyStr(simBufError, token, RESPONSE_BUF_SZ);
			if(strcmp((const char*)token, "AT+HTTPINIT") == 0 ||
				strcmp((const char*)token, SIM_NO_RESPONSE_TEXT) == 0){
				return SIM_RESTART;
			}
			D(printf("ERROR: INIT_HTTP_SERVICE %s\r\n", token));
			osDelay(300);
			httpDeInit();
		} else {
			break;
		}
	}
	if(!retriesCount)
		return SIM_FAIL;

	memset(tmpSimBuf, '\0', COMMAND_BUF_SZ);
	sprintf((char*)tmpSimBuf,"\"%s\",\"%s\"", URL_HTTP_PARAMETER, httpAddr);
	if(httpWriteCommand(SET_HTTP_PARAMETERS_VALUE, tmpSimBuf, retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}

	memset(tmpSimBuf, '\0', COMMAND_BUF_SZ);
	if(isJson)sprintf((char*)tmpSimBuf,"\"%s\",\"application/json\"", CONTENT_HTTP_PARAMETER);
	else sprintf((char*)tmpSimBuf,"\"%s\",\"text/plain\"", CONTENT_HTTP_PARAMETER);
	if(httpWriteCommand(SET_HTTP_PARAMETERS_VALUE, tmpSimBuf, retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	return SIM_SUCCESS;
}

void httpDeInit(){
	char* retMsg =  simExecCommand(TERM_HTTP_SERVICE);
	D(printf("DEINIT: %s\r\n", retMsg));
}

char* simExecCommand(char* httpCommand){
	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
	sprintf((char*)simBufCommand, "AT+%s\r\n", httpCommand);
	return simTxATCommand(simBufCommand, strlen(simBufCommand));
}

u8 httpGet(char** pRxData, u8 retriesCount, u32 httpTimeout){
	if(httpWriteCommand(HTTP_METHOD_ACTION, "0", retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	if(uInfoSim.pRxBuf[6] == '\0')
		waitIdle("+HTTPACTION", &uInfoSim.irqFlags, 200, USART_TIMEOUT);
	if(uInfoSim.pRxBuf[6] == '\0'){
		D(printf("ERROR: +HTTPACTION no resp\r\n"));
		// sprintf(bufResponse, "+HTTPACTION no resp\r\n");
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
	}else if(atoi((char*)uInfoSim.pRxBuf + 23) == HTTP_CODE_OK){
		D(printf("OK: HTTP_CODE_OK\r\n"));
		return httpRead(pRxData);
	}else{
		D(printf("%s\r\n", uInfoSim.pRxBuf));
		// sprintf(bufResponse, "HTTP_CODE_OK: %d\r\n", atoi((char*)gsmUartInfo.rxBuffer + 23));
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
	}
	return SIM_FAIL;
}

u8 httpRead(char** pRxData){
	char* token;
	u16 httpCode;
	simExecCommand(READ_THE_HTTP_SERVER_RESPONSE);
	osDelay(100);
//	waitIdle("waitIdle status httpRead", &gsmUartInfo.irqFlags);
	if(uInfoSim.pRxBuf[0] == '\0'){
		D(printf("ERROR: HTTPREAD sim no resp\r\n"));
//		createLog(logError, LOG_SZ_ERROR, "HTTPREAD sim no resp\r\n");
		return SIM_FAIL;
	} else
		token = strtok((char*)uInfoSim.pRxBuf, SIM_SEPARATOR_TEXT);

//	waitSim("wait HTTPREAD resp server");
	if(token == NULL || token[0] == '\0')
		return SIM_HTTP_BAD_CODE_REQUEST;
	else if(uInfoSim.pRxBuf[strlen(token) + 4] == '\0'){
		D(printf("ERROR: HTTPREAD server no response\r\n"));
//		createLog(logError, LOG_SZ_ERROR, "HTTPREAD server no resp\r\n");
		return SIM_HTTP_BAD_CODE_REQUEST;
	} else
		token = strtok((char*)uInfoSim.pRxBuf + strlen(token) + 4, SIM_SEPARATOR_TEXT);

		//	httpWriteCommand(READ_THE_HTTP_SERVER_RESPONSE, "0,39", 3, SIM_OK);
	if(token == NULL || token[0] == '\0'){
		D(printf("ERROR: httpRead() token NULL\r\n"));
//		token = SIM_NO_RESPONSE_TEXT;
		return SIM_FAIL;
	}
	/*copyStr(responseSIMbuf, token, sizeof(responseSIMbuf));
	*pRxData = responseSIMbuf;*/
	*pRxData = token;
	if((httpCode = atoi(token + strlen(token) - 4)) == HTTP_CODE_OK){
		D(printf("status POST: 200\r\n"));
		return SIM_SUCCESS;
	}else{
		D(printf("ERROR status POST: %d\r\n", httpCode));
//		char tmpBuf[50];
//		sprintf(tmpBuf, "HTTPREAD server code: %s\r\n", token);
//		createLog(logError, LOG_SZ_ERROR, tmpBuf);
	}
	return SIM_FAIL;
}

char* httpWriteData(char* data, u16 sz){
	return simTxATCommand(data, sz);
}

u8 httpPost(char* txData, u16 szTx, char** pRxData, u8 retriesCount, u32 httpTimeout){
	char param[15];

	sprintf(param,"%d,%d", (int)szTx, (int)httpTimeout);
	if(httpWriteCommand(INPUT_HTTP_DATA, param, retriesCount, SIM_DOWNLOAD) == SIM_FAIL){
		return SIM_FAIL;
	}

	char* retMsg = httpWriteData(txData, szTx);
	char* token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
	if(strcmp((const char*)token, (const char*)SIM_OK_TEXT)){
		D(printf("ERROR: txData %s\r\n", retMsg));
		return SIM_FAIL;
	}
	D(printf("OK: txData %s return : %s\r\n", txData, retMsg));

	if(httpWriteCommand(HTTP_METHOD_ACTION, "1", 1, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	if(uInfoSim.pRxBuf[6] == '\0')
		waitIdle("+HTTPACTION", &uInfoSim.irqFlags, 200, USART_TIMEOUT);
	if(uInfoSim.pRxBuf[6] == '\0'){
		D(printf("ERROR: +HTTPACTION no resp\r\n"));
	}else if(atoi((char*)uInfoSim.pRxBuf + 23) == HTTP_CODE_OK){
		D(printf("OK: HTTP_CODE_OK\r\n"));
		return httpRead(pRxData);
	}else{
		D(printf("%s\r\n", uInfoSim.pRxBuf + 8));
	}
	return SIM_FAIL;
}

u8 simCheckCSQ(){
	u8 csq = 0;
	char* retMsg;
	char* token;
	retMsg = simExecCommand(SIM_CSQ);  // check signal level
	token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	csq = (token != NULL) && (strlen(token) > 8) ? atoi(token + 6) : 0;
	return csq;
}

u8 simGetSzSoft(u32* szSoft){
	char*	retMsg;
	httpDeInit();
	if(httpInit(urls.getSzSoft, 2) == SIM_SUCCESS){
		D(printf("OK: httpInit(getSoft)\r\n"));
//		HAL_GPIO_TogglePin(LED2R_GPIO_Port, LED2R_Pin);
		httpGet(&retMsg, 10, 10000);
		if(atoi(retMsg) > 1000){
			*szSoft = atoi(retMsg);
			return SIM_SUCCESS;
		}
	}
	return SIM_FAIL;
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

void simReset(){
	u8 tmpSimBadResponse = 0;
	char* retMsg;
	char* token;
	D(printf("WARINIG!: R E S E T !\r\n"));
	simOff();
	simOn();
	while(1){
		simInit();
		retMsg = simTxATCommand("AT\r\n", strlen("AT\r\n"));
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
		D(printf("simInit AT: %s\r\n", token));
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

u8 testCipCmd(char* command, char* sucMsg){
	char* retMsg;
	char* token;
	retMsg = simExecCommand(command);
	token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') 
		token = SIM_NO_RESPONSE_TEXT;
	if(strcmp((const char*)token, (const char*)sucMsg)){
		D(printf("ERROR: %s %s\r\n", command, token));
		return SIM_FAIL;
	} else{
		D(printf("OK: %s %s\r\n", command, token));
	}
	return SIM_SUCCESS;
}

u8 simTCPTest(){
	//AT+CIPSHUT

	char param[40];
	if(testCipCmd(SIM_CIPSHUT, SIM_OK_CIPSHUT) == SIM_FAIL){
		return SIM_FAIL;
	}

	memset(param, '\0', 40);
	sprintf(param,"%d", (int)0);
	if(httpWriteCommand(SIM_CIPMUX, param, 3, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}

	memset(param, '\0', 40);
	sprintf(param,"%d", (int)1);
	if(httpWriteCommand(SIM_CGATT, param, 3, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	
	if(testCipCmd(SIM_CIPSTATUS, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	osDelay(300);

	memset(param, '\0', 40);
	sprintf(param,"\"%s\",\"gdata\",\"gdata\"", (char*)"internet");
	if(httpWriteCommand(SIM_CSTT, param, 3, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}

	if(testCipCmd(SIM_CIPSTATUS, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	osDelay(300);

	if(testCipCmd(SIM_CIICR, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}

	if(testCipCmd(SIM_CIPSTATUS, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	osDelay(300);

	simExecCommand(SIM_CIFSR);

	if(testCipCmd(SIM_CIPSTATUS, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
	osDelay(300);

	memset(param, '\0', 40);
	sprintf(param,"\"%s\",\"%s\",\"%s\"", (char*)"TCP", (char*)"92.53.104.16", (char*)"40471");
	if(httpWriteCommand(SIM_CIPSTART, param, 3, SIM_OK_TEXT) == SIM_FAIL){
		return SIM_FAIL;
	}
    
	osDelay(500);


}
