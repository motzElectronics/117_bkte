#include "../Utils/Inc/utils_pckgs_manager.h"
#include "../Utils/Inc/utils_bkte.h"

WebPckg webPckgs[CNT_WEBPCKGS];
static u16 webPream = BKTE_PREAMBLE;
static u8 endBytes[] = {0x0D, 0x0A}; // reverse order

static void addInfo(WebPckg* pPckg, u8* src, u16 sz){
    memcpy(&pPckg->buf[pPckg->shift], src, sz);
    pPckg->shift += sz;
}

void clearWebPckg(WebPckg* pPckg){
    memset(pPckg->buf, '\0', SZ_WEB_PCKG);
    pPckg->shift = 0;
    pPckg->isFull = 0;
}

void initWebPckg(WebPckg* pPckg, u16 len){
    static u32 num = 0;
    num++;
    addInfo(pPckg, (u8*)&webPream, 2);
    addInfo(pPckg, (u8*)&num, 4);
    addInfo(pPckg, (u8*)&bkte.idMCU, 12);
    addInfo(pPckg, (u8*)&len, 2);
}

void addInfoToWebPckg(WebPckg* pPckg, u8* src, u16 sz, u8 cnt, u8 cmdData){
    addInfo(pPckg, &cmdData, 1);
    addInfo(pPckg, &cnt, 1);
    addInfo(pPckg, src, sz);
}

void showWebPckg(WebPckg* pPckg){
    for(u16 i = 0; i < pPckg->shift; i++){
        D(printf("%02x", pPckg->buf[i]));
    }
    D(printf("\r\n"));
}

void closeWebPckg(WebPckg* pPckg){
    addInfo(pPckg, (u8*)endBytes, 2);
    pPckg->isFull = 1;
}

u8 isNotFullPckg(){
    for(u8 i = 0; i < CNT_WEBPCKGS; i++){
        if(!webPckgs[i].isFull)
            return 1;
    }
    return 0;
}

WebPckg* getFreePckg(){
    for(u8 i = 0; i < CNT_WEBPCKGS; i++){
        if(!webPckgs[i].isFull)
            return &webPckgs[i];
    }
}