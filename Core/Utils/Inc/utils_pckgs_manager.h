#ifndef INC_UTILS_PCKG_MANAGER_H_
#define INC_UTILS_PCKG_MANAGER_H_
#include "main.h"

#define SZ_WEB_PCKG     1500

typedef struct{
    u16 shift;
	u8 buf[SZ_WEB_PCKG];
}WebPckg;

void clearWebPckg();
void initWebPckg(u16 len);
void addInfoToWebPckg(u8* src, u16 sz, u8 cnt, u8 cmdData);
void showWebPckg();

#endif