// game/AA_GetDataWord.h -- CT3 game/AA_GetDataWord.c (0x018bc690..0x018bda90): the movie's display (SCE's
// ezmpegstr disp.c, AA_-prefixed and grown with the captions' reader). Slice 1 (2026-11-12) wrote the
// caption readers, the caption buffer, the pacing and the display switches; the GS side (AA_GS_SetClut,
// AA_clearGsMem, AA_setImageTag / 2, AA_vblankHandler) is slice 2's.
#pragma once

#include "sdk/scetypes.h"

// The captions' picture (AA_MoiveMojiFB): 512 x 448 pixels of 4 bits (PSMT4, CLUT at AA_CLUT_CBP), two
// pixels a byte, 0x100 bytes a row -- the AA_setImageTag upload's 0x1c00 qwords.
#define AA_MOJI_FB_WIDTH 0x200
#define AA_MOJI_FB_HEIGHT 0x1c0
#define AA_MOJI_FB_BYTES (AA_MOJI_FB_WIDTH * AA_MOJI_FB_HEIGHT / 2)   /* 0x1c000: AA_CleanMojiBuffer's 0x7000 words */

// A caption record (AA_TestRead): u32 time stamp, u32 kind, then for AA_MOJI_PUT four u16 (x, y, width,
// height in pixels) and height rows of width / 2 bytes. Any other kind clears the picture. The list ends
// with a time stamp of AA_MOJI_END. Little-endian, byte-aligned (the readers go a byte at a time).
#define AA_MOJI_PUT 1
#define AA_MOJI_END ((int)0xffffffff)

extern int AA_isCountVblank;            /* sbss 0x0193f0bc: the vblank counts (AA_startDisplay .. AA_endDisplay) */
extern int AA_vblankCount;              /* sbss 0x0193f0c0: vblanks since AA_startDisplay */
extern int AA_isFrameEnd;               /* sbss 0x0193f0c4: the odd field was sent; the image-end frees its buffer */
extern int AA_oddeven;                  /* sbss 0x0193f0c8: the field the vblank saw (CSR FIELD) */
extern int AA_handler_error;            /* sbss 0x0193f0cc: sceGsSyncPath's answer in the vblank */
extern u_int AA_MojiBuffWk;             /* sbss 0x0193f0d0: the caption picture AA_setImageTag uploads */

// PORT: the captions' picture on the host. The ROM keeps it in the decode work's tail (AA_MoiveMojiFB =
// AA_mpegWork + 0x1e4000, an EE address in a movie bank); the host decoder needs no such work, so the
// picture lives here and AA_MoiveMojiFB only says whether there is one. AA_MojiFB() is NULL when not.
extern u_char AA_MojiFBImage[AA_MOJI_FB_BYTES];
u_char *AA_MojiFB(void);

u_short AA_GetDataWord(const u_char **cursor);                                           // CT3 0x018bc690
u_int AA_GetDataLong(const u_char **cursor);                                             // CT3 0x018bc6c0
u_int AA_GetDataLongRead(const u_char *data);                                            // CT3 0x018bc720
void AA_TestRead(const u_char **cursor, int time);                                       // CT3 0x018bc750
void AA_CleanMojiBuffer(void);                                                           // CT3 0x018bc8a0
void AA_test_RamInit(void);                                                              // CT3 0x018bc8f0
void AA_GS_SetClut(int cbp);                                                             // CT3 0x018bc900
int shouldDelay(void);                                                                   // CT3 0x018bc960
void AA_endDisplay(void);                                                                // CT3 0x018bc9f0
void AA_startDisplay(int waitEven);                                                      // CT3 0x018bca00
int AA_handler_endimage(int channel);                                                    // CT3 0x018bca50
int AA_vblankHandler(int cause);                                                         // CT3 0x018bca90
void AA_clearGsMem(int r, int g, int b, int w, int h);                                   // CT3 0x018bd830
