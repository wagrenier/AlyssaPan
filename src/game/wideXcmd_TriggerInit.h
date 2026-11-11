// game/wideXcmd_TriggerInit.h -- CT3 game/wideXcmd_TriggerInit.c: the declarations other TUs call
// (game/wideXcmd_DebugPrint.c first, 2026-11-10). Prototypes from the call sites; the TU that writes
// these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// An xcmd: argv[0] (atoi) is read first; asYumi_Run_Check passes "0", "4" and Alyssa's bank.
void ssXcmd_LockToTargChara(int argc, char **argv, short bank);                         // CT3 0x018dfd70

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int ssXcmd_SetHideAttackPole(int argc, char **argv, int bank);                           // CT3 0x018de410
int wideXcmd_TriggerInit(int argc, char **argv, int bank);                               // CT3 0x018dd550
int ssXcmd_test3(int argc, char **argv, int bank);                                       // CT3 0x018e0e30
int ssXcmd_ArrowSetStartPos(int argc, char **argv, int bank);                            // CT3 0x018e0420
int ssXcmd_ScnAtkRequest(int argc, char **argv, int bank);                               // CT3 0x018e0c10
int ssXcmd_SetDurabilityToVar(int argc, char **argv, int bank);                          // CT3 0x018e0b40
int ssXcmd_SetArrowPath(int argc, char **argv, int bank);                                // CT3 0x018e0670
int ssXcmd_SetArrowPath2(int argc, char **argv, int bank);                               // CT3 0x018e0590
int ssXcmd_GetAtkHitInfoID(int argc, char **argv, int bank);                             // CT3 0x018dfb80
int ssXcmd_CheckAtkHitMaterial(int argc, char **argv, int bank);                         // CT3 0x018dfa10
int ssXcmd_CheckAtkHitChara(int argc, char **argv, int bank);                            // CT3 0x018df8a0
int ssXcmd_ArrowSetChild(int argc, char **argv, int bank);                               // CT3 0x018e01d0
int ssXcmd_ArrowSetHitPos(int argc, char **argv, int bank);                              // CT3 0x018e0a90
int ssXcmd_ObjectSetHitPos(int argc, char **argv, int bank);                             // CT3 0x018e09c0
int ssXcmd_GetCharFootEndMaterial(int argc, char **argv, int bank);                      // CT3 0x018df610
int ssXcmd_GetAttackMotionID(int argc, char **argv, int bank);                           // CT3 0x018e08a0
int ssXcmd_SetArwRotMaxID(int argc, char **argv, int bank);                              // CT3 0x018e0170
int test_ssXcmd_ChangeGroupID(int argc, char **argv, int bank);                          // CT3 0x018df600
int test_ssXcmd_BrainSet(int argc, char **argv, int bank);                               // CT3 0x018df5f0
int test_ssXcmd_SetGroupID(int argc, char **argv, int bank);                             // CT3 0x018dee50
int test_ssXcmd_BrainSet2(int argc, char **argv, int bank);                              // CT3 0x018df5e0
int test_ssXcmd_BrainSet3(int argc, char **argv, int bank);                              // CT3 0x018df5d0
int test_ssXcmd_BrainEnd(int argc, char **argv, int bank);                               // CT3 0x018deed0
int test_ssXcmd_BrainSet4(int argc, char **argv, int bank);                              // CT3 0x018def30
int ssXcmd_GetChaseAIState(int argc, char **argv, int bank);                             // CT3 0x018ded70
int ssXcmd_SetChaseFlagAndTimer(int argc, char **argv, int bank);                        // CT3 0x018dec60
int ssXcmd_SetAppearanceTimer(int argc, char **argv, int bank);                          // CT3 0x018dec30
int ssXcmd_SetAllControlDisable(int argc, char **argv, int bank);                        // CT3 0x018de9d0
int ssXcmd_SetNoReactionTimer(int argc, char **argv, int bank);                          // CT3 0x018de9a0
int ssXcmd_StayGroup(int argc, char **argv, int bank);                                   // CT3 0x018de890
int ssXcmd_IsMonAI_SearchMode(int argc, char **argv, int bank);                          // CT3 0x018de7e0
int ssXcmd_SetMonReturnTimer(int argc, char **argv, int bank);                           // CT3 0x018de670
int ssXcmd_AXE_InitParam(int argc, char **argv, int bank);                               // CT3 0x018de260
int ssXcmd_AXE_SetStartPos(int argc, char **argv, int bank);                             // CT3 0x018de100
int ssXcmd_AXE_SetPath(int argc, char **argv, int bank);                                 // CT3 0x018ddcd0
int ssXcmd_AXE_CheckHitAndCompDir(int argc, char **argv, int bank);                      // CT3 0x018dd760
int ssXcmd_AXE_ThrowEnd(int argc, char **argv, int bank);                                // CT3 0x018dd6b0
int ssXcmd_ForceIntoSearchMode(int argc, char **argv, int bank);                         // CT3 0x018de5e0
int ssXcmd_MonsterSetDraw(int argc, char **argv, int bank);                              // CT3 0x018de580
int wideXcmd_GlobalSetEnable(int argc, char **argv, int bank);                           // CT3 0x018dd590
int ssXcmd_CheckAtkHitALYSSA(int argc, char **argv, int bank);                           // CT3 0x018df6c0
int ssXcmd_GetMonAImode(int argc, char **argv, int bank);                                // CT3 0x018de4e0
int ssXcmd_SetAdvantageousTime(int argc, char **argv, int bank);                         // CT3 0x018de4a0
