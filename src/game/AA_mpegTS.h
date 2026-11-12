// game/AA_mpegTS.h -- CT3 game/AA_mpegTS.c: the movie's video decoder and picture queue (SCE's ezmpegstr
// videodec.c / vobuf.c, AA_-prefixed), the types game/movie.c defines objects of (2026-11-10; this TU
// written 2026-11-12).
#pragma once

#include "sdk/scetypes.h"
#include "sdk/libmpeg.h"
#include "game/AA_viBuf.h"

#include <stddef.h>

// AA_videoDecGetState's answers (ezmpegstr's VdState).
#define VD_STATE_NORMAL 0
#define VD_STATE_ABORT 1                /* AA_videoDecAbort: readMpeg's pad skip; AA_decBs0 stops */
#define VD_STATE_FLUSH 2                /* AA_videoDecFlush wrote the sequence end */
#define VD_STATE_END 3                  /* AA_videoDecMain is through; readMpeg's loops stop on it */

#define VD_RESET_WORDS 8                /* AA_videoDecReset clears 0xb0..0xcc */

// The video decoder (AA_videoDec, sbss 0x0193f410: 0xd8 bytes in the symtab): the libmpeg context, the
// stream ring, the state. initAll / termAll keep the two interrupt handler ids at +0xd0 / +0xd4.
// PORT, layout: host -- ViBuf holds host pointers; the EE offsets are comments (sceMpeg is 0x48 both ways).
typedef struct VideoDec {
    sceMpeg mpeg;           /* EE 0x00 (frameCount +8: readMpeg's skip needs more than 15) */
    ViBuf vibuf;            /* EE 0x48 */
    int state;              /* EE 0xa8: VD_STATE_* */
    int wAC;                /* EE 0xac: never read here */
    int skipping;           /* EE 0xb0: AA_decBs0 told libmpeg to drop B pictures (first word of the reset) */
    int wB4[VD_RESET_WORDS - 1]; /* EE 0xb4..0xcc: cleared by AA_videoDecReset, not read here */
    int hid_endimage;       /* EE 0xd0: AddDmacHandler(DMAC_GIF, AA_handler_endimage) */
    int hid_vblank;         /* EE 0xd4: AddIntcHandler(INTC_VBLANK_S, AA_vblankHandler) */
} VideoDec;
static_assert(offsetof(VideoDec, vibuf) == 0x48, "VideoDec.vibuf");

// The decoded-picture queue (voBuf, sbss 0x0193f340: 0x48 bytes on the EE). A ring of `size` pictures:
// data[] the picture buffers, tag[] their GS records; the decoder writes at `write`, `count` are full.
// CT3 grew the sample's 0x14 bytes with the display's pacing (AA_GetDataWord.c) and the fade words
// (AA_decBs0 / AA_ChangeFedeInPlene). PORT, layout: host (data / tag / fade are host pointers).
#define VOBUF_SKEWS 6                   /* shouldDelay's history of AA_syncGetSkew answers */
#define VOBUF_FADES 4                   /* fade words: the two pictures' even / odd blocks */
#define VOBUF_DELAY_HOLD_RESET 10       /* AA_voBufReset's delayHold */
typedef struct VoBuf {
    u_int *data;            /* EE 0x00: AA_voBufData: each entry a picture's EE address (a movie bank) */
    struct VoTag *tag;      /* EE 0x04: AA_voBufTag */
    int write;              /* EE 0x08: the next picture the decoder fills */
    int count;              /* EE 0x0c: pictures decoded and not yet shown */
    int size;               /* EE 0x10: AA_VOBUF_COUNT */
    int delayHold;          /* EE 0x14: vblanks left before AA_vblankHandler asks shouldDelay again */
    int skew[VOBUF_SKEWS];  /* EE 0x18: the last six skews, newest at skewIndex - 1 */
    int skewIndex;          /* EE 0x30: the next skew slot, mod VOBUF_SKEWS */
    int fadeReady;          /* EE 0x34: the fade words are set: the vblank counts AA_FadeInCount down */
    u_int *fade[VOBUF_FADES]; /* EE 0x38: RGBAQ words of the fade sprites (AA_setImageTag's last argument):
                                 [0] / [1] picture 0's even / odd block, [2] / [3] picture 1's */
} VoBuf;

// One picture's GS upload tags (AA_voBufTag[2], sbss 0x0193f6d0: 0x30 bytes). The two words at +0x10 are
// the tag blocks AA_MovieUseBank gives a movie bank each ("MPEG GS tag 0" / "MPEG Gs tag 1"; the ROM puts
// the second one 0x20000 bytes after the first in AA_MemoryMapSet): tag[0] goes out on the even field,
// tag[1] on the odd one (AA_vblankHandler). status and pts are written by AA_voBufIncCount. layout: ee.
#define VOBUF_STATUS_FREE 0             /* the odd field went out: the buffer is the decoder's again */
#define VOBUF_STATUS_TOPDONE 1          /* the even field went out, the odd one is due */
#define VOBUF_STATUS_FULL 2             /* decoded, waiting for an even field */
typedef struct VoTag {
    int status;             /* 0x00: VOBUF_STATUS_* */
    int w04;
    u_long pts;             /* 0x08: the picture's time stamp (AA_syncClockInVideo's argument) */
    u_int tag[2];           /* 0x10: EE addresses of the picture's tag blocks */
} VoTag;
static_assert(sizeof(VoTag) == 0x18, "VoTag");
static_assert(offsetof(VoTag, pts) == 0x08, "VoTag.pts");
static_assert(offsetof(VoTag, tag) == 0x10, "VoTag.tag");

#define AA_VOBUF_COUNT 2                /* picture buffers (initAll's AA_voBufCreate count) */
#define AA_PICTURE_BLOCKS 0x400         /* sceMpegGetPicture's room: 1024 macroblocks (512 x 512) */

// libmpeg callbacks (registered by AA_videoDecCreate).
int AA_mpegTS(sceMpeg *mp, sceMpegCbData *cbdata, void *data);                           // CT3 0x018c1680
int AA_mpegRestartDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *data);                   // CT3 0x018c16c0
int AA_mpegStopDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *data);                      // CT3 0x018c16e0
int AA_mpegNodata(sceMpeg *mp, sceMpegCbData *cbdata, void *data);                       // CT3 0x018c1700
int AA_mpegError(sceMpeg *mp, sceMpegCbData *cbdata, void *data);                        // CT3 0x018c1730

void AA_ReTagSet(void *tag);                                                             // CT3 0x018c1760 (host tag block)
int AA_decBs0(VideoDec *vd);                                                             // CT3 0x018c1790
void AA_videoDecMain(void *videoDec);                                                    // CT3 0x018c1b00
int AA_videoDecIsFlushed(VideoDec *vd);                                                  // CT3 0x018c1b60
int AA_videoDecFlush(VideoDec *vd);                                                      // CT3 0x018c1ba0
int AA_videoDecInputCount(VideoDec *vd);                                                 // CT3 0x018c1c70
int AA_videoDecPutTs(VideoDec *vd, int64_t pts, int64_t dts, u_char *start, int len);   // CT3 0x018c1c80
int AA_videoDecSetState(VideoDec *vd, int state);                                        // CT3 0x018c1cc0
int AA_videoDecGetState(VideoDec *vd);                                                   // CT3 0x018c1cd0
void AA_videoDecAbort(VideoDec *vd);                                                     // CT3 0x018c1ce0
int AA_videoDecDelete(VideoDec *vd);                                                     // CT3 0x018c1cf0
void AA_videoDecReset(VideoDec *vd);                                                     // CT3 0x018c1d30
void AA_videoDecEndPut(VideoDec *vd, int size);                                          // CT3 0x018c1d60
void AA_videoDecBeginPut(VideoDec *vd, u_char **ptr0, int *len0, u_char **ptr1, int *len1); // CT3 0x018c1d70
int AA_videoDecSetStream(VideoDec *vd, int strType, int ch, sceMpegCallback cb, void *data); // CT3 0x018c1d80
// mpegWork and viBufData are movie banks' EE addresses (AA_MovieUseBank); the rest gHeapSound blocks.
int AA_videoDecCreate(VideoDec *vd, u_int mpegWork, int workSize, u_int viBufData, void *viBufTag,
                      int tagCount, void *timeStamp, int timeStamps);                    // CT3 0x018c1da0
void AA_voBufDecCount(VoBuf *f);                                                         // CT3 0x018c1ea0
VoTag *AA_voBufGetTag(VoBuf *f);                                                         // CT3 0x018c1ec0 (NULL: nothing to show)
int AA_voBufIsEmpty(VoBuf *f);                                                           // CT3 0x018c1f30
u_int *AA_voBufGetData(VoBuf *f);                                                        // CT3 0x018c1f40 (NULL: full)
void AA_voBufIncCount(VoBuf *f, u_long pts);                                             // CT3 0x018c1f90
int AA_voBufIsFull(VoBuf *f);                                                            // CT3 0x018c2030
void AA_voBufReset(VoBuf *f);                                                            // CT3 0x018c2050
void AA_voBufDelete(VoBuf *f);                                                           // CT3 0x018c2080
void AA_voBufCreate(VoBuf *f, u_int *data, VoTag *tag, int n);                           // CT3 0x018c2090
