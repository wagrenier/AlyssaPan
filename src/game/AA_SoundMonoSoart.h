// game/AA_SoundMonoSoart.h -- CT3 game/AA_SoundMonoSoart.c: the movie's audio decoder (SCE's ezmpegstr
// audiodec.c, AA_-prefixed, with CT3's time-stamp rings and mono mode). Written 2026-11-12, all 20.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// AudioDec.state (ezmpegstr's AU_STATE_*).
#define AU_STATE_INIT 0                 /* reading the 40-byte SShd / SSbd header */
#define AU_STATE_PRESET 1               /* header read: fill the IOP ring before the start */
#define AU_STATE_PLAYING 2              /* the SPU2's auto-DMA plays the IOP ring */
#define AU_STATE_PAUSE 3

#define AU_HEADER_SIZE 0x28             /* SpuStreamHeader + SpuStreamBody */
#define AU_UNIT_SIZE 0x400              /* one interleave unit: 0x200 bytes left, 0x200 right */
#define AU_ZERO_BUFF_SIZE 0x800         /* _0_buf / iopZero: the silence the pause writes */

// A PCM time stamp (16 bytes): `len` bytes from `pos` of a ring start at `pts` (90 kHz).
typedef struct AudioTimeStamp {
    u_long pts;                         /* 0x00 */
    int pos;                            /* 0x08 byte offset in the ring */
    int len;                            /* 0x0c */
} AudioTimeStamp;
static_assert(sizeof(AudioTimeStamp) == 0x10, "AudioTimeStamp");

// The stream's header, as the PSS audio stream starts ('SShd', then 'SSbd' and the PCM).
typedef struct SpuStreamHeader {
    char id[4];                         /* 0x00 'SShd' */
    int size;                           /* 0x04 */
    int type;                           /* 0x08 0 / 1: 16-bit PCM, 2: SPU2 ADPCM */
    int rate;                           /* 0x0c */
    int ch;                             /* 0x10 */
    int interSize;                      /* 0x14 */
    int loopStart;                      /* 0x18 */
    int loopEnd;                        /* 0x1c */
} SpuStreamHeader;
static_assert(sizeof(SpuStreamHeader) == 0x20, "SpuStreamHeader");

typedef struct SpuStreamBody {
    char id[4];                         /* 0x00 'SSbd' */
    int size;                           /* 0x04 */
} SpuStreamBody;
static_assert(sizeof(SpuStreamBody) == 0x08, "SpuStreamBody");

// The movie's audio decoder state (AA_audioDec, sbss 0x0193f390: 0x7c bytes in the symtab). layout: host --
// data and the two time-stamp rings are host pointers (EE words at +0x30, +0x44, +0x54); iopBuff / iopZero
// are IOP addresses, below 4 GB on the host too (sdk/sif.cpp). The offsets are the EE's.
typedef struct AudioDec {
    int state;                          /* 0x00 AU_STATE_* */
    SpuStreamHeader sshd;               /* 0x04 */
    SpuStreamBody ssbd;                 /* 0x24 */
    int hdrCount;                       /* 0x2c header bytes read */
    u_char *data;                       /* 0x30 the EE ring (movie.c's AA_audioBuff) */
    int put;                            /* 0x34 */
    int count;                          /* 0x38 bytes waiting */
    int size;                           /* 0x3c */
    int totalBytes;                     /* 0x40 */
    AudioTimeStamp *eeTs;               /* 0x44 stamps of the EE ring */
    int eeTsSize;                       /* 0x48 */
    int eeTsCount;                      /* 0x4c stamps added, never wrapped */
    int eeTsPut;                        /* 0x50 */
    AudioTimeStamp *iopTs;              /* 0x54 stamps of the IOP ring */
    int iopTsSize;                      /* 0x58 */
    int iopTsCount;                     /* 0x5c */
    int iopTsPut;                       /* 0x60 */
    u_int iopBuff;                      /* 0x64 the IOP ring */
    int iopBuffSize;                    /* 0x68 */
    int iopLastPos;                     /* 0x6c */
    int iopPausePos;                    /* 0x70 */
    int totalBytesSent;                 /* 0x74 */
    u_int iopZero;                      /* 0x78 AU_ZERO_BUFF_SIZE of silence */
} AudioDec;
static_assert(offsetof(AudioDec, ssbd) == 0x24 && offsetof(AudioDec, hdrCount) == 0x2c, "AudioDec header");

void AA_SoundMonoSoart(u_char *unit);                                                    // CT3 0x018bb610
void AA_MonSoart(u_char *data, int size);                                                // CT3 0x018bb680
void Ass_StopIopDataTrans(void);                                                         // CT3 0x018bb7c0
int AA_audioDecSendToIOP(AudioDec *ad);                                                  // CT3 0x018bbeb0
int AA_audioDecIsPreset(AudioDec *ad);                                                   // CT3 0x018bc130
// copy2area.c's pair: the free room of the ring (the header first), then the bytes put there and their pts.
void AA_audioDecEndPut(AudioDec *ad, int size, u_long pts);                              // CT3 0x018bc150
void AA_audioDecBeginPut(AudioDec *ad, u_char **ptr0, int *len0, u_char **ptr1, int *len1); // CT3 0x018bc2a0
void AA_audioDecReset(AudioDec *ad);                                                     // CT3 0x018bc360
void AA_audioDecStart(AudioDec *ad);                                                     // CT3 0x018bc3b0
void AA_audioDecResume(AudioDec *ad);                                                    // CT3 0x018bc3c0
void AA_audioDecPause(AudioDec *ad);                                                     // CT3 0x018bc450
int AA_audioDecDelete(AudioDec *ad);                                                     // CT3 0x018bc4d0
// CT3's eight-argument create (ezmpegstr's plus the EE / IOP time-stamp rings): initAll's call.
int audioDecCreate(AudioDec *ad, void *buff, int buffSize, int iopBuffSize, void *eeTimeStamp,
                   int eeTimeStamps, void *iopTimeStamp, int iopTimeStamps);             // CT3 0x018bc4e0
int audioDecInit(AudioDec *ad, int iopBuffSize);                                         // CT3 0x018bc620
