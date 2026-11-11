// game/asXcmd.h -- CT3 game/asXcmd.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void asXcmd_AddForLockToTargChara(int a0, int a1, int a2);                               // CT3 0x018db340

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int asXcmd_ShadowOrderBW(int argc, char **argv, int bank);                               // CT3 0x018dc1b0
int asXcmd_PlayMotionByNowMotion(int argc, char **argv, int bank);                       // CT3 0x018dc050
int asXcmd_CameraGuriGuriSetting(int argc, char **argv, int bank);                       // CT3 0x018dbf40
int asXcmd_SeekBarSetting(int argc, char **argv, int bank);                              // CT3 0x018dbd80
int asXcmd_CharacterAutoAiming(int argc, char **argv, int bank);                         // CT3 0x018dbc20
int asXcmd_I_have_x_Arrows(int argc, char **argv, int bank);                             // CT3 0x018dba30
int asXcmd_ShootArrow(int argc, char **argv, int bank);                                  // CT3 0x018dba20
int asXcmd_AttachObject(int argc, char **argv, int bank);                                // CT3 0x018db880
int asXcmd_CameraQuake(int argc, char **argv, int bank);                                 // CT3 0x018db7b0
int asXcmd_SlowMotion(int argc, char **argv, int bank);                                  // CT3 0x018db770
int asXcmd_SetHitPoint(int argc, char **argv, int bank);                                 // CT3 0x018db580
int asXcmd_SettingCameraReset(int argc, char **argv, int bank);                          // CT3 0x018db510
int asXcmd_I_have_x_Mothes(int argc, char **argv, int bank);                             // CT3 0x018db160
int asXcmd_SetMoth(int argc, char **argv, int bank);                                     // CT3 0x018db000
int asXcmd_ForceOperateAnimation(int argc, char **argv, int bank);                       // CT3 0x018daf80
int asXcmd_ForceOperateMirroring(int argc, char **argv, int bank);                       // CT3 0x018daf30
int asXcmd_ALYSSA_ComeHere(int argc, char **argv, int bank);                             // CT3 0x018dae40
int asXcmd_DistanceBetweenChar(int argc, char **argv, int bank);                         // CT3 0x018dad50
int asXcmd_PanicSet(int argc, char **argv, int bank);                                    // CT3 0x018dab60
