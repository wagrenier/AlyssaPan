// ga/gaDrawObject_Redraw_Polygon.c -- Clock Tower 3, the clipped path's divided polygons redrawn by the EE
// (CT3 0x0189f718..0x018a0880, a hand-written asm object; the file name is the tool's, from its first
// function). Read from the disassembly (mipsdis.py): the decompiler loses the routines' register hand-offs
// (DivSub's computed jumps through s2, the clipper's return through ra = the loop's continue).
//
// gaBGT_DrawObject's clipped path leaves the polygons the index pass divided (too big, or across the near
// plane) as vertex / primitive records (gaDrawObject_SetBig / SetNear). Here they are put in screen space
// (trans_for_gs), cut against the GS guard band (+-0.5 of the view scale: GA_GUARD_*) one plane at a time,
// and sent as GS triangles (ST, RGBAQ, XYZF2 x 3) through the MFIFO, bypassing VU1.
//
// PORT (open-items.md (kv), Claude's call): the host draw module draws the whole object (the GPU clips), so
// the packet is built ROM-exact in ga_div_redraw_gspacket_work but not sent: drawing it would draw the
// divided polygons twice. The VU0 floats are taken as IEEE; a register the ROM leaves stale (an ST's w, a
// cut point's guard z) is 0 here (none reaches the GS).
//
// Session 2026-11-12 (scheduled run): all six functions.

#include "ga/gaDrawObject_Redraw_Polygon.h"
#include "ga/gaBGT_DrawObject.h"
#include "ga/gaDrawObject.h"
#include "ga/gaInitDrawObject.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "sdk/eekernel.h"

#include <math.h>
#include <string.h>

#define SPR_WORK ((GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET))

// The symtab's buffers (bss). The first two are SetBig / SetNear's output (gaDrawObject.c fills them).
GA_REDRAW_VERTEX ga_div_redraw_vertex_work[GA_DIV_REDRAW_VERTICES];          // CT3 0x01a2c640
GA_REDRAW_PRIM ga_div_redraw_index_work[GA_DIV_REDRAW_PRIMS];                // CT3 0x01a2be40
GA_REDRAW_PRIM ga_div_redraw_gsindex_work[GA_DIV_REDRAW_GSINDEX_PRIMS];      // CT3 0x01a07e40
GA_GS_VERTEX ga_div_redraw_gsvertex_work[GA_DIV_REDRAW_GSVERTICES];          // CT3 0x01a0be40
u_char ga_div_redraw_gspacket_work[GA_DIV_REDRAW_GSPACKET_SIZE];             // CT3 0x01957e40

// The packet head's VIFcode (word 3 of its first qword: DIRECT, qwc in the immediate) and the GIFtag's EOP.
#define VIF_DIRECT_CODE 0x50000000u
#define GIFTAG_EOP 0x8000

// The DMA chain the Output routines start fromSPR with (scratchpad 0x9e0: one REF to the packet, or the
// draw attr's GS upload then the packet), and the DMAtag words it uses.
#define GA_SPR_REDRAW_CHAIN 0x9e0
#define DMA_REF 0x30000000u
#define DMA_ADDR_SPR 0x80000000u                /* a DMAtag address in the scratchpad (fromSPR's SADR) */
#define DMA_ADDR_SPR_DATA 0x10000000u           /* + to a scratchpad address seen from the DMA tag's side */
#define DMA_ADDR_MASK 0x0fffffffu

// ---- EE addresses in the work buffers (PORT: NULL past a buffer's end; the ROM writes on) ----

static GA_REDRAW_VERTEX *RedrawVertexPtr(u_int ee)
{
    if (ee - GA_DIV_REDRAW_VERTEX_WORK_EE < sizeof(ga_div_redraw_vertex_work))
        return &ga_div_redraw_vertex_work[(ee - GA_DIV_REDRAW_VERTEX_WORK_EE) / sizeof(GA_REDRAW_VERTEX)];
    return NULL;
}

static GA_REDRAW_PRIM *RedrawPrimPtr(u_int ee)
{
    if (ee - GA_DIV_REDRAW_INDEX_WORK_EE < sizeof(ga_div_redraw_index_work))
        return &ga_div_redraw_index_work[(ee - GA_DIV_REDRAW_INDEX_WORK_EE) / sizeof(GA_REDRAW_PRIM)];
    return NULL;
}

static GA_REDRAW_PRIM *GsPrimPtr(u_int ee)
{
    if (ee - GA_DIV_REDRAW_GSINDEX_WORK_EE < sizeof(ga_div_redraw_gsindex_work))
        return &ga_div_redraw_gsindex_work[(ee - GA_DIV_REDRAW_GSINDEX_WORK_EE) / sizeof(GA_REDRAW_PRIM)];
    return NULL;
}

static GA_GS_VERTEX *GsVertexPtr(u_int ee)
{
    if (ee - GA_DIV_REDRAW_GSVERTEX_WORK_EE < sizeof(ga_div_redraw_gsvertex_work))
        return &ga_div_redraw_gsvertex_work[(ee - GA_DIV_REDRAW_GSVERTEX_WORK_EE) / sizeof(GA_GS_VERTEX)];
    return NULL;
}

// The packet cursor is an uncached-accelerated address (0x2xxxxxxx); n bytes from it, or NULL.
static u_char *GsPacketPtr(u_int ee, u_int n)
{
    u_int off = (ee & DMA_ADDR_MASK) - GA_DIV_REDRAW_GSPACKET_WORK_EE;

    if (off > sizeof(ga_div_redraw_gspacket_work) || n > sizeof(ga_div_redraw_gspacket_work) - off)
        return NULL;
    return ga_div_redraw_gspacket_work + off;
}

// A relocated word of the object's bank (NULL for 0, or when the object is in no bank).
static const void *ObjEePtr(int bank, u_int ee)
{
    return bank >= 0 ? BankEePtr(bank, ee) : NULL;
}

// ---- VU0 macro operations ----

// VFTOI0: truncation, saturating at the int range.
static int FtoI0(float f)
{
    if (f != f)
        return 0;
    if (f >= 2147483648.0f)
        return 0x7fffffff;
    if (f <= -2147483648.0f)
        return (int)0x80000000;
    return (int)f;
}

// VCLIPw.xyz: the six flags of v against |w| (bit 0 +x .. bit 5 -z).
static u_int VuClip(const float v[3], float w)
{
    float a = fabsf(w);
    u_int f = 0;

    if (v[0] > a) f |= 0x01;
    if (v[0] < -a) f |= 0x02;
    if (v[1] > a) f |= 0x04;
    if (v[1] < -a) f |= 0x08;
    if (v[2] > a) f |= 0x10;
    if (v[2] < -a) f |= 0x20;
    return f;
}

// ---- trans_for_gs ----

// layout: host (the VU0 registers trans_for_gs loads before its loop)
typedef struct GS_TRANSFORM {
    float scale[4];         /* vf16: GA_SPR_WORK 0x60 (work21, work22, work23 * 16): x, y, z per 1/z */
    float offset[4];        /* vf17: 0x70 (work26, work27, the object's Z offset); w the fog base (below) */
    float perspective;      /* vf18.w: 0x8c, work[6] (q = this / z) */
    float fogConst[4];      /* vf29: 0xb0 (work9, work25, 255, work24) */
    float stOffset[2];      /* vf30.xy: 0xc0, the object's +0x78 */
    float guard[4];         /* GA_SPR_WORK 0x30 / 0x34 (the view scale) and 0x4c (0.5): the guard band */
    float colourAdd[4];     /* vf20: the qword at the object's +0x38, added to every colour */
} GS_TRANSFORM;

static void LoadTransform(GS_TRANSFORM *t, const GA_SPR_WORK *spr, const float *colourAdd)
{
    int k;

    memcpy(t->scale, &spr->work21, 0x10);
    memcpy(t->offset, &spr->work26, 0x10);
    t->perspective = spr->work6b;
    memcpy(t->fogConst, &spr->work9, 0x10);
    memcpy(t->stOffset, &spr->objReg78, 8);
    t->guard[0] = spr->viewScaleX;
    t->guard[1] = spr->viewScaleY;
    t->guard[2] = 0.0f;                 /* unused: only x, y are scaled */
    t->guard[3] = spr->half04c;
    // vf17.w = (fog base + work9) * 16 (four VADD.w of itself).
    t->offset[3] = (t->offset[3] + t->fogConst[0]) * 16.0f;
    for (k = 0; k < 4; k++)
        t->colourAdd[k] = colourAdd[k];
}

// The loop's common tail (0x018a0448.. / 0x018a076c..): one record to screen space, its colour given.
static void TransformVertex(GA_GS_VERTEX *out, const GA_REDRAW_VERTEX *in, const float colour[4], const GS_TRANSFORM *t)
{
    const float *v = in->view;
    float st0, st1, q, d2;
    int k;

    for (k = 0; k < 4; k++) {
        float c = colour[k];

        c = c > 0.0f ? c : 0.0f;                        /* VMAXz vf0.z */
        out->rgba[k] = c < t->fogConst[2] ? c : t->fogConst[2]; /* VMINIz vf29.z (255) */
    }
    st0 = in->st[0] + t->stOffset[0];
    st1 = in->st[1] + t->stOffset[1];
    d2 = t->fogConst[3] + v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    q = t->perspective / v[2];
    out->xyzf[0] = t->offset[0] + t->scale[0] * v[0] * q;
    out->xyzf[1] = t->offset[1] + t->scale[1] * v[1] * q;
    out->xyzf[2] = t->offset[2] + t->scale[2] * q;      /* vf1.z = 1 */
    out->xyzf[3] = t->offset[3] + t->fogConst[1] / d2;
    out->stq[0] = st0 * q;
    out->stq[1] = st1 * q;
    out->stq[2] = q;                                    /* vf2.z = 1 */
    out->stq[3] = 0.0f;                                 /* PORT: vf6.w is stale in the ROM */
    out->clip.xyz[0] = v[0] * t->guard[0] * q;          /* + GA_SPR_WORK 0x40 (0) */
    out->clip.xyz[1] = v[1] * t->guard[1] * q;
    out->clip.xyz[2] = v[2];
    out->clip.f.flags = VuClip(out->clip.xyz, t->guard[3]) & GA_GUARD_ALL;
}

// The lit kind's colour (0x018a05d4..0x018a0768): the object's light work (obj +0x3c, past its VIF head
// qword) holds four directional lights and four point lights, structure of arrays:
//   +0x00..0x20 the directions' x / y / z (a light per lane), +0x30 their intensities;
//   +0x40..0x60 the points' x / y / z, +0x70 a scale on the vectors to them;
//   +0x80..0xb0 the directional colours, +0xc0..0xf0 the point colours.
// The normal goes through the light matrix (obj +0x5c, 3 rows); a directional light adds
// max(n . dir, 0) x intensity of its colour; a point light max(n . p, 0) x (1 / |p| - 1), floored at 0,
// with p = (point - vertex) x scale. With the object's +0x30 (its light scale) non-zero, alpha becomes a
// highlight: (max(r . -dir0, 0))^4 x scale x -intensity0, r the view vector reflected about the normal.
#define GA_LIGHTS 4
typedef struct GA_LIGHT_SET {
    float dir[3][4];
    float intensity[4];
    float point[3][4];
    float pointScale[4];
    float dirColour[GA_LIGHTS][4];
    float pointColour[GA_LIGHTS][4];
} GA_LIGHT_SET;
static_assert(sizeof(GA_LIGHT_SET) == 0x100, "GA_LIGHT_SET");

static void LitColour(float out[4], const GA_REDRAW_VERTEX *in, const float *lightMatrix, const GA_LIGHT_SET *ls,
                      const float colourAdd[4], float lightScale, int highlight)
{
    float n[3], p[3][4], dsq[4], inv[4], d[4], pt[4];
    int k, j;

    for (k = 0; k < 3; k++)
        n[k] = lightMatrix[0 * 4 + k] * in->normal[0] + lightMatrix[1 * 4 + k] * in->normal[1] +
               lightMatrix[2 * 4 + k] * in->normal[2];
    for (j = 0; j < 3; j++)
        for (k = 0; k < 4; k++)
            p[j][k] = (ls->point[j][k] - in->view[j]) * ls->pointScale[k];
    for (k = 0; k < 4; k++) {
        dsq[k] = p[0][k] * p[0][k] + p[1][k] * p[1][k] + p[2][k] * p[2][k];
        inv[k] = 1.0f / sqrtf(dsq[k]);                  /* VRSQRT vf0.w, between DI / EI */
        d[k] = ls->intensity[k] * n[0] * ls->dir[0][k] + ls->intensity[k] * n[1] * ls->dir[1][k] +
               ls->intensity[k] * n[2] * ls->dir[2][k];
        pt[k] = n[0] * p[0][k] + n[1] * p[1][k] + n[2] * p[2][k];
        d[k] = d[k] > 0.0f ? d[k] : 0.0f;
        pt[k] = pt[k] > 0.0f ? pt[k] : 0.0f;
        pt[k] = pt[k] * inv[k] - pt[k];
        pt[k] = pt[k] > 0.0f ? pt[k] : 0.0f;
    }
    for (k = 0; k < 4; k++) {
        float c = in->colour[k] + colourAdd[k];

        for (j = 0; j < GA_LIGHTS; j++)
            c += ls->dirColour[j][k] * d[j] + ls->pointColour[j][k] * pt[j];
        out[k] = c;
    }
    if (highlight) {
        float v[3], len2, q, vn, r[3], s;

        len2 = in->view[0] * in->view[0] + in->view[1] * in->view[1] + in->view[2] * in->view[2];
        q = 1.0f / sqrtf(len2);
        for (k = 0; k < 3; k++)
            v[k] = in->view[k] * q;
        vn = -2.0f * (v[0] * n[0] + v[1] * n[1] + v[2] * n[2]);    /* 0xc0000000 through QMTC2 */
        for (k = 0; k < 3; k++)
            r[k] = v[k] + n[k] * vn;
        s = -r[0] * ls->dir[0][0] - r[1] * ls->dir[1][0] - r[2] * ls->dir[2][0];
        s = s > 0.0f ? s : 0.0f;
        s = s * s;
        s = s * s;
        out[3] = lightScale * -ls->intensity[0] * s;
    }
}

// CT3 0x018a03e0
// The redraw records to screen space, then the primitive list rebased onto the results. flags is the
// object's first byte: GA_LIGHT_KIND_1 lights the vertices (the normals), any other kind takes their colour.
// Also puts the object's +0x70 / +0x78 into GA_SPR_WORK 0x78 / 0xc0.
void trans_for_gs(u_char *obj, int flags)
{
    GA_SPR_WORK *spr = SPR_WORK;
    int bank = BankOfHostPtr(obj);
    const float *colourAdd = (const float *)ObjEePtr(bank, *(const u_int *)(obj + GA_OBJ_POSITION));
    int lit = (flags & GA_LIGHT_KIND_1) != 0;
    const float *lightMatrix = NULL;
    const GA_LIGHT_SET *ls = NULL;
    float lightScale = 0.0f;
    int highlight = 0;
    GS_TRANSFORM t;
    u_int src, dst, cur, out;
    static const float kZero[4];

    spr->objReg70 = *(const u_long *)(obj + GA_OBJ_REG70);
    spr->objReg78 = *(const u_long *)(obj + GA_OBJ_REG78);
    LoadTransform(&t, spr, colourAdd ? colourAdd : kZero);
    if (lit) {
        lightMatrix = (const float *)ObjEePtr(bank, *(const u_int *)(obj + GA_OBJ_LIGHT_MATRIX));
        const u_char *work = (const u_char *)ObjEePtr(bank, *(const u_int *)(obj + GA_OBJ_LIGHT_WORK));

        ls = work ? (const GA_LIGHT_SET *)(work + 0x10) : NULL;     /* past the VIF head qword */
        memcpy(&lightScale, obj + GA_OBJ_LIGHT_SCALE, sizeof(lightScale));
        highlight = *(const u_int *)(obj + GA_OBJ_LIGHT_SCALE) != 0;
    }
    // ROM: a do-while (the first record is transformed even with none; the callers test that first).
    dst = GA_DIV_REDRAW_GSVERTEX_WORK_EE;
    for (src = GA_DIV_REDRAW_VERTEX_WORK_EE; src != spr->divRedrawVertexEE; src += sizeof(GA_REDRAW_VERTEX)) {
        const GA_REDRAW_VERTEX *in = RedrawVertexPtr(src);
        GA_GS_VERTEX *o = GsVertexPtr(dst);
        float colour[4];
        int k;

        if (!in || !o)
            break;
        if (lit && lightMatrix && ls) {
            LitColour(colour, in, lightMatrix, ls, t.colourAdd, lightScale, highlight);
        } else {
            for (k = 0; k < 4; k++)
                colour[k] = in->colour[k] + t.colourAdd[k];
        }
        TransformVertex(o, in, colour, &t);
        dst += sizeof(GA_GS_VERTEX);
    }
    spr->gsVertexEE = dst;
    // The primitives: the vertex addresses moved from the redraw buffer to the GS one (same index), the
    // table entry kept.
    out = GA_DIV_REDRAW_GSINDEX_WORK_EE;
    for (cur = GA_DIV_REDRAW_INDEX_WORK_EE; cur != spr->divRedrawIndexEE; cur += sizeof(GA_REDRAW_PRIM)) {
        const GA_REDRAW_PRIM *p = RedrawPrimPtr(cur);
        GA_REDRAW_PRIM *q = GsPrimPtr(out);
        int k;

        if (!p || !q)
            break;
        for (k = 0; k < 3; k++)
            q->vertexEE[k] = p->vertexEE[k] + (GA_DIV_REDRAW_GSVERTEX_WORK_EE - GA_DIV_REDRAW_VERTEX_WORK_EE);
        q->primTableEE = p->primTableEE;
        out += sizeof(GA_REDRAW_PRIM);
    }
    spr->gsIndexEndEE = out;
}

// ---- the clipper ----

// The guard planes in the order DivSub tests them (the highest flag first), with the guard band's bound
// (GA_SPR_WORK 0x230: -0.5, +0.5) and the flags a cut point keeps (the planes still to test: CF & s7).
// layout: host (the ROM spells each plane out as its own code)
typedef struct GUARD_PLANE {
    u_int flag;
    int axis;           /* 0 x, 1 y */
    int bound;          /* 0: 0x230 (-0.5), 1: 0x234 (+0.5) */
    u_int keep;
} GUARD_PLANE;
static const GUARD_PLANE kGuardPlanes[4] = {
    {GA_GUARD_NEG_Y, 1, 0, GA_GUARD_POS_Y | GA_GUARD_NEG_X | GA_GUARD_POS_X},  /* 0x0189fe10 / 0x0189ff84 */
    {GA_GUARD_POS_Y, 1, 1, GA_GUARD_NEG_X | GA_GUARD_POS_X},                   /* 0x0189fe48 / 0x0189ffbc */
    {GA_GUARD_NEG_X, 0, 0, GA_GUARD_POS_X},                                    /* 0x0189fe80 / 0x0189fff4 */
    {GA_GUARD_POS_X, 0, 1, 0},                                                 /* 0x0189feb8 / 0x018a002c */
};

// The point on the edge from base towards other where the guard coordinate is the bound:
// base - (base - other) t, t = (base - bound) / (base - other) on that axis. Position and colour lerp all
// four components, STQ x, y, z, the guard position x, y; its flags are VCLIPw's for the planes kept.
static void CutPoint(GA_GS_VERTEX *out, const GA_GS_VERTEX *base, const GA_GS_VERTEX *other, const GUARD_PLANE *pl,
                     float boundValue, float guardW)
{
    float t = (base->clip.xyz[pl->axis] - boundValue) / (base->clip.xyz[pl->axis] - other->clip.xyz[pl->axis]);
    int k;

    memset(out, 0, sizeof(*out));
    for (k = 0; k < 4; k++) {
        out->xyzf[k] = base->xyzf[k] - (base->xyzf[k] - other->xyzf[k]) * t;
        out->rgba[k] = base->rgba[k] - (base->rgba[k] - other->rgba[k]) * t;
    }
    for (k = 0; k < 3; k++)
        out->stq[k] = base->stq[k] - (base->stq[k] - other->stq[k]) * t;
    for (k = 0; k < 2; k++)
        out->clip.xyz[k] = base->clip.xyz[k] - (base->clip.xyz[k] - other->clip.xyz[k]) * t;
    out->clip.f.flags = VuClip(out->clip.xyz, guardW) & pl->keep;  /* PORT: z (stale in the ROM) is 0 */
}

static void AppendPrim(GA_REDRAW_STATE *st, u_int a, u_int b, u_int c, u_int tableEE)
{
    GA_REDRAW_PRIM *p = GsPrimPtr(st->listEndEE);

    if (!p)
        return;     /* PORT: the list is full (the ROM writes on into ga_div_redraw_gsvertex_work) */
    p->vertexEE[0] = a;
    p->vertexEE[1] = b;
    p->vertexEE[2] = c;
    p->primTableEE = tableEE;
    st->listEndEE += sizeof(GA_REDRAW_PRIM);
}

// CT3 0x0189f994
// A primitive some of whose vertices are outside: cut at the first plane (in kGuardPlanes' order) any of
// them is outside of. The vertices are taken in their cyclic order from the one that differs: with one
// outside (C), A and B the others, the two cut points n1 (on C-A) and n2 (on C-B) make the quad's two
// triangles (A, B, n1) and (B, n2, n1); with two outside, the inside one (A) and the cut points on A-B and
// A-C make one. The new vertices go at GA_SPR_WORK.gsVertexEE (two each time), the triangles at the list's
// end, where the loop meets them again for the remaining planes. (The ROM jumps to the cut through s2 and
// returns through ra, the caller's continue.)
void gaDrawObject_Redraw_Polygon_DivSub(GA_REDRAW_STATE *st, const GA_REDRAW_PRIM *prim, u_int orFlags)
{
    GA_SPR_WORK *spr = SPR_WORK;
    const GUARD_PLANE *pl = NULL;
    const GA_GS_VERTEX *v[3];
    u_int out[3], ee[3], nEE;
    GA_GS_VERTEX *n1, *n2;
    int k, first, outside = 0;
    float bounds[2];

    for (k = 0; k < 4 && !pl; k++)
        if (orFlags & kGuardPlanes[k].flag)
            pl = &kGuardPlanes[k];
    if (!pl)
        return;
    for (k = 0; k < 3; k++) {
        ee[k] = prim->vertexEE[k];
        v[k] = GsVertexPtr(ee[k]);
        if (!v[k])
            return;
        out[k] = (v[k]->clip.f.flags & pl->flag) != 0;
        outside += (int)out[k];
    }
    // The vertex the case turns on: the one outside (one out), or the one inside (two out).
    for (first = 0; first < 3; first++)
        if ((int)out[first] == (outside == 1))
            break;
    nEE = spr->gsVertexEE;
    n1 = GsVertexPtr(nEE);
    n2 = GsVertexPtr(nEE + sizeof(GA_GS_VERTEX));
    if (!n1 || !n2)
        return;     /* PORT: the vertex buffer is full (the ROM writes on) */
    bounds[0] = spr->minusHalf230;
    bounds[1] = spr->half234;
    if (outside == 1) {
        int a = (first + 1) % 3, b = (first + 2) % 3, c = first;

        CutPoint(n1, v[c], v[a], pl, bounds[pl->bound], spr->half23c);
        CutPoint(n2, v[c], v[b], pl, bounds[pl->bound], spr->half23c);
        AppendPrim(st, ee[a], ee[b], nEE, prim->primTableEE);
        AppendPrim(st, ee[b], nEE + sizeof(GA_GS_VERTEX), nEE, prim->primTableEE);
    } else {
        int a = first, b = (first + 1) % 3, c = (first + 2) % 3;

        CutPoint(n1, v[a], v[b], pl, bounds[pl->bound], spr->half23c);
        CutPoint(n2, v[a], v[c], pl, bounds[pl->bound], spr->half23c);
        AppendPrim(st, ee[a], nEE, nEE + sizeof(GA_GS_VERTEX), prim->primTableEE);
    }
    spr->gsVertexEE = nEE + 2 * sizeof(GA_GS_VERTEX);
}

// One GS vertex in the packet: ST(Q) as it is, RGBAQ and XYZF2 through VFTOI0.
static void PutGsVertex(u_char *p, const GA_GS_VERTEX *v)
{
    int k, c[4];

    memcpy(p, v->stq, 0x10);
    for (k = 0; k < 4; k++)
        c[k] = FtoI0(v->rgba[k]);
    memcpy(p + 0x10, c, 0x10);
    for (k = 0; k < 4; k++)
        c[k] = FtoI0(v->xyzf[k]);
    memcpy(p + 0x20, c, 0x10);
}

// The packet's start: the cursor (GA_SPR_WORK 0x14), back at the buffer's start once it reaches the wrap
// point; the GS data begins two qwords in (the VIF head and the GIFtag).
static void BeginPacket(GA_REDRAW_STATE *st, const GA_SPR_WORK *spr, int bank)
{
    u_int head = spr->divRedrawPacketEE;

    if ((int)(GA_DIV_REDRAW_GSPACKET_WORK_EE + GA_GSPACKET_WRAP + GA_EE_UNCACHED_ACCEL - head) <= 0)
        head = GA_DIV_REDRAW_GSPACKET_WORK_EE + GA_EE_UNCACHED_ACCEL;
    st->packetHeadEE = head;
    st->packetEE = head + 0x20;
    st->count = 0;
    st->listEndEE = spr->gsIndexEndEE;
    st->bank = bank;
}

// Both redraws' loop: header = 1 puts the primitive's 3-qword GS header (its GA_OBJ_PRIM_TABLE entry)
// before its vertices.
static void RedrawLoop(GA_REDRAW_STATE *st, int header)
{
    u_int cur;
    u_int size = header ? 0xc0 : 0x90;

    for (cur = GA_DIV_REDRAW_GSINDEX_WORK_EE; cur != st->listEndEE; cur += sizeof(GA_REDRAW_PRIM)) {
        const GA_REDRAW_PRIM *p = GsPrimPtr(cur);
        const GA_GS_VERTEX *v[3];
        u_int f[3];
        u_char *pkt;
        int k;

        if (!p)
            break;
        for (k = 0; k < 3; k++) {
            v[k] = GsVertexPtr(p->vertexEE[k]);
            f[k] = v[k] ? v[k]->clip.f.flags : GA_GUARD_ALL;
        }
        if (!v[0] || !v[1] || !v[2])
            continue;
        if (f[0] & f[1] & f[2] & GA_GUARD_ALL)
            continue;           /* all outside one plane */
        if (f[0] | f[1] | f[2]) {
            gaDrawObject_Redraw_Polygon_DivSub(st, p, f[0] | f[1] | f[2]);
            continue;
        }
        pkt = GsPacketPtr(st->packetEE, size);
        if (pkt) {
            u_char *at = pkt;

            if (header) {
                const void *table = ObjEePtr(st->bank, p->primTableEE);

                if (table)
                    memcpy(at, table, 0x30);
                else
                    memset(at, 0, 0x30);
                at += 0x30;
            }
            for (k = 0; k < 3; k++)
                PutGsVertex(at + k * 0x30, v[k]);
        }
        st->packetEE += size;
        st->count++;
    }
}

// The Output routines' end: the packet's VIF head (DMAtag 0, NOP, NOP, DIRECT qwc + 1) and GIFtag (giftag's
// with NLOOP = the primitives, EOP), the DMA chain at scratchpad 0x9e0, then fromSPR into the MFIFO once the
// ring has room. PORT: the chain is written, not started (the host draws the whole object, (kv)).
static void OutputPacket(GA_REDRAW_STATE *st, const u_long giftag[2])
{
    GA_SPR_WORK *spr = SPR_WORK;
    u_int qwc;
    u_char *head;
    u_int *chain = (u_int *)SCRATCHPAD(GA_SPR_REDRAW_CHAIN);

    spr->divRedrawPacketEE = st->packetEE;
    if (st->count == 0)
        return;
    qwc = (st->packetEE - st->packetHeadEE - 0x20) >> 4;
    head = GsPacketPtr(st->packetHeadEE, 0x20);
    if (head) {
        memset(head, 0, 0x10);
        ((u_int *)head)[3] = VIF_DIRECT_CODE + 1 + qwc;
        memcpy(head + 0x10, giftag, 0x10);
        *(u_short *)(head + 0x10) = (u_short)(st->count | GIFTAG_EOP);
    }
    if (spr->drawAttr != 0) {
        u_int attr = spr->drawAttr;
        u_int addr = attr + 0x10;
        // PORT: the host reaches only a scratchpad upload (gaBGT_DrawObject.c's SetupClipTail does the same).
        u_int n = (int)(attr - SCRATCHPAD_EE_BASE) >= 0 ? *(const u_short *)SCRATCHPAD(attr - SCRATCHPAD_EE_BASE) : 0;

        if ((int)(addr - SCRATCHPAD_EE_BASE) >= 0)
            addr += DMA_ADDR_SPR_DATA;
        chain[0] = DMA_REF + n;
        chain[1] = addr;
        chain[4] = DMA_REF + 2 + qwc;
        chain[5] = st->packetHeadEE & DMA_ADDR_MASK;
    } else {
        chain[0] = DMA_REF + 2 + qwc;
        chain[1] = st->packetHeadEE & DMA_ADDR_MASK;
    }
}

// CT3 0x018a00f8
void gaDrawObject_Redraw_Polygon_Output(GA_REDRAW_STATE *st)
{
    OutputPacket(st, SPR_WORK->giftagTri);
}

// CT3 0x018a026c
void gaDrawObject1_Redraw_Polygon_Output(GA_REDRAW_STATE *st)
{
    OutputPacket(st, SPR_WORK->giftagTriNop);
}

// CT3 0x0189f720
// The divided polygons to the GS: nothing when SetBig / SetNear put none (GA_SPR_WORK.divRedrawIndexEE at
// the buffer's start); else trans_for_gs, the clipping loop, and the packet's send.
void gaDrawObject_Redraw_Polygon(u_char *obj)
{
    GA_SPR_WORK *spr = SPR_WORK;
    GA_REDRAW_STATE st;

    if (spr->divRedrawIndexEE == GA_DIV_REDRAW_INDEX_WORK_EE)
        return;
    trans_for_gs(obj, obj[GA_OBJ_FLAGS]);
    BeginPacket(&st, spr, BankOfHostPtr(obj));
    RedrawLoop(&st, 0);
    gaDrawObject_Redraw_Polygon_Output(&st);
}

// CT3 0x0189f84c
// The same with each primitive's GS header from GA_OBJ_PRIM_TABLE (no caller in the ROM).
void gaDrawObject1_Redraw_Polygon(u_char *obj)
{
    GA_SPR_WORK *spr = SPR_WORK;
    GA_REDRAW_STATE st;

    if (spr->divRedrawIndexEE == GA_DIV_REDRAW_INDEX_WORK_EE)
        return;
    trans_for_gs(obj, obj[GA_OBJ_FLAGS]);
    BeginPacket(&st, spr, BankOfHostPtr(obj));
    RedrawLoop(&st, 1);
    gaDrawObject1_Redraw_Polygon_Output(&st);
}
