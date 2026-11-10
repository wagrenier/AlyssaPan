// game/CpDivideEECom.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/CpDivideEECom.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018d1e70
// The ROM answers &gEEStat (bss 0x01943a40); until this TU is written the stub answers a zeroed record of
// its own, so FlProc's read of +0x50 (the IOP not busy) holds (game/ArtoolDebugOpen.c, 2026-11-10).
EE_IOP_STAT * CpGetEEIOPStatus(void)
{
    static EE_IOP_STAT s_idle;
    ALYSSAPAN_STUB("CpGetEEIOPStatus", 0x018d1e70);
    return &s_idle;
}
