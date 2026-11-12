// game/asUseArrow.h -- CT3 game/asUseArrow.c: Alyssa's bow arrows (ArrowInfo and its arrow records).
// Slice 1 (2026-11-11): the record layout from asXcmd_I_have_x_Arrows (the allocator, asXcmd.c),
// asShootArrowSetting, asAdjustArrowDirection and asArrowOperate's field uses.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// One arrow of the bank that asXcmd_I_have_x_Arrows armed (ArrowInfo.list, 0x50 bytes each, gHeapJunc).
// Every field is a value: the EE layout holds on the host.
typedef struct ARROW {
    u_char state;                       /* 0x00: ARROW_STATE_* */
    u_char level;                       /* 0x01: asGetTheArrowLevel's answer; asArrowOperate picks the hit
                                           sound by it (0..1, 2..3, else) and sets 0 / 2 / 5 by kind */
    u_char kind;                        /* 0x02: asShootArrowSetting's kind (BOW_ARROW_*, wideXcmd_DebugPrint.c);
                                           asGetTheArrowRad sets ARROW_KIND_STEEP */
    u_char pad03;
    short timer;                        /* 0x04: frames in the state (0 on entry) */
    u_short shootArg1;                  /* 0x06: asShootArrowSetting's second argument (low byte) */
    u_short shootArg2;                  /* 0x08: its third (low byte) */
    u_short part;                       /* 0x0a: the arrow's own part in ArrowInfo.bank (the script's 1st number) */
    u_short markPart;                   /* 0x0c: the part placed ARROW_MARK_DIST ahead of the bow along dir
                                           (the 2nd number) */
    short hitId;                        /* 0x0e: the id a scene hit's object carries at +0xda (the 3rd number) */
    int counter;                        /* 0x10: cleared with the state; asArrowOperate's */
    int hitKind;                        /* 0x14: asArrowOperate: ARROW_HIT_* when a scene hit took the arrow */
    u_char pad18[8];
    float dir[4];                       /* 0x20: the flight direction, xyz unit, w 1 */
    float pos[4];                       /* 0x30: the arrow's position */
    float posPrev[4];                   /* 0x40: pos at the start of the frame (asArrowOperate) */
} ARROW;
static_assert(sizeof(ARROW) == 0x50, "ARROW");
static_assert(offsetof(ARROW, hitId) == 0x0e, "ARROW.hitId");
static_assert(offsetof(ARROW, dir) == 0x20, "ARROW.dir");
static_assert(offsetof(ARROW, posPrev) == 0x40, "ARROW.posPrev");

enum {
    ARROW_STATE_ASK = 1,                /* asChangeTheArrowState: only ask (never a record's state) */
    ARROW_STATE_READY = 2,              /* asXcmd_I_have_x_Arrows: free for asShootArrowSetting */
    ARROW_STATE_SHOT = 3,               /* asShootArrowSetting: nocked / in flight (asArrowOperate) */
    ARROW_STATE_FLYING = 4,             /* asArrowOperate on MOTION_BOW_DRAW; 3 and 4 are "the arrow out" */
    ARROW_STATE_5 = 5,                  /* asArrowOperate on MOTION_BOW_SHOOT (role unread) */
    ARROW_STATE_HIT = 6,                /* asArrowOperate: a scene hit's object matched hitId */
};
#define ARROW_KIND_STEEP 3              /* asGetTheArrowRad: dir . up >= ARROW_STEEP_DOT */
#define ARROW_HIT_KIND0 1               /* asArrowOperate: hit record kind 0 (the arrow is placed at the plane) */
#define ARROW_HIT_KIND1 2               /* hit record kind 1 */

// ArrowInfo (sbss 0x0193cea0, 0x18 bytes in the symtab). layout: host (list is a pointer; EE offsets).
typedef struct ARROW_INFO {
    u_char active;                      /* EE 0x00: asXcmd_I_have_x_Arrows armed the bank */
    u_char bank;                        /* EE 0x01: the bank whose parts are the arrows */
    u_char count;                       /* EE 0x02: arrows in list (the script's numbers / 3) */
    u_char pad03;
    char bankName[16];                  /* EE 0x04: the bank's name then (asArrowOperate drops the arrows
                                           once the bank holds another file) */
    ARROW *list;                        /* EE 0x14: gHeapJunc, count records */
} ARROW_INFO;
extern ARROW_INFO ArrowInfo;            /* sbss 0x0193cea0 */

#define TRIG_EVENT_ARROW_USED 0x111     /* asUseArrow: global event, an arrow of ArrowList was used */

void asInitArrowInfo(void);                                                              // CT3 0x018666c0
void asDestroyArrowInfo(void);                                                           // CT3 0x018666e0
void asArrowOperate(void);                                                               // CT3 0x01866d80
// asPIA1_KEY_TBL_YUMI (wideXcmd_DebugPrint.c): the out arrow's dir . up (0 with none); at
// ARROW_STEEP_DOT and above the arrow's kind becomes ARROW_KIND_STEEP.
float asGetTheArrowRad(void);                                                            // CT3 0x01866720
// The out arrow's level, -1 with none.
int asGetTheArrowLevel(void);                                                            // CT3 0x01866810
// The index of the first arrow out (state 3 or 4) whose state is not `state`: ARROW_STATE_ASK only asks;
// any other state is set (timer and counter cleared). -1 for none.
int asChangeTheArrowState(int state);                                                    // CT3 0x01866980
// Shoot an arrow of kind `kind` (a ready slot becomes ARROW_STATE_SHOT); its index, -1 with none.
int asShootArrowSetting(u_char kind, u_short a1, u_short a2);                             // CT3 0x01866c10
void asAdjustArrowDirection(ARROW *arrow, int carry);                                    // CT3 0x01866a30
// The arrow whose hitId is `id`, NULL for none.
ARROW *asArrowInformation(short id);                                                     // CT3 0x01866910
// The arrow that scene hit `id` (the hit table's +0x18) of ArrowInfo.bank carries, NULL for none.
ARROW *asAttackedArrowInformation(int id);                                               // CT3 0x01866890
// From tiXcmd_UseItem (tiXcmd_TransItem.c): use the arrow of kind `arrow` (ArrowData index); the list
// shifts down one.
void asUseArrow(int arrow);                                                              // CT3 0x018665e0
