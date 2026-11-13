// game/RotateCamera.h -- CT3 game/RotateCamera.c: the two x commands (written 2026-11-13).
// Confirmed by the TU: (argc, argv, bank), answering XCMD_WAIT / XCMD_DONE.
#pragma once

#include "sdk/scetypes.h"

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int LockerCamera(int argc, char **argv, int bank);                                       // CT3 0x018d9b50
int RotateCamera(int argc, char **argv, int bank);                                       // CT3 0x018d9a40
