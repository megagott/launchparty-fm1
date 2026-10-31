
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../firmware/src/launchparty_layout.h"
#include "../firmware/src/launchparty_fx.h"
static struct {unsigned cursor,playing;uint8_t frame[128];}lp;
static lp_fx_t lp_fx;
static uint32_t lp_fx_sent[128];static unsigned lp_mode=1,accept=1,sends,len;
static uint8_t wire[88];
static int lp_send_sysex(const uint8_t*b,uint32_t n) {assert(n<=88);if(!accept)return 0;memcpy(wire,b,n);len=n;sends++;return 1;}
static void lp_flush_show(uint32_t now) {
    uint8_t b[88]={240,0,32,41,2,14,3},pads[16];
    uint32_t keys[16],len=7,count=0,visited=0,k,c,rgb,key;
    while(visited++<128 && count<16) {
        k=lp.cursor;lp.cursor=(k+1)&127;
        if(lp_programmer_note(k)==255)continue;
        c=lp_fx_color(lp.frame[k],lp_fx.shift);
        rgb=lp_fx_rgb(&lp_fx,k,lp.frame[k],now,lp.playing || lp_mode!=1);
        /* Lit pads at full brightness use the exact original hardware palette.
         * Only dimmed pads/tails need RGB approximation. */
        key=lp_fx.brightness==100 && (lp.frame[k] || !rgb)?c:0x1000000u|rgb;
        if(lp_fx_sent[k]==key)continue;
        pads[count]=(uint8_t)k;keys[count++]=key;
        b[len++]=(key&0x1000000u)?3:0;b[len++]=lp_programmer_note(k);
        if(key&0x1000000u){b[len++]=(uint8_t)(rgb>>16);b[len++]=(uint8_t)(rgb>>8);b[len++]=(uint8_t)rgb;}
        else b[len++]=(uint8_t)c;
    }
    if(!count)return;
    b[len++]=247;
    if(lp_send_sysex(b,len))for(k=0;k<count;k++)lp_fx_sent[pads[k]]=keys[k];
}
int main(void){
unsigned i;lp_fx_init(&lp_fx);lp.playing=1;
for(i=0;i<128;i++)lp_fx_sent[i]=0xffffffffu;
for(i=0;i<8;i++)lp_flush_show(0);
assert(lp_fx_sent[36]==0 && lp_fx_sent[99]==0);
sends=0;lp.frame[36]=5;lp.cursor=36;lp_flush_show(10);
assert(sends==1 && len==11 && wire[7]==0 && wire[8]==11 && wire[9]==5 && wire[10]==247);
sends=0;lp_flush_show(20);assert(sends==0);
lp_fx.brightness=50;lp.cursor=36;accept=0;lp_flush_show(30);assert(lp_fx_sent[36]==5);
accept=1;lp.cursor=36;lp_flush_show(40);assert(wire[7]==3 && wire[8]==11 && wire[9]==63 && wire[10]==0 && wire[11]==0);
assert(lp_fx_sent[36]==(0x1000000u|0x3f0000));
lp_fx.brightness=100;lp.cursor=36;lp_flush_show(50);assert(wire[7]==0 && wire[9]==5);
lp_fx.tail_ms=1000;lp.cursor=36;lp_flush_show(100);lp.frame[36]=0;lp.cursor=36;lp_flush_show(600);assert(wire[7]==3 && wire[9]==63);
lp.playing=0;lp.cursor=36;lp_flush_show(601);assert(wire[7]==0 && wire[9]==0);
puts("PASS: actual output batches, correct Programmer index, native palette bypass, RGB dimming, queue backpressure, restore and blackout");
}
