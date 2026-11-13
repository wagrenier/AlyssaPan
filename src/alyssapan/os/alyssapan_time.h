// alyssapan/os/alyssapan_time.h -- a seam game code includes (cpp-port.md "Scope"): where a spin loop of
// the ROM would hold a host core, rx sleeps instead.
#pragma once

#include <stdint.h>

void AlyssaPan_Sleep(unsigned int milliseconds);
void AlyssaPan_SleepNs(uint64_t nanoseconds);

// The game's wait for its field (gaWaitVSync's spin on the VSync callback's flag) begins / ends: the profiler
// counts the time between as waiting, not work (alyssapan/core/profiler.h). One thread waits at a time.
void AlyssaPan_FieldWaitBegin(void);
void AlyssaPan_FieldWaitEnd(void);
