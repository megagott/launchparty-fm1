static void lp_pad_event(uint32_t pkt) {
    uint32_t st=(pkt>>8)&0xF0,note=(pkt>>16)&127,v=(pkt>>24)&127,pad;
    if(st==0xB0 && v) {
        if(note==91)lp_change_mode(0);
        if(note==92)lp_change_mode(1);
        if(note==93)lp_change_mode(2);
        return;
    }
    if(st!=0x90 && st!=0x80)return;
    if(lp_programmer) {
        uint32_t x=note%10,y=note/10;
        if(x<1 || x>8 || y<1 || y>8)return;
        pad=(y-1)*8+x-1;
    } else {if(note<36 || note>99)return;pad=note-36;}
    if(lp_mode==1) {
        if(st==0x90 && v)lp_touch_press(pad,v,fm1_ms);
        else lp_touch_release(pad,fm1_ms);
    } else if(st==0x90 && v) {
        uint32_t slot=lp_ripple_count++%8;
        lp_ripple_pad[slot]=(uint8_t)pad;lp_ripple_color[slot]=(uint8_t)(v>90?53:v>50?49:37);
        lp_ripple_time[slot]=fm1_ms;
    }
}

