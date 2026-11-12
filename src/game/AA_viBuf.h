// game/AA_viBuf.h -- CT3 game/AA_viBuf.c: the movie's video stream ring (SCE's ezmpegstr vibuf.c,
// AA_-prefixed), the type game/AA_mpegTS.c embeds in VideoDec. Written 2026-11-12 (all 13 functions).
#pragma once

#include "sdk/scetypes.h"

#include <stdint.h>

#define VIBUF_ELM_SIZE 0x800            /* one DMA tag's block of the ring: 2048 bytes */
#define VIBUF_TS_SIZE 0x18              /* sizeof(TimeStamp) on the EE */
#define VIBUF_TAG_SIZE 0x10             /* one DMA tag of the chain (a qword) */
#define VIBUF_ELM_QWC (VIBUF_ELM_SIZE / 16)

// The ring's chain: REF tags (id 3) over the blocks the DMA holds, a REFE (id 0) on the last, and past
// the n tags a NEXT (id 2) back to the first (AA_viBufReset).
#define VIBUF_TAG_REFE 0
#define VIBUF_TAG_NEXT 2
#define VIBUF_TAG_REF 3

// D3 / D4 CHCR bits the ring writes: from memory in chain mode (5) stops the channel, STR starts it, the
// TAG field's id (bits 28..30) says which tag the chain resumes after (REF: read the next one at TADR).
#define VIBUF_CHCR_STOP 0x5u
#define VIBUF_CHCR_STR 0x100u
#define VIBUF_CHCR_TAG_ID_SHIFT 28
#define VIBUF_CHCR_KEEP 0x0fffffffu

// IPU_BP: the bit pointer (bits 0..6), the FIFO's quadwords (IFC, bits 8..11; FP, bits 16..17).
#define IPU_BP_BP(bp) ((bp) & 0x7f)
#define IPU_BP_FIFO_QWC(bp) ((((bp) >> 16) & 3) + (((bp) >> 8) & 0xf))
#define IPU_CTRL_BUSY 0x80000000u
#define IPU_CTRL_IFC_PENDING 0xf0u

// PORT: the DMA addresses of the chain (tag ADDR, D4 MADR / TADR) are not host pointers -- no DMA reads
// them and a host pointer does not fit their 32 bits. The ring's bytes sit at VIBUF_DMA_DATA + offset and
// the tag list at VIBUF_DMA_TAG + offset; the ROM's arithmetic on (u_int)f->data and f->tag & 0x0fffffff
// runs on these bases unchanged (AA_getFIFOindex in AA_scTag2.c reads them the same way).
#define VIBUF_DMA_DATA 0x10000000u
#define VIBUF_DMA_TAG 0x20000000u

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
// blocks from dmaStart, the callback has written readBytes past them. PORT, layout: host -- data, tag and ts
// are host pointers (AA_videoDecCreate resolves the movie bank's EE address), so the EE offsets are
// comments. The ROM stores tag as the uncached alias (| 0x20000000); the host keeps the plain pointer.
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
// readBytes rounded up to a whole block (the stream's last bytes become a block the DMA can take).
void AA_viBufFlush(ViBuf *f);                                                            // CT3 0x018c08f0
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
int AA_viBufRestartDMA(ViBuf *f);                                                        // CT3 0x018c09e0
int AA_viBufStopDMA(ViBuf *f);                                                           // CT3 0x018c0ce0
// Retires the blocks the IPU has read and hands it the whole blocks written since (libmpeg's Nodata).
int AA_viBufAddDMA(ViBuf *f);                                                            // CT3 0x018c0dc0
int AA_viBufReset(ViBuf *f);                                                             // CT3 0x018c1160
int AA_viBufCreate(ViBuf *f, u_char *data, void *tag, int n, TimeStamp *ts, int n_ts);   // CT3 0x018c12c0
// Called only by AA_viBufPutTs: trims the queued stamps whose bytes a new stamp's bytes overlap.
int AA_viBufModifyPts(ViBuf *f, TimeStamp *ts);                                          // CT3 0x018c0750
