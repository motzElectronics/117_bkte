#include "../Tasks/Inc/task_iwdg.h"

void taskManageIWDG(void const * argument){

  for(;;){
    LL_IWDG_ReloadCounter(IWDG);
    osDelay(3000);
  }

}