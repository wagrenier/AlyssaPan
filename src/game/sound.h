// game/sound.h -- CT3 game/sound.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void Sound_system_init(void);                                                            // CT3 0x018cbd00
void Sound_FirstInitialization(void);                                                    // CT3 0x018cbda0
void SSSetSoundMode(int mode);                                                           // CT3 0x018cbe30
