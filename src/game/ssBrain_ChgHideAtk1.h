// game/ssBrain_ChgHideAtk1.h -- CT3 game/ssBrain_ChgHideAtk1.c: the declarations other TUs call
// (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that writes
// these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
// Brain initialiser: slot 0 of the ssAI_BrainInit table (0x01938580).
int ssBrain_Init1(struct CHARACTER *chr);                                                // CT3 0x01880b60
