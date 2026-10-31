#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "../firmware/src/launchparty_layout.h"
int main(void){
 const unsigned legacy_grid[8][8]={{36,37,38,39,68,69,70,71},{40,41,42,43,72,73,74,75},{44,45,46,47,76,77,78,79},{48,49,50,51,80,81,82,83},{52,53,54,55,84,85,86,87},{56,57,58,59,88,89,90,91},{60,61,62,63,92,93,94,95},{64,65,66,67,96,97,98,99}};
 const unsigned old_edge[]={26,27,28,29,30,31,32,33,34,35,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,12,13,14,15,16,17,18,19};
 const unsigned new_edge[]={90,99,91,92,93,94,95,96,97,98,89,79,69,59,49,39,29,19,80,70,60,50,40,30,20,10,101,102,103,104,105,106,107,108,1,2,3,4,5,6,7,8};
 unsigned x,y,i,n,count=0,seen[128]={0};
 for(y=0;y<8;y++)for(x=0;x<8;x++){assert(lp_source_note(x,y)==legacy_grid[y][x]);assert(lp_programmer_note(legacy_grid[y][x])==11+x+10*y);}
 for(i=0;i<42;i++)assert(lp_programmer_note(old_edge[i])==new_edge[i]);
 for(n=0;n<128;n++){i=lp_programmer_note(n);if(i!=255){assert(i<128 && !seen[i]++);count++;}}
 assert(count==106);puts("PASS: all 106 physical positions: 64 grid + 42 border/logo/shift controls, unique and matching official diagrams");}
