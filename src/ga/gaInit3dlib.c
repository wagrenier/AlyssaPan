// ga/gaInit3dlib.c -- CT3 ga/gaInit3dlib.c (0x018a7700..0x018a85c8): the ga library's 3D helpers, a GCC
// object of VU0 macro code. Reconstructed 2026-11-10 from the disassembly (mipsdis.py, vudis.py for the
// two VU0 micro routines): all 25 symtab functions.
//
// VU0 on the host: the vector registers become float[4] locals; VMULA / VMADDA chains keep their order
// ((x * r0 + y * r1) + z * r2) + w * r3; VDIV / VRSQRT go through VuDiv / VuRsqrt, which answer the VU's
// +-FLT_MAX for a zero divisor (hazards.md: the host's inf would make NaN of a zero vector); other
// infinities and denormals are not clamped. Each function reads all its inputs before it
// stores, as the ROM's loads-then-stores do, so a destination may alias a source.
//
// Two blocks of code in the object have no symbol, no Ghidra function and no caller (dead code, kept here as
// a note only): 0x018a77a8 (dst.xyz = src.xyz / |src.xyz|, w kept) and 0x018a7808 (the same, answering
// |src.xyz|).

#include "ga/gaInit3dlib.h"

#include "alyssapan/rendering/alyssapan_drawga.h"
#include "ga/gaDma.h"
#include "ga/gaSinCos.h"
#include "game/ct3_types.h"
#include "sdk/eekernel.h"

#include <float.h>
#include <math.h>
#include <string.h>

// PORT: the ROM's VU0 microprogram (0x018fe500, MPG code). The host has no VU0, so this is a zeroed stand-in
// of the ROM's size, as vu1_object_draw_program is: gaInit3dlib still sends it. Its two routines are
// gaContMulMatrix's products, done natively below.
alignas(16) u_int vu0_3dlib_program[VU0_3DLIB_PROGRAM_QWC * 4];

// VDIV: a zero divisor answers +-FLT_MAX, the sign the operands' signs XORed.
static float VuDiv(float a, float b)
{
    if (b == 0.0f)
        return (signbit(a) != signbit(b)) ? -FLT_MAX : FLT_MAX;
    return a / b;
}

// VRSQRT 1 / sqrt(|x|): zero answers FLT_MAX.
static float VuRsqrt(float x)
{
    if (x == 0.0f)
        return FLT_MAX;
    return 1.0f / sqrtf(fabsf(x));
}

// v * m (a row vector times the matrix's rows), into out (which may be v).
static void VecMulMatrix(float *out, const float *v, float (*m)[4])
{
    float r[4];
    for (int j = 0; j < 4; j++)
        r[j] = ((v[0] * m[0][j] + v[1] * m[1][j]) + v[2] * m[2][j]) + v[3] * m[3][j];
    memcpy(out, r, sizeof(r));
}

// CT3 0x018a7700
// Loads the VU0 microprogram (the .vutext image between 0x018fe500 and 0x018ff430).
void gaInit3dlib(void)
{
    gaDmaSendN_VIF0(vu0_3dlib_program, VU0_3DLIB_PROGRAM_QWC);
}

// CT3 0x018a7730
// x, y, z copied (one doubleword and one word); dst's w is left alone.
void gaCopyVectorXYZ(float *dst, float *src)
{
    float x = src[0], y = src[1], z = src[2];
    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
}

// CT3 0x018a7748
// dst.xyz = a.xyz / b.xyz; dst.w = a.w (the qword is stored whole).
void gaDivVectorXYZ(float *dst, float *a, float *b)
{
    float r[4] = { VuDiv(a[0], b[0]), VuDiv(a[1], b[1]), VuDiv(a[2], b[2]), a[3] };
    memcpy(dst, r, sizeof(r));
}

// CT3 0x018a7780
// Written early (game/ahRelocateSunPs2ObjectArry.c slice 13): VMUL.xyz, then x + y + z (VADDY.x, VADDZ.x).
float gaInnerProductXYZ(float *a, float *b)
{
    float s = a[0] * b[0];
    s += a[1] * b[1];
    return s + a[2] * b[2];
}

// CT3 0x018a77d0
// dst.xyz = src.xyz scaled to length 1 (VRSQRT), w kept; answers that scale, 1 / |src.xyz|.
float gaNormalizeVector2XYZ(float *dst, float *src)
{
    float len2 = (src[0] * src[0] + src[1] * src[1]) + src[2] * src[2];
    float q = VuRsqrt(len2);
    float r[4] = { src[0] * q, src[1] * q, src[2] * q, src[3] };
    memcpy(dst, r, sizeof(r));
    return q;
}

// CT3 0x018a7848
// dst.xyz = src.xyz clamped to [lo, hi] (VMAX, then VMINI); w kept.
void gaClampVectorXYZ(float *dst, float *src, float lo, float hi)
{
    float r[4];
    for (int i = 0; i < 3; i++) {
        float v = src[i] > lo ? src[i] : lo;
        r[i] = v < hi ? v : hi;
    }
    r[3] = src[3];
    memcpy(dst, r, sizeof(r));
}

// CT3 0x018a7870
// dst.xyz = a + (b - a) * t (ACC = a, then VMADDX); w = a.w.
void gaInterVectorXYZ(float *dst, float *a, float *b, float t)
{
    float r[4];
    for (int i = 0; i < 3; i++)
        r[i] = a[i] + (b[i] - a[i]) * t;
    r[3] = a[3];
    memcpy(dst, r, sizeof(r));
}

// CT3 0x018a7898
// dst = a / b per lane -- except w: the ROM divides vf1z, which already holds a.z / b.z, by b.w (VDIV Q,
// vf1z, vf2w), so w = (a.z / b.z) / b.w. Kept (no caller in the ROM to say which was meant).
void gaDivVector(float *dst, float *a, float *b)
{
    float r[4];
    r[0] = VuDiv(a[0], b[0]);
    r[1] = VuDiv(a[1], b[1]);
    r[2] = VuDiv(a[2], b[2]);
    r[3] = VuDiv(r[2], b[3]);
    memcpy(dst, r, sizeof(r));
}

// CT3 0x018a78d8
// dst = src clamped to [lo, hi] on all four lanes.
void gaClampVector(float *dst, float *src, float lo, float hi)
{
    float r[4];
    for (int i = 0; i < 4; i++) {
        float v = src[i] > lo ? src[i] : lo;
        r[i] = v < hi ? v : hi;
    }
    memcpy(dst, r, sizeof(r));
}

// CT3 0x018a7900
// dst = b * a: each row of b times a (a's rows in vf4..vf7, b's rows run through them).
void gaMulMatrix(float (*dst)[4], float (*a)[4], float (*b)[4])
{
    float r[4][4];
    for (int i = 0; i < 4; i++)
        VecMulMatrix(r[i], b[i], a);
    memcpy(dst, r, sizeof(r));
}

// CT3 0x018a7978
// gaMulMatrix over a list of records, through the VU0 micro routines 0x000 / 0x010 (vcallms), software-
// pipelined: record i + 1's matrices are loaded before record i's product is stored, so a record whose
// a or b is the previous record's dst reads the old matrix -- kept as the ROM does it. count must be >= 1
// (0 runs on in the ROM).
void gaContMulMatrix(GA_MUL_MATRIX_REC *recs, int count)
{
    float a[4][4], b[4][4], r[4][4];

    memcpy(a, recs[0].a, sizeof(a));
    memcpy(b, recs[0].b, sizeof(b));
    for (int i = 0;; i++) {
        for (int k = 0; k < 4; k++)
            VecMulMatrix(r[k], b[k], a);
        if (--count == 0) {
            memcpy(recs[i].dst, r, sizeof(r));
            return;
        }
        memcpy(a, recs[i + 1].a, sizeof(a));
        memcpy(b, recs[i + 1].b, sizeof(b));
        memcpy(recs[i].dst, r, sizeof(r));
    }
}

// CT3 0x018a7a80
// All 16 words read, then stored transposed.
void gaTransposeMatrix(float (*dst)[4], float (*src)[4])
{
    float r[4][4];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r[j][i] = src[i][j];
    memcpy(dst, r, sizeof(r));
}

// CT3 0x018a7b50
// The rigid inverse: the 3x3 transposed by MMI (PEXTLW / PEXTUW / PCPYLD / PCPYUD, each new row's w = 0),
// the new translation -(t * R^T) with t = a's row 3, whose w is kept.
void gaInversMatrix(float (*dst)[4], float (*a)[4])
{
    float r[4][4];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            r[i][j] = a[j][i];
        r[i][3] = 0.0f;
    }
    for (int j = 0; j < 3; j++)
        r[3][j] = 0.0f - ((r[0][j] * a[3][0] + r[1][j] * a[3][1]) + r[2][j] * a[3][2]);
    r[3][3] = a[3][3];
    memcpy(dst, r, sizeof(r));
}

// CT3 0x018a7bc0
// Z first: dst = Z(angles[2]), then Y(angles[1]), then X(angles[0]) applied on the right.
void gaSetRotMatrixXYZ(float (*dst)[4], int *angles)
{
    gaSetRotMatrixZ(dst, angles[2]);
    gaRotMatrixY(dst, angles[1], dst);
    gaRotMatrixX(dst, angles[0], dst);
}

// CT3 0x018a7c10
// X first, from angles[2] -- the ROM's: the Z-Y-X order uses the X rotation for the last angle -- then
// Y(angles[1]), then Z(angles[0]).
void gaSetRotMatrixZYX(float (*dst)[4], int *angles)
{
    gaSetRotMatrixX(dst, angles[2]);
    gaRotMatrixY(dst, angles[1], dst);
    gaRotMatrixZ(dst, angles[0], dst);
}

// CT3 0x018a7c60
// dst = src * X(angle).
void gaRotMatrixX(float (*dst)[4], int angle, float (*src)[4])
{
    float rot[4][4];
    gaSetRotMatrixX(rot, angle);
    gaMulMatrix(dst, rot, src);
}

// CT3 0x018a7ca8
// dst = src * Y(angle).
void gaRotMatrixY(float (*dst)[4], int angle, float (*src)[4])
{
    float rot[4][4];
    gaSetRotMatrixY(rot, angle);
    gaMulMatrix(dst, rot, src);
}

// CT3 0x018a7cf0
// dst = src * Z(angle).
void gaRotMatrixZ(float (*dst)[4], int angle, float (*src)[4])
{
    float rot[4][4];
    gaSetRotMatrixZ(rot, angle);
    gaMulMatrix(dst, rot, src);
}

// CT3 0x018a7d38
// dst = src * XYZ(angles), the rotation built in dst itself.
void gaRotMatrixXYZ(float (*dst)[4], int *angles, float (*src)[4])
{
    gaSetRotMatrixXYZ(dst, angles);
    gaMulMatrix(dst, dst, src);
}

// CT3 0x018a7d78
// dst = src * ZYX(angles), the rotation built in dst itself.
void gaRotMatrixZYX(float (*dst)[4], int *angles, float (*src)[4])
{
    gaSetRotMatrixZYX(dst, angles);
    gaMulMatrix(dst, dst, src);
}

// The secant and tangent of a side plane whose offset over the projection distance is t.
static void SidePlane(GA_3DLIB_WORK *w, int plane, float t, float sign, float proj)
{
    w->planeSec[plane] = sqrtf(t * t + proj * proj) / proj;
    w->planeTan[plane] = (sign < 0.0f ? -t : t) / proj;
}

// CT3 0x018a7db8
// Fills the 3dlib work from the screen, the draw buffer's Z format and the bank's fog. The ROM's sqrt.s is
// followed by a sqrtf call when it answers NaN; sqrtf alone gives the same.
void gaSet3dlibStruct(float clipZ, void *work, GA_DBUFFER_DATA *drawbuffer, GA_FOGCTRL_DATA *fogctrl,
                      float *screen)
{
    GA_3DLIB_WORK *w = (GA_3DLIB_WORK *)work;
    float proj;

    w->zero10 = 0;
    switch ((drawbuffer->zbuf.data >> GA_ZBUF_PSM_SHIFT) & GA_ZBUF_PSM_MASK) {
    case GA_ZBUF_PSM_Z32:
        w->zMax = GA_ZMAX_Z32;
        break;
    case GA_ZBUF_PSM_Z24:
        w->zMax = GA_ZMAX_Z24;
        break;
    default:
        w->zMax = GA_ZMAX_Z16;
        break;
    }

    w->screenW = screen[GA_SCREEN_W];
    w->screenH = screen[GA_SCREEN_H];
    w->scaleX = screen[GA_SCREEN_SCALE_X];
    w->scaleY = screen[GA_SCREEN_SCALE_Y];
    w->fog[0] = fogctrl->fog[0];
    w->fog[1] = fogctrl->fog[1];
    w->fog[2] = fogctrl->fog[2];
    w->proj = screen[GA_SCREEN_PROJ];
    w->zNear = screen[GA_SCREEN_NEAR];
    w->zFar = screen[GA_SCREEN_FAR];

    proj = screen[GA_SCREEN_PROJ];
    // callcheck: sqrtf (SidePlane makes it, once per plane)
    // PORT: the left and right planes reach AlyssaPan_GaWiden() times as far, so objects in a wide
    // window's side bands are not culled (the ROM's spans at 4:3).
    SidePlane(w, GA_PLANE_LEFT, screen[GA_SCREEN_CX] / screen[GA_SCREEN_SCALE_X] * AlyssaPan_GaWiden(), -1.0f,
              proj);
    SidePlane(w, GA_PLANE_RIGHT,
              (screen[GA_SCREEN_W] - screen[GA_SCREEN_CX]) / screen[GA_SCREEN_SCALE_X] * AlyssaPan_GaWiden(), 1.0f,
              proj);
    SidePlane(w, GA_PLANE_TOP, (screen[GA_SCREEN_H] - screen[GA_SCREEN_CY]) / screen[GA_SCREEN_SCALE_Y],
              -1.0f, proj);
    SidePlane(w, GA_PLANE_BOTTOM, screen[GA_SCREEN_CY] / screen[GA_SCREEN_SCALE_Y], 1.0f, proj);

    w->zClipScale = w->zNear * (w->zMax - clipZ);
    w->scaleX16 = screen[GA_SCREEN_SCALE_X] * GA_SUBPIXEL;
    w->scaleYNeg16 = -screen[GA_SCREEN_SCALE_Y] * GA_SUBPIXEL;
    w->zFogScale = screen[GA_SCREEN_NEAR] * (screen[GA_SCREEN_11] - clipZ) / screen[GA_SCREEN_PROJ];
    w->fog0Sq = fogctrl->fog[0] * fogctrl->fog[0];
    w->fog1Fog0Sq16 = fogctrl->fog[1] * w->fog0Sq * GA_SUBPIXEL;
    w->gsOffsetX = (screen[GA_SCREEN_OFS_X] + GA_GS_OFFSET_CENTRE + screen[GA_SCREEN_CX]) * GA_SUBPIXEL;
    w->gsOffsetY = (screen[GA_SCREEN_OFS_Y] + GA_GS_OFFSET_CENTRE + screen[GA_SCREEN_CY]) * GA_SUBPIXEL;
    w->proj70 = screen[GA_SCREEN_PROJ];
}

// CT3 0x018a8140
// The ROM's slip: it copies dst's 0x80 bytes and src's to its stack, then stores the src copy over the dst
// copy -- a0 was moved to the stack buffer by the first loop -- so nothing outside the function changes.
// Kept as a no-op (no ROM caller); gaCopyObject2 is the working copy.
void gaCopyObject(void *dst, void *src)
{
    u_char dstCopy[GA_OBJECT_SIZE], srcCopy[GA_OBJECT_SIZE];

    memcpy(dstCopy, dst, GA_OBJECT_SIZE);
    memcpy(srcCopy, src, GA_OBJECT_SIZE);
    memcpy(dstCopy, srcCopy, GA_OBJECT_SIZE);
}

// CT3 0x018a8210
// Written early (game/ahRelocateSunPs2ObjectArry.c slice 13): the 32 words of a ga draw object copied
// (the ROM loads all 32, then stores them).
void gaCopyObject2(void *dst, const void *src)
{
    memmove(dst, src, GA_OBJECT_SIZE);
}

// CT3 0x018a8258
// The four directional lights go through the view's rotation only (a copy of view with row 3's xyz
// cleared, its w kept) into dirView's columns, w negated; the four point lights' xyz go through the whole
// view into omniView's columns, their w copied. The point lights reuse one qword: gaCopyVectorXYZ replaces
// its xyz, so each product's w carries the previous product's w (0 for a view whose w column is 0, 0, 0, 1)
// -- kept.
void gaSetLight(void *lightWork, float (*view)[4])
{
    GA_LIGHT_WORK *lw = (GA_LIGHT_WORK *)lightWork;
    float rot[4][4];
    float v[4];

    memcpy(rot, view, sizeof(rot));
    rot[3][0] = 0.0f;
    rot[3][1] = 0.0f;
    rot[3][2] = 0.0f;
    for (int i = 0; i < LIGHT_SLOTS; i++) {
        VecMulMatrix(v, lw->dirPos[i], rot);
        lw->dirView[0][i] = v[0];
        lw->dirView[1][i] = v[1];
        lw->dirView[2][i] = v[2];
        lw->dirView[3][i] = -v[3];
    }

    memset(v, 0, sizeof(v));
    for (int i = 0; i < LIGHT_SLOTS; i++) {
        gaCopyVectorXYZ(v, lw->omniPos[i]);
        VecMulMatrix(v, v, view);
        lw->omniView[0][i] = v[0];
        lw->omniView[1][i] = v[1];
        lw->omniView[2][i] = v[2];
        lw->omniView[3][i] = lw->omniPos[i][3];
    }
}

// CT3 0x018a8548
// The screen's roll: a 2x2 rotation by roll, scaled by the 3dlib work's x / y scales, into the scratchpad
// (0x70000300). The work is the one GA_SPR_WORK.work3dlibEE names (0x70000400, main's per-frame copy).
void gaSetZRollMatrix(int roll)
{
    const GA_SPR_WORK *spr = (const GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET);
    const GA_3DLIB_WORK *w;
    float *m = (float *)SCRATCHPAD(GA_SPR_ZROLL_MATRIX);
    float s, c, sx, sy;

    // PORT: the ROM reads the work wherever the word points; on the host only a scratchpad address maps.
    if (spr->work3dlibEE < SCRATCHPAD_EE_BASE ||
        spr->work3dlibEE - SCRATCHPAD_EE_BASE + sizeof(GA_3DLIB_WORK) > SCRATCHPAD_SIZE)
        return;
    w = (const GA_3DLIB_WORK *)SCRATCHPAD(spr->work3dlibEE - SCRATCHPAD_EE_BASE);
    sy = w->scaleY;
    sx = w->scaleX;
    gaSinCos(&s, &c, roll);
    m[5] = c * sy;
    m[1] = s * sy;
    m[0] = c * sx;
    m[4] = -s * sx;
}
