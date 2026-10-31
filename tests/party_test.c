#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_mix.h"
#include "../firmware/src/launchparty_party.h"
int main(void) {
    lp_player_t a;lp_mix_t m;lp_fx_t f;lp_party_t p;
    unsigned scene=0,t,old,changes=0,overlays=0,last=0,seed;
    lp_init(&a,0);lp_mix_init(&m,0);lp_fx_init(&f);
    lp_party_begin(&p,&a,&scene,0);
    lp_party_step(&p,&a,&m,&f,&scene,0,120,31999);assert(scene==0);
    lp_party_step(&p,&a,&m,&f,&scene,0,120,32000);
    assert(p.next_show>p.tick && scene<12);
    /* Run many sets and seeds: only adjacent/repeated A, bounded compatible B. */
    for(seed=1;seed<=10;seed++) {
        scene=0;lp_party_begin(&p,&a,&scene,0);p.rng=seed;
        for(t=0;t<1000000;t+=50) {
            old=scene;last=p.next_show;
            lp_party_step(&p,&a,&m,&f,&scene,0,120,t);
            assert(scene<12);
            if(p.next_show!=last) {
                assert(p.tick%3840<100);assert(scene==old || scene==(old+1)%12);changes++;
            }
            if(m.key>=0){assert(m.key<27);overlays++;}
            assert(f.brightness<=100 && f.tail_ms<=3000 && f.speed<8 && f.shift>=-13 && f.shift<=13);
        }
    }
    assert(changes>100 && overlays>100);
    lp_party_begin(&p,&a,&scene,1000000);
    p.manual_knobs=15;f.shift=12;f.brightness=42;f.tail_ms=2300;f.speed=7;
    lp_party_step(&p,&a,&m,&f,&scene,0,240,1020000);
    assert(f.shift==12 && f.brightness==42 && f.tail_ms==2300 && f.speed==7);
    lp_party_step(&p,&a,&m,&f,&scene,1u<<26,240,1020100);assert(m.key==26 && p.overlay==-1);
    lp_party_step(&p,&a,&m,&f,&scene,0,240,1020200);assert(m.key==-1 && m.fading);
    lp_party_end(&p,&m,0,1020300);assert(!p.on);
    old=scene;lp_party_step(&p,&a,&m,&f,&scene,0,240,1030000);assert(scene==old);
    for(seed=0;seed<12;seed++)for(t=0;t<100;t++) {
        unsigned b=lp_party_b(&p,seed);assert(b<27);
        if(seed<2)assert(b<7);
        else if(seed<4)assert(b>=7 && b<=13);
        else if(seed==4)assert(b>=2 && b<=5);
        else if(seed<9)assert(b>=22);
        else if(seed<11)assert(b>=16 && b<=21);
        else assert(b>=13 && b<=15);
    }
    puts("PASS: 10 long random sets; bar-boundary adjacent/repeat shows; compatible overlays; knob and keyboard takeover; auto exit.");
}
