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

#define LR_TASK_PARCE_VALID_PCKG        0
#define LR_TASK_PARCE_INCVALID_PCKG     1

typedef struct{
    u32 timeStart;
    u16 timeShift;
    u16 time;
    u8  isTimerOn;
}LoraTimer;


void initTransmition();
void lrStartTim(LoraTimer* pTim, u16 shift);
void lrUpdTim(LoraTimer* pTim);
void lrStopTim(LoraTimer* pTim);
u8 parceReq(u8* pBufData, LoraAlgTransition* pRx, LoraAlgTransition* pTx);
u8 parceAnsw(u8* pBufData, LoraAlgTransition* pRx, LoraAlgTransition* pTx);
void updReq(LoraAlgTransition* pTx);
void updAnsw(LoraAlgTransition* pTx);
void serializeLoraAlgTrans(u8* pBuf, LoraAlgTransition* pTx);
void deserializeLoraAlgTrans(LoraAlgTransition* pTx, u8* pBuf);
void clearLoraAlgTrans(LoraAlgTransition* pckg);


#endif