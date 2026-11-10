// game/AA_videoCallback.h -- CT3 game/AA_videoCallback.c: the movie's stream file and read ring (SCE's
// ezmpegstr strfile.c / readbuf.c, AA_-prefixed), the declarations other TUs call (game/movie.c first,
// 2026-11-10). Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "sdk/libmpeg.h"

#include <stddef.h>

// The movie file being streamed (AA_infile, sbss 0x0193f4f0: 0x38 bytes in the symtab). readMpeg reads
// +4 (the bytes to stream, ezmpegstr's `size`); AA_movie tests +0x34 (non-zero: the open failed for good,
// so it skips readMpeg and reports "File Open Error!!"). layout: ee (the rest is this TU's to name).
typedef struct StrFile {
    int w00;                /* 0x00: ezmpegstr's isOnCD (unread so far) */
    int size;               /* 0x04: the file's bytes */
    u_char raw08[0x2c];
    int openError;          /* 0x34: AA_strFileOpen gave up (CT3's; AA_CdDvd_trial_Ct counts the tries) */
} StrFile;
static_assert(sizeof(StrFile) == 0x38, "StrFile");
static_assert(offsetof(StrFile, openError) == 0x34, "StrFile.openError");

// The read ring between the stream file and the demuxer (AA_readBuf, sbss 0x0193f6b0: 0x10 bytes).
// AA_MemoryMapSet allocates its 0x60000-byte data into the first word; readMpeg passes words 0 and 3
// to sceMpegDemuxPssRing (the ring and its size). layout: host (the first word is a gHeapSound block).
typedef struct ReadBuf {
    u_char *data;           /* EE 0x00: _SunAlloc(gHeapSound, AA_READBUF_SIZE) */
    int w04[2];             /* EE 0x04..0x08: this TU's (put / get) */
    int size;               /* EE 0x0c: the ring's bytes */
} ReadBuf;

int AA_videoCallback(sceMpeg *mp, sceMpegCbData *data, void *readBuf);                  // CT3 0x018bfbf0
int AA_readBufEndGet(ReadBuf *b, int size);                                              // CT3 0x018bfd00
int AA_readBufBeginGet(ReadBuf *b, u_char **ptr);                                        // CT3 0x018bfd20
int AA_readBufEndPut(ReadBuf *b, int size);                                              // CT3 0x018bfd60
int AA_readBufBeginPut(ReadBuf *b, u_char **ptr);                                        // CT3 0x018bfdb0
void AA_readBufDelete(ReadBuf *b);                                                       // CT3 0x018bfde0
void AA_readBufCreate(ReadBuf *b);                                                       // CT3 0x018bfdf0
int AA_strFileRead(StrFile *file, void *buff, int size);                                 // CT3 0x018bfe10
int AA_strFileClose(StrFile *file);                                                      // CT3 0x018bfe80
int AA_strFileOpen(StrFile *file, const char *filename);                                 // CT3 0x018bff20
int AA_strFileInit(StrFile *file);                                                       // CT3 0x018c02d0
