// game/PicPaz.h -- CT3 game/PicPaz.c: the declarations other TUs call (ssGrpPortalCheck.c, 2026-11-11).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int PicPaz(int argc, char **argv, int bank);                                             // CT3 0x018da090
int aaXcmd_MovieFedeOutCancel(int argc, char **argv, int bank);                          // CT3 0x018da0a0
int aaXcmd_SceneEndBank(int argc, char **argv, int bank);                                // CT3 0x018da900
int aaXcmd_McWorkAlloc(int argc, char **argv, int bank);                                 // CT3 0x018da210
int aaXcmd_CheckMCFile(int argc, char **argv, int bank);                                 // CT3 0x018da530
int aaXcmd_MCFileList(int argc, char **argv, int bank);                                  // CT3 0x018da420
int aaXcmd_McCursorUpDate(int argc, char **argv, int bank);                              // CT3 0x018da280
