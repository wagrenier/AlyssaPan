// ga/pad_button_read.h -- CT3 ga/pad_button_read.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// One controller's state as gaPadRead leaves it (the global pad holds two: 0x48 bytes).
typedef struct PAD_DATA {
    u_short cnt;                        /* 0x00: buttons held */
    u_short pad02;
    u_short trg;                        /* 0x04: buttons pressed this frame */
    u_short key;                        /* 0x06: key repeat */
    u_char rest[0x1c];
} PAD_DATA;
static_assert(sizeof(PAD_DATA) == 0x24, "PAD_DATA");
static_assert(offsetof(PAD_DATA, trg) == 0x04, "PAD_DATA.trg");
static_assert(offsetof(PAD_DATA, key) == 0x06, "PAD_DATA.key");

void gaPadRead(PAD_DATA *pad, int mode);                                                 // CT3 0x018a0b40
