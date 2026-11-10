// game/ArtoolDebugOpen.h -- CT3 game/ArtoolDebugOpen.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void ArtoolDebugOpen(void);                                                              // CT3 0x0189e180
void IopLoadModules(const char *elfPath);                                                // CT3 0x0189f2f0
int FlInit(void);                                                                        // CT3 0x0189f160
