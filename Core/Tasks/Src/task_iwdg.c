#include "../Tasks/Inc/task_iwdg.h"
#include "../Drivers/Inc/spiflash.h"

extern u8 isRxNewFirmware;

u16 iwdgTaskReg;
u32 iwdgErrCount;

void taskManageIWDG(void const* argument) {
    iwdgTaskReg = 0;
    iwdgErrCount = 0;

    for (;;) {
        if ((isRxNewFirmware && (iwdgTaskReg & IWDG_TASK_REG_NEW_BIN) == IWDG_TASK_REG_NEW_BIN) || 
               (!isRxNewFirmware && (iwdgTaskReg & IWDG_TASK_REG_ALL) == IWDG_TASK_REG_ALL)) {
            iwdgTaskReg = 0;
            iwdgErrCount = 0;
            printf("iwdg: ok\r\n");
        } else {
            iwdgErrCount++;
            printf("iwdg: err %d\r\n", iwdgErrCount);
        }

        if (iwdgErrCount > 700) {
            spiFlashSaveData();
            NVIC_SystemReset();
        }

        LL_IWDG_ReloadCounter(IWDG);
        osDelay(3000);
        HAL_GPIO_TogglePin(LED3G_GPIO_Port, LED3G_Pin);
    }
}