#include "../Utils/Inc/utils_pckgs_manager.h"
#include "../Utils/Inc/utils_bkte.h"
WebPckg webPckg;
static u16 webPream = BKTE_PREAMBLE;

static void addInfo(u8* src, u16 sz){
    memcpy(&webPckg.buf[webPckg.shift], src, sz);
    webPckg.shift += sz;
}

void clearWebPckg(){
    memset(webPckg.buf, '\0', SZ_WEB_PCKG);
    webPckg.shift = 0;
}

void initWebPckg(u16 len){
    static u32 num = 0;
    num++;
    addInfo((u8*)&webPream, 2);
    addInfo((u8*)&num, 4);
    addInfo((u8*)&bkte.idMCU, 12);
    addInfo((u8*)&len, 2);
}

void addInfoToWebPckg(u8* src, u16 sz, u8 cnt, u8 cmdData){
    addInfo(&cmdData, 1);
    addInfo(&cnt, 1);
    addInfo(src, sz);
}

void showWebPckg(){
    for(u16 i = 0; i < webPckg.shift; i++){
        D(printf("%02x", webPckg.buf[i]));
    }
    D(printf("\r\n"));
}