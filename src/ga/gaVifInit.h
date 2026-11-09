// ga/gaVifInit.h -- CT3 ga/gaVifInit.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void gaVifInit(void);                                                                    // CT3 0x018a1948
void gaVifcodeFlush_VIF1(void);                                                          // CT3 0x018a19b8
int gaVIFSync_VIF1(void);                                                                // CT3 0x018a1a28
