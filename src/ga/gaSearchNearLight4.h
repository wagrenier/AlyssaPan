// ga/gaSearchNearLight4.h -- CT3 ga/gaSearchNearLight4.c: the declarations other TUs call
// (game/Character_Init.c first, 2026-11-10). Prototype confirmed by the TU (2026-11-12).
#pragma once

#include "sdk/scetypes.h"

// For each of posCount positions, the indices of the (up to) four lights nearest to it by
// |p - light.xyz|^2 * light.w^2, counted only below 1.0 (w: the inverse range), nearest first, one byte
// each (NEAR_LIGHT_NONE: none) at nearest[p * 4 + rank]. Every pass of eight positions writes 32 bytes:
// nearest[] needs posCount rounded up to 8, times 4. SceneLightSet / CharLightSet / asSearchObject pass
// one position. The fifth argument is in t0 (Ghidra's call view drops it).
void gaSearchNearLight4(u_char *nearest, float (*pos)[4], int posCount, float (*lights)[4], int lightCount); // CT3 0x018a7538
