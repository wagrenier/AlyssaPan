// ga/gaSinCos.h -- CT3 ga/gaSinCos.c: the declarations other TUs call (ahRelocateSunPs2ObjectArry.c slice 12, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// *s, *c = the sine and cosine of angle (ahSin / ahCos read the first / second; QuatSlerp.c slice 1).
void gaSinCos(float *s, float *c, int angle);                                            // CT3 0x018a6b70
// m = the rotation by angle (0x10000 = a full turn) about X / Y / Z.
void gaSetRotMatrixX(float (*m)[4], int angle);                                          // CT3 0x018a6ba0
void gaSetRotMatrixY(float (*m)[4], int angle);                                          // CT3 0x018a6c10
void gaSetRotMatrixZ(float (*m)[4], int angle);                                          // CT3 0x018a6c70
