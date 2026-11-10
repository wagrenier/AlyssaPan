// ga/gaDma.h -- CT3 ga/gaDma.c (0x018a1188..0x018a1738): the ga library's DMA channel wrappers.
// Prototypes from the ROM's register use (written 2026-11-09); the decompiler drops the arguments it
// forwards to libdma, so each signature here was read from the asm.
#pragma once

#include "sdk/libdma.h"
#include "sdk/scetypes.h"

extern sceDmaEnv ga_dma_env;   /* bss 0x01c8d708 (EE 20 bytes; host: rbadr is a host pointer) */
extern sceDmaChan *dmaVif0;    /* bss 0x01c8d71c */
extern sceDmaChan *dmaVif1;    /* bss 0x01c8d720 */
extern sceDmaChan *dmafromSPR; /* bss 0x01c8d724 */
extern sceDmaChan *dmatoSPR;   /* bss 0x01c8d728 */
extern sceDmaChan *dmaGIF;     /* bss 0x01c8d72c */

void gaDmaInit(void);                                                                    // CT3 0x018a1188
void gaDmaOpen_MFIFO_VIF1(void *ring, int size);                                         // CT3 0x018a1228
void gaDmaClose_MFIFO_VIF1(void);                                                        // CT3 0x018a1308
void gaDmaSendS_VIF0(void *tag);                                                         // CT3 0x018a1388
void gaDmaSendN_VIF0(void *addr, int qwc);                                               // CT3 0x018a13c8
void gaDmaSync_VIF0(void);                                                               // CT3 0x018a1410
void gaDmaSendS_fromSPR(void *tag);                                                      // CT3 0x018a1440
void gaDmaSendN_fromSPR(void *addr, void *spr, int qwc, int rele);                       // CT3 0x018a1460
void gaDmaSendI_fromSPR(void *addr, void *spr, int tqwc, int sqwc, int qwc);             // CT3 0x018a14b8
void gaDmaSync_fromSPR(void);                                                            // CT3 0x018a1548
void gaDmaSendS_toSPR(void *tag);                                                        // CT3 0x018a1578
void gaDmaSendN_toSPR(void *spr, void *addr, int qwc, int rele);                         // CT3 0x018a1598
void gaDmaSendI_toSPR(void *spr, void *addr, int tqwc, int sqwc, int qwc);               // CT3 0x018a15f0
void gaDmaSync_toSPR(void);                                                              // CT3 0x018a1680
void gaDmaSendS_GIF(void *tag);                                                          // CT3 0x018a16b0
void gaDmaSendN_GIF(void *addr, int qwc);                                                // CT3 0x018a16f0
