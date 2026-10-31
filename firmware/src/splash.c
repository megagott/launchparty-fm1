/* SPDX-License-Identifier: GPL-3.0-only */
#include "launchparty_logo.h"
/* Procedural neon intro: each animation strip is composed off-screen. */
static void lp_boot_frame(uint32_t phase)
{
    uint32_t x,y,k;
    static const uint16_t neon[]={RGB(120,255,30),RGB(255,30,180),RGB(30,220,255),RGB(180,80,255)};
    cv_begin(240,124,RGB(3,5,12));
    for(y=0;y<8;y++)for(x=0;x<8;x++) {
        int32_t dx=(int32_t)x*2-7,dy=(int32_t)y*2-7;
        uint32_t radius=(uint32_t)(dx<0?-dx:dx)+(uint32_t)(dy<0?-dy:dy);
        uint32_t wave=(radius+32-phase%32)%8;
        uint16_t c=wave<2?neon[(phase/8+(x>=4)+2*(y>=4))%4]:wave<4?RGB(12,28,40):RGB(6,10,20);
        cv_rect(61+x*15,2+y*15,12,12,c);
    }
    for(k=0;k<10;k++) {
        uint32_t px=(k*47+phase*3)%240,py=(k*31+phase)%124;
        if(px<52 || px>188)cv_rect(px,py,2,2,neon[k%4]);
    }
    cv_blit(0,54);lcd_sync();
}
static void sloop_splash(void)
{
    lcd_fill(0,0,240,240,C_BLACK);
    lp_logo_draw(0,8,4);
    lp_boot_frame(0);
    cv_begin(240,52,C_BLACK);
    cv_text(120-text_w(&FONT_S,"SOUND INTO LIGHT")/2,0,&FONT_S,"SOUND INTO LIGHT",RGB(40,220,255));
    cv_text(120-text_w(&FONT_S,"MANDALARS.ORG / 0.3.1")/2,24,&FONT_S,"MANDALARS.ORG / 0.3.1",RGB(180,110,255));
    cv_blit(0,186);lcd_sync();
}
