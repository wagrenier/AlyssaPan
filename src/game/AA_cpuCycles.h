// game/AA_cpuCycles.h -- CT3 game/AA_cpuCycles.c: the movie player's audio / video clock (SCE's ezmpegstr
// sample code, AA_-prefixed). Written 2026-11-12, all 4 functions.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// EE cycles (COP0 Count, 294.912 MHz) to 90 kHz pts ticks: 1.25 / 4096 (the ROM's double 0x3f34000000000000;
// 294,912,000 * 1.25 / 4096 = 90,000).
#define AA_PTS_PER_CYCLE (1.25 / 4096.0)

// The last time stamp each stream reported and the cycle count when it did (lastVideo 0x0193f710, lastAudio
// 0x0193f720, 16 bytes each in the symtab: the pts dword at +0, the count word at +8, 4 bytes of padding).
struct AA_SYNC_CLOCK {
    long long pts;          /* +0x00: sd -- signed: a negative pts is never recorded */
    u_int cycles;           /* +0x08: sw -- AA_cpuCycles() when pts arrived */
};
static_assert(sizeof(AA_SYNC_CLOCK) == 0x10, "AA_SYNC_CLOCK");
static_assert(offsetof(AA_SYNC_CLOCK, pts) == 0x00, "AA_SYNC_CLOCK.pts");
static_assert(offsetof(AA_SYNC_CLOCK, cycles) == 0x08, "AA_SYNC_CLOCK.cycles");

// The EE's cycle counter (COP0 Count, 294.912 MHz, wraps at 32 bits). readMpeg busy-waits on it.
u_int AA_cpuCycles(void);                                                                // CT3 0x018c0310
// The audio clock minus the video clock now, in 90 kHz pts ticks: each clock is its last pts plus the
// time since it came (AA_PTS_PER_CYCLE). Positive: the picture lags the sound (AA_shouldSkip drops pictures
// while all its answers are positive); shouldDelay holds a picture after six answers at or under -6000.
int AA_syncGetSkew(void);                                                                // CT3 0x018c0320
// The display tells the clock which picture went out (AA_vblankHandler: the even field of `pts`).
// A negative pts (no stamp) is ignored.
void AA_syncClockInVideo(u_long pts);                                                    // CT3 0x018c03c0
// The sound tells the clock the pts it plays (AA_audioDecSendToIOP, from the IOP ring's stamps).
void AA_syncClockInAudio(u_long pts);                                                    // CT3 0x018c0400
