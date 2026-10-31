#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_mix.h"
#include "../firmware/src/launchparty_touch.h"
int main(void) {
    unsigned mode,t;
    for(mode=0;mode<35;mode++) {
        lp_touch_clear();lp_touch.mode=mode;lp_touch.duck_time=1000;
        for(t=1000;t<=5000;t++) {
            if(t%20==0)lp_touch_press((t/20)%64,127,t);
            lp_touch_service(t);
            assert(lp_touch.duck>=400 && lp_touch.duck<=1000);
            assert(lp_touch_background(3)&0xffffffu);
        }
        assert(lp_touch.duck==400);
        for(t=5001;t<7000;t++) {unsigned k;for(k=0;k<64;k++)lp_touch_release(k,t);lp_touch_service(t);assert(lp_touch.duck>=400);}
        assert(lp_touch.duck==1000);
    }
    puts("PASS: all35 effects retain at least40% background during held pads and rapid retriggers; no blackout; full recovery after release.");
}
