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

// The brain's mode functions (tiAvoidObject.c slice 3, 2026-11-12): entries of ssAI_charaMain's tables
// (ssAI_<Mode>Mode run the mode, ssAI_Chg<Mode>Mode pick the next; ssAI_DamageMode answers non-zero when
// the damage handling took the frame). Prototypes from that call site: chr is the character.
void tiBrain_ChgTrace_Ghost(struct CHARACTER *chr);                                      // CT3 0x01875b10
void tiBrain_ChgReturn2(struct CHARACTER *chr);                                          // CT3 0x01875b20
void tiBrain_ChgReturn1(struct CHARACTER *chr);                                          // CT3 0x01875b80
void tiBrain_ChgAttack_Ghost4(struct CHARACTER *chr);                                    // CT3 0x01875c20
void tiBrain_ChgAttack_Ghost3(struct CHARACTER *chr);                                    // CT3 0x01875f00
void tiBrain_ChgAttack_Ghost2(struct CHARACTER *chr);                                    // CT3 0x01875f60
void tiBrain_ChgAttack_Ghost1(struct CHARACTER *chr);                                    // CT3 0x01876190
void tiBrain_ChgApproach_Ghost2(struct CHARACTER *chr);                                  // CT3 0x01876400
void tiBrain_ChgApproach_Ghost1(struct CHARACTER *chr);                                  // CT3 0x01876670
void tiBrain_ChgSearch_Ghost3(struct CHARACTER *chr);                                    // CT3 0x018768f0
void tiBrain_ChgSearch_Ghost2(struct CHARACTER *chr);                                    // CT3 0x01876b50
void tiBrain_ChgSearch_Ghost1(struct CHARACTER *chr);                                    // CT3 0x01876d90
void tiBrain_Trace_Ghost1(struct CHARACTER *chr);                                        // CT3 0x01877050
int tiBrain_Damage3(struct CHARACTER *chr);                                              // CT3 0x018770e0
int tiBrain_Damage2(struct CHARACTER *chr);                                              // CT3 0x01877280
int tiBrain_Damage1(struct CHARACTER *chr);                                              // CT3 0x01877610
void tiBrain_Return_Ghost1(struct CHARACTER *chr);                                       // CT3 0x01877950
void tiBrain_Attack_Ghost4(struct CHARACTER *chr);                                       // CT3 0x01877a00
void tiBrain_Attack_Ghost3(struct CHARACTER *chr);                                       // CT3 0x01878280
void tiBrain_Attack_Ghost2(struct CHARACTER *chr);                                       // CT3 0x01878960
void tiBrain_Attack_Ghost1(struct CHARACTER *chr);                                       // CT3 0x01879200
void tiBrain_Approach_Ghost2(struct CHARACTER *chr);                                     // CT3 0x01879a80
void tiBrain_Approach_Ghost1(struct CHARACTER *chr);                                     // CT3 0x01879d70
void tiBrain_Search_Ghost3(struct CHARACTER *chr);                                       // CT3 0x0187a2b0
void tiBrain_Search_Ghost2(struct CHARACTER *chr);                                       // CT3 0x0187a760
void tiBrain_Search_Ghost1(struct CHARACTER *chr);                                       // CT3 0x0187a7b0
