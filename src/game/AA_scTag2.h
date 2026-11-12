// game/AA_scTag2.h -- CT3 game/AA_scTag2.c: the declarations game/AA_mpegTS.c calls (2026-11-12).
// Prototypes from its call sites (SCE's ezmpegstr videodec.c names); the TU that writes them confirms.
// Both are LOCAL in the symtab, in AA_mpegTS.c's own local run (open-items.md (hf)): one object, probably.
#pragma once

#include "sdk/scetypes.h"

typedef struct VideoDec VideoDec;
typedef struct ViBuf ViBuf;

// Declared 2026-11-12 from game/AA_viBuf.c's calls (ezmpegstr's vibuf.c helpers). DMA addresses are the
// ring's host DMA space (AA_viBuf.h VIBUF_DMA_DATA / VIBUF_DMA_TAG), not host pointers.
// Writes one DMA tag (a qword's low 64 bits): qwc, id << 28, addr << 32.
void AA_scTag2(void *tag, u_int addr, int id, int qwc);                                  // CT3 0x018c1340
// D4 (toIPU) / D3 (fromIPU) CHCR, written with the channel held by D_ENABLEW and interrupts off.
void AA_setD4_CHCR(u_int chcr);                                                          // CT3 0x018c1370
void AA_setD3_CHCR(u_int chcr);                                                          // CT3 0x018c13e0
// The ring block a D4 MADR points into (0 when it points past the tag list: the NEXT tag's end).
int AA_getFIFOindex(ViBuf *f, u_int madr);                                               // CT3 0x018c1450
// Non-zero when pos lies in [start, start + len) of a ring of `size` bytes.
int AA_IsPtsInRegion(int pos, int start, int len, int size);                             // CT3 0x018c1490

// Non-zero while the video lags the audio enough to drop B pictures (lastVideo / lastAudio, sbss 0x0193f710).
int AA_shouldSkip(VideoDec *vd);                                                         // CT3 0x018c14b0
// Copies the source span(s) ps0[s0], ps1[s1] into the destination span(s) pd0[d0], pd1[d1] (a ring's two
// pieces); answers the bytes copied.
int AA_cpy2area(u_char *pd0, int d0, u_char *pd1, int d1, const u_char *ps0, int s0, const u_char *ps1,
                int s1);                                                                 // CT3 0x018c1540
