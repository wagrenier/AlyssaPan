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
// From data_load_set's calls (Character_Init.c, 2026-11-10): names are 4-char bank names, -1 / -2
// as the ROM passes them (-2: TrgSceneDraw places the bank at DrawOnPosSetVectorTemp).
int TrgSceneLoad(char *name, int bank, int a2);                                          // CT3 0x018b38b0
void TrgGLBLSceneLoad(char *name, int bank);                                             // CT3 0x018b3940
void TrgGLBLSceneDraw(int mode, char *name, int bank);                                   // CT3 0x018b3990
void TrgCharacterBehaviorLoad(int chara, char *name, int bank, int a3);                  // CT3 0x018b2d00
void TrgCharacterBehaviorSet(int chara, char *name, int a2);                             // CT3 0x018b24c0
void TrgCharacterShapeLoad(int chara, char *name, int bank, int a3);                     // CT3 0x018b2d60
void TrgCharacterShapeSet(int chara, char *name, int a2, int a3, int a4);                // CT3 0x018b26f0
