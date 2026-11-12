// game/AA_scTag2.c -- CT3 0x018c1340..0x018c1680: the movie ring's DMA helpers (SCE's ezmpegstr vibuf.c)
// and two of videodec.c's, AA_-prefixed. Reconstructed 2026-11-12, all 7 functions.
//
// PORT: no DMAC runs on the host. The CHCR writes land in the register shims (sdk/eeregs.h) that
// AA_viBuf.c's host IPU stand-in reads, and DMA addresses live in the ring's host DMA space (AA_viBuf.h
// VIBUF_DMA_DATA / VIBUF_DMA_TAG), so the ROM's arithmetic on them runs unchanged.

#include "game/AA_scTag2.h"

#include "game/AA_cpuCycles.h"
#include "game/AA_mpegTS.h"
#include "game/AA_viBuf.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"

#include <string.h>

// CT3 0x018c1340
void AA_scTag2(void *tag, u_int addr, int id, int qwc)
{
    *(u_long *)tag = (u_long)(u_int)qwc | (u_long)addr << DMA_TAG_ADDR_SHIFT
                     | (u_long)(u_int)id << DMA_TAG_ID_SHIFT;
}

// CT3 0x018c1370
void AA_setD4_CHCR(u_int chcr)
{
    DIntr();
    REG_DMAC_ENABLEW = REG_DMAC_ENABLER | D_ENABLE_CPND;
    REG_DMAC_4_IPU_TO_CHCR = chcr;
    REG_DMAC_ENABLEW = REG_DMAC_ENABLER & ~D_ENABLE_CPND;
    EIntr();
}

// CT3 0x018c13e0
void AA_setD3_CHCR(u_int chcr)
{
    DIntr();
    REG_DMAC_ENABLEW = REG_DMAC_ENABLER | D_ENABLE_CPND;
    REG_DMAC_3_IPU_FROM_CHCR = chcr;
    REG_DMAC_ENABLEW = REG_DMAC_ENABLER & ~D_ENABLE_CPND;
    EIntr();
}

// CT3 0x018c1450: the ROM compares MADR with the physical end of the tag list ((tag + (n + 1) * 16) &
// 0x0fffffff) and divides its offset from data; on the host both are the ring's DMA-space bases.
int AA_getFIFOindex(ViBuf *f, u_int madr)
{
    u_int tagEnd = (VIBUF_DMA_TAG + (f->n + 1) * VIBUF_TAG_SIZE) & EE_PHYS_MASK;

    if (madr == tagEnd)
        return 0;
    return (int)((madr - VIBUF_DMA_DATA) >> VIBUF_ELM_SHIFT);
}

// CT3 0x018c1490
int AA_IsPtsInRegion(int pos, int start, int len, int size)
{
    return (pos + size - start) % size < len;
}

// CT3 0x018c14b0
int AA_shouldSkip(VideoDec *vd)
{
    int i;

    vd->skew[vd->skewIndex] = AA_syncGetSkew();
    vd->skewIndex = (vd->skewIndex + 1) % VD_SKEWS;
    for (i = 0; i < VD_SKEWS; i++) {
        if (vd->skew[i] < 1)
            return 0;
    }
    return 1;
}

// CT3 0x018c1540
int AA_cpy2area(u_char *pd0, int d0, u_char *pd1, int d1, const u_char *ps0, int s0, const u_char *ps1,
                int s1)
{
    if (d0 + d1 < s0 + s1)
        return 0;
    if (s0 < d0) {
        int room = d0 - s0; /* what pd0 has left after ps0 */

        memcpy(pd0, ps0, s0);
        if (s1 < room) {
            memcpy(pd0 + s0, ps1, s1);
        } else {
            memcpy(pd0 + s0, ps1, room);
            memcpy(pd1, ps1 + room, s1 - room);
        }
    } else {
        memcpy(pd0, ps0, d0);
        memcpy(pd1, ps0 + d0, s0 - d0);
        memcpy(pd1 + (s0 - d0), ps1, s1);
    }
    return s0 + s1;
}
