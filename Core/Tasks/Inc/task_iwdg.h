#ifndef __TASK_IWDG_H
#define __TASK_IWDG_H

#include "main.h"
#include "cmsis_os.h"

extern u16 iwdgTaskReg;

#define IWDG_TASK_REG_WEB_PCKG  0x0001
#define IWDG_TASK_REG_ENERGY    0x0002
#define IWDG_TASK_REG_WIRELESS  0x0004
#define IWDG_TASK_REG_TEMP      0x0008
#define IWDG_TASK_REG_ALIVE     0x0010
#define IWDG_TASK_REG_WEB_EXCH  0x0020
#define IWDG_TASK_REG_NEW_BIN   0x0040

#define IWDG_TASK_REG_ALL       0x003F

#endif