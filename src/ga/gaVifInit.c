// ga/gaVifInit.c -- Clock Tower 3, the ga library's VIF set-up and VIF / VU status polls
// (CT3 0x018a1948..0x018a1a48, data 0x0192e640..0x0192e790). Object extent from the symtab: its local run
// (.text 0x018a1948 and .data 0x0192e640, `gcc2_compiled.`) ends where gaInitDrawObject's object starts
// (0x018a1a48) and where gaInitInterrupt's .data starts (0x0192e790). Reconstructed 2026-11-09.
//
// Host: there is no VIF or VU. The packets still go through the DMA wrappers as the ROM sends them (the
// host DMAC drops what is not a scratchpad transfer, sdk/libdma.cpp), and VIFn_STAT reads idle
// (sdk/eeregs.h), so the polls return at once: the draws they order are made natively.

#include "ga/gaVifInit.h"

#include "ga/gaDma.h"
#include "ga/gaDmaSync_VIF1.h"
#include "sdk/eeregs.h"
#include "sdk/libgraph.h"

#define VC_STCYCL_1_1 VC(SCE_VIF1_STCYCL, 0, 0x0101) /* CL 1, WL 1: unpack writes every qword */
#define FLOAT_BITS_ONE 0x3f800000u                   /* 1.0f, STROW's fill value */

#define MSKPATH3_IMM 1 /* the ROM's immediate: bit 15 clear, so PATH3 is left unmasked */

#define VIF_INIT_VIF0_QWC 1
#define VIF_INIT_VIF1_QWC 4
#define VIFCODE_QWC 1

// VIF0: one qword. Its STMASK's mask word is not in it (the ROM sends 1 qword): VIF0 takes the next word
// it receives as the mask.
alignas(16) u_int vifcode_init_VIF0[VIFCODE_QW_WORDS] = {
    VC_STCYCL_1_1, VC(SCE_VIF1_ITOP, 0, 0), VC(SCE_VIF1_STMOD, 0, 0), VC(SCE_VIF1_STMASK, 0, 0),
};
// VIF1: cycle, double-buffer offset / base, ITOP, normal mode, PATH3 unmasked, mask 0, row = 1.0f.
alignas(16) u_int vifcode_init_VIF1[4 * VIFCODE_QW_WORDS] = {
    VC_STCYCL_1_1,
    VC(SCE_VIF1_OFFSET, 0, 0),
    VC(SCE_VIF1_BASE, 0, 0),
    VC(SCE_VIF1_ITOP, 0, 0),
    VC(SCE_VIF1_STMOD, 0, 0),
    VC(SCE_VIF1_MSKPATH3, 0, MSKPATH3_IMM),
    VC(SCE_VIF1_STMASK, 0, 0),
    0, /* the mask */
    VC(SCE_VIF1_STROW, 0, 0),
    FLOAT_BITS_ONE,
    FLOAT_BITS_ONE,
    FLOAT_BITS_ONE,
    FLOAT_BITS_ONE,
    VC_NOP,
    VC_NOP,
    VC_NOP,
};
alignas(16) u_int vifcode_mscal_0000[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0000)};
alignas(16) u_int vifcode_mscal_0000_flush[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0000),
                                                                VC_FLUSH};
alignas(16) u_int vifcode_mscal_0002[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0002)};
alignas(16) u_int vifcode_mscal_0002_flushe[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0002),
                                                                 VC_FLUSHE};
alignas(16) u_int vifcode_mscal_0002_flush_mscnt[VIFCODE_QW_WORDS] = {VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0002), VC_FLUSH,
                                                                      VC_MSCNT};
alignas(16) u_int vifcode_flush_mscnt_flushe[VIFCODE_QW_WORDS] = {VC_FLUSH, VC_MSCNT, VC_FLUSHE, VC_NOP};
alignas(16) u_int vifcode_mscal_0002_flush_mscnt_flushe[VIFCODE_QW_WORDS] = {VC_MSCAL(VU_MPG_ENTRY_0002), VC_FLUSH,
                                                                             VC_MSCNT, VC_FLUSHE};
alignas(16) u_int vifcode_mscal_0004_flush_mscnt_flushe[VIFCODE_QW_WORDS] = {VC_MSCAL(VU_MPG_ENTRY_0004), VC_FLUSH,
                                                                             VC_MSCNT, VC_FLUSHE};
alignas(16) u_int vifcode_mscnt_flushe[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_MSCNT, VC_FLUSHE};
alignas(16) u_int vifcode_mscnt[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_NOP, VC_MSCNT};
alignas(16) u_int vifcode_flush[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_NOP, VC_FLUSH};
alignas(16) u_int vifcode_flushe[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_NOP, VC_FLUSHE};
alignas(16) u_int vifcode_mscal_0006[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0006)};
alignas(16) u_int vifcode_mscal_0008[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0008)};
alignas(16) u_int vifcode_flushe_mscnt_flushe[VIFCODE_QW_WORDS] = {VC_FLUSHE, VC_MSCNT, VC_FLUSHE, VC_NOP};
alignas(16) u_int vifcode_mscal_000A[VIFCODE_QW_WORDS] = {VC_NOP, VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_000A)};

// CT3 0x018a1948: reset VIF0's and VIF1's modes and registers.
void gaVifInit(void)
{
    gaDmaSendN_VIF0(vifcode_init_VIF0, VIF_INIT_VIF0_QWC);
    gaDmaSync_VIF0();
    gaDmaSendN_VIF1(vifcode_init_VIF1, VIF_INIT_VIF1_QWC);
    gaDmaSync_VIF1();
}

// CT3 0x018a1988: wait for VU0's microprogram (no caller in the ROM).
void gaVUSync_VU0(void)
{
    while (REG_VIF0_STAT & VIF0_STAT_VU0_BUSY)
        ;
}

// CT3 0x018a19b8
void gaVifcodeFlush_VIF1(void)
{
    gaDmaSendN_VIF1(vifcode_flush, VIFCODE_QWC);
    gaDmaSync_VIF1();
}

// CT3 0x018a19e0 (no caller in the ROM)
void gaVifcodeFlushe_VIF1(void)
{
    gaDmaSendN_VIF1(vifcode_flushe, VIFCODE_QWC);
    gaDmaSync_VIF1();
}

// CT3 0x018a1a08: non-zero while VIF0 holds data or works (no caller in the ROM).
u_int gaVIFSync_VIF0(void)
{
    return REG_VIF0_STAT & (VIF0_STAT_FQC_M | VIF_STAT_VEW | VIF_STAT_VPS_M);
}

// CT3 0x018a1a28: non-zero while VIF1 holds data or works.
u_int gaVIFSync_VIF1(void)
{
    return REG_VIF1_STAT & (VIF1_STAT_FQC_M | VIF_STAT_VGW | VIF_STAT_VEW | VIF_STAT_VPS_M);
}
