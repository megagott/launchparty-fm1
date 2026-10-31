#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/launchparty_player.h"
static unsigned char data[148];
static lp_scene_t scene={data,sizeof data,"fixture"};
static unsigned count,capacity=128;static unsigned char output[128];
static int sink(void *ctx,uint8_t n,uint8_t c) {(void)ctx;if(!capacity)return 0;capacity--;output[n]=c;count++;return 1;}
int main(void) {
    lp_player_t p;unsigned i;memcpy(data,"LPS2",4);data[4]=0x80;data[5]=7;data[8]=2;
    data[12+36]=53;
    data[140]=0xC0;data[141]=7;data[142]=36;data[143]=0;
    data[144]=0xC0;data[145]=7;data[146]=123;data[147]=49;
    /* duration = 1920; second event must be before the end. */
    data[144]=0x80;data[145]=7; /* 896 delta; absolute 1856 */
    assert(lp_validate(&scene));lp_init(&p,0);assert(lp_start(&p,&scene,LP_LOOP));
    lp_update(&p,0);assert(p.frame[36]==53);
    capacity=0;assert(lp_flush(&p,sink,0,128)==0 && p.sent[36]==255);
    capacity=128;assert(lp_flush(&p,sink,0,128)==128 && output[36]==53);
    lp_update(&p,500);assert(p.frame[36]==0);
    lp_update(&p,980);assert(p.frame[123]==49);
    lp_update(&p,1000);assert(p.frame[36]==53 && p.frame[123]==0);
    assert(lp_start(&p,&scene,LP_ONCE));lp_update(&p,2000);assert(!p.playing);
    assert(lp_start(&p,&scene,LP_HOLD));lp_release(&p,&scene);assert(!p.playing);
    assert(lp_request(&p,&scene,LP_HOLD,1));lp_release(&p,&scene);assert(!p.pending_on);
    assert(lp_request(&p,&scene,LP_ONCE,1));lp_update(&p,2499);assert(!p.playing);lp_update(&p,2500);assert(p.playing && p.elapsed==0);
    lp_stop(&p);capacity=128;lp_flush(&p,sink,0,128);for(i=0;i<128;i++)assert(output[i]==0);
    data[147]=255;assert(!lp_start(&p,&scene,LP_LOOP));
    puts("PASS: loops, one-shot, hold/release, beat launch, initial state, blackout, backpressure and malformed bank");
    return 0;
}
