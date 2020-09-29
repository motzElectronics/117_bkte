#include "../Utils/Inc/utils_sd.h"
static FIL fil;
static char* fileNameError = (char*)"er.txt";
static char* fileNameWarning = (char*)"wr.txt";
static u8 bufSectorLogError[SZ_SECTOR];
static u8 bufSectorLogWarning[SZ_SECTOR];
SdSector sdSectorLogError;
SdSector sdSectorLogWarning;

void sdInit(){
    if(fatInit() == FAT_ERROR_NOT_MOUNT){
		D(printf("ERROR: f_mount\r\n"));
    }else{
        sdSectorLogError.szSector = SZ_SECTOR;
        sdSectorLogError.pBufSec = bufSectorLogError;
        sdSectorLogError.fileName = fileNameError;
        cleanSector(&sdSectorLogError);

        sdSectorLogWarning.szSector = SZ_SECTOR;
        sdSectorLogWarning.pBufSec = bufSectorLogWarning;
        sdSectorLogWarning.fileName = fileNameWarning;
        cleanSector(&sdSectorLogWarning);
    }
}

void sdWrite(char* str, u16 szStr, SdSector* pSdSector){
    if(pSdSector->freeSz < szStr){
        sdWriteSector(pSdSector);
        cleanSector(pSdSector);
    }
    strcat(pSdSector->pBufSec, str);
    pSdSector->freeSz -= szStr;
}

void sdWriteSector(SdSector* pSdSector){
    u32 bWrite;
    if(f_open(&fil, pSdSector->fileName, FA_WRITE | FA_OPEN_APPEND) == FR_OK){

        if(f_write(&fil, pSdSector->pBufSec, pSdSector->szSector - pSdSector->freeSz, (UINT*)&bWrite) != FR_OK){
            D(printf("ERROR: f_write\r\n"));
        }

        if((f_close(&fil)) == FR_OK)
            D(printf("OK: f_close\r\n"));
        else 
            D(printf("ERROR: f_close\r\n"));
    } else{
        D(printf("ERROR: f_open\r\n"));
    }
}

void cleanSector(SdSector* pSec){
    pSec->freeSz = SZ_SECTOR;
    memset(pSec->pBufSec, pSec->szSector, '\0');
}