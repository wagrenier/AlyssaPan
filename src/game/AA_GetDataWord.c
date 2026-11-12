// game/AA_GetDataWord.c -- CT3 0x018bc690..0x018bda90: the movie's display. SCE's ezmpegstr disp.c
// (startDisplay, endDisplay, vblankHandler, handler_endimage, setImageTag, clearGsMem), AA_-prefixed, grown
// with CT3's captions (AA_TestRead draws a caption file's records into a 4-bit picture the image transfer
// lays over the movie) and an A/V pacing check (shouldDelay holds a picture while the video runs ahead).
//
// Slice 1 (2026-11-12): the caption readers, the caption picture, shouldDelay and the display switches.
// Slice 2 is the GS side -- AA_GS_SetClut, AA_clearGsMem, AA_setImageTag / AA_setImageTag2 and
// AA_vblankHandler -- which builds the GIF packets that upload a decoded picture and its captions and swaps
// the fields: there the host draw is decided (alyssapan_drawui.h's AlyssaPan_DrawUiMovie for the picture).

#include "game/AA_GetDataWord.h"

#include "common/alyssapan_stub.h"
#include "game/AA_cpuCycles.h"
#include "game/AA_mpegTS.h"
#include "game/movie.h"
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

// ---- stubs (slice 2: the GS side) -----------------------------------------------------------------------

// stub of CT3 0x018bc900
void AA_GS_SetClut(int cbp)
{
    ALYSSAPAN_STUB("AA_GS_SetClut", 0x018bc900);
}

// stub of CT3 0x018bd830
void AA_clearGsMem(int r, int g, int b, int w, int h)
{
    ALYSSAPAN_STUB("AA_clearGsMem", 0x018bd830);
}

// stub of CT3 0x018bca90
int AA_vblankHandler(int cause)
{
    ALYSSAPAN_STUB("AA_vblankHandler", 0x018bca90);
    return {};
}
