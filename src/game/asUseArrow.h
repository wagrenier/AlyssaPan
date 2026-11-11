// game/asUseArrow.h -- CT3 game/asUseArrow.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void asInitArrowInfo(void);                                                              // CT3 0x018666c0
void asDestroyArrowInfo(void);                                                           // CT3 0x018666e0
void asArrowOperate(void);                                                               // CT3 0x01866d80
// From asPIA1_KEY_TBL_YUMI (wideXcmd_DebugPrint.c, 2026-11-10): the bow's draw (f0), 0.5 shoots.
float asGetTheArrowRad(void);                                                            // CT3 0x01866720
