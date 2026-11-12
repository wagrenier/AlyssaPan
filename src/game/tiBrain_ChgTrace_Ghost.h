// game/tiBrain_ChgTrace_Ghost.h -- CT3 game/tiBrain_ChgTrace_Ghost.c: the declarations other TUs call
// (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that writes
// these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
// Brain initialisers: slots of the ssAI_BrainInit table (0x01938580), indexed by the brain's kind byte
// (+0x230); each reads the character's brain (CHARACTER.brain) itself (tiBrain_Init2: a0 only).
int tiBrain_Init1(struct CHARACTER *chr);                                                // CT3 0x0187aeb0
int tiBrain_Init2(struct CHARACTER *chr);                                                // CT3 0x0187ae40
int tiBrain_Init3(struct CHARACTER *chr);                                                // CT3 0x0187adb0
