/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), HÃ¼gelton Instruments */
/* SLOOP menu (HOME held): COLOR, LOWCUT, ZOOM, LIGHTS, KEYS, NOTES, USB AUDIO, HARDWARE CALIBRATION, ABOUT. */
/* ------------------------------------------------------------ menu --- */
enum { MI_COLOR, MI_LOWCUT, MI_ZOOM, MI_LIGHTS, MI_KEYS, MI_NOTES, MI_USB, MI_PANEL, MI_ABOUT, MI_JAM, MI_LOGO, MI_SONGS, MI_BACK, MI_COUNT };
static const char *const MI_NAME[MI_COUNT] = {"COLOR", "LOWCUT", "ZOOM", "LIGHTS", "KEYS", "NOTES", "USB AUDIO",
                                              "HARDWARE CALIBRATION", "ABOUT", "GROOVES", "LOGO", "SONGS", "BACK"};
static const char *const MY_GROOVE_NAME[] = {"FOREST JAM", "PSY ROLL", "PSY GALLOP", "GOA ACID", "HITECH", "TEKK", "BACK"};
static uint8_t my_groove_sel, my_song_sel;
static void my_song_picker(void) {ui.menu=6;my_song_sel=0;ui.force=1;}
static const char *const LIGHTS_NAME[LIGHTS_N] = {"OFF", "LOW", "MID", "HIGH"};   /* every button lit, the labels readable */
static const char *const KEYS_NAME[KEYS_N] = {"OFF", "C KEYS", "WHITE KEYS"};      /* keys lit too, at the LIGHTS level */
#define MI_DY 14                                   /* rows between two menu lines */

static void draw_menu(void)
{
    uint32_t i, pass, sig = ui.menu * 7u + my_song_sel * 47u + my_groove_sel * 31u + ui.menu_sel * 131u + settings.palette * 1009u + settings.lowcut * 7919u +
                            settings.zoom * 104729u + lights_lvl * 1299709u + lights_keys * 15485863u +
                            lights_notes * 32452843u + usb_full * 49979687u;
    if (!ui.force && sig == ui.menu_sig)
        return;
    ui.menu_sig = sig;
    if(ui.menu==5) {sloop_splash();return;}
    if (ui.force)                                   /* head + rule + two bands cover rows 0..229 */
        lcd_fill(0, H_HEAD + 1 + 124 + 95, 240, 240 - (H_HEAD + 1 + 124 + 95), C_BLACK);
    cv_begin(240, H_HEAD, C_BLACK);
    cv_text(4, 1, &FONT_S, ui.menu == 2 ? "ABOUT" : ui.menu >= 6 ? "SONGS" : ui.menu >= 3 ? "GROOVES" : "MENU", C_HI);
    cv_blit(0, Y_HEAD);
    lcd_fill(0, H_HEAD, 240, 1, C_LINE);
    for (pass = 0; pass < 2u; pass++) {             /* the canvas holds 124 rows: draw in two bands */
        cv_begin(240, pass ? 95u : 124u, C_BLACK);
        cv_oy = pass ? -124 : 0;
        if(ui.menu==6) {
            for(i=0;i<5;i++) {const char *name=i<4?MY_SONG_NAMES[i]:"USER SONG";
                cv_text(14,12+i*25,&FONT_S,name,i==my_song_sel?C_WHITE:C_GRAY);
                if(i==my_song_sel)cv_rect(4,18+i*25,3,3,C_HI);
            }
            cv_text(4,150,&FONT_S,"240 BARS / EPIC JOURNEY",C_HI);
            cv_text(4,174,&FONT_S,"OCT+ LOAD  OCT- BACK",C_DIM);
        } else if(ui.menu==7) {
            cv_text(4,8,&FONT_S,MY_SONG_NAMES[my_song_sel],C_HI);
            cv_text(4,44,&FONT_S,"REPLACE CURRENT LOOP?",C_WHITE);
            cv_text(4,68,&FONT_S,"USER SLOTS KEPT",C_HI);
            cv_text(4,94,&FONT_S,"SAVE YOUR WORK FIRST",C_AMB);
            cv_text(4,134,&FONT_S,"OCT+ LOAD SONG",C_HI);
            cv_text(4,158,&FONT_S,"OCT- CANCEL",C_DIM);
        } else if (ui.menu == 3) {
            for (i = 0; i < 7u; i++) {
                cv_text(14, 8 + i * 20, &FONT_S, MY_GROOVE_NAME[i], i == my_groove_sel ? C_WHITE : C_GRAY);
                if (i == my_groove_sel) cv_rect(4, 14 + i * 20, 3, 3, C_HI);
            }
            cv_text(4, 150, &FONT_S, "PRESETS SELECT", C_AMB);
            cv_text(4, 174, &FONT_S, "OCT+ LOAD  OCT- BACK", C_DIM);
        } else if (ui.menu == 4) {
            cv_text(4, 8, &FONT_S, MY_GROOVE_NAME[my_groove_sel], C_HI);
            cv_text(4, 44, &FONT_S, "REPLACE CURRENT LOOP?", C_WHITE);
            cv_text(4, 68, &FONT_S, "SAVE YOUR WORK FIRST", C_AMB);
            cv_text(4, 110, &FONT_S, "OCT+ REPLACE", C_HI);
            cv_text(4, 134, &FONT_S, "OCT- CANCEL", C_DIM);
        } else if (ui.menu == 2) {
            cv_text(4, 4, &FONT_L, "MYCELIUM", C_HI);
            cv_text(4, 36, &FONT_S, "SLOOP / FELUCCA", C_AMB);
            cv_text(4, 54, &FONT_S, FELUCCA_VERSION, C_HI);
            cv_text(cv_text(4, 72, &FONT_S, "LEO KUROSHITA", C_HI) + 8, 72, &FONT_S, "@KUROGEDELIC", C_AMB);
            cv_text(4, 88, &FONT_S, "H\xDCGELTON INSTRUMENTS", C_HI);   /* Latin-1 U-umlaut */
            cv_text(4, 104, &FONT_S, "HUGELTON.COM", C_AMB);
            cv_text(4, 119, &FONT_S, "GPL-3.0, NO WARRANTY", C_HI);
            cv_text(4, 132, &FONT_S, "GITHUB.COM/ISOD89/SLOOP-FM1", C_AMB);   /* (the source of this firmware) */
            cv_text(4, 146, &FONT_S, "FONT: TERMINUS (OFL)", C_DIM);
            cv_text(4, 159, &FONT_S, "SAMPLES: VERSILIAN (CC0)", C_DIM);
            cv_text(4, 172, &FONT_S, "+ SONIC PI (CC0)", C_DIM);
            cv_text(4, 185, &FONT_S, "PHASE: CRISPYZEBRA (GPL)", C_DIM);
            cv_text(4, 198, &FONT_S, "VOICE: REF. KLATTSCH (MIT)", C_DIM);
        } else {
            for (i = 0; i < MI_COUNT; i++) {
                int32_t y = 4 + (int32_t)i * MI_DY;
                int sel = i == ui.menu_sel;
                if (sel)
                    cv_rect(4, y + 6, 3, 3, C_WHITE);
                cv_text(14, y, &FONT_S, MI_NAME[i], sel ? C_WHITE : C_GRAY);
                if (i == MI_LOWCUT || i == MI_ZOOM || i == MI_NOTES)
                    cv_text(100, y, &FONT_S, (i == MI_LOWCUT ? settings.lowcut : i == MI_ZOOM ? settings.zoom : lights_notes)
                                                ? "ON" : "OFF", C_HI);
                if (i == MI_LIGHTS)
                    cv_text(100, y, &FONT_S, LIGHTS_NAME[lights_lvl % LIGHTS_N], C_HI);
                if (i == MI_USB)                       /* the USB audio input: follows MASTER, or full level */
                    cv_text(100, y, &FONT_S, usb_full ? "FULL" : "MASTER", C_HI);
                if (i == MI_KEYS)
                    cv_text(100, y, &FONT_S, KEYS_NAME[lights_keys % KEYS_N], lights_lvl ? C_HI : C_DIM);   /* (needs LIGHTS) */
                if (i == MI_COLOR) {
                    uint32_t k;
                    cv_text(100, y, &FONT_S, PALETTES[settings.palette].name, C_HI);
                    for (k = 0; k < 5u; k++)
                        cv_rect(160 + (int32_t)k * 14, y + 3, 10, 10, pal[k]);
                }
            }
            cv_text(4, 4 + MI_COUNT * MI_DY + 4, &FONT_S, "PRESETS MOVE  KNOB 1 SET", C_DIM);
            cv_text(4, 4 + MI_COUNT * MI_DY + 18, &FONT_S, "OCT+ OK   OCT- BACK", C_DIM);
        }
        cv_oy = 0;
        cv_blit(0, H_HEAD + 1 + pass * 124u);
    }
}

static void enc_drop(void)                             /* knob turns nobody takes */
{
    uint32_t k;
    for (k = 0; k < NE; k++)
        panel_enc(k);
}

static void menu_close(void)
{
    if (song.playing || transport_req)
        settings_later = 1;                            /* (a flash write stops the audio: once stopped) */
    else
        settings_save();                               /* palette / panel table, if changed */
    ui.menu = 0;
    ui.force = 1;
    go_home();
}

/* menu: PRESETS moves, OCT+ confirms, OCT- cancels (ABOUT -> list -> close) */
static void menu_input(uint32_t pressed)
{
    int32_t s, preset_move;
    uint32_t ok = (pressed >> panel.btn[B_OCTUP]) & 1u, back = (pressed >> panel.btn[B_OCTDN]) & 1u;
    if (back) {
        if (ui.menu >= 2)
            ui.menu = ui.menu==7?6:ui.menu == 4 ? 3 : 1, ui.force = 1;
        else
            menu_close();
        return;
    }
    preset_move = panel_enc(EN_PRESET);
    if ((s = preset_move) != 0 && ui.menu == 1)
        ui.menu_sel = (uint8_t)((ui.menu_sel + (s > 0 ? 1u : MI_COUNT - 1u)) % MI_COUNT);
    s = panel_enc(EN_K1);
    if(ui.menu==5) {enc_drop();return;}
    if(ui.menu>=6) {
        if(ui.menu==6 && preset_move) my_song_sel=(uint8_t)((my_song_sel+(preset_move>0?1u:4u))%5u);
        if(ok && ui.menu==6) {
            if(song.playing||transport_req||song.rec||rec_wait||ci_on)ui_message("STOP PLAYBACK FIRST");
            else if(my_song_sel==4) {my_song_exit();menu_close();studio_open(SC_SONG);}
            else ui.menu=7;
            ui.force=1;
        } else if(ok && ui.menu==7) {
            if(song.playing||transport_req||song.rec||rec_wait||ci_on)ui_message("STOP PLAYBACK FIRST");
            else {mycelium_song_load(my_song_sel);menu_close();studio_open(SC_SONG);ui_message("PLAY EPIC SONG");}
            ui.force=1;
        }
        enc_drop();return;
    }
    if (ui.menu >= 3) {
        int32_t move = preset_move;
        if (ui.menu == 3 && move) my_groove_sel = (uint8_t)((my_groove_sel + (move > 0 ? 1u : 6u)) % 7u);
        if (ok && ui.menu == 3) {
            if (my_groove_sel == 6) ui.menu = 1;
            else if (song.playing || transport_req || rec_wait || ci_on) ui_message("STOP PLAYBACK FIRST");
            else ui.menu = 4;
            ui.force = 1;
        } else if (ok && ui.menu == 4) {
            if (song.playing || transport_req || rec_wait || ci_on) ui_message("STOP PLAYBACK FIRST");
            else {
                uint32_t i;
                my_song_exit();
                for (i = 0; i < NTRK; i++) track_defaults(&trk[i]);
                for (i = 0; i < NPART; i++) {
                    uint32_t e = 1, p = i == 1 ? 21u : 23u;
                    if (i == 0) e = 0, p = my_groove_sel == 4 ? 17u : 16u;
                    if (my_groove_sel == 3 && i == 1) e = 0, p = 18u;
                    if (my_groove_sel == 4 && i == 1) p = 25u;
                    set_engine_of(&trk[i], e); apply_preset_to(&trk[i], p);
                }
                TDRUM->p[P_E0] = my_groove_sel == 0 ? MY_DRUM_FOREST : my_groove_sel == 4 ? MY_DRUM_HITECH : MY_DRUM_PSY;
                song.solo = 0;
                song.g[G_DRLVL] = 100; song.g[G_DRREV] = 12;
                song.g[G_DUST] = 0; song.g[G_FILT] = 0; song.g[G_ROUTE] = 0; song.g[G_DUCK] = 28;
                song.g[G_ENGGO]=32;
                song.g[G_NAME]=0;
#if FELUCCA_ARRANGER
                arrangement_enabled = 0; live_req = -1; live_sec = -1;
#endif
                if (my_groove_sel) {
                    mycelium_psy_groove(my_groove_sel);
                    if(my_groove_sel==5) {trk[0].p[P_DIST]=22;trk[0].p[P_LEVEL]=110;}
                    trk[1].p[P_LEVEL] = 92; trk[2].p[P_LEVEL] = 88;
                } else mycelium_factory_groove();
                song.rec = 0; mycelium_start_ready = 1;
                menu_close(); ui_message("PLAY A KEY TO START");
            }
        }
        enc_drop();
        return;
    }
    if (s != 0 && ui.menu == 1 && ui.menu_sel == MI_COLOR) {
        settings.palette = (settings.palette + (s > 0 ? 1u : NPALETTES - 1u)) % NPALETTES;
        palette_set(settings.palette);              /* (the menu signature redraws) */
    }
    if ((s != 0 || ok) && ui.menu == 1 && (ui.menu_sel == MI_LOWCUT || ui.menu_sel == MI_ZOOM)) {
        /* KNOB 1: right = ON, left = OFF; OCT+ toggles */
        uint32_t *v = ui.menu_sel == MI_LOWCUT ? &settings.lowcut : &settings.zoom;
        *v = s > 0 ? 1u : s < 0 ? 0u : !*v;
        fx_lowcut = (uint8_t)(settings.lowcut != 0);
        ok = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_USB) {     /* right FULL, left MASTER; OCT+ toggles */
        usb_full = (uint8_t)(s > 0 ? 1u : s < 0 ? 0u : !usb_full);
        ok = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && ui.menu_sel == MI_NOTES) {   /* (the same: right ON, left OFF) */
        lights_notes = (uint8_t)(s > 0 ? 1u : s < 0 ? 0u : !lights_notes);
        ok = 0;
    }
    if ((s != 0 || ok) && ui.menu == 1 && (ui.menu_sel == MI_LIGHTS || ui.menu_sel == MI_KEYS)) {
        /* KNOB 1: brighter / dimmer (stops at the ends); OCT+ steps round */
        uint8_t *v = ui.menu_sel == MI_LIGHTS ? &lights_lvl : &lights_keys;
        uint32_t n = ui.menu_sel == MI_LIGHTS ? LIGHTS_N : KEYS_N;
        if (s > 0 && *v + 1u < n)
            (*v)++;
        else if (s < 0 && *v > 0u)
            (*v)--;
        else if (!s)
            *v = (uint8_t)((*v + 1u) % n);
        if (ui.menu_sel == MI_KEYS && lights_keys && !lights_lvl)
            lights_lvl = LIGHTS_LOW;                   /* keys lit need a level: the lowest */
        ok = 0;
    }
    if (ok && ui.menu == 1) {
        switch (ui.menu_sel) {
        case MI_COLOR:                                 /* OCT+ steps through the palettes too */
            settings.palette = (settings.palette + 1u) % NPALETTES;
            palette_set(settings.palette);
            break;
        case MI_PANEL:
            panel_setup();
            ui.force = 1;
            break;
        case MI_ABOUT:
            ui.menu = 2;
            ui.force = 1;
            break;
        case MI_JAM:
            ui.menu = 3; my_groove_sel = 1; ui.force = 1;
            break;
        case MI_SONGS:
            ui.menu=6;my_song_sel=0;ui.force=1;break;
        case MI_LOGO:
            my_fill=0;ui.menu=5;ui.force=1;
            break;
        default:
            menu_close();
            break;
        }
    }
    enc_drop();                                        /* swallow the rest while the menu is up */
}

