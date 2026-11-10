// game/main2.h -- CT3 game/main2.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void main2(void);                                                                        // CT3 0x018b20f0
void CharacterAllFree(void);                                                             // CT3 0x018b23f0
void SelfDiscardSceneLoadUpdate(void);                                                   // CT3 0x018b2e60
void SelfDiscardSceneLoadInit(void);                                                     // CT3 0x018b3480
void SwapDrawingDoor(void);                                                              // CT3 0x018b22f0
// From mc_load_set's calls (Character_Init.c, 2026-11-10): the first arguments are 0 there.
void TrgCharacterDraw(int mode, int chara);                                              // CT3 0x018b2300
void TrgSceneDraw(int a0, int a1, int a2, char *name, int bank);                         // CT3 0x018b3590
