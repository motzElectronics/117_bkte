/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BAT_PWR_EN_Pin GPIO_PIN_0
#define BAT_PWR_EN_GPIO_Port GPIOC
#define IS_CHRG_BAT_Pin GPIO_PIN_1
#define IS_CHRG_BAT_GPIO_Port GPIOC
#define SIM_PWR_EN_Pin GPIO_PIN_2
#define SIM_PWR_EN_GPIO_Port GPIOC
#define ADC_BAT_Pin GPIO_PIN_0
#define ADC_BAT_GPIO_Port GPIOA
#define USART2_RE_Pin GPIO_PIN_1
#define USART2_RE_GPIO_Port GPIOA
#define SPI1_CS_LORA_Pin GPIO_PIN_4
#define SPI1_CS_LORA_GPIO_Port GPIOA
#define IRQ_LORA_Pin GPIO_PIN_4
#define IRQ_LORA_GPIO_Port GPIOC
#define IRQ_LORA_EXTI_IRQn EXTI4_IRQn
#define PWR_STATE_Pin GPIO_PIN_1
#define PWR_STATE_GPIO_Port GPIOB
#define PWR_STATE_EXTI_IRQn EXTI1_IRQn
#define ONEWIRE_3_EN_Pin GPIO_PIN_8
#define ONEWIRE_3_EN_GPIO_Port GPIOE
#define ONEWIRE_2_EN_Pin GPIO_PIN_9
#define ONEWIRE_2_EN_GPIO_Port GPIOE
#define ONEWIRE_4_EN_Pin GPIO_PIN_10
#define ONEWIRE_4_EN_GPIO_Port GPIOE
#define ONEWIRE_1_EN_Pin GPIO_PIN_11
#define ONEWIRE_1_EN_GPIO_Port GPIOE
#define RF_PWR_Pin GPIO_PIN_10
#define RF_PWR_GPIO_Port GPIOB
#define SPI2_CS_MEM_Pin GPIO_PIN_12
#define SPI2_CS_MEM_GPIO_Port GPIOB
#define GSM_PWRKEY_Pin GPIO_PIN_13
#define GSM_PWRKEY_GPIO_Port GPIOD
#define UART1_RE_Pin GPIO_PIN_8
#define UART1_RE_GPIO_Port GPIOA
#define SD_PWR_EN_Pin GPIO_PIN_0
#define SD_PWR_EN_GPIO_Port GPIOD
#define SD_DETECTED_Pin GPIO_PIN_1
#define SD_DETECTED_GPIO_Port GPIOD
#define LED1G_Pin GPIO_PIN_3
#define LED1G_GPIO_Port GPIOD
#define LED3G_Pin GPIO_PIN_4
#define LED3G_GPIO_Port GPIOD
#define LED4R_Pin GPIO_PIN_5
#define LED4R_GPIO_Port GPIOD
#define LED4G_Pin GPIO_PIN_6
#define LED4G_GPIO_Port GPIOD
#define LED2R_Pin GPIO_PIN_7
#define LED2R_GPIO_Port GPIOD
#define LED2G_Pin GPIO_PIN_0
#define LED2G_GPIO_Port GPIOE
#define LED1R_Pin GPIO_PIN_1
#define LED1R_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define DEBUG 0

#if DEBUG
  #define D(x)  x
#else
  #define D(x)
#endif

#define LOG_SZ_ERROR	100
#define WAIT_TIMEOUT 	15000
#define DUMMY_BYTE		0xFF

#define URL_FILE_SZ							(char*)"http://188.242.176.25:8080/api/filesize?uid="
#define URL_TIME							(char*)"http://188.242.176.25:8080/api/time"
#define URL_GET_NEW_FIRMWARE				(char*)"http://188.242.176.25:8080/api/getFile"
#define URL_MEASURE							(char*)"http://188.242.176.25:8080/api/add/measures"

extern char logError[LOG_SZ_ERROR]; 
typedef uint8_t			u8;
typedef uint16_t		u16;
typedef uint32_t		u32;
typedef uint64_t		u64;
typedef unsigned int 	uint;

typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;


typedef union{
	struct{
		u8 isIrqTx:		1;
		u8 isIrqRx: 	1;
		u8 isIrqIdle:	1;
	};
	u8 regIrq;
}IrqFlags;

typedef struct{
	u8 hour;
	u8 min;
	u8 sec;
	u8 year;
	u8 month;
	u8 day;
}DateTime;

typedef struct{
	char* addMeasure;
	char* getTime;
	char* getSzSoft;
	char* getPartFirmware;
}HttpUrl;

typedef struct{
	u64		header;
	u8		numFirmware;
	char	verFirmware;
}FIRMWARE_INFO;

extern HttpUrl urls;

u8 waitRx(char* waitStr, IrqFlags* pFlags, u16 pause, u16 timeout);
u8 waitTx(char* waitStr, IrqFlags* pFlags, u16 pause, u16 timeout);
u8 waitIdle(char* waitStr, IrqFlags* pFlags, u16 pause, u16 timeout);

void urlsInit();
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
