// ga/gaDma.c -- Clock Tower 3, the ga library's DMA channel wrappers (CT3 0x018a1188..0x018a1738).
// Object extent from the symtab: its local run (.text 0x018a1188, `gcc2_compiled.`) ends where the next
// object's .text section and `_DmaVIF1_Sync_error` start (0x018a1738) -- a GCC object, like the rest of the
// prebuilt ga library. Globals: common bss 0x01c8d708..0x01c8d730. Reconstructed 2026-11-09.
//
// Host: the DMAC finishes every transfer at once (sdk/libdma.cpp); the scratchpad is a host buffer
// (sdk/eekernel.h SCRATCHPAD), so a scratchpad address here is a host pointer and the ROM's EE -> DMA
// address step (0x7000xxxx -> 0x8000xxxx, "SPR" bit) has nothing left to do: gaDmaSprAddr keeps the step
// visible. D_CTRL / D_ENABLEW are written as the ROM writes them (sdk/eeregs.h host variables).

#include "ga/gaDma.h"

#include "sdk/eekernel.h"
#include "sdk/eeregs.h"

/* D_CTRL bits (EE manual): RELE releases the bus every RCYC cycles; STS / STD the stall source / drain. */
#define D_CTRL_RELE 0x002
#define D_CTRL_STS_STD0_M 0x070 /* STS and STD's low bit: fromSPR clears them before a transfer */
#define D_CTRL_RCYC_M 0x700
/* D_ENABLEW: CPND suspends every channel while the MFIFO is rewired. */
#define D_ENABLEW_CPND 0x10000

#define GA_DMA_ENV_MFD_ON 1 /* ga_dma_env.mfd while the MFIFO is open (the ROM's value) */
#define GA_DMA_ENV_TQWC_DEFAULT 1

sceDmaEnv ga_dma_env;   /* bss 0x01c8d708 */
sceDmaChan *dmaVif0;    /* bss 0x01c8d71c */
sceDmaChan *dmaVif1;    /* bss 0x01c8d720 */
sceDmaChan *dmafromSPR; /* bss 0x01c8d724 */
sceDmaChan *dmatoSPR;   /* bss 0x01c8d728 */
sceDmaChan *dmaGIF;     /* bss 0x01c8d72c */

// The ROM hands the DMAC a scratchpad packet as `addr - 0x70000000 | 0x80000000` (bit 31 = SPR) and writes
// SADR the same way. Host pointers into the scratchpad buffer need no translation.
#define gaDmaSprAddr(addr) (addr)

// CT3 0x018a1188
void gaDmaInit(void)
{
    ga_dma_env.tqwc = GA_DMA_ENV_TQWC_DEFAULT;
    ga_dma_env.sts = 0;
    ga_dma_env.std = 0;
    ga_dma_env.mfd = 0;
    ga_dma_env.rcycle = 0;
    ga_dma_env.express = 0;
    ga_dma_env.notify = 0;
    ga_dma_env.sqwc = 0;
    ga_dma_env.rbadr = 0;
    ga_dma_env.rbmsk = 0;
    sceDmaPutEnv(&ga_dma_env);
    dmaVif0 = sceDmaGetChan(SCE_DMA_VIF0);
    dmaVif1 = sceDmaGetChan(SCE_DMA_VIF1);
    dmafromSPR = sceDmaGetChan(SCE_DMA_fromSPR);
    dmatoSPR = sceDmaGetChan(SCE_DMA_toSPR);
    dmaGIF = sceDmaGetChan(SCE_DMA_GIF);
}

// CT3 0x018a1228: VIF1 drains a ring buffer that fromSPR fills (MFIFO). The DMAC is suspended while the
// env, D_CTRL and both channels are rewired; VIF1 is left running in chain mode from the ring's start.
void gaDmaOpen_MFIFO_VIF1(void *ring, int size)
{
    REG_DMAC_ENABLEW = D_ENABLEW_CPND;
    ga_dma_env.rbmsk = (u_int)(size - 1) & ~0xfu;
    ga_dma_env.rbadr = ring;
    ga_dma_env.mfd = GA_DMA_ENV_MFD_ON;
    sceDmaPutEnv(&ga_dma_env);
    REG_DMAC_CTRL = (REG_DMAC_CTRL & ~D_CTRL_RELE & ~D_CTRL_RCYC_M) | D_CTRL_RELE;
    dmafromSPR->madr = ring;
    dmafromSPR->chcr.bits &= ~D_CHCR_DIR_M & ~D_CHCR_MOD_M & ~D_CHCR_STR_M;
    REG_DMAC_ENABLEW = 0;
    dmaVif1->tadr = (sceDmaTag *)ring;
    dmaVif1->chcr.bits = ((dmaVif1->chcr.bits | D_CHCR_DIR_M) & ~D_CHCR_MOD_M) | D_CHCR_MOD_CHAIN | D_CHCR_STR_M;
    SYNC(0);
}

// CT3 0x018a1308
void gaDmaClose_MFIFO_VIF1(void)
{
    REG_DMAC_ENABLEW = D_ENABLEW_CPND;
    ga_dma_env.rbadr = 0;
    ga_dma_env.rbmsk = 0;
    dmafromSPR->chcr.bits &= ~D_CHCR_STR_M;
    dmaVif1->chcr.bits &= ~D_CHCR_STR_M;
    ga_dma_env.mfd = 0;
    sceDmaPutEnv(&ga_dma_env);
    REG_DMAC_ENABLEW = 0;
    SYNC(0);
}

// CT3 0x018a1388
void gaDmaSendS_VIF0(void *tag)
{
    sceDmaSend(dmaVif0, gaDmaSprAddr(tag));
}

// CT3 0x018a13c8
void gaDmaSendN_VIF0(void *addr, int qwc)
{
    sceDmaSendN(dmaVif0, gaDmaSprAddr(addr), qwc);
}

// CT3 0x018a1410
void gaDmaSync_VIF0(void)
{
    while (dmaVif0->chcr.bits & D_CHCR_STR_M)
        ;
}

// CT3 0x018a1440
void gaDmaSendS_fromSPR(void *tag)
{
    sceDmaSend(dmafromSPR, tag);
}

// CT3 0x018a1460: `qwc` quadwords from the scratchpad at `spr` to `addr`.
void gaDmaSendN_fromSPR(void *addr, void *spr, int qwc, int rele)
{
    dmafromSPR->sadr = gaDmaSprAddr(spr);
    REG_DMAC_CTRL = (REG_DMAC_CTRL & ~(D_CTRL_RELE | D_CTRL_STS_STD0_M)) | ((u_int)rele << 1);
    sceDmaSendN(dmafromSPR, addr, qwc);
}

// CT3 0x018a14b8: interleaved: `tqwc` quadwords land, `sqwc` are skipped in memory.
void gaDmaSendI_fromSPR(void *addr, void *spr, int tqwc, int sqwc, int qwc)
{
    dmafromSPR->sadr = gaDmaSprAddr(spr);
    ga_dma_env.sqwc = (u_short)sqwc;
    ga_dma_env.tqwc = (u_short)tqwc;
    sceDmaPutEnv(&ga_dma_env);
    REG_DMAC_CTRL = REG_DMAC_CTRL & ~D_CTRL_RELE;
    sceDmaRecvI(dmafromSPR, addr, qwc);
}

// CT3 0x018a1548
void gaDmaSync_fromSPR(void)
{
    while (dmafromSPR->chcr.bits & D_CHCR_STR_M)
        ;
}

// CT3 0x018a1578
void gaDmaSendS_toSPR(void *tag)
{
    sceDmaSend(dmatoSPR, tag);
}

// CT3 0x018a1598: `qwc` quadwords from `addr` to the scratchpad at `spr`.
void gaDmaSendN_toSPR(void *spr, void *addr, int qwc, int rele)
{
    dmatoSPR->sadr = gaDmaSprAddr(spr);
    REG_DMAC_CTRL = (REG_DMAC_CTRL & ~D_CTRL_RELE) | ((u_int)rele << 1);
    sceDmaSendN(dmatoSPR, addr, qwc);
}

// CT3 0x018a15f0: interleaved: `tqwc` quadwords taken, `sqwc` skipped in memory.
void gaDmaSendI_toSPR(void *spr, void *addr, int tqwc, int sqwc, int qwc)
{
    dmatoSPR->sadr = gaDmaSprAddr(spr);
    ga_dma_env.sqwc = (u_short)sqwc;
    ga_dma_env.tqwc = (u_short)tqwc;
    sceDmaPutEnv(&ga_dma_env);
    REG_DMAC_CTRL = REG_DMAC_CTRL & ~D_CTRL_RELE;
    sceDmaSendI(dmatoSPR, addr, qwc);
}

// CT3 0x018a1680
void gaDmaSync_toSPR(void)
{
    while (dmatoSPR->chcr.bits & D_CHCR_STR_M)
        ;
}

// CT3 0x018a16b0
void gaDmaSendS_GIF(void *tag)
{
    sceDmaSend(dmaGIF, gaDmaSprAddr(tag));
}

// CT3 0x018a16f0
void gaDmaSendN_GIF(void *addr, int qwc)
{
    sceDmaSendN(dmaGIF, gaDmaSprAddr(addr), qwc);
}
