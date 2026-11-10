// game/AlignedNum.h -- CT3 game/AlignedNum.c: rounding up, a busy wait, and loading one IOP module
// (CT3 0x0189f620..0x0189f718). Reconstructed 2026-11-09.
#pragma once

#include "sdk/scetypes.h"

// Busy-wait count IopLoadModule spins between attempts while the IOP refuses a module (0x0189f6f4).
#define IOP_LOAD_RETRY_WAIT 0x4000000

int AlignedNum(int num, int align);                       // CT3 0x0189f620
void WaitLoop(int count);                                 // CT3 0x0189f650
void IopLoadModule(const char *dir, const char *name);    // CT3 0x0189f6a0
