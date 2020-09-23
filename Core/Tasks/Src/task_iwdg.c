#include "../Tasks/Inc/task_iwdg.h"
extern IWDG_HandleTypeDef hiwdg;

void taskManageIWDG(void const * argument){

  for(;;){
    HAL_IWDG_Refresh(&hiwdg);
    osDelay(3000);
  }

}