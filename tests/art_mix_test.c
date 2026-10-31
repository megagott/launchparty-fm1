#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_mix.h"
#include "../firmware/src/launchparty_party.h"
int main(void) {
    lp_mix_t m;lp_player_t a;lp_fx_t f;lp_party_t p;unsigned s=0,t;
    lp_mix_init(&m,0);lp_init(&a,0);lp_fx_init(&f);
    lp_start(&a,&lp_bank[0],LP_LOOP);a.frame[36]=21;
    lp_mix_transition(&m,&a,&f,0);
    assert(lp_mix_key(&m,36,53,0)==(0x1000000u|lp_key_rgb(21)));
    assert(lp_mix_key(&m,36,53,300)!=lp_mix_key(&m,36,53,0));
    lp_mix_ambience(&m,600);assert(lp_mix_key(&m,36,53,600)==53);
    lp_mix_keys(&m,1,610);m.player.frame[36]=21;
    for(t=610;t<=1610;t+=10)lp_mix_ambience(&m,t);
    assert(m.duck==45);assert(lp_mix_key(&m,36,53,1610)==lp_b_tint(21,36,1610));
    m.player.frame[37]=0;assert(lp_mix_key(&m,37,53,1610)!=(unsigned)53);
    lp_mix_keys(&m,0,1610);
    for(t=1610;t<=2910;t+=10)lp_mix_ambience(&m,t);
    assert(m.duck==100);assert(lp_mix_key(&m,36,53,2910)==53);
    lp_party_begin(&p,&a,&s,0);
    for(t=0;t<=128000;t+=10) {
        lp_party_step(&p,&a,&m,&f,&s,0,120,t);
        if(t==0)assert(p.phase==0 && p.speed==2);
        if(t==32000)assert(p.phase==1 && p.speed==3);
        if(t==80000)assert(p.phase==2 && p.speed==4 && p.brightness>=95);
        if(t==112000)assert(p.phase==3 && p.speed==2 && p.tail>=1000);
        if(t==128000)assert(p.phase==0);
    }
    lp_mix_clear(&m);assert(m.duck==100 && !m.transition);
    puts("PASS: 600ms A crossfade; smooth 45% background duck and recovery; untouched B colour; all four dramatic phases and cycle wrap.");
}
