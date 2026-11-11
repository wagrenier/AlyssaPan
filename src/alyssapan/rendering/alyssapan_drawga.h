/* ==========================================================================
 *  alyssapan_drawga.h
 *
 *  The native draw module of CT3's ga draw objects (the VU1 mesh path,
 *  ga/gaBGT_DrawObject.c; cpp-port.md). Where the ROM builds a VIF1 chain
 *  that hands the object's blocks to the VU1 renderer (positions, ST,
 *  colours, normals, the index record, the object's matrix and position,
 *  the light work), the game calls this with the same blocks, read in
 *  place: no VIF, no VU1, no GS emulation. Started 2026-11-10 with the
 *  no-clip path (gaBGT_DrawObject_NoClippingCheck2's nc2 light0 / light1
 *  set-ups); the host module records the frame's objects, the transform
 *  and the pass that draws them come when the VU1 programs are read
 *  (open-items.md). Keep it C.
 * ======================================================================== */
#ifndef ALYSSAPAN_DRAWGA_H
#define ALYSSAPAN_DRAWGA_H

#include <stdint.h>

/* Which VU1 set-up the ROM chose (the object's flags & 0x1c, then the
 * set-up function): 0 is light0 (no normals: colours as they are), 1 is
 * light1 (normals and the light work). */
enum
{
    ALYSSAPAN_GA_LIGHT0 = 0,
    ALYSSAPAN_GA_LIGHT1 = 1,
};

/* One ga draw object as its set-up sends it. Every pointer is a host
 * pointer into the bank (or the scratchpad), valid during the call only:
 * the module copies what it keeps. */
struct AlyssaPanGaObject
{
    int light;                  /* ALYSSAPAN_GA_LIGHT* */
    int flags;                  /* the object's flags byte (+0x00) */
    int vertexCount;            /* +0x48 */
    int primCount;              /* +0x44 */
    const float *positions;     /* +0x60: vertexCount x (x y z w) */
    const void *stBlock;        /* +0x68: one qword of VIF codes, then vertexCount x (s t) floats */
    const void *colourBlock;    /* +0x6c: one qword of VIF codes, then vertexCount x RGBA bytes */
    const void *normalBlock;    /* +0x64, light1 only: one qword of VIF codes, then vertexCount qwords */
    const uint16_t *indices;    /* +0x50: primCount x 4 halfwords (qword offsets of the vertices) */
    const float *position;      /* +0x38: the qword at that EE address, the part's ALL_TABLE.partsPos: the
                                   ambient RGB the light0 program adds to each colour (vf20), alpha in w */
    const float *matrix;        /* +0x58: 4 qwords */
    const float *lightMatrix;   /* +0x5c, light1 only: 3 qwords */
    const void *lightWork;      /* +0x3c, light1 only: 17 qwords (GA_LIGHT_WORK) */
    float lightScale;           /* +0x30, light1 only (the set-up stores it at scratchpad 0xcc) */
    uint64_t gsReg70;           /* +0x70: two floats, not a GS register: the VU1 program's Z offset and
                                   fog base (the header's 0x78 = vf17.zw) */
    uint64_t gsReg78;           /* +0x78: two floats, not a GS register: the ST offset (the header's 0xc0 =
                                   vf30.xy) */
    const void *upload;         /* the attr argument: a GS upload packet (halfword qwc, then data) or null */
    const float *vuHeader;      /* the scratchpad's 0x60..0xd0 as the set-up left it: the 3dlib qwords the
                                   VU1 program reads (ALYSSAPAN_GA_HDR_*), 4 floats each */
};

/* Float indices into vuHeader: what the VU1 program loads each qword as. */
enum
{
    ALYSSAPAN_GA_HDR_SCALE = 0,   /* vf16 (0x60): projection scale x, y, z (12.4 units) */
    ALYSSAPAN_GA_HDR_OFFSET = 4,  /* vf17 (0x70): screen centre x, y; Z offset; fog base */
    ALYSSAPAN_GA_HDR_COUNTS = 8,  /* vf18 (0x80): 0, vertices, prims; w the projection distance */
    ALYSSAPAN_GA_HDR_FOG = 20,    /* vf29 (0xb0): fog bias, fog scale, colour max (255), fog distance bias */
    ALYSSAPAN_GA_HDR_ST = 24,     /* vf30 (0xc0): ST offset s, t; w the light1 scale */
};

/* flags & 3, the VU1 program's variant: which screen-space winding it drops (the ADC bit on the third
 * vertex's XYZF2). */
enum
{
    ALYSSAPAN_GA_CULL_NEGATIVE = 0, /* drops a triangle whose screen cross product's z is negative */
    ALYSSAPAN_GA_CULL_POSITIVE = 1, /* drops it when that z is zero or positive */
    /* 2 and 3: no culling */
};

/* The object, in the order the game draws it. */
void AlyssaPan_DrawGaObject(const AlyssaPanGaObject *object);

/* The texture the ga objects after it draw with, as the texture buffer
 * packet (game/QuatSlerp.h SUN_TBUFFER_DATA) sets it: where the ROM sends
 * TEX0_1 / CLAMP_1 to the GS (sunBGT_ExecTBufferData_VIF1), the game hands
 * over the texture record's level 0 and CLUT in place, decoded at their
 * source (rendering::TextureStore::imageOfGsTexture). Added 2026-11-11
 * (host task alyssapan/rendering/ga_atld.cpp). pixels null: no texture. */
struct AlyssaPanGaTexture
{
    uint64_t tex0;              /* TEX0_1 as the packet holds it (TW, TH, PSM, TFX, TCC) */
    uint64_t clamp;             /* CLAMP_1 */
    int width;                  /* SUN_TEX_ATTR's size: the pixels' row length and rows */
    int height;
    const void *pixels;         /* level 0, raw GS pixels in PSM order (no swizzle: the ROM uploads them
                                   with an image transfer) */
    const uint32_t *clut;       /* the CLUT TEX0.CBP picks (16 or 256 PSMCT32 entries, CSM1), or null */
};

void AlyssaPan_DrawGaTexture(const AlyssaPanGaTexture *texture);

/* A flat colour over the whole screen, kept in draw order with the
 * objects: where the ROM sends a full-window GIF SPRITE under a draw
 * control of ALPHA_1 (Cs - Cd) * FIX + Cd (tiFade_Draw's init_black /
 * init_white), the game calls this with the sprite's RGBA (r in the low
 * byte, 0..255) and the FIX (0..128, 128 = the colour alone). The host
 * covers the whole window at any aspect, not the 512 x 448 GS window.
 * Added 2026-11-11 (host task alyssapan/rendering/ga_fill.cpp). */
void AlyssaPan_DrawGaFill(uint32_t rgba, int alphaFix);

/* GS frame-buffer copies, kept in draw order with the objects (added
 * 2026-11-11, Clipping_Portal.c slice 11: CaptureDBuffer, tiVRAMViewer).
 * Where the ROM points the draw buffer at another GS address and draws
 * the frame buffer into it as a texture, or draws a GS region as a
 * textured sprite, the game names the copy: no GS memory on the host.
 *
 * Capture: the frame drawn so far, scaled into a width x height picture
 * kept under the GS word address fbp (the effect buffer, 0xa8000). */
void AlyssaPan_DrawGaCapture(uint32_t fbp, int width, int height);

/* Copy: the picture kept under fbp (a capture; the frame drawn so far
 * when fbp is the draw buffer), texWidth x texHeight of it, as a sprite
 * over the GS window rectangle x0, y0 .. x1, y1 (pixels of the 512 x 448
 * frame, inclusive), modulated by grey 0x80 (the texel as it is) and
 * blended (Cs - Cd) * FIX + Cd, FIX 0..128. */
void AlyssaPan_DrawGaCopy(uint32_t fbp, int texWidth, int texHeight, float x0, float y0, float x1, float y1,
                          int alphaFix);

#endif /* ALYSSAPAN_DRAWGA_H */
