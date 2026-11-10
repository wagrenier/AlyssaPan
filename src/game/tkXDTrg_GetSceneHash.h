// game/tkXDTrg_GetSceneHash.h -- CT3 game/tkXDTrg_GetSceneHash.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void tkXcmd_DoorTrigger_Init(void);                                                      // CT3 0x018ed880
// From data_save_set / data_load_set (Character_Init.c, 2026-11-10).
void tkXcmd_DoorFlagClear(u_char *eventFlags);                                           // CT3 0x018ed320
void SceneDoorFlagClear(u_long roomHash);                                                // CT3 0x018eda60
