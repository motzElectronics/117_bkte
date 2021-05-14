#include "../Tasks/Inc/task_web_exchange.h"
#include "../Tasks/Inc/task_iwdg.h"

extern u16 iwdgTaskReg;

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
    u32 order_num = 0;
    offAllLeds();
    vTaskSuspend(webExchangeHandle);

    for (;;) {
        iwdgTaskReg |= IWDG_TASK_REG_WEB_EXCH;
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
                osEvent evt = osMessageGet(queueWebPckgHandle, 60000);
                if (evt.status == osEventMessage) {
                    curPckg = (WebPckg*)evt.value.p;
                } else {
                    continue;
                }
            }

            osMutexWait(mutexWebHandle, osWaitForever);
            memcpy(&order_num, &curPckg->buf[2], 4);
            printf("TCP Send: sz %d, num %d, addr 0x%08x\r\n", curPckg->shift, order_num, curPckg);
            statSend = sendTcp(curPckg->buf, curPckg->shift);
            if (statSend == TCP_OK) {
                clearWebPckg(curPckg);
                curPckg = NULL;
            } else if (statSend == TCP_SEND_ER) {
                osDelay(1000);
            }
            osMutexRelease(mutexWebHandle);

            osDelay(10);
        }
    }
}
