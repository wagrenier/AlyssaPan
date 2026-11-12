// game/ssBrain_ChgHideAtk1.h -- CT3 game/ssBrain_ChgHideAtk1.c: the declarations other TUs call
// (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that writes
// these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
// Brain initialiser: slot 0 of the ssAI_BrainInit table (0x01938580).
int ssBrain_Init1(struct CHARACTER *chr);                                                // CT3 0x01880b60

// The brain's mode functions (tiAvoidObject.c slice 3, 2026-11-12): entries of ssAI_charaMain's tables
// (ssAI_<Mode>Mode run the mode, ssAI_Chg<Mode>Mode pick the next; ssAI_DamageMode answers non-zero when
// the damage handling took the frame). Prototypes from that call site: chr is the character.
void ssBrain_ChgHideAtk1(struct CHARACTER *chr);                                         // CT3 0x0187b160
void ssBrain_ChgReturn1(struct CHARACTER *chr);                                          // CT3 0x0187b260
void ssBrain_ChgAttack1(struct CHARACTER *chr);                                          // CT3 0x0187b270
void ssBrain_ChgTrace1(struct CHARACTER *chr);                                           // CT3 0x0187b640
void ssBrain_ChgApproach1(struct CHARACTER *chr);                                        // CT3 0x0187ba60
void ssBrain_ChgSearch1(struct CHARACTER *chr);                                          // CT3 0x0187be60
int ssBrain_Damage1(struct CHARACTER *chr);                                              // CT3 0x0187c4a0
void ssBrain_HideAtk1(struct CHARACTER *chr);                                            // CT3 0x0187d310
void ssBrain_Return1(struct CHARACTER *chr);                                             // CT3 0x0187d6e0
void ssBrain_Attack1(struct CHARACTER *chr);                                             // CT3 0x0187d770
void ssBrain_Trace1(struct CHARACTER *chr);                                              // CT3 0x0187e3d0
void ssBrain_Approach1(struct CHARACTER *chr);                                           // CT3 0x0187f000
void ssBrain_Search1(struct CHARACTER *chr);                                             // CT3 0x0187fbe0
// From ssAI_SetMonsterPos (tiAvoidObject.c slice 6, 2026-11-12): -1 when no trace could be planned.
int ssAI_SetTraceMode(struct CHARACTER *chr, int a1);                                    // CT3 0x018817b0
