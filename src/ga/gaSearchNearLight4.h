// ga/gaSearchNearLight4.h -- CT3 ga/gaSearchNearLight4.c: the declarations other TUs call
// (game/Character_Init.c first, 2026-11-10). Prototype from the call sites; the TU confirms it.
#pragma once

#include "sdk/scetypes.h"

// For each of posCount positions, the indices of the (up to) four lights nearest to it, one byte each
// (NEAR_LIGHT_NONE: none) into nearest[]. SceneLightSet / CharLightSet pass one position. The fifth
// argument is in t0 (the ROM's call sites; Ghidra's call view drops it).
void gaSearchNearLight4(u_char *nearest, float (*pos)[4], int posCount, float (*lights)[4], int lightCount); // CT3 0x018a7538
