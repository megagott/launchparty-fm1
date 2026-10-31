/* Show effects operate after decoding: stored clips are never rewritten.
 * Native palette output is retained at unity; RGB values for dimmed tails
 * use the conventional Launchpad palette approximation (not calibration). */
static const uint32_t lp_palette_rgb[128]={
0x000000,0x1e1e1e,0x7f7f7f,0xffffff,0xff4c4c,0xff0000,0x590000,0x190000,
0xffbd6c,0xff5400,0x591d00,0x271b00,0xffff4c,0xffff00,0x595900,0x191900,
0x88ff4c,0x54ff00,0x1d5900,0x142b00,0x4cff4c,0x00ff00,0x005900,0x001900,
0x4cff5e,0x00ff19,0x00590d,0x001902,0x4cff88,0x00ff55,0x00591d,0x001f12,
0x4cffb7,0x00ff99,0x005935,0x001912,0x4cc3ff,0x00a9ff,0x004152,0x001019,
0x4c88ff,0x0055ff,0x001d59,0x000819,0x4c4cff,0x0000ff,0x000059,0x000019,
0x874cff,0x5400ff,0x190064,0x0f0030,0xff4cff,0xff00ff,0x590059,0x190019,
0xff4c87,0xff0054,0x59001d,0x220013,0xff1500,0x993500,0x795100,0x436400,
0x033900,0x005735,0x00547f,0x0000ff,0x00454f,0x2500cc,0x7f7f7f,0x202020,
0xff0000,0xbdff2d,0xafff00,0x64ff09,0x108b00,0x00ff87,0x00a9ff,0x002aff,
0x3f00ff,0x7a00ff,0xb21a7d,0x402100,0xff4a00,0x88e106,0x72ff15,0x00ff00,
0x3bff26,0x59ff71,0x38ffcc,0x5b8aff,0x3151c6,0x877fe9,0xd31dff,0xff005d,
0xff7f00,0xb9b000,0x90ff00,0x835d07,0x392b00,0x144c10,0x0d5038,0x15152a,
0x16205a,0x693c1c,0xa8000a,0xde513d,0xd86a1c,0xffe126,0x9ee12f,0x67b50f,
0x1e1e30,0xdcff6b,0x80ffbd,0x9a99ff,0x8e66ff,0x404040,0x757575,0xe0ffff,
0xa00000,0x350000,0x1ad000,0x074200,0xb9b000,0x3f3100,0xb35f00,0x4b1502};
typedef struct {
    int16_t shift;
    uint16_t brightness,tail_ms;
    uint8_t speed;
    uint8_t held[128];
    uint32_t last_on[128];
} lp_fx_t;
static const uint16_t lp_speed_q8[]={64,128,192,256,384,512,768,1024};
static void lp_fx_init(lp_fx_t *f) {
    uint32_t i;uint8_t *p=(uint8_t*)f;
    for(i=0;i<sizeof(*f);i++)p[i]=0;
    f->brightness=100;f->speed=3;
}
static uint8_t lp_fx_color(uint8_t c,int32_t shift) {
    if(!c || !shift)return c;
    /* Rotate the 14 native hue groups, keeping each group's shade. Special
     * palette entries stay intact rather than guessing another colour. */
    if(c>=4 && c<60) {
        int32_t group=(c-4)/4;
        group=(group+shift+28)%14;
        return (uint8_t)(4+group*4+(c-4)%4);
    }
    return c;
}
static uint32_t lp_fx_rgb(lp_fx_t *f,uint32_t n,uint8_t color,uint32_t now,int active) {
    uint32_t level=f->brightness,c,age;
    if(!active){f->held[n]=0;return 0;}
    if(color){f->held[n]=color;f->last_on[n]=now;}
    else {
        age=now-f->last_on[n];
        if(!f->tail_ms || !f->held[n] || age>=f->tail_ms){f->held[n]=0;return 0;}
        color=f->held[n];level=level*(f->tail_ms-age)/f->tail_ms;
    }
    c=lp_palette_rgb[lp_fx_color(color,f->shift)];
    return (((c>>16)*level/100/2)<<16)|((((c>>8)&255)*level/100/2)<<8)|((c&255)*level/100/2);
}
