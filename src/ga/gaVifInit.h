// ga/gaVifInit.h -- CT3 ga/gaVifInit.c (0x018a1948..0x018a1a48, data 0x0192e640..0x0192e790): the ga
// library's VIF set-up, its canned VIFcode qwords (MSCAL / MSCNT / FLUSH* in every combination the
// renderer sends), and the VIF / VU status polls. Reconstructed 2026-11-09; the object is GCC-built.
#pragma once

#include "sdk/scetypes.h"

// VIFn_STAT fields the polls read (EE manual, VIF registers).
#define VIF_STAT_VPS_M 0x00000003  /* VIF packet status: idle / waiting for data / decoding / transferring */
#define VIF_STAT_VEW 0x00000004    /* a VIF code waits for the VU (VU microprogram running) */
#define VIF_STAT_VGW 0x00000008    /* VIF1: waiting for the GIF (PATH3 / FLUSHA) */
#define VIF0_STAT_FQC_M 0x0f000000 /* VIF0 FIFO quadword count (bits 24..27) */
#define VIF1_STAT_FQC_M 0x1f000000 /* VIF1 FIFO quadword count (bits 24..28) */
// gaVUSync_VU0 waits on VIF0_STAT bit 3 (0x10003800 & 8), not on VEW (bit 2): kept as the ROM reads it.
#define VIF0_STAT_VU0_BUSY 0x00000008

// One quadword of VIFcodes, as the DMAC sends it (the definitions are 16-byte aligned).
#define VIFCODE_QW_WORDS 4

extern u_int vifcode_init_VIF0[VIFCODE_QW_WORDS];                     /* data 0x0192e640 */
extern u_int vifcode_init_VIF1[4 * VIFCODE_QW_WORDS];                 /* data 0x0192e650 */
extern u_int vifcode_mscal_0000[VIFCODE_QW_WORDS];                    /* data 0x0192e690 */
extern u_int vifcode_mscal_0000_flush[VIFCODE_QW_WORDS];              /* data 0x0192e6a0 */
extern u_int vifcode_mscal_0002[VIFCODE_QW_WORDS];                    /* data 0x0192e6b0 */
extern u_int vifcode_mscal_0002_flushe[VIFCODE_QW_WORDS];             /* data 0x0192e6c0 */
extern u_int vifcode_mscal_0002_flush_mscnt[VIFCODE_QW_WORDS];        /* data 0x0192e6d0 */
extern u_int vifcode_flush_mscnt_flushe[VIFCODE_QW_WORDS];            /* data 0x0192e6e0 */
extern u_int vifcode_mscal_0002_flush_mscnt_flushe[VIFCODE_QW_WORDS]; /* data 0x0192e6f0 */
extern u_int vifcode_mscal_0004_flush_mscnt_flushe[VIFCODE_QW_WORDS]; /* data 0x0192e700 */
extern u_int vifcode_mscnt_flushe[VIFCODE_QW_WORDS];                  /* data 0x0192e710 */
extern u_int vifcode_mscnt[VIFCODE_QW_WORDS];                         /* data 0x0192e720 */
extern u_int vifcode_flush[VIFCODE_QW_WORDS];                         /* data 0x0192e730 */
extern u_int vifcode_flushe[VIFCODE_QW_WORDS];                        /* data 0x0192e740 */
extern u_int vifcode_mscal_0006[VIFCODE_QW_WORDS];                    /* data 0x0192e750 */
extern u_int vifcode_mscal_0008[VIFCODE_QW_WORDS];                    /* data 0x0192e760 */
extern u_int vifcode_flushe_mscnt_flushe[VIFCODE_QW_WORDS];           /* data 0x0192e770 */
extern u_int vifcode_mscal_000A[VIFCODE_QW_WORDS];                    /* data 0x0192e780 */

void gaVifInit(void);            // CT3 0x018a1948
void gaVUSync_VU0(void);         // CT3 0x018a1988
void gaVifcodeFlush_VIF1(void);  // CT3 0x018a19b8
void gaVifcodeFlushe_VIF1(void); // CT3 0x018a19e0
u_int gaVIFSync_VIF0(void);      // CT3 0x018a1a08
u_int gaVIFSync_VIF1(void);      // CT3 0x018a1a28
