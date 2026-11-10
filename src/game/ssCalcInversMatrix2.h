// game/ssCalcInversMatrix2.h -- CT3 game/ssCalcInversMatrix2.c: the declarations other TUs call
// (asMakeStraw first, 2026-11-10). Prototypes from the call sites; the TU that writes these confirms them.
#pragma once

#include "sdk/scetypes.h"

// out = the inverse of m (asMakeStraw keeps its translation row too).
void ssCalcInversMatrix2(float (*out)[4], float (*m)[4]);                                // CT3 0x018a88e0
