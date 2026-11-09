// ga/gaObjectClippingCheck.h -- CT3 ga/gaObjectClippingCheck.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once
#include "sdk/scetypes.h"
// < 0: outside (not drawn); 0: inside; > 0: crosses a clip plane (stored in the object's +2).
int gaObjectClippingCheck(u_char *obj);                                                  // CT3 0x018a3078
