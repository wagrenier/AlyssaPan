// ga/gaInitInterrupt.c -- Clock Tower 3, the ga library's VSync (CT3 0x018a86a8..0x018a8810).
// Object extent from the symtab: its local run (.data 0x0192e790, .text 0x018a86a8, gaVsyncCallBack) and
// `gcc2_compiled.` -- a GCC object, like the rest of the prebuilt ga library. Reconstructed 2026-11-09.

#include "ga/gaInitInterrupt.h"

#include "alyssapan/rendering/alyssapan_drawga.h"

#include "game/unassigned_data.h"
#include "sdk/eeregs.h"
#include "sdk/libgraph.h"

volatile int ga_main_timer;                      /* bss  0x01c8d730 (common) */
volatile int ga_down_timer;                      /* bss  0x01c8d734 (common) */
volatile int wait_vsync_state = WAIT_VSYNC_IDLE; /* data 0x0192e790 */

// CT3 0x018a86e0 (static): the VSync callback. A waiting gaWaitVSync is released and T1 restarts from
// this VSync (gaInitMeter's CPU meter counts from it); a VSync nobody waited for counts as a dropped one.
static int gaVsyncCallBack(int)
{
    if (wait_vsync_state == WAIT_VSYNC_WAITING) {
        REG_RCNT1_COUNT = 0;
        wait_vsync_state = WAIT_VSYNC_IDLE;
    } else {
        ga_down_timer = ga_down_timer + 1;
    }
    ga_main_timer = ga_main_timer + 1;
    return 0;
}

// CT3 0x018a86a8
void gaInitInterrupt(void)
{
    ga_main_timer = 0;
    ga_down_timer = 0;
    sceGsSyncVCallback(gaVsyncCallBack);
}

// CT3 0x018a8740: waits for the next VSync, then shows the frame just drawn (DISPFB's FBP = the draw
// buffer) and swaps the two buffer addresses. The ROM reads no argument.
int gaWaitVSync(void)
{
    int field;
    int draw;

    wait_vsync_state = WAIT_VSYNC_WAITING;
    field = (int)((REG_GS_CSR >> GS_CSR_FIELD_SHIFT) & 1);
    while (wait_vsync_state != WAIT_VSYNC_IDLE)
        ;

    // PORT: DISPFB is never sent to a GS (sceGsPutDispEnv is empty): the host draw module shows the frame
    // drawn since the last swap.
    AlyssaPan_DrawGaSwap();
    draw = ga_draw_fbuf_address;
    *(u_long *)&ga_disp.dispfb = (*(u_long *)&ga_disp.dispfb & ~GS_DISPFB_FBP_MASK)
                               | (u_long)((u_int)draw >> GA_FBUF_ADDRESS_TO_FBP_SHIFT);
    ga_draw_fbuf_address = ga_disp_fbuf_address;
    ga_disp_fbuf_address = draw;
    sceGsPutDispEnv(&ga_disp);
    return field;
}
