// game/AA_cpuCycles.h -- CT3 game/AA_cpuCycles.c: the declarations other TUs call (game/movie.c first,
// 2026-11-10). Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// The EE's cycle counter (8 bytes in the ROM: COP0 Count, 294.912 MHz). readMpeg busy-waits on it.
u_int AA_cpuCycles(void);                                                                // CT3 0x018c0310
// The picture's lead over the audio clock (shouldDelay's use, 2026-11-12: six answers at or under -6000 in a
// row make the display hold a picture). AA_cpuCycles.c confirms the sign and unit.
int AA_syncGetSkew(void);                                                                // CT3 0x018c0320
// The display tells the clock which picture went out (AA_vblankHandler: the even field of `pts`).
void AA_syncClockInVideo(u_long pts);                                                    // CT3 0x018c03c0
// The sound tells the clock the pts it plays (AA_audioDecSendToIOP, from the IOP ring's stamps).
void AA_syncClockInAudio(u_long pts);                                                    // CT3 0x018c0400
