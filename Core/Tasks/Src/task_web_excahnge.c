#include "../Tasks/Inc/task_web_exchange.h"
extern osThreadId webExchangeHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId createWebPckgHandle;
extern osMutexId mutexWebHandle;
extern osMessageQId queueWebPckgHandle;

static WebPckg* curPckg = NULL;

void taskWebExchange(void const * argument){
    // vTaskSuspend(webExchangeHandle);    
	u8 statSend = TCP_OK;
	offAllLeds();
	vTaskSuspend(webExchangeHandle);
	// vTaskSuspend(webExchangeHandle);

	for(;;){
		if(statSend == TCP_OK || statSend == SEND_TCP_ER_LOST_PCKG){
			if(curPckg != NULL){
				clearWebPckg(curPckg);
				curPckg = NULL;
			}
			xQueueReceive(queueWebPckgHandle, &curPckg, portMAX_DELAY); 
		}
		xSemaphoreTake(mutexWebHandle, portMAX_DELAY);
		while((statSend = openSendTcp(curPckg->buf, curPckg->shift)) != TCP_OK && statSend != SEND_TCP_ER_LOST_PCKG);
		statSend = fastSendTcp(statSend);
		xSemaphoreGive(mutexWebHandle);
		
	}
}

u8 fastSendTcp(u8 statSend){ //while open tcp connection
	while(statSend == TCP_OK){
		clearWebPckg(curPckg);
		curPckg = NULL;
		if(xQueueReceive(queueWebPckgHandle, &curPckg, 100) == pdPASS){
			D(printf("OK: fastSend\r\n"));
			statSend = simTCPSend(curPckg->buf, curPckg->shift);
		} else {
			break;
		}
	}
	return statSend;
}
