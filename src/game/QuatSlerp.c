// game/QuatSlerp.c -- Clock Tower 3 module 0x01851bf0..0x01851f00: QuatSlerp, ahCos, ahSin, ahAcos.
// No local symbol ties these four to a neighbour: the cut before BRANCH_CLEAR follows the names (math here,
// game state after). Code moved unchanged from the TU map's QuatSlerp.c (slices 1-2, 2026-11-10) by the
// split of 2026-11-11 (architecture.md "The TU map").

#include "game/QuatSlerp.h"
#include "sdk/scetypes.h"
#include "ga/gaSinCos.h"

#include <math.h>
#include <string.h>

// ---- angles (slice 1, 2026-11-10; 0x10000 = a full turn) ----

// CT3 0x0190a600 (ah_acos_table): acos(i / 4096) for i = 0..4097, the ROM's floats.
static const u_int ah_acos_table_bits[AH_ACOS_TABLE_SIZE] = {
#include "game/ah_acos_table.inc"
};

static float ahAcosTable(int i)
{
    float f;
    memcpy(&f, &ah_acos_table_bits[i], sizeof(f));
    return f;
}

// CT3 0x01851dc0
// A negative cosine answers 0x8000 - acos(|c|). 0 is 0x4000, |c| >= 1 is 0 (or 0x8000); from
// AH_ACOS_NEAR_ONE the table is too coarse and acosf is called (on the signed value, masked to 16
// bits -- the ROM returns that directly, without the 0x8000 - step); below it the table is
// interpolated linearly at c * 4096.
int ahAcos(float c)
{
    int sign = 1, base = 0, angle, i;
    float x;

    if (c < 0.0f) {
        sign = -1;
        base = ANGLE_HALF_TURN;
        c = -c;
    }
    if (c == 0.0f)
        return ANGLE_QUARTER_TURN;
    if (c >= 1.0f) {
        angle = 0;
    } else if (c >= AH_ACOS_NEAR_ONE) {
        return (int)(ANGLE_FULL_TURN_F * acosf(c * (float)sign) / AH_TWO_PI) & 0xffff;
    } else {
        x = c * AH_ACOS_STEPS;
        i = (int)x;
        angle = (int)(ahAcosTable(i) + (x - (float)i) * (ahAcosTable(i + 1) - ahAcosTable(i)));
    }
    return base + sign * angle;
}

// CT3 0x01851d60: gaSinCos's second answer.
float ahCos(int angle)
{
    float s, c = 0.0f;
    gaSinCos(&s, &c, angle);
    return c;
}

// CT3 0x01851d90: gaSinCos's first answer.
float ahSin(int angle)
{
    float s = 0.0f, c;
    gaSinCos(&s, &c, angle);
    return s;
}

// VU0's 4-lane dot as the ROM sums it (ACC: w + z + y + x).
static float QuatDot(const float *a, const float *b)
{
    return a[3] * b[3] + a[2] * b[2] + a[1] * b[1] + a[0] * b[0];
}

// CT3 0x01851bf0: spherical blend of two unit quaternions, ts = {weight of q0, weight of q1} (1 - t,
// t). q0 is negated while the dot is negative (the short way round); nearly equal quaternions
// (dot >= 0.9999998) answer q1. ts[3] is overwritten with the angle as a float (the ROM's scratch).
void QuatSlerp(float *q, float *q0, float *q1, float *ts)
{
    float a[4];
    float dot, inv, s0, s1, len;
    int angle, i;

    for (i = 0; i < 4; i++)
        a[i] = q0[i];
    for (;;) {
        dot = QuatDot(a, q1);
        if (!(dot < 0.0f))
            break;
        for (i = 0; i < 4; i++)
            a[i] *= -1.0f;
    }
    if (!(dot < 0.9999998f)) {
        for (i = 0; i < 4; i++)
            q[i] = q1[i];
        return;
    }
    angle = ahAcos(dot);
    inv = 1.0f / ahSin(angle);
    ts[3] = (float)angle;
    // vmulw.xyz + vftoi0: each weight times the angle, truncated
    s0 = ahSin((int)(ts[0] * ts[3])) * inv;
    s1 = ahSin((int)(ts[1] * ts[3])) * inv;
    for (i = 0; i < 4; i++)
        q[i] = a[i] * s0 + q1[i] * s1;
    // VU0 RSQRT of the w + z + y + x sum of squares, all four lanes
    len = 1.0f / sqrtf(QuatDot(q, q));
    for (i = 0; i < 4; i++)
        q[i] *= len;
}
