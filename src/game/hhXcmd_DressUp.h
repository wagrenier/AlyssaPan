// game/hhXcmd_DressUp.h -- CT3 game/hhXcmd_DressUp.c: the sound x commands (written 2026-11-11, all 22).
#pragma once

#include "sdk/scetypes.h"

void hhXcmd_InitPlayStopMIDI(void);                                                      // CT3 0x018dc410
// A footstep of bank name (C_0000 Alyssa, E_HA..E_BA the stalkers) on the floor under its character.
int FootSEConversion(const char *name, int reqArg2, int objId, int bank, int posVol);   // CT3 0x018dcc50

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514), the table's SS_XCMD
// shape (xcmd_tn.h): argv[0..argc-1] the script's argument strings, bank the script's bank.
int hhXcmd_LoadSE(int argc, char **argv, int bank);                                      // CT3 0x018dd4d0
int hhXcmd_PurgeSE(int argc, char **argv, int bank);                                     // CT3 0x018dd490
int hhXcmd_PlaySE(int argc, char **argv, int bank);                                      // CT3 0x018dd200
int hhXcmd_VolumeChangeSE(int argc, char **argv, int bank);                              // CT3 0x018dcba0
int hhXcmd_PitchChangeSE(int argc, char **argv, int bank);                               // CT3 0x018dcaf0
int hhXcmd_PanChangeSE(int argc, char **argv, int bank);                                 // CT3 0x018dca40
int hhXcmd_StopSE(int argc, char **argv, int bank);                                      // CT3 0x018dc9e0
int hhXcmd_EffectSE(int argc, char **argv, int bank);                                    // CT3 0x018dc930
int hhXcmd_PlayADS(int argc, char **argv, int bank);                                     // CT3 0x018dc890
int hhXcmd_StopADS(int argc, char **argv, int bank);                                     // CT3 0x018dc7e0
int hhXcmd_VolumeADS(int argc, char **argv, int bank);                                   // CT3 0x018dc810
int hhXcmd_StopSESp1(int argc, char **argv, int bank);                                   // CT3 0x018dc5e0
int hhXcmd_PlaySESp1(int argc, char **argv, int bank);                                   // CT3 0x018dc640
int hhXcmd_PlayMIDI(int argc, char **argv, int bank);                                    // CT3 0x018dc500
int hhXcmd_StopMIDI(int argc, char **argv, int bank);                                    // CT3 0x018dc420
int hhXcmd_VolumeMIDI(int argc, char **argv, int bank);                                  // CT3 0x018dc3a0
int hhXcmd___________Free3(int argc, char **argv, int bank);                             // CT3 0x018dc390
int hhXcmd___________Free4(int argc, char **argv, int bank);                             // CT3 0x018dc380
int hhXcmd___________Free5(int argc, char **argv, int bank);                             // CT3 0x018dc370
int hhXcmd_DressUp(int argc, char **argv, int bank);                                     // CT3 0x018dc330
