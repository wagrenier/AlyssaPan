// ga/gaInit3dlib.h -- CT3 ga/gaInit3dlib.c (0x018a7700..0x018a85c8, a GCC object of the ga library): the
// 3D library -- VU0 vector and matrix helpers, the 3dlib work (gaSet3dlibStruct), the light matrices
// (gaSetLight) and the Z-roll matrix in the scratchpad. Written 2026-11-10 from the disassembly (the
// functions are VU0 macro code; the decompiler cannot follow them).
//
// Conventions read from the code: a vector is float[4] (a qword), a matrix float[4][4] of rows, and a vector
// is a row multiplied on the left: v * M = v.x * M[0] + v.y * M[1] + v.z * M[2] + v.w * M[3] (VMULAX /
// VMADDAY / VMADDAZ / VMADDW over the rows). gaMulMatrix(dst, a, b) therefore leaves dst = b * a: each row of
// b times a -- "apply b's transform, then a's".
#pragma once

#include "sdk/scetypes.h"
#include "ga/gaInitDrawObject.h"

#include <stddef.h>

// ---- the screen and the 3dlib work -----------------------------------------------------------------
// screen_0's twelve floats (main.c passes them to gaSet3dlibStruct). The names come from this TU's use:
// [0] / [1] and [4] / [5] build the GS offsets ((ofs + 2048 + centre) * 16), [2] / [3] and [6] / [7] the
// side planes, [8] is the projection distance (512), [9] / [10] the Z range (GA_SPR_WORK.zScale = 1 / (far -
// near)), [11] the value gaSet3dlibStruct's +0x5c scales.
#define GA_SCREEN_OFS_X 0
#define GA_SCREEN_OFS_Y 1
#define GA_SCREEN_W 2
#define GA_SCREEN_H 3
#define GA_SCREEN_CX 4
#define GA_SCREEN_CY 5
#define GA_SCREEN_SCALE_X 6
#define GA_SCREEN_SCALE_Y 7
#define GA_SCREEN_PROJ 8
#define GA_SCREEN_NEAR 9
#define GA_SCREEN_FAR 10
#define GA_SCREEN_11 11
#define GA_SCREEN_FLOATS 12

// The Z buffer's largest value by its pixel format (ZBUF.PSM, the draw buffer's zbuf bits 24..27).
#define GA_ZBUF_PSM_SHIFT 24
#define GA_ZBUF_PSM_MASK 0xf
#define GA_ZBUF_PSM_Z32 0
#define GA_ZBUF_PSM_Z24 1
#define GA_ZMAX_Z32 4294967040.0f       /* 0x4f7fffff */
#define GA_ZMAX_Z24 16777215.0f         /* 0x4b7fffff */
#define GA_ZMAX_Z16 65535.0f            /* 0x477fff00 */

#define GA_SUBPIXEL 16.0f               /* the GS's 12.4 fixed point */
#define GA_GS_OFFSET_CENTRE 2048.0f     /* the GS's drawing area is centred on 2048 */

// The side planes, in gaSet3dlibStruct's order (planeSec[] / planeTan[]).
#define GA_PLANE_LEFT 0
#define GA_PLANE_RIGHT 1
#define GA_PLANE_TOP 2
#define GA_PLANE_BOTTOM 3

// The 3dlib work gaSet3dlibStruct fills (ga3dlibwork, 0x400 bytes; main copies it to the scratchpad at
// 0x70000400 each frame through tnInit3dlibWork). GA_SPR_WORK's "work[n]" is this struct's n-th float.
// layout: ee (it is copied to the scratchpad and read by index). Names from gaSet3dlibStruct's stores only.
typedef struct GA_3DLIB_WORK {
    float screenW;                      /* 0x00: screen[2] */
    float screenH;                      /* 0x04: screen[3] */
    float scaleX;                       /* 0x08: screen[6]; gaSetZRollMatrix scales the roll by it */
    float scaleY;                       /* 0x0c: screen[7] */
    u_int zero10;                       /* 0x10: cleared first */
    float zMax;                         /* 0x14: GA_ZMAX_* by the Z buffer's format */
    float proj;                         /* 0x18: screen[8], the projection distance */
    float fog[3];                       /* 0x1c: the bank's fog floats (GA_FOGCTRL_DATA.fog) */
    float zNear;                        /* 0x28: screen[9] */
    float zFar;                         /* 0x2c: screen[10] */
    float planeSec[4];                  /* 0x30: sqrt(t^2 + proj^2) / proj per side plane (GA_PLANE_*) */
    float planeTan[4];                  /* 0x40: -t / proj (left, top) or t / proj (right, bottom) */
    float zClipScale;                   /* 0x50: zNear * (zMax - clipZ) */
    float scaleX16;                     /* 0x54: screen[6] * 16 */
    float scaleYNeg16;                  /* 0x58: -screen[7] * 16 */
    float zFogScale;                    /* 0x5c: zNear * (screen[11] - clipZ) / proj */
    float fog0Sq;                       /* 0x60: fog[0]^2 */
    float fog1Fog0Sq16;                 /* 0x64: fog[1] * fog[0]^2 * 16 */
    float gsOffsetX;                    /* 0x68: (screen[0] + 2048 + screen[4]) * 16 */
    float gsOffsetY;                    /* 0x6c: (screen[1] + 2048 + screen[5]) * 16 */
    float proj70;                       /* 0x70: screen[8] again */
} GA_3DLIB_WORK;
static_assert(offsetof(GA_3DLIB_WORK, zMax) == 0x14, "GA_3DLIB_WORK.zMax");
static_assert(offsetof(GA_3DLIB_WORK, fog) == 0x1c, "GA_3DLIB_WORK.fog");
static_assert(offsetof(GA_3DLIB_WORK, planeSec) == 0x30, "GA_3DLIB_WORK.planeSec");
static_assert(offsetof(GA_3DLIB_WORK, planeTan) == 0x40, "GA_3DLIB_WORK.planeTan");
static_assert(offsetof(GA_3DLIB_WORK, zClipScale) == 0x50, "GA_3DLIB_WORK.zClipScale");
static_assert(offsetof(GA_3DLIB_WORK, gsOffsetX) == 0x68, "GA_3DLIB_WORK.gsOffsetX");
static_assert(sizeof(GA_3DLIB_WORK) == 0x74, "GA_3DLIB_WORK");

// gaSetZRollMatrix's 2x2 roll-and-scale block in the scratchpad (EE 0x70000300: floats [0] [1] / [4] [5]).
#define GA_SPR_ZROLL_MATRIX 0x300

// The VU0 microprogram gaInit3dlib loads (0x018fe500..0x018ff430, 0xf3 qwords of MPG). Its routines at
// micro 0x000 and 0x010 are gaContMulMatrix's two matrix products (vf8..11 = vf8..11 * vf4..7, vf16..19 =
// vf16..19 * vf12..15).
#define VU0_3DLIB_PROGRAM_QWC 0xf3
extern u_int vu0_3dlib_program[VU0_3DLIB_PROGRAM_QWC * 4]; /* CT3 0x018fe500 */

// gaContMulMatrix's records: dst = b * a for each (0xc bytes of EE pointers in the ROM; layout: host, the
// function has no ROM caller).
typedef struct GA_MUL_MATRIX_REC {
    float (*dst)[4];
    float (*a)[4];
    float (*b)[4];
} GA_MUL_MATRIX_REC;

void gaInit3dlib(void);                                                                  // CT3 0x018a7700
void gaCopyVectorXYZ(float *dst, float *src);                                            // CT3 0x018a7730
void gaDivVectorXYZ(float *dst, float *a, float *b);                                     // CT3 0x018a7748
// a.xyz . b.xyz (asSearchObject's calls: a0, a1 -> f0).
float gaInnerProductXYZ(float *a, float *b);                                             // CT3 0x018a7780
float gaNormalizeVector2XYZ(float *dst, float *src);                                     // CT3 0x018a77d0
void gaClampVectorXYZ(float *dst, float *src, float lo, float hi);                       // CT3 0x018a7848
void gaInterVectorXYZ(float *dst, float *a, float *b, float t);                          // CT3 0x018a7870
void gaDivVector(float *dst, float *a, float *b);                                        // CT3 0x018a7898
void gaClampVector(float *dst, float *src, float lo, float hi);                          // CT3 0x018a78d8
// dst = b * a (each row of b times a; see the conventions above). dst may be a or b.
void gaMulMatrix(float (*dst)[4], float (*a)[4], float (*b)[4]);                        // CT3 0x018a7900
void gaContMulMatrix(GA_MUL_MATRIX_REC *recs, int count);                                // CT3 0x018a7978
void gaTransposeMatrix(float (*dst)[4], float (*src)[4]);                                // CT3 0x018a7a80
// dst = a's inverse for a rigid matrix (the 3x3 transposed, the translation turned back); in place is fine.
void gaInversMatrix(float (*dst)[4], float (*a)[4]);                                     // CT3 0x018a7b50
// Angles are the ga library's int units (gaSinCos's).
void gaSetRotMatrixXYZ(float (*dst)[4], int *angles);                                    // CT3 0x018a7bc0
void gaSetRotMatrixZYX(float (*dst)[4], int *angles);                                    // CT3 0x018a7c10
void gaRotMatrixX(float (*dst)[4], int angle, float (*src)[4]);                          // CT3 0x018a7c60
void gaRotMatrixY(float (*dst)[4], int angle, float (*src)[4]);                          // CT3 0x018a7ca8
void gaRotMatrixZ(float (*dst)[4], int angle, float (*src)[4]);                          // CT3 0x018a7cf0
void gaRotMatrixXYZ(float (*dst)[4], int *angles, float (*src)[4]);                      // CT3 0x018a7d38
void gaRotMatrixZYX(float (*dst)[4], int *angles, float (*src)[4]);                      // CT3 0x018a7d78
// clipZ: main_rtn passes 50000.0f ($f12); work: ga3dlibwork (GA_3DLIB_WORK); screen: screen_0's twelve floats.
void gaSet3dlibStruct(float clipZ, void *work, GA_DBUFFER_DATA *drawbuffer, GA_FOGCTRL_DATA *fogctrl,
                      float *screen);                                                    // CT3 0x018a7db8
#define GA_OBJECT_SIZE 0x80             /* a ga draw object, as gaCopyObject2 copies it */
void gaCopyObject(void *dst, void *src);                                                 // CT3 0x018a8140
// dst's 0x80 bytes = src's (asStockShadowVolume's template copies).
void gaCopyObject2(void *dst, const void *src);                                          // CT3 0x018a8210
// Loads a part's lights (GA_LIGHT_WORK, ct3_types.h) for drawing under the view matrix view.
void gaSetLight(void *lightWork, float (*view)[4]);                                      // CT3 0x018a8258
void gaSetZRollMatrix(int roll);                                                         // CT3 0x018a8548
