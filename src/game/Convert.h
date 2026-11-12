// game/Convert.h -- CT3 game/Convert.c (0x018518d0..0x01851bf0): quaternion <-> rotation matrix.
// Prototypes confirmed by the TU (2026-11-12): q is { x, y, z, w }, m four rows of four floats.
#pragma once

#include "sdk/scetypes.h"

void ConvertMatrix2Quat(float *q, float (*m)[4]);                                        // CT3 0x018518d0
void ConvertQuat2Matrix(float (*m)[4], float *q);                                        // CT3 0x01851b10
