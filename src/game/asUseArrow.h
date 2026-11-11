// game/asUseArrow.h -- CT3 game/asUseArrow.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void asInitArrowInfo(void);                                                              // CT3 0x018666c0
void asDestroyArrowInfo(void);                                                           // CT3 0x018666e0
void asArrowOperate(void);                                                               // CT3 0x01866d80
// From asPIA1_KEY_TBL_YUMI (wideXcmd_DebugPrint.c, 2026-11-10): the bow's draw (f0), 0.5 shoots.
float asGetTheArrowRad(void);                                                            // CT3 0x01866720
// From asPIA0_KEY_TBL_YUMI (wideXcmd_DebugPrint.c, 2026-11-10; argument types from their bodies):
// the index of the first arrow in state 3 or 4 (state 1 only asks; any other sets it), -1 for none.
int asChangeTheArrowState(int state);                                                    // CT3 0x01866980
// Shoot an arrow of kind `kind` (a free slot, state 2, becomes 3); -1 with no free slot.
int asShootArrowSetting(u_char kind, u_short a1, u_short a2);                             // CT3 0x01866c10
// From tiXcmd_UseItem (tiXcmd_TransItem.c, 2026-11-11): the argument is the item variable's value; the
// body (read by the decompiler) clears ArrowList[0] and shifts the list down.
void asUseArrow(int arrow);                                                              // CT3 0x018665e0
