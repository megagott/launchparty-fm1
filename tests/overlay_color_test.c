#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_mix.h"
int main(void) {
    lp_mix_t m;unsigned c,n,t,k,count=0,first=0,vary=0;
    static const unsigned expected[]={0x01107f00,0x01007f48,0x01005a7f,0x014b107f,0x017f0040};
    for(k=0;k<27;k++) {
        assert(lp_b_scene[k]!=14 && lp_b_scene[k]!=15);
        assert(!strstr(lp_bank[lp_b_scene[k]].name,"Schrift") && !strstr(lp_bank[lp_b_scene[k]].name,"Lars"));
        lp_mix_init(&m,0);lp_mix_keys(&m,1u<<k,0);
        assert(m.player.scene==&lp_bank[lp_b_scene[k]]);
    }
    for(c=0;c<5;c++) {
        lp_b_color=(uint8_t)c;
        assert(lp_b_tint(3,36,0)==expected[c]);
        for(n=36;n<100;n++)assert(lp_b_tint(3,n,1000)==expected[c]);
    }
    lp_mix_init(&m,0);lp_mix_keys(&m,1,0);m.player.frame[36]=3;m.player.frame[37]=0;
    for(c=0;c<5;c++) {
        lp_b_color=(uint8_t)c;
        assert(lp_mix_key(&m,36,53,0)==expected[c]);
        assert(lp_mix_key(&m,37,53,0)==53);
    }
    lp_b_color=5;
    for(t=0;t<1000;t+=160)for(n=36;n<100;n++) {
        unsigned key=lp_b_tint(3,n,t);
        if(!first)first=key;if(key!=first)vary=1;
        for(c=0;c<5;c++)if(key==expected[c])count++;
    }
    assert(vary && count==7*64);
    puts("PASS: all27 non-text keyboard/Auto Party clips; five uniform neon colours; immediate held-clip recolour; transparent black; final animated Neon Bunt uses only neon hues.");
}
