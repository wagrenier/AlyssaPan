// game/AA_mpegTS.h -- CT3 game/AA_mpegTS.c: the movie's video decoder and picture buffers (SCE's ezmpegstr
// videodec.c / vobuf.c, AA_-prefixed), the types game/movie.c defines objects of (2026-11-10).
// Prototypes from movie.c's call sites (slice 2); the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "sdk/libmpeg.h"

#include <stddef.h>

// AA_videoDecGetState's answers (ezmpegstr's VdState).
#define VD_STATE_NORMAL 0
#define VD_STATE_ABORT 1
#define VD_STATE_FLUSH 2
#define VD_STATE_END 3                  /* readMpeg's loops stop on it */

// The video decoder (AA_videoDec, sbss 0x0193f410: 0xd8 bytes in the symtab). It starts with the libmpeg
// context: readMpeg reads mpeg.frameCount (+8: more than 15 lets the pad skip the movie) and demuxes into
// it. initAll / termAll keep the two interrupt handler ids at +0xd0 / +0xd4 (ezmpegstr's hid_endimage /
// hid_vblank). layout: ee (sceMpeg is 0x48 on the EE and on the 64-bit host; the rest is this TU's).
typedef struct VideoDec {
    sceMpeg mpeg;           /* 0x00 */
    u_char raw48[0x88];
    int hid_endimage;       /* 0xd0: AddDmacHandler(DMAC_GIF, AA_handler_endimage) */
    int hid_vblank;         /* 0xd4: AddIntcHandler(INTC_VBLANK_S, AA_vblankHandler) */
} VideoDec;
static_assert(sizeof(VideoDec) == 0xd8, "VideoDec");
static_assert(offsetof(VideoDec, mpeg.frameCount) == 0x08, "VideoDec.mpeg.frameCount");
static_assert(offsetof(VideoDec, hid_endimage) == 0xd0, "VideoDec.hid_endimage");

// The decoded-picture queue (voBuf, sbss 0x0193f340: 0x48 bytes). AA_MemoryMapSet clears the five words
// at +0x34 (AA_MemoryMapFree only the first); the head is this TU's (ezmpegstr's ring). CT3 grew the
// sample's 0x14 bytes with the display's pacing state (AA_GetDataWord.c, 2026-11-12): the vblank's hold
// count and shouldDelay's skew history. layout: ee.
#define VOBUF_SKEWS 6                   /* shouldDelay's history of AA_syncGetSkew answers */
typedef struct VoBuf {
    u_char raw[0x14];       /* 0x00: the ring (data, tag, write, count, size in ezmpegstr): this TU names it */
    int delayHold;          /* 0x14: vblanks left before AA_vblankHandler asks shouldDelay again */
    int skew[VOBUF_SKEWS];  /* 0x18: the last six skews, newest at skewIndex - 1 */
    int skewIndex;          /* 0x30: the next skew slot, mod VOBUF_SKEWS */
    int w34[5];             /* 0x34..0x44: w34[0] != 0 lets the vblank count AA_FadeInCount down */
} VoBuf;
static_assert(sizeof(VoBuf) == 0x48, "VoBuf");
static_assert(offsetof(VoBuf, delayHold) == 0x14, "VoBuf.delayHold");
static_assert(offsetof(VoBuf, skew) == 0x18, "VoBuf.skew");
static_assert(offsetof(VoBuf, skewIndex) == 0x30, "VoBuf.skewIndex");
static_assert(offsetof(VoBuf, w34) == 0x34, "VoBuf.w34");

// One picture's GS upload tags (AA_voBufTag[2], sbss 0x0193f6d0: 0x30 bytes). The two words at +0x10 are
// the tag blocks AA_MovieUseBank gives a movie bank each ("MPEG GS tag 0" / "MPEG Gs tag 1"; the ROM puts
// the second one 0x20000 bytes after the first in AA_MemoryMapSet). layout: ee.
typedef struct VoTag {
    int w00[4];             /* 0x00: vobuf.c's (status, ...) */
    u_int tag[2];           /* 0x10: EE addresses of the picture's tag blocks */
} VoTag;
static_assert(sizeof(VoTag) == 0x18, "VoTag");

#define AA_VOBUF_COUNT 2                /* picture buffers (initAll's AA_voBufCreate count) */

void AA_ReTagSet(u_int *tag);                                                            // CT3 0x018c1760
void AA_videoDecMain(void *videoDec);                                                    // CT3 0x018c1b00
int AA_videoDecIsFlushed(VideoDec *vd);                                                  // CT3 0x018c1b60
int AA_videoDecFlush(VideoDec *vd);                                                      // CT3 0x018c1ba0
int AA_videoDecGetState(VideoDec *vd);                                                   // CT3 0x018c1cd0
void AA_videoDecAbort(VideoDec *vd);                                                     // CT3 0x018c1ce0
int AA_videoDecDelete(VideoDec *vd);                                                     // CT3 0x018c1cf0
void AA_videoDecSetStream(VideoDec *vd, int strType, int ch, sceMpegCallback cb, void *data); // CT3 0x018c1d80
// mpegWork and viBufData are movie banks' EE addresses (AA_MovieUseBank); the rest gHeapSound blocks.
int AA_videoDecCreate(VideoDec *vd, u_int mpegWork, int workSize, u_int viBufData, void *viBufTag,
                      int tagCount, void *timeStamp, int timeStamps);                    // CT3 0x018c1da0
int AA_voBufIsFull(VoBuf *f);                                                            // CT3 0x018c2030
void AA_voBufDelete(VoBuf *f);                                                           // CT3 0x018c2080
void AA_voBufCreate(VoBuf *f, u_int *data, VoTag *tag, int n);                           // CT3 0x018c2090
void AA_voBufDecCount(VoBuf *f);                                                         // CT3 0x018c1ea0
