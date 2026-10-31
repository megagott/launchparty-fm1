static void lp_flush_show(uint32_t now) {
    uint8_t b[88]={240,0,32,41,2,14,3},pads[16];
    uint32_t keys[16],len=7,count=0,visited=0,k,c,rgb,key;
    while(visited++<128 && count<16) {
        k=lp.cursor;lp.cursor=(k+1)&127;
        if(lp_programmer_note(k)==255)continue;
        c=lp_fx_color(lp.frame[k],lp_fx.shift);
        rgb=lp_fx_rgb(&lp_fx,k,lp.frame[k],now,lp.playing || lp_mode!=1);
        /* Lit pads at full brightness use the exact original hardware palette.
         * Only dimmed pads/tails need RGB approximation. */
        key=lp_fx.brightness==100 && (lp.frame[k] || !rgb)?c:0x1000000u|rgb;
        key=lp_mix_key(&lp_mix,k,key,now);
        key=lp_touch_key(k,lp_touch_background(key),now);
        if(lp_fx_sent[k]==key)continue;
        pads[count]=(uint8_t)k;keys[count++]=key;
        b[len++]=(key&0x1000000u)?3:0;b[len++]=lp_programmer_note(k);
        if(key&0x1000000u){b[len++]=(uint8_t)((key>>16)&127);b[len++]=(uint8_t)((key>>8)&127);b[len++]=(uint8_t)(key&127);}
        else b[len++]=(uint8_t)key;
    }
    if(!count)return;
    b[len++]=247;
    if(lp_send_sysex(b,len))for(k=0;k<count;k++)lp_fx_sent[pads[k]]=keys[k];
}

