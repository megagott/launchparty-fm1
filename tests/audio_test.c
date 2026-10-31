#include <assert.h>
#include <stdint.h>
#define LAUNCHPARTY 1
static void lp_audio_block(uint32_t n);
static int32_t lp_aftertouch(const void *track,uint32_t note);
#define main upstream_hostsim_main
#include "hostsim.c"
#undef main
#define LAUNCHPARTY_CORE_TEST 1
#include "../firmware/src/launchparty.c"
static void input(unsigned cable,unsigned status,unsigned note,unsigned vel) {
    midi_in_q[mi_w%MQ]=(status>>4)|(cable<<4)|(status<<8)|(note<<16)|(vel<<24);mi_w++;
}
static int gates(unsigned track) {
    unsigned i;int n=0;for(i=0;i<NVOICE;i++)if(trk[track].v[i].active && trk[track].v[i].gate)n++;return n;
}
int main(void) {
    unsigned i,b,peak=0;int32_t out[CTL*2];
    host_tracks_init();
    for(i=0;i<NPART;i++){host_preset(&trk[i],0,0);trk[i].p[P_AMODE]=0;}
    song.master_q12=2048;song.sel=0;mycelium_start_ready=0;
    lp_mode=0;input(0,0x90,60,100);
    for(b=0;b<100;b++){mix_block(out,CTL);for(i=0;i<CTL*2;i++){unsigned x=out[i]<0?-out[i]:out[i];if(x>peak)peak=x;}}
    assert(peak>0 && peak<32768);assert(gates(0)>0);assert(!song.playing && !song.rec);
    input(0,0xA0,60,100);mix_block(out,CTL);assert(lp_aftertouch(&trk[0],60)==9600);assert(lp_aftertouch(&trk[1],60)==0);
    input(0,0x80,60,0);mix_block(out,CTL);assert(gates(0)==0);
    assert(lp_aftertouch(&trk[0],60)==0);
    lp_mode=1;input(0,0x90,61,100);mix_block(out,CTL);assert(gates(0)==0);
    lp_mode=2;input(0,0x90,62,100);mix_block(out,CTL);assert(gates(0)>0);
    panic_req=7;mix_block(out,CTL);assert(gates(0)==0);
    input(1,0x90,60,100);input(1,0x91,60,100);mix_block(out,CTL);assert(gates(0)>0 && gates(1)>0);
    input(1,0x80,60,0);mix_block(out,CTL);assert(gates(0)==0 && gates(1)>0);
    input(1,0x81,60,0);mix_block(out,CTL);assert(gates(1)==0);
    printf("PASS: actual synth DSP peak %u, note release, silent show pads, visual jam, panic, independent TRS tracks; no musical transport\n",peak);
    return 0;
}
