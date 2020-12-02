/*
 * sim800.h
 *
 *  Created on: Feb 20, 2020
 *      Author: bls
 */

#ifndef INC_SIM800_H_
#define INC_SIM800_H_

/* AT Commands for HTTP Application */
#include "main.h"
#include "usart.h"
#include "cmsis_os.h"
#include "../Utils/Inc/utils_bkte.h"
#include "../Utils/Inc/utils_sd.h"
// #define RESPONSE_BUF_SZ				990
#define COMMAND_BUF_SZ				100
#define COMMAND_BUF_SZ				100
#define SIM_LEN_HTTP_CODE			3
#define SIM_TIMEOUT_RESPONSE		15000

//#define SIM_SZ_MAX_TX_DATA

#define INIT_HTTP_SERVICE										(char*) "HTTPINIT"
#define TERM_HTTP_SERVICE										(char*) "HTTPTERM"
#define SET_HTTP_PARAMETERS_VALUE								(char*) "HTTPPARA"
#define INPUT_HTTP_DATA											(char*) "HTTPDATA"
#define HTTP_METHOD_ACTION										(char*) "HTTPACTION"
#define READ_THE_HTTP_SERVER_RESPONSE							(char*) "HTTPREAD"
#define SAVE_HTTP_APLICATION_CONTEXT							(char*) "HTTPSCONT"
#define READ_HTTP_STATUS										(char*) "HTTPSTATUS"
#define READ_THE_HTTP_HEADER_INFORMATION_OF_SERVER_RESPONSE		(char*) "HTTPHEAD"

#define SIM_CMD_ECHO_OFF										(char*) "ATE0"
#define SIM_CMD_TYPE_ANSWER_NUM									(char*) "ATV0"


#define SIM_SUCCESS									0
#define SIM_FAIL									1
#define SIM_RESTART									2
#define SIM_HTTP_BAD_CODE_REQUEST					3

#define SIM_SZ_NO_GPSINFO							35
#define SIM_SZ_GPSINFO								100


#define SIM_Error_HTTP_INIT							2
#define SIM_Error_HTTP_PARAMETER_SETUP				3
#define SIM_Error_HTTP_WRITE_DATA					4
#define SIM_Error_HTTP_SEND_POST					5
#define SIM_Error_HTTP_POST_RESPONSE_NOT_SUPPORTED	6
#define SIM_Error_HTTP_NOT_RESPONDING_POST			7
#define SIM_Error_HTTP_TERMINATE					8
#define SIM_Error_HTTP_SEND_GET						9
#define SIM_Error_General_Reset_Configuration		10
#define SIM_Error_General_Echo_						11
#define SIM_Error_IO_Problem						12
#define SIM_Error_GPRS_Problem						13
#define SIM_Error_General_Get_Signal				14
#define SIM_Error_General_Get_Imei					15

#define URL_HTTP_PARAMETER					(char*) "URL"
#define UA_HTTP_PARAMETER					(char*) "UA"
#define PROIP_HTTP_PARAMETER				(char*) "PROIP"
#define PROPORT_HTTP_PARAMETER				(char*) "PROPORT"
#define REDIR_HTTP_PARAMETER				(char*) "REDIR"
#define BREAK_HTTP_PARAMETER				(char*) "BREAK"
#define BREAKEND_HTTP_PARAMETER				(char*) "BREAKEND"
#define TIMEOUT_HTTP_PARAMETER				(char*) "TIMEOUT"
#define CONTENT_HTTP_PARAMETER				(char*) "CONTENT"
#define USERDATA_HTTP_PARAMETER				(char*) "USERDATA"

#define SIM_SAPBR							(char*) "SAPBR"
#define SIM_CIPGSMLOC						(char*) "CIPGSMLOC"
#define SIM_CCLK							(char*) "CCLK?"
#define SIM_CLTS							(char*) "CLTS"

#define SIM_CIPGSMLOC						(char*) "CIPGSMLOC"
#define SIM_CCLK							(char*) "CCLK?"
#define SIM_CLTS							(char*) "CLTS"

#define SIM_CGPS							(char*) "CGPS"
#define SIM_CGPSINFO						(char*) "CGPSINFO"

#define SIM_CSQ								(char*)"CSQ"

#define SIM_CGNSINF							(char*)"CGNSINF"

#define SIM_CIPSHUT							(char*)"CIPSHUT"
#define SIM_CIPMUX							(char*)"CIPMUX"
#define SIM_CIPMODE							(char*)"CIPMODE"
#define SIM_CGATT							(char*)"CGATT"
#define SIM_CSTT							(char*)"CSTT"
#define SIM_CIICR							(char*)"CIICR"
#define SIM_CIFSR							(char*)"CIFSR"
#define SIM_CIPSTART						(char*)"CIPSTART"
#define SIM_CIPSTATUS						(char*)"CIPSTATUS"
#define SIM_CIPSEND							(char*)"CIPSEND"

#define SIM_CIPSTAT_INIT					(char*)"STATE: IP INITIAL"
#define SIM_CIPSTAT_START					(char*)"STATE: IP START"
#define SIM_CIPSTAT_GPRSACT					(char*)"STATE: IP GPRSACT"
#define SIM_CIPSTAT_STATUS					(char*)"STATE: IP STATUS"
#define SIM_CIPSTAT_CON_OK					(char*)"STATE: CONNECT OK"





/* Acknowledges execution of a Command
 * */
#define SIM_OK								(char*)"0"
#define SIM_OK_CHAR							(u8)'0'
#define SIM_OK_TEXT							(char*) "OK"
#define SIM_OK_CIPSHUT						(char*) "SHUT OK"

/* A connection has been established; the DCE is moving from
 * Command state to online data state
 * */
#define SIM_CONNECT							(char*)"1"
#define SIM_CONNECT_TEXT					(char*) "CONNECT"


/* The DCE has detected an incoming call signal from
 * network
 * */
#define SIM_RING							(char*)"2"
#define SIM_RING_TEXT						(char*) "RING"

/* The connection has been terminated or the attempt to
 * establish a connection failed
 * */
#define SIM_NO_CARRIER						(char*)"3"
#define SIM_NO_CARRIER_TEXT					(char*)"NO CARRIER"
/* Command not recognized, Command line maximum length
 * exceeded, parameter value invalid, or other problem with
 * processing the Command line
 * */
#define SIM_ERROR							(char*)"4"
#define SIM_ERROR_TEXT						(char*) "ERROR"


/* No dial tone detected
 * */
#define SIM_NO_DIALTONE						(char*)"6"
#define SIM_NO_DIALTONE_TEXT				(char*) "NO DIALTONE"


/* Engaged (busy) signal detected
 * */
#define SIM_BUSY							(char*)"7"
#define SIM_BUSY_TEXT						(char*) "BUSY"
/* "@" (Wait for Quiet Answer) dial modifier was used, but
 * remote ringing followed by five seconds of silence was not
 * detected before expiration of the connection timer (S7)
 * */
#define SIM_NO_ANSWER						(char*)"8"
#define SIM_NO_ANSWER_TEXT					(char*) "NO ANSWER"


#define SIM_PROCEEDING						(char*)("9")
#define SIM_PROCEEDING_TEXT					(char*) "PROCEEDING"

#define SIM_NO_RESPONSE						(char*)("5")
#define SIM_NO_RESPONSE_TEXT				(char*)("NO RESPONSE")
#define SIM_DOWNLOAD						(char*)"DOWNLOAD"

#define SIM_SEPARATOR_TEXT					(char*)"\r\n"


#define SIM_WAIT_FOREVER					HAL_MAX_DELAY
#define SIM_BUFFER_SIZE  					0x3ff
#define HTTP_RESPONSE_MAXIMUM_LENGTH		64

#define HTTP_CODE_OK						200


/*typedef enum
{
	receiveIdle = 0,
	receiving = 1,
	specResponseOccure = 2,
	specTimeOccure = 3,
	timeoutOccure = 4,
	bufOverflowOccure = 5,
}SIMrxStatTypeDef;*/

/*typedef struct{
	u32		WaitForSpecifiedResponse;
	u32		WaitForSpecifiedTime;
	u32 	Timeout;
}SIMrxOptionTypeDef;*/

/*typedef struct
{
	u8 response[RESPONSE_BUF_SZ];
	u8* tokenList[10];
	u8 tokenListSz;
	u8 res;
	SIMrxStatTypeDef rxStatus;

}SIMrxBufTypeDef;*/


//typedef struct{
//	u8	isIrqTx;
//	u8	isIrqRx;
//	u8	isIrqIdle;
//	u8	rxBuffer[SIM_SZ_RX_RESPONSE];
//	PtrHuart pHuart;
//}SIMuartInfo;


void httpDeInit();
u8 httpInit(char* httpAddr, u8 retriesCount);
char* simExecCommand(char* httpCommand);
char* simTxATCommand(char* command, u16 sz);
u8 httpPost(char* txData, u16 szTx, char** pRxData, u8 retriesCount, u32 httpTimeout);
u8 httpGetFirmwareSize(char** timestamp, u8 retriesCount, u32 httpTimeout);
u8 httpReadFirmwareSize(char** pRxData);
u8 httpWriteCommand(char* commandCode, char* params, u8 retriesCount, char* SUCCES_RETURN);
void simWriteCommand(char* command);
void simInit();
void simHttpInit(char*);
char* simGetStatusAnsw();
void copyStr(char* dist, char* source, u16 distSz);
void copyStrFromTo(char* dist, char* source, u16 numFrom, u16 numTo, u16 distSz);
u8 gpsInit(u8 retriesCount);
u8 httpRead(char** pRxData);
u8 simGetSzSoft(u32* szSoft);
//void waitSim(char* waitStr);
u8 httpGet(char** pRxData, u8 retriesCount, u32 httpTimeout);

u8 simGetDateTime(char* timestamp);
// u8 simGetSignalLevel();
u8 simCheckCSQ();
void simReset();

void simOn();

void simHardwareReset();

u8 simTCPTest();
u8 simTCPOpen();
u8 simTCPSend(u8* data, u16 sz);
u8 simTCPCheckStatus(const char* stat, u16 timeout, u16 delay);
u8 simCmd(char* cmdCode, char* params, u8 retriesCnt, char* SUCCESS_RET);
char* simTxATCmd(char* command, u16 sz);
long long simGetPhoneNum();

#define SIM_GPS_INIT() simCmd(SIM_SAPBR, "1,1", 1, SIM_OK_TEXT)

#endif /* INC_SIM800_H_ */
