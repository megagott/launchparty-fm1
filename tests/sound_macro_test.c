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
int main(void) {
    unsigned e[]={0,0,1},pr[]={21,22,29},values[]={0,32,64,96,127};
    unsigned j,k,z,b,i,worst=0;int32_t out[CTL*2];
    for(j=0;j<3;j++)for(k=0;k<5;k++)for(z=0;z<5;z++) {
        unsigned peak=0;double energy=0;int level,trim;
        host_tracks_init();
        for(i=0;i<NPART;i++){trk_all_off(&trk[i]);host_preset(&trk[i],0,0);trk[i].p[P_AMODE]=0;}
        host_preset(&trk[0],e[j],pr[j]);song.master_q12=2048;song.sel=0;mycelium_start_ready=0;lp_mode=0;
        level=trk[0].p[P_LEVEL];trim=trk[0].p[P_ED_FX];
        for(i=0;i<4;i++)if(k==4 || i==k)lp_sound_apply(&trk[0],i,values[z]);
        assert(trk[0].p[P_LEVEL]==level && trk[0].p[P_ED_FX]==trim && !trk[0].p[P_LD_AMP]);
        assert(trk[0].p[P_SUS]>=96 && !trk[0].p[P_MUTE]);
        for(i=0;i<P_E0;i++)assert(trk[0].p[i]>=TP[i].min && trk[0].p[i]<=TP[i].max);
        for(i=0;i<8;i++)assert(trk[0].p[P_E0+i]>=ENGINES[e[j]]->edit[i].min && trk[0].p[P_E0+i]<=ENGINES[e[j]]->edit[i].max);
        input_on(&trk[0],48,90);input_on(&trk[0],55,90);input_on(&trk[0],62,90);input_on(&trk[0],67,90);
        for(b=0;b<FS*3/CTL;b++) {
            mix_block(out,CTL);
            for(i=0;i<CTL*2;i++){unsigned a=out[i]<0?-out[i]:out[i];if(a>peak)peak=a;if(b>FS/CTL)energy+=(double)out[i]*out[i];}
        }
        if(peak>worst)worst=peak;
        assert(energy>1000000 && peak>100 && peak<32767);
        assert(!song.playing && !song.rec);
    }
    printf("PASS: 75 four-note DSP scenarios, single/all macro extremes; sustained signal, bounded parameters, unchanged level/trim, peak %u\n",worst);
}
