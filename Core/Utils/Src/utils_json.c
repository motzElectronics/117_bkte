/*
 * m_utils_json.c
 *
 *  Created on: Jun 12, 2020
 *      Author: annsi
 */
#include "../Utils/Inc/utils_json.h"
extern BKTE bkte;
PckgJsonEn pckgJsonEn;
static time_t timestamp;
char tmpJsonGnss[100];
//"001112605ed68011, 6002.79657,N,03020.3559,E,005,+0000,000"

/*void addGnssPckgToJson(PckgGnss* pPckgGnss){
	static u16 cnt = 0;
	cnt++;
	memset(tmpJsonGnss, '\0', 100);
	setTM(&timestamp, &pPckgGnss->dateTime);
	if(pckgJsonGnss.numPckg == 0){
		memset(pckgJsonGnss.jsonGnssTxBuf, '\0', sizeof(pckgJsonGnss.jsonGnssTxBuf));
		sprintf(pckgJsonGnss.jsonGnssTxBuf, "{\"h\":\"02%08x%08x%08x%04x%02x%04x\", \"d\":[",
				(uint)bsg.idMCU[0], (uint)bsg.idMCU[1], (uint)bsg.idMCU[2],
				(uint)bsg.idTrain, (uint)bsg.idTrainCar, (uint)bsg.idReceiver);
		sprintf(tmpJsonGnss, "\"%04x%02x60%08x, %04d.%d,%c,%05d.%d,%c,%d,0,%d\"",
				cnt, bsg.idDev, (int)timestamp,
				pPckgGnss->latitude.fst, pPckgGnss->latitude.sec, pPckgGnss->latitude.indicator,
				pPckgGnss->longitude.fst, pPckgGnss->longitude.sec, pPckgGnss->longitude.indicator,
				pPckgGnss->speed, pPckgGnss->cource);
	} else {

	sprintf(tmpJsonGnss, ",\"%04x%02x60%08x, %04d.%d,%c,%05d.%d,%c,%d,0,%d\"",
			cnt, bsg.idDev, (int)timestamp,
			pPckgGnss->latitude.fst, pPckgGnss->latitude.sec, pPckgGnss->latitude.indicator,
			pPckgGnss->longitude.fst, pPckgGnss->longitude.sec, pPckgGnss->longitude.indicator,
			pPckgGnss->speed, pPckgGnss->cource);
	}

	strcat(pckgJsonGnss.jsonGnssTxBuf, tmpJsonGnss);
	pckgJsonGnss.numPckg++;



}*/

/*u32 addEnPckgToJson(PckgEnergy* pckg){
	static u16 cnt = 0;
	cnt++;
	memset(tmpJsonGnss, '\0', 100);
	setTM(&timestamp, &pckg->dateTime);
	if(pckgJsonEn.numPckg == 0){
		memset(pckgJsonEn.jsonEnTxBuf, '\0', sizeof(pckgJsonEn.jsonEnTxBuf));
		sprintf(pckgJsonEn.jsonEnTxBuf, "{\"h\":\"02%08x%08x%08x%04x%02x%04x\", \"d\":[",
				(uint)bkte.idMCU[0], (uint)bkte.idMCU[1], (uint)bkte.idMCU[2],
				(uint)bkte.idTrain, (uint)bkte.idTrainCar, (uint)bkte.idReceiver);

		sprintf(tmpJsonGnss, "\"%04x%02x50%08x%04x%04x%08x%08x\"",
				(uint)cnt, (uint)bkte.idDev, (int)timestamp, (uint)pckg->energyData.current, (uint)pckg->energyData.volt,
				(uint)pckg->energyData.enAct, (uint)pckg->energyData.enReact);
	} else {

		sprintf(tmpJsonGnss, ",\"%04x%02x50%08x%04x%04x%08x%08x\"",
				(uint)cnt, (uint)bkte.idDev, (int)timestamp, (uint)pckg->energyData.current, (uint)pckg->energyData.volt,
				(uint)pckg->energyData.enAct, (uint)pckg->energyData.enReact);
	}

	strcat(pckgJsonEn.jsonEnTxBuf, tmpJsonGnss);
	pckgJsonEn.numPckg++;
	return timestamp;
}

u16 closeGnssJson(){
	u16 numPckg;
	strcat(pckgJsonEn.jsonEnTxBuf, "]}\n");
	D(printf("%s\r\n", pckgJsonEn.jsonEnTxBuf));
	numPckg = pckgJsonEn.numPckg;
	pckgJsonEn.numPckg = 0;
	return numPckg;
}*/

void pckgUpdFirmwareToJson(PckgUpdFirmware* source, char* dist, u16 szDist){
	/*static u16 cnt = 0;
	cnt++;
	memset(dist, '\0', szDist);
	sprintf(dist, "{\"h\":\"02%08x%08x%08x%04x%02x%04x\", \"d\":[\"%04x%02x23%08x%08x%08x\"]}",
				(uint)bkte.idMCU[0], (uint)bkte.idMCU[1], (uint)bkte.idMCU[2],
				(uint)bkte.idTrain, (uint)bkte.idTrainCar, (uint)bkte.idReceiver,
				(uint)cnt, (uint)bkte.idDev,  (int)timestamp,
				(uint)source->fromByte, (uint)source->toByte);*/
	D(printf("pckgUpdFirmwareToJson()\r\n"));
}

