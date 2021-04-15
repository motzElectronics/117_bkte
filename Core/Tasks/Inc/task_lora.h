#ifndef __TASK_LORA_H
#define __TASK_LORA_H

#include "main.h"
#include "spi.h"
#include "../Drivers/Inc/sx1272.h"
#include "../Utils/Inc/utils_crc.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"

#define LR_TASK_TIME_SLOT      1000
#define LR_TASK_TIME_FULL    (BKTE_ID_TRAINCAR_MAX - BKTE_ID_TRAINCAR) * LR_TASK_TIME_SLOT * 2

#define LR_TASK_PARSE_VALID_PCKG        0
#define LR_TASK_PARSE_INCVALID_PCKG     1

typedef struct{
    u32 timeStart;
    u32 timeShift;
    u32 time;
    u8  isTimerOn;
}LoraTimer;

typedef struct{
    u32 txPckg;
    u32 rxPckg;
    u32 rxPckgAnsw[BKTE_AMOUNTS];
    u32 rxPckgRcv[BKTE_AMOUNTS];
}LoraStatPckg;


void initTransmition();
void lrStartTim(LoraTimer* pTim, u16 shift);
void lrUpdTim(LoraTimer* pTim);
void lrStopTim(LoraTimer* pTim);
u8 parseReq(u8* pBufData, LoraAlgTransition* pRx, LoraAlgTransition* pTx);
u8 parseAnsw(u8* pBufData, LoraAlgTransition* pRx, LoraAlgTransition* pTx);
// u8 parseAnswMaster(u8* pBufData, LoraAlgTransition* pRx, LoraAlgTransition* pTx);
void updReq(LoraAlgTransition* pTx);
void updAnsw(LoraAlgTransition* pTx);
void serializeLoraAlgTrans(u8* pBuf, LoraAlgTransition* pTx);
void deserializeLoraAlgTrans(LoraAlgTransition* pTx, u8* pBuf);
void clearLoraAlgTrans(LoraAlgTransition* pckg);

void updStat(LoraStatPckg* pckgStat, LoraAlgTransition* pckg);
void initLoraStat(LoraStatPckg* pckg);
void parseServicesData(LoraAlgTransition* pRx);
void updServices(LoraAlgTransition* pTx);
void cpyRxInf(LoraAlgTransition* pRx, LoraAlgTransition* pTx);


#endif