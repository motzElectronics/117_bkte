#include "../Tasks/Inc/task_wireless_sens.h"
extern osThreadId wirelessSensHandle;
static char bufSens[100];
CircularBuffer circBufWirelessSens = {.buf = NULL, .max = 0};
void taskWirelessSens(void const * argument){
    cBufInit(&circBufWirelessSens, uInfoWirelessSens.pRxBuf, uInfoWirelessSens.szRxBuf, CIRC_TYPE_WIRELESS);
    vTaskSuspend(wirelessSensHandle);
    for(;;){
        uInfoWirelessSens.irqFlags.isIrqRx = 0;
        while(!waitRx("", &uInfoWirelessSens.irqFlags, 1000, 15000)){
            D(printf("ERROR: NO WIRELESS SENS\r\n"));
        }
        while(cBufRead(&circBufWirelessSens, (u8*)bufSens, 0)){
            D(printf("OK: read wireless sens\r\n", bufSens));
        }
        D(printf("empty wirelessSensHandle\r\n"));
    }

}