#ifndef INC_UTILS_SD_H_
#define INC_UTILS_SD_H_
#include "main.h"
#include "../Utils/Inc/utils_bkte.h"
#include "fatfs.h"

#define SZ_SECTOR   4096

typedef struct{
    u16     szSector;
    u8*     pBufSec;
    u16     freeSz;
    char*   fileName;
}SdSector;

extern SdSector sdSectorLogError;
extern SdSector sdSectorLogWarning;

void sdInit();
void sdWrite(char* str, u16 szStr, SdSector* pSdSector);
void sdWriteSector(SdSector* pSdSector);
void cleanSector(SdSector* pSec);


#endif /* INC_UTILS_FLASH_H_ */