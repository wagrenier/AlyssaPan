// game/tiBrain_ChgTrace_BOSS_BALOWS.h -- CT3 game/tiBrain_ChgTrace_BOSS_BALOWS.c: the declarations other
// TUs call (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that
// writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
// Brain initialisers: slots 3 and 4 of the ssAI_BrainInit table (0x01938580).
int tiBrain_Init_BOSS1(struct CHARACTER *chr);                                           // CT3 0x01874960
int tiBrain_Init_BOSS_SCISSORS_W(struct CHARACTER *chr);                                 // CT3 0x01874830
