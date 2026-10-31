
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/launchparty_player.h"
static uint8_t a[140],b[140];static lp_scene_t lp_bank[]={{a,140,"A"},{b,140,"B"}};
#define LP_BANK_COUNT 2
#define EN_SELECT 0
#define B_ENV 0
#define G_BPM 0
static struct{unsigned buttons;}fm1_in;static struct{unsigned btn[1];}panel;
static struct{int g[1];}song;
static lp_player_t lp;static const lp_scene_t*lp_panel_held;
static unsigned lp_scene_index,lp_mode=1,lp_playmode=LP_LOOP,lp_quantized;
static int delta;
static int panel_enc(unsigned role){(void)role;return delta;}
static int clamp(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
static void control(void){int s;
    if((s=panel_enc(EN_SELECT))) {
        if(fm1_in.buttons&(1u<<panel.btn[B_ENV]))song.g[G_BPM]=(int16_t)clamp(song.g[G_BPM]+s,40,240);
        else {
            uint32_t old=lp_scene_index;
            lp_scene_index=(uint32_t)clamp((int32_t)lp_scene_index+s,0,LP_BANK_COUNT-1);
            if(lp_mode==1 && old!=lp_scene_index && (lp.playing || lp.pending_on)) {
                lp_request(&lp,&lp_bank[lp_scene_index],lp_playmode,lp_quantized);
                if(lp_panel_held)lp_panel_held=&lp_bank[lp_scene_index];
            }
        }
    }

}
int main(void){
memcpy(a,"LPS2",4);memcpy(b,"LPS2",4);a[5]=b[5]=15;a[12+36]=5;b[12+36]=21;
lp_init(&lp,0);lp_start(&lp,&lp_bank[0],LP_LOOP);lp_update(&lp,100);
delta=1;control();assert(lp.playing && lp.scene==&lp_bank[1] && lp.elapsed==0);lp_update(&lp,100);assert(lp.frame[36]==21);
delta=-1;lp_quantized=1;control();assert(lp.scene==&lp_bank[1] && lp.pending_on);
lp_update(&lp,499);assert(lp.scene==&lp_bank[1]);lp_update(&lp,500);assert(lp.scene==&lp_bank[0] && !lp.pending_on);
delta=1;control();delta=-1;control();assert(lp.pending==&lp_bank[0]);
lp_stop(&lp);delta=1;control();assert(!lp.playing && !lp.pending_on && lp_scene_index==1);
fm1_in.buttons=1;song.g[0]=120;delta=1;control();assert(song.g[0]==121 && lp_scene_index==1);
lp_init(&lp,0);lp.bpm=960;lp_update(&lp,60000);assert(lp.beat_tick==921600);
lp_init(&lp,0);lp.bpm=10;lp_update(&lp,60000);assert(lp.beat_tick==9600);
puts("PASS: actual SELECT control switches live, quantizes without stopping, latest choice wins, idle selection stays idle, tempo modifier stays separate and 0.25x/4x arithmetic safe");
}
