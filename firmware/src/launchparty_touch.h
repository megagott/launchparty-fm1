/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LAUNCHPARTY_TOUCH_H
#define LAUNCHPARTY_TOUCH_H
typedef struct { uint32_t time,released;uint8_t x,y,velocity,active,held,ring_start; } lp_touch_hit_t;
typedef struct {lp_touch_hit_t hit[8];uint32_t next,duck_time;uint16_t duck;uint8_t mode;} lp_touch_t;
static lp_touch_t lp_touch={.duck=1000};
#define LP_TOUCH_VARIANTS 35
static uint32_t lp_touch_color_index(void) {
    uint32_t c=lp_touch.mode%5;
    return (lp_touch.mode/5)&1?4-c:c;
}
static uint32_t lp_touch_rgb(void) {
    static const uint32_t colors[]={0x20ff00,0x00ff90,0x00b4ff,0x9620ff,0xff0080};
    return colors[lp_touch_color_index()];
}
static uint32_t lp_touch_shape(void) {
    static const uint8_t shapes[]={5,4,1,0,2,3,6};
    return shapes[lp_touch.mode/5];
}
static int lp_touch_alive(const lp_touch_hit_t *h,uint32_t now) {
    uint32_t shape=lp_touch_shape();
    if(!h->active)return 0;
    if(shape==5)return h->held || now-h->released<120;
    if(shape==6)return h->held || now-h->time<1000 || now-h->released<180;
    return now-h->time<700;
}
static void lp_touch_clear(void) {
    uint32_t i;for(i=0;i<8;i++)lp_touch.hit[i].active=0;lp_touch.duck=1000;
}
static void lp_touch_service(uint32_t now) {
    uint32_t i,active=0,dt=now-lp_touch.duck_time,step,target;
    lp_touch.duck_time=now;if(dt>50)dt=50;
    for(i=0;i<8;i++)if(lp_touch_alive(&lp_touch.hit[i],now))active=1;
    target=active?400:1000;
    if(lp_touch.duck>target) {
        step=dt*(target?10:20);lp_touch.duck=(uint16_t)(lp_touch.duck-target<step?target:lp_touch.duck-step);
    } else if(lp_touch.duck<target) {
        step=dt*2;lp_touch.duck=(uint16_t)(target-lp_touch.duck<step?target:lp_touch.duck+step);
    }
}
static uint32_t lp_touch_background(uint32_t key) {
    uint32_t rgb;
    if(lp_touch.duck==1000)return key;
    rgb=lp_key_rgb(key);
    return 0x1000000u|(((rgb>>16)*lp_touch.duck/1000)<<16)|(((((rgb>>8)&127)*lp_touch.duck/1000))<<8)|((rgb&127)*lp_touch.duck/1000);
}
static void lp_touch_press(uint32_t pad,uint32_t velocity,uint32_t now) {
    uint32_t slot=lp_touch.next++%8;
    lp_touch_hit_t *h=&lp_touch.hit[slot];
    h->x=(uint8_t)(pad%8);h->y=(uint8_t)(pad/8);
    h->time=h->released=now;h->velocity=(uint8_t)velocity;h->active=h->held=1;
    {uint32_t d=7-h->y;h->ring_start=h->x;
     if(7-h->x<d){d=7-h->x;h->ring_start=(uint8_t)(8+7-h->y);}
     if(h->y<d){d=h->y;h->ring_start=(uint8_t)(16+7-h->x);}
     if(h->x<d)h->ring_start=(uint8_t)(24+h->y);}
}
static void lp_touch_release(uint32_t pad,uint32_t now) {
    uint32_t i;for(i=0;i<8;i++) {
        lp_touch_hit_t *h=&lp_touch.hit[i];
        if(h->active && h->held && h->x==pad%8 && h->y==pad/8){h->held=0;h->released=now;}
    }
}
/* Immediate physical perimeter: top, right, Track Select row, left.
 * The second bottom row and the logo/Shift corners are not in this ring. */
static int32_t lp_touch_ring_index(uint32_t note) {
    if(note>=28 && note<=35)return (int32_t)note-28;
    if(note>=100 && note<=107)return 8+(int32_t)note-100;
    if(note>=116 && note<=123)return 16+123-(int32_t)note;
    if(note>=108 && note<=115)return 24+115-(int32_t)note;
    return -1;
}
static uint32_t lp_touch_key(uint32_t note,uint32_t key,uint32_t now) {
    uint32_t n,x=0,y=0,i,age,radius,distance=0,level,rgb=lp_touch_rgb(),total=0,lit,shape=lp_touch_shape(),grid=note>=36 && note<=99;
    int32_t ring=lp_touch_ring_index(note);
    if(!grid && (shape!=6 || ring<0))return key;
    if(grid){n=note-36;x=n%4+(n>=32?4:0);y=(n%32)/4;}
    for(i=0;i<8;i++) {
        lp_touch_hit_t *h=&lp_touch.hit[i];int32_t dx,dy,delta;
        if(!h->active)continue;
        age=now-h->time;if(!lp_touch_alive(h,now)){h->active=0;continue;}
        dx=(int32_t)x-h->x;dy=(int32_t)y-h->y;distance=(uint32_t)(dx*dx+dy*dy);
        if(shape>=5) {
            if(grid) {if(distance)continue;level=127;
                if(shape==5 && !h->held)level=127*(120-(now-h->released))/120;
            } else {
                uint32_t head=(h->ring_start+age/30)%32,trail=(head+32-(uint32_t)ring)%32;
                if(trail>=5)continue;level=(5-trail)*24;
            }
            total+=level;continue;
        }
        radius=age*6/700;delta=(int32_t)distance-(int32_t)(radius*radius);
        if(delta<0)delta=-delta;
        lit=(uint32_t)delta<=radius+1;
        if(shape==1) {
            radius=1+age*3/700;delta=(int32_t)distance-(int32_t)(radius*radius);
            if(delta<0)delta=-delta;lit=(uint32_t)delta<=radius+1;
        } else if(shape==2) {
            static const int8_t vx[]={1,1,0,-1,-1,-1,0,1},vy[]={0,1,1,1,0,-1,-1,-1};
            uint32_t angle=(age/55+i)&7;radius=1+age*3/700;
            lit=dx==vx[angle]*(int32_t)radius && dy==vy[angle]*(int32_t)radius;
        } else if(shape==3) {
            uint32_t hash=(x+1)*73856093u^(y+1)*19349663u^(age/65+i+1)*83492791u;
            lit=distance<=25 && hash%7==0;
        } else if(shape==4) {
            /* Tight, stationary eight-neighbour ring, four short pulses. */
            uint32_t ax=(uint32_t)(dx<0?-dx:dx),ay=(uint32_t)(dy<0?-dy:dy);
            lit=ax<=1 && ay<=1 && (ax || ay) && (age/85)%2==0;
        }
        if(distance==0 && age<90){level=127;}
        else if(lit) {
            level=(700-age)*(64+h->velocity/2)/700;
        } else continue;
        total+=level;
    }
    if(!total)return key;
    if(total>127)total=127;
    /* Exact selected hue even above coloured A/B and overlapping hits. */
    return 0x1000000u|(((rgb>>16)*total/255)<<16)|(((((rgb>>8)&255)*total/255))<<8)|((rgb&255)*total/255);
}
#endif
