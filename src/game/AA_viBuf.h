// game/AA_viBuf.h -- CT3 game/AA_viBuf.c: the movie's video stream ring (SCE's ezmpegstr vibuf.c,
// AA_-prefixed), the type game/AA_mpegTS.c embeds in VideoDec (2026-11-12). Fields named from
// AA_mpegTS.c's reads (AA_videoDecBeginPut / EndPut / InputCount, ezmpegstr's names); prototypes from its
// calls. The TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <stdint.h>

#define VIBUF_ELM_SIZE 0x800            /* one DMA tag's block of the ring: 2048 bytes */
#define VIBUF_TS_SIZE 0x18              /* sizeof(TimeStamp) on the EE */

// A video time stamp: the PES packet's pts / dts and where its payload sits in the ring (AA_videoDecPutTs;
// AA_mpegTS reads pts / dts back). layout: ee (int64_t for the EE's 64-bit long).
typedef struct TimeStamp {
    int64_t pts;            /* 0x00 */
    int64_t dts;            /* 0x08 */
    int pos;                /* 0x10: byte offset of the payload in the ring */
    int len;                /* 0x14 */
} TimeStamp;
static_assert(sizeof(TimeStamp) == VIBUF_TS_SIZE, "TimeStamp");

// The IPU's DMA state the ring saves around a stop (ezmpegstr's sceIpuDmaEnv: D4 / D3 channels, BP, CTRL).
typedef struct ViBufDmaEnv {
    u_int d4madr, d4tadr, d4qwc, d4chcr;
    u_int d3madr, d3qwc, d3chcr;
    u_int ipubp, ipuctrl;
} ViBufDmaEnv;
static_assert(sizeof(ViBufDmaEnv) == 0x24, "ViBufDmaEnv");

// The ring (VideoDec +0x48, 0x60 bytes on the EE): `n` blocks of VIBUF_ELM_SIZE; the IPU's DMA holds dmaN
// blocks from dmaStart, the callback has written readBytes past them. PORT, layout: host -- data and tag are
// host pointers (AA_videoDecCreate resolves the movie bank's EE address), so the EE offsets are comments.
typedef struct ViBuf {
    u_char *data;           /* EE 0x00: the ring (the "MPEG Video stream" banks) */
    void *tag;              /* EE 0x04: the DMA tag list (AA_viBufTag) */
    int n;                  /* EE 0x08: blocks in the ring */
    int dmaStart;           /* EE 0x0c: first block the DMA holds */
    int dmaN;               /* EE 0x10: blocks the DMA holds */
    int readBytes;          /* EE 0x14: bytes written after them */
    int buffSize;           /* EE 0x18: n * VIBUF_ELM_SIZE */
    ViBufDmaEnv env;        /* EE 0x1c */
    int sema;               /* EE 0x40: guards the counts (WaitSema / SignalSema) */
    int isActive;           /* EE 0x44 */
    int64_t totalBytes;     /* EE 0x48: bytes ever written */
    TimeStamp *ts;          /* EE 0x50: the time stamp ring (AA_timeStamp) */
    int n_ts;               /* EE 0x54 */
    int count_ts;           /* EE 0x58 */
    int wt_ts;              /* EE 0x5c */
} ViBuf;

int AA_viBufGetTs(ViBuf *f, TimeStamp *ts);                                              // CT3 0x018c0440
int AA_viBufPutTs(ViBuf *f, TimeStamp *ts);                                              // CT3 0x018c0620
int AA_viBufFlush(ViBuf *f);                                                             // CT3 0x018c08f0
// The bytes not yet decoded: dmaN * VIBUF_ELM_SIZE + readBytes, under the sema (Ghidra inlined it into
// AA_videoDecInputCount, a tail jump).
int AA_viBufCount(ViBuf *f);                                                             // CT3 0x018c0940
// readBytes and totalBytes += size, under the sema (inlined into AA_videoDecEndPut).
void AA_viBufEndPut(ViBuf *f, int size);                                                 // CT3 0x018c1000
// The free room after the written bytes, under the sema: at = ((dmaStart + dmaN) * VIBUF_ELM_SIZE +
// readBytes) % buffSize, room = (n - 2 - dmaN) * VIBUF_ELM_SIZE - readBytes; when buffSize - at < room the
// room wraps (ptr0 = data + at for buffSize - at, ptr1 = data for the rest), else ptr1 NULL / len1 0
// (inlined into AA_videoDecBeginPut).
void AA_viBufBeginPut(ViBuf *f, u_char **ptr0, int *len0, u_char **ptr1, int *len1);   // CT3 0x018c1060
int AA_viBufDelete(ViBuf *f);                                                            // CT3 0x018c0990
void AA_viBufRestartDMA(ViBuf *f);                                                       // CT3 0x018c09e0
void AA_viBufStopDMA(ViBuf *f);                                                          // CT3 0x018c0ce0
void AA_viBufAddDMA(ViBuf *f);                                                           // CT3 0x018c0dc0
int AA_viBufReset(ViBuf *f);                                                             // CT3 0x018c1160
int AA_viBufCreate(ViBuf *f, u_char *data, void *tag, int n, TimeStamp *ts, int n_ts);   // CT3 0x018c12c0
