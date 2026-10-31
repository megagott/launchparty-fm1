/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LAUNCHPARTY_STORE_H
#define LAUNCHPARTY_STORE_H
/* Two transactional banks in the documented user-sample region. Never write
 * if any of the three slots still has an FSMP sample header. Projects,
 * settings, presets, firmware and the updater area are outside this range. */
#define LP_STORE_BASE 0xA0000u
#define LP_STORE_SIZE 0x1E000u
#define LP_STORE_CAP (LP_STORE_SIZE-256u)
#define LP_STORE_MAGIC 0x4255504Cu
typedef struct {uint32_t magic,seq,len,crc,hcrc;} lp_store_head_t;
static const uint8_t *lp_store_pointer(uint32_t off);
static void lp_store_cache(uint32_t off,uint32_t n);
static void lp_store_idle(void);
static lp_scene_t lp_user_scene[LP_BANK_COUNT];
static lp_store_head_t lp_store_head;
static int32_t lp_store_current=-1;
static struct {uint32_t base,len,crc,received,seq;uint8_t open;} lp_store_upload;
static uint32_t lp_store_base(uint32_t copy){return LP_STORE_BASE+copy*LP_STORE_SIZE;}
static int lp_store_body_ok(const uint8_t *body,uint32_t len) {
    uint32_t i,off,n,k,dir=16+LP_BANK_COUNT*32;
    if(len<dir || body[0]!='L'||body[1]!='P'||body[2]!='B'||body[3]!='K' || body[4]!=1 || body[5] ||
       (body[6]|(uint32_t)body[7]<<8)!=LP_BANK_COUNT || lp_u32(body+8)!=len)return 0;
    for(i=0;i<LP_BANK_COUNT;i++) {
        const uint8_t *e=body+16+i*32;lp_scene_t scene;
        off=lp_u32(e);n=lp_u32(e+4);if(!n){if(off)return 0;continue;}
        if(off<dir || off>len || n>len-off || n>8192 || n<140 || body[off]!='L'||body[off+1]!='P'||body[off+2]!='S'||body[off+3]!='2')return 0;
        if(e[31])return 0;
        for(k=8;k<31;k++)if(e[k] && (e[k]<32 || e[k]>126))return 0;
        scene.data=body+off;scene.size=n;scene.name=(const char*)e+8;
        if(!lp_validate(&scene))return 0;
    }
    return 1;
}
static int lp_store_copy_ok(uint32_t copy,lp_store_head_t *h) {
    uint32_t base=lp_store_base(copy);const uint8_t *body=lp_store_pointer(base+256);
    if(st_read(base,h,sizeof(*h)) || h->magic!=LP_STORE_MAGIC || h->len>LP_STORE_CAP ||
       h->hcrc!=st_crc32(h,16) || st_crc32(body,h->len)!=h->crc)return 0;
    return lp_store_body_ok(body,h->len);
}
static void lp_store_scan(void) {
    lp_store_head_t a,b;uint32_t i,va,vb;const uint8_t *body;
    lp_store_cache(LP_STORE_BASE,2*LP_STORE_SIZE);
    va=lp_store_copy_ok(0,&a);vb=lp_store_copy_ok(1,&b);
    lp_store_current=vb && (!va || b.seq>a.seq)?1:va?0:-1;
    for(i=0;i<LP_BANK_COUNT;i++)lp_user_scene[i].data=0;
    if(lp_store_current<0)return;
    lp_store_head=lp_store_current?b:a;body=lp_store_pointer(lp_store_base((uint32_t)lp_store_current)+256);
    for(i=0;i<LP_BANK_COUNT;i++) {
        const uint8_t *e=body+16+i*32;uint32_t n=lp_u32(e+4);
        if(n){lp_user_scene[i].data=body+lp_u32(e);lp_user_scene[i].size=n;lp_user_scene[i].name=(const char*)e+8;}
    }
}
static const lp_scene_t *lp_scene_at(uint32_t i) {
    return lp_user_scene[i].data?&lp_user_scene[i]:&lp_bank[i];
}
static int lp_store_conflict(void) {
    uint32_t i,magic;
    for(i=0;i<3;i++)if(st_read(0xA0000u+i*0x14000u,&magic,4) || magic==0x504D5346u)return 1;
    return 0;
}
static int lp_store_begin(uint32_t len,uint32_t crc) {
    uint32_t i,base;
    lp_store_upload.open=0;
    if(len<16+32*LP_BANK_COUNT || len>LP_STORE_CAP)return 1;
    if(lp_store_conflict())return 7;
    base=lp_store_base(lp_store_current==0?1:0);
    for(i=0;i<LP_STORE_SIZE;i+=4096){if(st_erase(base+i))return 4;lp_store_idle();}
    lp_store_cache(base,LP_STORE_SIZE);
    lp_store_upload.base=base;lp_store_upload.len=len;lp_store_upload.crc=crc;
    lp_store_upload.received=0;lp_store_upload.seq=lp_store_current<0?1:lp_store_head.seq+1;lp_store_upload.open=1;
    return 0;
}
static int lp_store_write(uint32_t offset,const uint8_t *data,uint32_t n) {
    if(!lp_store_upload.open || offset!=lp_store_upload.received || !n || n>256 || n>lp_store_upload.len-offset)return 2;
    if(st_prog(lp_store_upload.base+256+offset,data,n)){lp_store_upload.open=0;return 4;}
    lp_store_upload.received+=n;return 0;
}
static int lp_store_commit(void) {
    lp_store_head_t h;const uint8_t *body;
    if(!lp_store_upload.open || lp_store_upload.received!=lp_store_upload.len)return 2;
    lp_store_upload.open=0;lp_store_cache(lp_store_upload.base,LP_STORE_SIZE);
    body=lp_store_pointer(lp_store_upload.base+256);
    if(st_crc32(body,lp_store_upload.len)!=lp_store_upload.crc || !lp_store_body_ok(body,lp_store_upload.len))return 3;
    h.magic=LP_STORE_MAGIC;h.seq=lp_store_upload.seq;h.len=lp_store_upload.len;h.crc=lp_store_upload.crc;h.hcrc=st_crc32(&h,16);
    if(st_prog(lp_store_upload.base,&h,sizeof(h)))return 4;
    lp_store_scan();return lp_store_current<0 || lp_store_head.seq!=h.seq?5:0;
}
#endif
