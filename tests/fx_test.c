#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_fx.h"
int main(void) {
 lp_fx_t f;unsigned c,level,last;uint32_t rgb;
 lp_fx_init(&f);assert(f.brightness==100 && f.shift==0 && f.tail_ms==0 && f.speed==3);
 for(c=0;c<128;c++)assert(lp_fx_color(c,0)==c);
 assert(lp_fx_color(0,7)==0);assert(lp_fx_color(5,1)==9);assert(lp_fx_color(5,-1)==57);
 for(c=4;c<60;c++)assert((lp_fx_color(c,7)-4)%4==(c-4)%4);
 for(level=0;level<=100;level++) {
  f.brightness=level;rgb=lp_fx_rgb(&f,36,5,100,1);
  assert((rgb>>16)<=127 && ((rgb>>8)&255)<=127 && (rgb&255)<=127);
 }
 f.brightness=0;assert(lp_fx_rgb(&f,36,53,100,1)==0);
 f.brightness=100;f.tail_ms=1000;
 last=lp_fx_rgb(&f,36,5,100,1)>>16;
 for(level=1;level<=1000;level++) {
  rgb=lp_fx_rgb(&f,36,0,100+level,1)>>16;assert(rgb<=last);last=rgb;
 }
 assert(last==0);assert(lp_fx_rgb(&f,36,0,1101,1)==0);
 lp_fx_rgb(&f,36,5,2000,1);assert(lp_fx_rgb(&f,36,0,2001,0)==0 && !f.held[36]);
 /* unsigned millisecond wrap */
 lp_fx_rgb(&f,36,5,0xfffffff0u,1);assert(lp_fx_rgb(&f,36,0,20,1)!=0);
 assert(lp_speed_q8[0]==64 && lp_speed_q8[3]==256 && lp_speed_q8[7]==1024);
 puts("PASS: original colours, shade-preserving palette shift, RGB bounds, brightness zero, monotonic tails, blackout and timer wrap");
}
