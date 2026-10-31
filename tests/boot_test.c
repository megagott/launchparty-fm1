#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint16_t screen[240*240];static unsigned clears;
static void lcd_sync(void){}
static void lcd_fill(unsigned x,unsigned y,unsigned w,unsigned h,uint16_t c){unsigned a,b;assert(x+w<=240 && y+h<=240);if(w==240&&h==240)clears++;for(b=y;b<y+h;b++)for(a=x;a<x+w;a++)screen[b*240+a]=c;}
static void lcd_blit(unsigned x,unsigned y,unsigned w,unsigned h,const uint16_t*p){unsigned a,b;assert(x+w<=240&&y+h<=240);for(b=0;b<h;b++)for(a=0;a<w;a++){unsigned c=p[b*w+a];screen[(y+b)*240+x+a]=(c>>8)|(c<<8);}}
#include "../firmware/src/gfx.c"
#include "../firmware/src/splash.c"
int main(void){unsigned i,k;FILE*f;sloop_splash();for(i=0;i<40;i++)lp_boot_frame(i);assert(clears==1);lp_boot_frame(12);f=fopen("build/boot-preview.ppm","wb");assert(f);fprintf(f,"P6\n240 240\n255\n");for(k=0;k<240*240;k++){unsigned c=screen[k];fputc(((c>>11)&31)*255/31,f);fputc(((c>>5)&63)*255/63,f);fputc((c&31)*255/31,f);}fclose(f);puts("PASS: 40 intro frames within display bounds, one initial clear only");}
