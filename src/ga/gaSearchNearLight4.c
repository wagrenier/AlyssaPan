// ga/gaSearchNearLight4.c -- Clock Tower 3 module ga/gaSearchNearLight4.c (0x018a7538..0x018a7700).
// One hand-written function (MMI and VU0 macro mode, the micro routines 0x048 / 0x054; s0..s7 saved in the
// scratchpad at 0x70000240..0x700002bf, which the host does not need), read from the disassembly
// (mipsdis.py 018a7538 018a7700) and the micro (vudis.py 18fe500 f3 48 54 64), 2026-11-12.
//
// The ROM: eight positions a pass (lqc2 vf8..vf15), transposed by micro 0x048 into x / y / z rows; then,
// one light a call (vf1, pipelined: light j + 1 is loaded while light j's answer is read back), micro
// 0x054 computes |p - light.xyz|^2 * light.w^2 for the eight positions (vf16 / vf17), or 1.001 for every
// position when ftoi0(light.w^2)'s low 16 bits are not zero (ibne vi4 to 0x064, loi 1.001). The EE keeps,
// per position, the four smallest keys in a min / max network (pminw / pmaxw, signed words), a key being
// the value's float bits with the low byte replaced by the light's index; the slots start at 0x3f8000ff
// (1.0 with index 0xff), so a light counts only when its value is below 1.0 (at 24 bits' precision; the
// index breaks ties). The answer is the four slots' low bytes, nearest first, four bytes per position.

#include "ga/gaSearchNearLight4.h"

#include <string.h>

#define NEAR_LIGHT_BLOCK 8                  // positions a pass (vf8..vf15)
#define NEAR_LIGHT_KEY_MASK 0xffffff00u     // the value's bits a key keeps (s0's mask)
#define NEAR_LIGHT_KEY_START 0x3f8000ff     // each slot's start: 1.0f, index NEAR_LIGHT_NONE (s1)
#define NEAR_LIGHT_FAR 1.001f               // micro 0x064's loi: the value of a light it rejects
#define NEAR_LIGHT_W_MASK 0xffff            // mtir takes ftoi0(w^2)'s low 16 bits (vi4)

// VU0 micro 0x054 for one position: the squared distance (mula / madda / madd: x, then y, then z) times
// the squared w, or NEAR_LIGHT_FAR when the light is rejected.
static float NearLightValue(const float pos[4], const float light[4], int rejected)
{
    float dx = pos[0] - light[0];
    float dy = pos[1] - light[1];
    float dz = pos[2] - light[2];
    float d2;

    if (rejected)
        return NEAR_LIGHT_FAR;
    d2 = dx * dx;
    d2 = d2 + dy * dy;
    d2 = d2 + dz * dz;
    return d2 * (light[3] * light[3]);
}

// ftoi0(w^2) & 0xffff != 0 (micro 0x058 / 0x05c / 0x060): any w with w^2 >= 1, unless the truncated
// square is a multiple of 65536; ftoi0 saturates at 0x7fffffff.
static int NearLightRejected(float w)
{
    float w2 = w * w;
    int i = (w2 >= 2147483648.0f) ? 0x7fffffff : (int)w2;

    return (i & NEAR_LIGHT_W_MASK) != 0;
}

// CT3 0x018a7538
// For each of posCount positions, the indices of the (up to) four lights nearest to it by
// |p - light|^2 * light.w^2 (w is the light's inverse range: only a value below 1.0 counts), nearest
// first, one byte each (NEAR_LIGHT_NONE: no light) at nearest[p * 4 + rank]. Positions go eight a pass and
// every pass writes 32 bytes, so nearest[] holds posCount rounded up to 8, times 4 bytes.
void gaSearchNearLight4(u_char *nearest, float (*pos)[4], int posCount, float (*lights)[4], int lightCount)
{
    int blocks = ((posCount + 7) & 0xfff8) / NEAR_LIGHT_BLOCK;
    int b;

    for (b = 0; b < blocks; b++) {
        int slot[NEAR_LIGHT_BLOCK][4];
        int p, j;

        for (p = 0; p < NEAR_LIGHT_BLOCK; p++)
            for (j = 0; j < 4; j++)
                slot[p][j] = NEAR_LIGHT_KEY_START;

        // PORT: the ROM's light loop is a do-while (lightCount 0 would run it about 2^28 times over memory
        // past the list) and its pipeline loads one light past the end; the host stops at the count.
        for (j = 0; j < lightCount; j++) {
            int rejected = NearLightRejected(lights[j][3]);

            for (p = 0; p < NEAR_LIGHT_BLOCK; p++) {
                int *s = slot[p];
                float v;
                u_int bits;
                int t, x, a, c;

                // PORT: the ROM reads all eight positions of a pass, past the caller's posCount (the
                // callers pass one); the host leaves the padding positions at NEAR_LIGHT_NONE.
                if (b * NEAR_LIGHT_BLOCK + p >= posCount)
                    break;
                v = NearLightValue(pos[b * NEAR_LIGHT_BLOCK + p], lights[j], rejected);
                memcpy(&bits, &v, sizeof(bits));
                t = (int)((bits & NEAR_LIGHT_KEY_MASK) | (u_int)j); // por with the index word (s7)

                // the ROM's network: the four smallest of the slots and t, sorted s[0] <= .. <= s[3]
                x = (s[3] < t) ? s[3] : t;
                a = (x < s[2]) ? x : s[2];
                s[3] = (x < s[2]) ? s[2] : x;
                c = (a < s[1]) ? a : s[1];
                s[2] = (a < s[1]) ? s[1] : a;
                x = (c < s[0]) ? c : s[0];
                s[1] = (c < s[0]) ? s[0] : c;
                s[0] = x;
            }
        }

        for (p = 0; p < NEAR_LIGHT_BLOCK; p++)
            for (j = 0; j < 4; j++)
                nearest[b * NEAR_LIGHT_BLOCK * 4 + p * 4 + j] = (u_char)(slot[p][j] & 0xff);
    }
}
