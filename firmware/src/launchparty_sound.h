/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LAUNCHPARTY_SOUND_H
#define LAUNCHPARTY_SOUND_H
static uint8_t lp_sound_value[NPART][4];
static void lp_sound_reset(uint32_t track) {
    lp_sound_value[track][0]=32;lp_sound_value[track][1]=40;
    lp_sound_value[track][2]=20;lp_sound_value[track][3]=60;
}
/* Timbre macros never write level, mute or trim. Hold envelopes retain a floor;
 * no tremolo/gating and no filter sweep to sub-audible cutoff. */
static void lp_sound_apply(track_t *t,uint32_t knob,uint32_t v) {
    int16_t *p=t->p;
    if(v>127)v=127;
    p[P_AMODE]=0;p[P_AHOLD]=0;p[P_LD_AMP]=0;
    if(p[P_SUS]<96)p[P_SUS]=96;
    if(p[P_ATK]>90)p[P_ATK]=90;
    if(p[P_REL]<60)p[P_REL]=60;
    if(knob==0) {
        p[P_ED_SHP]=(int16_t)(v*40/127);
        if(t->eng_req==0) {
            p[P_E0]=(int16_t)(v<32?4:v<64?0:v<96?1:2);
            p[P_E1]=(int16_t)(4+v*66/127);p[P_E2]=64;
            p[P_E3]=(int16_t)(v*12/127);p[P_E6]=(int16_t)(v*32/127);
        } else if(t->eng_req==1) {
            p[P_E0]=(int16_t)(v*7/127);p[P_E1]=(int16_t)(1+v*10/127);
            p[P_E2]=(int16_t)(2+v*12/127);p[P_E4]=(int16_t)(22+v*88/127);
        }
    } else if(knob==1) {
        if(t->eng_req==0) {
            p[P_E4]=(int16_t)(58+v*48/127);p[P_E5]=(int16_t)(12+v*92/127);
            p[P_ED_FLT]=12;
        } else if(t->eng_req==1) {
            p[P_E6]=(int16_t)(v*50/127);p[P_E5]=(int16_t)(40+v*70/127);
            p[P_ED_SHP]=(int16_t)(v*48/127);
        } else p[P_ED_SHP]=(int16_t)(v*48/127);
    } else if(knob==2) {
        p[P_LRATE]=(int16_t)(12+v*105/127);
        p[P_LWAVE]=(int16_t)(v<48?0:v<88?1:4);
        p[P_LD_SHP]=(int16_t)(8+v*44/127);
        p[P_LD_PIT]=(int16_t)(v*24/127);p[P_LD_FLT]=(int16_t)(v*16/127);
        p[P_LFADE]=0;
    } else {
        p[P_CHOR]=(int16_t)(20+v*65/127);
        p[P_DLY]=(int16_t)(10+v*72/127);p[P_REV]=(int16_t)(25+v*65/127);
    }
}
static void lp_sound_turn(uint32_t knob,int32_t delta) {
    uint32_t track=song.sel%NPART;
    int32_t v=lp_sound_value[track][knob]+delta*4;
    lp_sound_value[track][knob]=(uint8_t)clamp(v,0,127);
    lp_sound_apply(&trk[track],knob,lp_sound_value[track][knob]);
}
#endif
