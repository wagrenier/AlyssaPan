// game/ct3_types.h -- CT3's shared game structs whose owning TU is not written yet (game/main.c first,
// 2026-11-09). Only the fields read so far are named; each struct moves to its owner's header when that TU
// is written (Character_All_Init's game/Character_Init.c, fileio.c's bank table, ...).
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// One character slot of Character[35] (0x01a4bfc0, 0x580 bytes each: the symtab's 0xc080 / 35).
typedef struct CHARACTER {
    s_char id;                          /* 0x000: -1 = slot free */
    u_char pad001;
    short bank;                         /* 0x002: its bank (BankInfo index), < 0 = none */
    u_char pad004[0x84];
    u_int flags;                        /* 0x088: CHARACTER_FLAG_* */
    u_char pad08c[0x24];
    qword posBefore;                    /* 0x0b0: last frame's position (main copies pos here first) */
    qword pos;                          /* 0x0c0 */
    u_char pad0d0[0x18];
    int rotBefore;                      /* 0x0e8: last frame's value of 0x0ec */
    int rot;                            /* 0x0ec */
    u_char pad0f0[0x490];
} CHARACTER;
static_assert(sizeof(CHARACTER) == 0x580, "CHARACTER");
static_assert(offsetof(CHARACTER, bank) == 0x002, "CHARACTER.bank");
static_assert(offsetof(CHARACTER, flags) == 0x088, "CHARACTER.flags");
static_assert(offsetof(CHARACTER, posBefore) == 0x0b0, "CHARACTER.posBefore");
static_assert(offsetof(CHARACTER, pos) == 0x0c0, "CHARACTER.pos");
static_assert(offsetof(CHARACTER, rotBefore) == 0x0e8, "CHARACTER.rotBefore");

#define CHARACTER_MAX 35                /* 0x23: Character[], AI_flag[], DirectLightCharaCounter[] */
#define CHARACTER_FLAG_SHADOW 0x40000000u /* main: the character's bank casts a shadow polygon */

// One loaded bank of BankInfo[86] (0x01aea360, 0x90 bytes each). A bank heading a group of banks has
// count > 0: the loops over BankInfo skip the rest of the group.
typedef struct BANK_INFO {
    int count;                          /* 0x00: > 0: banks in this group (this one included) */
    int groupCount;                     /* 0x04: added to the index (minus one) to skip the group */
    char name[16];                      /* 0x08: the bank's file ("GAME.BIN", "MENU.BIN", ...) */
    int type;                           /* 0x18: BANK_TYPE_* */
    u_char pad1c[4];
    int loaded;                         /* 0x20: non-zero while the bank is in memory */
    u_char pad24[0x6c];
} BANK_INFO;
static_assert(sizeof(BANK_INFO) == 0x90, "BANK_INFO");
static_assert(offsetof(BANK_INFO, name) == 0x08, "BANK_INFO.name");
static_assert(offsetof(BANK_INFO, type) == 0x18, "BANK_INFO.type");
static_assert(offsetof(BANK_INFO, loaded) == 0x20, "BANK_INFO.loaded");

#define BANK_MAX 86                     /* 0x56: BankInfo[], the per-scene tables (FogCtrlAddr, ...) */
#define BANK_TYPE_SHADOW 1              /* main: banks of this type cast shadow polygons */

// A screen filter's fade (DemoFilterChangeWork, FilterChangeWork: 0x28 bytes). from[0] / to[0] are the
// alpha main sets to 0xff or 0, the next three the colour; timer counts down to 0.
typedef struct FILTER_CHANGE_WORK {
    short active;                       /* 0x00 */
    short mode;                         /* 0x02 */
    int timer;                          /* 0x04 */
    int from[4];                        /* 0x08 */
    int to[4];                          /* 0x18 */
} FILTER_CHANGE_WORK;
static_assert(sizeof(FILTER_CHANGE_WORK) == 0x28, "FILTER_CHANGE_WORK");
static_assert(offsetof(FILTER_CHANGE_WORK, timer) == 0x04, "FILTER_CHANGE_WORK.timer");
static_assert(offsetof(FILTER_CHANGE_WORK, to) == 0x18, "FILTER_CHANGE_WORK.to");

// One fog animation slot of FogAnimeBuff[86] (0x48 bytes): system_init clears its head.
typedef struct FOG_ANIME {
    short a;                            /* 0x00 */
    short b;                            /* 0x02 */
    u_char c;                           /* 0x04 */
    u_char d;                           /* 0x05 */
    u_char rest[0x42];
} FOG_ANIME;
static_assert(sizeof(FOG_ANIME) == 0x48, "FOG_ANIME");
