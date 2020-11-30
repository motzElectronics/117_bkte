#include "../Tasks/Inc/task_web_exchange.h"
extern osThreadId webExchangeHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId createWebPckgHandle;
extern osMutexId mutexWebHandle;
extern osMessageQId queueWebPckgHandle;

static WebPckg* curPckg;

void taskWebExchange(void const * argument){
    // vTaskSuspend(webExchangeHandle);    
	u8 statSend = SEND_OK;
	offAllLeds();
	vTaskSuspend(webExchangeHandle);
	vTaskSuspend(webExchangeHandle);

	for(;;){
		if(statSend == SEND_OK || statSend == SEND_ER_LOST_PCKG){ xQueueReceive(queueWebPckgHandle, &curPckg, portMAX_DELAY); }
		while((statSend = openSendTcp(curPckg->buf, curPckg->shift)) == SEND_ER);
		statSend = fastSendTcp(statSend);
		osDelay(3000);
	}
}

u8 procReturnStatus(u8 ret){
	static u8 notSend = 0;
	if(ret == SEND_ER && notSend < 2){
		notSend++;	
	}else if(ret == SEND_ER && notSend == 2){
		notSend++;
		simReset();
	} else if(ret == SEND_ER && notSend == 3){
		notSend = 0;
		simReset();
		ret = SEND_ER_LOST_PCKG;
	} else if(ret == SEND_OK){
		notSend = 0;
	}
	return ret;
}

u8 openSendTcp(u8* data, u16 sz){
	u8 ret = SEND_OK;
	xSemaphoreTake(mutexWebHandle, portMAX_DELAY);
	waitGoodCsq();
	if(simTCPinit() != SIM_SUCCESS){
		D(printf("ER: simTCPinit\r\n"));
		ret = SEND_ER;
	}
	if(ret == SEND_OK && simTCPOpen() != SIM_SUCCESS){
		D(printf("ER: simTCPOpen\r\n"));
		ret = SEND_ER;
	}
	if(ret == SEND_OK && simTCPSend(data, sz) != SIM_SUCCESS){
		D(printf("ER: simTCPSend\r\n"));
		ret = SEND_ER;
	}
	xSemaphoreGive(mutexWebHandle);

	return procReturnStatus(ret);
}

u8 fastSendTcp(u8 statSend){ //while open tcp connection
	static portBASE_TYPE xStatus;
	while(statSend == SEND_OK && (xStatus = xQueueReceive(queueWebPckgHandle, &curPckg, 100)) == pdPASS){
		if((xStatus = xSemaphoreTake(mutexWebHandle, 100)) == pdPASS){
			D(printf("OK: fastSend\r\n"));
			statSend = simTCPSend(curPckg->buf, curPckg->shift);
			xSemaphoreGive(mutexWebHandle);
		} else {
			break;
		}
	}
	return statSend;
}
