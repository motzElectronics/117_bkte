#include "../Tasks/Inc/task_web_exchange.h"
extern osThreadId webExchangeHandle;
extern osThreadId createWebPckgHandle;
extern osThreadId createWebPckgHandle;
extern osMutexId mutexWebHandle;
extern osMessageQId queueWebPckgHandle;

static WebPckg* curPckg = NULL;

// void taskWebExchange(void const* argument) {
//     // vTaskSuspend(webExchangeHandle);
//     u8 statSend = TCP_OK;
//     offAllLeds();
//     vTaskSuspend(webExchangeHandle);
//     // vTaskSuspend(webExchangeHandle);

//     for (;;) {
//         if (statSend == TCP_OK || statSend == TCP_SEND_ER_LOST_PCKG) {
//             if (curPckg != NULL) {
//                 clearWebPckg(curPckg);
//                 curPckg = NULL;
//             }
//             xQueueReceive(queueWebPckgHandle, &curPckg, osWaitForever);
//         }
//         osMutexWait(mutexWebHandle, osWaitForever);
//         while ((statSend = openSendTcp(curPckg->buf, curPckg->shift)) !=
//         TCP_OK &&
//                statSend != TCP_SEND_ER_LOST_PCKG);
//         statSend = fastSendTcp(statSend);
//         osMutexRelease(mutexWebHandle);
//     }
// }

// u8 fastSendTcp(u8 statSend) {  // while open tcp connection
//     while (statSend == TCP_OK) {
//         clearWebPckg(curPckg);
//         curPckg = NULL;
//         if (xQueueReceive(queueWebPckgHandle, &curPckg, 15000) == pdPASS) {
//             D(printf("OK: fastSend\r\n"));
//             statSend = simTCPSend(curPckg->buf, curPckg->shift);

//         } else {
//             break;
//         }
//     }
//     return statSend;
// }

void taskWebExchange(void const* argument) {
    u8 statSend = TCP_OK;
    offAllLeds();
    vTaskSuspend(webExchangeHandle);

    for (;;) {
        if (bkte.isTCPOpen == 0) {
            osMutexWait(mutexWebHandle, osWaitForever);
            openTcp();
            osMutexRelease(mutexWebHandle);
        } else if (bkte.isTCPOpen == 1) {
            if (statSend == TCP_OK) {
                if (curPckg != NULL) {
                    clearWebPckg(curPckg);
                    curPckg = NULL;
                }
                xQueueReceive(queueWebPckgHandle, &curPckg, osWaitForever);
            }

            osMutexWait(mutexWebHandle, osWaitForever);
            statSend = sendTcp(curPckg->buf, curPckg->shift);
            if (statSend == TCP_OK) {
                clearWebPckg(curPckg);
                curPckg = NULL;
            }
            osMutexRelease(mutexWebHandle);

            osDelay(10);
        }
    }
}
