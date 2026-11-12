// game/QuatSlerp.h -- CT3 game/QuatSlerp.c: the quaternion slerp and the angle helpers (0x01851bf0..0x01851f00).
// Split from the TU map's QuatSlerp.c on 2026-11-11 (architecture.md "The TU map"): the rest of that range is
// game/BRANCH_CLEAR.c.
#pragma once

// The game's angles are u_short turns (asSearchObject's cone and step); from asSearchObject's calls.
#define ANGLE_QUARTER_TURN 0x4000
#define ANGLE_HALF_TURN 0x8000
#define ANGLE_FULL_TURN_F 65536.0f
#define AH_TWO_PI 6.2831855f            /* 0x40c90fdb */
#define AH_ACOS_STEPS 4096.0f           /* ah_acos_table's steps over 0..1 */
#define AH_ACOS_TABLE_SIZE 4098
#define AH_ACOS_NEAR_ONE 0.9988f        /* 0x3f7fb15b: acosf from here on */
float ahCos(int angle);                                                                  // CT3 0x01851d60
float ahSin(int angle);                                                                  // CT3 0x01851d90
int ahAcos(float c);                                                                     // CT3 0x01851dc0
// q = the slerp from q0 to q1; ts = {1 - t, t} (asMakeStraw passes the pair it also gives
// asGetBeziePosition).
void QuatSlerp(float *q, float *q0, float *q1, float *ts);                               // CT3 0x01851bf0
