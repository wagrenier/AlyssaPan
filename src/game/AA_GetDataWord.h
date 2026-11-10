// game/AA_GetDataWord.h -- CT3 game/AA_GetDataWord.c: the movie's display (SCE's ezmpegstr disp.c,
// AA_-prefixed), the declarations other TUs call (game/movie.c first, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

extern int AA_isCountVblank;            /* sbss 0x0193f0bc: AA_movie sets it over the fade-out */

void AA_CleanMojiBuffer(void);                                                           // CT3 0x018bc8a0
void AA_test_RamInit(void);                                                              // CT3 0x018bc8f0
void AA_GS_SetClut(int cbp);                                                             // CT3 0x018bc900
void AA_endDisplay(void);                                                                // CT3 0x018bc9f0
void AA_startDisplay(int waitEven);                                                      // CT3 0x018bca00
int AA_handler_endimage(int channel);                                                    // CT3 0x018bca50
int AA_vblankHandler(int cause);                                                         // CT3 0x018bca90
void AA_clearGsMem(int r, int g, int b, int w, int h);                                   // CT3 0x018bd830
