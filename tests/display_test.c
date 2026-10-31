
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define C_BLACK 0
#define C_WHITE 1
#define C_HI 2
#define C_GRAY 3
#define RGB(r,g,b) ((uint16_t)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3)))
static unsigned fm1_ms,lp_last_draw,lp_mode=1,lp_scene_index,lp_programmer=1,lp_din=1,lp_transport_up,lp_configured;
static struct { unsigned playing,pending_on; uint8_t frame[128],sent[128]; } lp;
static struct { unsigned force; } ui;
static struct {const char*name;} lp_bank[]={{"Scene 1"},{"Scene 2"}};
#include "../firmware/src/launchparty_fx.h"
static lp_fx_t lp_fx={.brightness=100,.speed=3};
static unsigned lp_effect_path;
static uint32_t lp_fx_sent[128];
static int FONT_S;
static unsigned fills,texts,clears;
static void lcd_fill(unsigned x,unsigned y,unsigned w,unsigned h,unsigned c) {fills++;if(w==240 && h==240)clears++;}
static void draw_text_box(unsigned x,unsigned y,unsigned w,const int*f,const char*s,unsigned c,int a) {texts++;}
static void str_cpy(char*d,const char*s,unsigned n) {snprintf(d,n,"%s",s);}

#include "../firmware/src/launchparty_layout.h"
static uint16_t lp_preview_color(uint32_t note) {
    uint32_t key=lp_effect_path==1?lp_fx_sent[note]:lp.sent[note],rgb;
    if(key==0xffffffffu || key==255 || key==0 || key==0x1000000u)return RGB(7,16,28);
    if(key&0x1000000u)
        return RGB(((key>>16)&127)*255/127,((key>>8)&127)*255/127,(key&127)*255/127);
    if(key>=128)return RGB(7,16,28);
    rgb=lp_palette_rgb[key];return RGB(rgb>>16,(rgb>>8)&255,rgb&255);
}
static void lp_digits(char *p,uint32_t value,uint32_t digits) {
    while(digits){p[--digits]=(char)('0'+value%10);value/=10;}
}
static void lp_draw(void) {
    static uint8_t valid,last_mode,last_status,last_programmer,last_din,last_up,last_configured;
    static uint32_t last_scene;
    static uint16_t last_cells[64];
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
        draw_text_box(0,4,240,&FONT_S,"LAUNCHPARTY 0.2.3 DEV",RGB(120,255,30),1);
    }
    if(!valid || last_mode!=lp_mode)
        draw_text_box(0,25,240,&FONT_S,lp_mode==1?"SHOWS":lp_mode==0?"SYNTH":"VISUAL JAM",RGB(255,30,180),1);
    if(!valid || last_scene!=lp_scene_index) {
        str_cpy(b,lp_bank[lp_scene_index].name,sizeof b);
        draw_text_box(4,47,232,&FONT_S,b,RGB(30,220,255),1);
    }
    for(i=0;i<64;i++) {
        uint16_t c=lp_preview_color(lp_source_note(i%8,i/8));
        if(!valid || last_cells[i]!=c)
            lcd_fill(52+(i%8)*17,79+(7-i/8)*13,14,10,c);
        last_cells[i]=c;
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
int main(void) {
fm1_ms=100;lp_draw(); assert(fills==68 && texts==6 && clears==0);
fills=texts=0;fm1_ms+=100;lp_draw();assert(!fills && !texts);
lp.frame[36]=53;fm1_ms+=100;lp_draw();assert(fills==0);lp.sent[36]=53;fm1_ms+=100;lp_draw();assert(fills==1 && texts==0 && clears==0);
fills=texts=0;lp_effect_path=1;lp_fx_sent[36]=0x1000000u|0x3f003f;fm1_ms+=100;lp_draw();assert(fills==1 && lp_preview_color(36)==RGB(126,0,126));
fills=texts=0;lp_fx_sent[36]=0x1000000u|0x1f001f;fm1_ms+=100;lp_draw();assert(fills==1 && lp_preview_color(36)==RGB(62,0,62));
fills=texts=0;lp_fx_sent[36]=0;fm1_ms+=100;lp_draw();assert(fills==1 && lp_preview_color(36)==RGB(7,16,28));
fills=texts=0;lp_effect_path=0;fm1_ms+=100;lp_draw();assert(fills==1 && lp_preview_color(36)==RGB(255,0,255));
fills=texts=0;lp.playing=1;fm1_ms+=100;lp_draw();assert(!fills && texts==1);
texts=0;lp_scene_index=1;fm1_ms+=100;lp_draw();assert(texts==1);
texts=0;lp_transport_up=1;lp_configured=1;fm1_ms+=100;lp_draw();assert(texts==1);
texts=fills=0;ui.force=1;fm1_ms+=100;lp_draw();assert(texts==6 && fills==68 && clears==0);
puts("PASS: sent-state only, RGB dimming/tails, blackout, native restore; no full-screen erases; idle draws nothing; changed cell/status/scene/link only; forced redraw");
}
