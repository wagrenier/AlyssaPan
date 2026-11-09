// ga/gaInitInterrupt.h -- CT3 ga/gaInitInterrupt.c: the ga library's VSync -- the frame counters the
// VSync callback keeps, and the wait that swaps the display and draw frame buffers (2026-11-09).
// The object is GCC-built (its symtab run carries `gcc2_compiled.`), unlike the CodeWarrior game code.
#pragma once

#include "sdk/scetypes.h"

// wait_vsync_state: gaWaitVSync raises it, the next VSync callback clears it (and zeroes T1).
#define WAIT_VSYNC_IDLE 0
#define WAIT_VSYNC_WAITING 1

// GS CSR FIELD (bit 13): the field being displayed, 1 = odd.
#define GS_CSR_FIELD_SHIFT 13

// DISPFB's FBP (bits 0..8) is in 2048-word pages; ga_*_fbuf_address are word addresses.
#define GS_DISPFB_FBP_MASK 0x1ffull
#define GA_FBUF_ADDRESS_TO_FBP_SHIFT 11

extern volatile int ga_main_timer;    /* bss  0x01c8d730: VSyncs since gaInitInterrupt */
extern volatile int ga_down_timer;    /* bss  0x01c8d734: VSyncs no gaWaitVSync was waiting for */
extern volatile int wait_vsync_state; /* data 0x0192e790: WAIT_VSYNC_* */

void gaInitInterrupt(void); // CT3 0x018a86a8
int gaWaitVSync(void);      // CT3 0x018a8740: answers the GS CSR FIELD bit read on entry
