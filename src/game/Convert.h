// game/Convert.h -- CT3 game/Convert.c: the declarations other TUs call (asMakeStraw first, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void ConvertMatrix2Quat(float *q, float (*m)[4]);                                        // CT3 0x018518d0
void ConvertQuat2Matrix(float (*m)[4], float *q);                                        // CT3 0x01851b10
