/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* GM drum part (track 4): the SAMPLE engine's KIT set (General MIDI percussion map,
 * tools/gen_samples.py GM_KIT), played by its step pattern, the keys when track 4
 * is selected, and its own MIDI channel (GLO -> DRUMS, default 10). Its own voices
 * (outside the parts' voice budget). One-shots: note-offs are ignored; a closed or
 * pedal hi-hat chokes the open one. LEVEL / REV: GLO > DRUMS (G_DRLVL, G_DRREV);
 * PAN and MUTE: the drum track's P_PAN / P_MUTE. Rendered from the audio ISR. */
#define NDRUM 6
#include "drum_synth.c"       /* synthesised kits (DS_KITS) */
/* P_E0 was unused on the drum track: it holds the kit. 0..4: the GM sample kit and its four
 * treatments (as before: old projects keep their kit), 5..: the synthesised kits. */
#define DRUM_SAMPLED 5u
#define DRUM_KITS (DRUM_SAMPLED + DS_NKITS)
static const char *const DRUM_KIT_NAMES[] = {"ACOUSTIC", "DEEP", "TIGHT", "BRIGHT", "DUST", DS_KIT_NAME_LIST};
static const char *const DRUM_KIT_STYLES[] = {"STUDIO", "SOFT", "PUNCHY", "BRIGHT", "DUSTY", DS_KIT_STYLE_LIST};
static uint32_t drum_kit(void) { return (uint32_t)clamp(TDRUM->p[P_E0], 0, DRUM_KITS - 1); }
#define MY_DRUM_PSY (DRUM_SAMPLED + 32u)
#define MY_DRUM_FOREST (DRUM_SAMPLED + 33u)
#define MY_DRUM_HITECH (DRUM_SAMPLED + 34u)
#define DRUM_DEFAULT_KIT MY_DRUM_PSY
static void mycelium_drum_step(int32_t step)
{
    uint32_t k = drum_kit(), pos = k == MY_DRUM_FOREST ? 1u : k == MY_DRUM_HITECH ? 2u : 0u;
    pos = (pos + (step > 0 ? 1u : 2u)) % 3u;
    TDRUM->p[P_E0] = (int16_t)(MY_DRUM_PSY + pos);
}

static struct {
    voice_t v[NDRUM];
    uint32_t age;
    int16_t set;                 /* SMP_SETS index of "PERC" (GM map), -1 = none */
    int32_t tail;                /* declick: the last output of cut voices, decaying */
    int32_t peak;                /* largest |output| since the UI last looked (TRACKS meter) */
    uint8_t kit[NDRUM];
    int32_t filter[NDRUM], env[NDRUM];
    uint8_t synth[NDRUM];        /* the voice plays a synthesised kit (ds[]) */
    dsv_t ds[NDRUM];
    volatile uint16_t hits;      /* bit per lane hit since the UI last looked (pads, key LEDs) */
    volatile uint8_t kick;       /* a kick was hit (fx.c DUCK) */
    int32_t a0, a1;              /* the drum track's mute / solo attenuation over this block (fx.c), Q15, 0 = heard */
} drums = {.set = -2};

static int32_t ds_buf[CTL];
/* Shared dry low-end bus: part 1 and the kick voices. */
static int32_t my_kb_l[CTL], my_kb_r[CTL];
static uint8_t my_kb_active;

static int32_t drum_set(void)
{
    uint32_t i;
    if (drums.set == -2) {
        drums.set = -1;
        for (i = 0; i < SMP_NSETS; i++)
            if (str_eq(SMP_SETS[i].name, "PERC"))
                drums.set = (int16_t)i;
    }
    return drums.set;
}

/* ------------------------------------------------------------ lanes --- */
/* The drum track's 16 sounds, one per white key, F3 (kick) .. G5 (cowbell): kicks, snare and clap,
 * the hi-hats, rim and a second snare, the toms, the cymbals, the percussion. Each plays a GM note
 * (the sampled kits: their samples; the synthesised kits: drum_synth.c DS_MAP). A black key plays
 * the lane of the white key left of it (two fingers on one sound). */
static const uint8_t LANE_NOTE[DRUM_LANES] = {36, 35, 38, 39, 42, 46, 44, 37, 40, 43, 48, 49, 51, 70, 63, 56};
static const char *const LANE_NAME[DRUM_LANES] = {
    "KICK", "KICK 2", "SNARE", "CLAP", "HAT", "OPEN HAT", "PEDAL", "RIM",
    "SNARE 2", "LOW TOM", "HI TOM", "CRASH", "RIDE", "SHAKER", "CONGA", "COWBELL"};
static const char *const LANE_SHORT[DRUM_LANES] = {           /* 5 characters: tiles, dials */
    "kick", "kick2", "snare", "clap", "hat", "open", "pedal", "rim",
    "snr 2", "tom l", "tom h", "crash", "ride", "shake", "conga", "bell"};
/* a GM note (MIDI in, old projects) -> its lane: the nearest sound of the 16 (35..81; below: kick, above: shaker) */
static const uint8_t LANE_OF_GM[81 - 35 + 1] = {
    /* 35 */ 1, 0, 7, 2, 3, 8, 9, 4, 9, 6,
    /* 45 */ 9, 5, 10, 10, 11, 10, 12, 11, 12, 13,
    /* 55 */ 11, 15, 11, 13, 12, 14, 14, 14, 14, 14,
    /* 65 */ 14, 14, 15, 15, 13, 13, 15, 15, 13, 13,
    /* 75 */ 7, 7, 7, 14, 14, 15, 15};
static uint32_t lane_of_note(uint32_t note)
{
    return note < 35u ? 0u : note > 81u ? 13u : LANE_OF_GM[note - 35u];
}
/* the lane of key k (0 = F3 .. 26 = G5): white keys in order, a black key the white key left of it */
static uint32_t lane_of_key(uint32_t k)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};   /* from F */
    int32_t i = W[k % 12u];
    if (i < 0)
        i = W[(k - 1u) % 12u];
    return (uint32_t)((int32_t)(k / 12u) * 7 + i) & 15u;
}
/* the white key of lane l (key index), for the key LEDs */
static uint32_t key_of_lane(uint32_t l)
{
    static const uint8_t K[7] = {0, 2, 4, 6, 7, 9, 11};      /* F G A B C D E */
    return (l / 7u) * 12u + K[l % 7u];
}

/* drum steps: a lane's bit, level, ratchet */
static int dstep_has(const dstep_t *s, uint32_t l) { return (s->on[(l >> 3) & 1u] >> (l & 7u)) & 1u; }
static uint32_t dstep_lvl(const dstep_t *s, uint32_t l) { return (s->lvl[(l >> 2) & 3u] >> ((l & 3u) * 2u)) & 3u; }
static uint32_t dstep_rat(const dstep_t *s, uint32_t l) { return (s->rat[(l >> 2) & 3u] >> ((l & 3u) * 2u)) & 3u; }
static uint32_t dstep_mask(const dstep_t *s) { return (uint32_t)s->on[0] | (uint32_t)s->on[1] << 8; }
static void dstep_set(dstep_t *s, uint32_t l, uint32_t lvl, uint32_t rat)   /* lane on, with its level and ratchet */
{
    uint32_t sh = (l & 3u) * 2u, b = (l >> 2) & 3u;
    s->on[(l >> 3) & 1u] |= (uint8_t)(1u << (l & 7u));
    s->lvl[b] = (uint8_t)((s->lvl[b] & ~(3u << sh)) | (lvl & 3u) << sh);
    s->rat[b] = (uint8_t)((s->rat[b] & ~(3u << sh)) | (rat & 3u) << sh);
}
static void dstep_clr(dstep_t *s, uint32_t l)                                /* lane off */
{
    uint32_t sh = (l & 3u) * 2u, b = (l >> 2) & 3u;
    s->on[(l >> 3) & 1u] &= (uint8_t)~(1u << (l & 7u));
    s->lvl[b] &= (uint8_t)~(3u << sh);
    s->rat[b] &= (uint8_t)~(3u << sh);
}

/* the velocity of a level: as played (LV_NORM: vel), ghost, soft, hard */
static uint32_t lvl_vel(uint32_t lvl, uint32_t vel)
{
    static const uint8_t V[4] = {0, 42, 72, 127};
    return lvl & 3u ? V[lvl & 3u] : vel;
}
/* a MIDI velocity -> the level it records as (keys play 100: LV_NORM) */
static uint32_t vel_lvl(uint32_t vel)
{
    return vel < 56u ? LV_GHOST : vel < 88u ? LV_SOFT : vel < 116u ? LV_NORM : LV_HARD;
}

static int32_t env_tick(track_t *t, voice_t *v);
static void my_kick_env_on(voice_t *v)
{
    v->env=TDRUM->p[P_ATK]?0:(1<<24);v->env_out=v->env>>9;
    v->stage=TDRUM->p[P_ATK]?1:2;
    v->ph[2]=(FS*60u*(uint32_t)TDRUM->p[P_SGATE])/((uint32_t)clamp(song.g[G_BPM],40,240)*128u);
}
static int32_t my_kick_env_tick(voice_t *v,uint32_t n)
{
    if(!TDRUM->p[P_ED_FX] || (v->note!=35u && v->note!=36u))return -1;
    if(v->ph[2]>n)v->ph[2]-=n;else {v->ph[2]=0;v->stage=3;}
    v->env_out=env_tick(TDRUM,v);return v->env_out;
}
static void drum_on(uint32_t note, uint32_t vel)
{
    int32_t si = drum_set();
    const smp_set_t *set;
    voice_t *v = &drums.v[0];
    uint32_t i, zi = 0xFFFFu, kit = drum_kit();
    uint32_t mykick = kit >= MY_DRUM_PSY && kit <= MY_DRUM_HITECH && (note == 35u || note == 36u);
    uint32_t myzone=0xFFFFu;
    for(i=0;i<sizeof(MYCELIUM_DRUM_ZONES)/sizeof(MYCELIUM_DRUM_ZONES[0]);i++)
        if(kit==DRUM_SAMPLED+MYCELIUM_DRUM_ZONES[i].kit && note==MYCELIUM_DRUM_ZONES[i].note)myzone=MYCELIUM_DRUM_ZONES[i].zone;
    if (note != 76u && note != 77u)                 /* the pads and key LEDs (not the click's wood block) */
        drums.hits |= (uint16_t)(1u << lane_of_note(note));
    if (note == 35u || note == 36u)
        drums.kick = 1;                             /* (DUCK) */
    if (kit >= DRUM_SAMPLED && !mykick && myzone==0xFFFFu) { /* remaining synthesized percussion */
        if (note == 42u || note == 44u)             /* hi-hat choke */
            for (i = 0; i < NDRUM; i++)
                if (drums.v[i].active && drums.v[i].note == 46u) {
                    drums.v[i].active = 0;
                    drums.tail += drums.v[i].s[7];
                }
        for (i = 0; i < NDRUM; i++) {
            if (!drums.v[i].active) {
                v = &drums.v[i];
                break;
            }
            if (drums.v[i].age < v->age)
                v = &drums.v[i];
        }
        if (v->active)
            drums.tail += v->s[7];
        i = (uint32_t)(v - drums.v);
        v->note = (uint8_t)note;
        v->vel = (uint8_t)vel;
        v->active = 1;
        my_kick_env_on(v);
        v->s[7] = 0;
        v->age = ++drums.age;
        drums.synth[i] = 1;
        drums.kit[i] = (uint8_t)kit;
        ds_on(&drums.ds[i], &DS_KITS[kit - DRUM_SAMPLED], note, vel);
        return;
    }
    if (si < 0)
        return;
    set = &SMP_SETS[si];
    for (i = 0; i < set->nz; i++)
        if (note >= SMP_ZONES[set->z0 + i].lo && note <= SMP_ZONES[set->z0 + i].hi)
            zi = set->z0 + i;
    if(myzone!=0xFFFFu)zi=myzone;
    else if (mykick) zi = MYCELIUM_KICK_ZONE;
    if (zi == 0xFFFFu)
        return;
    if (note == 42u || note == 44u)                 /* hi-hat choke */
        for (i = 0; i < NDRUM; i++)
            if (drums.v[i].active && drums.v[i].note == 46u) {
                drums.v[i].active = 0;
                drums.tail += drums.v[i].s[7];          /* fade what it was playing, not a step */
            }
    for (i = 0; i < NDRUM; i++) {                   /* free voice, else the oldest */
        if (!drums.v[i].active) {
            v = &drums.v[i];
            break;
        }
        if (drums.v[i].age < v->age)
            v = &drums.v[i];
    }
    if (v->active)
        drums.tail += v->s[7];                      /* stolen voice: fade its last value */
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->active = 1;
    my_kick_env_on(v);
    v->s[7] = 0;
    v->age = ++drums.age;
    v->s[4] = (int32_t)zi;
    v->ph[0] = v->ph[1] = 0;
    v->s[0] = v->s[1] = v->s[2] = 0;
    v->s[3] = sample_next(&SMP_ZONES[zi], v, 0);
    v->s[5] = (int32_t)((pow2_q16((int32_t)(mykick ? 33u : note) * 16 - SMP_ZONES[zi].root16) >> 8) * (SMP_ZONES[zi].rate >> 8));
    {
        uint32_t vi = (uint32_t)(v - drums.v);
        int32_t shift = kit == 1u ? (note <= 36u ? -5 : -2) : kit == 3u ? 2 : kit == 4u ? -1 : 0;
        drums.kit[vi] = (uint8_t)kit;
        drums.synth[vi] = 0;
        drums.filter[vi] = 0;
        drums.env[vi] = 32767;
        if (shift) v->s[5] = (int32_t)((pow2_q16((int32_t)note * 16 + shift * 16 - SMP_ZONES[zi].root16) >> 8) * (SMP_ZONES[zi].rate >> 8));
    }
}

/* adds the drums into the dry mix and the reverb send; mono != 0: into mono instead, before the
 * pan and the send (the SLICER, slicer.c slicer_drums, does those after it) */
static inline int32_t my_drum_dist(int32_t s) {
    int32_t d=clamp(TDRUM->p[P_DIST],0,127),g,x,y;
    if(!d)return s;
    g=4096+d*d*2;
    x=clamp(s,-230000,230000);
    y=softclip(((x>>2)*g)>>10);
    return s+mulq15(y-s,d*258);
}
static inline void drums_mix(int32_t *ml, int32_t *mr, int32_t *rev, int32_t *mono, uint32_t n)
{
    uint32_t k, i;
    int32_t lvl = song.g[G_DRLVL] * 200, send = song.g[G_DRREV] * 258, pk = drums.peak;
    int32_t pan = trk[TRK_DRUM].p[P_PAN], gl = 4096 - (pan > 0 ? pan * 64 : 0), gr = 4096 + (pan < 0 ? pan * 64 : 0);
    for (i = 0; i < n && drums.tail; i++) {         /* declick tail, ~0.4 ms */
        if (mono) {
            mono[i] += drums.tail;
        } else {
            ml[i] += drums.tail;
            mr[i] += drums.tail;
        }
        drums.tail -= drums.tail / 16 + (drums.tail > 0 ? 1 : drums.tail < 0 ? -1 : 0);
    }
    for (k = 0; k < NDRUM; k++) {               /* synthesised voices: render, then as below */
        voice_t *v = &drums.v[k];
        uint32_t m = n < CTL ? n : CTL;             /* (the mix runs in blocks of CTL) */
        int32_t e0,e1;
        if (!v->active || !drums.synth[k])
            continue;
        e0=v->env_out;e1=my_kick_env_tick(v,m);
        if (!ds_render(&drums.ds[k], ds_buf, m))
            v->active = 0;
        for (i = 0; i < m; i++) {
            int32_t s = mulq15(ds_buf[i], mulq15(lvl, 32767 - drums.a0 - (((drums.a1 - drums.a0) * (int32_t)i) >> CTL_LOG2)));
            if(e1>=0)s=mulq15(s,e0+(((e1-e0)*(int32_t)i)>>CTL_LOG2));
            s=my_drum_dist(s);
            v->s[7] = s;
            if (s > pk || -s > pk)
                pk = s < 0 ? -s : s;
            if (mono) {
                if(my_kb_active && (v->note==35u || v->note==36u)) {my_kb_l[i]+=s;my_kb_r[i]+=s;}
                else mono[i] += s;
                continue;
            }
            if (my_kb_active && (v->note == 35u || v->note == 36u)) {
                my_kb_l[i] += (s * gl) >> 12; my_kb_r[i] += (s * gr) >> 12;
            } else {
                ml[i] += (s * gl) >> 12; mr[i] += (s * gr) >> 12;
            }
            if (send)
                rev[i] += mulq15(s, send);
        }
        if (!v->active) {
            drums.tail += v->s[7];                  /* ended: no step at the end */
            v->s[7] = 0;
        }
    }
    for (k = 0; k < NDRUM; k++) {
        voice_t *v = &drums.v[k];
        const smp_zone_t *z = &SMP_ZONES[v->s[4]];
        if (drums.synth[k])
            continue;
        uint32_t frac = v->ph[1], stepq = (uint32_t)v->s[5];   /* Q16 source samples per output (drum_on) */
        int32_t g,e0,e1;
        if (!v->active)
            continue;
        e0=v->env_out;e1=my_kick_env_tick(v,n);
        g = mulq15(lvl, v->vel * 258);
        g += g * 3 >> 2;                           /* x1.75 (+5 dB): as loud as the synthesised kits */
        for (i = 0; i < n; i++) {
            int32_t s;
            frac += stepq;
            while (frac >= 65536u) {
                frac -= 65536u;
                v->s[2] = v->s[3];
                if (v->ph[0] >= z->n) {
                    v->active = 0;
                    break;
                }
                v->s[3] = sample_next(z, v, 0);
            }
            if (!v->active)
                break;
            s = v->s[2] + (((v->s[3] - v->s[2]) * (int32_t)(frac >> 1)) >> 15);
            s = mulq15(s, mulq15(g, 32767 - drums.a0 - (((drums.a1 - drums.a0) * (int32_t)i) >> CTL_LOG2)));
            if(e1>=0)s=mulq15(s,e0+(((e1-e0)*(int32_t)i)>>CTL_LOG2));
            if (drums.kit[k] == 1u || drums.kit[k] == 4u) {
                drums.filter[k] += (s - drums.filter[k]) >> (drums.kit[k] == 1u ? 2 : 1);
                s = drums.filter[k];
                if (drums.kit[k] == 4u) s = (s >> 8) * 256;
            } else if (drums.kit[k] == 2u) {
                s = mulq15(s, drums.env[k]);
                drums.env[k] -= (drums.env[k] >> 11) + 1;
                if (drums.env[k] <= 0) v->active = 0;
            }
            s=my_drum_dist(s);
            v->s[7] = s;
            if (s > pk || -s > pk)
                pk = s < 0 ? -s : s;
            if (mono) {
                if(my_kb_active && (v->note==35u || v->note==36u)) {my_kb_l[i]+=s;my_kb_r[i]+=s;}
                else mono[i] += s;
                continue;
            }
            if (my_kb_active && (v->note == 35u || v->note == 36u)) {
                my_kb_l[i] += (s * gl) >> 12; my_kb_r[i] += (s * gr) >> 12;
            } else {
                ml[i] += (s * gl) >> 12; mr[i] += (s * gr) >> 12;
            }
            if (send)
                rev[i] += mulq15(s, send);
        }
        v->ph[1] = frac;
    }
    drums.peak = pk;
}
static void drums_render(int32_t *ml, int32_t *mr, int32_t *rev, uint32_t n) { drums_mix(ml, mr, rev, 0, n); }
static void drums_render_mono(int32_t *mono, uint32_t n) { drums_mix(0, 0, 0, mono, n); }
