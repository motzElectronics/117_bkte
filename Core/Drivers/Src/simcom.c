/*
 * sim800.c
 *
 *  Created on: Feb 20, 2020
 *      Author: bls
 */

#include "../Drivers/Inc/simcom.h"

// //#define PEER_CLOSED 1 // (u8*)"http://188.242.176.25:8080/api/add/measures";
// #define PEER_CLOSED 1 // (u8*)"http://ptsv2.com/t/rt98e-1582717549/post";

static char simBufCmd[COMMAND_BUF_SZ];
static char simBufError[COMMAND_BUF_SZ];
static char tmpSimBuf[COMMAND_BUF_SZ];

u8 isJson = 1;

void simInit(){
	char* retMsg;
	char* token;
	u8 fail = 0; 
	u8 simBadAnsw;
	u8 isInit = 0;
	while(!isInit){
		simHardwareReset();
		simTxATCmd("ATE0\r\n", strlen("ATE0\r\n"));
		retMsg = simTxATCmd("AT\r\n", strlen("AT\r\n"));
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
		D(printf("simInit AT: %s\r\n", token));
		if((strcmp(token, SIM_OK_TEXT)) != 0){
			sdWriteLog(SD_ER_MSG_AT, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
			bkte.erFlags.simAT = 1;
			D(printf("ERROR: simInit() AT: %s\r\n", token));
			simBadAnsw = (simBadAnsw + 1) % 10;
			if(!simBadAnsw){
				sdWriteLog(SD_MSG_MCU_RESET, SD_LEN_MCU_RESET, NULL, 0, &sdSectorLogs);
				D(printf("WARINTING!: T O T A L  R E S E T\r\n"));
				sdUpdLog(&sdSectorLogError);
				sdUpdLog(&sdSectorLogs);
		//				  createLog(logError, LOG_SZ_ERROR, "ERROR: TOTAL RESET \r\n");
				osDelay(3000);
				HAL_NVIC_SystemReset();
			}
		} else{
			bkte.erFlags.simAT = 0;
			osDelay(5000);
			if(SIM_GPS_INIT() != SIM_SUCCESS){ 
				fail++;
				bkte.erFlags.simSAPBR = 1;
				if(fail > 10){
					sdWriteLog(SD_ER_SAPBR, SD_LEN_ER_MSG, "1", 1, &sdSectorLogError);
					sdUpdLog(&sdSectorLogError);
					sdUpdLog(&sdSectorLogs);
					fail = 0;  
					HAL_NVIC_SystemReset();
				}
				sdWriteLog(SD_ER_SAPBR, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);      
				D(printf("ERROR: NOT CONNECT GPS\r\n"));
			} else {
				isInit = 1;
				bkte.erFlags.simSAPBR = 0;
			}
		}
	}

	// SIM_GPS_INIT();
	D(printf("OK: simInit()\r\n"));
}

char* simGetStatusAnsw(){
	waitIdle("wait simGetStatusAnsw()", &(uInfoSim.irqFlags), 200, 20000);
	if(uInfoSim.irqFlags.isIrqIdle){
		uInfoSim.irqFlags.isIrqIdle = 0;
		return (char*)uInfoSim.pRxBuf;
	} else{
		return SIM_NO_RESPONSE_TEXT;
	}

}

/*void simWriteCommand(char* command){
	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
	sprintf(simBufCommand, "%s\r\n", command);
	uartTx(simBufCommand, strlen(simBufCommand), &uInfoSim);
}*/

char* simTxATCmd(char* command, u16 sz){
	uartTx(command, sz, &uInfoSim);
	return simGetStatusAnsw();
}

u8 simCmd(char* cmdCode, char* params, u8 retriesCnt, char* SUCCESS_RET){
	char* retMsg;
	char* token;
 	memset(simBufCmd, '\0', COMMAND_BUF_SZ);
	if(params == NULL)
		sprintf((char*)simBufCmd, "AT+%s\r\n", cmdCode);
	else{
		sprintf((char*)simBufCmd, "AT+%s=%s\r\n", cmdCode, params);
	}
	for(; retriesCnt > 0; --retriesCnt){
		retMsg = simTxATCmd(simBufCmd, strlen(simBufCmd));
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
		if(SUCCESS_RET != NULL && strcmp((const char*)token, (const char*)SUCCESS_RET)){
			copyStr(simBufError, token, COMMAND_BUF_SZ);
			sprintf((char*)simBufError + strlen(token), "\r\n");
			if(strcmp((const char*)simBufError, (const char*)simBufCmd) == 0){
				return SIM_RESTART;
			}
			D(printf("ERROR: %s ret: %s\r\n", simBufCmd, token));
		} else{
			D(printf("OK: %s ret: %s\r\n", simBufCmd, token));
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
/*	simHttpInit(urls.getTime);
	u8 retStatus = httpGet(&retMsg, 10, 10000);
	memcpy(timestamp, retMsg + 13, 10);
	D(printf("%s\r\n", retMsg));
	return retStatus;*/
}

void simHttpInit(char* url){
/*	u8 resCode;
	httpDeInit();
	for(u8 i = 0; i < 3; i++){
		if((resCode = httpInit(url, 2)) != SIM_SUCCESS){
			D(printf("ERROR: httpInit()\r\n"));
			sdWriteLog(SD_ER_MSG_HTTPINIT_MYFUN, SD_LEN_HTTP, url, strlen(url), &sdSectorLogError);
			httpDeInit();
			if(resCode == SIM_RESTART) 
				simReset();
		} else{
			D(printf("OK: simHttpInit()\r\n"));
			break;
		}
	}*/
}

/*u8 httpInit(char* httpAddr, u8 retriesCount){
	char* retMsg;
	char* token;
	for(; retriesCount > 0; --retriesCount){
		retMsg = simExecCommand(INIT_HTTP_SERVICE);
		token = strtok(retMsg, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') 
			token = SIM_NO_RESPONSE_TEXT;
		if(strcmp((const char*)token, (const char*)SIM_OK_TEXT)){
			copyStr(simBufError, token, COMMAND_BUF_SZ);
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
	if(!retriesCount){
		bkte.erFlags.simHINIT = 1;
		sdWriteLog(SD_ER_HTTPINIT, SD_LEN_HTTP, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	bkte.erFlags.simHINIT = 0;

	memset(tmpSimBuf, '\0', COMMAND_BUF_SZ);
	sprintf((char*)tmpSimBuf,"\"%s\",\"%s\"", URL_HTTP_PARAMETER, httpAddr);
	if(httpWriteCommand(SET_HTTP_PARAMETERS_VALUE, tmpSimBuf, retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		sdWriteLog(SD_ER_HTTPPARA, SD_LEN_HTTP, tmpSimBuf, strlen(tmpSimBuf), &sdSectorLogError);
		bkte.erFlags.simHPARA = 1;
		return SIM_FAIL;
	}

	bkte.erFlags.simHPARA = 0;
	memset(tmpSimBuf, '\0', COMMAND_BUF_SZ);
	if(isJson)sprintf((char*)tmpSimBuf,"\"%s\",\"application/json\"", CONTENT_HTTP_PARAMETER);
	else sprintf((char*)tmpSimBuf,"\"%s\",\"text/plain\"", CONTENT_HTTP_PARAMETER);
	if(httpWriteCommand(SET_HTTP_PARAMETERS_VALUE, tmpSimBuf, retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		sdWriteLog(SD_ER_HTTPPARA, SD_LEN_HTTP, tmpSimBuf, strlen(tmpSimBuf), &sdSectorLogError);
		bkte.erFlags.simHPARA = 1;
		return SIM_FAIL;
	}
	bkte.erFlags.simHPARA = 0;
	return SIM_SUCCESS;
}*/

/*void httpDeInit(){
	char* retMsg =  simExecCommand(TERM_HTTP_SERVICE);
	D(printf("DEINIT: %s\r\n", retMsg));
}*/

/*char* simExecCommand(char* httpCommand){
	memset(simBufCommand, '\0', COMMAND_BUF_SZ);
	sprintf((char*)simBufCommand, "AT+%s\r\n", httpCommand);
	return simTxATCommand(simBufCommand, strlen(simBufCommand));
}*/

u8 httpGet(char** pRxData, u8 retriesCount, u32 httpTimeout){
	/*if(httpWriteCommand(HTTP_METHOD_ACTION, "0", retriesCount, SIM_OK_TEXT) == SIM_FAIL){
		sdWriteLog(SD_ER_HTTPACTION, SD_LEN_HTTP, "0", 1, &sdSectorLogError);
		bkte.erFlags.simHACT = 1;
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
		bkte.erFlags.simHACT = 0;
		return httpRead(pRxData);
	}else{
		D(printf("%s\r\n", uInfoSim.pRxBuf));
		// sprintf(bufResponse, "HTTP_CODE_OK: %d\r\n", atoi((char*)gsmUartInfo.rxBuffer + 23));
//		createLog(logError, LOG_SZ_ERROR, bufResponse);
	}
	bkte.erFlags.simHACT = 1;
	sdWriteLog(SD_ER_MSG_HTTPGET_MYFUN, SD_LEN_MYFUN, NULL, 0, &sdSectorLogError);*/
	return SIM_FAIL;
}

u8 httpRead(char** pRxData){
	char* token;
	u16 httpCode;
// 	simExecCommand(READ_THE_HTTP_SERVER_RESPONSE);
// 	osDelay(100);
// //	waitIdle("waitIdle status httpRead", &gsmUartInfo.irqFlags);
// 	if(uInfoSim.pRxBuf[0] == '\0'){
// 		sdWriteLog(SD_ER_HTTPREAD, SD_LEN_HTTP, NULL, 0, &sdSectorLogError);
// 		bkte.erFlags.simHREAD = 1;
// 		D(printf("ERROR: HTTPREAD sim no resp\r\n"));
// //		createLog(logError, LOG_SZ_ERROR, "HTTPREAD sim no resp\r\n");
// 		return SIM_FAIL;
// 	} else
// 		token = strtok((char*)uInfoSim.pRxBuf, SIM_SEPARATOR_TEXT);

// //	waitSim("wait HTTPREAD resp server");
// 	if(token == NULL || token[0] == '\0'){
// 		sdWriteLog(SD_ER_HTTPREAD, SD_LEN_HTTP, "1", 1, &sdSectorLogError);
// 		bkte.erFlags.simHREAD = 1;
// 		return SIM_HTTP_BAD_CODE_REQUEST;
// 	}
// 	else if(uInfoSim.pRxBuf[strlen(token) + 4] == '\0'){
// 		sdWriteLog(SD_ER_HTTPREAD, SD_LEN_HTTP, "2", 1, &sdSectorLogError);
// 		bkte.erFlags.simHREAD = 1;
// 		D(printf("ERROR: HTTPREAD server no response\r\n"));
// //		createLog(logError, LOG_SZ_ERROR, "HTTPREAD server no resp\r\n");
// 		return SIM_HTTP_BAD_CODE_REQUEST;
// 	} else
// 		token = strtok((char*)uInfoSim.pRxBuf + strlen(token) + 4, SIM_SEPARATOR_TEXT);

// 		//	httpWriteCommand(READ_THE_HTTP_SERVER_RESPONSE, "0,39", 3, SIM_OK);
// 	if(token == NULL || token[0] == '\0'){
// 		bkte.erFlags.simHREAD = 1;
// 		sdWriteLog(SD_ER_HTTPREAD, SD_LEN_HTTP, "3", 1, &sdSectorLogError);
// 		D(printf("ERROR: httpRead() token NULL\r\n"));
// //		token = SIM_NO_RESPONSE_TEXT;
// 		return SIM_FAIL;
// 	}
// 	/*copyStr(responseSIMbuf, token, sizeof(responseSIMbuf));
// 	*pRxData = responseSIMbuf;*/
// 	*pRxData = token;
// 	bkte.erFlags.simHREAD = 0;
// 	if((httpCode = atoi(token + strlen(token) - 4)) == HTTP_CODE_OK){
// 		bkte.erFlags.simHCODE = 0;
// 		D(printf("status POST: 200\r\n"));
// 		return SIM_SUCCESS;
// 	}else{
// 		bkte.erFlags.simHCODE = 1;
// 		D(printf("ERROR status POST: %d\r\n", httpCode));
// 		sdWriteLog(SD_ER_HTTPCODE, SD_LEN_HTTP, token, strlen(token), &sdSectorLogError);
// //		char tmpBuf[50];
// //		sprintf(tmpBuf, "HTTPREAD server code: %s\r\n", token);
// //		createLog(logError, LOG_SZ_ERROR, tmpBuf);
// 	}
	return SIM_FAIL;
}

char* simDownloadData(char* data, u16 sz){
	return simTxATCmd(data, sz);
}

u8 httpPost(char* txData, u16 szTx, char** pRxData, u8 retriesCount, u32 httpTimeout){
	/*char param[15];

	sprintf(param,"%d,%d", (int)szTx, (int)httpTimeout);
	if(httpWriteCommand(INPUT_HTTP_DATA, param, retriesCount, SIM_DOWNLOAD) == SIM_FAIL){
		sdWriteLog(SD_ER_HTTPDATA, SD_LEN_HTTP, param, strlen(param), &sdSectorLogError);
		bkte.erFlags.simHDATA = 1;
		return SIM_FAIL;
	}
	bkte.erFlags.simHDATA = 0;
	char* retMsg = httpWriteData(txData, szTx);
	char* token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') token = SIM_NO_RESPONSE_TEXT;
	if(strcmp((const char*)token, (const char*)SIM_OK_TEXT)){
		sdWriteLog(SD_ER_HTTPDATA_UART, SD_LEN_HTTP, NULL, 0, &sdSectorLogError);
		bkte.erFlags.simHDATAU = 1;
		D(printf("ERROR: txData %s\r\n", retMsg));
		return SIM_FAIL;
	}
	D(printf("OK: txData %s return : %s\r\n", txData, retMsg));
	bkte.erFlags.simHDATAU = 0;
	if(httpWriteCommand(HTTP_METHOD_ACTION, "1", 1, SIM_OK_TEXT) == SIM_FAIL){
		sdWriteLog(SD_ER_HTTPACTION, SD_LEN_HTTP, "1", 1, &sdSectorLogError);
		bkte.erFlags.simHACT = 1;
		return SIM_FAIL;
	}
	bkte.erFlags.simHACT = 0;
	if(uInfoSim.pRxBuf[6] == '\0')
		waitIdle("+HTTPACTION", &uInfoSim.irqFlags, 200, USART_TIMEOUT);
	if(uInfoSim.pRxBuf[6] == '\0'){
		bkte.erFlags.simHACT = 1;
		D(printf("ERROR: +HTTPACTION no resp\r\n"));
	}else if(atoi((char*)uInfoSim.pRxBuf + 23) == HTTP_CODE_OK){
		D(printf("OK: HTTP_CODE_OK\r\n"));
		bkte.erFlags.simHACT = 0;
		return httpRead(pRxData);
	}else{
		D(printf("%s\r\n", uInfoSim.pRxBuf + 8));
		sdWriteLog(SD_ER_HTTPACTION, SD_LEN_HTTP, uInfoSim.pRxBuf + 8, strlen(uInfoSim.pRxBuf + 8), &sdSectorLogError);
		bkte.erFlags.simHACT = 1;
	}
	return SIM_FAIL;*/
}

u8 simCheckCSQ(){
	u8 csq = 0;
	char* retMsg;
	char* token;

	retMsg = simTxATCmd("AT+CSQ\r\n", 8);  // check signal level
	token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	csq = (token != NULL) && (strlen(token) > 8) ? atoi(token + 6) : 0;
	return csq;
}

u8 simGetSzSoft(u32* szSoft){
	char*	retMsg;
/*	httpDeInit();
	if(httpInit(urls.getSzSoft, 2) == SIM_SUCCESS){
		D(printf("OK: httpInit(getSoft)\r\n"));
//		HAL_GPIO_TogglePin(LED2R_GPIO_Port, LED2R_Pin);
		httpGet(&retMsg, 10, 10000);
		if(atoi(retMsg) > 1000){
			*szSoft = atoi(retMsg);
			return SIM_SUCCESS;
		}
	}*/
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
	osDelay(3000);
}

void simHardwareReset(){
	D(printf("WARINIG!: R E S E T !\r\n"));
	sdWriteLog(SD_MSG_SIM_RESET, SD_LEN_SIM_RESET, NULL, 0, &sdSectorLogs);
	simOff();
	simOn();
}

void simReset(){
	simInit();
}



/*u8 testCipCmd(char* command, char* sucMsg){
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
}*/

u8 simTCPCheckStatus(const char* stat, u16 timeout, u16 delay){
	char* token;
	while(timeout > delay){
		timeout -= delay;
		if(simCmd(SIM_CIPSTATUS, NULL, 3, SIM_OK_TEXT) != SIM_SUCCESS){
			return SIM_FAIL;
		}
		osDelay(delay);
		token = strtok((char*)uInfoSim.pRxBuf + 6, SIM_SEPARATOR_TEXT);
		if(token == NULL || token[0] == '\0') 
			token = SIM_NO_RESPONSE_TEXT;
		if(strcmp((const char*)token, stat) != 0){
			D(printf("ER: %s instead %s\r\n", token, stat));
		} else {
			D(printf("%s\r\n", token));
			return SIM_SUCCESS;
		}
	}
	return SIM_FAIL;
}

u8 simTCPinit(){
	char* token;
	if(simCmd(SIM_CIPSHUT, NULL, 3, SIM_OK_CIPSHUT) != SIM_SUCCESS){
		sdWriteLog(SD_ER_SAPBR, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}

	if(simCmd(SIM_CGATT, "1", 3, SIM_OK_TEXT) != SIM_SUCCESS){
		sdWriteLog(SD_ER_CGATT, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	
	if(simTCPCheckStatus(SIM_CIPSTAT_INIT, 200, 50) != SIM_SUCCESS){
		sdWriteLog(SD_ER_CIPSTAT_INIT, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}

	if(simCmd(SIM_CSTT, "\"internet\",\"gdata\",\"gdata\"", 3, SIM_OK_TEXT) == SIM_FAIL){ // maybe can delete gdata. test it.
		sdWriteLog(SD_ER_CSTT, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	if(simTCPCheckStatus(SIM_CIPSTAT_START, 200, 50) != SIM_SUCCESS){
		sdWriteLog(SD_ER_CIPSTAT_START, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}

	if(simCmd(SIM_CIICR, NULL, 3, SIM_OK_TEXT) == SIM_FAIL){
		sdWriteLog(SD_ER_CIICR, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	if(simTCPCheckStatus(SIM_CIPSTAT_GPRSACT, 200, 50) != SIM_SUCCESS){
		sdWriteLog(SD_ER_CIPSTAT_GPRSACT, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}

	if(simCmd(SIM_CIFSR, NULL, 3, NULL) == SIM_FAIL){
		sdWriteLog(SD_ER_CIFSR, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	if(simTCPCheckStatus(SIM_CIPSTAT_STATUS, 200, 50) != SIM_SUCCESS){
		sdWriteLog(SD_ER_CIPSTAT_STATUS, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	return SIM_SUCCESS;

}

u8 simTCPOpen(){
	static u8 params[40];
	char* token;
	memset(params, '\0', 40);
	sprintf(params,"\"%s\",\"%s\",%d", (char*)"TCP", urls.tcpAddr, urls.tcpPort);
	if(simCmd(SIM_CIPSTART, params, 3, SIM_OK_TEXT) == SIM_FAIL){
		sdWriteLog(SD_ER_CIPSTART_OK, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	osDelay(1500);
	token = strtok((char*)uInfoSim.pRxBuf + 6, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') 
			token = SIM_NO_RESPONSE_TEXT;
	if(strcmp((const char*)token, (const char*)"CONNECT OK") != 0){
		sdWriteLog(SD_ER_CIPSTART_CON_OK, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	if(simTCPCheckStatus(SIM_CIPSTAT_CON_OK, 7000, 200) != SIM_SUCCESS){
		sdWriteLog(SD_ER_CIPSTAT_CON_OK, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	return SIM_SUCCESS;
}

u8 simTCPSend(u8* data, u16 sz){
	static u8 params[8];
	char* token;
	char* retMsg;
	memset(params, '\0', 8);
	sprintf(params,"%d", sz);
	if(simCmd(SIM_CIPSEND, params, 3, "> ") == SIM_FAIL){
		sdWriteLog(SD_ER_CIPSEND, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	retMsg = simDownloadData(data, sz);
	token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') 
			token = SIM_NO_RESPONSE_TEXT;
	if(strcmp((const char*)token, (const char*)"SEND OK") != 0){
		D(printf("ER: simDownloadData()\r\n"));
		sdWriteLog(SD_ER_DOWNLOAD_DATA_AND_SEND_OK, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	} else{
		D(printf("OK: simTCPSend()\r\n"));
	}
	return SIM_SUCCESS;

	// D(printf("%s", retMsg));
    //     showWebPckg();


}

long long simGetPhoneNum(){
	char* retMsg;
        char* token;
	retMsg = simTxATCmd("AT+CNUM\r\n", strlen("AT+CNUM\r\n"));
        if(retMsg[0] != '\0'){
          return atoll(retMsg + 15);
        } else
          return 0; 
}

u8 procReturnStatus(u8 ret){
	static u8 notSend = 0;
	if(ret == INIT_TCP_ER || ret == OPEN_TCP_ER){
		notSend = 0;
		return ret;
	}
	if(ret == SEND_TCP_ER && notSend < 3){
		notSend++;	
	}else if(ret == SEND_TCP_ER && notSend == 3){
		notSend++;
		simReset();
	} else if(ret == SEND_TCP_ER && notSend == 4){
		notSend = 0;
		simReset();
		ret = SEND_TCP_ER_LOST_PCKG;
	} else if(ret == SEND_OK){
		notSend = 0;
	}
	return ret;
}

u8 openSendTcp(u8* data, u16 sz){
	u8 ret = SEND_OK;

	waitGoodCsq();
	if(simTCPinit() != SIM_SUCCESS){
		D(printf("ER: simTCPinit\r\n"));
		ret = INIT_TCP_ER;
	}
	if(ret == SEND_OK && simTCPOpen() != SIM_SUCCESS){
		D(printf("ER: simTCPOpen\r\n"));
		ret = OPEN_TCP_ER;
	}
	if(ret == SEND_OK && simTCPSend(data, sz) != SIM_SUCCESS){
		D(printf("ER: simTCPSend\r\n"));
		ret = SEND_TCP_ER;
	}
	return procReturnStatus(ret);
}