/* SPDX-License-Identifier: GPL-3.0-only
 * Launchparty experimental standalone show player + directly played synth.
 * Audio owns synth voices. Main loop owns show state and the LED transport.
 * This first build deliberately does not start musical patterns/arrangements.
 */
#include "launchparty_player.h"
#ifndef LAUNCHPARTY_CORE_TEST
#define LP_EXTERNAL_SCENES 1
#endif
#include "launchparty_bank.h"
#include "launchparty_layout.h"
#ifndef LAUNCHPARTY_CORE_TEST
static const uint8_t *lp_store_pointer(uint32_t off){return fm1_xip_ptr(off);}
static void lp_store_cache(uint32_t off,uint32_t n){uint32_t irq=irq_save();fl_inval(off,n);irq_restore(irq);}
static void lp_store_idle(void){fm1_wdt_feed();}
#include "launchparty_store.h"
#endif
#include "launchparty_fx.h"
#include "launchparty_mix.h"
#include "launchparty_party.h"
#include "launchparty_sound.h"
#include "launchparty_touch.h"
static lp_player_t lp;
static lp_mix_t lp_mix;
static lp_party_t lp_party;
static uint32_t lp_web_keys;
static lp_fx_t lp_fx;
static uint32_t lp_fx_sent[128];
static uint8_t lp_effect_path=255;
static uint32_t lp_scene_index, lp_last_send, lp_last_draw, lp_prev_keys;
static uint8_t lp_mode=1, lp_playmode=LP_LOOP, lp_quantized, lp_programmer=1;
static uint8_t lp_din=0, lp_transport_up, lp_configured;
static uint32_t lp_visual_q[64];
static volatile uint32_t lp_vw,lp_vr,lp_visual_drops;
static uint8_t lp_held[128],lp_owner[128];
static uint8_t lp_ripple_pad[8],lp_ripple_color[8];
static uint32_t lp_ripple_time[8],lp_ripple_count;
static uint8_t lp_pressure[128];
static const lp_scene_t *lp_panel_held;
static int32_t lp_aftertouch(const void *track,uint32_t note) {
    if(note>=128 || !lp_owner[note] || track!=&trk[lp_owner[note]-1])return 0;
    return (int32_t)lp_pressure[note]*96; /* up to ~48 cutoff indices, without editing the preset */
}

static void lp_visual_push(uint32_t pkt) {
    if(lp_vw-lp_vr<64) {lp_visual_q[lp_vw&63]=pkt;RING_PUBLISH();lp_vw++;}
    else lp_visual_drops++;
}
/* Runs between audio blocks, not in a UI routine. Sound never waits for LEDs. */
static void lp_audio_block(uint32_t n) {
    uint32_t i,pr=panic_req;
    (void)n;panic_req=0;transport_req=0;song.playing=0;song.rec=0;rec_wait=0;
    for(i=0;i<NPART;i++) {
        engine_block(&trk[i]);
        if((pr>>i)&1) {trk_all_off(&trk[i]);trk[i].nheld=0;}
    }
    if(pr) {for(i=0;i<128;i++){lp_held[i]=lp_owner[i]=0;lp_pressure[i]=0;}}
    keyboard_block();
    for(i=0;i<32 && mi_r!=mi_w;i++) {
        uint32_t pkt=midi_in_q[mi_r%MQ],st=(pkt>>8)&0xF0,ch=(pkt>>8)&15;
        uint32_t note=(pkt>>16)&127,vel=(pkt>>24)&127,trs=(pkt>>4)&15;
        track_t *t;
        mi_r++;
        if(st==0xB0 && (note==120 || note==123)) {panic_req=(1u<<NPART)-1;continue;}
        if(st==0xA0 && !trs) {lp_pressure[note]=(uint8_t)vel;continue;}
        if(st!=0x90 && st!=0x80) {if(st==0xB0 && !trs)lp_visual_push(pkt);continue;}
        if(!trs)lp_visual_push(pkt);
        if(trs) {
            t=midi_route(ch,note,st==0x90 && vel);
            if(!is_drum(t)) {if(st==0x90 && vel)input_on(t,note,vel);else input_off(t,note);}
            continue;
        }
        if(st==0x80 || !vel) {
            if(lp_owner[note])input_off(&trk[lp_owner[note]-1],lp_held[note]);
            lp_owner[note]=0;lp_pressure[note]=0;
        } else if(trs || lp_mode==0 || lp_mode==2) {
            uint32_t selected=trs && ch<NPART?ch:song.sel%NPART;
            t=&trk[selected];
            if(lp_owner[note])input_off(&trk[lp_owner[note]-1],lp_held[note]);
            lp_held[note]=(uint8_t)note;lp_owner[note]=(uint8_t)(selected+1);
            input_on(t,note,vel);
        }
    }
}

#ifndef LAUNCHPARTY_CORE_TEST
static int lp_send_note(void *ctx,uint8_t note,uint8_t color) {
    uint32_t mapped=note;(void)ctx;
    if(!usb.config || usb.suspended || mo_w-mo_r>=MQ-4)return 0;
    mapped=lp_programmer_note(note);
    if(mapped==255)return 1;
    midi_out_q[mo_w%MQ]=9u|0x90u<<8|mapped<<16|(uint32_t)color<<24;
    RING_PUBLISH();mo_w++;return 1;
}
/* Nonblocking complete SysEx; never waits for USB in the show loop. */
static int lp_send_sysex(const uint8_t *b,uint32_t len) {
#if FELUCCA_OTA
    uint32_t n=(len+2)/3,i;
    if(!usb.config || sx_busy || so_w-so_r+n>SXQ)return 0;
    for(i=0;i<len;i+=3) {
        uint32_t k=len-i>=3?3:len-i,cin=(i+k<len)?4:k+4;
        sx_out_q[so_w%SXQ]=cin|(uint32_t)b[i]<<8|(k>1?(uint32_t)b[i+1]<<16:0)|(k>2?(uint32_t)b[i+2]<<24:0);
        RING_PUBLISH();so_w++;
    }
    return 1;
#else
    (void)b;(void)len;return 0;
#endif
}
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
static void lp_change_mode(uint32_t mode) {
    lp_party_end(&lp_party,&lp_mix,0,fm1_ms);
    lp_mode=(uint8_t)(mode%3);panic_req=(1u<<NPART)-1;
    lp_mix_clear(&lp_mix);
    lp_touch_clear();
    lp_ripple_count=0;
    {uint32_t i;for(i=0;i<128;i++){lp_fx.held[i]=0;lp_fx_sent[i]=0xffffffffu;lp.sent[i]=255;}}
}
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
static void lp_overlay(void) {
    uint32_t i,x,y;
    if(lp_mode==1)return;
    for(i=0;i<8 && i<lp_ripple_count;i++) {
        uint32_t age=fm1_ms-lp_ripple_time[i],decay=500+(uint32_t)TSEL->p[P_REV]*8,radius;
        if(age>=decay)continue;
        radius=age*8/decay;
        for(y=0;y<8;y++)for(x=0;x<8;x++) {
            int32_t dx=(int32_t)x-lp_ripple_pad[i]%8,dy=(int32_t)y-lp_ripple_pad[i]/8;
            uint32_t dist=(uint32_t)(dx<0?-dx:dx)+(uint32_t)(dy<0?-dy:dy);
            if(dist==radius)lp.frame[lp_source_note(x,y)]=lp_ripple_color[i];
        }
    }
    for(y=0;y<8;y++)for(x=0;x<8;x++) {
        i=11+x+10*y;
        if(lp_owner[i] && lp_pressure[i])lp.frame[lp_source_note(x,y)]=lp_pressure[i]>100?3:lp_pressure[i]>60?9:45;
    }
}
static void lp_control(void) {
    uint32_t pressed=fm1_input_edges(0),keys=fm1_in.notes|lp_web_keys,k;
    int32_t s;
    fm1_input_note_edges();
    if(pressed&(1u<<panel.btn[B_HOME]))lp_change_mode(lp_mode+1);
    if(lp_mode==1 && (pressed&(1u<<panel.btn[B_SAVE]))) {
        lp_panel_held=0;
        if(lp_party.on)lp_party_end(&lp_party,&lp_mix,keys,fm1_ms);
        else {lp_playmode=LP_LOOP;lp_party_begin(&lp_party,&lp,&lp_scene_index,fm1_ms);}
    }
    if(pressed&(1u<<panel.btn[B_PLAY])) {
        lp_party_end(&lp_party,&lp_mix,keys,fm1_ms);
        if(lp.playing || lp.pending_on)lp_stop(&lp);
        else {lp_request(&lp,lp_scene_at(lp_scene_index),lp_playmode,lp_quantized);if(lp_playmode==LP_HOLD)lp_panel_held=lp_scene_at(lp_scene_index);}
    }
    if(lp_panel_held && !(fm1_in.buttons&(1u<<panel.btn[B_PLAY]))) {lp_release(&lp,lp_panel_held);lp_panel_held=0;}
    if(pressed&(1u<<panel.btn[B_REC]))lp_playmode=(uint8_t)((lp_playmode+1)%3);
    if(pressed&(1u<<panel.btn[B_SEQ]))lp_quantized^=1;
    if(pressed&(1u<<panel.btn[B_ARP]))lp_din^=1;
    if(pressed&(1u<<panel.btn[B_GLO])) {
        /* Reassert Programmer mode, never silently change the show layout. */
        lp_programmer=1;lp_configured=0;
        for(k=0;k<128;k++){lp.sent[k]=255;lp_fx_sent[k]=0xffffffffu;}
    }
    if(pressed&(1u<<panel.btn[B_FX])) {lp_party.on=0;lp_touch_clear();lp_stop(&lp);lp_mix_clear(&lp_mix);lp_mix.keys=keys&((1u<<LP_B_COUNT)-1);panic_req=(1u<<NPART)-1;}
    if(lp_mode==1) {
        if(pressed&(1u<<panel.btn[B_OCTDN])) {if(lp_b_color)lp_b_color--;lp_b_color_time=fm1_ms;}
        if(pressed&(1u<<panel.btn[B_OCTUP])) {if(lp_b_color<5)lp_b_color++;lp_b_color_time=fm1_ms;}
    } else {
        if(pressed&(1u<<panel.btn[B_OCTDN]) && song.octave>-3)song.octave--;
        if(pressed&(1u<<panel.btn[B_OCTUP]) && song.octave<3)song.octave++;
    }
    if((s=panel_enc(EN_SELECT))) {
        if(fm1_in.buttons&(1u<<panel.btn[B_ENV]))song.g[G_BPM]=(int16_t)clamp(song.g[G_BPM]+s,40,240);
        else {
            lp_party_end(&lp_party,&lp_mix,keys,fm1_ms);
            uint32_t old=lp_scene_index;
            lp_scene_index=(uint32_t)clamp((int32_t)lp_scene_index+s,0,LP_BANK_COUNT-1);
            if(lp_mode==1 && old!=lp_scene_index && (lp.playing || lp.pending_on)) {
                lp_request(&lp,lp_scene_at(lp_scene_index),lp_playmode,lp_quantized);
                if(lp_panel_held)lp_panel_held=lp_scene_at(lp_scene_index);
            }
        }
    }
    if((s=panel_enc(EN_ALGO))) {
        if(lp_mode==1)lp_touch.mode=(uint8_t)clamp((int32_t)lp_touch.mode+s,0,LP_TOUCH_VARIANTS-1);
        else song.sel=(uint8_t)clamp((int32_t)song.sel+s,0,NPART-1);
    }
    if((s=panel_enc(EN_PRESET))) {uint32_t total,cur=preset_pos(&total);if(total)preset_go((cur+total+(s%(int32_t)total))%total);TSEL->p[P_AMODE]=TSEL->p[P_AHOLD]=0;lp_sound_reset(song.sel%NPART);}
    if(lp_mode==1 && !(fm1_in.buttons&(1u<<panel.btn[B_ENV]))) {
        if((s=panel_enc(EN_K1))) {lp_party.manual_knobs|=1;lp_fx.shift=(int16_t)clamp(lp_fx.shift+s,-13,13);}
        if((s=panel_enc(EN_K2))) {lp_party.manual_knobs|=2;lp_fx.brightness=(uint16_t)clamp(lp_fx.brightness+s*5,0,100);}
        if((s=panel_enc(EN_K3))) {lp_party.manual_knobs|=4;lp_fx.speed=(uint8_t)clamp(lp_fx.speed+s,0,7);}
        if((s=panel_enc(EN_K4))) {lp_party.manual_knobs|=8;lp_fx.tail_ms=(uint16_t)clamp(lp_fx.tail_ms+s*100,0,3000);}
    } else {
        if((s=panel_enc(EN_K1)))lp_sound_turn(0,s);
        if((s=panel_enc(EN_K2)))lp_sound_turn(1,s);
        if((s=panel_enc(EN_K3)))lp_sound_turn(2,s);
        if((s=panel_enc(EN_K4)))lp_sound_turn(3,s);
    }
    if(lp_mode!=1)for(k=0;k<27;k++)if((keys&~lp_prev_keys)&(1u<<k)) {
        uint32_t slot=lp_ripple_count++%8;lp_ripple_pad[slot]=(uint8_t)(k%64);lp_ripple_color[slot]=53;lp_ripple_time[slot]=fm1_ms;
    }
    if(lp_mode==1 && !lp_party.on && !(pressed&(1u<<panel.btn[B_FX])))lp_mix_keys(&lp_mix,keys,fm1_ms);
    lp_prev_keys=keys;
}
static void lp_main_init(void) {
    uint32_t i;
    lp_init(&lp,fm1_ms);lp_prev_keys=0;
    lp_store_scan();
    lp_mix_init(&lp_mix,fm1_ms);
    lp_fx_init(&lp_fx);for(i=0;i<128;i++)lp_fx_sent[i]=0xffffffffu;
    song.sel=0;preset_go(0); /* ALIEN BLOOM: direct-play atmospheric default. */
    for(i=0;i<NPART;i++) {lp_sound_reset(i);trk[i].p[P_AMODE]=0;trk[i].p[P_AHOLD]=0;}
    song.g[G_DUCK]=0;song.g[G_DRCH]=0;song.g[G_SYNC]=0;
    lp_transport_up=0;lp_configured=0;
}
static void lp_service(void) {
    uint32_t i,now=fm1_ms,up=usb.config && !usb.suspended;
    if(lp_store_upload.open)return;
    lp_control();
    if(lp_visual_drops) {lp_party.on=0;lp_touch_clear();lp_stop(&lp);lp_mix_clear(&lp_mix);lp_visual_drops=0;}
    for(i=0;i<64 && lp_vr!=lp_vw;i++) {uint32_t pkt=lp_visual_q[lp_vr&63];lp_vr++;lp_pad_event(pkt);}
    lp_party_step(&lp_party,&lp,&lp_mix,&lp_fx,&lp_scene_index,fm1_in.notes|lp_web_keys,(uint32_t)song.g[G_BPM],now);
    lp.bpm=(uint16_t)((uint32_t)song.g[G_BPM]*(lp_mode==1?lp_speed_q8[lp_fx.speed]:256)/256);lp_update(&lp,now);lp_overlay();
    lp_mix.player.bpm=(uint16_t)song.g[G_BPM];lp_update(&lp_mix.player,now);
    lp_mix_ambience(&lp_mix,now);
    lp_touch_service(now);
    if(up!=lp_transport_up) {lp_transport_up=(uint8_t)up;lp_configured=0;for(i=0;i<128;i++){lp.sent[i]=255;lp_fx_sent[i]=0xffffffffu;}}
    if(up && !lp_configured) {
        /* Always use the Programmer target layout. */
        static const uint8_t prog[]={240,0,32,41,2,14,14,1,247};
        lp_configured=(uint8_t)lp_send_sysex(prog,9);
    }
    if(up && lp_configured && now-lp_last_send>=(lp_din?33u:16u)) {
        uint32_t effects=lp_mode==1;
        if(effects!=lp_effect_path) {
            lp_effect_path=(uint8_t)effects;
            for(i=0;i<128;i++){lp.sent[i]=255;lp_fx_sent[i]=0xffffffffu;lp_fx.held[i]=0;}
        }
        if(lp_mode==1 && !effects)
            lp_flush(&lp,lp_send_note,0,lp_din?24:48);
        else if(lp_mode==1)lp_flush_show(now);
        else lp_flush(&lp,lp_send_note,0,lp_din?24:48);
        lp_last_send=now;
    }
}
/* Display the state accepted by the same MIDI transport as the Launchpad.
 * Reading these caches does not advance envelopes or alter show/tail state. */
static uint16_t lp_preview_color(uint32_t note) {
    uint32_t key=lp_effect_path==1?lp_fx_sent[note]:lp.sent[note],rgb;
    if(key==0xffffffffu || key==255 || key==0 || key==0x1000000u)return RGB(7,16,28);
    if(key&0x1000000u)
        return RGB(((key>>16)&127)*255/127,((key>>8)&127)*255/127,(key&127)*255/127);
    if(key>=128)return RGB(7,16,28);
    rgb=lp_palette_rgb[key];return RGB(rgb>>16,(rgb>>8)&255,rgb&255);
}
static void lp_digits(char *p,uint32_t value,uint32_t digits) {
    while(digits){p[--digits]=(char)('0'+value%10);value/=10;}
}
static void lp_draw(void) {
    static uint8_t valid,last_mode,last_status,last_programmer,last_din,last_up,last_configured;
    static uint32_t last_scene;
    static uint32_t last_preset,last_engine,last_track;
    static uint16_t last_cells[64];
    static int16_t last_shift;
    static uint16_t last_brightness,last_tail;
    static uint8_t last_speed;
    static uint8_t last_auto;
    static uint8_t last_phase;
    static uint8_t last_touch_mode;
    static uint8_t last_bcolor,last_color_hint;
    static uint8_t last_sound_controls;
    static uint32_t last_sound_values;
    static int32_t last_bkey;
    char b[32];uint32_t i,status=lp.playing?1:lp.pending_on?2:0;
    uint32_t sound_controls=lp_mode!=1 || (fm1_in.buttons&(1u<<panel.btn[B_ENV]));
    uint8_t *sv=lp_sound_value[song.sel%NPART];
    uint32_t sound_values=sv[0]|(uint32_t)sv[1]<<8|(uint32_t)sv[2]<<16|(uint32_t)sv[3]<<24;
    uint32_t color_hint=lp_b_color_time && fm1_ms-lp_b_color_time<1800;
    if(fm1_ms-lp_last_draw<100)return;lp_last_draw=fm1_ms;
    /* Never erase a live screen before repainting it. Each changed text box
     * is composed off-screen; unchanged labels and cells generate no SPI work. */
    if(ui.force) {valid=0;ui.force=0;}
    if(!valid) {
        lcd_fill(0,0,120,2,RGB(120,255,30));lcd_fill(120,0,120,2,RGB(255,30,180));
        lcd_fill(20,70,100,1,RGB(30,220,255));lcd_fill(120,70,100,1,RGB(180,80,255));
        lp_logo_draw(1,6,2);
        draw_text_box(124,4,116,&FONT_S,"0.3.1 WEB",RGB(190,135,255),1);
    }
    if(!valid || last_mode!=lp_mode || last_bkey!=lp_mix.key || last_touch_mode!=lp_touch.mode || last_bcolor!=lp_b_color || last_color_hint!=color_hint) {
        static const char *touch_shapes[]={"SOLO ","BLINKRING ","HALO ","EXPLOSION ","WIRBEL ","FUNKEN ","RAHMEN ",};
        static const char *touch_colors[]={"NEON GRUEN","NEON TUERKIS","NEON CYAN","NEON VIOLETT","NEON PINK"};
        if(lp_mode==1 && lp_mix.key>=0) {
            str_cpy(b,"B00 ",sizeof b);lp_digits(b+1,(uint32_t)lp_mix.key+1,2);str_cpy(b+4,lp_b_color_names[lp_b_color],sizeof b-4);
            draw_text_box(0,25,240,&FONT_S,b,RGB(255,30,180),1);
        } else {
            if(lp_mode==1 && color_hint) {str_cpy(b,"B: ",sizeof b);str_cpy(b+3,lp_b_color_names[lp_b_color],sizeof b-3);}
            else if(lp_mode==1) {uint32_t len;str_cpy(b,touch_shapes[lp_touch.mode/5],sizeof b);len=str_len(b);str_cpy(b+len,touch_colors[lp_touch_color_index()],sizeof b-len);}
            else str_cpy(b,lp_mode==0?"SYNTH":"VISUAL JAM",sizeof b);
            draw_text_box(0,25,240,&FONT_S,b,RGB(255,30,180),1);
        }
    }
    if(!valid || last_scene!=lp_scene_index || last_mode!=lp_mode || last_preset!=TSEL->preset || last_engine!=TSEL->eng_req || last_track!=song.sel) {
        if(lp_mode==1)str_cpy(b,lp_scene_at(lp_scene_index)->name,sizeof b);
        else if(user_of(TSEL)<UP_SLOTS)up_name(user_of(TSEL),b);
        else {const engine_t *e=ENGINES[TSEL->eng_req%NENGINES];str_cpy(b,e->presets[TSEL->preset%e->npresets].name,sizeof b);}
        draw_text_box(4,47,232,&FONT_S,b,RGB(30,220,255),1);
    }
    for(i=0;i<64;i++) {
        uint16_t c=lp_preview_color(lp_source_note(i%8,i/8));
        if(!valid || last_cells[i]!=c)
            lcd_fill(52+(i%8)*17,79+(7-i/8)*13,14,10,c);
        last_cells[i]=c;
    }
    if(!valid || last_phase!=lp_party.phase || last_auto!=lp_party.on || last_status!=status || last_up!=lp_transport_up || last_configured!=lp_configured) {
        static const char *phases[]={"AUTO: ERWACHEN","AUTO: WACHSTUM","AUTO: EKSTASE","AUTO: NACHTRAUM"};
        draw_text_box(0,186,240,&FONT_S,!lp_transport_up?"USB NOT CONNECTED":!lp_configured?"MIDI CONFIG PENDING":lp_party.on?phases[lp_party.phase]:status==1?"PLAYING / USB OK":status==2?"NEXT BEAT / USB OK":"STOPPED / USB OK",!lp_transport_up?RGB(255,70,100):status==1?RGB(120,255,30):RGB(30,220,255),1);
    }
    if(!sound_controls) {
        if(!valid || last_sound_controls || last_mode!=lp_mode || last_shift!=lp_fx.shift || last_brightness!=lp_fx.brightness) {
            str_cpy(b,"K1:+00  K2:100%",sizeof b);
            b[3]=lp_fx.shift<0?'-':'+';lp_digits(b+4,(uint32_t)(lp_fx.shift<0?-lp_fx.shift:lp_fx.shift),2);
            lp_digits(b+11,lp_fx.brightness,3);draw_text_box(0,205,240,&FONT_S,b,RGB(255,70,190),1);
        }
        if(!valid || last_sound_controls || last_mode!=lp_mode || last_speed!=lp_fx.speed || last_tail!=lp_fx.tail_ms) {
            str_cpy(b,"K3:100% K4:0000ms",sizeof b);
            lp_digits(b+3,(uint32_t)lp_speed_q8[lp_fx.speed]*100/256,3);
            lp_digits(b+11,lp_fx.tail_ms,4);draw_text_box(0,222,240,&FONT_S,b,RGB(180,110,255),1);
        }
    } else {
        if(!valid || !last_sound_controls || last_sound_values!=sound_values || last_track!=song.sel) {
            str_cpy(b,"MUT:000 WARP:000",sizeof b);lp_digits(b+4,sv[0]*100/127,3);lp_digits(b+13,sv[1]*100/127,3);
            draw_text_box(0,205,240,&FONT_S,b,RGB(255,70,190),1);
            str_cpy(b,"MOVE:000 SPACE:000",sizeof b);lp_digits(b+5,sv[2]*100/127,3);lp_digits(b+15,sv[3]*100/127,3);
            draw_text_box(0,222,240,&FONT_S,b,RGB(180,110,255),1);
        }
    }
    last_shift=lp_fx.shift;last_brightness=lp_fx.brightness;last_speed=lp_fx.speed;last_tail=lp_fx.tail_ms;
    last_phase=lp_party.phase;last_auto=lp_party.on;last_bkey=lp_mix.key;last_mode=lp_mode;last_scene=lp_scene_index;last_status=(uint8_t)status;
    last_preset=TSEL->preset;last_engine=TSEL->eng_req;last_track=song.sel;
    last_sound_controls=(uint8_t)(sound_controls!=0);last_sound_values=sound_values;
    last_touch_mode=lp_touch.mode;
    last_bcolor=lp_b_color;last_color_hint=(uint8_t)color_hint;
    last_programmer=lp_programmer;last_din=lp_din;last_up=lp_transport_up;last_configured=lp_configured;valid=1;
}
#include "launchparty_web.h"
#endif
