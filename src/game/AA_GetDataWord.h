// game/AA_GetDataWord.h -- CT3 game/AA_GetDataWord.c: the movie's display (SCE's ezmpegstr disp.c,
// AA_-prefixed), the declarations other TUs call (game/movie.c first, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void AA_GS_SetClut(int cbp);                                                             // CT3 0x018bc900
void AA_clearGsMem(int r, int g, int b, int w, int h);                                   // CT3 0x018bd830
