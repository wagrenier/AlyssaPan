// ga/gaInitMeter.h -- CT3 ga/gaInitMeter.c: the ga library's frame meters, read off EE timer T1 (2026-11-09).
// The object is GCC-built (its symtab run carries `__gnu_compiled_c` / `gcc2_compiled.`), like
// ga/gaInitInterrupt.c beside it, whose VSync callback zeroes T1 at every waited-for VSync.
#pragma once

#include "sdk/scetypes.h"

// T1_MODE as gaInitMeter writes it: CLKS = BUSCLK / 256 (bits 0..1 = 2), CUE (bit 7) counting.
#define GA_METER_T1_CLKS_BUSCLK_256 0x02
#define GA_METER_T1_CUE 0x80
#define GA_METER_T1_MODE (GA_METER_T1_CUE | GA_METER_T1_CLKS_BUSCLK_256)

// COP0 register 9, Count: the EE core's cycle counter.
#define GA_COP0_COUNT 9

extern int ga_range_timer; /* data 0x0192e798: T1 counts in one whole frame (VSync to VSync), the meters' 100% */
extern int cpu_time_count; /* data 0x0192e79c: T1 when gaEndCpuMeter ran (the CPU's share of the frame) */
extern int gs_time_count;  /* data 0x0192e7a0: T1 when gaEndGSMeter ran (the GS's share of the frame) */

void gaInitMeter(void);       // CT3 0x018a8810
void gaStartMeter(void);      // CT3 0x018a8878
void gaEndCpuMeter(void);     // CT3 0x018a8888
void gaEndGSMeter(void);      // CT3 0x018a88a0
int gaGetCpuTimer(void);      // CT3 0x018a88b8: T1 now
u_int gaGetEEcoreTimer(void); // CT3 0x018a88c8: COP0 Count now
