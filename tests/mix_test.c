#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
#include "../firmware/src/launchparty_fx.h"
#include "../firmware/src/launchparty_mix.h"
int main(void) {
    lp_mix_t m;lp_player_t a;lp_fx_t fx;unsigned k,n,lit=0;
    lp_init(&a,0);assert(lp_start(&a,&lp_bank[0],LP_LOOP));
    lp_mix_init(&m,0);lp_fx_init(&fx);
    for(k=0;k<LP_B_COUNT;k++) {
        lp_mix_keys(&m,0,k*1000);lp_mix_keys(&m,1u<<k,k*1000);
        assert(m.key==(int)k && m.player.scene==&lp_bank[lp_b_scene[k]]);
        lp_update(&a,k*1000);lp_update(&m.player,k*1000+125);
        for(n=0;n<128;n++) {
            unsigned b=m.player.frame[n];
            assert(lp_mix_key(&m,n,53,k*1000+125)==(b?lp_b_tint(b,n,k*1000+125):53));
            if(b)lit++;
        }
    }
    assert(lit);
    lp_mix_keys(&m,1,21000);lp_mix_keys(&m,3,21001);assert(m.key==1);
    lp_mix_keys(&m,1,21002);assert(m.key==0);
    m.player.frame[36]=21;m.player.frame[37]=0;
    lp_mix_keys(&m,0,21003);assert(m.key==-1 && m.fading);
    assert(lp_mix_key(&m,37,53,21004)==53);
    assert(lp_mix_key(&m,36,53,21003)==(0x1000000u|lp_key_rgb(lp_b_tint(21,36,21003))));
    assert(lp_mix_key(&m,36,53,21303)==53);
    assert(lp_mix_key(&m,36,53,21153)!=(0x1000000u|lp_key_rgb(lp_b_tint(21,36,21003))));
    lp_mix_keys(&m,1,22000);m.player.frame[36]=21;
    fx.shift=13;fx.brightness=0;fx.tail_ms=3000;fx.speed=7;
    assert(lp_mix_key(&m,36,0x1000000u|lp_fx_rgb(&fx,36,53,22000,1),22000)==lp_b_tint(21,36,22000));
    m.player.bpm=120;a.bpm=480;
    lp_mix_keys(&m,0,23000);lp_mix_keys(&m,1,23000);
    lp_update(&m.player,23500);assert(m.player.elapsed==960);
    lp_mix_clear(&m);assert(m.key==-1 && !m.fading && !m.player.playing);
    puts("PASS: 27 non-text key mappings, transparent black, B priority, release fade, held-key fallback, FX isolation, independent B timing and panic.");
}
