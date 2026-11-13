// game/AA_GetDataWord.c -- CT3 0x018bc690..0x018bda90: the movie's display. SCE's ezmpegstr disp.c
// (startDisplay, endDisplay, vblankHandler, handler_endimage, setImageTag, clearGsMem), AA_-prefixed, grown
// with CT3's captions (AA_TestRead draws a caption file's records into a 4-bit picture the image transfer
// lays over the movie) and an A/V pacing check (shouldDelay holds a picture while the video runs ahead).
//
// Slice 1 (2026-11-12): the caption readers, the caption picture, shouldDelay and the display switches.
// Slice 2 (2026-11-12): the GS side -- AA_GS_SetClut, AA_clearGsMem, AA_setImageTag / AA_setImageTag2 and
// AA_vblankHandler -- whose GIF chains become host records drawn through alyssapan_drawui.h (below).

#include "game/AA_GetDataWord.h"

#include "common/alyssapan_stub.h"
#include "game/AA_cpuCycles.h"
#include "game/AA_mpegTS.h"
#include "game/movie.h"
#include "game/ct3_types.h"
#include "alyssapan/rendering/alyssapan_drawui.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"

#include <string.h>

#define AA_SKEW_DELAY_LIMIT (-6000)     /* shouldDelay: a skew at or under this in every slot holds a picture */

// ---- data ---------------------------------------------------------------------------------------------
int AA_isCountVblank;                                       /* sbss 0x0193f0bc */
int AA_vblankCount;                                         /* sbss 0x0193f0c0 */
int AA_isFrameEnd;                                          /* sbss 0x0193f0c4 */
int AA_oddeven;                                             /* sbss 0x0193f0c8 */
int AA_handler_error;                                       /* sbss 0x0193f0cc */
u_int AA_MojiBuffWk;                                        /* sbss 0x0193f0d0 */

// PORT: the caption picture's host home (AA_GetDataWord.h).
u_char AA_MojiFBImage[AA_MOJI_FB_BYTES];

u_char *AA_MojiFB(void)
{
    return AA_MoiveMojiFB != 0 ? AA_MojiFBImage : NULL;
}

// ---- the caption file's readers --------------------------------------------------------------------------

// CT3 0x018bc690: the next 16-bit little-endian word at *cursor, a byte at a time.
u_short AA_GetDataWord(const u_char **cursor)
{
    u_int lo, hi;

    lo = *(*cursor)++;
    hi = *(*cursor)++;
    return (u_short)(lo + (hi << 8));
}

// CT3 0x018bc6c0: the next 32-bit little-endian word at *cursor.
u_int AA_GetDataLong(const u_char **cursor)
{
    u_int b0, b1, b2, b3;

    b0 = *(*cursor)++;
    b1 = *(*cursor)++;
    b2 = *(*cursor)++;
    b3 = *(*cursor)++;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

// CT3 0x018bc720: the 32-bit little-endian word at `data`, without moving.
u_int AA_GetDataLongRead(const u_char *data)
{
    return data[0] | (data[1] << 8) | (data[2] << 16) | ((u_int)data[3] << 24);
}

// CT3 0x018bc750: plays the caption records due by `time` (the movie's clock): a record whose stamp is
// reached is consumed -- AA_MOJI_PUT copies its rows into the caption picture at (x, y), any other kind
// clears the picture. One record per call; the end mark (-1) stops the list.
void AA_TestRead(const u_char **cursor, int time)
{
    int x, y, w, h, row, col, stamp;
    u_char *fb, *dst;

    if (*cursor == NULL || AA_MoiveMojiFB == 0)
        return;
    stamp = (int)AA_GetDataLongRead(*cursor);
    if (stamp == AA_MOJI_END || time < stamp)
        return;
    AA_GetDataLong(cursor);                         /* the stamp */
    if ((int)AA_GetDataLong(cursor) != AA_MOJI_PUT) {
        AA_CleanMojiBuffer();
        return;
    }
    x = AA_GetDataWord(cursor);
    y = AA_GetDataWord(cursor);
    w = AA_GetDataWord(cursor);
    h = AA_GetDataWord(cursor);
    fb = AA_MojiFB();
    for (row = 0; row < h; row++) {
        dst = fb + (x + (y + row) * AA_MOJI_FB_WIDTH) / 2;
        for (col = 0; col < w; col += 2)
            *dst++ = *(*cursor)++;
    }
}

// ---- the caption picture ------------------------------------------------------------------------------

// CT3 0x018bc8a0: clears the caption picture (0x7000 words), if the movie has one.
void AA_CleanMojiBuffer(void)
{
    u_char *fb = AA_MojiFB();

    if (fb != NULL)
        memset(fb, 0, AA_MOJI_FB_BYTES);
}

// CT3 0x018bc8f0: the picture the upload sends is the caption picture; cleared.
void AA_test_RamInit(void)
{
    AA_MojiBuffWk = AA_MoiveMojiFB;
    AA_CleanMojiBuffer();
}

// ---- pacing and the display's switches ----------------------------------------------------------------

// CT3 0x018bc960: records the skew between picture and audio; 1 when the last VOBUF_SKEWS skews were all
// at or under AA_SKEW_DELAY_LIMIT (the video runs steadily ahead: hold this picture a field longer).
int shouldDelay(void)
{
    int i;

    voBuf.skew[voBuf.skewIndex] = AA_syncGetSkew();
    voBuf.skewIndex = (voBuf.skewIndex + 1) % VOBUF_SKEWS;
    for (i = 0; i < VOBUF_SKEWS; i++) {
        if (voBuf.skew[i] > AA_SKEW_DELAY_LIMIT)
            return 0;
    }
    return 1;
}

// CT3 0x018bc9f0: the vblank stops counting and sending pictures.
void AA_endDisplay(void)
{
    AA_isCountVblank = 0;
    AA_frd = 0;
}

// CT3 0x018bca00: waits for a vblank that leaves the field `waitEven` (readMpeg passes 1: start on an
// even field), then lets the vblank count and send pictures.
void AA_startDisplay(int waitEven)
{
    while (sceGsSyncV(0) == waitEven)
        ;
    AA_frd = 0;
    AA_isCountVblank = 1;
    AA_vblankCount = 0;
}

// CT3 0x018bca50: the GIF transfer's end (the DMAC handler): once the odd field of a picture went out, its
// buffer is free for the decoder. PORT: the ROM's SYNC / EI before the return have no host meaning; the
// host registers no DMAC handler (movie.c's initAll), so slice 2's draw calls this where the transfer ends.
int AA_handler_endimage(int channel)
{
    (void)channel;
    if (AA_isFrameEnd != 0) {
        AA_voBufDecCount(&voBuf);
        AA_isFrameEnd = 0;
    }
    return 0;
}


// ---- the GS side (slice 2, 2026-11-12) -------------------------------------------------------------------
//
// PORT: the ROM's GS side builds GIF chains and sends them by DMA; the host has no GS (the user's rule). The
// tag blocks hold AA_IMAGE_TAG records of the same decisions, and where the vblank sends one, AA_sendImage
// draws it through the host draw module: the picture (AlyssaPan_DrawUiMovie), the caption picture (a 4-bit
// texture with the CLUT AA_GS_SetClut uploads, handed over as a TIM2 picture), the fade (the frame's colour
// scaled by As, plus Cs), then kicks the frame. The ROM draws a field at a time (224 lines, the half offset
// picking the field); the host draws the whole frame on each field. The transfer ends at once on the host:
// the vblank calls AA_handler_endimage after each send, where the ROM's DMAC handler would fire.

// The picture's place in the host frame (UiCanvas: 512 x 512, window 0x7000..0x9000 in 12.4): all of it.
// The ROM's sprite spans the field's 512 x 224 (window 1792..2304 x 1824..2048 with the field's draw env),
// both fields the 448 lines the TV shows as its whole 4:3 picture. PORT: the host frame fills the window at
// any aspect, so the picture and its captions are drawn under AlyssaPan_DrawUiFit: the frame's 4:3 box,
// bars around it on a wider or narrower window, the movie never stretched to the window's shape (the user,
// 2026-11-13). The fades still cover the whole window, as they covered the whole TV.
#define AA_HOST_FRAME 0x200
#define AA_HOST_WINDOW_MIN 0x7000                                       /* 1792.0: the frame's left / top */
#define AA_HOST_WINDOW_MAX 0x9000                                       /* 2304.0: its right / bottom */

// The GS values the ROM's chains carry.
#define AA_GS_PRIM 0x00
#define AA_GS_RGBAQ 0x01
#define AA_GS_ALPHA_1 0x42
#define AA_GS_TEST_1 0x47
#define AA_PRIM_SPRITE 0x006
#define AA_PRIM_CAPTIONS 0x116                  /* sprite, textured, UV */
#define AA_PRIM_FADE 0x046                      /* sprite, blended */
#define AA_CAPTION_TEST 0x3000f                 /* alpha test: pass where A != 0 (CLUT entry 0 is clear) */
#define AA_CAPTION_TEX0 0x2007c00e65423800ull   /* PSMT4 at 0x3800, 512 x 512, TCC, decal, CLUT 0x3e00 */
#define AA_ALPHA_ADD_FIX 0x8000000068ull        /* Cs * FIX + Cd, FIX 0x80: the fade's + Cs on the host */
#define AA_RGBAQ_GREY 0x80404040u               /* the ROM's RGBAQ before a fade patches it */
#define AA_RGBAQ_CLEAR_WHITE 0x80000000u        /* As 0x80, no colour: the fade at rest */
#define AA_UV_ORIGIN 8                          /* the sprites' UV start, half a texel (12.4) */
#define AA_ALPHA_ONE 128.0f

// CT3 0x0192e7c0 `dt` (data, 0x44 bytes): the caption picture's CLUT, sixteen greys from clear black up,
// then a white the upload (8 x 2 colours) never sends.
static u_int dt[17] = {
    0x00000000, 0x800f0f0f, 0x801f1f1f, 0x802f2f2f, 0x803f3f3f, 0x804f4f4f, 0x805f5f5f, 0x806f6f6f,
    0x807f7f7f, 0x808f8f8f, 0x809f9f9f, 0x80afafaf, 0x80bfbfbf, 0x80cfcfcf, 0x80dfdfdf, 0x80efefef,
    0x80ffffff,
};
#define AA_CLUT_COLORS 16

// PORT: what the GS holds between sends -- the last picture uploaded and the captions sent with it, the
// CLUT -- as the TIM2 picture the host draw module decodes (keyed by its content: redecoded only when a
// caption changes).
#define TIM2_FILE_HEADER 0x10
#define TIM2_PICTURE_HEADER 0x30
#define TIM2_IDTEX4 4
#define TIM2_CLUT_RGBA32 3
#define AA_MOJI_TIM2_PIXELS (TIM2_FILE_HEADER + TIM2_PICTURE_HEADER)
#define AA_MOJI_TIM2_CLUT (AA_MOJI_TIM2_PIXELS + AA_MOJI_FB_BYTES)
#define AA_MOJI_TIM2_BYTES (AA_MOJI_TIM2_CLUT + AA_CLUT_COLORS * 4)
static u_char s_gsMoji[AA_MOJI_TIM2_BYTES];
static const void *s_gsPicture;                 /* the macroblocks last uploaded */
static int s_gsPictureCount;                    /* uploads so far: keys the host's movie texture */
static int s_gsPictureWidth, s_gsPictureHeight;
static const char s_movieLayer = 0;             /* the display's draw list (the host's layer key) */

static void AA_put16(u_char *p, u_int v) { p[0] = (u_char)v; p[1] = (u_char)(v >> 8); }
static void AA_put32(u_char *p, u_int v) { AA_put16(p, v); AA_put16(p + 2, v >> 16); }

// PORT: the TIM2 headers around the GS's caption picture and CLUT, written once.
static void AA_gsMojiInit(void)
{
    u_char *pic = s_gsMoji + TIM2_FILE_HEADER;
    u_long tex0 = AA_CAPTION_TEX0;

    if (s_gsMoji[0] == 'T')
        return;
    memcpy(s_gsMoji, "TIM2", 4);
    s_gsMoji[4] = 4;                                            /* format version */
    s_gsMoji[5] = 0;                                            /* the picture follows at 0x10 */
    AA_put16(s_gsMoji + 6, 1);                                  /* pictures */
    AA_put32(pic + 0x00, AA_MOJI_TIM2_BYTES - TIM2_FILE_HEADER);
    AA_put32(pic + 0x04, AA_CLUT_COLORS * 4);
    AA_put32(pic + 0x08, AA_MOJI_FB_BYTES);
    AA_put16(pic + 0x0c, TIM2_PICTURE_HEADER);
    AA_put16(pic + 0x0e, AA_CLUT_COLORS);
    pic[0x10] = 0;                                              /* picture format */
    pic[0x11] = 1;                                              /* mipmaps */
    pic[0x12] = TIM2_CLUT_RGBA32;
    pic[0x13] = TIM2_IDTEX4;
    AA_put16(pic + 0x14, AA_MOJI_FB_WIDTH);
    AA_put16(pic + 0x16, AA_MOJI_FB_HEIGHT);
    AA_put32(pic + 0x18, (u_int)tex0);
    AA_put32(pic + 0x1c, (u_int)(tex0 >> 32));
}

// The vertex of a sprite corner, window and UV in 12.4.
static void AA_corner(int x, int y, int u, int v)
{
    AlyssaPanUiVertex vertex;

    memset(&vertex, 0, sizeof(vertex));
    vertex.x = (uint16_t)x;
    vertex.y = (uint16_t)y;
    vertex.u = (uint16_t)u;
    vertex.v = (uint16_t)v;
    AlyssaPan_DrawUiVertex(&s_movieLayer, &vertex);
}

// PORT: the DMA send of a tag block -- what its chain would have the GS do, drawn and kicked.
static void AA_sendImage(sceDmaChan *gif, void *block)
{
    const AA_IMAGE_TAG *t = (const AA_IMAGE_TAG *)block;
    const void *lists[1] = { &s_movieLayer };
    u_int rgbaq;
    u_char color[4];
    u_char *fb;

    sceDmaSend(gif, block);
    if (t == NULL)
        return;
    AA_gsMojiInit();
    if (t->picture != NULL) {
        // The uploads: the picture's macroblocks, and the caption picture (AA_MojiBuffWk) as it is now.
        s_gsPicture = t->picture;
        s_gsPictureCount++;
        s_gsPictureWidth = t->width;
        s_gsPictureHeight = t->height;
        fb = AA_MojiFB();
        if (fb != NULL)
            memcpy(s_gsMoji + AA_MOJI_TIM2_PIXELS, fb, AA_MOJI_FB_BYTES);
    }

    // The picture, over the whole 4:3 box.
    AlyssaPan_DrawUiFit(&s_movieLayer, 1);
    if (s_gsPicture != NULL)
        AlyssaPan_DrawUiMovie(&s_movieLayer, &voBuf, s_gsPictureCount, s_gsPicture, s_gsPictureWidth >> 4,
                              s_gsPictureHeight >> 4, 0, 0, AA_HOST_FRAME, AA_HOST_FRAME);

    // The captions: the same rectangle of the caption picture, its clear pixels tested away.
    AlyssaPan_DrawUiTexture(&s_movieLayer, s_gsMoji, 0);
    AlyssaPan_DrawUiReg(&s_movieLayer, AA_GS_TEST_1, AA_CAPTION_TEST);
    rgbaq = AA_RGBAQ_GREY;
    memcpy(color, &rgbaq, 4);
    AlyssaPan_DrawUiPrim(&s_movieLayer, AA_PRIM_CAPTIONS, color);
    AlyssaPan_DrawUiBegin(&s_movieLayer, ALYSSAPAN_UIV_UV);
    AA_corner(AA_HOST_WINDOW_MIN, AA_HOST_WINDOW_MIN, AA_UV_ORIGIN, AA_UV_ORIGIN);
    AA_corner(AA_HOST_WINDOW_MAX, AA_HOST_WINDOW_MAX, t->width * 16 + AA_UV_ORIGIN, t->height * 16 + AA_UV_ORIGIN);
    AlyssaPan_DrawUiFit(&s_movieLayer, 0);

    // The fade: Cd * As, and + Cs where the blend's D is Cs.
    rgbaq = t->fadeRgbaq;
    AlyssaPan_DrawUiFrameScale(&s_movieLayer, (float)(rgbaq >> 24) / AA_ALPHA_ONE);
    if (t->fadeAlpha == AA_FADE_ALPHA_SCALE_ADD && (rgbaq & 0x00ffffff) != 0) {
        memcpy(color, &rgbaq, 4);
        AlyssaPan_DrawUiReg(&s_movieLayer, AA_GS_ALPHA_1, AA_ALPHA_ADD_FIX);
        AlyssaPan_DrawUiPrim(&s_movieLayer, AA_PRIM_FADE, color);
        AlyssaPan_DrawUiBegin(&s_movieLayer, 0);
        AA_corner(AA_HOST_WINDOW_MIN, AA_HOST_WINDOW_MIN, 0, 0);
        AA_corner(AA_HOST_WINDOW_MAX, AA_HOST_WINDOW_MAX, 0, 0);
    }
    AlyssaPan_DrawUiKick(lists, 1);
}

// CT3 0x018bc900: uploads the caption picture's CLUT (`dt`, 8 x 2 PSMCT32) to `cbp` and waits.
// PORT: the GS's CLUT is the host caption picture's (s_gsMoji); `cbp` is always AA_CLUT_CBP.
void AA_GS_SetClut(int cbp)
{
    sceGsLoadImage load;

    (void)cbp;
    sceGsSetDefLoadImage(&load, (short)cbp, 1, SCE_GS_PSMCT32, 0, 0, 8, 2);
    FlushCache(0);
    sceGsExecLoadImage(&load, (u_long128 *)dt);
    AA_gsMojiInit();
    memcpy(s_gsMoji + AA_MOJI_TIM2_CLUT, dt, AA_CLUT_COLORS * 4);
    sceGsSyncPath(0, 0);
}

#define AA_VBLANK_DELAY_HOLD 5
#define GS_CSR_FIELD_SHIFT 13
#define AA_HALF_CENTRE 0x800

// PORT: a VoTag's tag block as a host pointer (its movie bank, AA_MovieUseBank's row).
static void *AA_tagBlock(const VoTag *tag, int field)
{
    int row = AA_MOVIE_ROW_GS_TAG0 + (int)(tag - AA_voBufTag);

    return BankEePtr(AA_MovieUseBank[row].bank, tag->tag[field]);
}

// CT3 0x018bca90: the vblank (INTC_VBLANK_S). Notes the field; while the
// display runs, sends the picture due: during the fade-out the last picture's block (AA_DISP_TAG) every
// field, else the queue's head -- its even field when decoded (telling the clock the picture went out),
// then its odd field, which frees its buffer at the transfer's end. A picture the video is steadily ahead
// with is held (shouldDelay, asked every AA_VBLANK_DELAY_HOLD fields). Counts the fade-in and the start's
// wait down.
int AA_vblankHandler(int cause)
{
    sceDmaChan *gif = sceDmaGetChan(SCE_DMA_GIF);
    VoTag *tag;

    // callcheck: sceDmaSend (in AA_sendImage, the PORT send that also draws)
    (void)cause;
    AA_oddeven = (int)(REG_GS_CSR >> GS_CSR_FIELD_SHIFT) & 1;
    if (AA_isCountVblank != 0) {
        AA_vblankCount++;
        AA_handler_error = sceGsSyncPath(0, 0);
        if (AA_handler_error == 0) {
            if (AA_FadeOutEnable != 0) {
                if (AA_DISP_TAG != NULL) {
                    sceGsSetHalfOffset((AA_oddeven & 1) != 0 ? &db.draw1 : &db.draw0, AA_HALF_CENTRE,
                                       AA_HALF_CENTRE, (short)(AA_oddeven ^ 1 ^ AA_YOffset));
                    if (AA_oddeven == 0) {
                        sceGsSwapDBuff(&db, 0);
                        sceGsSyncPath(0, 0);
                        AA_sendImage(gif, AA_DISP_TAG);
                        AA_handler_endimage(DMAC_GIF);          /* PORT: the transfer's end */
                    } else if (AA_oddeven == 1) {
                        sceGsSwapDBuff(&db, 1);
                        sceGsSyncPath(0, 0);
                        AA_sendImage(gif, AA_DISP_TAG);
                        AA_isFrameEnd = 1;
                        AA_handler_endimage(DMAC_GIF);
                    }
                }
                AA_frd++;
                return 0;
            }
            tag = AA_voBufGetTag(&voBuf);
            if (tag == NULL) {
                AA_frd++;
                return 0;
            }
            sceGsSetHalfOffset((AA_oddeven & 1) != 0 ? &db.draw1 : &db.draw0, AA_HALF_CENTRE, AA_HALF_CENTRE,
                               (short)(AA_oddeven ^ 1 ^ AA_YOffset));
            if (voBuf.delayHold != 0)
                voBuf.delayHold--;
            if (voBuf.delayHold == 0 && shouldDelay() != 0 && AA_isWithAudio != 0) {
                voBuf.delayHold = AA_VBLANK_DELAY_HOLD;
                return 0;
            }
            if (AA_oddeven == 0 && tag->status == VOBUF_STATUS_FULL) {
                sceGsSwapDBuff(&db, 0);
                AA_syncClockInVideo(tag->pts);
                sceGsSyncPath(0, 0);
                AA_sendImage(gif, AA_tagBlock(tag, 0));
                tag->status = VOBUF_STATUS_TOPDONE;
                AA_handler_endimage(DMAC_GIF);
            } else if (AA_oddeven == 1 && tag->status == VOBUF_STATUS_TOPDONE) {
                sceGsSwapDBuff(&db, 1);
                sceGsSyncPath(0, 0);
                AA_sendImage(gif, AA_tagBlock(tag, 1));
                tag->status = VOBUF_STATUS_FREE;
                AA_isFrameEnd = 1;
                AA_handler_endimage(DMAC_GIF);
            }
        }
    }
    if (voBuf.fadeReady != 0 && AA_FadeInCount > 0)
        AA_FadeInCount--;
    if (AA_WaitCount > 0)
        AA_WaitCount--;
    return 0;
}

// The fade sprite's record, the ROM's two branches: towards black (Cd * As, no colour) or the sprite's
// colour (Cd * As + Cs) with the RGBAQ given; *fade gets the word the caller patches.
static void AA_setFade(AA_IMAGE_TAG *t, int towardsBlack, u_int rgbaq, u_int **fade)
{
    t->fadeAlpha = towardsBlack ? AA_FADE_ALPHA_SCALE : AA_FADE_ALPHA_SCALE_ADD;
    t->fadeRgbaq = rgbaq;
    *fade = &t->fadeRgbaq;
}

// CT3 0x018bcdd0: the fade-out's block (AA_ReTagSet): the picture and captions already in the GS, then the
// fade sprite, at rest (As 0x80, no colour), its blend after AA_MvOutWhite.
void AA_setImageTag2(void *tag, int width, int height, u_int **fade)
{
    // callcheck: sceGifPkInit, sceGifPkReset, sceGifPkCnt, sceGifPkOpenGifTag, sceGifPkAddGsAD, sceGifPkCloseGifTag, sceGifPkEnd, sceGifPkTerminate (PORT: the GIF chain is the AA_IMAGE_TAG record)
    AA_IMAGE_TAG *t = (AA_IMAGE_TAG *)tag;

    t->picture = NULL;
    t->width = width;
    t->height = height;
    AA_setFade(t, AA_MvOutWhite == AA_MV_BLACK, AA_RGBAQ_CLEAR_WHITE, fade);
}

// CT3 0x018bd1c0: a picture's block (AA_decBs0): unless `keep`, the uploads of its width / 16 x height / 16
// macroblocks (column-major, 0x400 bytes each, TBP 0x2a00) and of the caption picture (PSMT4 at 0x3800);
// then the picture's sprite, the captions' sprite and the fade sprite -- with AA_MvInWhite AA_MV_FADE a
// darkening at rest, else a grey added (0x40) until the caller patches the word.
void AA_setImageTag(void *tag, const void *picture, int keep, int width, int height, u_int **fade)
{
    // callcheck: sceGifPkInit, sceGifPkReset, sceGifPkCnt, sceGifPkOpenGifTag, sceGifPkAddGsAD, sceGifPkCloseGifTag, sceGifPkReserve, sceGifPkRef, sceGifPkEnd, sceGifPkTerminate (PORT: the GIF chain is the AA_IMAGE_TAG record)
    AA_IMAGE_TAG *t = (AA_IMAGE_TAG *)tag;

    t->picture = keep == 0 ? picture : NULL;
    t->width = width;
    t->height = height;
    if (AA_MvInWhite == AA_MV_FADE)
        AA_setFade(t, 1, AA_RGBAQ_CLEAR_WHITE, fade);
    else
        AA_setFade(t, 0, AA_RGBAQ_GREY, fade);
}

// CT3 0x018bd830: fills the GS's frame buffers (the draw env `w` wide, both fields of `h` lines) with
// (r, g, b) by one sprite, and waits. PORT: the host frame filled with the colour and kicked.
void AA_clearGsMem(int r, int g, int b, int w, int h)
{
    const void *lists[1] = { &s_movieLayer };
    u_char color[4];

    // callcheck: memalign, free, sceDmaGetChan, sceGsSetDefDrawEnv, FlushCache, sceGsSyncPath, sceGsPutDrawEnv, sceGifPkInit, sceGifPkReset, sceGifPkEnd, sceGifPkOpenGifTag, sceGifPkAddGsAD, sceGifPkCloseGifTag, sceGifPkTerminate, sceDmaSend (PORT: the packet and its send are the host draw and kick)
    (void)w;
    (void)h;
    color[0] = (u_char)r;
    color[1] = (u_char)g;
    color[2] = (u_char)b;
    color[3] = 0;
    AlyssaPan_DrawUiPrim(&s_movieLayer, AA_PRIM_SPRITE, color);
    AlyssaPan_DrawUiBegin(&s_movieLayer, 0);
    AA_corner(AA_HOST_WINDOW_MIN, AA_HOST_WINDOW_MIN, 0, 0);
    AA_corner(AA_HOST_WINDOW_MAX, AA_HOST_WINDOW_MAX, 0, 0);
    AlyssaPan_DrawUiKick(lists, 1);
}
