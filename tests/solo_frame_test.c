#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_mix.h"
#include "../firmware/src/launchparty_layout.h"
#include "../firmware/src/launchparty_touch.h"
static unsigned light(unsigned n,unsigned t){return (lp_touch_key(n,0,t)&0xffffffu)!=0;}
int main(void) {
    static const unsigned wire[]={91,92,93,94,95,96,97,98,89,79,69,59,49,39,29,19,108,107,106,105,104,103,102,101,10,20,30,40,50,60,70,80};
    unsigned n,t,k,count,ring_count,seen[32]={0};
    for(n=0;n<128;n++) {
        int index=lp_touch_ring_index(n);
        if(index>=0){assert(index<32 && !seen[index]);seen[index]=1;assert(lp_programmer_note(n)==wire[index]);}
    }
    for(n=0;n<32;n++)assert(seen[n]);
    for(k=0;k<64;k++) {
        lp_touch_clear();lp_touch.mode=0;lp_touch_press(k,127,1000);lp_touch.duck_time=1000;
        for(t=1010;t<=1060;t+=10)lp_touch_service(t);
        assert(lp_touch.duck==400);assert(lp_touch_background(53)&0xffffffu);count=0;
        for(n=0;n<128;n++)if(lp_programmer_note(n)!=255 && light(n,1100))count++;
        assert(count==1 && light(lp_source_note(k%8,k/8),1100));
        assert(light(lp_source_note(k%8,k/8),5000));
        lp_touch_release(k,5000);
        assert(!light(lp_source_note(k%8,k/8),5120));
    }
    lp_touch_clear();lp_touch.mode=30;lp_touch_press(27,127,1000);lp_touch.duck_time=1000;
    for(t=1010;t<=1060;t+=10)lp_touch_service(t);
    for(t=1060;t<1960;t+=30) {
        count=ring_count=0;
        for(n=0;n<128;n++)if(lp_programmer_note(n)!=255 && light(n,t)) {
            if(n>=36 && n<=99){assert(n==lp_source_note(3,3));count++;}
            else {assert(lp_touch_ring_index(n)>=0);ring_count++;}
        }
        assert(count==1 && ring_count==5);
    }
    assert(light(lp_source_note(3,3),5000));lp_touch_release(27,5000);
    for(t=5010;t<=5800;t+=10)lp_touch_service(t);
    assert(lp_touch.duck==1000);
    lp_touch_clear();lp_touch_press(0,127,6000);lp_touch_release(0,6030);
    assert(light(lp_source_note(0,0),6800));
    assert(lp_touch_key(lp_source_note(0,0),0,7100)==0);
    puts("PASS: solo on all64 pads; held/release lifecycle; all32 physical Programmer perimeter positions; one grid pad plus five-LED comet; held laps, short-tap lap and brightness recovery.");
}
