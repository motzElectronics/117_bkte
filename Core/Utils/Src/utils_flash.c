
#include "../Utils/Inc/utils_flash.h"
//#include "utils_bsg.h"
u32* data;
//extern char logError[LOG_SZ_ERROR];
void flashClearPage(u32 page){
	while(HAL_FLASH_Unlock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Unlock()\r\n"));
	FLASH_Erase_Sector(page, VOLTAGE_RANGE_3);
	D(printf("FLASH_Erase_Sector\r\n"));
	while(HAL_FLASH_Lock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Lock()\r\n"));
}

void flashWrite(u8* pData, u32 szHex, u32* distAddr){
	u32 lenWords = szHex % 4 == 0 ? szHex / 4 : (szHex / 4) + 1;
	data = (u32*)pData;
	while(HAL_FLASH_Unlock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Unlock()\r\n"));
	for (u32 i = 0; i < lenWords; i++){
//		test = getFlashData(*distAddr);
		while(HAL_FLASH_Program(TYPEPROGRAM_WORD, *distAddr, (u32)(*(data + i))) != HAL_OK)
			HAL_Delay(100);
		if(getFlashData(*distAddr) != *(data + i)){
			D(printf("ERROR: flashWrite()\r\n"));
//			  createLog(logError, LOG_SZ_ERROR, "VCE PIZDEC\r\n");
		}
		(*distAddr) += 4;

	}
	while(HAL_FLASH_Lock() != HAL_OK) D(printf("ERROR: HAL_FLASH_Lock()\r\n"));

}

