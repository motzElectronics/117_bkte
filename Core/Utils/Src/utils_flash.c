/*
 * utils_flash.c
 *
 *  Created on: 5 мая 2020 г.
 *      Author: annsi
 */

#include "../Utils/Inc/utils_flash.h"
//#include "utils_bsg.h"
u32 hexFirmware[FLASH_LEN_WORDS_INNER_BUF];
//extern char logError[LOG_SZ_ERROR];
void flashClearPage(u32 page){
	while(HAL_FLASH_Unlock() != HAL_OK) printf("ERROR: HAL_FLASH_Unlock()\r\n");
	FLASH_Erase_Sector(page, VOLTAGE_RANGE_3);
	printf("FLASH_Erase_Sector\r\n");
	while(HAL_FLASH_Lock() != HAL_OK) printf("ERROR: HAL_FLASH_Lock()\r\n");
}

void flashWrite(char* pData, u32 szHex, u32* distAddr){
	charToHex(pData, szHex);
	u32 lenWords = szHex % 4 == 0 ? szHex / 4 : (szHex / 4) + 1;
	while(HAL_FLASH_Unlock() != HAL_OK) printf("ERROR: HAL_FLASH_Unlock()\r\n");
	for (u32 i = 0; i < lenWords; i++){
//		test = getFlashData(*distAddr);
		while(HAL_FLASH_Program(TYPEPROGRAM_WORD, *distAddr, *(hexFirmware + i)) != HAL_OK)
			HAL_Delay(100);
		if(getFlashData(*distAddr) != *(hexFirmware + i)){
			printf("ERROR: flashWrite()\r\n");
//			  createLog(logError, LOG_SZ_ERROR, "VCE PIZDEC\r\n");
		}
		(*distAddr) += 4;

	}
	while(HAL_FLASH_Lock() != HAL_OK) printf("ERROR: HAL_FLASH_Lock()\r\n");

}

void charToHex(char* pData, u32 szHex){
	u32 lenMsg = szHex * 2;
	char tmp[9];
	u32 tmpU8;
	memset(hexFirmware, '\0', FLASH_LEN_WORDS_INNER_BUF * 4);
	for(u32 i = 0; i < lenMsg; i += 8){
		memset(tmp, '\0', 9);
		if(i + 8 <= lenMsg) memcpy(tmp, pData + i, 8);
		else memcpy(tmp, pData + i, lenMsg - i);
		tmpU8 = strtoul(tmp, NULL, 16);
		hexFirmware[i / 8] = ((tmpU8 & 0xFF) << 24) | (((tmpU8 >> 8) & 0xFF)<< 16) | (((tmpU8 >> 16) & 0xFF) << 8) | ((tmpU8 >> 24) & 0xFF);

	}
	printf("hexFirmware\r\n");
}

