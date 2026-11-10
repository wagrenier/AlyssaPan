// ga/gaSinCos.h -- CT3 ga/gaSinCos.c (0x018a6b70..0x018a7538): the ga library's hand-written helpers --
// the sine table and rotation matrices, VU0 float / int conversion, word / dword / qword copies, a byte
// fill, a perspective projection, the point-light block and a vector list transform. Reconstructed
// 2026-11-10 from the disassembly (mipsdis.py; vudis.py for gaProjectScreen's VU0 micro routines).
#pragma once

#include "sdk/scetypes.h"

struct GA_LIGHT_WORK;

// The ga library's angle: 0x10000 = a full turn; the sine table resolves it to 4 units (0x4000 entries,
// the angle rounded to the nearest multiple of 4, any int wrapped by its low 16 bits).
#define GA_ANGLE_FULL_TURN 0x10000
#define GA_SINCOS_TABLE_SIZE 0x4000     /* {sin, cos} pairs */
extern const u_int ga_sincos_table[GA_SINCOS_TABLE_SIZE * 2]; /* CT3 0x0190e640, float bit patterns */

// *s, *c = the sine and cosine of angle (ahSin / ahCos read the first / second; QuatSlerp.c).
void gaSinCos(float *s, float *c, int angle);                                            // CT3 0x018a6b70
// m = the rotation by angle about X / Y / Z (row vectors: v' = v * m); an angle that rounds to entry 0
// writes the identity.
void gaSetRotMatrixX(float (*m)[4], int angle);                                          // CT3 0x018a6ba0
void gaSetRotMatrixY(float (*m)[4], int angle);                                          // CT3 0x018a6c10
void gaSetRotMatrixZ(float (*m)[4], int angle);                                          // CT3 0x018a6c70
// VFTOI0 / VITOF0: truncation toward zero (saturating), and int to float.
int gaInt(float f);                                                                      // CT3 0x018a6cd0
float gaFloat(int i);                                                                    // CT3 0x018a6ce8
// memmove of count words / dwords / qwords (backwards when dst is above src).
void gaMemCopy32(void *dst, const void *src, u_int count);                               // CT3 0x018a6d00
void gaMemCopy64(void *dst, const void *src, u_int count);                               // CT3 0x018a6e28
void gaMemCopy128(void *dst, const void *src, u_int count);                              // CT3 0x018a6f50
// memset of count bytes.
void gaMemFill8(void *dst, u_char value, u_int count);                                   // CT3 0x018a7078
// out[i] = the screen position of in[i] * m (the scratchpad's projection constants, GA_SPR_WORK +0x60..
// +0x8c), w = 1; count is rounded up to even (one more point is written), count >= 1.
void gaProjectScreen(float (*out)[4], float (*m)[4], float (*in)[4], int count);         // CT3 0x018a7110
// lw's four point lights from the lights index[0..3] names (NEAR_LIGHT_NONE ends the list): positions
// pos[] under m into omniView (one column per light), colours color[] into omniColor.
void gaSetPointLight(struct GA_LIGHT_WORK *lw, float (*m)[4], float (*pos)[4], float (*color)[4],
                     const u_char *index);                                               // CT3 0x018a71c0
// out[i] = in[i] * m (all four components, in's w included), count vectors; count <= 0 does nothing.
void gaContApplyMatrix(float (*out)[4], float (*m)[4], float (*in)[4], int count);       // CT3 0x018a74ac
