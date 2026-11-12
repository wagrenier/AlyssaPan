// ga/gaDrawObject.c -- Clock Tower 3, the EE side of the ga draw object's local-view path (CT3
// 0x018a4fd8..0x018a6b70, a hand-written asm object: a .text section symbol starts it, tumap.py; its first
// two words, nop and a stray j gaDMAStopMessage_VIF1, are gaBGT_DrawObject.c's tail).
//
// Every function keeps its state in the scratchpad (t9 = 0x70000000, GA_SPR_WORK) and reads the object
// (a0) in place. The vertex block they share sits in the BGT packet at vertexPacketEE: a 0x20-byte head
// (DMAtag CNT, the VIF UNPACK V4-32 code), then one qword per vertex in the local view (GA_LOCAL_VERTEX):
// xyz, the clip flags at +0xc, and +0xe set to GA_VERTEX_USED by the index pass when a kept polygon uses
// the vertex.
//
// Slice 1 (2026-11-12): the local views (not the billboards), the three index passes and the vertex
// reference tables, from the disassembly (mipsdis.py) and the VU0 micro routines (vudis.py 18fe500 f3 ...).
// Still stubs: the billboard views (micro 0x0e9 / 0x112 and 0x13b / 0x148, with the Z-roll rows at
// scratchpad 0x300) and the big / near division (SetBig / SetNear; open-items.md).

#include "common/alyssapan_stub.h"
#include "ga/gaBGT_DrawObject.h"
#include "ga/gaDrawObject.h"
#include "ga/gaInitDrawObject.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "sdk/eekernel.h"

#include <string.h>

#define SPR_WORK ((GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET))

// The division lists the index passes fill (symtab objects, 0x200 bytes each: 64 raw index entries).
// gaBGT_DrawObject points GA_SPR_WORK's cursors at their EE addresses; SetBig / SetNear read them back.
#define GA_DIV_INDEX_ENTRIES (0x200 / 8)
u_long ga_bigdiv_index_work[GA_DIV_INDEX_ENTRIES];  // CT3 0x0193e6c0
u_long ga_neardiv_index_work[GA_DIV_INDEX_ENTRIES]; // CT3 0x0193e8c0

// PORT: a cursor (an EE address in one of the lists) as a host pointer; NULL past a list's end (the ROM
// writes on, into the next symtab object).
static u_long *DivListPtr(u_int ee)
{
    if (ee - GA_BIGDIV_INDEX_WORK_EE < sizeof(ga_bigdiv_index_work))
        return &ga_bigdiv_index_work[(ee - GA_BIGDIV_INDEX_WORK_EE) / 8];
    if (ee - GA_NEARDIV_INDEX_WORK_EE < sizeof(ga_neardiv_index_work))
        return &ga_neardiv_index_work[(ee - GA_NEARDIV_INDEX_WORK_EE) / 8];
    return NULL;
}

// A scratchpad EE address as a host pointer.
static u_char *SprPtr(u_int ee)
{
    return (u_char *)SCRATCHPAD(ee - SCRATCHPAD_EE_BASE);
}

// The vertex block's first vertex, after its DMAtag / VIF head.
static GA_LOCAL_VERTEX *LocalVertices(const GA_SPR_WORK *spr)
{
    return (GA_LOCAL_VERTEX *)(SprPtr(spr->vertexPacketEE) + GA_VERTEX_BLOCK_HEAD);
}

// CT3 0x018a5fe0
// The used vertices' indices, packed at the block's end: walking down from the last vertex, each one an
// index pass marked used writes its index into the next slot down (slot >= vertex, so a slot's own mark
// is read before it is overwritten). drawVertexCount = the slots written; the set-ups send that many.
void gaDrawObject_MakeVertexReferenceTable(u_char *obj)
{
    GA_SPR_WORK *spr = SPR_WORK;
    GA_LOCAL_VERTEX *v = LocalVertices(spr);
    int n = (int)*(const u_int *)(obj + GA_OBJ_VERTEX_COUNT);
    int slot = n;
    int i;

    for (i = n - 1; i >= 0; i--) {
        if (v[i].w.f.used == GA_VERTEX_USED)
            v[--slot].w.ref = (u_int)i;
    }
    spr->drawVertexCount = (u_int)(n - slot);
}

// CT3 0x018a6038
// A culled object (no GA_OBJ_FLAG_DOUBLE_SIDED) takes the table above; a double-sided one references
// every vertex in order.
void gaDrawObject_MakeVertexReferenceTableNoClip(u_char *obj)
{
    GA_SPR_WORK *spr = SPR_WORK;
    GA_LOCAL_VERTEX *v;
    u_int n, i;

    if (!(obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_DOUBLE_SIDED)) {
        gaDrawObject_MakeVertexReferenceTable(obj);    /* the ROM's beq to 0x018a5fe0 */
        return;
    }
    v = LocalVertices(spr);
    n = *(const u_int *)(obj + GA_OBJ_VERTEX_COUNT);
    // PORT: the ROM's loop is a do-while (n = 0 would run off the block); objects have vertices.
    for (i = 0; i < n; i++)
        v[i].w.ref = i;
    spr->drawVertexCount = n;
}

// ---- the local views (slice 1) ----

// The block heads both views and the index passes write: the DMAtag CNT (1 + qwords) in the low word, the
// next qword zero but its VIF code (UNPACK of n elements).
#define DMA_CNT 0x10000000u
#define VIF_UNPACK_V4_32(n) (0x6c000000u | ((u_int)(n) << 16))          /* to VU1 0 */
#define VIF_UNPACK_V4_16(n) (0x6d000000u | ((u_int)(n) << 16) | 0x17f)  /* to VU1 0x17f */

static void PutBlockHead(u_char *head, u_int qwc, u_int vifcode)
{
    *(u_int *)head = DMA_CNT + qwc;
    memset(head + 0x10, 0, 0x10);
    ((u_int *)(head + 0x10))[3] = vifcode;
}

// The views' constant at scratchpad 0x2cc (VU0's vf3.w): the smallest Z the perspective divides by.
#define GA_SPR_VU0_ZMIN 0x2cc
#define GA_VU0_ZMIN_BITS 0x29e12e13u    /* 1.0e-13f */

// v * m with v's w taken as 1 (the micro routines' mulax / madday / maddaz / maddw by vf0.w), all four
// components.
static void ToLocal(float out[4], const float *m, const float *v)
{
    int c;

    for (c = 0; c < 4; c++)
        out[c] = v[0] * m[c] + v[1] * m[4 + c] + v[2] * m[8 + c] + m[12 + c];
}

static float AbsF(float f) { return f < 0.0f ? -f : f; }

// VU CLIP's six bits for |w|: +x -x +y -y +z -z.
static u_int ClipBits(const float *p, float w)
{
    u_int f = 0;

    w = AbsF(w);
    if (p[0] > w) f |= 0x01;
    if (p[0] < -w) f |= 0x02;
    if (p[1] > w) f |= 0x04;
    if (p[1] < -w) f |= 0x08;
    if (p[2] > w) f |= 0x10;
    if (p[2] < -w) f |= 0x20;
    return f;
}

// The clip flags each vertex carries in w (micro 0x079 / 0x0a1: CLIP against vf0.w = 1, then vf2.w = 0.5;
// FCGET; (~fc & 0x3f0) | (fc & 0xf)). Set bits mean, from the bottom: x / y beyond the screen (+-0.5:
// +x -x +y -y), Z between near and far (+-0.5 after the Z scale: GA_CLIP_Z_INSIDE), x / y inside the
// guard box (+-1: GA_CLIP_XY_INSIDE).
#define GA_CLIP_SCREEN_OUT 0x00f        /* beyond the screen's side, one bit per side */
#define GA_CLIP_Z_FAR_INSIDE 0x010      /* not beyond far */
#define GA_CLIP_Z_NEAR_INSIDE 0x020     /* not before near */
#define GA_CLIP_XY_INSIDE 0x3c0         /* inside the guard box, one bit per side */
#define GA_CLIP_INSIDE 0x3f0            /* the index passes' accept mask: all of the above inside */
#define GA_CLIP_FRONT 0x400             /* the index passes put the facing test here (VU0 Sx << 3) */

static u_short ClipFlags(const float *p)
{
    u_int fc = (ClipBits(p, 1.0f) << 6) | ClipBits(p, 0.5f);

    return (u_short)((~fc & GA_CLIP_INSIDE) | (fc & GA_CLIP_SCREEN_OUT));
}

// The view's common frame (both forms): the object's matrix, positions and vertex count; the block's
// head; stPacketEE = the block's end. The ROM transforms 4 vertices a pass (the slots up to the next 4 are
// scratch, overwritten by the next block); the host transforms n.
static GA_LOCAL_VERTEX *BeginView(u_char *obj, u_int packetEE, const float **m, const float **pos, u_int *n)
{
    GA_SPR_WORK *spr = SPR_WORK;
    int bank = BankOfHostPtr(obj);
    u_char *head = SprPtr(packetEE);

    *n = *(const u_int *)(obj + GA_OBJ_VERTEX_COUNT);
    *pos = (const float *)BankEePtr(bank, *(const u_int *)(obj + GA_OBJ_POSITIONS));
    *m = (const float *)BankEePtr(bank, *(const u_int *)(obj + GA_OBJ_MATRIX));
    spr->stPacketEE = packetEE + GA_VERTEX_BLOCK_HEAD + *n * 0x10;
    return (GA_LOCAL_VERTEX *)(head + GA_VERTEX_BLOCK_HEAD);
}

// CT3 0x018a4fe0
// Each vertex through the object's matrix, then the screen scales and the perspective divide on VU0
// (micro 0x079 / 0x0a1, vf1 = scratchpad 0x30, vf2 = 0x40): x, y * viewScale * work6 / max(z, 1e-13); Z to
// zScale * z + zOffset (near..far -> -0.5..0.5). w = the clip flags (ClipFlags). VU0 keeps its own copy
// (sqi from 0x004 of its memory); the host's readers take the packet's.
void gaDrawObject_VertexLocalViewWithClippingFlagSet(u_char *obj, u_int packetEE)
{
    GA_SPR_WORK *spr = SPR_WORK;
    const float *m, *pos;
    u_int n, i;
    GA_LOCAL_VERTEX *out = BeginView(obj, packetEE, &m, &pos, &n);
    float zMin;

    *(u_int *)SprPtr(SCRATCHPAD_EE_BASE + GA_SPR_VU0_ZMIN) = GA_VU0_ZMIN_BITS;
    memcpy(&zMin, SprPtr(SCRATCHPAD_EE_BASE + GA_SPR_VU0_ZMIN), sizeof(zMin));
    for (i = 0; i < n; i++) {
        float p[4], q, s[3];

        ToLocal(p, m, pos + i * 4);
        q = spr->work6 / (p[2] > zMin ? p[2] : zMin);
        s[0] = p[0] * spr->viewScaleX * q + 0.0f;
        s[1] = p[1] * spr->viewScaleY * q + 0.0f;
        s[2] = p[2] * spr->zScale + spr->zOffset;
        memcpy(out[i].xyz, s, sizeof(s));
        out[i].w.ref = ClipFlags(s);    /* mfir.w: the flags as an integer word (+0xe clear) */
    }
    PutBlockHead(SprPtr(packetEE), 1 + n, VIF_UNPACK_V4_32(n));
}

// CT3 0x018a5100
// The no-clip view (micro 0x0c9 / 0x0d9): the object's matrix only, all four components kept.
void gaDrawObject_VertexLocalView(u_char *obj, u_int packetEE)
{
    const float *m, *pos;
    u_int n, i;
    GA_LOCAL_VERTEX *out = BeginView(obj, packetEE, &m, &pos, &n);

    for (i = 0; i < n; i++)
        ToLocal((float *)&out[i], m, pos + i * 4);
    PutBlockHead(SprPtr(packetEE), 1 + n, VIF_UNPACK_V4_32(n));
}

// ---- the index passes (slice 1) ----

// The facing test (micro 0x155 / 0x161 for the clipped passes, 0x16d / 0x183 for RemakeIndex): the
// normal (b - a) x (c - a) dotted with a, summed x + y, then + z; VU0's MAC flag Sx (0x80) / Zx (0x08).
static float FacingDot(const float *a, const float *b, const float *c)
{
    float e1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    float e2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    float n[3] = {e1[1] * e2[2] - e2[1] * e1[2], e1[2] * e2[0] - e2[2] * e1[0], e1[0] * e2[1] - e2[0] * e1[1]};

    return n[0] * a[0] + n[1] * a[1] + n[2] * a[2];
}

#define VU_MAC_SX 0x80
#define VU_MAC_ZX 0x08

// GA_OBJ_FLAGS bit 0 (the variant's low bit) flips which side faces: the clipped passes want Sx clear,
// RemakeIndex wants Sx and Zx clear.
#define GA_OBJ_FLAG_FLIP 0x01

// One index record entry: three vertex offsets (vertex * 16) and a fourth halfword; the passes send all
// four >> 4 (PSRLH 4).
static u_long EntryForVu1(u_long e)
{
    u_long r = 0;
    int k;

    for (k = 0; k < 4; k++)
        r |= (u_long)(((u_short)(e >> (k * 16))) >> 4) << (k * 16);
    return r;
}

static void MarkUsed(GA_LOCAL_VERTEX *v, u_long e)
{
    int k;

    for (k = 0; k < 3; k++)
        v[((u_short)(e >> (k * 16))) >> 4].w.f.used = GA_VERTEX_USED;
}

// The index block's end: a zero entry (not on the double-sided copy), the count, the block head (CNT
// 1 + ceil(count / 2) qwords, UNPACK V4-16 count to VU1 0x17f) and tailPacketEE (16-aligned after it).
static void EndIndex(u_int packetEE, u_long *outEnd, u_int count, int terminate)
{
    GA_SPR_WORK *spr = SPR_WORK;
    u_int endEE = packetEE + 0x20 + count * 8;

    if (terminate)
        *outEnd = 0;
    spr->drawPrimCount = count;
    PutBlockHead(SprPtr(packetEE), 1 + ((count + 1) >> 1), VIF_UNPACK_V4_16(count));
    spr->tailPacketEE = (endEE + 8) & ~0xfu;
}

// Push a raw entry on a division list (GA_SPR_WORK's big / near cursors, EE addresses of symtab buffers).
static void PushDiv(u_int *cursorEE, u_long e)
{
    // PORT: the lists are ROM buffers (ga_bigdiv_index_work, ga_neardiv_index_work); the host keeps them
    // here, at the same EE addresses' offsets.
    u_long *p = DivListPtr(*cursorEE);

    if (p)
        *p = e;
    *cursorEE += 8;
}

// The two clipped passes (0x018a53c8, 0x018a5730 with near): a polygon whose three vertices are all
// inside (GA_CLIP_INSIDE, and none beyond a screen side for all three) and that faces the camera is kept;
// one inside Z that faces it but leaves the guard box goes to the big list; with near, one that crosses
// the near plane goes to the near list (facing untested), one wholly before it is dropped. A double-sided
// object skips the facing test.
static void RemakeClipped(u_char *obj, u_int packetEE, int withNear)
{
    GA_SPR_WORK *spr = SPR_WORK;
    int bank = BankOfHostPtr(obj);
    u_char flags = obj[GA_OBJ_FLAGS];
    GA_LOCAL_VERTEX *v = LocalVertices(spr);
    const u_long *in = (const u_long *)BankEePtr(bank, *(const u_int *)(obj + GA_OBJ_INDICES));
    u_long *out = (u_long *)(SprPtr(packetEE) + 0x20);
    u_int prims = *(const u_int *)(obj + GA_OBJ_PRIM_COUNT);
    int sided = !(flags & GA_OBJ_FLAG_DOUBLE_SIDED);
    u_int accept = sided ? (GA_CLIP_INSIDE | GA_CLIP_FRONT) ^ ((flags & GA_OBJ_FLAG_FLIP) << 10) : GA_CLIP_INSIDE;
    u_int count = 0, t;

    // PORT: the ROM's loop runs prims entries, preloading entry prims (its end test is prims + 1 entries
    // on); prims = 0 would never end. The host reads prims entries.
    for (t = 0; t < prims; t++) {
        u_long e = in[t];
        const GA_LOCAL_VERTEX *a = &v[(u_short)e >> 4];
        const GA_LOCAL_VERTEX *b = &v[(u_short)(e >> 16) >> 4];
        const GA_LOCAL_VERTEX *c = &v[(u_short)(e >> 32) >> 4];
        u_int s = a->w.f.clip & b->w.f.clip & c->w.f.clip;

        if (sided && FacingDot(a->xyz, b->xyz, c->xyz) < 0.0f)
            s |= VU_MAC_SX << 3;
        if (s == accept) {
            MarkUsed(v, e);
            out[count++] = EntryForVu1(e);
            continue;
        }
        if (withNear) {
            u_int any = a->w.f.clip | b->w.f.clip | c->w.f.clip;

            if (!(any & GA_CLIP_Z_NEAR_INSIDE))
                continue;                               /* wholly before near */
            if (!(s & GA_CLIP_Z_NEAR_INSIDE)) {
                PushDiv(&spr->nearDivIndexEE, e);       /* crosses near */
                continue;
            }
        }
        if ((s | GA_CLIP_XY_INSIDE) == accept)
            PushDiv(&spr->bigDivIndexEE, e);
    }
    EndIndex(packetEE, &out[count], count, 1);
}

// CT3 0x018a53c8
void gaDrawObject_RemakeIndexWithClippingCheck(u_char *obj, u_int packetEE)
{
    RemakeClipped(obj, packetEE, 0);
}

// CT3 0x018a5730
void gaDrawObject_RemakeIndexWithClippingCheck_WithNear(u_char *obj, u_int packetEE)
{
    RemakeClipped(obj, packetEE, 1);
}

// CT3 0x018a5b88
// The no-clip pass: facing only, two polygons a step (micro 0x16d / 0x183 read the vertices from VU0's
// copy at 4 + (vertex & 0x7f)). A double-sided object's entries are copied whole, >> 4, unmarked, with no
// zero entry after them (MakeVertexReferenceTableNoClip then references every vertex).
void gaDrawObject_RemakeIndex(u_char *obj, u_int packetEE)
{
    GA_SPR_WORK *spr = SPR_WORK;
    int bank = BankOfHostPtr(obj);
    u_char flags = obj[GA_OBJ_FLAGS];
    GA_LOCAL_VERTEX *v = LocalVertices(spr);
    const u_long *in = (const u_long *)BankEePtr(bank, *(const u_int *)(obj + GA_OBJ_INDICES));
    u_long *out = (u_long *)(SprPtr(packetEE) + 0x20);
    u_int prims = *(const u_int *)(obj + GA_OBJ_PRIM_COUNT);
    u_int accept, count = 0, t;

    if (flags & GA_OBJ_FLAG_DOUBLE_SIDED) {
        // PORT: the ROM copies whole 64-byte blocks (prims * 8 + 0x38 rounded down to 0x40); the host the
        // entries.
        for (t = 0; t < prims; t++)
            out[t] = EntryForVu1(in[t]);
        EndIndex(packetEE, &out[prims], prims, 0);
        return;
    }
    accept = VU_MAC_SX ^ ((flags & GA_OBJ_FLAG_FLIP) << 7);
    // PORT: the ROM tests pairs, so an odd count tests entry prims too (a zero entry: a degenerate polygon,
    // Zx set, dropped); the host tests prims entries. It also reads VU0's copy, vertex & 0x7f: an object
    // over 128 vertices would read the wrong ones there -- the host reads the packet's.
    for (t = 0; t < prims; t++) {
        u_long e = in[t];
        float d = FacingDot(v[(u_short)e >> 4].xyz, v[(u_short)(e >> 16) >> 4].xyz, v[(u_short)(e >> 32) >> 4].xyz);
        u_int mac = d < 0.0f ? VU_MAC_SX : d == 0.0f ? VU_MAC_ZX : 0;

        if (mac != accept)
            continue;
        out[count++] = EntryForVu1(e);
        MarkUsed(v, e);
    }
    EndIndex(packetEE, &out[count], count, 1);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018a6080
void gaDrawObject_SetBig(u_char *obj)
{
    ALYSSAPAN_STUB("gaDrawObject_SetBig", 0x018a6080);
}

// stub of CT3 0x018a621c
void gaDrawObject_SetNear(u_char *obj)
{
    ALYSSAPAN_STUB("gaDrawObject_SetNear", 0x018a621c);
}

// stub of CT3 0x018a51e8
void gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard(u_char *obj, u_int packetEE)
{
    ALYSSAPAN_STUB("gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard", 0x018a51e8);
}

// stub of CT3 0x018a52f0
void gaDrawObject_VertexLocalView_Billboard(u_char *obj, u_int packetEE)
{
    ALYSSAPAN_STUB("gaDrawObject_VertexLocalView_Billboard", 0x018a52f0);
}
