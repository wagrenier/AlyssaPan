// game/tkXDTrg_GetSceneHash.h -- CT3 game/tkXDTrg_GetSceneHash.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void tkXcmd_DoorTrigger_Init(void);                                                      // CT3 0x018ed880
// From data_save_set / data_load_set (Character_Init.c, 2026-11-10).
void tkXcmd_DoorFlagClear(u_char *eventFlags);                                           // CT3 0x018ed320
void SceneDoorFlagClear(u_long roomHash);                                                // CT3 0x018eda60

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int tkXcmd_AiSoundPosition(int argc, char **argv, int bank);                             // CT3 0x018ee5e0
int tkXcmd_SceneDraw(int argc, char **argv, int bank);                                   // CT3 0x018ee5b0
int tkXcmd_WaitUntil(int argc, char **argv, int bank);                                   // CT3 0x018ee5a0
int tkXcmd_DoorTrigger(int argc, char **argv, int bank);                                 // CT3 0x018ed9e0
