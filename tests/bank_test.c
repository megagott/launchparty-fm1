#include <stdio.h>
#include <assert.h>
#include "../firmware/src/launchparty_player.h"
#include "../firmware/src/launchparty_bank.h"
int main(void) {
    unsigned i,m,n;lp_player_t p;
    for(i=0;i<LP_BANK_COUNT;i++) {
        uint32_t hash=2166136261u;
        lp_init(&p,0);assert(lp_start(&p,&lp_bank[i],LP_ONCE));
        for(m=0;m<=10000 && (m==0 || p.playing);m++) {
            lp_update(&p,m);
            for(n=0;n<128;n++){hash^=p.frame[n];hash*=16777619u;}
        }
        assert(!p.playing && !p.faults);printf("%u %u %u\n",i,m,hash);
    }
    return 0;
}
