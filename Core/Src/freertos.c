/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
#include "usart.h"

/* USER CODE END Variables */
osThreadId getEnergyHandle;
uint32_t getEnergyBuffer[ 256 ];
osStaticThreadDef_t getEnergyControlBlock;
osThreadId getNewBinHandle;
uint32_t getNewBinBuffer[ 256 ];
osStaticThreadDef_t getNewBinControlBlock;
osThreadId keepAliveHandle;
uint32_t keepAliveBuffer[ 256 ];
osStaticThreadDef_t keepAliveControlBlock;
osThreadId webExchangeHandle;
uint32_t webExchangeBuffer[ 256 ];
osStaticThreadDef_t webExchangeControlBlock;
osThreadId getTempHandle;
uint32_t getTempBuffer[ 256 ];
osStaticThreadDef_t getTempControlBlock;
osThreadId manageIWDGHandle;
uint32_t manageIWDGBuffer[ 128 ];
osStaticThreadDef_t manageIWDGControlBlock;
osThreadId loraHandle;
uint32_t loraBuffer[ 256 ];
osStaticThreadDef_t loraControlBlock;
osMutexId mutexWriteToEnergyBufHandle;
osStaticMutexDef_t mutexWriteToEnergyBufControlBlock;
osMutexId mutexWebHandle;
osStaticMutexDef_t mutexWebControlBlock;
osMutexId mutexRTCHandle;
osStaticMutexDef_t mutexRTCControlBlock;
osMutexId mutexSDHandle;
osStaticMutexDef_t mutexSDControlBlock;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void taskGetEnergy(void const * argument);
void taskGetNewBin(void const * argument);
void taskKeepAlive(void const * argument);
void taskWebExchange(void const * argument);
void taskGetTemp(void const * argument);
void taskManageIWDG(void const * argument);
void taskLora(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
  
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}                   
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of mutexWriteToEnergyBuf */
  osMutexStaticDef(mutexWriteToEnergyBuf, &mutexWriteToEnergyBufControlBlock);
  mutexWriteToEnergyBufHandle = osMutexCreate(osMutex(mutexWriteToEnergyBuf));

  /* definition and creation of mutexWeb */
  osMutexStaticDef(mutexWeb, &mutexWebControlBlock);
  mutexWebHandle = osMutexCreate(osMutex(mutexWeb));

  /* definition and creation of mutexRTC */
  osMutexStaticDef(mutexRTC, &mutexRTCControlBlock);
  mutexRTCHandle = osMutexCreate(osMutex(mutexRTC));

  /* definition and creation of mutexSD */
  osMutexStaticDef(mutexSD, &mutexSDControlBlock);
  mutexSDHandle = osMutexCreate(osMutex(mutexSD));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of getEnergy */
  osThreadStaticDef(getEnergy, taskGetEnergy, osPriorityNormal, 0, 256, getEnergyBuffer, &getEnergyControlBlock);
  getEnergyHandle = osThreadCreate(osThread(getEnergy), NULL);

  /* definition and creation of getNewBin */
  osThreadStaticDef(getNewBin, taskGetNewBin, osPriorityNormal, 0, 256, getNewBinBuffer, &getNewBinControlBlock);
  getNewBinHandle = osThreadCreate(osThread(getNewBin), NULL);

  /* definition and creation of keepAlive */
  osThreadStaticDef(keepAlive, taskKeepAlive, osPriorityNormal, 0, 256, keepAliveBuffer, &keepAliveControlBlock);
  keepAliveHandle = osThreadCreate(osThread(keepAlive), NULL);

  /* definition and creation of webExchange */
  osThreadStaticDef(webExchange, taskWebExchange, osPriorityNormal, 0, 256, webExchangeBuffer, &webExchangeControlBlock);
  webExchangeHandle = osThreadCreate(osThread(webExchange), NULL);

  /* definition and creation of getTemp */
  osThreadStaticDef(getTemp, taskGetTemp, osPriorityNormal, 0, 256, getTempBuffer, &getTempControlBlock);
  getTempHandle = osThreadCreate(osThread(getTemp), NULL);

  /* definition and creation of manageIWDG */
  osThreadStaticDef(manageIWDG, taskManageIWDG, osPriorityNormal, 0, 128, manageIWDGBuffer, &manageIWDGControlBlock);
  manageIWDGHandle = osThreadCreate(osThread(manageIWDG), NULL);

  /* definition and creation of lora */
  osThreadStaticDef(lora, taskLora, osPriorityNormal, 0, 256, loraBuffer, &loraControlBlock);
  loraHandle = osThreadCreate(osThread(lora), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_taskGetEnergy */
/**
  * @brief  Function implementing the getEnergy thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_taskGetEnergy */
__weak void taskGetEnergy(void const * argument)
{
  /* USER CODE BEGIN taskGetEnergy */
  /* Infinite loop */
  for(;;)
  {
		osDelay(50);
  }
  /* USER CODE END taskGetEnergy */
}

/* USER CODE BEGIN Header_taskGetNewBin */
/**
* @brief Function implementing the getNewBin thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskGetNewBin */
__weak void taskGetNewBin(void const * argument)
{
  /* USER CODE BEGIN taskGetNewBin */
  /* Infinite loop */
  for(;;)
  {
	osDelay(1000);
  }
  /* USER CODE END taskGetNewBin */
}

/* USER CODE BEGIN Header_taskKeepAlive */
/**
* @brief Function implementing the keepAlive thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskKeepAlive */
__weak void taskKeepAlive(void const * argument)
{
  /* USER CODE BEGIN taskKeepAlive */
  /* Infinite loop */
	for(;;)
	{
		osDelay(1);
	}
  /* USER CODE END taskKeepAlive */
}

/* USER CODE BEGIN Header_taskWebExchange */
/**
* @brief Function implementing the webExchange thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskWebExchange */
__weak void taskWebExchange(void const * argument)
{
  /* USER CODE BEGIN taskWebExchange */
  /* Infinite loop */
	for(;;)
	{
		osDelay(3000);
	}
  /* USER CODE END taskWebExchange */
}

/* USER CODE BEGIN Header_taskGetTemp */
/**
* @brief Function implementing the getTemp thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskGetTemp */
__weak void taskGetTemp(void const * argument)
{
  /* Infinite loop */
  for(;;)
  {
      osDelay(1000);
  }
  /* USER CODE END taskGetTemp */
}

/* USER CODE BEGIN Header_taskManageIWDG */
/**
* @brief Function implementing the manageIWDG thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskManageIWDG */
__weak void taskManageIWDG(void const * argument)
{
  /* USER CODE BEGIN taskManageIWDG */
  /* Infinite loop */
  for(;;)
  {
	  osDelay(1);
  }
  /* USER CODE END taskManageIWDG */
}

/* USER CODE BEGIN Header_taskLora */
/**
* @brief Function implementing the lora thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_taskLora */
__weak void taskLora(void const * argument)
{
  /* USER CODE BEGIN taskLora */

  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END taskLora */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
