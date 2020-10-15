/*
 * utils_bkte.h
 *
 *  Created on: Mar 16, 2020
 *      Author: bls
 */

#ifndef INC_UTILS_BKTE_H_
#define INC_UTILS_BKTE_H_

#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"

#include "fatfs.h"
#include "../Drivers/Inc/simcom.h"
#include "usart.h"
#include "time.h"
#include "stdlib.h"
#include "../Drivers/Inc/ds2482.h"
#include "../Drivers/Inc/spiflash.h"
#include "../Utils/Inc/circularBuffer.h"

//!-------------CONFIGURE PARAMS---------------
#define BKTE_ID_TRAINCAR		0
#define BKTE_ID_TRAINCAR_MAX	3
#define BKTE_IS_LORA_MASTER		1

#define BKTE_ID_FIRMWARE		3
#define BKTE_ID_TRAIN			1706
//!------------CONFIGURE PARAMS----------------

#define BKTE_ADDR_ID_MCU	0x1FFF7A10
#define BKTE_MAX_CNT_1WIRE	4
#define LEN_TIMESTAMP		13

#define BKTE_NUM_HEADER_55	11
#define BKTE_NUM_HEADER_B9	22
#define BKTE_NUM_CURRENT	12
#define BKTE_NUM_VOLT		14
#define BKTE_SZ_CURRENT		2
#define BKTE_SZ_VOLT		2
#define BKTE_NUM_ACT_ENERGY		16
#define BKTE_SZ_ENERGY			4
#define BKTE_NUM_REACT_ENERGY	27

#define BKTE_ID_DEV_BKTE		0x10
#define BKTE_ID_DEV_BKT			0x11
#define BKTE_ID_DEV_BSG			0x12

#define BKTE_SZ_UART_MSG		132
#define BKTE_SZ_TEMP_MSG		4

#define BKTE_ID_BOOT			1

#define SZ_PART_NEW_SOFTWARE	1360

#define BKTE_VER_BETA_FIRMWARE		(char)'B'
#define BKTE_VER_STABLE_FIRMWARE	(char)'S'

#define BKTE_PERCENT_DEVIATION_ENERGY_DATA 	(float)0.03
#define BKTE_ENERGY_FULL_LOOP				(u8)10

#define BKTE_PREAMBLE_EN		0xABCD
#define BKTE_PREAMBLE_EN1		0xAB
#define BKTE_PREAMBLE_EN2		0xCD

#define SZ_MAX_TX_DATA			4096

#define BKTE_MAX_TEMP                   100
#define BKTE_MIN_TEMP                   -100
#define BKTE_NO_TEMP                    -127

#define BKTE_SAVE_NUM_PAGE				2047

#define BKTE_BAD_TIMESTAMP				2997993600

#define BKTE_BIG_DIF_RTC_SERVTIME		600


typedef struct{
	u8 canMeasure;
	s8 curTemp;
}TempLine;

typedef struct{
	u32 enAct;
	u32 enReact;
	s16 current;
	u16 volt;
}EnergyData;

typedef struct{
	u32 posFile;
	u32 lenLog;
	char* fNameLog;
	char* fNameAddr;
}FInfo;

typedef union{
	struct {
		u16 isChrgBat: 	1;
		u16 isPwrState:	1;
		u16 adcVoltBat:	9; 
	};
	u16 pwrReg;
}PWRInfo;

typedef struct{
	u8	isOwActive[BKTE_MAX_CNT_1WIRE];
	u8	isFatMount;
	u32	idMCU[3];
	u16	idTrain;
	u8	idTrainCar;
	u16	idReceiver;
	u8	idDev;
	u8	idFirmware;
	u8	idBoot;
	PWRInfo pwrInfo;
//	FInfo	fInfo[NUM_READ_FILES];
}BKTE;

typedef enum{
	NUM_FILE_ENERGY = 0,
	NUM_FILE_TEMP,
	NUM_FILE_RSSI
}NUM_FILE;

typedef enum{
	TEL_OFF_DEV = 0x0010,
	TEL_ON_DEV = 0x0001,
	TEL_KEEP_ALIVE = 0x0013,
	TEL_ID_FIRMWARE = 0x1001,
	TEL_CHANGE_TIME = 0x1011,
	TEL_BIG_DIFFER_RTC_SERVERTIME = 0x1030,
	TEL_SERV_FLASH_CIRC_BUF_FULL = 0x2001,
	TEL_SERV_FLASH_CIRC_BUF_END_HEAD = 0x2002,
	TEL_SERV_FLASH_CIRC_BUF_END_TAIL = 0x2003,
	TEL_SERV_FLASH_CIRC_BUF_HALF_HEAD = 0x2004,
	TEL_SERV_FLASH_CIRC_BUF_HALF_TAIL = 0x2005,
	TEL_BAD_RESPONSE_SERVER = 0x2006,
	TEL_BAD_RTC_TIME = 0x2007,
	TEL_BAD_ALL_CRC = 0x02008,
	TEL_LVL_CSQ = 0x7010
}TYPE_TELEMETRY;

typedef enum{
	MSG_TEMP = 0xF000,
	MSG_TELEMETRY = 0x0000
}TYPE_MSG;

typedef struct{
	u16			preambule;
	EnergyData	energyData;
	DateTime 	dateTime;
	u8			crc;
}PckgEnergy;

//typedef struct{
//	u16			preambule;
//	Coord		latitude;
//	Coord		longitude;
//	u16			speed;
//	u16			cource;
//	DateTime	dateTime;
//	u8			crc;
//}PckgGnss;

typedef struct{
	s8 temp[BKTE_MAX_CNT_1WIRE];
	DateTime dateTime;
}PckgTemp;


typedef struct{
	u32 fromByte;
	u32 toByte;
}PckgUpdFirmware;


void bkteInit();
void getMaxNumDS1820(BKTE* pBkte);
void resetTempLine(u8 numLine);
void setTempLine(u8 numLine);
void fillPckgEnergy(PckgEnergy* pckg, u16* data);
void fillTempPckgEnergy(PckgEnergy* pckg, s8* data);
void fillTelemetry(PckgEnergy* pckg, TYPE_TELEMETRY typeTel, u32 value);
u32 getFlashData(u32 ADDR);
void setDateTime(DateTime* dt);
void setTM(time_t* pTimeStamp, DateTime* dt);
u8 getDeviation(EnergyData* pCurData, EnergyData* pLastData);
u8 crc8(char *pcBlock, int len);
u8 isCrcOk(char* pData, int len);
void offAllLeds();
void offAllRedLeds();
void toggleGreenLeds();
void toggleRedLeds();
u32 getServerTime();

time_t getTimeStamp();

u8 getGnssPckg(u8* pBuf, u16 szBuf, PckgEnergy* pPckgGnss, u8 szPckg);
void checkBufForWritingToFlash();
void updSpiFlash();
extern BKTE bkte;

#endif /* INC_UTILS_BKTE_H_ */
