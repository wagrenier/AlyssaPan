/* ==========================================================================
 *  alyssapan_drawui.h
 *
 *  The native draw module of the 2D (rendering::UiCanvas; cpp-port.md).
 *  Carried over from YamiPan, where FS2's UiDrawer calls it. The game's
 *  2D code keeps every decision the ROM makes -- primitive, colours,
 *  texture, blend, wrap, scissor, order -- and hands them here as the GS
 *  register values it would have packed, instead of writing GIF packets.
 *  No GS emulator: the module turns the values into host draws; textures
 *  arrive as their file (a TIM2 picture) and are decoded at their source to
 *  GPU textures, never through GS memory. CT3's own 2D path is not read
 *  yet: grow this header from what its drawers need, keep it C.
 *
 *  `list` is the packet the game drew into (one per packet list): each is
 *  a layer, drawn in the order the kick names them. Coordinates are the GS
 *  window's (12.4 fixed point, 0x8000 = 2048.0, the draw env's centre): the
 *  module maps them to its frame from the renderer-owned display size and
 *  aspect, never a fixed 4:3.
 * ======================================================================== */
#ifndef ALYSSAPAN_DRAWUI_H
#define ALYSSAPAN_DRAWUI_H

#include <stdint.h>

/* One vertex as the GS would get it: RGBAQ bytes (alpha 0x80 opaque), UV
 * in texels times 16, XY in window coordinates times 16, Z the register's
 * 32 bits. Only what the open format names is
 * meaningful. */
struct AlyssaPanUiVertex
{
    uint8_t  rgba[4];
    uint16_t u, v;
    uint16_t x, y;
    uint32_t z;
};

/* The vertex formats (UiVertexFormat): bit 0 RGBAQ, bit 1 UV, bit 2 in
 * groups of three (only every third vertex kicks). */
enum
{
    ALYSSAPAN_UIV_RGBAQ   = 1,
    ALYSSAPAN_UIV_UV      = 2,
    ALYSSAPAN_UIV_GROUPED = 4,
};

/* One A+D register write (ALPHA_1, CLAMP_1, TEX1_1, SCISSOR_1, FRAME_1,
 * ZBUF_1, and the BITBLTBUF / TRXPOS / TRXREG / TRXDIR set of a local copy,
 * which the module performs between its render targets on TRXDIR). */
void AlyssaPan_DrawUiReg(const void *list, int reg, uint64_t value);

/* PRIM, and with `rgbaq` non-null the colour every following vertex
 * shares (the flat draws). */
void AlyssaPan_DrawUiPrim(const void *list, uint64_t prim, const uint8_t *rgbaq);

/* Binds picture `picture` of the TIM2 file at `tim2` (null keeps the bound
 * one). Decoded during the call, keyed by its content: the file may be freed
 * right after. */
void AlyssaPan_DrawUiTexture(const void *list, const void *tim2, int picture);

/* Non-zero: the vertices that follow are pulled towards the frame's
 * centre, by as much as the display is wider than the 4:3 set the 2D was
 * composed for, so that what is drawn centred in raw window coordinates
 * (text, its strip, a prompt) keeps its shape at any aspect. Zero ends it.
 * The factor is 1 at 4:3 and on anything narrower. */
void AlyssaPan_DrawUiSqueeze(const void *list, int on);

/* What AlyssaPan_DrawUiSqueeze pulls by: 1 at 4:3 and narrower, under 1 wider.
 * For a shape placed anywhere over the frame: its own width times this keeps
 * its shape where its place still spreads over the whole display. */
float AlyssaPan_DrawUiSqueezeFactor(void);

/* Non-zero: the vertices and movie pictures that follow land in a 4:3 box,
 * the largest the display holds, centred -- the frame pulled towards its
 * centre in x on a wider display, in y on a narrower one. For what filled
 * the TV's whole 4:3 picture (a movie): its shape kept at any aspect, bars
 * around it, never stretched to the window. Zero ends it. */
void AlyssaPan_DrawUiFit(const void *list, int on);

/* Passes over the whole frame so far, at any aspect: the colour buffer
 * times `factor`, 0..4 (the GS's A = Cd, C = FIX), and pushed away from a
 * colour, (Cd - Cs) * As + Cd (ALPHA_1 0x41, `rgba` as RGBAQ, As on the
 * 0x80 scale). Neither touches the frame's alpha. */
void AlyssaPan_DrawUiFrameScale(const void *list, float factor);
void AlyssaPan_DrawUiFrameContrast(const void *list, uint32_t rgba);

/* Starts a vertex run of `format`; AlyssaPan_DrawUiVertex adds to it. */
void AlyssaPan_DrawUiBegin(const void *list, int format);
void AlyssaPan_DrawUiVertex(const void *list, const AlyssaPanUiVertex *vertex);

/* A movie picture: `macroblocks` are the decoded 16 x 16 RGBA32 blocks
 * (0x400 bytes each, alpha 0x80 opaque), `columns` x `rows` of them in
 * column-major order, as the game would have sent them by image transfer,
 * stretched over the frame's rect at (x, y), `width` x `height` pixels (as
 * the game's sprite stretched it; AlyssaPan_DrawUiFit places it). The module
 * keeps them as the texture of `owner` (the game's movie object), rebuilt
 * only when `picture` (the count of decoded pictures) moves: decoded at the
 * source, never through GS memory. */
void AlyssaPan_DrawUiMovie(const void *list, const void *owner, int picture, const void *macroblocks,
                           int columns, int rows, int x, int y, int width, int height);

/* The end of a frame: the packet lists that hold draws, in chain order
 * (= the layer order), as the game's kick would send them on VIF1.
 * The module draws those layers in this order. */
void AlyssaPan_DrawUiKick(const void *const *lists, int count);

#endif /* ALYSSAPAN_DRAWUI_H */
