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
    const float *position;      /* +0x38: the object's position qword */
    const float *matrix;        /* +0x58: 4 qwords */
    const float *lightMatrix;   /* +0x5c, light1 only: 3 qwords */
    const void *lightWork;      /* +0x3c, light1 only: 17 qwords (GA_LIGHT_WORK) */
    float lightScale;           /* +0x30, light1 only (the set-up stores it at scratchpad 0xcc) */
    uint64_t gsReg70;           /* +0x70: a GS register value (the header's 0x78) */
    uint64_t gsReg78;           /* +0x78: a GS register value (the header's 0xc0) */
    const void *upload;         /* the attr argument: a GS upload packet (halfword qwc, then data) or null */
};

/* The object, in the order the game draws it. */
void AlyssaPan_DrawGaObject(const AlyssaPanGaObject *object);

#endif /* ALYSSAPAN_DRAWGA_H */
