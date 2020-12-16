#include "../Tasks/Inc/task_web_exchange.h"
extern osThreadId webExchangeHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId createWebPckgHandle;
extern osMutexId mutexWebHandle;
extern osMessageQId queueWebPckgHandle;

static WebPckg* curPckg = NULL;

void taskWebExchange(void const * argument){
    // vTaskSuspend(webExchangeHandle);    
	u8 statSend = SEND_OK;
	offAllLeds();
	vTaskSuspend(webExchangeHandle);
	// vTaskSuspend(webExchangeHandle);

	for(;;){
		if(statSend == SEND_OK || statSend == SEND_TCP_ER_LOST_PCKG){
			if(curPckg != NULL){
				freeWebPckg(curPckg);
			}
			xQueueReceive(queueWebPckgHandle, &curPckg, portMAX_DELAY); 
		}
		xSemaphoreTake(mutexWebHandle, portMAX_DELAY);
		while((statSend = openSendTcp(curPckg->buf, curPckg->shift)) != SEND_OK && statSend != SEND_TCP_ER_LOST_PCKG);
		statSend = fastSendTcp(statSend);
		xSemaphoreGive(mutexWebHandle);
		
	}
}

u8 fastSendTcp(u8 statSend){ //while open tcp connection
	static portBASE_TYPE xStatus;
	while(statSend == SEND_OK){
		freeWebPckg(curPckg);
		if((xStatus = xQueueReceive(queueWebPckgHandle, &curPckg, 100)) == pdPASS){
			D(printf("OK: fastSend\r\n"));
			statSend = simTCPSend(curPckg->buf, curPckg->shift);
		}
	}
	return statSend;
}
