/* SPDX-License-Identifier: GPL-3.0-only */
/* Original Mandalars epic arrangements. Generated at section boundaries, no flash writes. */
static const char *const MY_SONG_NAMES[4]={"FOREST ODYSSEY","PSY ASCENSION","TEKK REACTOR","HITECH PORTAL"};
static const uint8_t MY_SONG_ORDER[4][16]={
    {0,1,2,3,1,2,3,0,1,2,3,1,2,2,1,0},
    {0,0,1,3,1,2,3,1,2,2,3,1,2,2,1,0},
    {0,1,2,1,2,2,3,1,2,2,1,2,3,2,2,0},
    {0,1,2,3,2,2,3,1,2,2,3,2,1,2,2,0}
};
static const uint8_t MY_SONG_BARS[16]={8,16,16,8,16,32,8,8,16,32,8,8,16,32,8,8};
static void my_song_note(uint32_t part,uint32_t step,uint32_t note,uint32_t vel) {
    step_t *s=&trk[part].step[step];s->time=ST_NOTE;s->n=1;s->note[0]=(uint8_t)note;s->vel=(uint8_t)vel;
}
static void mycelium_song_scene(uint32_t scene) {
    uint32_t i,k,kind=arrangement.reserved[1]-1u,chapter=arrangement_clock.index;
    uint32_t root=kind==2?34u:36u, upper=chapter>=8u?12u:0u;
    uint8_t panic_before=panic_req;
    static const uint8_t MOTIF[4][16]={
        {0,3,7,10,3,0,6,7,12,10,7,3,6,0,3,7},
        {0,7,3,10,12,7,5,3,0,3,7,12,10,7,3,0},
        {0,0,7,0,3,0,10,7,0,12,7,0,3,7,0,10},
        {0,1,7,10,13,7,3,1,12,10,7,3,1,7,13,10}
    };
    for(k=0;k<NTRK;k++) {trk_all_off(&trk[k]);track_defaults(&trk[k]);}
    for(k=0;k<NPART;k++) {
        trk[k].eng_req=(uint8_t)(k?1:0);
        apply_preset_to(&trk[k],k==0?(kind==3?17:16):k==1?(scene==0||scene==3?20:kind==3?25:kind==1?27:kind==2?15:21):(scene==0?17:kind==3?28:23));
    }
    panic_req=panic_before; /* already released above; don't defer a panic into the next beat */
    for(k=0;k<NTRK;k++) {steps_clear(&trk[k]);trk[k].p[P_SLEN]=64;trk[k].p[P_SDIV]=2;trk[k].p[P_SSWING]=0;trk[k].p[P_AMODE]=0;trk[k].seq_active=1;}
    TDRUM->p[P_E0]=kind==0?MY_DRUM_FOREST:kind==3?MY_DRUM_HITECH:MY_DRUM_PSY;
    song.g[G_BPM]=kind==0?156:kind==1?148:kind==2?170:196;
    song.g[G_DRLVL]=100;song.g[G_DRREV]=12;song.solo=0;
    song.g[G_FILT]=0;song.g[G_ROUTE]=0;song.g[G_DUST]=0;song.g[G_NAME]=0;song.g[G_DUCK]=28;song.g[G_ENGGO]=32;
    trk[0].p[P_LEVEL]=kind==2?110:100;trk[0].p[P_SGATE]=kind==3?38:80;
    trk[1].p[P_LEVEL]=scene==2?88:72;trk[2].p[P_LEVEL]=scene==2?76:64;
    trk[1].p[P_SGATE]=scene==0||scene==3?116:kind==3?32:52;trk[2].p[P_SGATE]=scene==0?120:32;
    trk[1].p[P_APROB]=scene==2?100:85;trk[2].p[P_APROB]=scene==2?82:65;
    /* Distinct roles: organic forest dialogue, melodic psy, distorted TEKK,
       chopped Hitech. Only drum/bass saturation is changed here. */
    if(kind==2) {trk[0].p[P_DIST]=48;TDRUM->p[P_DIST]=58;trk[1].p[P_SGATE]=26;}
    if(kind==0) {trk[1].p[P_APROB]=78;trk[2].p[P_APROB]=72;}
    if(kind==1) {trk[1].p[P_APROB]=100;trk[1].p[P_SGATE]=72;}
    if(kind==3) {trk[1].p[P_APROB]=96;trk[2].p[P_APROB]=90;trk[2].p[P_SGATE]=18;}
    for(i=0;i<64;i++) {
        uint32_t bass=kind==2?i%4==2:kind==3?i%4>=2:kind==0?i%4==2||i%4==3:i%4!=0;
        if(scene==1 && chapter<4) bass=i%4==2;
        if((scene==1||scene==2)&&bass) my_song_note(0,i,root+(scene==2 && i>=56 && i%8==7?7:0),i%4==1?100:90);
        if(scene==0||scene==3) {
            if(i%16==0) my_song_note(1,i,root+24+MOTIF[kind][(i/16+chapter)%16],78);
            else trk[1].step[i].time=ST_TIE;
            if(i==30||i==62) my_song_note(2,i,root+36+MOTIF[kind][(i/8+chapter)%16],68);
        } else {
            uint32_t hit=kind==0?(i+chapter)%8==3||i%16==14:
                kind==1?(scene==1?i%4==2:i%2==1):
                kind==2?i%8==7||i%16==3:
                i%4==1||i%8==7||(scene==2&&i%16==10);
            if(hit) {
                my_song_note(1,i,root+24+MOTIF[kind][(i/2+chapter*3)%16]+(kind==1?upper:0),i%8==7?94:78);
                if(kind==3 && i%16==15) trk[1].step[i].rat=1;
                if(kind==1 && i%16==14) trk[1].step[i].flags=SF_ACCENT|SF_SLIDE;
            }
            uint32_t reply=kind==0?(i+chapter*2)%16==9:
                kind==1?i%16==0||i%16==10:
                kind==2?i%16==11:
                i%8==3||i%16==14;
            if(reply) my_song_note(2,i,root+31+MOTIF[kind][(i/4+chapter)%16],kind==3?82:72);
        }
        if(scene==1||scene==2) {
            if(i%4==0 || (kind==2 && scene==2 && i==63)) dstep_set(&TDRUM->dstep[i],0,LV_NORM,0);
            if(kind==3?i%2==1:i%4==2) dstep_set(&TDRUM->dstep[i],4,LV_SOFT,0);
            if(kind==2 && i%8==4) dstep_set(&TDRUM->dstep[i],3,LV_NORM,0);
            else if(scene==2 && kind==1 && i%16==12) dstep_set(&TDRUM->dstep[i],3,LV_SOFT,0);
            else if(scene==2 && kind==3 && i%16==12) dstep_set(&TDRUM->dstep[i],3,LV_SOFT,1);
            if(kind==0 && (i+chapter)%16==11) dstep_set(&TDRUM->dstep[i],2,LV_SOFT,0);
            if(kind==2 && scene==2 && i%16==15) dstep_set(&TDRUM->dstep[i],0,LV_SOFT,0);
            if(scene==2 && i>=60 && i%2==0) dstep_set(&TDRUM->dstep[i],4,LV_SOFT,kind==3?2:1);
        } else if(scene==0 && chapter>0 && i%8==6) dstep_set(&TDRUM->dstep[i],4,LV_SOFT,0);
        else if(scene==3 && i>=56 && i%2==0) dstep_set(&TDRUM->dstep[i],4,LV_SOFT,1);
    }
    my_bass_style=kind==2?1:kind==0||kind==3?0:2;my_fill=0;song.rec=0;mycelium_start_ready=0;
    sync_reload=1;ui.force=1;
}
static void mycelium_song_load(uint32_t kind) {
    uint32_t i;
    if(!my_song_active()) {my_user_order=arrangement;my_user_order_kept=1;}
    my_song_apply=mycelium_song_scene;
    arrangement.count=16;arrangement.loop=0;arrangement.reserved[0]=0x4d;arrangement.reserved[1]=(uint8_t)(kind+1u);
    for(i=0;i<16;i++) {arrangement.entry[i].scene=MY_SONG_ORDER[kind][i];arrangement.entry[i].bars=MY_SONG_BARS[i];}
    arrangement_clock.index=0;arrangement_clock.running=0;
    fm1_irq_off();mycelium_song_scene(0);fm1_irq_on();
    arrangement_enabled=1;live_req=-1;live_sec=-1;mycelium_start_ready=1;
}
