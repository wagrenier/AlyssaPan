// game/tnXcmd.h -- CT3 game/tnXcmd.c: the declarations other TUs call (ssGrpPortalCheck.c, 2026-11-11).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int tnXcmd_Probability(int argc, char **argv, int bank);                                 // CT3 0x018f3eb0
int tnXcmd_PlayerDamage(int argc, char **argv, int bank);                                // CT3 0x018f3e80
int tnXcmd_PlayerControllStart(int argc, char **argv, int bank);                         // CT3 0x018f3e60
int tnXcmd_PlayerControllStop(int argc, char **argv, int bank);                          // CT3 0x018f3e40
int tnXcmd_MonsterControllStart(int argc, char **argv, int bank);                        // CT3 0x018f3e20
int tnXcmd_MonsterControllStop(int argc, char **argv, int bank);                         // CT3 0x018f3e00
int tnXcmd_LeverClattering(int argc, char **argv, int bank);                             // CT3 0x018f3cb0
int tnXcmd_LeverClatteringInit(int argc, char **argv, int bank);                         // CT3 0x018f3dd0
