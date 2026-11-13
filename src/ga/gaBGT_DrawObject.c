// ga/gaBGT_DrawObject.c -- Clock Tower 3, the ga draw object's send to the VU1 renderer (CT3
// 0x018a3148..0x018a4fd8, an asm object: a .text section symbol starts it, tumap.py).
//
// Slice 1 (2026-11-10): the no-clip path, read from the disassembly (mipsdis.py: the decompiler turns these
// hand-written functions into stores through misread registers). gaBGT_DrawObject_NoClippingCheck2 keeps
// the object's counts in the scratchpad (t9 = 0x70000000, GA_SPR_WORK), calls the set-up its light kind
// picks, then copies the packet the set-up built (at GA_SPR_WORK.bgtPacketEE, up to the end word at
// scratchpad 0x104) to the BGT ring through D8 (SPR -> memory), waiting for room behind VIF1's TADR, and
// flips the packet area. The set-up is a chain of: a CNT of 15 qwords (the header: UNPACK V4-32 14 qwords to
// VU1 0x1a0 -- the 3dlib constants at 0x60..0x90 with the counts, the object's position, light1's light
// matrix, the object's matrix, the GS register values), then REF tags to the object's own blocks (colours,
// positions, ST, normals for light1, the light work for light1 as a CNT of 17 qwords), MSCAL of the VU1
// program, the index record (UNPACK V4-16 to 0x17f), an optional DIRECT upload (the attr argument), and a
// REF to the scratchpad's FLUSH / MSCNT / FLUSHE.
//
// Host: no VIF, no VU1. The set-ups keep their scratchpad stores (the 3dlib header the renderer would read)
// and hand the same blocks, with that header, to the native draw module (alyssapan_drawga.h), which runs the
// VU1 program's maths (slice 2: rendering/ga_mesh.cpp, architecture.md). The D8 copy has nothing to copy;
// the packet area still flips as the ROM's does.
//
// Dead, written for the record (2026-11-10): gaBGT_DrawObject_nc2_SetupDma_light2 (0x018a4c78) -- the call
// at 0x018a3f44 is never branched to (any light kind other than 0 goes to light1), and its code is light1's.
//
// Slice 3 (2026-11-10): the clipped path (gaBGT_DrawObject) and the shared local-view path
// (gaBGT_DrawObject_NoClippingCheck, which NoClippingCheck2's billboard objects jump into at 0x018a3984),
// with their set-ups SetupDma_light0 / 1 / 2, from the disassembly. There the EE puts the vertices in the
// local view (gaDrawObject.c, VU0), remakes the index record with the clipping flags, and the packet's
// vertex block holds the result; polygons too big or too near for the GS are divided and redrawn
// (gaDrawObject_SetBig / SetNear, gaDrawObject_Redraw_Polygon). The set-ups build the packet in the
// scratchpad as the ROM does; the host draw module gets the object read in place, as the nc2 set-ups give
// it (PORT: the GPU clips, so the whole object goes; the EE's division is the GS's workaround).

#include "common/alyssapan_stub.h"
#include "ga/gaBGT_DrawObject.h"
#include "ga/gaDmaSync_VIF1.h"
#include "ga/gaDrawObject.h"
#include "ga/gaDrawObject_Redraw_Polygon.h"
#include "ga/gaInitDrawObject.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "sdk/eekernel.h"
#include "alyssapan/rendering/alyssapan_drawga.h"

#include <string.h>

#define SPR_WORK ((GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET))

// The GIFtag at GA_SPR_WORK 0x90: the set-ups put NLOOP = the primitive count, with EOP.
#define GIFTAG_EOP 0x8000

// PORT: a relocated word of the object's bank as a host pointer.
static const void *ObjEePtr(int bank, const u_char *obj, int offset)
{
    return BankEePtr(bank, *(const u_int *)(obj + offset));
}

// The stores both nc2 set-ups make into the 3dlib header before building the chain.
static void SetupHeader(GA_SPR_WORK *spr, const u_char *obj)
{
    spr->objReg70 = *(const u_long *)(obj + GA_OBJ_REG70);
    spr->vertexSkip = 0;
    spr->vertexCount = spr->drawVertexCount;
    spr->primCount = spr->drawPrimCount;
    *(u_short *)&spr->giftagTri[0] = (u_short)(spr->drawPrimCount | GIFTAG_EOP);
    spr->objReg78 = *(const u_long *)(obj + GA_OBJ_REG78);
}

// What both set-ups send, read in place.
static void FillObject(AlyssaPanGaObject *o, const GA_SPR_WORK *spr, const u_char *obj, int attr)
{
    int bank = BankOfHostPtr(obj);

    memset(o, 0, sizeof(*o));
    o->flags = obj[GA_OBJ_FLAGS];
    o->vertexCount = (int)spr->drawVertexCount;
    o->primCount = (int)spr->drawPrimCount;
    o->positions = (const float *)ObjEePtr(bank, obj, GA_OBJ_POSITIONS);
    o->stBlock = ObjEePtr(bank, obj, GA_OBJ_ST);
    o->colourBlock = ObjEePtr(bank, obj, GA_OBJ_COLOURS);
    o->indices = (const uint16_t *)ObjEePtr(bank, obj, GA_OBJ_INDICES);
    o->position = (const float *)ObjEePtr(bank, obj, GA_OBJ_POSITION);
    o->matrix = (const float *)ObjEePtr(bank, obj, GA_OBJ_MATRIX);
    o->gsReg70 = spr->objReg70;
    o->gsReg78 = spr->objReg78;
    // PORT: the ROM reads attr as an address (0 = no upload); main_rtn and its kin always pass 0.
    o->upload = (const void *)(uintptr_t)(u_int)attr;
    // The 14-qword header's 3dlib part (scratchpad 0x60..0xd0): the VU1 program's constants.
    o->vuHeader = &spr->work21;
}

// CT3 0x018a41ac
static void gaBGT_DrawObject_nc2_SetupDma_light0(const u_char *obj, int attr)
{
    GA_SPR_WORK *spr = SPR_WORK;
    AlyssaPanGaObject o;

    SetupHeader(spr, obj);
    FillObject(&o, spr, obj, attr);
    o.light = ALYSSAPAN_GA_LIGHT0;
    AlyssaPan_DrawGaObject(&o);
}

// CT3 0x018a4690
static void gaBGT_DrawObject_nc2_SetupDma_light1(const u_char *obj, int attr)
{
    GA_SPR_WORK *spr = SPR_WORK;
    AlyssaPanGaObject o;
    int bank;

    spr->objLight = *(const float *)(obj + GA_OBJ_LIGHT_SCALE);
    SetupHeader(spr, obj);
    FillObject(&o, spr, obj, attr);
    bank = BankOfHostPtr(obj);
    o.light = ALYSSAPAN_GA_LIGHT1;
    o.normalBlock = ObjEePtr(bank, obj, GA_OBJ_NORMALS);
    o.lightMatrix = (const float *)ObjEePtr(bank, obj, GA_OBJ_LIGHT_MATRIX);
    o.lightWork = ObjEePtr(bank, obj, GA_OBJ_LIGHT_WORK);
    o.lightScale = spr->objLight;
    AlyssaPan_DrawGaObject(&o);
}

// CT3 0x018a4c78
// callcheck: gaDMAStopMessage_VIF1 (not this function's: the stray j at 0x018a4fd8 after its jr ra)
// Dead in the ROM: its one call (NoClippingCheck2, 0x018a3f44) is never branched to. Its 0x35c bytes of
// code are nc2_SetupDma_light1's, instruction for instruction (mipsdis.py diff: only the branch targets
// move); the MSCAL it writes comes from the object's flags, not from the function, so light2 objects run the
// same VU1 program choice as light1's here. Its last two words (0x018a4fd4: nop, j gaDMAStopMessage_VIF1)
// follow the final jr ra and nothing reaches them -- the next object's stray tail, as at AlignedNum.c's head.
// Kept for the record; NoClippingCheck2 names it where the ROM's dead call sits.
static void gaBGT_DrawObject_nc2_SetupDma_light2(const u_char *obj, int attr)
{
    gaBGT_DrawObject_nc2_SetupDma_light1(obj, attr);
}

// The tail every path shares: the packet went to the ring (D8; nothing to copy on the host), the packet
// area flips.
static void FlipPacketArea(GA_SPR_WORK *spr)
{
    spr->bgtPacketEE = spr->bgtPacketEE == SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET
                           ? SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET_B
                           : SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET;
}

// ---- the local-view set-ups (slice 3) ----

// DMAtag / VIF code words the set-ups write.
#define DMA_CNT 0x10000000u                     /* DMAtag id 1 (CNT); | qwc */
#define DMA_REF 0x30000000u                     /* DMAtag id 3 (REF); | qwc */
#define DMA_ADDR_SPR 0x80000000u                /* a DMAtag address in the scratchpad */
#define VIF_FLUSH_CODE 0x11000000u
#define VIF_MSCAL_CODE(addr) (0x14000000u | (addr))
#define VIF_MSCNT_CODE 0x17000000u
#define VIF_UNPACK_V4_32_CODE(num, addr) (0x6c000000u | ((u_int)(num) << 16) | (addr))

#define GA_VU1_HEADER_ADDR 0x1a0                /* where the header qwords go in VU1 memory */
#define GA_CLIP_HEADER_QWC 0x0b                 /* the clipped header: the VIF qword + 10 qwords */
#define GA_CLIP_HEADER_UNPACK 10
#define GA_LIGHT_WORK_QWC 0x11                  /* GA_LIGHT_WORK: 17 qwords */

// Where each block sits in the packet (GA_SPR_WORK.bgtPacketEE): the header is 0xc0 bytes; light0 puts its
// colours' REF after it, light1 / light2 send theirs after the vertices.
#define GA_PKT_VERTEX_LIGHT0 0xd0
#define GA_PKT_VERTEX_LIGHT1 0xc0
#define GA_PKT_COLOUR_LIGHT0 0xc0
// The index block after the vertex block's end (stPacketEE): light0 one REF (the ST), light1 / light2 the
// normals, MSCAL, colours, the light work (CNT + 17 qwords), FLUSHE MSCNT FLUSHE and the ST.
#define GA_PKT_INDEX_LIGHT0 0x10
#define GA_PKT_INDEX_LIGHT1 0x170

// A scratchpad EE address as a host pointer.
static u_char *SprPtr(u_int ee)
{
    return (u_char *)SCRATCHPAD(ee - SCRATCHPAD_EE_BASE);
}

// A DMAtag's two low words.
static void PutTag(u_char *p, u_int idQwc, u_int addr)
{
    ((u_int *)p)[0] = idQwc;
    ((u_int *)p)[1] = addr;
}

// The qword counts of the object's blocks behind their VIF head qword.
static u_int ColourQwc(u_int n) { return ((n + 3) >> 2) + 1; }  /* RGBA bytes, 4 to a qword */
static u_int StQwc(u_int n) { return ((n + 1) >> 1) + 1; }      /* ST pairs, 2 to a qword */

// The header stores the local-view set-ups share, and the header block at the packet's start: CNT, the
// VIF qword (UNPACK V4-32 10 qwords to VU1 0x1a0), the 3dlib constants 0x60..0x90, the object's position,
// [light1 / light2: the light matrix], the scratchpad's 0xb0 and 0xc0.
static u_char *SetupClipHeader(GA_SPR_WORK *spr, const u_char *obj, int lit)
{
    u_char *pkt = SprPtr(spr->bgtPacketEE);
    int bank = BankOfHostPtr(obj);
    u_int n = *(const u_int *)(obj + GA_OBJ_VERTEX_COUNT);

    if (lit)
        spr->objLight = *(const float *)(obj + GA_OBJ_LIGHT_SCALE);
    spr->objReg70 = *(const u_long *)(obj + GA_OBJ_REG70);
    spr->vertexSkip = n - spr->drawVertexCount;
    spr->vertexCount = n;
    spr->primCount = spr->drawPrimCount;
    *(u_short *)&spr->giftagTri[0] = (u_short)(spr->drawPrimCount | GIFTAG_EOP);
    spr->objReg78 = *(const u_long *)(obj + GA_OBJ_REG78);

    *(u_long *)(pkt + 0x08) = 0;
    *(u_long *)(pkt + 0x00) = DMA_CNT | GA_CLIP_HEADER_QWC;
    memset(pkt + 0x10, 0, 0x10);
    ((u_int *)(pkt + 0x10))[3] = VIF_UNPACK_V4_32_CODE(GA_CLIP_HEADER_UNPACK, GA_VU1_HEADER_ADDR);
    memcpy(pkt + 0x20, &spr->work21, 0x40);                       /* scratchpad 0x60..0x90 */
    memcpy(pkt + 0x60, ObjEePtr(bank, obj, GA_OBJ_POSITION), 0x10);
    if (lit)
        memcpy(pkt + 0x70, ObjEePtr(bank, obj, GA_OBJ_LIGHT_MATRIX), 0x30);
    memcpy(pkt + 0xa0, &spr->work9, 0x10);                         /* scratchpad 0xb0 */
    memcpy(pkt + 0xb0, &spr->objReg78, 0x10);                      /* scratchpad 0xc0 */
    return pkt;
}

// The tail both set-ups end with at tailPacketEE: the optional GS upload (attr: a halfword qwc, then the
// data, sent as a REF behind a FLUSH), then a REF to the scratchpad's FLUSH MSCNT FLUSHE; the packet's end
// goes to packetEndEE.
static void SetupClipTail(GA_SPR_WORK *spr)
{
    u_char *t = SprPtr(spr->tailPacketEE);
    u_int attr = spr->drawAttr;

    if (attr != 0) {
        u_int addr = attr + 0x10;
        // PORT: the host reaches only a scratchpad upload; main_rtn and its kin pass 0.
        u_int qwc = (int)(attr - SCRATCHPAD_EE_BASE) >= 0 ? *(const u_short *)SprPtr(attr) : 0;

        memset(t + 0x10, 0, 0x10);
        ((u_int *)t)[0] = DMA_CNT | 1;
        ((u_int *)t)[4] = VIF_FLUSH_CODE;
        if ((int)(attr - SCRATCHPAD_EE_BASE) >= 0)
            addr += DMA_ADDR_SPR - SCRATCHPAD_EE_BASE;
        PutTag(t + 0x20, DMA_REF | qwc, addr);
        PutTag(t + 0x30, DMA_REF | 1, DMA_ADDR_SPR | offsetof(GA_SPR_WORK, vifFlushMscntFlushe));
        spr->packetEndEE = spr->tailPacketEE + 0x40;
    } else {
        PutTag(t, DMA_REF | 1, DMA_ADDR_SPR | offsetof(GA_SPR_WORK, vifFlushMscntFlushe));
        spr->packetEndEE = spr->tailPacketEE + 0x10;
    }
}

// The host hand-off of a local-view set-up: the object read in place, whole (the GPU clips).
static void DrawClipped(const GA_SPR_WORK *spr, const u_char *obj, int light)
{
    AlyssaPanGaObject o;
    int bank = BankOfHostPtr(obj);

    // PORT: the billboard local view (gaDrawObject_VertexLocalView*_Billboard) is not on the host yet: the
    // module's transform is the full matrix's, which would turn a billboard; none is handed over.
    if (obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_BILLBOARD)
        return;
    FillObject(&o, spr, obj, (int)spr->drawAttr);
    o.vertexCount = (int)*(const u_int *)(obj + GA_OBJ_VERTEX_COUNT);
    o.primCount = (int)*(const u_int *)(obj + GA_OBJ_PRIM_COUNT);
    o.light = light;
    if (light != ALYSSAPAN_GA_LIGHT0) {
        o.normalBlock = ObjEePtr(bank, obj, GA_OBJ_NORMALS);
        o.lightMatrix = (const float *)ObjEePtr(bank, obj, GA_OBJ_LIGHT_MATRIX);
        o.lightWork = ObjEePtr(bank, obj, GA_OBJ_LIGHT_WORK);
        o.lightScale = spr->objLight;
    }
    AlyssaPan_DrawGaObject(&o);
}

// CT3 0x018a400c
static void gaBGT_DrawObject_SetupDma_light0(u_char *obj)
{
    GA_SPR_WORK *spr = SPR_WORK;
    u_char *pkt = SetupClipHeader(spr, obj, 0);
    u_int n = *(const u_int *)(obj + GA_OBJ_VERTEX_COUNT);

    PutTag(pkt + GA_PKT_COLOUR_LIGHT0, DMA_REF | ColourQwc(n), *(const u_int *)(obj + GA_OBJ_COLOURS));
    ((u_int *)(SprPtr(spr->vertexPacketEE) + 0x10))[0] = VIF_MSCAL_CODE(GA_VU1_PROG_CLIP_LIGHT0);
    PutTag(SprPtr(spr->stPacketEE), DMA_REF | StQwc(n), *(const u_int *)(obj + GA_OBJ_ST));
    // The index block's VIF qword, as the ROM writes it.
    ((u_int *)(SprPtr(spr->indexPacketEE) + 0x10))[0] = DMA_CNT;
    ((u_int *)(SprPtr(spr->indexPacketEE) + 0x10))[1] = VIF_MSCNT_CODE;
    SetupClipTail(spr);
    DrawClipped(spr, obj, ALYSSAPAN_GA_LIGHT0);
}

// The body of SetupDma_light1 (mscal = GA_SPR_WORK.vifMscal6) and SetupDma_light2 (vifMscalA): the ROM has
// the same code twice but for that one REF. The MSCAL picks the VU1 program (6: light1's lighting 0x2b9,
// 0xa: light2's 0x3b8), so the host is told which (light).
static void SetupDmaLit(u_char *obj, u_int mscal, int light)
{
    GA_SPR_WORK *spr = SPR_WORK;
    u_char *st;
    u_int n = *(const u_int *)(obj + GA_OBJ_VERTEX_COUNT);
    int bank = BankOfHostPtr(obj);

    SetupClipHeader(spr, obj, 1);
    st = SprPtr(spr->stPacketEE);
    PutTag(st + 0x00, DMA_REF | (n + 1), *(const u_int *)(obj + GA_OBJ_NORMALS));
    PutTag(st + 0x10, DMA_REF | 1, DMA_ADDR_SPR | mscal);
    PutTag(st + 0x20, DMA_REF | ColourQwc(n), *(const u_int *)(obj + GA_OBJ_COLOURS));
    ((u_int *)st)[12] = DMA_CNT | GA_LIGHT_WORK_QWC;                /* +0x30, the low word only */
    memcpy(st + 0x40, ObjEePtr(bank, obj, GA_OBJ_LIGHT_WORK), GA_LIGHT_WORK_QWC * 0x10);
    PutTag(st + 0x150, DMA_REF | 1, DMA_ADDR_SPR | offsetof(GA_SPR_WORK, vifFlusheMscntFlushe));
    PutTag(st + 0x160, DMA_REF | StQwc(n), *(const u_int *)(obj + GA_OBJ_ST));
    ((u_int *)(SprPtr(spr->indexPacketEE) + 0x10))[0] = VIF_MSCNT_CODE;
    SetupClipTail(spr);
    DrawClipped(spr, obj, light);
}

// CT3 0x018a4404
static void gaBGT_DrawObject_SetupDma_light1(u_char *obj)
{
    SetupDmaLit(obj, offsetof(GA_SPR_WORK, vifMscal6), ALYSSAPAN_GA_LIGHT1);
}

// CT3 0x018a49ec
static void gaBGT_DrawObject_SetupDma_light2(u_char *obj)
{
    SetupDmaLit(obj, offsetof(GA_SPR_WORK, vifMscalA), ALYSSAPAN_GA_LIGHT2);
}

// One light kind's local-view chain, as the ROM spells it out per kind: the vertex block's place, the
// local view (billboard or not), the index pass at the index block, then -- with primitives left -- the
// reference table, the set-up, the D8 copy and the flip.
typedef void (*LOCAL_VIEW_FN)(u_char *obj, u_int packetEE);
typedef void (*SETUP_FN)(u_char *obj);

static void LocalViewPass(u_char *obj, u_int vertexOffset, u_int indexOffset, LOCAL_VIEW_FN view,
                          LOCAL_VIEW_FN viewBillboard, LOCAL_VIEW_FN remakeIndex, SETUP_FN refTable, SETUP_FN setup)
{
    GA_SPR_WORK *spr = SPR_WORK;

    spr->vertexPacketEE = spr->bgtPacketEE + vertexOffset;
    if (obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_BILLBOARD)
        viewBillboard(obj, spr->vertexPacketEE);
    else
        view(obj, spr->vertexPacketEE);
    spr->indexPacketEE = spr->stPacketEE + indexOffset;
    remakeIndex(obj, spr->indexPacketEE);
    // PORT: the host draws the whole object (DrawClipped: the GPU clips), so an object whose polygons all
    // went to the big / near division lists (gaDrawObject.c's index passes) still goes; the ROM sends
    // only the kept polygons here and redraws the divided ones (gaDrawObject_Redraw_Polygon).
    if (spr->drawPrimCount == 0 && spr->bigDivIndexEE == GA_BIGDIV_INDEX_WORK_EE &&
        spr->nearDivIndexEE == GA_NEARDIV_INDEX_WORK_EE)
        return;
    refTable(obj);
    setup(obj);
    FlipPacketArea(spr);
}

// The local-view path's body (0x018a3984): NoClippingCheck2 jumps here for a billboard object.
static void NoClipBody(u_char *obj)
{
    switch (obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_LIGHT) {
    case GA_LIGHT_KIND_0:
        LocalViewPass(obj, GA_PKT_VERTEX_LIGHT0, GA_PKT_INDEX_LIGHT0, gaDrawObject_VertexLocalView,
                      gaDrawObject_VertexLocalView_Billboard, gaDrawObject_RemakeIndex,
                      gaDrawObject_MakeVertexReferenceTableNoClip, gaBGT_DrawObject_SetupDma_light0);
        break;
    case GA_LIGHT_KIND_1:
    case GA_LIGHT_KIND_2:
        // light2's own chain (0x018a3c08, SetupDma_light2) is never branched to: both go to light1's.
        LocalViewPass(obj, GA_PKT_VERTEX_LIGHT1, GA_PKT_INDEX_LIGHT1, gaDrawObject_VertexLocalView,
                      gaDrawObject_VertexLocalView_Billboard, gaDrawObject_RemakeIndex,
                      gaDrawObject_MakeVertexReferenceTableNoClip, gaBGT_DrawObject_SetupDma_light1);
        break;
    default:
        // the ROM's break instruction
        ALYSSAPAN_STUB("gaBGT_DrawObject_NoClippingCheck: unknown light kind (the ROM breaks)", 0x018a39ac);
        break;
    }
}

// CT3 0x018a3958
// callcheck: gaBGT_DrawObject_SetupDma_light0, gaBGT_DrawObject_SetupDma_light1, gaBGT_DrawObject_SetupDma_light2, gaDrawObject_MakeVertexReferenceTableNoClip, gaDrawObject_RemakeIndex, gaDrawObject_VertexLocalView, gaDrawObject_VertexLocalView_Billboard
// (made by NoClipBody / LocalViewPass; light2's chain, 0x018a3c08, is dead)
void gaBGT_DrawObject_NoClippingCheck(u_char *obj, int attr)
{
    SPR_WORK->drawAttr = (u_int)attr;
    NoClipBody(obj);
}

// CT3 0x018a3d20
// callcheck: gaBGT_DrawObject_nc2_SetupDma_light2 is the ROM's dead call (0x018a3f44, see the file head).
void gaBGT_DrawObject_NoClippingCheck2(u_char *obj, int attr)
{
    GA_SPR_WORK *spr = SPR_WORK;

    spr->drawAttr = (u_int)attr;
    spr->drawVertexCount = *(u_int *)(obj + GA_OBJ_VERTEX_COUNT);
    spr->drawPrimCount = *(u_int *)(obj + GA_OBJ_PRIM_COUNT);
    if (obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_BILLBOARD) {
        NoClipBody(obj);    /* the ROM jumps to 0x018a3984 */
        return;
    }
    if ((obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_LIGHT) == 0)
        gaBGT_DrawObject_nc2_SetupDma_light0(obj, attr);
    else
        gaBGT_DrawObject_nc2_SetupDma_light1(obj, attr);
    (void)gaBGT_DrawObject_nc2_SetupDma_light2;     /* the ROM's dead call, 0x018a3f44 */
    FlipPacketArea(spr);
}

// The clipped path's end (0x018a31e8, and 0x018a35a8 with the near pass first): polygons the index pass
// moved to the near / big lists are set up, then every divided polygon is redrawn.
static void ClippedEnd(u_char *obj, int nearPass)
{
    GA_SPR_WORK *spr = SPR_WORK;

    if (nearPass && spr->nearDivIndexEE != GA_NEARDIV_INDEX_WORK_EE)
        gaDrawObject_SetNear(obj);
    if (spr->bigDivIndexEE != GA_BIGDIV_INDEX_WORK_EE)
        gaDrawObject_SetBig(obj);
    gaDrawObject_Redraw_Polygon(obj);
}

// CT3 0x018a3150
void gaBGT_DrawObject(u_char *obj, int attr)
{
    GA_SPR_WORK *spr = SPR_WORK;
    LOCAL_VIEW_FN remakeIndex;
    int nearPass;

    spr->drawAttr = (u_int)attr;
    spr->divRedrawVertexEE = GA_DIV_REDRAW_VERTEX_WORK_EE;
    spr->divRedrawIndexEE = GA_DIV_REDRAW_INDEX_WORK_EE;
    spr->nearDivIndexEE = GA_NEARDIV_INDEX_WORK_EE;
    spr->bigDivIndexEE = GA_BIGDIV_INDEX_WORK_EE;
    nearPass = (obj[GA_OBJ_FLAGS2] & GA_OBJ_FLAG2_NEAR) != 0;
    remakeIndex = nearPass ? gaDrawObject_RemakeIndexWithClippingCheck_WithNear
                           : gaDrawObject_RemakeIndexWithClippingCheck;

    switch (obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_LIGHT) {
    case GA_LIGHT_KIND_0:
        LocalViewPass(obj, GA_PKT_VERTEX_LIGHT0, GA_PKT_INDEX_LIGHT0, gaDrawObject_VertexLocalViewWithClippingFlagSet,
                      gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard, remakeIndex,
                      gaDrawObject_MakeVertexReferenceTable, gaBGT_DrawObject_SetupDma_light0);
        break;
    case GA_LIGHT_KIND_1:
        LocalViewPass(obj, GA_PKT_VERTEX_LIGHT1, GA_PKT_INDEX_LIGHT1, gaDrawObject_VertexLocalViewWithClippingFlagSet,
                      gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard, remakeIndex,
                      gaDrawObject_MakeVertexReferenceTable, gaBGT_DrawObject_SetupDma_light1);
        break;
    case GA_LIGHT_KIND_2:
        // The ROM's slip, kept: with the near pass, a light2 object that is not a billboard jumps back into
        // light1's chain after its local view (0x018a385c: j 0x018a3754), so it is set up as light1.
        LocalViewPass(obj, GA_PKT_VERTEX_LIGHT1, GA_PKT_INDEX_LIGHT1, gaDrawObject_VertexLocalViewWithClippingFlagSet,
                      gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard, remakeIndex,
                      gaDrawObject_MakeVertexReferenceTable,
                      nearPass && !(obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_BILLBOARD) ? gaBGT_DrawObject_SetupDma_light1
                                                                              : gaBGT_DrawObject_SetupDma_light2);
        break;
    default:
        // the ROM's break instruction; it then falls into the end
        ALYSSAPAN_STUB("gaBGT_DrawObject: unknown light kind (the ROM breaks)", 0x018a31e4);
        break;
    }
    ClippedEnd(obj, nearPass);
}
