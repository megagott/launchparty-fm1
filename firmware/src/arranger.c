/* SPDX-License-Identifier: GPL-3.0-only */
/* Firmware adapter. Scene snapshots are already resident in RAM. No flash
 * operation, UI draw or allocation is allowed from these audio callbacks. */
#include "arranger.h"
static arr_config_t arrangement;
static arr_clock_t arrangement_clock;
static uint8_t arrangement_enabled;
/* Factory songs are procedural virtual scenes; user project slots stay intact. */
static void (*my_song_apply)(uint32_t scene);
static arr_config_t my_user_order;
static uint8_t my_user_order_kept;
static int my_song_active(void) { return my_song_apply && arrangement.reserved[0]==0x4du && arrangement.reserved[1]>=1u && arrangement.reserved[1]<=4u; }
static void my_song_exit(void) {
    if(my_song_active() && my_user_order_kept) arrangement=my_user_order;
    my_user_order_kept=0; arrangement_enabled=0;
}
static uint32_t arrangement_ready(void);              /* project.c */
static void arrangement_apply(uint32_t scene);         /* project.c */

static int arrangement_start(void)
{
    int scene;
    if (!arrangement_enabled) return 1;
    scene = arr_begin(&arrangement_clock, &arrangement, arrangement_ready());
    if (scene < 0) return 0;
    arrangement_apply((uint32_t)scene);
    song.rec = 0;                                    /* song playback does not overwrite patterns */
    return 1;
}
