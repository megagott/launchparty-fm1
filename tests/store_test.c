#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../firmware/src/launchparty_player.h"
#define LP_EXTERNAL_SCENES
#include "../firmware/src/launchparty_bank.h"
static uint8_t flash[0x100000];static int fail_prog;
static uint32_t st_crc32(const void *ptr,uint32_t n){const uint8_t*p=ptr;uint32_t c=~0u;while(n--){c^=*p++;for(int i=0;i<8;i++)c=(c>>1)^((c&1)?0xedb88320:0);}return ~c;}
static int st_read(uint32_t o,void*p,uint32_t n){assert(o>=0xA0000&&o+n<=0xDC000);memcpy(p,flash+o,n);return 0;}
static int st_prog(uint32_t o,const void*p,uint32_t n){assert(o>=0xA0000&&o+n<=0xDC000);if(fail_prog)return -1;for(uint32_t i=0;i<n;i++)flash[o+i]&=((const uint8_t*)p)[i];return 0;}
static int st_erase(uint32_t o){assert(o>=0xA0000&&o+4096<=0xDC000);memset(flash+o,255,4096);return 0;}
#include "../firmware/src/launchparty_store.h"
static const uint8_t*lp_store_pointer(uint32_t o){assert(o>=0xA0000&&o<0xDC000);return flash+o;}
static void lp_store_cache(uint32_t o,uint32_t n){(void)o;(void)n;}
static void lp_store_idle(void){}
static void wr(uint8_t*p,uint32_t n){p[0]=n;p[1]=n>>8;p[2]=n>>16;p[3]=n>>24;}
static void upload(uint8_t*b,uint32_t n){assert(!lp_store_begin(n,st_crc32(b,n)));for(uint32_t i=0;i<n;i+=256)assert(!lp_store_write(i,b+i,n-i>256?256:n-i));assert(!lp_store_commit());}
int main(void){uint8_t b[1468]={76,80,66,75,1,0,41,0};memset(flash,255,sizeof flash);wr(b+8,sizeof b);wr(b+16,1328);wr(b+20,140);memcpy(b+24,"My show",8);memcpy(b+1328,"LPS2",4);wr(b+1332,960);lp_store_scan();assert(lp_store_current==-1);assert(lp_scene_at(0)==&lp_bank[0]);upload(b,sizeof b);assert(lp_store_current==0&&lp_store_head.seq==1);assert(lp_scene_at(0)!=&lp_bank[0]);
 assert(!lp_store_begin(sizeof b,st_crc32(b,sizeof b)));assert(!lp_store_write(0,b,256));lp_store_scan();assert(lp_store_current==0&&lp_store_head.seq==1);lp_store_upload.open=0;upload(b,sizeof b);assert(lp_store_current==1&&lp_store_head.seq==2);
 b[1328]='X';assert(!lp_store_begin(sizeof b,st_crc32(b,sizeof b)));for(uint32_t i=0;i<sizeof b;i+=256)assert(!lp_store_write(i,b+i,sizeof b-i>256?256:sizeof b-i));assert(lp_store_commit()==3);lp_store_scan();assert(lp_store_current==1&&lp_store_head.seq==2);b[1328]='L';
 assert(!lp_store_begin(sizeof b,0));assert(lp_store_write(1,b,256)==2);assert(lp_store_write(0,b,257)==2);lp_store_upload.open=0;
 wr(flash+0xC8000,0x504D5346);assert(lp_store_begin(sizeof b,0)==7);assert(lp_store_head.seq==2);
 puts("PASS: both banks, reboot scan, interrupted write, malformed clip, bounds, sample protection");return 0;}
