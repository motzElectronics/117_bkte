#ifndef INC_UTILS_SD_H_
#define INC_UTILS_SD_H_
#include "main.h"
#include "../Utils/Inc/utils_bkte.h"
#include "fatfs.h"

#define SZ_SECTOR   4096
#define LEN_TIME       9

#define LEN_SYMB_ENDL   2
#define STR_ENDL        (char*)"\r\n"

//!-------ERROR MSG-------
#define SD_ER_MSG_AT           (char*)"0 "
#define SD_LEN_MSG_AT           2

#define SD_ER_SAPBR             (char*)"1 "
#define SD_LEN_SAPBR            2

#define SD_ER_MSG_HTTPINIT     (char*)"10 "
#define SD_LEN_HTTP            3

#define SD_ER_MSG_HTTPPARA      (char*)"11 "

#define SD_ER_HTTPDATA          (char*)"12 "
#define SD_ER_HTTPDATA_UART     (char*)"13 "

#define SD_ER_HTTPACTION        (char*)"14 "

#define SD_ER_HTTPCODE          (char*)"15 "


#define SD_ER_MSG_HTTPINIT_MYFUN   (char*)"100 "
#define SD_LEN_MYFUN                4

#define SD_ER_MSG_HTTPPOST_MYFUN   (char*)"101 "

#define SD_ER_MSG_HTTPGET_MYFUN     (char*)"102 "

#define SD_ER_MSG_GET_SZ_NEW_BIN_MYFUN    (char*)"103 "

//!-------MSG-------------
#define SD_MSG_START_BKTE       (char*)"1 "
#define SD_LEN_START_BKTE       2

#define SD_MSG_OFF_BKTE         (char*)"2 "
#define SD_LEN_OFF_BKTE         2

#define SD_MSG_KEEP_ALIVE       (char*)"3 "
#define SD_LEN_KEEP_ALIVE       2

#define SD_MSG_SIM_RESET        (char*)"4 "
#define SD_LEN_SIM_RESET        2

#define SD_MSG_MCU_RESET        (char*)"5 "
#define SD_LEN_MCU_RESET        2

#define SD_MSG_NEW_BIN          (char*)"6 "
#define SD_LEN_NEW_BIN          2



typedef struct{
    u16     szSector;
    u8*     pBufSec;
    u16     freeSz;
    char*   fileName;
}SdSector;

extern SdSector sdSectorLogError;
extern SdSector sdSectorLogs;

void sdInit();
void sdWriteLog(char* strMsg, u16 szMsg, char* strParams, u16 szParams, SdSector* pSec);
void sdWriteSector(SdSector* pSdSector);
void cleanSector(SdSector* pSec);
void sdUpdLog(SdSector* pSec);


#endif /* INC_UTILS_FLASH_H_ */