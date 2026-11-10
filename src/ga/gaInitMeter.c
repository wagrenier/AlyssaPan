// ga/gaInitMeter.c -- Clock Tower 3, the ga library's frame meters (CT3 0x018a8810..0x018a88e0).
// Object extent from the symtab: its local run (.data 0x0192e798, .text 0x018a8810, `__gnu_compiled_c`,
// `gcc2_compiled.`) -- a GCC object of the prebuilt ga library, after ga/gaInitInterrupt.c. Reconstructed
// 2026-11-09. T1 restarts at each waited-for VSync (gaInitInterrupt.c's callback), so a meter read is
// the time since the frame began; ga_range_timer is one frame's worth to scale it by.

#include "ga/gaInitMeter.h"

#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/libgraph.h"

int ga_range_timer; /* data 0x0192e798 */
int cpu_time_count; /* data 0x0192e79c */
int gs_time_count;  /* data 0x0192e7a0 */

// CT3 0x018a8810: sets T1 to BUSCLK / 256 and measures one whole frame with it, VSync to VSync.
void gaInitMeter(void)
{
    REG_RCNT1_MODE = GA_METER_T1_MODE;
    ga_range_timer = 0;
    sceGsSyncV(0);
    REG_RCNT1_COUNT = 0;
    sceGsSyncV(0);
    ga_range_timer = (int)REG_RCNT1_COUNT;
}

// CT3 0x018a8878
void gaStartMeter(void)
{
    REG_RCNT1_COUNT = 0;
}

// CT3 0x018a8888
void gaEndCpuMeter(void)
{
    cpu_time_count = (int)REG_RCNT1_COUNT;
}

// CT3 0x018a88a0
void gaEndGSMeter(void)
{
    gs_time_count = (int)REG_RCNT1_COUNT;
}

// CT3 0x018a88b8
int gaGetCpuTimer(void)
{
    return (int)REG_RCNT1_COUNT;
}

// CT3 0x018a88c8: a tail jump to GetCop0(9); the ROM's prototype says void, its v0 is Count.
u_int gaGetEEcoreTimer(void)
{
    return GetCop0(GA_COP0_COUNT);
}
