// ga/pad_button_read.h -- CT3 ga/pad_button_read.c: the ga library's controller reader (start-up, the
// per-frame read with libpad's detection walk, vibration). Reconstructed 2026-11-11.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

#define GA_PAD_PORT_0 0 /* controller port 1 */
#define GA_PAD_PORT_1 1 /* controller port 2 */
#define GA_PAD_PORTS  2 /* gaPadRead walks both; the global pad[] holds one PAD_DATA each */

// One controller's state as gaPadRead leaves it (the global pad holds two: 0x48 bytes).
typedef struct PAD_DATA {
    u_short cnt;                        /* 0x00: buttons held (SCE_PAD* bits, 1 = down) */
    u_short pad02;
    u_short trg;                        /* 0x04: buttons pressed this frame */
    u_short key;                        /* 0x06: key repeat: the press, then after 15 frames held every
                                           3rd frame (GA_PAD_READ_GAME) or every frame (GA_PAD_READ_PLAIN) */
    u_short old;                        /* 0x08: last frame's cnt */
    u_char id;                          /* 0x0a: the controller id the detection found (7 DualShock 2,
                                           4 digital) */
    u_char press_mode;                  /* 0x0b: 1 once pressure mode is on: press[] is filled */
    u_char press[12];                   /* 0x0c: the pressure bytes (0..255), in the DualShock 2's order
                                           (PAD_PRESS_*; camera_main reads them, 2026-11-10) */
    u_char rest18[4];
    u_char stick[4];                    /* 0x1c: the analog bytes mirrored about 0x80 (0x80 centred; 0xff
                                           = left / up, 0x01 = right / down): PAD_STICK_* */
    u_char rest20;
    u_char vibration;                   /* 0x21: gaPadSetVibration's request: gaPadRead drives the
                                           motors while set */
    u_char repeat;                      /* 0x22: frames cnt has stayed the same (key repeat's clock) */
    u_char rest23;
} PAD_DATA;
static_assert(sizeof(PAD_DATA) == 0x24, "PAD_DATA");
static_assert(offsetof(PAD_DATA, trg) == 0x04, "PAD_DATA.trg");
static_assert(offsetof(PAD_DATA, key) == 0x06, "PAD_DATA.key");
static_assert(offsetof(PAD_DATA, old) == 0x08, "PAD_DATA.old");
static_assert(offsetof(PAD_DATA, id) == 0x0a, "PAD_DATA.id");
static_assert(offsetof(PAD_DATA, press_mode) == 0x0b, "PAD_DATA.press_mode");
static_assert(offsetof(PAD_DATA, press) == 0x0c, "PAD_DATA.press");
static_assert(offsetof(PAD_DATA, stick) == 0x1c, "PAD_DATA.stick");
static_assert(offsetof(PAD_DATA, vibration) == 0x21, "PAD_DATA.vibration");
static_assert(offsetof(PAD_DATA, repeat) == 0x22, "PAD_DATA.repeat");

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
#define PAD_PRESS_COUNT 12

// PAD_DATA.stick: which byte is which axis (pad_button_read takes the report's bytes 4..7 in order).
#define PAD_STICK_RX 0
#define PAD_STICK_RY 1
#define PAD_STICK_LX 2
#define PAD_STICK_LY 3
#define PAD_STICK_COUNT 4

// gaPadRead's mode (main.c passes 1 under CONTROLLER_MODE_100, else 0; movie.c the same).
#define GA_PAD_READ_GAME 0  /* the left stick also presses the d-pad; key repeats every 3rd frame */
#define GA_PAD_READ_PLAIN 1 /* buttons as they are; key repeats every frame once it starts */

int gaPadInit(void);                                                                     // CT3 0x018a1130: 1 when both ports opened
void gaPadRead(PAD_DATA *pad, int mode);                                                 // CT3 0x018a0b40
// small == 1 runs the small motor; large: the large motor's level (0..255). From tiVibrationOn / Off.
void gaPadSetVibration(PAD_DATA *pad, u_char small, u_char large);                        // CT3 0x018a0ac0
