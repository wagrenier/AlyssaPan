// game/tiAvoidObject.h -- CT3 game/tiAvoidObject.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void ssAI_main(void);                                                                    // CT3 0x01863ed0
void ssAI_GlobalInit(void);                                                              // CT3 0x01866490
void ssAI_SceneInit(int scene);                                                          // CT3 0x018660a0
// The AI's part of a save image (Character_Init.c's data_save_set / data_load_set pass the image).
void ssAI_MemSaveData(u_char *data);                                                     // CT3 0x018662a0
void ssAI_MemLoadInit(u_char *data);                                                     // CT3 0x018662d0
