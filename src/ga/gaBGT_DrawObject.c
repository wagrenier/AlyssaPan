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
// and hand the same blocks to the native draw module (alyssapan_drawga.h). The D8 copy has nothing to copy;
// the packet area still flips as the ROM's does.
//
// Read, not reconstructed (dead): gaBGT_DrawObject_nc2_SetupDma_light2 (0x018a4c78) -- the call at
// 0x018a3f44 is never branched to (any light kind other than 0 goes to light1).

#include "common/alyssapan_stub.h"
#include "ga/gaBGT_DrawObject.h"
#include "ga/gaDmaSync_VIF1.h"
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
    spr->zero080 = 0;
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

// The tail every path shares: the packet went to the ring (D8; nothing to copy on the host), the packet
// area flips.
static void FlipPacketArea(GA_SPR_WORK *spr)
{
    spr->bgtPacketEE = spr->bgtPacketEE == SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET
                           ? SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET_B
                           : SCRATCHPAD_EE_BASE + GA_SPR_BGT_PACKET;
}

// CT3 0x018a3d20
void gaBGT_DrawObject_NoClippingCheck2(u_char *obj, int attr)
{
    GA_SPR_WORK *spr = SPR_WORK;

    spr->drawVertexCount = *(u_int *)(obj + GA_OBJ_VERTEX_COUNT);
    spr->drawPrimCount = *(u_int *)(obj + GA_OBJ_PRIM_COUNT);
    if (obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_BILLBOARD) {
        // 0x018a3984: the billboard / clipped vertex path, shared with gaBGT_DrawObject_NoClippingCheck
        // (gaDrawObject_VertexLocalView*, RemakeIndex, MakeVertexReferenceTableNoClip, then the non-nc2
        // set-ups): the next slice.
        ALYSSAPAN_STUB("gaBGT_DrawObject_NoClippingCheck2 (billboard path, 0x018a3984)", 0x018a3984);
        return;
    }
    if ((obj[GA_OBJ_FLAGS] & GA_OBJ_FLAG_LIGHT) == 0)
        gaBGT_DrawObject_nc2_SetupDma_light0(obj, attr);
    else
        gaBGT_DrawObject_nc2_SetupDma_light1(obj, attr);
    FlipPacketArea(spr);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018a3150
void gaBGT_DrawObject(u_char *obj, int attr)
{
    ALYSSAPAN_STUB("gaBGT_DrawObject", 0x018a3150);
}
