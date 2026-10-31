/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LAUNCHPARTY_PLAYER_H
#define LAUNCHPARTY_PLAYER_H
#include <stdint.h>
typedef struct { const uint8_t *data; uint32_t size; const char *name; } lp_scene_t;
enum { LP_ONCE, LP_LOOP, LP_HOLD };
typedef struct {
    const lp_scene_t *scene, *pending;
    const uint8_t *data;
    uint8_t decoded[8192];
    uint32_t duration, count, cursor, read, next_tick, index, elapsed;
    uint32_t last_ms, fraction, beat_tick, pending_tick, faults, skipped;
    uint16_t bpm;
    uint8_t playing, mode, pending_mode, pending_on;
    uint8_t base[128], frame[128], sent[128];
} lp_player_t;
typedef int (*lp_send_fn)(void *, uint8_t, uint8_t);
static uint32_t lp_u32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static int lp_packed(const lp_scene_t *s) {
    return s && s->size>=8 && s->data[0]=='L' && s->data[1]=='P' && s->data[2]=='C' && s->data[3]=='2';
}
static int lp_unpack(const lp_scene_t *s,uint8_t *out) {
    uint32_t p=8,n=0,size=lp_u32(s->data+4),token,len,d,i;
    if(size<140 || size>8192)return 0;
    while(p<s->size) {
        token=s->data[p++];len=token<128?token+1:(token&127)+3;
        if(len>size-n)return 0;
        if(token<128) {
            if(len>s->size-p)return 0;
            if(out)for(i=0;i<len;i++)out[n+i]=s->data[p+i];
            p+=len;
        } else {
            if(s->size-p<2)return 0;d=s->data[p]|(uint32_t)s->data[p+1]<<8;p+=2;
            if(!d || d>n)return 0;
            if(out)for(i=0;i<len;i++)out[n+i]=out[n+i-d];
        }
        n+=len;
    }
    return n==size;
}
/* LPS2 is validated before any decoder pointer is followed. */
static int lp_validate(const lp_scene_t *s) {
    uint32_t i,p=140,t=0,delta,shift; uint8_t b;
    if(lp_packed(s))return lp_unpack(s,0);
    if (!s || s->size<140 || s->data[0]!='L' || s->data[1]!='P' || s->data[2]!='S' || s->data[3]!='2') return 0;
    if (!lp_u32(s->data+4)) return 0;
    for (i=12;i<140;i++) if(s->data[i]>127) return 0;
    for (i=0;i<lp_u32(s->data+8);i++) {
        delta=shift=0;
        do {
            if (p>=s->size || shift>28) return 0;
            b=s->data[p++];
            if (shift==28 && (b&127)>15) return 0;
            delta|=(uint32_t)(b&127)<<shift; shift+=7;
        } while(b&128);
        if(t+delta<t || t+delta>=lp_u32(s->data+4) || p+2>s->size) return 0;
        t+=delta;
        if(s->data[p]>127 || s->data[p+1]>127) return 0;
        p+=2;
    }
    return p==s->size;
}
static void lp_init(lp_player_t *p,uint32_t now) {
    uint32_t i; uint8_t *b=(uint8_t *)p;
    for(i=0;i<sizeof(*p);i++) b[i]=0;
    p->bpm=120; p->last_ms=now;
    for(i=0;i<128;i++) p->sent[i]=255;
}
static void lp_next(lp_player_t *p) {
    uint32_t delta=0,shift=0; uint8_t b;
    if(p->index>=p->count) {p->next_tick=p->duration;return;}
    do {b=p->data[p->read++];delta|=(uint32_t)(b&127)<<shift;shift+=7;} while(b&128);
    p->next_tick+=delta;
}
static void lp_reset_scene(lp_player_t *p) {
    uint32_t i;
    p->read=140;p->index=0;p->next_tick=0;
    for(i=0;i<128;i++) p->base[i]=p->data[12+i];
    lp_next(p);
}
static int lp_start(lp_player_t *p,const lp_scene_t *s,uint8_t mode) {
    lp_scene_t raw;
    if(mode>LP_HOLD || !lp_validate(s)) {p->faults++;return 0;}
    raw=*s;
    if(lp_packed(s)) {
        if(!lp_unpack(s,p->decoded)){p->faults++;return 0;}
        raw.data=p->decoded;raw.size=lp_u32(s->data+4);
        if(!lp_validate(&raw)){p->faults++;p->playing=0;return 0;}
    }
    p->data=raw.data;
    p->scene=s;p->duration=lp_u32(raw.data+4);p->count=lp_u32(raw.data+8);
    p->elapsed=0;p->playing=1;p->mode=mode;p->pending_on=0;
    lp_reset_scene(p);return 1;
}
static void lp_stop(lp_player_t *p) {
    uint32_t i;p->playing=0;p->pending_on=0;
    for(i=0;i<128;i++) p->base[i]=p->frame[i]=0;
}
static void lp_release(lp_player_t *p,const lp_scene_t *s) {
    if(p->pending_on && p->pending==s && p->pending_mode==LP_HOLD)p->pending_on=0;
    if(p->playing && p->scene==s && p->mode==LP_HOLD)lp_stop(p);
}
static int lp_request(lp_player_t *p,const lp_scene_t *s,uint8_t mode,int quantized) {
    if(mode>LP_HOLD || !lp_validate(s)) {p->faults++;return 0;}
    if(!quantized)return lp_start(p,s,mode);
    p->pending=s;p->pending_mode=mode;p->pending_on=1;
    p->pending_tick=p->beat_tick+(960-p->beat_tick%960);
    return 1;
}
/* Catch up state only; output never replays a stale queue of LED messages. */
static void lp_update(lp_player_t *p,uint32_t now) {
    uint32_t dt=now-p->last_ms,units,advance=0,i,budget=256;
    p->last_ms=now;
    if(dt>60000) {p->faults++;lp_stop(p);return;}
    while(dt) {
        uint32_t chunk=dt>1000?1000:dt; /* safe through 240 BPM x 4 speed */
        units=chunk*(uint32_t)p->bpm*960+p->fraction;
        advance+=units/60000;p->fraction=units%60000;dt-=chunk;
    }
    p->beat_tick+=advance;
    if(p->pending_on && (int32_t)(p->beat_tick-p->pending_tick)>=0) {
        const lp_scene_t *s=p->pending;uint8_t mode=p->pending_mode;
        uint32_t overshoot=p->beat_tick-p->pending_tick;
        lp_start(p,s,mode);advance=overshoot;
    }
    if(p->playing) {
        p->elapsed+=advance;
        if(p->elapsed>=p->duration) {
            if(p->mode==LP_ONCE)lp_stop(p);
            else {p->elapsed%=p->duration;lp_reset_scene(p);}
        }
        while(p->playing && p->index<p->count && p->next_tick<=p->elapsed && budget--) {
            uint8_t pad=p->data[p->read++],color=p->data[p->read++];
            p->base[pad]=color;p->index++;lp_next(p);
        }
    }
    for(i=0;i<128;i++)p->frame[i]=p->base[i];
}
/* Backpressure: sent[] changes ONLY after the transport accepts a message.
 * Round-robin prevents high-index pads starving during animation overload. */
static uint32_t lp_flush(lp_player_t *p,lp_send_fn send,void *ctx,uint32_t max_notes) {
    uint32_t n=0,visited=0;
    while(visited++<128 && n<max_notes) {
        uint32_t k=p->cursor;
        if(p->sent[k]!=p->frame[k]) {
            if(!send(ctx,(uint8_t)k,p->frame[k]))break;
            p->sent[k]=p->frame[k];n++;
        }
        p->cursor=(k+1)&127;
    }
    return n;
}
#endif
