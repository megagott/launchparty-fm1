/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LAUNCHPARTY_MIX_H
#define LAUNCHPARTY_MIX_H
#define LP_B_FIRST 12u
#define LP_B_COUNT 27u
#define LP_B_FADE_MS 300u
/* Keyboard and Auto Party exclude the MYCELIUM and LARS text scrolls. */
static const uint8_t lp_b_scene[LP_B_COUNT]={12,13,16,17,18,34,35,32,33,19,20,21,37,22,23,24,25,26,27,28,38,39,36,40,29,30,31};
static uint8_t lp_b_color;
static uint32_t lp_b_color_time;
static const char *lp_b_color_names[]={"NEON GRUEN","NEON TUERKIS","NEON CYAN","NEON VIOLETT","NEON PINK","NEON BUNT"};
static uint32_t lp_b_tint(uint8_t palette,uint32_t note,uint32_t now) {
    static const uint32_t colors[]={0x20ff00,0x00ff90,0x00b4ff,0x9620ff,0xff0080};
    uint32_t source=lp_palette_rgb[palette],level=source>>16,g=(source>>8)&255,b=source&255,color=lp_b_color,rgb;
    if(g>level)level=g;if(b>level)level=b;
    if(color==5) {
        uint32_t n=note>=36 && note<=99?note-36:note;
        color=(n%4+(n>=32?4:0)+(n%32)/4+now/160)%5;
    }
    rgb=colors[color];
    return 0x1000000u|(((rgb>>16)*level/510)<<16)|(((((rgb>>8)&255)*level/510))<<8)|((rgb&255)*level/510);
}
typedef struct {
    lp_player_t player;
    uint32_t keys, released;
    uint32_t duck_time, transition_time, previous[128];
    int32_t key;
    uint8_t fading, duck, transition, snapshot[128];
} lp_mix_t;
static void lp_mix_init(lp_mix_t *m,uint32_t now) {
    uint32_t i;uint8_t *p=(uint8_t*)m;
    for(i=0;i<sizeof(*m);i++)p[i]=0;
    lp_init(&m->player,now);m->key=-1;m->duck=100;m->duck_time=now;
}
static void lp_mix_clear(lp_mix_t *m) {
    lp_stop(&m->player);m->keys=0;m->key=-1;m->fading=0;m->duck=100;m->transition=0;
}
static void lp_mix_keys(lp_mix_t *m,uint32_t keys,uint32_t now) {
    uint32_t added,k;int32_t next=m->key;
    keys&=(1u<<LP_B_COUNT)-1;added=keys&~m->keys;
    for(k=0;k<LP_B_COUNT;k++)if(added&(1u<<k))next=(int32_t)k;
    if(next>=0 && !(keys&(1u<<next))) {
        next=-1;
        for(k=0;k<LP_B_COUNT;k++)if(keys&(1u<<k))next=(int32_t)k;
    }
    if(next!=m->key) {
        if(next>=0) {
            m->player.last_ms=now;m->player.fraction=0;
            lp_start(&m->player,lp_scene_at(lp_b_scene[next]),LP_LOOP);
            lp_update(&m->player,now);m->fading=0;
        } else {
            for(k=0;k<128;k++)m->snapshot[k]=m->player.frame[k];
            lp_stop(&m->player);m->released=now;m->fading=1;
        }
        m->key=next;
    }
    m->keys=keys;
}
static uint32_t lp_key_rgb(uint32_t key) {
    uint32_t c;
    if(key&0x1000000u)return key&0xffffffu;
    c=lp_palette_rgb[key&127];
    return ((c>>17)<<16)|((((c>>8)&255)/2)<<8)|((c&255)/2);
}
static uint32_t lp_mix_blend(uint32_t a,uint32_t b,uint32_t amount,uint32_t total) {
    uint32_t ar=lp_key_rgb(a),br=lp_key_rgb(b),r,g,v;
    r=((ar>>16)*(total-amount)+(br>>16)*amount)/total;
    g=(((ar>>8)&127)*(total-amount)+((br>>8)&127)*amount)/total;
    v=((ar&127)*(total-amount)+(br&127)*amount)/total;
    return 0x1000000u|(r<<16)|(g<<8)|v;
}
static uint32_t lp_mix_background(lp_mix_t *m,uint32_t n,uint32_t a,uint32_t now) {
    uint32_t age,rgb;
    if(m->transition) {
        age=now-m->transition_time;
        if(age<600)a=lp_mix_blend(m->previous[n],a,age,600);
    }
    if(m->duck<100) {
        rgb=lp_key_rgb(a);
        a=0x1000000u|(((rgb>>16)*m->duck/100)<<16)|(((((rgb>>8)&127)*m->duck/100))<<8)|((rgb&127)*m->duck/100);
    }
    return a;
}
static void lp_mix_transition(lp_mix_t *m,lp_player_t *a,lp_fx_t *fx,uint32_t now) {
    uint32_t n,rgb,key;
    for(n=0;n<128;n++) {
        rgb=lp_fx_rgb(fx,n,a->frame[n],now,a->playing);
        key=fx->brightness==100 && (a->frame[n] || !rgb)?lp_fx_color(a->frame[n],fx->shift):0x1000000u|rgb;
        /* Capture only A, before ducking and before B. */
        if(m->transition && now-m->transition_time<600)
            key=lp_mix_blend(m->previous[n],key,now-m->transition_time,600);
        m->previous[n]=key;
    }
    m->transition_time=now;m->transition=1;
}
static void lp_mix_ambience(lp_mix_t *m,uint32_t now) {
    uint32_t step=(now-m->duck_time)/10,target=100,active=m->key>=0 || (m->fading && now-m->released<LP_B_FADE_MS);
    if(active)target=(m->key>=0 && (m->key<2 || m->key>=24))?45:65;
    if(step>20)step=20;
    if(step) {
        m->duck_time=now;
        if(m->duck>target)m->duck=(uint8_t)(m->duck-target<step?target:m->duck-step);
        else if(m->duck<target)m->duck=(uint8_t)(target-m->duck<step?target:m->duck+step);
    }
    if(m->transition && now-m->transition_time>=600)m->transition=0;
}
/* B is composited after A's effects. Zero is transparent, never a blackout. */
static uint32_t lp_mix_key(lp_mix_t *m,uint32_t n,uint32_t a,uint32_t now) {
    uint32_t b,age,alpha,ar,br,r,g,v;
    a=lp_mix_background(m,n,a,now);
    if(m->key>=0) {b=m->player.frame[n];return b?lp_b_tint((uint8_t)b,n,now):a;}
    if(!m->fading || !(b=m->snapshot[n]))return a;
    age=now-m->released;if(age>=LP_B_FADE_MS)return a;
    alpha=LP_B_FADE_MS-age;ar=lp_key_rgb(a);br=lp_key_rgb(lp_b_tint((uint8_t)b,n,now));
    r=((ar>>16)*(LP_B_FADE_MS-alpha)+(br>>16)*alpha)/LP_B_FADE_MS;
    g=(((ar>>8)&127)*(LP_B_FADE_MS-alpha)+((br>>8)&127)*alpha)/LP_B_FADE_MS;
    v=((ar&127)*(LP_B_FADE_MS-alpha)+(br&127)*alpha)/LP_B_FADE_MS;
    return 0x1000000u|(r<<16)|(g<<8)|v;
}
#endif
