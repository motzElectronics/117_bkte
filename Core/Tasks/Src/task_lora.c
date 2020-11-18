#include "../Tasks/Inc/task_lora.h"

static SX1272 node;
static Config_Group config;
extern osThreadId loraHandle;

LoraAlgTransition loraTransitionPckgTx;
LoraAlgTransition loraTransitionPckgRx;
static LoraTimer	lrTim;

static u8 bufData[PAYLOAD_LENGTH];
extern osSemaphoreId semLoraRxPckgHandle;

static char binaryTest[BKTE_AMOUNTS + 1];
static void strrev();
static void lcdShow(u8* str);

extern osMutexId mutexWriteToEnergyBufHandle;
static PckgEnergy tmpPckg;

LoraStatPckg loraStatPckg;

static u8 testRssi;
u32 stTime = 0;

static u8 retStatus;

void taskLora(void const * argument){
	vTaskSuspend(loraHandle);
	config.CarrierFreq = 0;
	node.hspi = &hspi1;
	node.config = &config;


	vTaskSuspend(loraHandle);
	initLoraStat(&loraStatPckg);
	HAL_GPIO_WritePin(RF_PWR_GPIO_Port, RF_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_SET);
	xSemaphoreTake(semLoraRxPckgHandle, portMAX_DELAY);
	sx1272_lora_init(&node);

	/*u8 test = 0;
	while(1){
		memset(uInfoLCD.pTxBuf, '\0', uInfoLCD.szTxBuf);
		sprintf(uInfoLCD.pTxBuf, "%d", test);
		uartTxLCD(uInfoLCD.pTxBuf, 2, &uInfoLCD);
		osDelay(300);
		test = (test + 1) % 10;
	}*/

	for(;;){
#if(BKTE_IS_LORA_MASTER)
		initTransmition();
		serializeLoraAlgTrans(bufData, &loraTransitionPckgTx);

		D(printf("OK: send R%d\r\n", loraTransitionPckgTx.loraGenInfo.flagsReq));
		stTime = HAL_GetTick();
		sx1272_send(bufData, PAYLOAD_LENGTH);
		loraStatPckg.txPckg++;
		HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_RESET);
		lrStopTim(&lrTim);
		lrStartTim(&lrTim, 0);
		while(lrTim.time < (BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR) * (LR_TASK_TIME_SLOT) * 2 &&
		((1 << ((BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR) - 1)) & loraTransitionPckgTx.loraGenInfo.flagsAnsw) == 0){
			if((retStatus = sx1272_receive(bufData, &testRssi, (BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR) * (LR_TASK_TIME_SLOT) * 2 - lrTim.time)) == LR_STAT_OK){
				HAL_GPIO_TogglePin(LED1G_GPIO_Port, LED1G_Pin);
				parceAnsw(bufData, &loraTransitionPckgRx, &loraTransitionPckgTx);
			} else if(retStatus == LR_STAT_BAD_CRC){
				fillTelemetry(&tmpPckg, TEL_LORA_BAD_CRC, 0);
				//cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckg, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
			}
			lrUpdTim(&lrTim);
		}
		updAnsw(&loraTransitionPckgTx);
		D(printf("Full time: %d\r\n", HAL_GetTick() - stTime));
		parceServicesData(&loraTransitionPckgTx);
		updStat(&loraStatPckg, &loraTransitionPckgTx);
		osDelay(200);
		HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_SET);
		osDelay(200);

#else	
		
		clearLoraAlgTrans(&loraTransitionPckgTx);
		lrStopTim(&lrTim);
		while((lrTim.time + lrTim.timeShift < BKTE_ID_TRAINCAR * LR_TASK_TIME_SLOT) &&
			((1 << (BKTE_ID_TRAINCAR - 1)) & loraTransitionPckgTx.loraGenInfo.flagsReq) == 0){
			if(((retStatus = sx1272_receive(bufData, &testRssi, (BKTE_ID_TRAINCAR * LR_TASK_TIME_SLOT - (lrTim.time + lrTim.timeShift)))) == LR_STAT_OK) &&
			(parceReq(bufData, &loraTransitionPckgRx, &loraTransitionPckgTx) == LR_TASK_PARCE_VALID_PCKG)){
				
				lrStartTim(&lrTim, loraTransitionPckgRx.loraGenInfo.flagsReq);
				HAL_GPIO_TogglePin(LED1G_GPIO_Port, LED1G_Pin);
			}else if(retStatus == LR_STAT_BAD_CRC){
				fillTelemetry(&tmpPckg, TEL_LORA_BAD_CRC, 0);
				cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckg, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
			}
			lrUpdTim(&lrTim);
		}
		D(printf("-stop recv tim-\r\n"));
		osDelay(100);
		updReq(&loraTransitionPckgTx);
		if((loraTransitionPckgTx.loraGenInfo.idTxRx & 0x0F) != BKTE_ID_TRAINCAR){	
			serializeLoraAlgTrans(bufData, &loraTransitionPckgTx);
			sx1272_send(bufData, PAYLOAD_LENGTH);
			HAL_GPIO_TogglePin(LED2G_GPIO_Port, LED2G_Pin);
		}

		lrStopTim(&lrTim);
		lrStartTim(&lrTim, 0);
		while(lrTim.time < (BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR) * LR_TASK_TIME_SLOT * 2 &&
		((1 << ((BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR) - 1)) & loraTransitionPckgTx.loraGenInfo.flagsAnsw) == 0){
			if(((retStatus = sx1272_receive(bufData, &testRssi, (BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR) * LR_TASK_TIME_SLOT * 2 - lrTim.time)) == LR_STAT_OK) &&
			(parceAnsw(bufData, &loraTransitionPckgRx, &loraTransitionPckgTx) == LR_TASK_PARCE_VALID_PCKG)){
				HAL_GPIO_TogglePin(LED1R_GPIO_Port, LED1R_Pin);
			}else if(retStatus == LR_STAT_BAD_CRC){
				fillTelemetry(&tmpPckg, TEL_LORA_BAD_CRC, 0);
				cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckg, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
			}
			lrUpdTim(&lrTim);
		}
		updAnsw(&loraTransitionPckgTx);
		serializeLoraAlgTrans(bufData, &loraTransitionPckgTx);
		sx1272_send(bufData, PAYLOAD_LENGTH);
		D(printf("-stop answ tim-\r\n"));
		D(printf(" \r\n"));
		lcdShow("     \r\n     ");
#endif
	}
}

void initTransmition(){
	loraTransitionPckgTx.loraGenInfo.idTrain = BKTE_ID_TRAIN;
	loraTransitionPckgTx.loraGenInfo.flagsAnsw = 0;
	loraTransitionPckgTx.loraGenInfo.flagsReq = 1 << BKTE_ID_TRAINCAR;
	loraTransitionPckgTx.loraGenInfo.idTxRx = (BKTE_ID_TRAINCAR << 4) | (BKTE_ID_TRAINCAR_MAX);
    //loraTransitionPckgTx.loraCarInfo[BKTE_ID_TRAINCAR].timeRcv = getTimeStamp();

	for(u8 i = 0; i < BKTE_AMOUNTS; i ++){
		loraTransitionPckgTx.loraCarInfo[i].statusInfo = 0;
		loraTransitionPckgTx.loraCarInfo[i].temperature = 0;	
	}

	if(loraStatPckg.txPckg % 3 == 0 && loraStatPckg.txPckg){
		loraTransitionPckgTx.loraGenInfo.cmd = LORA_CMD_SERVICES;
		updServices(&loraTransitionPckgTx);
	} else {
		loraTransitionPckgTx.loraGenInfo.cmd = LORA_CMD_MONITOR;
	}
	


}

u8 getLastTransmitter(u16 rvMask){
	u8 i = 0;
	memset(binaryTest, '0', BKTE_AMOUNTS);
	while(rvMask != 0){
		binaryTest[i] = ((rvMask & 0x01) == 1) ? '1' : '0'; 
		rvMask = rvMask >> 1;
		i++;
	}
	strrev();
	return i - 1;
}

void lrStartTim(LoraTimer* pTim, u16 shift){
	if(!pTim->isTimerOn){
		D(printf("-start tim-\r\n"));
		u8 i = getLastTransmitter(shift);
		pTim->isTimerOn = 1;
		pTim->timeShift = LR_TASK_TIME_SLOT * i;
		pTim->timeStart = HAL_GetTick();
	}
}

void lrUpdTim(LoraTimer* pTim){
	if(pTim->isTimerOn)
		pTim->time = HAL_GetTick() - pTim->timeStart;
}

void lrStopTim(LoraTimer* pTim){
	pTim->time = 0;
	pTim->timeShift = 0;
	pTim->isTimerOn = 0;
}

u8 parceReq(u8* pBufData, LoraAlgTransition* pRx, LoraAlgTransition* pTx){
	u8 ret = LR_TASK_PARCE_INCVALID_PCKG;
	deserializeLoraAlgTrans(pRx, pBufData);
	u8 rcv = getLastTransmitter(pRx->loraGenInfo.flagsReq);
	if(pRx->loraGenInfo.flagsReq && 
	(pRx->loraGenInfo.flagsReq < (1 << BKTE_ID_TRAINCAR)) && (pRx->loraGenInfo.flagsAnsw == 0)){
		memset(uInfoLCD.pTxBuf, '\0', uInfoLCD.szTxBuf);
		sprintf(uInfoLCD.pTxBuf, "R%d %d\r\n%s", rcv, testRssi, binaryTest);
		uartTxLCD(uInfoLCD.pTxBuf, strlen(uInfoLCD.pTxBuf), &uInfoLCD);
		D(printf("OK: LORA: RECV %s\r\n", binaryTest));
		cpyRxInf(pRx, pTx);
		if(pTx->loraGenInfo.idTxRx == 0)
			pTx->loraGenInfo.idTxRx = pRx->loraGenInfo.idTxRx;

		if(pRx->loraGenInfo.cmd == LORA_CMD_SERVICES){
			D(printf("LORA: SERVICES PCKG\r\n"));
			parceServicesData(pRx);
		}
		ret = LR_TASK_PARCE_VALID_PCKG;
	}
	return ret;
}


u8 parceAnsw(u8* pBufData, LoraAlgTransition* pRx, LoraAlgTransition* pTx){
	u8 ret = LR_TASK_PARCE_INCVALID_PCKG;
	deserializeLoraAlgTrans(pRx, pBufData);
	u8 rcv = getLastTransmitter(pRx->loraGenInfo.flagsAnsw);

	if(pRx->loraGenInfo.flagsAnsw && 
	(pRx->loraGenInfo.flagsAnsw < (1 << (BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR)))){
		memset(uInfoLCD.pTxBuf, '\0', uInfoLCD.szTxBuf);
		sprintf(uInfoLCD.pTxBuf, "A%d\r\n%s", BKTE_ID_TRAINCAR_MAX - rcv, binaryTest);
		uartTxLCD(uInfoLCD.pTxBuf, strlen(uInfoLCD.pTxBuf), &uInfoLCD);
		D(printf("OK: LORA: ANSW %s\r\n", binaryTest));
		cpyRxInf(pRx, pTx);
		ret = LR_TASK_PARCE_VALID_PCKG;
	}
	return ret;
}


void lcdShow(u8* str){
	uartTxLCD(str, strlen(str), &uInfoLCD);
}

void updReq(LoraAlgTransition* pTx){
	// pTx->loraCarInfo[BKTE_ID_TRAINCAR].timeRcv = getTimeStamp();
	pTx->loraGenInfo.flagsReq |= (1 << BKTE_ID_TRAINCAR);
	if(pTx->loraGenInfo.cmd == LORA_CMD_SERVICES){
		updServices(pTx);
	}
}

void updAnsw(LoraAlgTransition* pTx){
	// pTx->loraCarInfo[BKTE_ID_TRAINCAR].timeAnsw = getTimeStamp();
	pTx->loraGenInfo.flagsAnsw |= (1 << (BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR));
}

void serializeLoraAlgTrans(u8* pBuf, LoraAlgTransition* pTx){
	memcpy(pBuf, &pTx->loraGenInfo.cmd, 1);
	memcpy(pBuf + 1, &pTx->loraGenInfo.flagsAnsw, 2);
	memcpy(pBuf + 3, &pTx->loraGenInfo.flagsReq, 2);
	memcpy(pBuf + 5, &pTx->loraGenInfo.idTrain, 2);
	memcpy(pBuf + 7, &pTx->loraGenInfo.idTxRx, 1);

	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		/*memcpy(pBuf + 8 + i * 2, &pTx->loraCarInfo[i].timeRcv, 4);
		memcpy(pBuf + 8 + i * 2 + 4, &pTx->loraCarInfo[i].timeAnsw, 4);*/

		memcpy(pBuf + 8 + i * 2, &pTx->loraCarInfo[i].statusInfo, 1);
		memcpy(pBuf + 8 + i * 2 + 1, &pTx->loraCarInfo[i].temperature, 1);
	}
}

void deserializeLoraAlgTrans(LoraAlgTransition* pTx, u8* pBuf){
	memcpy(&pTx->loraGenInfo.cmd, pBuf, 1);
	memcpy(&pTx->loraGenInfo.flagsAnsw, pBuf + 1, 2);
	memcpy(&pTx->loraGenInfo.flagsReq, pBuf + 3, 2);
	memcpy(&pTx->loraGenInfo.idTrain, pBuf + 5, 2);
	memcpy(&pTx->loraGenInfo.idTxRx, pBuf + 7, 1);

	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		memcpy(&pTx->loraCarInfo[i].statusInfo, pBuf + 8 + i * 2, 1);
		memcpy(&pTx->loraCarInfo[i].temperature, pBuf + 8 + i * 2 + 1, 1);
	}

	/*for(u8 i = 0; i < BKTE_ID_TRAINCAR_MAX + 1; i++){
		memcpy(&pTx->loraCarInfo[i].timeRcv, pBuf + 8 + i * 2,  4);
		memcpy(&pTx->loraCarInfo[i].timeAnsw, pBuf + 8 + i * 2 + 4, 4);

		memcpy(&pTx->loraCarInfo[i].statusInfo, pBuf + 8 + i * 2 + 8, 1);
		memcpy(&pTx->loraCarInfo[i].temperature, pBuf + 8 + i * 2 + 9, 1);
	}*/
}

void clearLoraAlgTrans(LoraAlgTransition* pckg){
	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		pckg->loraCarInfo[i].statusInfo = 0;
		pckg->loraCarInfo[i].temperature = 0;
	}

	pckg->loraGenInfo.cmd = 0;
	pckg->loraGenInfo.flagsAnsw = 0;
	pckg->loraGenInfo.flagsReq = 0;
	pckg->loraGenInfo.idTrain = BKTE_ID_TRAIN;
	pckg->loraGenInfo.idTxRx = 0;
}

void strrev(){
	u8 cnt = strlen(binaryTest);
	char tmp;
    for(u8 g = 0; g < cnt/2; g++)
    {
        tmp = binaryTest[g];
        binaryTest[g]=binaryTest[cnt - 1 - g];
        binaryTest[cnt - 1 - g] = tmp;
    }

}

void initLoraStat(LoraStatPckg* pckg){
	pckg->rxPckg = 0;
	pckg->txPckg = 0;
	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		pckg->rxPckgAnsw[i] = 0;
		pckg->rxPckgRcv[i] = 0;
	}
	
}

void updStat(LoraStatPckg* pckgStat, LoraAlgTransition* pckg){
	static u8 cnt = 0;
	u8 perMaster, perRcv, perAnsw;
	cnt = (cnt + 1) % 10;
	if(pckg->loraGenInfo.flagsAnsw > (1 << (BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR)))
		pckgStat->rxPckg++;
	perMaster = (int)(pckgStat->rxPckg / (1.0 * pckgStat->txPckg) * 100);
	D(printf("STAT: gen %d \r\n", perMaster));

	if(!cnt){
		fillTelemetry(&tmpPckg, TEL_LORA_LINK_MASTER, perMaster);
		//cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckg, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
	}

	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		if(((pckg->loraGenInfo.flagsAnsw >> (BKTE_ID_TRAINCAR_MAX - i)) & 0x01))
			pckgStat->rxPckgAnsw[i]++;
		
		if(((pckg->loraGenInfo.flagsReq >>  i) & 0x01))
			pckgStat->rxPckgRcv[i]++;
		
		perRcv = (int)(pckgStat->rxPckgRcv[i] / ((float)pckgStat->txPckg) * 100);
		perAnsw = (int)(pckgStat->rxPckgAnsw[i] / ((float)pckgStat->txPckg) * 100);
		
		D(printf("STAT: %d rcv: %d ; answ: %d\r\n", i,  perRcv, perAnsw));

		if(!cnt){
			fillTelemetry(&tmpPckg, TEL_LORA_LINK_EDGE, ((i & 0xFF) << 16) | ((perRcv & 0xFF) << 8) | perAnsw & 0xFF);
			//cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckg, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
		}
	}

/*#if(DEBUG)
	D(printf("---time RCV---\r\n"));
	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		D(printf("%d) %d\r\n", i, pckg->loraCarInfo[i].timeRcv));
	}
	D(printf("\r\n"));

	D(printf("---time ANSW---\r\n"));
	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		D(printf("%d) %d\r\n", i, pckg->loraCarInfo[i].timeAnsw));
	}
	D(printf("\r\n"));
#endif*/



	fillTelemetry(&tmpPckg, TEL_LORA_FLAGS, (((pckg->loraGenInfo.flagsReq & 0xFFFF) << 16) | pckg->loraGenInfo.flagsAnsw & 0xFFFF));
	//cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckg, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
	D(printf("flags telemetry: %d\r\n", (((pckg->loraGenInfo.flagsReq & 0xFFFF) << 16) | pckg->loraGenInfo.flagsAnsw & 0xFFFF)));
	D(printf("\r\n"));
}


void parceServicesData(LoraAlgTransition* pRx){
	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		if((pRx->loraCarInfo[i].statusInfo || pRx->loraCarInfo[i].temperature) && i != BKTE_ID_TRAIN){
			loraTransitionPckgTx.loraCarInfo[BKTE_ID_TRAINCAR].statusInfo = bkte.erFlags.errReg & 0xFF;
			fillTelemetry(&tmpPckg, TEL_PERIPH_STAT, 
				(i << 16) |(pRx->loraCarInfo[i].temperature << 8) | (pRx->loraCarInfo[i].statusInfo) & 0xFF);
			//cBufSafeWrite(&circBufPckgEnergy, (u8*)&tmpPckg, SZ_PCKGENERGY, mutexWriteToEnergyBufHandle, portMAX_DELAY);
			checkBufForWritingToFlash();
		}
	}
}

void updServices(LoraAlgTransition* pTx){
	pTx->loraCarInfo[BKTE_ID_TRAINCAR].statusInfo = bkte.erFlags.errReg & 0xFF;
	pTx->loraCarInfo[BKTE_ID_TRAINCAR].temperature = (bkte.erFlags.errReg >> 8) & 0xFF;
}

void cpyRxInf(LoraAlgTransition* pRx, LoraAlgTransition* pTx){
	pTx->loraGenInfo.flagsAnsw |= pRx->loraGenInfo.flagsAnsw;
	pTx->loraGenInfo.flagsReq |= pRx->loraGenInfo.flagsReq;
	pTx->loraGenInfo.cmd = pRx->loraGenInfo.cmd;
	for(u8 i = 0; i < BKTE_AMOUNTS; i++){
		pTx->loraCarInfo[i].statusInfo = pRx->loraCarInfo[i].statusInfo;
		pTx->loraCarInfo[i].temperature = pRx->loraCarInfo[i].temperature;
	}
}