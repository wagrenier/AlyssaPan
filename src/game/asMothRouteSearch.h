// game/asMothRouteSearch.h -- CT3 game/asMothRouteSearch.c: MothInfo (the moths a script arms) and the moth's
// route search around scene collision (2026-11-11). The record layout comes from the arming command
// (asXcmd_I_have_x_Mothes, asXcmd.c) and asMothOperate's uses; the fields not named here stay wNN until
// asMothOperate's TU reads them.
#pragma once

#include "sdk/scetypes.h"

// One moth of the bank asXcmd_I_have_x_Mothes armed (MothInfo.list, 0x50 bytes each on the EE, gHeapJunc).
// layout: host (mem is a pointer; the EE offsets are in the comments, the port allocates sizeof(MOTH)).
typedef struct MOTH {
    u_char state;                       /* EE 0x00: 1 when armed; asMothOperate's states (3 and 4 search a
                                           route, 7 once timer passes 199) */
    u_char w01;                         /* EE 0x01 */
    u_char w02;                         /* EE 0x02 */
    u_char bank;                        /* EE 0x03: the collision bank asMothRouteSearch tests (0 when armed) */
    short w04;                          /* EE 0x04 */
    short arg[4];                       /* EE 0x06: the script's four numbers per moth (arg[0] the SE number
                                           asMothOperate plays) */
    short w0e;                          /* EE 0x0e */
    short w10;                          /* EE 0x10 */
    short w12;                          /* EE 0x12: not cleared when armed */
    short w14;                          /* EE 0x14 */
    short w16;                          /* EE 0x16 */
    short w18;                          /* EE 0x18 */
    void *mem;                          /* EE 0x1c: gHeapJunc block, freed by asDestroyMothInfo */
    short w20;                          /* EE 0x20 */
    short w22;                          /* EE 0x22 */
    short w24;                          /* EE 0x24 */
    short routeSearch;                  /* EE 0x26: != 0: states 3 / 4 route around collision */
    int timer;                          /* EE 0x28 */
    float target[4];                    /* EE 0x40: where the moth heads (zeroed when armed) */
} MOTH;

// MothInfo (sbss 0x0193ced0, 0x28 bytes). layout: host (list is a pointer; EE offsets).
typedef struct MOTH_INFO {
    u_char active;                      /* EE 0x00: asXcmd_I_have_x_Mothes armed the bank */
    u_char bank;                        /* EE 0x01: the bank whose parts are the moths */
    u_char count;                       /* EE 0x02: moths in list (the script's numbers / 4) */
    u_char w03;                         /* EE 0x03: 0 when armed */
    short clatter;                      /* EE 0x04: the pad rumble slot (ControllerClatteringSet), -1 none */
    u_short stage;                      /* EE 0x06: MOTH_STAGE_*, indexes endTicks */
    int endTicks[2];                    /* EE 0x08: gTotalTicks when that stage's moths first went,
                                           -1 until then (asInitMothInfo) */
    char bankName[16];                  /* EE 0x10: the armed bank's file name */
    MOTH *list;                         /* EE 0x20: gHeapJunc, count records */
    int paused;                         /* EE 0x24: asMothPause's mode (MOTH_PAUSE_*), 0 when armed / freed */
} MOTH_INFO;
extern MOTH_INFO MothInfo;              /* sbss 0x0193ced0 */

#define MOTH_STAGE_OTHER 0              /* MothInfo.stage: any bank but MOTH_STAGE_1A_BANK */
#define MOTH_STAGE_1A 1                 /* MothInfo.stage: the armed bank is MOTH_STAGE_1A_BANK */
#define MOTH_STAGE_1A_BANK "MT1A.BIN"   /* @396 (rodata 0x01936b08) */
#define MOTH_SE_BANK_ARMED "Z_GA"       /* @249 (rodata 0x0193a910): 蛾 (moth), loaded when armed */
#define MOTH_SE_BANK_GONE "Z_REI"       /* @320 (rodata 0x019386b0): loaded back when the moths go */
#define TRIG_EVENT_MOTHS_GONE 0x47      /* asDestroyMothInfo removes this global event */
#define TRIG_EVENT_MOTHS_ARMED 0x8c     /* asXcmd_I_have_x_Mothes removes this global event */

void asInitMothInfo(void);                                                               // CT3 0x01882c10
// Frees the moths; reloadSE != 0 loads the SE bank MOTH_SE_BANK_GONE back.
void asDestroyMothInfo(int reloadSE);                                                    // CT3 0x01882c40
// From `from` to `to` on `bank`'s collision: 0 and out = to when the line is clear. Otherwise the line is
// turned about the hit direction (stepAround per step, 0x10000 a turn) and tilted away from it (stepTilt
// per ring, below 0x8000): 0 and out = the first clear end found, or 1 and out = the first hit point.
int asMothRouteSearch(int bank, float *from, float *to, float *out, int stepAround, int stepTilt); // CT3 0x01882890
