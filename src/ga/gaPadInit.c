// ga/gaPadInit.c -- Clock Tower 3, the ga library's pad start-up (CT3 0x018a1130..0x018a1188).
// Reconstructed 2026-11-10. The symtab puts this function's two DMA buffers in the same unbroken local
// run as pad_button_read's statics (indices 1607..1618, closed by the .text section symbol of the GCC
// object at 0x018a1188), so gaPadInit is most likely the tail of ga/pad_button_read.c's object; it stays
// its own file until the pad_button_read.c session merges them (open-items.md).

#include "ga/gaPadInit.h"

#include "sdk/libpad.h"

#define GA_PAD_MODE_DEFAULT 0 /* scePadInit's mode: the only one libpad defines */
#define GA_PAD_SLOT         0 /* no multitap: slot 0 of each port */

// libpad DMAs each port's state into its buffer: scePadDmaBufferMax qwords, 64-byte aligned on the EE.
alignas(64) static u_long128 pad_dma_buf0[scePadDmaBufferMax]; /* bss 0x0193e580, port 0 */
alignas(64) static u_long128 pad_dma_buf1[scePadDmaBufferMax]; /* bss 0x0193e480, port 1 */

// CT3 0x018a1130: starts libpad and opens both controller ports; 1 when both opened, 0 when either
// failed (port 1 is not tried once port 0 fails). The caller (IOPInit) ignores the result.
int gaPadInit(void)
{
    scePadInit(GA_PAD_MODE_DEFAULT);
    if (scePadPortOpen(GA_PAD_PORT_0, GA_PAD_SLOT, pad_dma_buf0) == 0)
        return 0;
    return scePadPortOpen(GA_PAD_PORT_1, GA_PAD_SLOT, pad_dma_buf1) != 0;
}
