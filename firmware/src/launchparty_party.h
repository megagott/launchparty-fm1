/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LAUNCHPARTY_PARTY_H
#define LAUNCHPARTY_PARTY_H
typedef struct {
    uint32_t rng,last_ms,tick,fraction,next_show,next_overlay,overlay_end,last_motion,last_target,last_hue;
    uint8_t on,manual_knobs,manual_keys,phase;
    int32_t overlay;
    int16_t shift;
    uint16_t brightness,tail;
    uint8_t speed;
} lp_party_t;
static uint32_t lp_party_random(lp_party_t *p) {
    uint32_t r=p->rng;r^=r<<13;r^=r>>17;r^=r<<5;p->rng=r;return r;
}
static int32_t lp_party_toward(int32_t v,int32_t target,int32_t step) {
    if(v<target)return target-v<step?target:v+step;
    return v-target<step?target:v-step;
}
static void lp_party_begin(lp_party_t *p,lp_player_t *a,uint32_t *scene,uint32_t now) {
    uint32_t i;uint8_t *b=(uint8_t*)p;
    for(i=0;i<sizeof(*p);i++)b[i]=0;
    p->rng=now^0x9e3779b9u;if(!p->rng)p->rng=1;
    p->on=1;p->last_ms=now;p->last_motion=now;p->overlay=-1;
    p->brightness=80;p->speed=2;p->tail=900;
    p->last_target=0xffffffffu;
    p->next_show=16*3840;p->next_overlay=8*3840;
    if(*scene>=LP_B_FIRST)*scene=0;
    lp_start(a,lp_scene_at(*scene),LP_LOOP);
}
static void lp_party_end(lp_party_t *p,lp_mix_t *mix,uint32_t keys,uint32_t now) {
    p->on=0;p->overlay=-1;lp_mix_keys(mix,keys,now);
}
/* Colour/movement families follow the authored best-of sequence. */
static uint32_t lp_party_b(lp_party_t *p,uint32_t a) {
    uint32_t r=lp_party_random(p);
    if(a<2)return r%7;
    if(a<4)return 7+r%7;
    if(a==4)return 2+r%4;
    if(a<9)return 22+r%5;
    if(a<11)return 16+r%6;
    return 13+r%3;
}
static void lp_party_step(lp_party_t *p,lp_player_t *a,lp_mix_t *mix,lp_fx_t *fx,
                          uint32_t *scene,uint32_t keys,uint32_t bpm,uint32_t now) {
    uint32_t dt,units,bar,r,position;
    if(!p->on)return;
    dt=now-p->last_ms;p->last_ms=now;
    if(dt>60000)dt=0;
    while(dt) {
        uint32_t chunk=dt>1000?1000:dt;
        units=chunk*bpm*960+p->fraction;p->tick+=units/60000;p->fraction=units%60000;dt-=chunk;
    }
    bar=p->tick/3840;
    position=bar%64;
    p->phase=(uint8_t)(position<16?0:position<40?1:position<56?2:3);
    if((int32_t)(p->tick-p->next_show)>=0) {
        r=lp_party_random(p);
        /* Occasional repeats, otherwise the next authored neighbour. */
        if(r%(p->phase==0?3:5) || *scene>=LP_B_FIRST)*scene=(*scene+1)%LP_B_FIRST;
        lp_mix_transition(mix,a,fx,now);
        lp_start(a,lp_scene_at(*scene),LP_LOOP);
        p->next_show=(bar+(p->phase==0?16:p->phase==2?4+4*(lp_party_random(p)%2):8))*3840;
    }
    keys&=(1u<<LP_B_COUNT)-1;
    if(keys) {p->manual_keys=1;p->overlay=-1;}
    else if(p->manual_keys) {p->manual_keys=0;p->next_overlay=(bar+4)*3840;}
    if(!keys && !p->manual_keys && (int32_t)(p->tick-p->next_overlay)>=0) {
        p->overlay=(int32_t)lp_party_b(p,*scene);
        p->overlay_end=(bar+(p->phase==3?4:2))*3840;
        p->next_overlay=(bar+(p->phase==0?16:p->phase==2?4+4*(lp_party_random(p)%2):8+4*(lp_party_random(p)%2)))*3840;
    }
    if(p->overlay>=0 && (int32_t)(p->tick-p->overlay_end)>=0)p->overlay=-1;
    lp_mix_keys(mix,keys?keys:p->overlay>=0?1u<<p->overlay:0,now);
    if(bar/4!=p->last_target) {
        p->last_target=bar/4;r=lp_party_random(p);
        if(p->phase==0) {
            p->shift=(int16_t)((int32_t)(r%3)-1);p->brightness=(uint16_t)(65+r%16);
            p->tail=(uint16_t)(800+lp_party_random(p)%5*100);p->speed=2;
        } else if(p->phase==1) {
            p->shift=(int16_t)((int32_t)(r%5)-2);p->brightness=(uint16_t)(80+r%16);
            p->tail=(uint16_t)(300+lp_party_random(p)%5*100);p->speed=3;
        } else if(p->phase==2) {
            p->shift=(int16_t)((int32_t)(r%9)-4);p->brightness=(uint16_t)(95+r%6);
            p->tail=(uint16_t)(lp_party_random(p)%3*100);p->speed=4;
        } else {
            p->shift=(int16_t)((int32_t)(r%3)-1);p->brightness=(uint16_t)(65+r%11);
            p->tail=(uint16_t)(1000+lp_party_random(p)%6*100);p->speed=2;
        }
    }
    if(bar!=p->last_hue) {
        p->last_hue=bar;
        if(!(p->manual_knobs&1))fx->shift=(int16_t)lp_party_toward(fx->shift,p->shift,1);
        if(!(p->manual_knobs&4))fx->speed=(uint8_t)lp_party_toward(fx->speed,p->speed,1);
    }
    if(now-p->last_motion>=100) {
        p->last_motion=now;
        if(!(p->manual_knobs&2))fx->brightness=(uint16_t)lp_party_toward(fx->brightness,p->brightness,1);
        if(!(p->manual_knobs&8))fx->tail_ms=(uint16_t)lp_party_toward(fx->tail_ms,p->tail,25);
    }
}
#endif
