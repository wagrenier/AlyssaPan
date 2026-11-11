// game/RotateCamera.h -- CT3 game/RotateCamera.c: the declarations other TUs call (ssGrpPortalCheck.c, 2026-11-11).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int LockerCamera(int argc, char **argv, int bank);                                       // CT3 0x018d9b50
int RotateCamera(int argc, char **argv, int bank);                                       // CT3 0x018d9a40
