#include "../Tasks/Inc/task_lora.h"

static SX1272 node;
static Config_Group config;
extern osThreadId loraHandle;
LoraPckg loraPckgTx = {.data.numTrain = BKTE_ID_TRAIN};
static LoraPckg loraPckgRx;
void taskLora(void const * argument){
	//   vTaskSuspend(loraHandle);
	config.CarrierFreq = 0;

	node.hspi = &hspi1;
	node.config = &config;

	vTaskSuspend(loraHandle);

	HAL_GPIO_WritePin(RF_PWR_GPIO_Port, RF_PWR_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_SET);
	
	sx1272_lora_init(&node);

	for(;;){
#if(LR_IS_TRANSMITTER)
		loraPckgTx.data.addrLora.addrRx = loraPckgTx.data.addrLora.addrRx < LORA_MAX_NUM_RECEIVER ? loraPckgTx.data.addrLora.addrRx + 1 : 1;
		loraPckgTx.data.addrLora.addrTx = LORA_NUM_MASTER;
		loraPckgTx.data.addrLora.addrCurTx = LORA_NUM_MASTER;
		loraPckgTx.data.rssiFromRemoteLora = 0xCC; // test data
		loraPckgTx.crc = calcCrc16((u8*)(&(loraPckgTx.data)), PAYLOAD_LENGTH);
		sx1272_send((u8*)&loraPckgTx);
		HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_RESET);
		do{
			sx1272_receive(&loraPckgRx);
		}while(loraPckgRx.data.rssiFromRemoteLora && (loraPckgRx.data.addrLora.addrCurTx != LORA_NUM_MASTER + 1 || 
			loraPckgRx.data.addrLora.addrRx != LORA_NUM_MASTER || loraPckgRx.data.numTrain != BKTE_ID_TRAIN));
		
		if(loraPckgRx.data.rssiFromRemoteLora){
			HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_RESET);
		}
		osDelay(1000);
		HAL_GPIO_WritePin(LED1R_GPIO_Port, LED1R_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_SET);
		osDelay(500);
#else
		if(sx1272_receive(&loraPckgRx) == LR_STAT_OK && loraPckgRx.data.numTrain == BKTE_ID_TRAIN){
			if((loraPckgRx.data.addrLora.addrCurTx == LORA_NUM_RECEIVER - 1 && loraPckgRx.data.addrLora.addrRx >= LORA_NUM_RECEIVER) || 
			(loraPckgRx.data.addrLora.addrCurTx == LORA_NUM_RECEIVER + 1 && loraPckgRx.data.addrLora.addrRx <= LORA_NUM_RECEIVER)){
				if(loraPckgRx.data.addrLora.addrRx == LORA_NUM_RECEIVER){
					loraPckgTx.data.addrLora.addrCurTx = LORA_NUM_RECEIVER;
					loraPckgTx.data.addrLora.addrTx = LORA_NUM_RECEIVER;
					loraPckgTx.data.addrLora.addrRx = loraPckgRx.data.addrLora.addrTx;
				} else {
					loraPckgTx.data.addrLora.addrCurTx = LORA_NUM_RECEIVER;
					loraPckgTx.data.addrLora.addrTx = loraPckgRx.data.addrLora.addrTx;
					loraPckgTx.data.addrLora.addrRx = loraPckgRx.data.addrLora.addrRx;
				}
				HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_RESET);
				
				loraPckgTx.crc = calcCrc16((u8*)(&(loraPckgTx.data)), PAYLOAD_LENGTH);
				osDelay(800);
				sx1272_send((u8*)&loraPckgTx);
				// osDelay(500);
				HAL_GPIO_WritePin(LED1G_GPIO_Port, LED1G_Pin, GPIO_PIN_SET);
			}
		}
#endif
	}

}