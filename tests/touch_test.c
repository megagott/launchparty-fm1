#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_mix.h"
#include "../firmware/src/launchparty_touch.h"
#include "../firmware/src/launchparty_layout.h"
static uint32_t lp_mode=1,lp_programmer=1,fm1_ms=1000,lp_ripple_count;
static uint8_t lp_ripple_pad[8],lp_ripple_color[8];static uint32_t lp_ripple_time[8];
static void lp_change_mode(uint32_t m){lp_mode=m;}
/* Actual packet handler is included by the test preparation script. */
#include "touch_event_actual.h"
int main(void) {
    unsigned x,y,mode,t,n,count;lp_mix_t mix;
    for(y=0;y<8;y++)for(x=0;x<8;x++) {
        unsigned next=lp_touch.next,src=lp_source_note(x,y),wire=11+x+y*10;
        lp_touch_clear();lp_pad_event(9u|0x90u<<8|wire<<16|127u<<24);
        assert(lp_touch.next==next+1 && lp_mode==1);
        assert(lp_touch.hit[next%8].x==x && lp_touch.hit[next%8].y==y);
        assert(lp_touch_key(src,21,1000)==0x010f7f00u);
        lp_pad_event(8u|0x80u<<8|wire<<16);assert(lp_touch.next==next+1);
        assert(lp_touch_key(src,21,1700)==21);
    }
    for(mode=0;mode<35;mode++) {
        static const uint32_t colors[]={0x20ff00,0x00ff90,0x00b4ff,0x9620ff,0xff0080};
        unsigned col=mode%5,prev;
        if((mode/5)&1)col=4-col;
        lp_touch_clear();lp_touch.mode=mode;lp_touch_press(27,110,1000);count=0;
        assert(lp_touch_rgb()==colors[col]);
        if(mode) {
            lp_touch.mode=mode-1;prev=lp_touch_color_index();lp_touch.mode=mode;
            assert(prev==col || prev+1==col || col+1==prev);
            if(mode%5==0)assert(prev==col);
        }
        for(t=1090;t<1690;t+=50)for(n=36;n<100;n++) {
            unsigned key=lp_touch_key(n,0,t);
            if(key){count++;assert((key&255)<=127 && ((key>>8)&255)<=127 && ((key>>16)&255)<=127);assert(lp_touch_key(n,53,t)==key);}
        }
        assert(count>0);
    }
    lp_mix_init(&mix,1000);lp_mix_keys(&mix,1,1000);mix.player.frame[36]=21;
    lp_touch_clear();lp_touch.mode=0;lp_touch_press(0,127,1000);
    assert(lp_touch_key(36,lp_touch_background(lp_mix_key(&mix,36,53,1000)),1000)==0x010f7f00u);
    lp_touch_clear();lp_touch.mode=5;lp_touch_press(27,127,1000);
    count=0;for(y=0;y<8;y++)for(x=0;x<8;x++) {
        unsigned key=lp_touch_key(lp_source_note(x,y),0,1040);
        if(key) {assert(x>=2 && x<=4 && y>=2 && y<=4);count++;}
    }
    assert(count==9); /* eight neighbours plus the brief centre flash */
    for(n=36;n<100;n++)assert(lp_touch_key(n,0,1120)==0);
    for(n=0;n<20;n++)lp_touch_press(n%64,127,1000);
    lp_touch_clear();for(n=36;n<100;n++)assert(lp_touch_key(n,53,1000)==53);
    puts("PASS: actual64 pad inputs; 35 monochrome neon variants with no background hue bleed; adjacent colour order; same-colour shape boundaries; tight blinkring; expiry and panic.");
}
