#include "../Utils/Inc/utils_sd.h"
static FIL FTest;
static char* fTest = (char*)"test.txt";

void sdWrite(char* str, u8 szStr){
    u32 bWrite;

    if(f_open(&FTest, fTest, FA_WRITE | FA_OPEN_APPEND) == FR_OK){

        if(f_write(&FTest, str, szStr, (UINT*)&bWrite) != FR_OK){
            D(printf("ERROR: f_write\r\n"));
        }

        if((f_close(&FTest)) == FR_OK)
            D(printf("OK: f_close\r\n"));
        else 
            D(printf("ERROR: f_close\r\n"));
    } else{
        D(printf("ERROR: f_open"));
    }
}