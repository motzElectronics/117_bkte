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
		if(statSend == SEND_OK || statSend == SEND_TCP_ER_LOST_PCKG){ xQueueReceive(queueWebPckgHandle, &curPckg, portMAX_DELAY); }
		while((statSend = openSendTcp(curPckg->buf, curPckg->shift)) == INIT_TCP_ER || statSend == OPEN_TCP_ER);
		statSend = fastSendTcp(statSend);
		osDelay(3000);
	}
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
	xSemaphoreTake(mutexWebHandle, portMAX_DELAY);
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
