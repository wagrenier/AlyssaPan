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
    u_char rest08[4];
    u_char press[12];                   /* 0x0c: the pressure bytes (0..255), in the DualShock 2's order
                                           (PAD_PRESS_*; camera_main reads them, 2026-11-10) */
    u_char rest18[4];
    u_char stick[4];                    /* 0x1c: the analog bytes (ControllerClatteringUpdate reads them in
                                           the order 2, 3, 0, 1; which is which stick: gaPadRead's TU) */
    u_char rest20[4];
} PAD_DATA;
static_assert(offsetof(PAD_DATA, stick) == 0x1c, "PAD_DATA.stick");
static_assert(offsetof(PAD_DATA, press) == 0x0c, "PAD_DATA.press");
// PAD_DATA.press: which byte follows which button (camera_main pairs each with its SCE_PAD* bit).
#define PAD_PRESS_RIGHT 0
#define PAD_PRESS_LEFT 1
#define PAD_PRESS_UP 2
#define PAD_PRESS_DOWN 3
#define PAD_PRESS_TRIANGLE 4
#define PAD_PRESS_CIRCLE 5
#define PAD_PRESS_CROSS 6
#define PAD_PRESS_SQUARE 7
#define PAD_PRESS_L1 8
#define PAD_PRESS_R1 9
#define PAD_PRESS_L2 10
#define PAD_PRESS_R2 11
static_assert(sizeof(PAD_DATA) == 0x24, "PAD_DATA");
static_assert(offsetof(PAD_DATA, trg) == 0x04, "PAD_DATA.trg");
static_assert(offsetof(PAD_DATA, key) == 0x06, "PAD_DATA.key");

void gaPadRead(PAD_DATA *pad, int mode);                                                 // CT3 0x018a0b40
// small == 1 runs the small motor; large: the large motor's level (0..255). From tiVibrationOn / Off.
void gaPadSetVibration(PAD_DATA *pad, u_char small, u_char large);                        // CT3 0x018a0ac0
