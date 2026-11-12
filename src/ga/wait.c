// ga/wait.c -- Clock Tower 3 module ga/wait.c (0x018a85c8..0x018a86a8), read from the disassembly
// (mipsdis.py 018a85c8 018a86a8), 2026-11-12.
//
// The ga library's debug object, as the release build left it: wait, a busy delay, then seventeen dumps
// and two DMA stop messages whose bodies (and every call to the dumps) were compiled out, each a bare
// `jr ra; nop`. The stop messages are still reached: the DMA syncs in gaDmaSync_VIF1.c tail-jump to them
// when a channel never drops STR, and several objects end in a stray `j gaDMAStopMessage_VIF1` the linker
// left as padding (hazards.md). In the ROM a timeout therefore says nothing and the game goes on.

#include "ga/wait.h"

#define WAIT_OUTER 500    // sltiu a0, 0x1f4
#define WAIT_INNER 10000  // sltiu v1, 0x2710: the inner count runs 1..9,999

// CT3 0x018a85c8: spin WAIT_OUTER x (WAIT_INNER - 1) empty iterations. The four nops in each loop are
// the compiler's padding against the R5900 short-loop erratum (a loop body under six instructions), not
// source. Volatile keeps the host from deleting the delay; nothing in the ROM calls it.
void wait(void)
{
    volatile u_int i;
    u_int j;

    for (j = 0; j < WAIT_OUTER; j++) {
        for (i = 1; i < WAIT_INNER; i++) {
        }
    }
}

// CT3 0x018a8610 .. 0x018a8688, 0x018a86a0: the dumps, empty in the release ROM.
void gaDump128(void) {}
void gaDump64(void) {}
void gaDump32(void) {}
void gaDumpF4(void) {}
void gaDumpF3I1(void) {}
void gaDumpVector(void) {}
void gaDumpIVector(void) {}
void gaDumpMatrix(void) {}
void gaDumpDmaEnv(void) {}
void gaDumpStatus_VIF0(void) {}
void gaDumpStatus_VIF1(void) {}
void gaDumpStatus_GIF(void) {}
void gaDumpVu0Regf(void) {}
void gaDumpVu1Regs(void) {}
void gaDumpVu1Regi(void) {}
void gaDumpVu1Regf(void) {}
void gaDebugDump(void) {}

// CT3 0x018a8690: a VIF1 DMA wait timed out (gaDmaSync_VIF1, gaDmaSendN_GIF ..); ra is the waiter's
// return address. Empty in the release ROM.
void gaDMAStopMessage_VIF1(void *ra)
{
    (void)ra;
}

// CT3 0x018a8698: a GIF DMA wait timed out (gaDmaSync_VIF1.c); empty in the release ROM.
void gaDMAStopMessage_GIF(void *ra)
{
    (void)ra;
}
