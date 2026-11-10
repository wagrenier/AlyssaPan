// game/AA_mpegTS.h -- CT3 game/AA_mpegTS.c: the movie's video decoder and picture buffers (SCE's ezmpegstr
// videodec.c / vobuf.c, AA_-prefixed), the types game/movie.c defines objects of (2026-11-10).
// Prototypes and fields come when this TU (or movie.c's playback slice) reaches them.
#pragma once

#include "sdk/scetypes.h"

// The video decoder (AA_videoDec, sbss 0x0193f410: 0xd8 bytes in the symtab). readMpeg reads its +8
// (a frame count: more than 15 lets the pad skip the movie) -- movie.c's slice 2. layout: ee (opaque).
typedef struct VideoDec {
    u_char raw[0xd8];
} VideoDec;
static_assert(sizeof(VideoDec) == 0xd8, "VideoDec");

// The decoded-picture queue (voBuf, sbss 0x0193f340: 0x48 bytes). AA_MemoryMapSet clears the five words
// at +0x34 (AA_MemoryMapFree only the first); the rest is this TU's. layout: ee.
typedef struct VoBuf {
    u_char raw[0x34];
    int w34[5];             /* 0x34..0x44 */
} VoBuf;
static_assert(sizeof(VoBuf) == 0x48, "VoBuf");

// One picture's GS upload tags (AA_voBufTag[2], sbss 0x0193f6d0: 0x30 bytes). The two words at +0x10 are
// the tag blocks AA_MovieUseBank gives a movie bank each ("MPEG GS tag 0" / "MPEG Gs tag 1"; the ROM puts
// the second one 0x20000 bytes after the first in AA_MemoryMapSet). layout: ee.
typedef struct VoTag {
    int w00[4];             /* 0x00: vobuf.c's (status, ...) */
    u_int tag[2];           /* 0x10: EE addresses of the picture's tag blocks */
} VoTag;
static_assert(sizeof(VoTag) == 0x18, "VoTag");

#define AA_VOBUF_COUNT 2                /* picture buffers (initAll's AA_voBufCreate count) */
