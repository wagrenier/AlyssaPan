// game/AA_mpegTS.c -- Clock Tower 3: the movie's video decoder (SCE's ezmpegstr videodec.c, AA_-prefixed)
// and its decoded-picture queue (vobuf.c). Written 2026-11-12, all 31 functions.
//
// The decoder thread (AA_videoDecMain, started by movie.c's initAll) decodes picture after picture into
// the two picture buffers (AA_decBs0); for the first it builds both pictures' GS records the vblank sends
// (AA_setImageTag, AA_GetDataWord.c), and queues each (AA_voBufIncCount). The stream ring (ViBuf,
// AA_viBuf.c) is fed by AA_videoCallback through AA_videoDecBeginPut / EndPut / PutTs. CT3 adds the
// fade-in: AA_ChangeFedeInPlene patches the four fade sprites' RGBAQ words while AA_FadeInCount counts
// down, and AA_ReTagSet builds the fade-out's block.
//
// PORT: the movie banks' EE addresses (picture buffers, GS tag blocks, the stream ring, the work area)
// become host pointers through BankEePtr of their AA_MovieUseBank row (open-items.md (ha)); the
// uncached-alias arithmetic (| 0x20000000) is dropped; sdk/libmpeg stands in for the IPU.

#include "game/AA_mpegTS.h"

#include "game/AA_GetDataWord.h"
#include "game/AA_scTag2.h"
#include "game/AA_viBuf.h"
#include "game/ct3_types.h"
#include "game/movie.h"
#include "sdk/eekernel.h"

#include <stdio.h>

#define AA_FADE_IN_FRAMES 0x3c          /* AA_FadeInCount's start (movie.c's AA_FADE_FRAMES) */
#define AA_RGBAQ_Q_ONE 0x80000000u      /* the fade sprite's RGBAQ at rest */
#define AA_RGBAQ_A_MIN 0x01000000u      /* the black fade's last alpha (1, not 0) */
#define AA_SEQ_END_SIZE 4

static int w_image_w;                   /* sbss 0x0193f734: the movie's width (first picture) */
static int w_image_h;                   /* sbss 0x0193f730 */

static const u_char s_seqEndCode[AA_SEQ_END_SIZE] = { 0x00, 0x00, 0x01, 0xb7 }; /* @48 0x01939c50 */

// PORT: the caption cursor. The ROM hands AA_TestRead &AA_MojiData, the captions' EE address, and lets it
// advance; the host reads from AA_MojiImage, so the cursor starts there with each movie (open-items.md (gx)).
static const u_char *s_mojiCursor;

// PORT: an EE address in a movie bank (AA_MovieUseBank's row) as a host pointer.
static void *AA_movieHost(int row, u_int ee)
{
    return BankEePtr(AA_MovieUseBank[row].bank, ee);
}

// CT3 0x018c1680: libmpeg's time stamp callback: the next stamp of the stream ring.
int AA_mpegTS(sceMpeg *mp, sceMpegCbData *cbdata, void *data)
{
    TimeStamp ts;

    (void)mp;
    (void)data;
    AA_viBufGetTs(&AA_videoDec.vibuf, &ts);
    cbdata->ts.pts = ts.pts;
    cbdata->ts.dts = ts.dts;
    return 1;
}

// CT3 0x018c16c0: libmpeg resumes the IPU's input DMA.
int AA_mpegRestartDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *data)
{
    (void)mp;
    (void)cbdata;
    (void)data;
    AA_viBufRestartDMA(&AA_videoDec.vibuf);
    return 1;
}

// CT3 0x018c16e0: libmpeg stops it.
int AA_mpegStopDMA(sceMpeg *mp, sceMpegCbData *cbdata, void *data)
{
    (void)mp;
    (void)cbdata;
    (void)data;
    AA_viBufStopDMA(&AA_videoDec.vibuf);
    return 1;
}

// CT3 0x018c1700: the IPU wants data: let the reader run, then hand the DMA what the ring holds.
int AA_mpegNodata(sceMpeg *mp, sceMpegCbData *cbdata, void *data)
{
    (void)mp;
    (void)cbdata;
    (void)data;
    AA_switchThread();
    AA_viBufAddDMA(&AA_videoDec.vibuf);
    return 1;
}

// CT3 0x018c1730: a decode error: printed, decoding goes on.
int AA_mpegError(sceMpeg *mp, sceMpegCbData *cbdata, void *data)
{
    (void)mp;
    (void)data;
    printf("%s\n", cbdata->error.errMessage);       /* @174 0x01935718 */
    return 1;
}

// CT3 0x018c1760: the fade-out's block (movie.c's fade-out): AA_setImageTag2 over the movie's size, its
// fade word to AA_FadePackAdr.
void AA_ReTagSet(void *tag)
{
    if (tag != NULL)
        AA_setImageTag2(tag, w_image_w, w_image_h, &AA_FadePackAdr);
}

// CT3 0x018c1a00: while the fade words are set, writes the fade-in's level into all four: with AA_MV_WHITE
// a grey of count * 256 / 60 per channel (from white down to none as AA_FadeInCount runs out), with
// AA_MV_FADE a black whose alpha falls from 0x80 to 1; else at rest.
static void AA_ChangeFedeInPlene(void)
{
    u_int rgbaq, level;
    int alpha, k;

    if (voBuf.fadeReady == 0)
        return;
    rgbaq = AA_RGBAQ_Q_ONE;
    if (AA_MvInWhite == AA_MV_WHITE) {
        int v = (AA_FadeInCount * 0x100) / AA_FADE_IN_FRAMES;

        level = v < 0 ? 0 : (u_int)v;
        rgbaq = level | (level << 8) | (level << 16) | AA_RGBAQ_Q_ONE;
    } else if (AA_MvInWhite == AA_MV_FADE) {
        alpha = 0x80 - (AA_FadeInCount * 0x80) / AA_FADE_IN_FRAMES;
        rgbaq = alpha < 1 ? AA_RGBAQ_A_MIN : (u_int)alpha << 24;
    }
    for (k = 0; k < VOBUF_FADES; k++)
        *voBuf.fade[k] = rgbaq;
    FlushCache(0);
}

// CT3 0x018c1790: the decode loop (ezmpegstr's decBs0, with CT3's captions and fade). Until the stream
// ends: stop on an abort (-1); wait for a free picture buffer; drop B pictures while the video lags; decode
// into the buffer; play the captions due; on the first picture build both pictures' GS records (the size
// comes from it) and arm the fade words; queue the picture.
int AA_decBs0(VideoDec *vd)
{
    int status = 1;
    u_int *entry;
    int i;

    s_mojiCursor = AA_MojiData != 0 ? AA_MojiImage : NULL;
    while (!sceMpegIsEnd(&vd->mpeg)) {
        if (AA_videoDecGetState(vd) == VD_STATE_ABORT) {
            AA_FadeInCount = 0;
            status = -1;
            AA_ChangeFedeInPlene();
            printf("decode thread: aborted\n");     /* @213 0x01935720 */
            break;
        }
        while ((entry = AA_voBufGetData(&voBuf)) == NULL) {
            AA_ChangeFedeInPlene();
            AA_switchThread();
        }
        if (AA_shouldSkip(vd) == 0) {
            if (vd->skipping != 0) {
                sceMpegSetDecodeMode(&vd->mpeg, -1, -1, -1);
                vd->skipping = 0;
            }
        } else if (vd->skipping == 0) {
            sceMpegSetDecodeMode(&vd->mpeg, -1, -1, 0);
            vd->skipping = 1;
        }
        i = sceMpegGetPicture(&vd->mpeg,
                              (sceIpuRGB32 *)AA_movieHost(AA_MOVIE_ROW_PICTURE0 + voBuf.write, *entry),
                              AA_PICTURE_BLOCKS);
        AA_ChangeFedeInPlene();
        if (i < 0)
            ErrMessage("sceMpegGetPicture() decode error");     /* @214 0x01935740 */
        AA_TestRead(&s_mojiCursor, vd->mpeg.frameCount);
        if (vd->mpeg.frameCount == 0) {
            w_image_w = vd->mpeg.width;
            w_image_h = vd->mpeg.height;
            for (i = 0; i < voBuf.size; i++) {
                const void *picture = AA_movieHost(AA_MOVIE_ROW_PICTURE0 + i, voBuf.data[i]);
                int row = AA_MOVIE_ROW_GS_TAG0 + i;

                AA_setImageTag(AA_movieHost(row, voBuf.tag[i].tag[0]), picture, 0, w_image_w, w_image_h,
                               &voBuf.fade[i * 2]);
                AA_setImageTag(AA_movieHost(row, voBuf.tag[i].tag[1]), picture, 1, w_image_w, w_image_h,
                               &voBuf.fade[i * 2 + 1]);
            }
            voBuf.fadeReady = 1;
        }
        AA_voBufIncCount(&voBuf, (u_long)vd->mpeg.pts);
        AA_switchThread();
    }
    sceMpegReset(&vd->mpeg);
    return status;
}

// CT3 0x018c1b00: the decoder thread's body: reset the rings, decode, wait until the vblank has shown every
// queued picture (the ROM spins), then say so.
void AA_videoDecMain(void *videoDec)
{
    VideoDec *vd = (VideoDec *)videoDec;

    AA_viBufReset(&vd->vibuf);
    AA_voBufReset(&voBuf);
    AA_decBs0(vd);
    while (((volatile VoBuf *)&voBuf)->count != 0) {
    }
    AA_videoDecSetState(vd, VD_STATE_END);
}

// CT3 0x018c1b60: nothing left in the ring and no reference picture held back.
int AA_videoDecIsFlushed(VideoDec *vd)
{
    return AA_videoDecInputCount(vd) == 0 && sceMpegIsRefBuffEmpty(&vd->mpeg) != 0;
}

// CT3 0x018c1ba0: ends the stream: writes the sequence end code into the ring (0 when there is no room for
// its 4 bytes), flushes the ring to the IPU, and marks the decoder flushing.
int AA_videoDecFlush(VideoDec *vd)
{
    u_char *ptr0 = NULL, *ptr1 = NULL;     /* PORT: set, so a stubbed ring answers "no room" */
    int len0 = 0, len1 = 0, n;

    AA_videoDecBeginPut(vd, &ptr0, &len0, &ptr1, &len1);
    if (len0 + len1 < AA_SEQ_END_SIZE)
        return 0;
    // PORT: the ROM writes through the uncached alias (addr & 0x0fffffff | 0x20000000).
    n = AA_cpy2area(ptr0, len0, ptr1, len1, s_seqEndCode, AA_SEQ_END_SIZE, NULL, 0);
    AA_videoDecEndPut(&AA_videoDec, n);
    AA_viBufFlush(&vd->vibuf);
    if (vd->state == VD_STATE_NORMAL)
        vd->state = VD_STATE_FLUSH;
    return 1;
}

// CT3 0x018c1c70: the bytes in the ring not yet decoded (a tail jump into the ring).
int AA_videoDecInputCount(VideoDec *vd)
{
    return AA_viBufCount(&vd->vibuf);
}

// CT3 0x018c1c80: a payload's time stamp, by its place in the ring; answers the ring's (0: the stamp
// ring is full, AA_videoCallback's "pts buffer overflow").
int AA_videoDecPutTs(VideoDec *vd, int64_t pts, int64_t dts, u_char *start, int len)
{
    TimeStamp ts;

    ts.pts = pts;
    ts.dts = dts;
    ts.pos = (int)(start - vd->vibuf.data);
    ts.len = len;
    return AA_viBufPutTs(&AA_videoDec.vibuf, &ts);
}

// CT3 0x018c1cc0: sets the state, answers the old one.
int AA_videoDecSetState(VideoDec *vd, int state)
{
    int old = vd->state;

    vd->state = state;
    return old;
}

// CT3 0x018c1cd0
int AA_videoDecGetState(VideoDec *vd)
{
    return vd->state;
}

// CT3 0x018c1ce0: readMpeg's pad skip: the decode loop stops at its next picture.
void AA_videoDecAbort(VideoDec *vd)
{
    vd->state = VD_STATE_ABORT;
}

// CT3 0x018c1cf0
int AA_videoDecDelete(VideoDec *vd)
{
    AA_viBufDelete(&vd->vibuf);
    sceMpegDelete(&vd->mpeg);
    return 1;
}

// CT3 0x018c1d30
void AA_videoDecReset(VideoDec *vd)
{
    int k;

    vd->state = VD_STATE_NORMAL;
    vd->skipping = 0;
    for (k = 0; k < VD_RESET_WORDS - 1; k++)
        vd->wB4[k] = 0;
}

// CT3 0x018c1d60: `size` bytes were written where AA_videoDecBeginPut said (a tail jump into the ring).
void AA_videoDecEndPut(VideoDec *vd, int size)
{
    AA_viBufEndPut(&vd->vibuf, size);
}

// CT3 0x018c1d70: the ring's free room, in one or two pieces (a tail jump into the ring).
void AA_videoDecBeginPut(VideoDec *vd, u_char **ptr0, int *len0, u_char **ptr1, int *len1)
{
    AA_viBufBeginPut(&vd->vibuf, ptr0, len0, ptr1, len1);
}

// CT3 0x018c1d80
int AA_videoDecSetStream(VideoDec *vd, int strType, int ch, sceMpegCallback cb, void *data)
{
    sceMpegAddStrCallback(&vd->mpeg, (sceMpegStrType)strType, ch, cb, data);
    return 1;
}

// CT3 0x018c1da0: libmpeg in the work area, the five callbacks, the stream ring.
int AA_videoDecCreate(VideoDec *vd, u_int mpegWork, int workSize, u_int viBufData, void *viBufTag,
                      int tagCount, void *timeStamp, int timeStamps)
{
    sceMpegCreate(&vd->mpeg, AA_movieHost(AA_MOVIE_ROW_DECODE_WORK, mpegWork), workSize);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbError, AA_mpegError, NULL);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbNodata, AA_mpegNodata, NULL);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbStopDMA, AA_mpegStopDMA, NULL);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbRestartDMA, AA_mpegRestartDMA, NULL);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbTimeStamp, AA_mpegTS, NULL);
    AA_videoDecReset(vd);
    AA_viBufCreate(&vd->vibuf, (u_char *)AA_movieHost(AA_MOVIE_ROW_VIDEO_STREAM, viBufData), viBufTag,
                   tagCount, (TimeStamp *)timeStamp, timeStamps);
    return 1;
}

// ---- vobuf.c ---------------------------------------------------------------------------------------------

// CT3 0x018c1ea0: the vblank showed a picture (AA_handler_endimage).
void AA_voBufDecCount(VoBuf *f)
{
    if (f->count > 0)
        f->count--;
}

// CT3 0x018c1ec0: the oldest queued picture's record, NULL when none.
VoTag *AA_voBufGetTag(VoBuf *f)
{
    if (AA_voBufIsEmpty(f))
        return NULL;
    return &f->tag[(f->size + f->write - f->count) % f->size];
}

// CT3 0x018c1f30
int AA_voBufIsEmpty(VoBuf *f)
{
    return f->count == 0;
}

// CT3 0x018c1f40: the picture buffer to decode into (its data[] entry), NULL when all are full.
u_int *AA_voBufGetData(VoBuf *f)
{
    if (AA_voBufIsFull(f))
        return NULL;
    return &f->data[f->write];
}

// CT3 0x018c1f90: the picture at `write` is decoded: mark its record full with its stamp, move on.
void AA_voBufIncCount(VoBuf *f, u_long pts)
{
    DIntr();
    f->tag[f->write].status = VOBUF_STATUS_FULL;
    f->tag[f->write].pts = pts;
    f->count++;
    f->write = (f->write + 1) % f->size;
    EIntr();
}

// CT3 0x018c2030
int AA_voBufIsFull(VoBuf *f)
{
    return f->count == f->size;
}

// CT3 0x018c2050: empty, and the display's pacing back to its start.
void AA_voBufReset(VoBuf *f)
{
    int k;

    f->count = 0;
    f->write = 0;
    f->delayHold = VOBUF_DELAY_HOLD_RESET;
    for (k = 0; k < VOBUF_SKEWS; k++)
        f->skew[k] = 0;
    f->skewIndex = 0;
}

// CT3 0x018c2080
void AA_voBufDelete(VoBuf *f)
{
    (void)f;
}

// CT3 0x018c2090: `n` pictures over data[] / tag[], all records free.
void AA_voBufCreate(VoBuf *f, u_int *data, VoTag *tag, int n)
{
    int i;

    f->data = data;
    f->tag = tag;
    f->size = n;
    f->count = 0;
    f->write = 0;
    for (i = 0; i < n; i++)
        f->tag[i].status = VOBUF_STATUS_FREE;
}
