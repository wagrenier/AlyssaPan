// game/AA_cpuCycles.h -- CT3 game/AA_cpuCycles.c: the declarations other TUs call (game/movie.c first,
// 2026-11-10). Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// The EE's cycle counter (8 bytes in the ROM: COP0 Count, 294.912 MHz). readMpeg busy-waits on it.
u_int AA_cpuCycles(void);                                                                // CT3 0x018c0310
