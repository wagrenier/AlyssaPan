// game/tnXcmd.h -- CT3 game/tnXcmd.c (0x018f3cb0..0x018f3f60): the tn x commands after xcmd_tn.c --
// the lever struggle, the player / monster control locks, scripted damage, a random event (2026-11-13).
#pragma once

#include "sdk/scetypes.h"
#include "sdk/libpad.h"

#define LEVER_CLATTER_BUTTON SCE_PADRright  /* 0x20 in pad_trg_button[0]: each press fills the lever meter */
#define PROBABILITY_RANGE 100               /* tnXcmd_Probability: tnRand(100) against argv[0] percent */

// The x commands (ssXcmd_functions slots 5..12, ssTrigExecuteActCmd's command 0x514): the table's SS_XCMD
// shape (xcmd_tn.h); each answers XCMD_DONE, LeverClattering XCMD_WAIT until the struggle ends.
int tnXcmd_Probability(int argc, char **argv, int bank);                                 // CT3 0x018f3eb0
int tnXcmd_PlayerDamage(int argc, char **argv, int bank);                                // CT3 0x018f3e80
int tnXcmd_PlayerControllStart(int argc, char **argv, int bank);                         // CT3 0x018f3e60
int tnXcmd_PlayerControllStop(int argc, char **argv, int bank);                          // CT3 0x018f3e40
int tnXcmd_MonsterControllStart(int argc, char **argv, int bank);                        // CT3 0x018f3e20
int tnXcmd_MonsterControllStop(int argc, char **argv, int bank);                         // CT3 0x018f3e00
int tnXcmd_LeverClattering(int argc, char **argv, int bank);                             // CT3 0x018f3cb0
int tnXcmd_LeverClatteringInit(int argc, char **argv, int bank);                         // CT3 0x018f3dd0
