#ifndef INC_UTILS_PCKG_MANAGER_H_
#define INC_UTILS_PCKG_MANAGER_H_
#include "main.h"

#define SZ_WEB_PCKG     1300
#define CNT_WEBPCKGS    3

typedef struct{
    u16 shift;
	u8 buf[SZ_WEB_PCKG];
    u8 isFull;
}WebPckg;

void clearWebPckg(WebPckg* pckg);
void initWebPckg(WebPckg* pckg, u16 len);
void addInfoToWebPckg(WebPckg* pckg, u8* src, u16 sz, u8 cnt, u8 cmdData);
void showWebPckg(WebPckg* pckg);
void closeWebPckg(WebPckg* pckg);

u8 isNotFullPckg();
WebPckg* getFreePckg();

#endif