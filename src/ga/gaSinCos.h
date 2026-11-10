// ga/gaSinCos.h -- CT3 ga/gaSinCos.c: the declarations other TUs call (ahRelocateSunPs2ObjectArry.c slice 12, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// m = the rotation by angle (0x10000 = a full turn) about X / Y.
void gaSetRotMatrixX(float (*m)[4], int angle);                                          // CT3 0x018a6ba0
void gaSetRotMatrixY(float (*m)[4], int angle);                                          // CT3 0x018a6c10
