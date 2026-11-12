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
// Slice 2 (2026-11-12): the billboard views (micro 0x0e9 / 0x112 and 0x13b / 0x148, with the Z-roll rows
// at scratchpad 0x300), and the clipped view's packet corrected to the view space.
// Slice 3 (2026-11-12): the big / near division (SetBig / SetNear), from the disassembly.

#include "ga/gaBGT_DrawObject.h"
#include "ga/gaDrawObject.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaInitDrawObject.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "sdk/eekernel.h"

#include <math.h>
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

// The same with v's own w (the billboard micros' maddw by vf10.w).
static void ToLocalW(float out[4], const float *m, const float *v)
{
    int c;

    for (c = 0; c < 4; c++)
        out[c] = v[0] * m[c] + v[1] * m[4 + c] + v[2] * m[8 + c] + v[3] * m[12 + c];
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

// A view-space point's clip flags: its screen position (x, y * viewScale * q, Z to zScale * z + zOffset;
// the micros' mul by vf1, mulq.xy, add vf2) through ClipFlags. The flags only: the packet keeps the view.
static u_short ScreenClipFlags(const GA_SPR_WORK *spr, const float *p, float q)
{
    float s[3];

    s[0] = p[0] * spr->viewScaleX * q + 0.0f;
    s[1] = p[1] * spr->viewScaleY * q + 0.0f;
    s[2] = p[2] * spr->zScale + spr->zOffset;
    return ClipFlags(s);
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
// Each vertex through the object's matrix into the packet (the view: maddw into vf24..vf31, sqc2'd as they
// are), w = its clip flags from the screen scales and the perspective divide on VU0 (micro 0x079 / 0x0a1,
// vf1 = scratchpad 0x30, vf2 = 0x40): x, y * viewScale * work6 / max(z, 1e-13); Z to zScale * z + zOffset
// (near..far -> -0.5..0.5); ScreenClipFlags. The screen position itself is not kept. VU0 keeps its own
// copy of the packet's (sqi from 0x004 of its memory); the host's readers take the packet's.
// (Slice 2, 2026-11-12: slice 1 wrote the screen position here; the facing test dots with the view.)
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
        float p[4], q;

        ToLocal(p, m, pos + i * 4);
        q = spr->work6 / (p[2] > zMin ? p[2] : zMin);
        memcpy(out[i].xyz, p, sizeof(out[i].xyz));
        out[i].w.ref = ScreenClipFlags(spr, p, q);  /* mfir.w: the flags as an integer word (+0xe clear) */
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

// ---- the billboard views (slice 2) ----

// A billboard object's vertex array holds one 0x40-byte record per billboard, four vertex slots: slot 0
// the centre (xyzw, its own w: the micros' maddw by vf10.w, not vf0.w), slot 1 the half extents (x, y);
// slots 2 and 3 are not read by the view (the VU1 side's, UVs or colours). Each record becomes four
// corners in the view, around the centre, in the screen plane: the two half-diagonals d0 = r0 * sx +
// r1 * sy and d1 = r0 * sx - r1 * sy (micro 0x0ed..0x0ef), r0 / r1 the rows at scratchpad 0x300 / 0x310
// (vf8 / vf9: the Z roll; their writer is not in the ga library, open-items.md).
#define GA_SPR_BILLBOARD_ROW0 0x300
#define GA_SPR_BILLBOARD_ROW1 0x310
#define GA_BILLBOARD_SLOTS 4            /* vertex slots per billboard record */

typedef struct GA_BILLBOARD_RECORD {
    float centre[4];                    /* 0x00: slot 0 */
    float half[4];                      /* 0x10: slot 1, x and y read */
    float vu1[2][4];                    /* 0x20: slots 2, 3 */
} GA_BILLBOARD_RECORD;
static_assert(sizeof(GA_BILLBOARD_RECORD) == GA_BILLBOARD_SLOTS * 0x10, "GA_BILLBOARD_RECORD");

// The record's four corners in the view (0x0f6..0x0f9 / 0x142..0x145): c - d0, c - d1, c + d0, c + d1 in
// x, y; z and w the centre's.
static void BillboardCorners(GA_LOCAL_VERTEX out[GA_BILLBOARD_SLOTS], const float *m, const GA_BILLBOARD_RECORD *rec)
{
    const float *r0 = (const float *)SprPtr(SCRATCHPAD_EE_BASE + GA_SPR_BILLBOARD_ROW0);
    const float *r1 = (const float *)SprPtr(SCRATCHPAD_EE_BASE + GA_SPR_BILLBOARD_ROW1);
    float c[4], d0[2], d1[2];
    int k;

    ToLocalW(c, m, rec->centre);
    for (k = 0; k < 2; k++) {
        d0[k] = r0[k] * rec->half[0] + r1[k] * rec->half[1];
        d1[k] = r0[k] * rec->half[0] - r1[k] * rec->half[1];
    }
    for (k = 0; k < GA_BILLBOARD_SLOTS; k++) {
        const float *d = (k & 1) ? d1 : d0;
        float sign = (k & 2) ? 1.0f : -1.0f;
        float p[4] = {c[0] + sign * d[0], c[1] + sign * d[1], c[2], c[3]};

        memcpy(&out[k], p, sizeof(p));
    }
}

// CT3 0x018a51e8
// The clipped billboard view (micro 0x0e9 / 0x112, a record each): the corners as above, then each one's
// clip flags in w from its screen position -- one perspective divide for all four, by the centre's Z
// (max(z, 1e-13), as the plain view). VU0 keeps the screen x, y (sqi.xy) in its own memory; the host's
// readers take the packet's.
void gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard(u_char *obj, u_int packetEE)
{
    GA_SPR_WORK *spr = SPR_WORK;
    const float *m, *pos;
    u_int n, i;
    GA_LOCAL_VERTEX *out = BeginView(obj, packetEE, &m, &pos, &n);
    float zMin;

    *(u_int *)SprPtr(SCRATCHPAD_EE_BASE + GA_SPR_VU0_ZMIN) = GA_VU0_ZMIN_BITS;
    memcpy(&zMin, SprPtr(SCRATCHPAD_EE_BASE + GA_SPR_VU0_ZMIN), sizeof(zMin));
    // PORT: the ROM runs two records a pass and writes the slots up to the next 8 (scratch, as the plain
    // view's); the host writes the records n covers.
    for (i = 0; i + GA_BILLBOARD_SLOTS <= n; i += GA_BILLBOARD_SLOTS) {
        GA_LOCAL_VERTEX *v = &out[i];
        float q;
        int k;

        BillboardCorners(v, m, (const GA_BILLBOARD_RECORD *)(pos + i * 4));
        q = spr->work6 / (v[0].xyz[2] > zMin ? v[0].xyz[2] : zMin);
        for (k = 0; k < GA_BILLBOARD_SLOTS; k++)
            v[k].w.ref = ScreenClipFlags(spr, v[k].xyz, q);
    }
    PutBlockHead(SprPtr(packetEE), 1 + n, VIF_UNPACK_V4_32(n));
}

// CT3 0x018a52f0
// The plain billboard view (micro 0x13b / 0x148): the corners only, w the centre's.
void gaDrawObject_VertexLocalView_Billboard(u_char *obj, u_int packetEE)
{
    const float *m, *pos;
    u_int n, i;
    GA_LOCAL_VERTEX *out = BeginView(obj, packetEE, &m, &pos, &n);

    for (i = 0; i + GA_BILLBOARD_SLOTS <= n; i += GA_BILLBOARD_SLOTS)
        BillboardCorners(&out[i], m, (const GA_BILLBOARD_RECORD *)(pos + i * 4));
    PutBlockHead(SprPtr(packetEE), 1 + n, VIF_UNPACK_V4_32(n));
}

// ---- the big list's set-up (slice 2) ----

// The redraw buffers GA_SPR_WORK's 0x20 / 0x24 cursors walk (symtab objects, defined with their reader in
// gaDrawObject_Redraw_Polygon.c; the host keeps them at the same EE addresses' offsets, as the division lists).
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

// The scratchpad qwords SetBig / SetNear stage their ST and colour reads through (0x2c0 / 0x2d0 / 0x2e0:
// the views' vf3 at 0x2c0, its w the 1e-13 at 0x2cc, is overwritten here).
#define GA_SPR_DIV_STAGE 0x2c0

// layout: host (pointers into the object's arrays; no EE counterpart)
// What the division reads a polygon's vertices from: the local view, and the object's normal / ST /
// colour arrays past their VIF head qword (obj +0x64 / 0x68 / 0x6c + 0x10; no normals when +0x64 is 0).
typedef struct DIV_SOURCE {
    const GA_LOCAL_VERTEX *view;
    const u_char *normals;      /* a qword per vertex: + the vertex offset */
    const u_char *st;           /* 8 bytes per vertex: + offset / 2 */
    const u_char *rgba;         /* 4 bytes per vertex: + offset / 4 */
    u_int primTableEE;          /* obj +0x54 + 0x10 */
    u_char *stage;
} DIV_SOURCE;

static void DivSource(DIV_SOURCE *src, const u_char *obj)
{
    int bank = BankOfHostPtr(obj);
    u_int normalsEE = *(const u_int *)(obj + GA_OBJ_NORMALS);

    src->view = LocalVertices(SPR_WORK);
    src->normals = normalsEE ? (const u_char *)BankEePtr(bank, normalsEE) + 0x10 : NULL;
    src->st = (const u_char *)BankEePtr(bank, *(const u_int *)(obj + GA_OBJ_ST)) + 0x10;
    src->rgba = (const u_char *)BankEePtr(bank, *(const u_int *)(obj + GA_OBJ_COLOURS)) + 0x10;
    src->primTableEE = *(const u_int *)(obj + GA_OBJ_PRIM_TABLE) + 0x10;
    src->stage = SprPtr(SCRATCHPAD_EE_BASE + GA_SPR_DIV_STAGE);
}

// A polygon's three vertices, in the order perm gives (out[j] = vertex perm[j]): the local-view qword,
// the normal, the ST, the colour (V4-8, sign-extended to integers, then ITOF0: the MMI PEXTLB / PEXTLH /
// PSRAW 24 and VITOF0). The STs are staged in the entry's order (sd: the low half of 0x2c0 + 0x10 k) and
// read back as whole qwords, so an ST's high half is what the staging qword held (the near value at 0x2c8
// at first, then the previous polygon's colour words; ROM-exact, kept); the colours are staged in perm's.
static void LoadPolygon(GA_REDRAW_VERTEX out[3], const DIV_SOURCE *src, const u_short off[3], const int perm[3])
{
    int j, k;

    for (k = 0; k < 3; k++)
        memcpy(src->stage + k * 0x10, src->st + off[k] / 2, 8);
    for (j = 0; j < 3; j++) {
        u_short o = off[perm[j]];

        memcpy(out[j].view, &src->view[o >> 4], 0x10);
        if (src->normals)
            memcpy(out[j].normal, src->normals + o, 0x10);
        else
            memset(out[j].normal, 0, 0x10);     /* PORT: the ROM stores whatever vf10..vf12 held */
        memcpy(out[j].st, src->stage + perm[j] * 0x10, 0x10);
    }
    for (j = 0; j < 3; j++) {
        int c[4];

        for (k = 0; k < 4; k++)
            c[k] = (signed char)src->rgba[off[perm[j]] / 4 + k];
        memcpy(src->stage + j * 0x10, c, sizeof(c));
    }
    for (j = 0; j < 3; j++)
        for (k = 0; k < 4; k++)
            out[j].colour[k] = (float)((const int *)(src->stage + j * 0x10))[k];
}

// layout: host (the ROM's t5 / t6)
// The output cursors: the vertex and primitive records written so far (EE addresses in the redraw
// buffers). PORT: a record past a buffer's end is not written (the ROM writes on).
typedef struct DIV_OUT {
    u_int vertexEE;
    u_int primEE;
} DIV_OUT;

// Store n vertex records and the primitives over them (corner[p][0..2] = record numbers), each with the
// polygon's table entry.
static void EmitPolygons(DIV_OUT *o, const GA_REDRAW_VERTEX *rec, int n, const int (*corner)[3], int prims, u_int tableEE)
{
    int i, k;

    for (i = 0; i < n; i++) {
        GA_REDRAW_VERTEX *dst = RedrawVertexPtr(o->vertexEE + i * (u_int)sizeof(GA_REDRAW_VERTEX));

        if (dst)
            *dst = rec[i];
    }
    for (i = 0; i < prims; i++) {
        GA_REDRAW_PRIM *prim = RedrawPrimPtr(o->primEE);

        if (prim) {
            for (k = 0; k < 3; k++)
                prim->vertexEE[k] = o->vertexEE + corner[i][k] * (u_int)sizeof(GA_REDRAW_VERTEX);
            prim->primTableEE = tableEE;
        }
        o->primEE += sizeof(GA_REDRAW_PRIM);
    }
    o->vertexEE += n * sizeof(GA_REDRAW_VERTEX);
}

static const int kIdentity[3] = {0, 1, 2};
static const int kOneTriangle[1][3] = {{0, 1, 2}};

// CT3 0x018a6080
// Every polygon the index pass put on the big list becomes three vertex records and one primitive record
// for the redraw (gaDrawObject_Redraw_Polygon), appended at GA_SPR_WORK's redraw cursors (after SetNear's,
// when it ran): the primitive holds the three records' EE addresses and its entry in the table at obj
// +0x54 (+ the index entry's fourth halfword). The caller runs it only when the big cursor moved.
void gaDrawObject_SetBig(u_char *obj)
{
    GA_SPR_WORK *spr = SPR_WORK;
    DIV_SOURCE src;
    DIV_OUT o = {spr->divRedrawVertexEE, spr->divRedrawIndexEE};
    u_int cur;

    DivSource(&src, obj);
    for (cur = GA_BIGDIV_INDEX_WORK_EE; cur != spr->bigDivIndexEE; cur += 8) {
        const u_long *entry = DivListPtr(cur);
        GA_REDRAW_VERTEX rec[3];
        u_short off[4];

        if (!entry)
            break;      /* PORT: past the near list the ROM reads on into the next object */
        memcpy(off, entry, sizeof(off));
        LoadPolygon(rec, &src, off, kIdentity);
        EmitPolygons(&o, rec, 3, kOneTriangle, 1, src.primTableEE + off[3]);
    }
    spr->divRedrawVertexEE = o.vertexEE;
    spr->divRedrawIndexEE = o.primEE;
}

// ---- the near list's set-up (slice 2) ----

// A point on an edge at the near plane, in the two forms the ROM computes (VU0 macro code, 0x018a67b0 /
// 0x018a6934): from the vertex behind towards one in front, a + (b - a) t (two in front), or from the one
// in front away, a - (a - b) t (one in front). Normals are renormalised (RSQRT), STs lerp x, y, colours all
// four. PORT: the components the ROM leaves as the VU registers held (the view's and the normal's w, the
// ST's z, w) are 0 here.
static GA_REDRAW_VERTEX NearPoint(const GA_REDRAW_VERTEX *a, const GA_REDRAW_VERTEX *b, float t, int away, int normals)
{
    GA_REDRAW_VERTEX r;
    GA_REDRAW_VERTEX *out = &r;
    float s = away ? -1.0f : 1.0f;
    float d = away ? 1.0f : -1.0f;      /* the edge vector: away a - b, else b - a */
    int k;

    memset(out, 0, sizeof(*out));
    for (k = 0; k < 3; k++)
        out->view[k] = a->view[k] + s * ((a->view[k] - b->view[k]) * d) * t;
    if (normals) {
        float len2 = 0.0f, q;

        for (k = 0; k < 3; k++) {
            out->normal[k] = a->normal[k] + s * ((a->normal[k] - b->normal[k]) * d) * t;
            len2 += out->normal[k] * out->normal[k];
        }
        q = 1.0f / sqrtf(len2);
        for (k = 0; k < 3; k++)
            out->normal[k] *= q;
    }
    for (k = 0; k < 2; k++)
        out->st[k] = a->st[k] + s * ((a->st[k] - b->st[k]) * d) * t;
    for (k = 0; k < 4; k++)
        out->colour[k] = a->colour[k] + s * ((a->colour[k] - b->colour[k]) * d) * t;
    return r;
}

// Which vertices are behind the near plane (z <= zNear: A, B, C = bits 0, 1, 2) picks the order the ROM
// loads them in (out[0..2] = vf5, vf6, vf7) and the case: all three in front copy whole; two in front
// (the third last) become a quad of two triangles; one in front (first) a triangle; none, nothing.
enum { NEAR_DROP, NEAR_WHOLE, NEAR_TWO_IN, NEAR_ONE_IN };
static const struct { int kind; int perm[3]; } kNearCase[8] = {
    {NEAR_WHOLE, {0, 1, 2}},    /* none behind */
    {NEAR_TWO_IN, {1, 2, 0}},   /* A */
    {NEAR_TWO_IN, {2, 0, 1}},   /* B */
    {NEAR_ONE_IN, {2, 0, 1}},   /* A, B */
    {NEAR_TWO_IN, {0, 1, 2}},   /* C */
    {NEAR_ONE_IN, {1, 2, 0}},   /* A, C */
    {NEAR_ONE_IN, {0, 1, 2}},   /* B, C */
    {NEAR_DROP, {0, 1, 2}},     /* all */
};
static const int kNearQuad[2][3] = {{0, 1, 2}, {1, 3, 2}};

#define GA_REDRAW_SENTINEL 0x70000000u  /* the cull loop's extra record: t9, the scratchpad base */

// CT3 0x018a621c
// The near list's polygons cut at the near plane (the 3dlib work's zNear, view z <= zNear behind; also
// stored at scratchpad 0x2c8) into the redraw buffers, from their start: the parts in front, as vertex
// and primitive records like SetBig's. Then, unless the object is double-sided, the primitives are culled
// in place by facing (VU0 micro 0x155 / 0x161 with FMAND Sx: kept when Sx is GA_OBJ_FLAG_FLIP's
// opposite; the index pass did not test the near polygons' facing); a sentinel record (0x70000000 x4) is
// written after the last for the pipelined loop. GA_SPR_WORK's cursors end at the records written, both
// back at the buffers' start when none was.
void gaDrawObject_SetNear(u_char *obj)
{
    GA_SPR_WORK *spr = SPR_WORK;
    const GA_3DLIB_WORK *lib = (const GA_3DLIB_WORK *)SprPtr(spr->work3dlibEE);
    float zNear = lib->zNear;
    DIV_SOURCE src;
    DIV_OUT o = {GA_DIV_REDRAW_VERTEX_WORK_EE, GA_DIV_REDRAW_INDEX_WORK_EE};
    u_int cur, end, keep;
    u_char flags = obj[GA_OBJ_FLAGS];

    DivSource(&src, obj);
    memcpy(src.stage + 8, &zNear, sizeof(zNear));      /* sw 0x2c8: vf1.z for the macro code */
    for (cur = GA_NEARDIV_INDEX_WORK_EE; cur != spr->nearDivIndexEE; cur += 8) {
        const u_long *entry = DivListPtr(cur);
        GA_REDRAW_VERTEX rec[4];
        u_short off[4];
        int behind = 0, k, normals;
        float t0, t1;

        if (!entry)
            break;      /* PORT: past the list the ROM reads on into the next object */
        memcpy(off, entry, sizeof(off));
        for (k = 0; k < 3; k++)
            if (src.view[off[k] >> 4].xyz[2] <= zNear)
                behind |= 1 << k;
        if (kNearCase[behind].kind == NEAR_DROP)
            continue;
        LoadPolygon(rec, &src, off, kNearCase[behind].perm);
        normals = src.normals != NULL;
        switch (kNearCase[behind].kind) {
        case NEAR_WHOLE:
            EmitPolygons(&o, rec, 3, kOneTriangle, 1, src.primTableEE + off[3]);
            break;
        case NEAR_TWO_IN:
            // rec[2] behind: the points on its edges to rec[0] and rec[1].
            t0 = (zNear - rec[2].view[2]) / (rec[0].view[2] - rec[2].view[2]);
            t1 = (zNear - rec[2].view[2]) / (rec[1].view[2] - rec[2].view[2]);
            // The records: rec[0], rec[1], then the point towards rec[0] (vf8), towards rec[1] (vf9).
            rec[3] = NearPoint(&rec[2], &rec[1], t1, 0, normals);
            rec[2] = NearPoint(&rec[2], &rec[0], t0, 0, normals);
            EmitPolygons(&o, rec, 4, kNearQuad, 2, src.primTableEE + off[3]);
            break;
        case NEAR_ONE_IN:
            // rec[0] in front: the points on its edges to rec[1] and rec[2].
            t0 = (rec[0].view[2] - zNear) / (rec[0].view[2] - rec[1].view[2]);
            t1 = (rec[0].view[2] - zNear) / (rec[0].view[2] - rec[2].view[2]);
            rec[1] = NearPoint(&rec[0], &rec[1], t0, 1, normals);
            rec[2] = NearPoint(&rec[0], &rec[2], t1, 1, normals);
            EmitPolygons(&o, rec, 3, kOneTriangle, 1, src.primTableEE + off[3]);
            break;
        }
    }
    spr->divRedrawVertexEE = o.vertexEE;
    if (o.primEE == GA_DIV_REDRAW_INDEX_WORK_EE) {
        spr->divRedrawVertexEE = GA_DIV_REDRAW_VERTEX_WORK_EE;
        spr->divRedrawIndexEE = GA_DIV_REDRAW_INDEX_WORK_EE;
        return;
    }
    end = o.primEE;
    if (flags & GA_OBJ_FLAG_DOUBLE_SIDED) {
        spr->divRedrawIndexEE = end;
        return;
    }
    {
        GA_REDRAW_PRIM *sentinel = RedrawPrimPtr(end);
        u_int accept = VU_MAC_SX ^ ((flags & GA_OBJ_FLAG_FLIP) << 7);

        if (sentinel) {
            sentinel->vertexEE[0] = sentinel->vertexEE[1] = sentinel->vertexEE[2] = GA_REDRAW_SENTINEL;
            sentinel->primTableEE = GA_REDRAW_SENTINEL;
        }
        // PORT: the ROM pipelines two primitives through VU0 and stops before testing the sentinel; the
        // host tests the primitives in order, compacting the kept ones to the buffer's start.
        keep = GA_DIV_REDRAW_INDEX_WORK_EE;
        for (cur = GA_DIV_REDRAW_INDEX_WORK_EE; cur != end; cur += sizeof(GA_REDRAW_PRIM)) {
            GA_REDRAW_PRIM *p = RedrawPrimPtr(cur);
            const GA_REDRAW_VERTEX *a, *b, *c;

            if (!p)
                break;
            a = RedrawVertexPtr(p->vertexEE[0]);
            b = RedrawVertexPtr(p->vertexEE[1]);
            c = RedrawVertexPtr(p->vertexEE[2]);
            if (!a || !b || !c)
                continue;
            if ((FacingDot(a->view, b->view, c->view) < 0.0f ? VU_MAC_SX : 0u) != accept)
                continue;
            *RedrawPrimPtr(keep) = *p;
            keep += sizeof(GA_REDRAW_PRIM);
        }
    }
    spr->divRedrawIndexEE = keep;
}
