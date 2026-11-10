// ga/gaDmaSync_VIF1.c -- Clock Tower 3, the ga library's hand-written VIF1 / GIF DMA kicks and syncs and the
// BGT ring's send (CT3 0x018a1738..0x018a1948). Reconstructed 2026-11-10 from the disassembly.
//
// Object extent from the symtab: a .text section symbol at 0x018a1738 (no gcc2_compiled.: an asm object)
// up to gaVifInit's object (0x018a1948). It opens with `_DmaVIF1_Sync_error` (0x018a1738), a lone
// `j gaDMAStopMessage_VIF1` nothing branches to (gaDmaSync_VIF1 jumps to the stop message itself), and
// gaDmaSync_GIF's `_gaDmaSync_GIF_error` is its last instruction.
//
// The ROM pokes the channel registers by address (D1 0x10009000, D2 0x1000a000, D8 0x1000d000); here they
// are libdma's channel blocks (sceDmaGetChan: the same fixed blocks gaDma.c's dmaVif1 / dmaGIF point at).
// Host: the DMAC finishes every transfer at once (sdk/libdma.cpp) and nothing reads VIF1 / GIF packets --
// the frame reaches the renderer through the draw modules -- so a kick writes the ROM's MADR / QWC / TADR
// and its CHCR with STR already clear (gaDmaDoneAtOnce), and the syncs return on their first poll.

#include "ga/gaDmaSync_VIF1.h"

#include "ga/gaInitDrawObject.h"
#include "ga/wait.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/libdma.h"

#include <string.h>

/* CHCR as the ROM writes it to start VIF1: DIR = from memory, MOD normal / chain, STR. */
#define D_CHCR_VIF1_NORMAL_GO (D_CHCR_STR_M | D_CHCR_DIR_M)                    /* 0x101 */
#define D_CHCR_VIF1_CHAIN_GO (D_CHCR_STR_M | D_CHCR_MOD_CHAIN | D_CHCR_DIR_M) /* 0x105 */

/* GIF_STAT: APATH (the path the GIF serves) and FQC (qwords in its FIFO); both 0 = the GIF is idle. */
#define GIF_STAT_APATH_M 0x00000c00
#define GIF_STAT_FQC_M 0x1f000000

#define QWORD_SIZE 16

// The ROM hands the DMAC a scratchpad address as `addr + 0x10000000` (0x7000xxxx -> 0x8000xxxx: bit 31 =
// SPR); a host pointer into the scratchpad buffer needs no translation (as gaDma.c's gaDmaSprAddr).
#define gaDmaSprAddr(addr) (addr)

// Host: the transfer the ROM starts with this CHCR is already over, so STR reads clear.
#define gaDmaDoneAtOnce(chcr) ((chcr) & ~D_CHCR_STR_M)

#define SPR_WORK ((GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET))

static int gaIsScratchpad(const void *addr)
{
    // The ROM's test: addr - 0x70000000 >= 0 (signed), i.e. the scratchpad (or above).
    const u_char *p = (const u_char *)addr;
    return p >= g_eeScratchpad && p < g_eeScratchpad + SCRATCHPAD_SIZE;
}

// CT3 0x018a1740: wait for VIF1's STR to drop; after GA_DMA_SYNC_TIMEOUT polls, report the caller.
void gaDmaSync_VIF1(void)
{
    sceDmaChan *d1 = sceDmaGetChan(SCE_DMA_VIF1);
    int count = GA_DMA_SYNC_TIMEOUT;

    do {
        if (--count == 0) {
            gaDMAStopMessage_VIF1(EE_RETURN_ADDRESS());
            return;
        }
    } while (d1->chcr.bits & D_CHCR_STR_M);
}

// CT3 0x018a1778: `qwc` quadwords at `addr` to VIF1, normal mode.
void gaDmaSendN_VIF1(void *addr, int qwc)
{
    sceDmaChan *d1 = sceDmaGetChan(SCE_DMA_VIF1);

    d1->madr = gaDmaSprAddr(addr);
    d1->qwc = (u_int)qwc;
    d1->chcr.bits = gaDmaDoneAtOnce(D_CHCR_VIF1_NORMAL_GO);
}

// CT3 0x018a17b8: a DMA chain at `tag` to VIF1.
void gaDmaSendS_VIF1(void *tag)
{
    sceDmaChan *d1 = sceDmaGetChan(SCE_DMA_VIF1);

    d1->tadr = (sceDmaTag *)gaDmaSprAddr(tag);
    d1->qwc = 0;
    d1->chcr.bits = gaDmaDoneAtOnce(D_CHCR_VIF1_CHAIN_GO);
}

// CT3 0x018a17f8: send a packet into the BGT ring (fromSPR writes it into the MFIFO VIF1 drains). A packet in
// memory is first copied into the scratchpad packet area, and the area flips to its other half so the next
// packet can be built while this one goes. Then: wait for fromSPR, set SADR / QWC, wait until the ring has
// more than the packet's room (or is empty: MADR == VIF1's TADR), start fromSPR.
void gaBGT_DmaSendN_fromSPR_VIF1(void *addr, int qwc)
{
    sceDmaChan *d8 = sceDmaGetChan(SCE_DMA_fromSPR);
    GA_SPR_WORK *spr = SPR_WORK;
    int room;

    if (!gaIsScratchpad(addr)) {
        u_char *packet = (u_char *)SCRATCHPAD(spr->bgtPacketEE - SCRATCHPAD_EE_BASE);

        memcpy(packet, addr, (size_t)qwc * QWORD_SIZE);
        addr = packet;
        spr->bgtPacketEE = spr->bgtPacketEE == SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET
                               ? SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET_B
                               : SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET;
    }

    while (d8->chcr.bits & D_CHCR_STR_M)
        ;
    d8->sadr = gaDmaSprAddr(addr);
    d8->qwc = (u_int)qwc;
    for (;;) {
        u_int head = REG_DMAC_8_SPR_FROM_MADR;
        u_int tail = REG_DMAC_1_VIF1_TADR;

        if (head == tail)
            break;
        room = (int)((tail - head) & REG_DMAC_RBSR) - qwc * QWORD_SIZE;
        if (room > 0)
            break;
    }
    d8->chcr.bits = gaDmaDoneAtOnce(d8->chcr.bits | D_CHCR_STR_M);
}

// CT3 0x018a18e8: wait for the GIF channel's STR to drop, then for the GIF itself to go idle (no active path,
// empty FIFO); one count of GA_DMA_SYNC_TIMEOUT polls covers both waits.
void gaDmaSync_GIF(void)
{
    sceDmaChan *d2 = sceDmaGetChan(SCE_DMA_GIF);
    int count = GA_DMA_SYNC_TIMEOUT;

    do {
        if (--count == 0)
            goto error;
    } while (d2->chcr.bits & D_CHCR_STR_M);

    for (;;) {
        if (--count == 0)
            break;
        if ((REG_GIF_STAT & (GIF_STAT_FQC_M | GIF_STAT_APATH_M)) == 0)
            return;
    }
error: /* _gaDmaSync_GIF_error */
    gaDMAStopMessage_GIF(EE_RETURN_ADDRESS());
}
