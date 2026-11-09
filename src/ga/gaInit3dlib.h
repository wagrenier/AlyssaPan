// ga/gaInit3dlib.h -- CT3 ga/gaInit3dlib.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void gaInit3dlib(void);                                                                  // CT3 0x018a7700
void gaSetZRollMatrix(int roll);                                                         // CT3 0x018a8548
// clipZ: main_rtn passes 50000.0f ($f12); screen: screen_0's twelve floats.
void gaSet3dlibStruct(float clipZ, void *work, void *drawbuffer, void *fogctrl, void *screen); // CT3 0x018a7db8
