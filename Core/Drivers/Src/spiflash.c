/*
 * m_spiflash.c
 *
 *  Created on: 11 июн. 2020 г.
 *      Author: YOBA-i516G
 */


#include "../Drivers/Inc/spiflash.h"
SPIFlash spiFlash64;

void spiFlashInit(u8* buf){
	
	spiFlash64.lock = 1;
	spiMemInfo.pHSpi = &hspi2;
	SPIFLASH_CS_UNSEL;
	osDelay(100);
	u32 id, tmp;

	id = spiFlashReadID();
	tmp = id & 0x0000FFFF;
	if(tmp){ //! change to 2017
		bkte.hwStat.isSPIFlash = 1;
	}
	D(printf("spiFlashId, series: %d %x\r\n", (int)id, (int)tmp));

	spiFlash64.blCnt = 128;
	spiFlash64.pgSz = 256;
	spiFlash64.secSz = 0x1000;
	spiFlash64.secCnt = spiFlash64.blCnt * 16;
	spiFlash64.pgCnt = (spiFlash64.secCnt * spiFlash64.secSz) / spiFlash64.pgSz;
	spiFlash64.blSz = spiFlash64.secSz * 16;
	spiFlash64.capacityKb = (spiFlash64.secCnt * spiFlash64.secSz) / 1024;
	spiFlash64.headNumPg = 0;
	spiFlash64.lock = 0;
	spiFlashRdPg(buf, 256, 0, BKTE_SAVE_NUM_PAGE);
    
	bkte.enAct = buf[16] | buf[17] << 8 | buf[18] << 16 | buf[19] << 24;
	bkte.enReact = buf[20] | buf[21] << 8 | buf[22] << 16 | buf[23] << 24;
    if (bkte.enAct > 0xA0000000) bkte.enAct = 0;
    if (bkte.enReact > 0xA0000000) bkte.enReact = 0;
    D(printf("Read energy: act %d, react %d\r\n", bkte.enAct, bkte.enReact));
    
	tmp = buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
    D(printf("Read headNumPg: %d\r\n", tmp));
	spiFlash64.headNumPg = tmp;
    
    tmp = buf[4] | buf[5] << 8 | buf[6] << 16 | buf[7] << 24;
    D(printf("Read tailNumPg: %d\r\n", tmp));
	spiFlash64.tailNumPg = tmp;

    if (spiFlash64.headNumPg > SPIFLASH_NUM_PG_GNSS 
         || spiFlash64.tailNumPg > SPIFLASH_NUM_PG_GNSS) {
            spiFlash64.headNumPg = 0;
            spiFlash64.tailNumPg = spiFlash64.headNumPg;
            spiFlashES(spiFlash64.headNumPg / SPIFLASH_NUM_PG_IN_SEC);
        }

    // spiFlash64.headNumPg = 0;
    // spiFlash64.tailNumPg = spiFlash64.headNumPg;
    // spiFlashES(spiFlash64.headNumPg / SPIFLASH_NUM_PG_IN_SEC);
    // spiFlashSaveData();
	//TODO:бинарный поиск head, tail

//	for(u8 i = 0; i < SPIFLASH_NUM_SEC_GNSS; i++)
//		spiFlashES(i);
}

u32 spiFlashReadID(void){
	u32 id;
	u8 tmp[4] = {SPIFLASH_CMD_JEDEC_ID, DUMMY_BYTE, DUMMY_BYTE, DUMMY_BYTE};
	SPIFLASH_CS_SEL;
	spiFlashTxRxCmd(tmp, sizeof(tmp));
	SPIFLASH_CS_UNSEL;
	id = (tmp[1] << 16) | (tmp[2] << 8) | tmp[3];
//	Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
	return id;
}

u8 spiFlashTxRxCmd(u8* data, u16 sz){
	static u8 ret = 0;
	spiMemInfo.irqFlags.regIrq = 0;
	HAL_SPI_TransmitReceive_DMA(spiMemInfo.pHSpi, data, data, sz); // spi2
	ret = waitRx("", &spiMemInfo.irqFlags, 50, WAIT_TIMEOUT);
	bkte.erFlags.flashNOIRQ = ~ret;
	return ret;
}

u8 spiFlashTxData(u8* data, u16 sz){
	static u8 ret = 0;
	spiMemInfo.irqFlags.regIrq = 0;
	HAL_SPI_Transmit_DMA(spiMemInfo.pHSpi, data, sz); // spi2
	ret = waitTx("", &spiMemInfo.irqFlags, 50, WAIT_TIMEOUT);
	bkte.erFlags.flashNOIRQ = ~ret;
	return ret;
}

u8 spiFlashRxData(u8* data, u16 sz){
	static u8 ret = 0;
	spiMemInfo.irqFlags.regIrq = 0;
	HAL_SPI_Receive_DMA(spiMemInfo.pHSpi, data, sz); // spi2
	ret = waitRx("", &spiMemInfo.irqFlags, 50, WAIT_TIMEOUT);
	bkte.erFlags.flashNOIRQ = ~ret;
	return ret;
}

void spiFlashES(u32 numSec)
{
    D(printf("spiFlash ErSec %d\r\n", numSec));
	u32 secAddr;

	while(spiFlash64.lock == 1)
		osDelay(100);

	secAddr = numSec * spiFlash64.secSz;
	u8 data[] = {SPIFLASH_SE, ((secAddr & 0xFF0000) >> 16), ((secAddr & 0xFF00) >> 8), (secAddr & 0xFF)};

	spiFlash64.lock = 1;
	spiFlashWaitReady();
	spiFlashWrEn();

	SPIFLASH_CS_SEL;
	spiFlashTxRxCmd(data, 4);
	SPIFLASH_CS_UNSEL;

	spiFlashWaitReady();
	osDelay(10);
	spiFlash64.lock = 0;
}

u8 spiFlashWaitReady(){
	u8 data[] = {SPIFLASH_RDSR, DUMMY_BYTE};
	osDelay(50);
	u8 i = 0, ret = 0;
	do{
		SPIFLASH_CS_SEL;
		osDelay(50);
		ret = spiFlashTxRxCmd(data, sizeof(data));
		i++;
		SPIFLASH_CS_UNSEL;
		osDelay(50);
	} while(((data[1] & 0x01) == 0x01) && (i < 3));
	return ret;
}

void spiFlashWrEn(){
	u8 cmd = SPIFLASH_WREN;
	SPIFLASH_CS_SEL;
	spiFlashTxRxCmd(&cmd, 1);
	SPIFLASH_CS_UNSEL;
	osDelay(10);
}

u8 spiFlashWrPg(u8 *pBuf, u32 sz, u32 offset, u32 numPage){
	D(printf("spiFlash WrPg %d\r\n", numPage));
	u32 addr;
	u8 ret = 0;
	if(spiFlash64.tailNumPg > spiFlash64.headNumPg && 
		(spiFlash64.tailNumPg - spiFlash64.headNumPg) < SPIFLASH_NUM_PG_IN_SEC 
		&& !(spiFlash64.headNumPg % SPIFLASH_NUM_PG_IN_SEC)){
			spiFlash64.tailNumPg = (spiFlash64.tailNumPg + (SPIFLASH_NUM_PG_IN_SEC - (spiFlash64.tailNumPg % SPIFLASH_NUM_PG_IN_SEC)) + 1) % SPIFLASH_NUM_PG_GNSS;
			ret = 1;
		}

	if(!spiFlash64.headNumPg)
		spiFlashES(0);
	else if(spiFlash64.headNumPg % SPIFLASH_NUM_PG_IN_SEC == 0)
		spiFlashES(spiFlash64.headNumPg / SPIFLASH_NUM_PG_IN_SEC);

	while(spiFlash64.lock == 1)
		osDelay(200);
	
	spiFlash64.lock = 1;
	
	if((offset + sz) > spiFlash64.pgSz)
		sz = spiFlash64.pgSz - offset;

	addr = (numPage * spiFlash64.pgSz) + offset;
	u8 data[] = {SPIFLASH_PP, ((addr & 0xFF0000) >> 16), ((addr & 0xFF00) >> 8), (addr & 0xFF)};
	// D(printf("spiFlashWaitReady()\r\n"));
	spiFlashWaitReady();
	// D(printf("spiFlashWrEn()\r\n"));
	spiFlashWrEn();

	SPIFLASH_CS_SEL;
	// D(printf("SPIFLASH_PP()\r\n"));
	spiFlashTxRxCmd(data, 4);
	// D(printf("spiFlashTxData()\r\n"));
	spiFlashTxData(pBuf, sz);
	SPIFLASH_CS_UNSEL;

	// D(printf("spiFlashWaitReady()\r\n"));
	spiFlashWaitReady();
	osDelay(10);
	spiFlash64.headNumPg = (spiFlash64.headNumPg + 1) % SPIFLASH_NUM_PG_GNSS;
	spiFlash64.lock = 0;
	return ret;
}

void spiFlashRdPg(u8 *pBuf, u32 sz, u32 offset, u32 numPage) {
	D(printf("spiFlash RdPg %d\r\n", numPage));
	u32 addr;
	while(spiFlash64.lock == 1)
		osDelay(200);
	spiFlash64.lock = 1;

	if((offset + sz) > spiFlash64.pgSz)
		sz = spiFlash64.pgSz - offset;

	addr = (numPage * spiFlash64.pgSz) + offset;
	u8 data[] = {SPIFLASH_FAST_READ, ((addr & 0xFF0000) >> 16), ((addr & 0xFF00) >> 8), (addr & 0xFF), DUMMY_BYTE};

	SPIFLASH_CS_SEL;
	spiFlashTxRxCmd(data, 5);
	spiFlashRxData(pBuf, sz);
	SPIFLASH_CS_UNSEL;

	osDelay(10);
	spiFlash64.tailNumPg = (spiFlash64.tailNumPg + 1) % SPIFLASH_NUM_PG_GNSS;
	spiFlash64.lock = 0;
}

int getDelayPages() {
    return spiFlash64.headNumPg >= spiFlash64.tailNumPg ? 
            spiFlash64.headNumPg - spiFlash64.tailNumPg : 
            spiFlash64.headNumPg + (SPIFLASH_NUM_PG_GNSS - spiFlash64.tailNumPg);
}

void spiFlashSaveData() {
	u32 addr;
    u8 buf[32];

    memset(buf, 0, 32);
    memcpy(&buf[0], &spiFlash64.headNumPg, 4);
    memcpy(&buf[4], &spiFlash64.tailNumPg, 4);
    memcpy(&buf[16], &bkte.enAct, 4);
    memcpy(&buf[20], &bkte.enReact, 4);

    spiFlashES(BKTE_SAVE_NUM_PAGE / SPIFLASH_NUM_PG_IN_SEC);

    while(spiFlash64.lock == 1)
        osDelay(200);
    spiFlash64.lock = 1;

    D(printf("Save pages: head %d, tail %d\r\n", spiFlash64.headNumPg, spiFlash64.tailNumPg));
    D(printf("Save energy: act %d, react %d\r\n", bkte.enAct, bkte.enReact));

    addr = (BKTE_SAVE_NUM_PAGE * spiFlash64.pgSz);
	u8 data[] = {SPIFLASH_PP, ((addr & 0xFF0000) >> 16), ((addr & 0xFF00) >> 8), (addr & 0xFF)};
	// D(printf("spiFlashWaitReady()\r\n"));
	spiFlashWaitReady();
	// D(printf("spiFlashWrEn()\r\n"));
	spiFlashWrEn();

	SPIFLASH_CS_SEL;
	// D(printf("SPIFLASH_PP()\r\n"));
	spiFlashTxRxCmd(data, 4);
	// D(printf("spiFlashTxData()\r\n"));
	spiFlashTxData(buf, 32);
	SPIFLASH_CS_UNSEL;

	// D(printf("spiFlashWaitReady()\r\n"));
	spiFlashWaitReady();
	osDelay(10);

    spiFlash64.lock = 0;
}

//u32 spiFlashReadByte(u8 *pBuffer, u32 Bytes_Address){
//	while(w25qxx.Lock==1)
//		W25qxx_Delay(1);
//
//	w25qxx.Lock=1;
//
//	W25QFLASH_CS_SELECT;
//	W25qxx_Spi(W25_FAST_READ);
//
//	if(w25qxx.ID >= W25Q256)
//		W25qxx_Spi((Bytes_Address & 0xFF000000) >> 24);
//
//	W25qxx_Spi((Bytes_Address & 0xFF0000) >> 16);
//	W25qxx_Spi((Bytes_Address& 0xFF00) >> 8);
//	W25qxx_Spi(Bytes_Address & 0xFF);
//	W25qxx_Spi(0);
//
//	*pBuffer = W25qxx_Spi(W25QXX_DUMMY_BYTE);
//
//	W25QFLASH_CS_UNSELECT;
//
//	w25qxx.Lock = 0;
//}
