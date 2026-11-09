// ga/gaBGT_DrawObject.h -- CT3 ga/gaBGT_DrawObject.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once
#include "sdk/scetypes.h"
void gaBGT_DrawObject(u_char *obj, int attr);                                            // CT3 0x018a3150
void gaBGT_DrawObject_NoClippingCheck2(u_char *obj, int attr);                           // CT3 0x018a3d20
