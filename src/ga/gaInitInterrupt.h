// ga/gaInitInterrupt.h -- CT3 ga/gaInitInterrupt.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void gaInitInterrupt(void);                                                              // CT3 0x018a86a8
void gaWaitVSync(int mode);                                                              // CT3 0x018a8740
