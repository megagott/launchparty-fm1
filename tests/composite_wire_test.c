#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
#include "../firmware/src/launchparty_layout.h"
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_mix.h"
#include "../firmware/src/launchparty_touch.h"
static lp_player_t lp;static lp_mix_t lp_mix;static lp_fx_t lp_fx;
static uint32_t lp_fx_sent[128];static uint32_t lp_mode=1,accept=1;
static unsigned seen[128],messages;
static uint32_t expected[128];
static int lp_send_sysex(const uint8_t *b,uint32_t len) {
    unsigned p=7,type,n,c;
    if(!accept)return 0;
    assert(len<=88 && b[0]==240 && b[len-1]==247);
    while(p<len-1) {
        type=b[p++];n=b[p++];assert(n<128);
        if(type==3){c=0x1000000u|(unsigned)b[p]<<16|(unsigned)b[p+1]<<8|b[p+2];p+=3;}
        else {assert(type==0);c=b[p++];}
        assert(c==expected[n]);seen[n]=1;messages++;
    }
    assert(p==len-1);return 1;
}
#include "composite_flush_actual.h"
static void check(unsigned now) {
    unsigned n,k,rgb,c;
    for(n=0;n<128;n++)lp_fx_sent[n]=0xffffffffu;
    for(n=0;n<128;n++) {
        if(lp_programmer_note(n)==255)continue;
        c=lp_fx_color(lp.frame[n],lp_fx.shift);
        rgb=lp_fx_rgb(&lp_fx,n,lp.frame[n],now,lp.playing);
        k=lp_fx.brightness==100 && (lp.frame[n] || !rgb)?c:0x1000000u|rgb;
        k=lp_mix_key(&lp_mix,n,k,now);k=lp_touch_key(n,lp_touch_background(k),now);
        expected[lp_programmer_note(n)]=k;lp_fx_sent[n]=0xffffffffu;seen[lp_programmer_note(n)]=0;
    }
    accept=0;lp_flush_show(now);for(n=0;n<128;n++)assert(lp_fx_sent[n]==0xffffffffu);
    accept=1;for(n=0;n<12;n++)lp_flush_show(now);
    for(n=0;n<128;n++)if(lp_programmer_note(n)!=255){assert(seen[lp_programmer_note(n)]);assert(lp_fx_sent[n]==expected[lp_programmer_note(n)]);}
}
int main(void) {
    unsigned n;lp_init(&lp,0);lp_mix_init(&lp_mix,0);lp_fx_init(&lp_fx);lp.playing=1;
    for(n=36;n<100;n++)lp.frame[n]=21;
    check(0);lp_touch_press(0,127,1000);check(1000);
    assert(expected[11]==0x010f7f00u);
    lp_touch_clear();lp_mix_transition(&lp_mix,&lp,&lp_fx,2000);
    for(n=36;n<100;n++)lp.frame[n]=53;
    check(2300);assert(expected[11]!=(0x1000000u|lp_key_rgb(53)));
    lp_mix_keys(&lp_mix,1,3000);lp_mix.player.frame[36]=21;lp_mix.duck=45;check(3000);assert(expected[11]==lp_b_tint(21,36,3000));
    lp_mix_keys(&lp_mix,0,3100);check(3250);
    lp_mix_keys(&lp_mix,1,3500);lp_mix.duck=100;
    for(n=36;n<100;n++)lp_mix.player.frame[n]=21;
    lp_touch.mode=10;lp_touch_press(0,127,3500);lp_touch.duck_time=3500;
    for(n=3510;n<=3570;n+=10)lp_touch_service(n);
    assert(lp_touch.duck==400);check(3570);assert(expected[11]==0x010f7f00u);
    assert(expected[88]==lp_touch_background(lp_b_tint(21,99,3570)));assert(expected[88]!=(unsigned)21);
    for(n=3580;n<=4600;n+=10)lp_touch_service(n);
    assert(lp_touch.duck==1000);check(4600);assert(expected[88]==lp_b_tint(21,99,4600));
    lp_touch_press(27,127,4700);lp_touch_service(4710);assert(lp_touch.duck<1000);
    lp_touch_clear();assert(lp_touch.duck==1000);
    printf("PASS: actual SysEx sender matches final preview/cache for touch, A crossfade, ducking, B fade and native B; backpressure; %u LED messages\n",messages);
}
