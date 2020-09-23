/*
 * circularBuffer.c
 *
 *  Created on: 10 ���. 2019 �.
 *      Author: annsi
 */


#include "../Utils/Inc/circularBuffer.h"
#include "../Utils/Inc/utils_bkte.h"

//u8 circularBuffer[SZ_CIRCULAR_BUF] = {0};

//CircularBuffer dmaBuffer;


void cBufInit(CBufHandle cbuf, u8* buf){
	cbuf->buf = buf;
	cbuf->max = SZ_CIRCULAR_BUF;
	cBufReset(cbuf);
	printf("cBufInit()\r\n");
}

void cBufReset(CBufHandle cbuf){
	cbuf->head = 0;
	cbuf->tail = 0;
	cbuf->writeAvailable = cbuf->max;
	cbuf->readAvailable = 0;
	cbuf->numPckgInBuf = 0;
	cbuf->curLenMsg = 0;
	memset(cbuf->buf, '\0', cbuf->max);
}

//u8 cBufIsEmpty(CBufHandle cbuf){
//	return (!cbuf->isFull && (cbuf->head == cbuf->tail));
//}

//u8 cBufGetQuantity(CBufHandle cbuf){
//
//	u8 sz = cbuf->max;
//	if(!cbuf->isFull){
//		sz = cbuf->head >= cbuf->tail ? cbuf->head - cbuf->tail : cbuf->max + cbuf->head - cbuf->tail;
//	}
//
//	return sz;
//}


void cBufWriteToBuf(CBufHandle cbuf, u8* data, u8 sz){
	if(cbuf->writeAvailable > sz){
		cbuf->remainWrite = cbuf->max - cbuf->head;
		if(sz > cbuf->remainWrite){
			memcpy(cbuf->buf + cbuf->head, data, cbuf->remainWrite);
			memcpy(cbuf->buf, data + cbuf->remainWrite, sz - cbuf->remainWrite);
		} else {
			memcpy(cbuf->buf + cbuf->head, data, sz);
		}

		cbuf->numPckgInBuf++;
		cbuf->head = (cbuf->head + sz) % cbuf->max;
		cbuf->writeAvailable -= sz;
		cbuf->readAvailable += sz;
	} else {
		printf("FULL CIRC BUFFER\r\n");
	}

}

//u8 cBufRead(CBufHandle cbuf, u8* dist, u8 sz){
//	while( cbuf->head != cbuf->tail){
//		if(cbuf->buf[cbuf->tail] == HEADER_BYTE1 && cbuf->buf[(cbuf->tail + 1) % cbuf->max] == HEADER_BYTE2){
//			cbuf->buf[cbuf->tail] = 0;
//			cbuf->buf[(cbuf->tail + 1) % cbuf->max] = 0;
//			cbuf->tail = (cbuf->tail + 2) % cbuf->max;
//
//			copyGetDatafromBuf(cbuf, dist, sz);
//			return 1;
//		}
//		(++(cbuf->tail)) % cbuf->max;
//	}
//	memset(cbuf->buf, 0, cbuf->max);
//	cBufReset(cbuf);
//	return 0;
//}


u16 cBufRead(CBufHandle cbuf, u8* dist, CircTypeBuf typeBuf, u8 sz){
	u16 lenMsg;
	switch(typeBuf){
	case CIRC_TYPE_SIM_UART:
		if((lenMsg = getLenMsgSimUart(cbuf))){
			copyGetDatafromBuf(cbuf, dist, lenMsg, CIRC_TYPE_SIM_UART);
		}
		break;
	case CIRC_TYPE_ENERGY_UART:
		if((lenMsg = getLenMsgEnergyUart(cbuf)) && lenMsg == BKTE_SZ_UART_MSG){
			copyGetDatafromBuf(cbuf, dist, lenMsg, CIRC_TYPE_ENERGY_UART);
		}
		else if(lenMsg){
			cBufReset(cbuf);
			printf("ERROR: CIRC_TYPE_ENERGY_UART lenMsg: %d\r\n", lenMsg);
		}
		break;
	case CIRC_TYPE_PCKG_ENERGY:
	case CIRC_TYPE_PCKG_RSSI:
	case CIRC_TYPE_PCKG_TEMP:
		lenMsg = sz;
//		printf("CIRC_TYPE_PCKG_TEMP, CIRC_TYPE_PCKG_RSSI, CIRC_TYPE_PCKG_ENERGY, copyGetDatafromBuf\r\n");
		copyGetDatafromBuf(cbuf, dist, lenMsg, CIRC_TYPE_PCKG_ENERGY);
		break;
	}
	return lenMsg;
}


void copyGetDatafromBuf(CBufHandle cbuf, u8* dist, u16 sz, CircTypeBuf type){

	cbuf->remainRead = cbuf->max - cbuf->tail;
	if(sz > cbuf->remainRead){
		memcpy(dist, cbuf->buf + cbuf->tail, cbuf->remainRead);
		memcpy(dist + cbuf->remainRead, cbuf->buf, sz - cbuf->remainRead);
		memset(cbuf->buf + cbuf->tail, '\0', cbuf->remainRead);
		memset(cbuf->buf, '\0', sz - cbuf->remainRead);

	} else{
		memcpy(dist, cbuf->buf + cbuf->tail, sz);
		memset(cbuf->buf + cbuf->tail, '\0', sz);
	}
	if(type == CIRC_TYPE_SIM_UART){
		cbuf->tail = (cbuf->tail + sz + CIRC_LEN_ENDS) % cbuf->max;
		cbuf->writeAvailable += CIRC_LEN_ENDS;
		cbuf->readAvailable -= CIRC_LEN_ENDS;
	}
	else if(type == CIRC_TYPE_ENERGY_UART || type == CIRC_TYPE_PCKG_ENERGY ||
			type == CIRC_TYPE_PCKG_TEMP)
		cbuf->tail = (cbuf->tail + sz) % cbuf->max;
	cbuf->writeAvailable += sz;
	cbuf->readAvailable -= sz;
	--(cbuf->numPckgInBuf);
}

u8 getLenMsgSimUart(CBufHandle cbuf){
	u16 lenMsg = 0;
	u16 tail = cbuf->tail;
	if(cbuf->numPckgInBuf > 0){
		while(tail != cbuf->head){
			if(cbuf->buf[tail] == CIRC_END1_MSG && cbuf->buf[(tail + 1) % cbuf->max] == CIRC_END2_MSG){
				cbuf->buf[tail] = '\0';
				cbuf->buf[(tail + 1) % cbuf->max] = '\0';
				break;
			}
			tail = (tail + 1) % cbuf->max;
			lenMsg++;
		}
	}
	return lenMsg;
}

u8 getLenMsgEnergyUart(CBufHandle cbuf){
	u16 lenMsg = 0;
	u16 tail = cbuf->tail;
	if(cbuf->numPckgInBuf > 1){
		while(tail != cbuf->head){
			if(cbuf->buf[tail] == CIRC_HEADER1 && cbuf->buf[(tail + 1) % cbuf->max] == CIRC_HEADER2){
//				cbuf->buf[tail] = '\0';
//				cbuf->buf[(tail + 1) % cbuf->max] = '\0';
//				tail = (tail + 2) % cbuf->max;
//				lenMsg = 2;
				cbuf->tail = tail;
				do{
					lenMsg++;
					if(tail == cbuf->head){
						lenMsg = 0;
						cBufReset(cbuf);
						printf("ERROR: NOHEADER\r\n");
					}
					tail = (tail + 1) % cbuf->max;

				}while(!(cbuf->buf[tail] == CIRC_HEADER1 && cbuf->buf[(tail + 1) % cbuf->max] == CIRC_HEADER2) && lenMsg);
				break;
			}
			cbuf->writeAvailable++;
			cbuf->readAvailable--;
			cbuf->buf[tail] = '\0';
			tail = (tail + 1) % cbuf->max;
		}
//		printf("WRITEAV: %d   READAVAIL: %d\r\n", cbuf->writeAvailable, cbuf->readAvailable);
	}

	return lenMsg;
}

