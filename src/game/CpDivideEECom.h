// game/CpDivideEECom.h -- CT3 game/CpDivideEECom.c (0x018d1cb0..): the EE <-> IOP status exchange. The
// declarations other TUs call (game/ArtoolDebugOpen.c first, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// gEEStat (bss 0x01943a40, 128 bytes): the status the IOP writes back. Only +0x50 is read so far (FlProc:
// no disc read while it is non-zero). layout: ee.
typedef struct EE_IOP_STAT {
    u_char unk00[0x50];
    int iopBusy;            // +0x50 != 0: the IOP is using the drive (FlProc waits)
    u_char unk54[0x2c];
} EE_IOP_STAT;
static_assert(sizeof(EE_IOP_STAT) == 0x80, "EE_IOP_STAT is 0x80 bytes");

EE_IOP_STAT *CpGetEEIOPStatus(void);    // CT3 0x018d1e70: &gEEStat (its uncached alias)
