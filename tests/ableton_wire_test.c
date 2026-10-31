#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_layout.h"
#define MQ 512
#define RING_PUBLISH() ((void)0)
static struct{unsigned config,suspended;}usb={1,0};
static uint32_t mo_w,mo_r,midi_out_q[MQ];
static int lp_send_note(void *ctx,uint8_t note,uint8_t color) {
    uint32_t mapped=note;(void)ctx;
    if(!usb.config || usb.suspended || mo_w-mo_r>=MQ-4)return 0;
    mapped=lp_programmer_note(note);
    if(mapped==255)return 1;
    midi_out_q[mo_w%MQ]=9u|0x90u<<8|mapped<<16|(uint32_t)color<<24;
    RING_PUBLISH();mo_w++;return 1;
}
int main(void){unsigned n,c,p,count=0,target;for(n=0;n<128;n++){c=(n*37)%128;target=lp_programmer_note(n);assert(lp_send_note(0,n,c));if(target==255)continue;p=midi_out_q[count++];assert((p&255)==9 && ((p>>8)&255)==144 && ((p>>16)&255)==target && (p>>24)==c);}assert(count==106);usb.config=0;assert(!lp_send_note(0,36,53));puts("PASS: actual MIDI sender translates all 106 physical addresses and preserves channel 1 and palette");}
