// game/AA_scTag2.h -- CT3 game/AA_scTag2.c: the movie ring's DMA helpers (SCE's ezmpegstr vibuf.c) and two
// of videodec.c's (AA_shouldSkip, AA_cpy2area), AA_-prefixed. Written 2026-11-12 (all 7 functions).
// AA_shouldSkip and AA_cpy2area are LOCAL in the symtab, in AA_mpegTS.c's local run (open-items.md (hf)),
// yet sit at the end of this range: the object boundary between them is not settled by a symbol.
#pragma once

#include "sdk/scetypes.h"

typedef struct VideoDec VideoDec;
typedef struct ViBuf ViBuf;

// D_ENABLEW / D_ENABLER's CPND bit: holds every DMA channel while a CHCR is rewritten.
#define D_ENABLE_CPND 0x10000u
// A D4 MADR's ring block: (MADR - data) >> 11 (VIBUF_ELM_SIZE).
#define VIBUF_ELM_SHIFT 11
// The DMA tag's fields (a qword's low 64 bits): QWC bits 0..15, ID bits 28..30, ADDR bits 32..63.
#define DMA_TAG_ID_SHIFT 28
#define DMA_TAG_ADDR_SHIFT 32
// The physical address of an uncached-alias one (the ROM strips the tag list's | 0x20000000).
#define EE_PHYS_MASK 0x0fffffffu

// DMA addresses are the ring's host DMA space (AA_viBuf.h VIBUF_DMA_DATA / VIBUF_DMA_TAG), not host pointers.
// Writes one DMA tag (a qword's low 64 bits): qwc, id << 28, addr << 32.
void AA_scTag2(void *tag, u_int addr, int id, int qwc);                                  // CT3 0x018c1340
// D4 (toIPU) / D3 (fromIPU) CHCR, written with the channels held by D_ENABLEW and interrupts off.
void AA_setD4_CHCR(u_int chcr);                                                          // CT3 0x018c1370
void AA_setD3_CHCR(u_int chcr);                                                          // CT3 0x018c13e0
// The ring block a D4 MADR points into (0 when it points past the tag list: the NEXT tag's end).
int AA_getFIFOindex(ViBuf *f, u_int madr);                                               // CT3 0x018c1450
// Non-zero when pos lies in [start, start + len) of a ring of `size` bytes.
int AA_IsPtsInRegion(int pos, int start, int len, int size);                             // CT3 0x018c1490

// Records AA_syncGetSkew's answer in vd->skew[]; non-zero while all VD_SKEWS of them are positive (the
// video lags: AA_decBs0 then tells libmpeg to drop B pictures).
int AA_shouldSkip(VideoDec *vd);                                                         // CT3 0x018c14b0
// Copies the source span(s) ps0[s0], ps1[s1] into the destination span(s) pd0[d0], pd1[d1] (a ring's two
// pieces); answers the bytes copied, 0 (nothing copied) when they do not fit.
int AA_cpy2area(u_char *pd0, int d0, u_char *pd1, int d1, const u_char *ps0, int s0, const u_char *ps1,
                int s1);                                                                 // CT3 0x018c1540
