// game/AA_videoCallback.h -- CT3 game/AA_videoCallback.c: the movie's stream file and read ring (SCE's
// ezmpegstr strfile.c / readbuf.c, AA_-prefixed, with CT3's packed-file and retry additions) and the
// video stream callback. Written 2026-11-12 (the TU's session); movie.c is the only caller.
#pragma once

#include "sdk/scetypes.h"
#include "sdk/libmpeg.h"

#include <stddef.h>

// Where a stream file lives (StrFile.isOnCD), from the name AA_strFileOpen is given.
enum {
    STRFILE_HOST   = 0,     // "<device>:<path>" other than cdrom0: sceOpen / sceRead / sceClose
    STRFILE_CD     = 1,     // "cdrom0:<path>": sceCdSearchFile, then the drive's stream (sceCdSt*)
    STRFILE_PACKED = 2,     // a bare name: a CT3PACK.DAT row (FlFindPacked), then the drive's stream
};

#define STRFILE_IOP_BUF_SIZE 0x28010    // AA_strFileInit's IOP block (the stream ring, 16-byte aligned)
#define STRFILE_ST_BUF_SECTS 0x50       // sceCdStInit's ring: 80 sectors ...
#define STRFILE_ST_BANKS     5          // ... in 5 banks
#define STRFILE_SECTOR_SHIFT 11         // 2048-byte sectors

// The SDK's file record as CT3's SDK had it (sceCdlFILE, 0x24 bytes: the host's sdk/libcdvd.h carries a
// longer name). AA_strFileOpen fills lsn and size; the name, date and flag only through sceCdSearchFile.
typedef struct AA_CDLFILE {
    u_int lsn;              /* 0x00: the file's first sector */
    u_int size;             /* 0x04: bytes */
    char name[16];          /* 0x08 */
    u_char date[8];         /* 0x18 */
    u_int flag;             /* 0x20: cleared for a packed file */
} AA_CDLFILE;
static_assert(sizeof(AA_CDLFILE) == 0x24, "AA_CDLFILE");

// The movie file being streamed (AA_infile, sbss 0x0193f4f0: 0x38 bytes in the symtab; ezmpegstr's StrFile
// plus CT3's openError). layout: host -- iopBuf is a host pointer here (the ROM's EE offsets beside each).
typedef struct StrFile {
    int isOnCD;             /* EE 0x00: STRFILE_HOST / _CD / _PACKED */
    int size;               /* EE 0x04: the file's bytes (readMpeg streams this many) */
    AA_CDLFILE fp;          /* EE 0x08: the disc record (lsn, size) */
    void *iopBuf;           /* EE 0x2c: sceSifAllocSysMemory(0, STRFILE_IOP_BUF_SIZE) (AA_strFileInit) */
    int fd;                 /* EE 0x30: sceOpen's descriptor (STRFILE_HOST) */
    int openError;          /* EE 0x34: AA_strFileOpen gave up for good: AA_movie skips readMpeg */
} StrFile;

// The read ring between the stream file and the demuxer (AA_readBuf, sbss 0x0193f6b0: 0x10 bytes;
// ezmpegstr's ReadBuf). AA_MemoryMapSet allocates its data (gHeapSound). layout: host (data's pointer).
typedef struct ReadBuf {
    u_char *data;           /* EE 0x00: _SunAlloc(gHeapSound, AA_READBUF_SIZE) */
    int put;                /* EE 0x04: where the next bytes go */
    int count;              /* EE 0x08: bytes in the ring not yet handed to the demuxer */
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
void AA_strFileInit(StrFile *file);                                                      // CT3 0x018c02d0
