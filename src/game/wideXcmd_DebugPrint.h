// game/wideXcmd_DebugPrint.h -- CT3 game/wideXcmd_DebugPrint.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

void asKatchuKekkaiCheck(int a0);                                                        // CT3 0x018f40d0
void asPanicStatusCheck(int mode);                                                       // CT3 0x018f42e0
void asCharacterPositionReviewInPausing(CHARACTER *chr);                                 // CT3 0x018f4bd0
void asCharacterForceMoveOperate(CHARACTER *chr);                                        // CT3 0x018f4c60
void asCharacterOperate(CHARACTER *chr, int a1, int a2, int a3);                         // CT3 0x018f4fc0
void asCharacterPositioning(CHARACTER *chr, int a1, int a2);                             // CT3 0x018f6f80
void asResetCharacterRotation(int a0, int a1, int a2);                                   // CT3 0x018f73b0
void asRapidLeverChangeOperate(int a0, int a1, int a2, int a3, int a4);                  // CT3 0x018fb4c0
void asMonsterDistanceStack_inPanic(int a0, int a1, int a2);                             // CT3 0x018fb940
void asAlyssaLockedByAnything(int mode);                                                 // CT3 0x018fbb50
void asSetCharacterKeyEnable(int enable);                                                // CT3 0x018fe370
void ChrMoveControllerMode_init(void);                                                   // CT3 0x018fe4b0
void asInitPlayerBehaviorControler(CHARACTER *chr);                                      // CT3 0x018fe380
