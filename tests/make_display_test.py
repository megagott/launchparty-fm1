from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'firmware/src/launchparty.c').read_text()
fn=s[s.index('static uint16_t lp_preview_color('):s.rindex('\n#endif')]
pre=r'''
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
'''
post=r'''
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
'''
(root/'tests/display_test.c').write_text(pre+'\n#include "../firmware/src/launchparty_layout.h"\n'+fn+post)
