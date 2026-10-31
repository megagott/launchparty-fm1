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
int main(int argc,char **argv) {
    unsigned e[3]={0,0,1},pr[3]={21,22,29},j,k,b,i,peak;
    const unsigned notes[]={48,55,62,67};int32_t out[CTL*2];
    assert(argc==2);
    for(j=0;j<3;j++) {
        char file[512];FILE *w;double energy=0;peak=0;
        host_tracks_init();
        for(k=0;k<NPART;k++){trk_all_off(&trk[k]);host_preset(&trk[k],0,0);trk[k].p[P_AMODE]=0;}
        host_preset(&trk[0],e[j],pr[j]);trk[0].p[P_AMODE]=0;
        song.master_q12=2048;song.sel=0;mycelium_start_ready=0;lp_mode=0;
        snprintf(file,sizeof(file),"%s/%s.wav",argv[1],ENGINES[e[j]]->presets[pr[j]].name);
        w=fopen(file,"wb");assert(w);wav_hdr(w,FS*12);
        for(k=0;k<4;k++)input_on(&trk[0],notes[k],90);
        for(b=0;b<FS*12/CTL;b++) {
            if(b==FS*6/CTL)for(k=0;k<4;k++)input_off(&trk[0],notes[k]);
            mix_block(out,CTL);
            for(i=0;i<CTL;i++) {
                unsigned ch;
                for(ch=0;ch<2;ch++){int32_t v=out[i*2+ch];unsigned a=v<0?-v:v;if(a>peak)peak=a;energy+=(double)v*v;}
                wav_put(w,out[i*2],out[i*2+1]);
            }
        }
        fclose(w);assert(peak>100 && peak<30000 && energy>0);assert(!song.playing && !song.rec);
        printf("PASS: %s, four-note chord + release, peak %u, RMS %.1f, no musical transport\n",ENGINES[e[j]]->presets[pr[j]].name,peak,sqrt(energy/(FS*12*2)));
    }
}
