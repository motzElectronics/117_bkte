/*
 * utils_bkte.h
 *
 *  Created on: Mar 16, 2020
 *      Author: bls
 */

#ifndef INC_UTILS_BKTE_H_
#define INC_UTILS_BKTE_H_

#include "../Drivers/Inc/ds2482.h"
#include "../Drivers/Inc/simcom.h"
#include "../Drivers/Inc/spiflash.h"
#include "../Utils/Inc/circularBuffer.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "main.h"
#include "stdlib.h"
#include "time.h"
#include "usart.h"

//!-------------CONFIGURE PARAMS---------------
#define BKTE_ID_TRAINCAR 1
#define BKTE_ID_TRAINCAR_MAX 2
#define BKTE_IS_LORA_MASTER 1

#define BKTE_ID_FIRMWARE 3
#define BKTE_ID_BOOT 2
#define BKTE_ID_PCB 3
#define BKTE_ID_TRAIN 1706
//!------------CONFIGURE PARAMS----------------
#define BKTE_THRESHOLD_CNT_PAGES 2
#define BKTE_AMOUNTS (BKTE_ID_TRAINCAR_MAX + 1)
#define BKTE_ADDR_ID_MCU 0x1FFF7A10
#define BKTE_MAX_CNT_1WIRE 4
#define LEN_TIMESTAMP 13

#define BKTE_NUM_HEADER_55 11
#define BKTE_NUM_HEADER_B9 22
#define BKTE_NUM_CURRENT 12
#define BKTE_NUM_VOLT 14
#define BKTE_SZ_CURRENT 2
#define BKTE_SZ_VOLT 2
#define BKTE_NUM_ACT_ENERGY 16
#define BKTE_SZ_ENERGY 4
#define BKTE_NUM_REACT_ENERGY 27

#define BKTE_ID_DEV_BKTE 0x10
#define BKTE_ID_DEV_BKT 0x11
#define BKTE_ID_DEV_BSG 0x12

#define SZ_PART_NEW_SOFTWARE 1300

#define BKTE_VER_BETA_FIRMWARE (char)'B'
#define BKTE_VER_STABLE_FIRMWARE (char)'S'

#define BKTE_PERCENT_DEVIATION_ENERGY_DATA (float)0.03
#define BKTE_MEASURE_FULL_LOOP (u8)60

#define SZ_MAX_TX_DATA 4096

#define BKTE_MAX_TEMP 100
#define BKTE_MIN_TEMP -100
#define BKTE_NO_TEMP -127
#define BKTE_WIRELESS_TEMP 101

#define BKTE_BAD_TIMESTAMP 2997993600

#define BKTE_BIG_DIF_RTC_SERVTIME 600

typedef union {
    struct {
        u16 simAT : 1;
        u16 simSAPBR : 1;
        u16 simHINIT : 1;
        u16 simHPARA : 1;
        u16 simHDATA : 1;
        u16 simHDATAU : 1;
        u16 simHACT : 1;
        u16 simHREAD : 1;
        u16 simHCODE : 1;
        u16 simCSQINF : 1;
        u16 flashNOIRQ : 1;
    };
    u16 errReg;
} ErrorFlags;

typedef struct {
    u8 canMeasure;
    s8 curTemp;
} TempLine;

typedef struct {
    u32 enAct;
    u32 enReact;
    s16 current;
    u16 volt;
    s8  temp[4];
} LastData;

typedef struct {
    u32 posFile;
    u32 lenLog;
    char* fNameLog;
    char* fNameAddr;
} FInfo;

typedef union {
    struct {
        u16 isChrgBat : 1;
        u16 isPwrState : 1;
        u16 adcVoltBat : 9;
    };
    u16 pwrReg;
} PWRInfo;

typedef union {
    struct {
        u8 isFatMount : 1;
        u8 isDS2482 : 1;
        u8 isSPIFlash : 1;
        u8 isLoraOk : 1;
    };
    u8 regHardWareStat;
} HardWareStatus;

typedef struct {
    u8 isOwActive[BKTE_MAX_CNT_1WIRE];
    u32 idMCU[3];
    HardWareStatus hwStat;
    PWRInfo pwrInfo;
    ErrorFlags erFlags;
    u8 isSentData;
    u8 isTCPOpen;
    u8 tcpErrCnt;
    u8 csq;
    u8 idNewFirmware;
    u8 isSpiFlashReady;
    LastData lastData;
    //	FInfo	fInfo[NUM_READ_FILES];
} BKTE;

typedef enum { NUM_FILE_ENERGY = 0, NUM_FILE_TEMP, NUM_FILE_RSSI } NUM_FILE;

typedef enum {
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
    TEL_BAD_ALL_CRC = 0x2008,
    TEL_LORA_LINK_EDGE = 0x2020,
    TEL_LORA_LINK_MASTER = 0x2021,
    TEL_LORA_FLAGS = 0x2022,
    TEL_LORA_BAD_CRC = 0x2023,
    TEL_NO_FATFS = 2030,
    TEL_NO_DS2482 = 2031,
    TEL_PERIPH_STAT = 2032,
    TEL_LVL_CSQ = 0x7010
} TYPE_TELEMETRY;

typedef enum { TEL_GR_GENINF = 1, TEL_GR_HARDWARE_STATUS } TELEMETRY_GROUP;

typedef enum {
    TEL_CD_GENINF_NUM_FIRMWARE = 1,
    TEL_CD_GENINF_NUM_BOOT,
    TEL_CD_GENINF_PHONE_NUM1,
    TEL_CD_GENINF_PHONE_NUM2,
    TEL_CD_GENINF_SIMCARD_MONEY,
    TEL_CD_GENINF_NUM_PCB
} TELEMETRY_CODE_GEN_INF;

typedef enum {
    TEL_CD_HW_BKTE = 1,
    TEL_CD_HW_SD,
    TEL_CD_HW_DS2482,
    TEL_CD_HW_SPI_FLASH,
    TEL_CD_HW_LORA,
    TEL_CD_HW_BATTERY,
    TEL_CD_HW_BKTE_ALIVE,
    TEL_CD_HW_WIRELESS_SENS_RSSI,
    TEL_CD_HW_UPDATED,
    TEL_CD_HW_UPDATE_ERR
} TELEMETRY_CODE_STATES;

typedef enum {
    CMD_DATA_VOLTAMPER = 1,
    CMD_DATA_ENERGY,
    CMD_DATA_TEMP,
    CMD_DATA_TELEMETRY = 5
} CMD_DATA;

typedef enum {
    CMD_REQUEST_SERVER_TIME = 0x11,
    CMD_REQUEST_NUM_FIRMWARE,
    CMD_REQUEST_SZ_FIRMWARE,
    CMD_REQUEST_PART_FIRMWARE
} CMD_REQUEST;

typedef enum { MSG_TEMP = 0xF000, MSG_TELEMETRY = 0x0000 } TYPE_MSG;

typedef struct {
    u32 unixTimeStamp;
    u32 enAct;
    u32 enReact;
} PckgEnergy;

typedef struct {
    u32 unixTimeStamp;
    s16 amper;
    u16 volt;
} PckgVoltAmper;

typedef struct {
    u32 unixTimeStamp;
    u32 data;
    u8 group;
    u8 code;
} PckgTelemetry;

typedef struct {
    u32 unixTimeStamp;
    s8 temp[BKTE_MAX_CNT_1WIRE];
} PckgTemp;

typedef struct {
    u32 fromByte;
    u32 toByte;
} PckgUpdFirmware;

void bkteInit();
void getMaxNumDS1820(BKTE* pBkte);
void resetTempLine(u8 numLine);
void setTempLine(u8 numLine);
void fillPckgEnergy(PckgEnergy* pckg, u16* data);
void fillPckgTemp(PckgTemp* pckg, s8* data);

void fillPckgVoltAmper(PckgVoltAmper* pckg, u16* data);

void setDateTime(DateTime* dt);
void setTM(time_t* pTimeStamp, DateTime* dt);

u8 getDeviation(LastData* pCurData, LastData* pLastData);
u8 crc8(char* pcBlock, int len);
u8 isCrcOk(char* pData, int len);
void offAllLeds();
void offAllRedLeds();
void toggleGreenLeds();
void toggleRedLeds();
void getServerTime();

u8 getGnssPckg(u8* pBuf, u16 szBuf, PckgEnergy* pPckgGnss, u8 szPckg);
// void checkBufForWritingToFlash();
void updSpiFlash(CircularBuffer* cbuf);
u8 waitGoodCsq(u32 timeout);

void saveData(u8* data, u8 sz, u8 cmdData, CircularBuffer* cbuf);
u32 getUnixTimeStamp();
u8 isDataFromFlashOk(char* pData, u8 len);
void copyTelemetry(u8* buf, PckgTelemetry* pckgTel);
void saveTelemetry(PckgTelemetry* pckg, CircularBuffer* cbuf);

void getNumFirmware();

extern BKTE bkte;

#endif /* INC_UTILS_BKTE_H_ */
