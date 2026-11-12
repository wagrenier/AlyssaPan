// game/AA_scTag2.h -- CT3 game/AA_scTag2.c: the declarations game/AA_mpegTS.c calls (2026-11-12).
// Prototypes from its call sites (SCE's ezmpegstr videodec.c names); the TU that writes them confirms.
// Both are LOCAL in the symtab, in AA_mpegTS.c's own local run (open-items.md (hf)): one object, probably.
#pragma once

#include "sdk/scetypes.h"

typedef struct VideoDec VideoDec;

// Non-zero while the video lags the audio enough to drop B pictures (lastVideo / lastAudio, sbss 0x0193f710).
int AA_shouldSkip(VideoDec *vd);                                                         // CT3 0x018c14b0
// Copies the source span(s) ps0[s0], ps1[s1] into the destination span(s) pd0[d0], pd1[d1] (a ring's two
// pieces); answers the bytes copied.
int AA_cpy2area(u_char *pd0, int d0, u_char *pd1, int d1, const u_char *ps0, int s0, const u_char *ps1,
                int s1);                                                                 // CT3 0x018c1540
