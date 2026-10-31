#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint16_t screen[240*240];static unsigned clears;
static void lcd_sync(void){}
static void lcd_fill(unsigned x,unsigned y,unsigned w,unsigned h,uint16_t c){unsigned a,b;assert(x+w<=240 && y+h<=240);if(w==240&&h==240)clears++;for(b=y;b<y+h;b++)for(a=x;a<x+w;a++)screen[b*240+a]=c;}
static void lcd_blit(unsigned x,unsigned y,unsigned w,unsigned h,const uint16_t*p){unsigned a,b;assert(x+w<=240&&y+h<=240);for(b=0;b<h;b++)for(a=0;a<w;a++){unsigned c=p[b*w+a];screen[(y+b)*240+x+a]=(c>>8)|(c<<8);}}
#include "../firmware/src/gfx.c"
#include "../firmware/src/splash.c"

#include <string.h>
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_layout.h"
static unsigned fm1_ms=100,lp_last_draw,lp_mode=1,lp_scene_index,lp_programmer=1,lp_din,lp_transport_up=1,lp_configured=1;
static struct{unsigned playing,pending_on;uint8_t frame[128];}lp;
static struct{unsigned force;}ui;
static struct{const char*name;}lp_bank[]={{"01 CUT 1 SPORE PORTAL"}};
static lp_fx_t lp_fx;
static void str_cpy(char*d,const char*s,unsigned n){snprintf(d,n,"%s",s);}
static void lp_digits(char *p,uint32_t value,uint32_t digits) {
    while(digits){p[--digits]=(char)('0'+value%10);value/=10;}
}
static void lp_draw(void) {
    static uint8_t valid,last_mode,last_status,last_programmer,last_din,last_up,last_configured;
    static uint32_t last_scene;
    static uint8_t last_cells[64];
    static int16_t last_shift;
    static uint16_t last_brightness,last_tail;
    static uint8_t last_speed;
    char b[32];uint32_t i,status=lp.playing?1:lp.pending_on?2:0;
    if(fm1_ms-lp_last_draw<100)return;lp_last_draw=fm1_ms;
    /* Never erase a live screen before repainting it. Each changed text box
     * is composed off-screen; unchanged labels and cells generate no SPI work. */
    if(ui.force) {valid=0;ui.force=0;}
    if(!valid) {
        lcd_fill(0,0,120,2,RGB(120,255,30));lcd_fill(120,0,120,2,RGB(255,30,180));
        lcd_fill(20,70,100,1,RGB(30,220,255));lcd_fill(120,70,100,1,RGB(180,80,255));
        draw_text_box(0,4,240,&FONT_S,"LAUNCHPARTY 0.2.2 DEV",RGB(120,255,30),1);
    }
    if(!valid || last_mode!=lp_mode)
        draw_text_box(0,25,240,&FONT_S,lp_mode==1?"SHOWS":lp_mode==0?"SYNTH":"VISUAL JAM",RGB(255,30,180),1);
    if(!valid || last_scene!=lp_scene_index) {
        str_cpy(b,lp_bank[lp_scene_index].name,sizeof b);
        draw_text_box(4,47,232,&FONT_S,b,RGB(30,220,255),1);
    }
    for(i=0;i<64;i++) {
        uint32_t c=lp.frame[lp_source_note(i%8,i/8)],rgb=lp_palette_rgb[c];
        if(!valid || last_cells[i]!=c)
            lcd_fill(52+(i%8)*17,79+(7-i/8)*13,14,10,c?RGB(rgb>>16,(rgb>>8)&255,rgb&255):RGB(7,16,28));
        last_cells[i]=(uint8_t)c;
    }
    if(!valid || last_status!=status || last_up!=lp_transport_up || last_configured!=lp_configured)
        draw_text_box(0,186,240,&FONT_S,!lp_transport_up?"USB NOT CONNECTED":!lp_configured?"MIDI CONFIG PENDING":status==1?"PLAYING / USB OK":status==2?"NEXT BEAT / USB OK":"STOPPED / USB OK",!lp_transport_up?RGB(255,70,100):status==1?RGB(120,255,30):RGB(30,220,255),1);
    if(lp_mode==1) {
        if(!valid || last_mode!=lp_mode || last_shift!=lp_fx.shift || last_brightness!=lp_fx.brightness) {
            str_cpy(b,"K1:+00  K2:100%",sizeof b);
            b[3]=lp_fx.shift<0?'-':'+';lp_digits(b+4,(uint32_t)(lp_fx.shift<0?-lp_fx.shift:lp_fx.shift),2);
            lp_digits(b+11,lp_fx.brightness,3);draw_text_box(0,205,240,&FONT_S,b,RGB(255,70,190),1);
        }
        if(!valid || last_mode!=lp_mode || last_speed!=lp_fx.speed || last_tail!=lp_fx.tail_ms) {
            str_cpy(b,"K3:100% K4:0000ms",sizeof b);
            lp_digits(b+3,(uint32_t)lp_speed_q8[lp_fx.speed]*100/256,3);
            lp_digits(b+11,lp_fx.tail_ms,4);draw_text_box(0,222,240,&FONT_S,b,RGB(180,110,255),1);
        }
    } else {
        if(!valid || last_mode!=lp_mode || last_programmer!=lp_programmer)
            draw_text_box(0,205,240,&FONT_S,"PROGRAMMER / 106 LEDS",RGB(255,70,190),1);
        if(!valid || last_mode!=lp_mode || last_din!=lp_din)
            draw_text_box(0,222,240,&FONT_S,lp_din?"DIN RATE / SYNTH KNOBS":"USB RATE / SYNTH KNOBS",RGB(180,110,255),1);
    }
    last_shift=lp_fx.shift;last_brightness=lp_fx.brightness;last_speed=lp_fx.speed;last_tail=lp_fx.tail_ms;
    last_mode=lp_mode;last_scene=lp_scene_index;last_status=(uint8_t)status;
    last_programmer=lp_programmer;last_din=lp_din;last_up=lp_transport_up;last_configured=lp_configured;valid=1;
}
int main(void){unsigned x,y,k;FILE*f;lp_fx_init(&lp_fx);lp.playing=1;for(y=0;y<8;y++)for(x=0;x<8;x++)if(x==y || x+y==7 || y==0)lp.frame[lp_source_note(x,y)]=(x<4?21:53);lp_draw();assert(clears==0);f=fopen("build/ui-neon-preview.ppm","wb");assert(f);fprintf(f,"P6\n240 240\n255\n");for(k=0;k<240*240;k++){unsigned c=screen[k];fputc(((c>>11)&31)*255/31,f);fputc(((c>>5)&63)*255/63,f);fputc((c&31)*255/31,f);}fclose(f);puts("PASS: actual neon UI render within display bounds, no full-screen clear");}
