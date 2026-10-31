/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef LAUNCHPARTY_WEB_H
#define LAUNCHPARTY_WEB_H
/* F0 7D 4C 50 cmd seq packed-body F7; response cmd|40, seq, status,
 * packed-body. Group packing: high-bit mask followed by up to seven bytes.
 * This protocol never exposes an arbitrary flash address. */
static void lp_web_reply(uint32_t cmd,uint32_t seq,uint32_t status,uint32_t n) {
    uint32_t i=0,p=7,j,mask,at;
    ed_out[0]=240;ed_out[1]=0x7d;ed_out[2]=0x4c;ed_out[3]=0x50;
    ed_out[4]=(uint8_t)(cmd|64);ed_out[5]=(uint8_t)seq;ed_out[6]=(uint8_t)status;
    while(i<n){at=p++;mask=0;for(j=0;j<7 && i<n;j++,i++){mask|=(ed_smp_buf[i]>>7)<<j;ed_out[p++]=ed_smp_buf[i]&127;}ed_out[at]=(uint8_t)mask;}
    ed_out[p++]=247;ota_wire_send(ed_out,p);
}
static void lp_web_stop(void) {
    lp_party.on=0;lp_web_keys=0;lp_stop(&lp);lp_mix_clear(&lp_mix);lp_touch_clear();
    panic_req=(1u<<NPART)-1;lp_panel_held=0;
}
static int lp_web_control(uint32_t n) {
    uint32_t op=ed_smp_buf[0],a=ed_smp_buf[1],v;
    if(!n || n>4)return 1;
    if(op==1 && n==1) {lp_mode=1;if(lp.playing)lp_stop(&lp);else lp_start(&lp,lp_scene_at(lp_scene_index),LP_LOOP);}
    else if(op==2 && n==1)lp_web_stop();
    else if(op==3 && n==2 && a<LP_BANK_COUNT) {lp_scene_index=a;lp_mode=1;if(lp.playing)lp_request(&lp,lp_scene_at(a),LP_LOOP,lp_quantized);}
    else if(op==4 && n==2 && a<2) {lp_mode=1;if(a){lp_playmode=LP_LOOP;lp_party_begin(&lp_party,&lp,&lp_scene_index,fm1_ms);}else lp_party_end(&lp_party,&lp_mix,0,fm1_ms);}
    else if(op==5 && n==2 && a<LP_TOUCH_VARIANTS)lp_touch.mode=(uint8_t)a;
    else if(op==6 && n==2 && a<6){lp_b_color=(uint8_t)a;lp_b_color_time=fm1_ms;}
    else if(op==7 && n==4 && a<4) {
        v=ed_smp_buf[2]|(uint32_t)ed_smp_buf[3]<<8;lp_party.manual_knobs|=1u<<a;
        if(a==0 && v<=26)lp_fx.shift=(int16_t)((int32_t)v-13);
        else if(a==1 && v<=100)lp_fx.brightness=(uint16_t)v;
        else if(a==2 && v<8)lp_fx.speed=(uint8_t)v;
        else if(a==3 && v<=3000)lp_fx.tail_ms=(uint16_t)v;
        else return 1;
    } else if(op==8 && n==3 && a<LP_B_COUNT && ed_smp_buf[2]<2) {
        if(ed_smp_buf[2])lp_web_keys|=1u<<a;else lp_web_keys&=~(1u<<a);
    } else if(op==9 && n==3) {v=a|(uint32_t)ed_smp_buf[2]<<8;if(v<40 || v>240)return 1;song.g[G_BPM]=(int16_t)v;}
    else if(op==10 && n==2 && a<3)lp_change_mode(a);
    else if(op==11 && n==1)lp_web_keys=0;
    else return 1;
    return 0;
}
static void lp_web_service(void) {
    const uint8_t *frame;uint32_t size,cmd,seq,n,status=0,out=0,off,len,i;
    if(!ota_frame_get(&frame,&size) || size<5 || frame[0]!=0x7d || frame[1]!=0x4c || frame[2]!=0x50)return;
    cmd=frame[3];seq=frame[4];n=ed_unpack7(frame+5,size-5,ed_smp_buf,sizeof(ed_smp_buf));ota_frame_done();
    if(cmd==1 && !n) {
        for(i=0;i<32;i++)ed_smp_buf[i]=0;
        ed_smp_buf[0]=1;ed_smp_buf[1]=(uint8_t)LP_BANK_COUNT;ed_smp_buf[2]=(uint8_t)LP_B_COUNT;ed_smp_buf[3]=3;
        ota_wr32(ed_smp_buf+4,LP_STORE_CAP);
        if(lp_store_current>=0){ota_wr32(ed_smp_buf+8,lp_store_head.len);ota_wr32(ed_smp_buf+12,lp_store_head.crc);ota_wr32(ed_smp_buf+16,lp_store_head.seq);}
        ed_smp_buf[20]=(uint8_t)lp_store_conflict();ed_smp_buf[21]='0';ed_smp_buf[22]='.';ed_smp_buf[23]='3';ed_smp_buf[24]='.';ed_smp_buf[25]='1';out=32;
    } else if(cmd==2 && n==8) {off=lp_u32(ed_smp_buf);len=lp_u32(ed_smp_buf+4);lp_web_stop();status=(uint32_t)lp_store_begin(off,len);}
    else if(cmd==3 && n>4 && n<=260)status=(uint32_t)lp_store_write(lp_u32(ed_smp_buf),ed_smp_buf+4,n-4);
    else if(cmd==4 && !n){lp_web_stop();status=(uint32_t)lp_store_commit();ui.force=1;}
    else if(cmd==5 && !n){lp_store_upload.open=0;}
    else if(cmd==6 && n==6 && lp_store_current>=0) {
        off=lp_u32(ed_smp_buf);len=ed_smp_buf[4]|(uint32_t)ed_smp_buf[5]<<8;
        if(!len || len>256 || off>lp_store_head.len || len>lp_store_head.len-off)status=1;
        else if(st_read(lp_store_base((uint32_t)lp_store_current)+256+off,ed_smp_buf,len))status=4;
        else out=len;
    } else if(cmd==7 && !lp_store_upload.open)status=(uint32_t)lp_web_control(n);
    else status=1;
    lp_web_reply(cmd,seq,status,out);
}
#endif
