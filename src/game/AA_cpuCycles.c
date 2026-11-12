// game/AA_cpuCycles.c -- Clock Tower 3: the movie player's audio / video clock (SCE's ezmpegstr sample code,
// AA_-prefixed). Written 2026-11-12, all 4 functions (0x018c0310..0x018c0440, symtab locals lastVideo /
// lastAudio).
//
// Each stream reports the pts it has just presented (the display from its vblank handler, the sound from
// the IOP ring's stamps); the skew extrapolates both from the cycle counter and subtracts them.
//
// PORT: Count comes from the host clock (GetCop0, eekernel.cpp), at the console's rate, so the 90 kHz
// conversion holds unchanged.

#include "game/AA_cpuCycles.h"

#include "sdk/eekernel.h"

#define AA_COP0_COUNT 9 /* mfc0 v0, $9 */

static AA_SYNC_CLOCK lastVideo; /* 0x0193f710 */
static AA_SYNC_CLOCK lastAudio; /* 0x0193f720 */

// Cycles since `since`, to pts ticks as the ROM does it: the 32-bit difference as a signed int, through a
// float (cvt.s.w, then fptodp), times AA_PTS_PER_CYCLE in double, truncated to 64 bits (__fixdfdi).
static long long AA_cyclesToPts(u_int since)
{
    return (long long)((double)(float)(int)(AA_cpuCycles() - since) * AA_PTS_PER_CYCLE);
}

// CT3 0x018c0310
u_int AA_cpuCycles(void)
{
    return GetCop0(AA_COP0_COUNT);
}

// CT3 0x018c0320
int AA_syncGetSkew(void)
{
    long long audio = AA_cyclesToPts(lastAudio.cycles) + lastAudio.pts;
    long long video = AA_cyclesToPts(lastVideo.cycles) + lastVideo.pts;

    return (int)(audio - video); /* dsll32 / dsra32: the low word, sign-extended */
}

// CT3 0x018c03c0
void AA_syncClockInVideo(u_long pts)
{
    if ((long long)pts >= 0) {
        lastVideo.cycles = AA_cpuCycles();
        lastVideo.pts = (long long)pts;
    }
}

// CT3 0x018c0400
void AA_syncClockInAudio(u_long pts)
{
    if ((long long)pts >= 0) {
        lastAudio.cycles = AA_cpuCycles();
        lastAudio.pts = (long long)pts;
    }
}
