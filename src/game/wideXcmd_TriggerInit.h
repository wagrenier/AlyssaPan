// game/wideXcmd_TriggerInit.h -- CT3 game/wideXcmd_TriggerInit.c: the declarations other TUs call
// (game/wideXcmd_DebugPrint.c first, 2026-11-10). Prototypes from the call sites; the TU that writes
// these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// An xcmd: argv[0] (atoi) is read first; asYumi_Run_Check passes "0", "4" and Alyssa's bank.
void ssXcmd_LockToTargChara(int argc, char **argv, short bank);                         // CT3 0x018dfd70
