// game/tiBrain_ChgTrace_BOSS_BALOWS.h -- CT3 game/tiBrain_ChgTrace_BOSS_BALOWS.c: the declarations other
// TUs call (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that
// writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
// Brain initialisers: slots 3 and 4 of the ssAI_BrainInit table (0x01938580).
int tiBrain_Init_BOSS1(struct CHARACTER *chr);                                           // CT3 0x01874960
int tiBrain_Init_BOSS_SCISSORS_W(struct CHARACTER *chr);                                 // CT3 0x01874830

// The brain's mode functions (tiAvoidObject.c slice 3, 2026-11-12): entries of ssAI_charaMain's tables
// (ssAI_<Mode>Mode run the mode, ssAI_Chg<Mode>Mode pick the next; ssAI_DamageMode answers non-zero when
// the damage handling took the frame). Prototypes from that call site: chr is the character.
void tiBrain_ChgTrace_BOSS_BALOWS(struct CHARACTER *chr);                                // CT3 0x01867fb0
void tiBrain_ChgAttack_BOSS_BALOWS(struct CHARACTER *chr);                               // CT3 0x01868070
void tiBrain_ChgApproach_BOSS_BALOWS(struct CHARACTER *chr);                             // CT3 0x01868200
void tiBrain_ChgTrace_BOSS_SCISSORS_M(struct CHARACTER *chr);                            // CT3 0x01868500
void tiBrain_ChgAttack_BOSS_SCISSORS_M(struct CHARACTER *chr);                           // CT3 0x01868580
void tiBrain_ChgApproach_BOSS_SCISSORS_M(struct CHARACTER *chr);                         // CT3 0x018685f0
void tiBrain_ChgAttack_BOSS_SCISSORS_W(struct CHARACTER *chr);                           // CT3 0x01868860
void tiBrain_ChgApproach_BOSS_SCISSORS_W(struct CHARACTER *chr);                         // CT3 0x01868870
void tiBrain_ChgTrace_BOSS_AXE(struct CHARACTER *chr);                                   // CT3 0x018688c0
void tiBrain_ChgAttack_BOSS_AXE(struct CHARACTER *chr);                                  // CT3 0x01868950
void tiBrain_ChgApproach_BOSS_AXE(struct CHARACTER *chr);                                // CT3 0x01868a00
void tiBrain_ChgTrace_BOSS_VITRIOL(struct CHARACTER *chr);                               // CT3 0x01868db0
void tiBrain_ChgAttack_BOSS_VITRIOL(struct CHARACTER *chr);                              // CT3 0x01868e50
void tiBrain_ChgApproach_BOSS_VITRIOL(struct CHARACTER *chr);                            // CT3 0x01868f00
void tiBrain_ChgTrace_BOSS_HAMM(struct CHARACTER *chr);                                  // CT3 0x01869150
void tiBrain_ChgAttack_BOSS_HAMM(struct CHARACTER *chr);                                 // CT3 0x018691e0
void tiBrain_ChgApproach_BOSS_HAMM(struct CHARACTER *chr);                               // CT3 0x018692f0
void tiBrain_ChgReturn_BOSS1(struct CHARACTER *chr);                                     // CT3 0x01869540
void tiBrain_Trace_BOSS_BALOWS(struct CHARACTER *chr);                                   // CT3 0x01869550
void tiBrain_Attack_BOSS_BALOWS(struct CHARACTER *chr);                                  // CT3 0x01869930
void tiBrain_Trace_BOSS_SCISSORS_M(struct CHARACTER *chr);                               // CT3 0x0186ab60
void tiBrain_Attack_BOSS_SCISSORS_M(struct CHARACTER *chr);                              // CT3 0x0186af60
void tiBrain_Attack_BOSS_SCISSORS_W(struct CHARACTER *chr);                              // CT3 0x0186b7a0
void tiBrain_Trace_BOSS_AXE(struct CHARACTER *chr);                                      // CT3 0x0186c540
void tiBrain_Attack_BOSS_AXE(struct CHARACTER *chr);                                     // CT3 0x0186cac0
void tiBrain_Trace_BOSS_VITRIOL(struct CHARACTER *chr);                                  // CT3 0x0186cf80
void tiBrain_Attack_BOSS_VITRIOL(struct CHARACTER *chr);                                 // CT3 0x0186d750
void tiBrain_Trace_BOSS_HAMM(struct CHARACTER *chr);                                     // CT3 0x0186e0c0
void tiBrain_Attack_BOSS_HAMM(struct CHARACTER *chr);                                    // CT3 0x0186e0d0
void tiBrain_Return_BOSS1(struct CHARACTER *chr);                                        // CT3 0x0186ec10
int tiBrain_Damage_BOSS1(struct CHARACTER *chr);                                         // CT3 0x0186ec40
void tiBrain_Approach_BOSS1(struct CHARACTER *chr);                                      // CT3 0x01870e00
