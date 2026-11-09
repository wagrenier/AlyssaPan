// ga/gaDma.h -- CT3 ga/gaDma.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void gaDmaInit(void);                                                                    // CT3 0x018a1188
void gaDmaSendN_toSPR(void *spr, void *src, int qwc, int mode);                          // CT3 0x018a1598
void gaDmaSync_toSPR(void);                                                              // CT3 0x018a1680
