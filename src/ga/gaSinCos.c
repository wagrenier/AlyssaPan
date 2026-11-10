// ga/gaSinCos.c -- CT3 ga/gaSinCos.c (0x018a6b70..0x018a7538): the ga library's hand-written assembly
// helpers (a .text section symbol starts the object; its .data is ga_sincos_table). Reconstructed
// 2026-11-10 from the disassembly (mipsdis.py) and, for gaProjectScreen, the VU0 micro routines 0x020 /
// 0x028 / 0x038 (vudis.py 18fe500 f3 20 28 38): all 13 symtab functions.
//
// VU0 on the host as in gaInit3dlib.c: vector registers become float[4] locals, VMULA / VMADDA chains keep
// their order ((x * r0 + y * r1) + z * r2) + w * r3, DIV goes through VuDiv (a zero divisor answers the
// VU's +-FLT_MAX). Every function reads a vector's inputs before it stores, as the ROM does, so a
// destination may alias its source.

#include "ga/gaSinCos.h"

#include "game/ct3_types.h"
#include "sdk/eekernel.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

// CT3 0x0190e640 (ga_sincos_table): {sin, cos} of i * 2pi / 0x4000 for i = 0..0x3fff, the ROM's floats.
const u_int ga_sincos_table[GA_SINCOS_TABLE_SIZE * 2] = {
#include "ga/ga_sincos_table.inc"
};

// The table entry of an angle: (angle + 2) & 0xfffc, in 4-unit steps -- the nearest entry, wrapped.
#define SINCOS_INDEX(angle) ((((u_int)(angle) + 2u) & 0xfffcu) >> 2)

static float SinCosWord(u_int word)
{
    float f;
    memcpy(&f, &word, sizeof(f));
    return f;
}

// The entry as the ROM's words: s = sin, c = cos, ns = sin with its sign bit flipped (XOR 0x80000000).
static void SinCosEntry(int angle, float *s, float *c, float *ns)
{
    u_int i = SINCOS_INDEX(angle);
    *s = SinCosWord(ga_sincos_table[i * 2]);
    *c = SinCosWord(ga_sincos_table[i * 2 + 1]);
    *ns = SinCosWord(ga_sincos_table[i * 2] ^ 0x80000000u);
}

// VDIV: a zero divisor answers +-FLT_MAX, the sign the operands' signs XORed.
static float VuDiv(float a, float b)
{
    if (b == 0.0f)
        return (signbit(a) != signbit(b)) ? -FLT_MAX : FLT_MAX;
    return a / b;
}

// The identity rows, as the ROM builds them from vf0 with VMR32 (vf7 / vf6 / vf5 / vf0).
static void SetIdentity(float (*m)[4])
{
    static const float identity[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    memcpy(m, identity, sizeof(identity));
}

// CT3 0x018a6b70
void gaSinCos(float *s, float *c, int angle)
{
    u_int i = SINCOS_INDEX(angle);
    *s = SinCosWord(ga_sincos_table[i * 2]);
    *c = SinCosWord(ga_sincos_table[i * 2 + 1]);
}

// CT3 0x018a6ba0
// Rows (1 0 0 0), (0 c s 0), (0 -s c 0), (0 0 0 1); entry 0 branches to the plain identity (so no -0.0).
void gaSetRotMatrixX(float (*m)[4], int angle)
{
    float s, c, ns;
    SetIdentity(m);
    if (SINCOS_INDEX(angle) == 0)
        return;
    SinCosEntry(angle, &s, &c, &ns);
    m[1][1] = c;
    m[1][2] = s;
    m[2][1] = ns;
    m[2][2] = c;
}

// CT3 0x018a6c10
// Rows (c 0 s 0), (0 1 0 0), (-s 0 c 0), (0 0 0 1).
void gaSetRotMatrixY(float (*m)[4], int angle)
{
    float s, c, ns;
    SetIdentity(m);
    if (SINCOS_INDEX(angle) == 0)
        return;
    SinCosEntry(angle, &s, &c, &ns);
    m[0][0] = c;
    m[0][2] = s;
    m[2][0] = ns;
    m[2][2] = c;
}

// CT3 0x018a6c70
// Rows (c s 0 0), (-s c 0 0), (0 0 1 0), (0 0 0 1).
void gaSetRotMatrixZ(float (*m)[4], int angle)
{
    float s, c, ns;
    SetIdentity(m);
    if (SINCOS_INDEX(angle) == 0)
        return;
    SinCosEntry(angle, &s, &c, &ns);
    m[0][0] = c;
    m[0][1] = s;
    m[1][0] = ns;
    m[1][1] = c;
}

// CT3 0x018a6cd0
// VFTOI0: toward zero; out of range saturates by sign (the VU has no NaN: its bit pattern is a huge value).
int gaInt(float f)
{
    if (isnan(f))
        return signbit(f) ? INT_MIN : INT_MAX;
    if (f >= 2147483648.0f)
        return INT_MAX;
    if (f <= -2147483648.0f)
        return INT_MIN;
    return (int)f;
}

// CT3 0x018a6ce8
// VITOF0.
float gaFloat(int i)
{
    return (float)i;
}

// CT3 0x018a6d00
// Eight words a pass, then the rest one by one; when dst > src (a signed compare) the same from the end.
// Every pass loads its words before it stores them, so any overlap copies as memmove does.
void gaMemCopy32(void *dst, const void *src, u_int count)
{
    memmove(dst, src, (size_t)count * 4);
}

// CT3 0x018a6e28
// gaMemCopy32 with dwords (LD / SD).
void gaMemCopy64(void *dst, const void *src, u_int count)
{
    memmove(dst, src, (size_t)count * 8);
}

// CT3 0x018a6f50
// gaMemCopy32 with qwords (LQ / SQ).
void gaMemCopy128(void *dst, const void *src, u_int count)
{
    memmove(dst, src, (size_t)count * 16);
}

// CT3 0x018a7078
// Bytes up to a 16-byte boundary, then 32 bytes a pass (the byte splatted into a qword by MMI, two SQs),
// then the tail bytes.
void gaMemFill8(void *dst, u_char value, u_int count)
{
    memset(dst, value, count);
}

// CT3 0x018a7110
// The EE part loads m into vf4..vf7 and the scratchpad's GA_SPR_WORK +0x60 / +0x70 / +0x80 into vf16 /
// vf17 / vf18, prescales vf16.xy by 1/16 and makes vf17.xyz = (vf17.xy / 16 - 2048, vf17.z / 16), then
// runs the micro routines two points a call, software-pipelined (each loads the next two inputs before
// it stores two outputs). Per point (micro 0x020 / 0x028 / 0x038):
//   p = in * m (w taken as 1), Q = vf18.w / p.z,
//   out = (vf17.x + vf16.x * p.x * Q, vf17.y + vf16.y * p.y * Q, vf17.z + vf16.z * Q, 1).
// The end test is the input pointer reaching in + ((count + 1) & 0xfffe): an odd count writes one point
// more, and a count of 0 never ends in the ROM (the host stops).
void gaProjectScreen(float (*out)[4], float (*m)[4], float (*in)[4], int count)
{
    const float *k16 = (const float *)SCRATCHPAD(0x60);
    const float *k17 = (const float *)SCRATCHPAD(0x70);
    const float *k18 = (const float *)SCRATCHPAD(0x80);
    const float sixteenth = 0.0625f;                // 0x3d800000
    const float sx = k16[0] * sixteenth, sy = k16[1] * sixteenth, sz = k16[2];
    const float ox = -2048.0f + k17[0] * sixteenth; // ACC.xy = 0 - 2048 (0x45000000), then + vf17 / 16
    const float oy = -2048.0f + k17[1] * sixteenth;
    const float oz = 0.0f + k17[2] * sixteenth;
    int n = (int)(((u_int)count + 1u) & 0xfffeu);

    for (int i = 0; i < n; i++) {
        float x = in[i][0], y = in[i][1], z = in[i][2];
        float p[3];
        for (int j = 0; j < 3; j++)
            p[j] = ((x * m[0][j] + y * m[1][j]) + z * m[2][j]) + m[3][j];
        float q = VuDiv(k18[3], p[2]);
        float r[4];
        r[0] = ox + (sx * p[0]) * q;
        r[1] = oy + (sy * p[1]) * q;
        r[2] = oz + sz * q;
        r[3] = 1.0f;
        memcpy(out[i], r, sizeof(r));
    }
}

// CT3 0x018a71c0
// Lights are taken in order until an index of NEAR_LIGHT_NONE (0xff); with k of them (0..4):
//   omniColor[j] = color[index[j]] for j < k (the rest untouched);
//   omniView = the transposed light positions: column j = (pos[index[j]].xyz * m, pos[index[j]].w),
//     where pos.xyz * m takes w as 1. Columns j >= k keep what the ROM's registers held there: rows 0..2
//     m's rows 0..2 (m[r][j]), row 3 1.0 -- except k = 0, which writes only row 3 = (1, 1, 1, 1).
void gaSetPointLight(struct GA_LIGHT_WORK *lw, float (*m)[4], float (*pos)[4], float (*color)[4],
                     const u_char *index)
{
    int k = 0;
    while (k < LIGHT_SLOTS && index[k] != NEAR_LIGHT_NONE)
        k++;

    if (k == 0) {
        for (int j = 0; j < LIGHT_SLOTS; j++)
            lw->omniView[3][j] = 1.0f;
        return;
    }

    float view[4][LIGHT_SLOTS];
    float col[LIGHT_SLOTS][4];
    for (int r = 0; r < 3; r++)
        for (int j = 0; j < LIGHT_SLOTS; j++)
            view[r][j] = m[r][j];
    for (int j = 0; j < LIGHT_SLOTS; j++)
        view[3][j] = 1.0f;
    for (int j = 0; j < k; j++) {
        const float *p = pos[index[j]];
        for (int r = 0; r < 3; r++)
            view[r][j] = ((p[0] * m[0][r] + p[1] * m[1][r]) + p[2] * m[2][r]) + m[3][r];
        view[3][j] = p[3];
        memcpy(col[j], color[index[j]], sizeof(col[j]));
    }
    for (int j = 0; j < k; j++)
        memcpy(lw->omniColor[j], col[j], sizeof(col[j]));
    memcpy(lw->omniView, view, sizeof(view));
}

// CT3 0x018a74ac
// Software-pipelined over count vectors (the next input loaded before the current output is stored).
void gaContApplyMatrix(float (*out)[4], float (*m)[4], float (*in)[4], int count)
{
    for (int i = 0; i < count; i++) {
        float x = in[i][0], y = in[i][1], z = in[i][2], w = in[i][3];
        float r[4];
        for (int j = 0; j < 4; j++)
            r[j] = ((x * m[0][j] + y * m[1][j]) + z * m[2][j]) + w * m[3][j];
        memcpy(out[i], r, sizeof(r));
    }
}
