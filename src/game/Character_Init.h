// game/Character_Init.h -- CT3 game/Character_Init.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void Character_All_Init(void);                                                           // CT3 0x018b4070
void IOPInit(char **argv);                                                               // CT3 0x018b4430
void MemorycardUpdate(void);                                                             // CT3 0x018b6080
void SceneLightSet(int scene);                                                           // CT3 0x018b4480
