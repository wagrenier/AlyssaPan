// game/asMothRouteSearch.h -- CT3 game/asMothRouteSearch.c: MothInfo (the moths a script arms) and the moth's
// route search around scene collision (2026-11-11). The record layout comes from the arming command
// (asXcmd_I_have_x_Mothes, asXcmd.c) and asMothOperate (2026-11-12).
#pragma once

#include "sdk/scetypes.h"

// One moth of the bank asXcmd_I_have_x_Mothes armed (MothInfo.list, 0x50 bytes each on the EE, gHeapJunc).
// layout: host (mem is a pointer; the EE offsets are in the comments, the port allocates sizeof(MOTH)).
// Fields named by asMothOperate (2026-11-12), which runs the state machine below.
typedef struct MOTH {
    u_char state;                       /* EE 0x00: MOTH_STATE_* */
    u_char phase;                       /* EE 0x01: MOTH_STATE_FLY's MOTH_PHASE_* */
    u_char next;                        /* EE 0x02: the state to take at the end of this frame, 0 = stay */
    u_char bank;                        /* EE 0x03: the route bank: its parts are the route points, its
                                           collision asMothRouteSearch tests (0 when resting) */
    short frames;                       /* EE 0x04: frames in the state (1 on its first), held at -2 */
    short arg[4];                       /* EE 0x06: the script's four numbers per moth (MOTH_ARG_*) */
    short angle;                        /* EE 0x0e: MOTH_STATE_CIRCLE's angle (ahSin units, + 0x400 a frame) */
    short radius;                       /* EE 0x10: MOTH_STATE_CIRCLE's radius about Alyssa */
    short swing;                        /* EE 0x12: MOTH_STATE_CIRCLE's up-down swing (not cleared when armed) */
    short home[3];                      /* EE 0x14: route-bank parts the body, look and look2 parts start at */
    void *mem;                          /* EE 0x1c: gHeapJunc block: u_short[routeCount], the route's
                                           route-bank parts; freed by asDestroyMothInfo */
    short routeCount;                   /* EE 0x20 */
    short routeStep;                    /* EE 0x22: the route point flown to */
    short speed;                        /* EE 0x24: MOTH_FLY_SPEED once flying */
    short routeSearch;                  /* EE 0x26: set by states FLY / CIRCLE for this frame's move */
    int timer;                          /* EE 0x28: ticks (gGlobalNextTicksInterval a frame) since the
                                           last flutter SE */
    int nearTicks;                      /* EE 0x2c: MOTH_STATE_CIRCLE: ticks on its circle */
    int farTicks;                       /* EE 0x30: MOTH_STATE_CIRCLE: ticks off it */
    float target[4];                    /* EE 0x40: where the moth heads (zeroed when armed) */
} MOTH;

#define MOTH_STATE_REST 1               /* armed, no route: hidden; next == MOTH_STATE_WAIT places it home */
#define MOTH_STATE_WAIT 2               /* a route is set: waits MOTH_RETURN_TICKS after the stage's last
                                           moth went (MothInfo.endTicks), then flies */
#define MOTH_STATE_FLY 3                /* follows its route; near Alyssa (MOTH_PHASE_SEEK) it circles her */
#define MOTH_STATE_CIRCLE 4             /* circles Alyssa; long enough on the circle and it lands on her */
#define MOTH_STATE_ON_ALYSSA 5          /* on Alyssa: a pad rattle (ControllerClattering) shakes it off */
#define MOTH_STATE_HIT 6                /* an attack hit its hit shape (MOTH_ARG_HIT): the death SE */
#define MOTH_STATE_DEAD 7
#define MOTH_PHASE_AIM 0                /* turn the look parts to the route point, then MOTH_PHASE_SEEK */
#define MOTH_PHASE_SEEK 1               /* may turn on Alyssa (MOTH_STATE_CIRCLE) */
#define MOTH_PHASE_AIM_AWAY 2           /* after leaving Alyssa: as AIM, then MOTH_PHASE_AWAY */
#define MOTH_PHASE_AWAY 3               /* does not turn on Alyssa until the next route point */
#define MOTH_ARG_BODY 0                 /* MOTH.arg: the moth's part (and its sound object) in the moth bank */
#define MOTH_ARG_LOOK 1                 /* the part placed ahead of the body (where it looks) */
#define MOTH_ARG_LOOK2 2                /* a second look part (the look's position at the body's height) */
#define MOTH_ARG_HIT 3                  /* its hit shape's part (HIT_DATA.part) */

// MothInfo (sbss 0x0193ced0, 0x28 bytes). layout: host (list is a pointer; EE offsets).
typedef struct MOTH_INFO {
    u_char active;                      /* EE 0x00: asXcmd_I_have_x_Mothes armed the bank */
    u_char bank;                        /* EE 0x01: the bank whose parts are the moths */
    u_char count;                       /* EE 0x02: moths in list (the script's numbers / 4) */
    u_char shown;                       /* EE 0x03: TRIG_EVENT_MOTHS_OUT as asMothOperate last read it (0 when armed) */
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
