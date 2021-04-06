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

u8 isJson = 1;

void simInit(){
	char* retMsg;
	char* token;
	u8 fail = 0; 
	u8 simBadAnsw = 0;
	u8 isInit = 0;

    bkte.isTCPOpen = 0;

	while(!isInit){
		simHardwareReset();
		simTxATCmd(SIM_CMD_ATE0, SIM_SZ_CMD_ATE0, 1000);
		retMsg = simTxATCmd(SIM_CMD_AT, SIM_SZ_CMD_AT, 1000);
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
		} else {
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
    
	D(printf("OK: simInit()\r\n"));
}

char* simGetStatusAnsw(u32 timeout){
	waitIdle("", &(uInfoSim.irqFlags), 200, timeout);
	if(uInfoSim.irqFlags.isIrqIdle){
		return (char*)uInfoSim.pRxBuf;
	} else{
		return SIM_NO_RESPONSE_TEXT;
	}

}


char* simTxATCmd(char* command, u16 sz, u32 timeout){
	uartTx(command, sz, &uInfoSim);
	return simGetStatusAnsw(timeout);
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
		retMsg = simTxATCmd(simBufCmd, strlen(simBufCmd), 20000);
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
			// D(printf("OK: %s ret: %s\r\n", simBufCmd, token));
			return SIM_SUCCESS;
		}
	}
	return SIM_FAIL;
}

void copyStr(char* dist, char* source, u16 distSz){
	memset(dist, '\0', distSz);
	sprintf(dist, "%s", source);
}


char* simDownloadData(char* data, u16 sz){
	return simTxATCmd(data, sz, 40000);
}

u8 simCheckCSQ(){
	u8 csq = 0;
	char* retMsg;
	char* token;

	retMsg = simTxATCmd("AT+CSQ\r\n", 8, 1000);  // check signal level
	token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	csq = (token != NULL) && (strlen(token) > 8) ? atoi(token + 6) : 0;
    bkte.csq = csq;
	return csq;
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
	static char params[40];
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
	static char params[8];
	char* token;
	char* retMsg;

    u32 ttt = HAL_GetTick();

	if(sz == 0){
		D(printf("ERROR SZ\r\n"));
		return SIM_FAIL;
	}
	D(printf("simDownloadData() sz:%d\r\n", sz));
	memset(params, '\0', 8);
	sprintf(params,"%d", sz);
	if(simCmd(SIM_CIPSEND, params, 1, "> ") == SIM_FAIL){
		sdWriteLog(SD_ER_CIPSEND, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	}
	retMsg = simDownloadData(data, sz);
	token = strtok(retMsg, SIM_SEPARATOR_TEXT);
	if(token == NULL || token[0] == '\0') 
			token = SIM_NO_RESPONSE_TEXT;

    ttt = HAL_GetTick() - ttt;

	if(strcmp((const char*)token, (const char*)"SEND OK") != 0){
		D(printf("ER: simDownloadData() %s time %d\r\n", token, ttt));
		sdWriteLog(SD_ER_DOWNLOAD_DATA_AND_SEND_OK, SD_LEN_ER_MSG, NULL, 0, &sdSectorLogError);
		return SIM_FAIL;
	} else{
		D(printf("OK: simTCPSend() time %d\r\n", ttt));
	}
	return SIM_SUCCESS;
}

long long simGetPhoneNum() {
	char* retMsg;
	retMsg = simTxATCmd(SIM_CMD_CNUM, SIM_SZ_CMD_CNUM, 2000);
        if(retMsg[0] != '\0') {
            printf("Num is %s\r\n", retMsg);
          return atoll(retMsg + 15);
        } else
          return 0; 
}

u8 procReturnStatus(u8 ret) {
	static u8 notSend = 0;
	if(ret != TCP_OK){
		notSend++;
		HAL_GPIO_WritePin(LED2R_GPIO_Port, LED2R_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED2G_GPIO_Port, LED2G_Pin, GPIO_PIN_SET);
	} else {
		notSend = 0;
		HAL_GPIO_WritePin(LED2R_GPIO_Port, LED2R_Pin, GPIO_PIN_SET);
		HAL_GPIO_TogglePin(LED2G_GPIO_Port, LED2G_Pin);
	}

	if (notSend == 1) {
		simReset();
		D(printf("DANGER DANGER HIGH VOLTAGE\r\n"));
		ret = TCP_SEND_ER_LOST_PCKG;
		notSend = 0;
	}

	return ret;
}

u8 openTcp() {
	u8 ret = TCP_OK;
	if (!waitGoodCsq(5400)) {
        D(printf("ER: waitGoodCsq\r\n"));
		ret = TCP_CSQ_ER;
    }
	if(ret == TCP_OK && simTCPinit() != SIM_SUCCESS) {
		D(printf("ER: simTCPinit\r\n"));
		ret = TCP_INIT_ER;
	}
	if(ret == TCP_OK && simTCPOpen() != SIM_SUCCESS) {
		D(printf("ER: simTCPOpen\r\n"));
		ret = TCP_OPEN_ER;
	}
    if (ret == TCP_OK) {
        bkte.isTCPOpen = 1;
    }
	return procReturnStatus(ret);

}

u8 sendTcp(u8* data, u16 sz) {
	u8 ret = TCP_OK;
    // if (!bkte.isTCPOpen) {
    //     return TCP_SEND_ER;
    // }
	if (!waitGoodCsq(60)) {
        D(printf("ER: waitGoodCsq\r\n"));
		ret = TCP_CSQ_ER;
    }
	if(ret == TCP_OK && simTCPSend(data, sz) != SIM_SUCCESS) {
		D(printf("ER: simTCPSend\r\n"));
		ret = TCP_SEND_ER;
	}
	return procReturnStatus(ret);
}