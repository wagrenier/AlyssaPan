// game/ssHitCheckAttackToScn.h -- CT3 game/ssHitCheckAttackToScn.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void ssProcessHit(void);                                                                 // CT3 0x018b1e60
void ssHitGlobalInit(void);                                                              // CT3 0x018b20e0
void ssHitInit(int scene);                                                               // CT3 0x018b2070

// ---- line-of-sight tests (from LosHitFadeUpdateTarget's calls, QuatSlerp.c slice 5, 2026-11-10) ----
// CheckLOS(bank, seg): != 0 when the segment from -> to hits the bank's collision.
typedef struct LOS_SEG {
    float from[4];                      /* 0x00: w 1 */
    float to[4];                        /* 0x10: w 1 */
} LOS_SEG;
static_assert(sizeof(LOS_SEG) == 0x20, "LOS_SEG");
#define LOS_MULTI_MAX 255
// CheckLOS_Multi(bank, q): != 0 on a hit; fills hits[0..count) with the hit records, whose short at
// LOS_HIT_ID the caller compares with its own id list. layout: host (the ROM's: 0x420 bytes on the
// stack, from at 0, to at 0x10, hits at 0x20, count at 0x41c; the hits are pointers).
typedef struct LOS_MULTI {
    float from[4];
    float to[4];
    void *hits[LOS_MULTI_MAX];
    u_int count;
} LOS_MULTI;
#define LOS_HIT_ID 0x198                /* short: the hit record's id (the faded part's hitIds list) */
int CheckLOS_Multi(int bank, LOS_MULTI *q);                                              // CT3 0x018aa950
int CheckLOS(int bank, LOS_SEG *seg);                                                    // CT3 0x018ab180
// What CheckLOS leaves after the segment on a hit, as tiCharaFootAdjust reads it (2026-11-11): the
// caller's buffer runs on past LOS_SEG -- +0x20 the hit point, +0x30 the hit record (an EE word there).
// layout: host (the record is a host pointer here); CheckLOS's TU confirms both and fills them.
typedef struct LOS_HIT {
    LOS_SEG seg;                        /* 0x00 */
    float point[4];                     /* 0x20: where the segment meets the surface */
    void *record;                       /* 0x30: the hit record (LOS_HIT_ID +0x198, LOS_HIT_FLAGS +0x194) */
} LOS_HIT;
#define LOS_HIT_FLAGS 0x194             /* u_int in the hit record */
#define LOS_HIT_FLAG_FLOOR 0x100000u    /* a surface feet stand on (tiCharaFootAdjust's only test) */
// The direction of v in the plane that drops `axis` (1: the XZ plane, from z and x), radians in [0, 2pi);
// a software double on the EE (from ssTrigLookAtCheck's call, 2026-11-11).
double ssRadianByO(const float *v, int axis);                                            // CT3 0x018ad300
