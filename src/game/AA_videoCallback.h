// game/AA_videoCallback.h -- CT3 game/AA_videoCallback.c: the movie's stream file and read ring (SCE's
// ezmpegstr strfile.c / readbuf.c, AA_-prefixed), the declarations other TUs call (game/movie.c first,
// 2026-11-10). Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// The movie file being streamed (AA_infile, sbss 0x0193f4f0: 0x38 bytes in the symtab). layout: ee
// (opaque until this TU names its fields).
typedef struct StrFile {
    u_char raw[0x38];
} StrFile;
static_assert(sizeof(StrFile) == 0x38, "StrFile");

// The read ring between the stream file and the demuxer (AA_readBuf, sbss 0x0193f6b0: 0x10 bytes).
// AA_MemoryMapSet allocates its 0x60000-byte data into the first word; readMpeg passes words 0 and 3
// to sceMpegDemuxPssRing. layout: host (the first word is a gHeapSound block).
typedef struct ReadBuf {
    u_char *data;           /* EE 0x00: _SunAlloc(gHeapSound, AA_READBUF_SIZE) */
    int w04[3];             /* EE 0x04..0x0c: this TU's (put / get / size) */
} ReadBuf;

int AA_strFileInit(StrFile *file);                                                       // CT3 0x018c02d0
