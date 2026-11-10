// ga/gaInitDrawObject.c -- Clock Tower 3, the ga library's draw set-up (0x018a1a48..0x018a3078): the
// scratchpad work area the VU1 renderer reads, the GS register packets and the BGT ring's start, sync and
// end. Object extent from the symtab: a .text section symbol at 0x018a1a48 (a GCC-built object, like
// gaVifInit's before it), up to gaObjectClippingCheck's object at 0x018a3078.
//
// Slice 1 (2026-11-09): every function but gaSetupTBufferData, gaBGT_SetLoadGSEnv and
// gaBGT_LoadGSTexClutData_VIF1 (no caller a smoke run reaches yet).
//
// Host: there is no VIF, VU or GS. The packets are built byte for byte as the ROM builds them and still
// go through the BGT DMA wrapper; the draws they would order are made natively by the host draw modules.
// The scratchpad (sdk/eekernel.h) keeps its EE layout, address words included (EE addresses, u_int).

#include "ga/gaInitDrawObject.h"

#include "ga/gaDma.h"
#include "ga/gaDmaSync_VIF1.h"
#include "ga/gaVifInit.h"
#include "game/unassigned_data.h"
#include "sdk/eekernel.h"
#include "sdk/eeregs.h"
#include "sdk/libgraph.h"

#include <string.h>

// PORT: the ROM's .vutext microprogram (VU1 MPG code). The host has no VU, so this is a zeroed stand-in
// of the ROM's size: gaInitDrawObject still sends it, and the host DMAC drops VIF1 transfers.
alignas(16) u_int vu1_object_draw_program[VU1_OBJECT_DRAW_PROGRAM_QWC * 4];

#define SPR_WORK ((GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET))
#define SPR_EE(offset) (SCRATCHPAD_EE_BASE + (u_int)(offset))

#define EE_UNCACHED_ACCEL 0x20000000u                         /* kseg: uncached-accelerated mirror */
#define GA_DIV_REDRAW_GSPACKET_WORK_EE 0x01957e40u            /* bss, 0xb0000 bytes (its owner: unread) */
#define VIF_UNPACK_V4_32 0x0c                                  /* UNPACK vn 3, vl 0 */
#define VU1_UNPACK_ADDR 0x100
#define VU1_UNPACK_QWC 11
#define COLOUR_MAX 255.0f

// GIFtags of the VU1 renderer's triangle strips (NLOOP filled per draw): PRE, PRIM TRIANGLE, PACKED, EOP.
#define GIFTAG_TRI_9 0x9001c00000008000ull         /* NREG 9 */
#define GIFTAG_TRI_9_REGS 0x412412412ull           /* (ST RGBAQ XYZF2) x 3 */
#define GIFTAG_TRI_12 0xc001c00000008000ull        /* NREG 12 */
#define GIFTAG_TRI_12_REGS 0x412412412eeeull       /* 3 NOPs, then (ST RGBAQ XYZF2) x 3 */

// The packets' sizes: qwords after the DMAtag (DIRECT sends one fewer: the VIFcode qword is not GS data).
#define GSINIT_QWC 6
#define DBUFFER_QWC 7
#define DRAWCTRL_QWC 8
#define FOGCTRL_QWC 3
#define TBUFFER_SEND_QWC 8                 /* gaBGT_ExecTBufferData_VIF1 sends 8 qwords */
#define MARK_PACKET_QWC 2
#define SYNC_PACKET_QWC 1

// GS register field units the ROM converts to.
#define GS_PAGE_SHIFT 11                   /* FBP / ZBP: byte address >> 11 (2048-word pages, as the ROM counts) */
#define GS_FBW_SHIFT 6                     /* FBW: 64-pixel units */
#define GS_FBP_FIELD 0xffffull             /* gaBGT_ExecDBufferData_VIF1 clears the low 16 bits */
#define GS_XYOFFSET_CENTRE 0x8000ull       /* 2048.0 in 12.4 */

// VIF1_STAT bits gaBGT_Status waits on: FQC, VPS, VEW, VGW, ER0 / ER1.
#define VIF1_STAT_BGT_BUSY 0x01f0300fu

static void SetPacketHead(GA_PACKET_HEAD *head, int qwc, int kind, int pairs)
{
    head->dmatag = GA_DMATAG_CNT(qwc);
    head->mark = GA_PACKET_MARK(kind);
    head->vifcode[0] = VC_NOP;
    head->vifcode[1] = VC_NOP;
    head->vifcode[2] = VC_NOP;
    head->vifcode[3] = GA_VIFCODE_DIRECT(qwc - 1);
    head->giftag = GA_GIFTAG_AD(pairs);
    head->giftagRegs = GA_GIFTAG_REGS_AD;
}

static void SetVifQword(u_int *qw, u_int w0, u_int w1, u_int w2, u_int w3)
{
    qw[0] = w0;
    qw[1] = w1;
    qw[2] = w2;
    qw[3] = w3;
}

// CT3 0x018a1a48: load the VU1 microprogram and lay out the scratchpad's constants and VIFcode templates.
void gaInitDrawObject(void)
{
    GA_SPR_WORK *spr = SPR_WORK;

    gaDmaSendN_VIF1(vu1_object_draw_program, VU1_OBJECT_DRAW_PROGRAM_QWC);
    gaDmaSync_VIF1();
    SetVifQword(spr->vifMscal6, VC_NOP, VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0006));
    SetVifQword(spr->vifFlushMscntFlushe, VC_FLUSH, VC_MSCNT, VC_FLUSHE, VC_NOP);
    SetVifQword(spr->vifFlusheMscntFlushe, VC_FLUSHE, VC_MSCNT, VC_FLUSHE, VC_NOP);
    SetVifQword(spr->vifFlush, VC_FLUSH, VC_NOP, VC_NOP, VC_NOP);
    SetVifQword(spr->vifMscntFlushe, VC_MSCNT, VC_FLUSHE, VC_NOP, VC_NOP);
    SetVifQword(spr->vifMscal8, VC_NOP, VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_0008));
    SetVifQword(spr->vifMscnt, VC_NOP, VC_NOP, VC_NOP, VC_MSCNT);
    SetVifQword(spr->vifMscalA, VC_NOP, VC_NOP, VC_NOP, VC_MSCAL(VU_MPG_ENTRY_000A));
    spr->zero040[0] = 0;
    spr->zero040[1] = 0;
    spr->half04c = 0.5f;
    SetVifQword(spr->unpackHead, VC_NOP, VC_NOP, VC_NOP,
                VC(SCE_VIF1_UNPACK | VIF_UNPACK_V4_32, VU1_UNPACK_QWC, VU1_UNPACK_ADDR));
    spr->giftagTriNop[1] = GIFTAG_TRI_12_REGS;
    spr->half23c = 0.5f;
    spr->giftagTri[0] = GIFTAG_TRI_9;
    spr->giftagTri[1] = GIFTAG_TRI_9_REGS;
    spr->giftagTriNop[0] = GIFTAG_TRI_12;
    spr->minusHalf230 = -0.5f;
    spr->divRedrawPacketEE = GA_DIV_REDRAW_GSPACKET_WORK_EE | EE_UNCACHED_ACCEL;
    spr->half234 = 0.5f;
}

// CT3 0x018a1c40: derive the renderer's constants from the 3dlib work (the scratchpad copy main passes).
// The ROM keeps the work's address at +0xec; on the host that word holds the copy's EE address.
void gaInit3dlibWork(float *work)
{
    GA_SPR_WORK *spr = SPR_WORK;

    spr->work3dlibEE = SPR_EE((u_char *)work - g_eeScratchpad);
    spr->viewScaleX = work[2] / work[0];
    spr->viewScaleY = work[3] / work[1];
    spr->zScale = 1.0f / (work[11] - work[10]);
    spr->work6 = work[6];
    spr->zOffset = ((work[11] + work[10]) * -0.5f) / (work[11] - work[10]);
    spr->work21 = work[21];
    spr->work22 = work[22];
    spr->zero06c = 0;
    spr->work23x16 = work[23] * 16.0f;
    spr->work26 = work[26];
    spr->work27 = work[27];
    spr->work6b = work[6];
    spr->work9 = work[9];
    spr->colourMax = COLOUR_MAX;
    spr->work25 = work[25];
    spr->work24 = work[24];
}

// CT3 0x018a1d58: the GS's fixed state -- colours clamp, PRMODE drives the primitive attributes, no scan
// mask, no per-pixel alpha blend.
void gaSetupGSInitData(GA_GSINIT_DATA *gsinit)
{
    SetPacketHead(&gsinit->head, GSINIT_QWC, GA_PACKET_GSINIT, 4);
    gsinit->ad[0].data = 1;
    gsinit->ad[0].addr = SCE_GS_COLCLAMP;
    gsinit->ad[1].data = 0;
    gsinit->ad[1].addr = SCE_GS_PRMODECONT;
    gsinit->ad[2].data = 0;
    gsinit->ad[2].addr = SCE_GS_SCANMSK;
    gsinit->ad[3].data = 0;
    gsinit->ad[3].addr = SCE_GS_PABE;
}

// CT3 0x018a1de0
void gaBGT_ExecGSInitData_VIF1(GA_GSINIT_DATA *gsinit)
{
    gaBGT_DmaSendN_fromSPR_VIF1(gsinit, GSINIT_QWC + 1);
}

// CT3 0x018a1df8: a draw buffer -- frame and Z buffers (byte addresses; FBP / ZBP are rewritten from the
// current buffers when the packet is sent), the scissor box, the primitive offset and dithering. The ROM
// adds the fields without masking; the offset is written as 0x8000 - offset (2048.0 - offset / 16 in 12.4).
void gaSetupDBufferData(GA_DBUFFER_DATA *dbuf, u_int fbAddr, u_int fbWidth, u_int fbPsm, u_int fbMsk,
                        u_int zbAddr, u_int zPsm, u_int zMsk, u_int scissorX0, u_int scissorY0,
                        u_int scissorX1, u_int scissorY1, u_int offsetX, u_int offsetY, u_int dthe)
{
    SetPacketHead(&dbuf->head, DBUFFER_QWC, GA_PACKET_DBUFFER, 5);
    dbuf->frame.data = (u_long)(fbAddr >> GS_PAGE_SHIFT) + ((u_long)fbMsk << 32) + ((u_long)fbPsm << 24) +
                       ((u_long)(fbWidth >> GS_FBW_SHIFT) << 16);
    dbuf->frame.addr = SCE_GS_FRAME_1;
    dbuf->zbuf.data = (u_long)(zbAddr >> GS_PAGE_SHIFT) + ((u_long)zPsm << 24) + ((u_long)zMsk << 32);
    dbuf->zbuf.addr = SCE_GS_ZBUF_1;
    dbuf->scissor.data = (u_long)scissorX0 + ((u_long)scissorY1 << 48) + ((u_long)scissorY0 << 32) +
                         ((u_long)scissorX1 << 16);
    dbuf->scissor.addr = SCE_GS_SCISSOR_1;
    dbuf->xyoffset.data = ((GS_XYOFFSET_CENTRE - offsetY) << 32) - offsetX + GS_XYOFFSET_CENTRE;
    dbuf->xyoffset.addr = SCE_GS_XYOFFSET_1;
    dbuf->dthe.data = dthe;
    dbuf->dthe.addr = SCE_GS_DTHE;
}

// CT3 0x018a1f60: point FBP / ZBP at the current draw and Z buffers, then send the packet.
void gaBGT_ExecDBufferData_VIF1(GA_DBUFFER_DATA *dbuf)
{
    dbuf->frame.data = (dbuf->frame.data & ~GS_FBP_FIELD) | (u_long)((u_int)ga_draw_fbuf_address >> GS_PAGE_SHIFT);
    dbuf->zbuf.data = (dbuf->zbuf.data & ~GS_FBP_FIELD) | (u_long)((u_int)ga_zbuf_address >> GS_PAGE_SHIFT);
    gaBGT_DmaSendN_fromSPR_VIF1(dbuf, DBUFFER_QWC + 1);
}

// CT3 0x018a1fd8: a draw control -- the primitive attributes, the alpha / depth tests, the blend, FBA,
// texture filtering and TEXA (each field added at its GS shift).
void gaSetupDrawCtrlData(GA_DRAWCTRL_DATA *ctrl, GA_DRAWCTRL_ARGS args)
{
    SetPacketHead(&ctrl->head, DRAWCTRL_QWC, GA_PACKET_DRAWCTRL, 6);
    ctrl->prmode.data = ((u_long)args.iip << 3) + ((u_long)args.fix << 10) + ((u_long)args.abe << 6) +
                        ((u_long)args.fst << 8) + ((u_long)args.tme << 4) + ((u_long)args.fge << 5) +
                        ((u_long)args.aa1 << 7) + ((u_long)args.ctxt << 9);
    ctrl->prmode.addr = SCE_GS_PRMODE;
    ctrl->test.data = (u_long)args.ate + ((u_long)args.ztst << 17) + ((u_long)args.afail << 12) +
                      ((u_long)args.datm << 15) + ((u_long)args.atst << 1) + ((u_long)args.aref << 4) +
                      ((u_long)args.date << 14) + ((u_long)args.zte << 16);
    ctrl->test.addr = SCE_GS_TEST_1;
    ctrl->alpha.data = (u_long)args.alphaA + ((u_long)args.alphaFix << 32) + ((u_long)args.alphaC << 4) +
                       ((u_long)args.alphaB << 2) + ((u_long)args.alphaD << 6);
    ctrl->alpha.addr = SCE_GS_ALPHA_1;
    ctrl->fba.data = args.fba;
    ctrl->fba.addr = SCE_GS_FBA_1;
    ctrl->tex1.data = (u_long)args.lcm + ((u_long)args.k << 32) + ((u_long)args.mmin << 6) +
                      ((u_long)args.mmag << 5) + ((u_long)args.mxl << 2) + ((u_long)args.tex1Bit15 << 15);
    ctrl->tex1.addr = SCE_GS_TEX1_1;
    ctrl->texa.data = args.texa;
    ctrl->texa.addr = SCE_GS_TEXA;
}

// CT3 0x018a22b0
void gaBGT_ExecDrawCtrlData_VIF1(GA_DRAWCTRL_DATA *ctrl)
{
    gaBGT_DmaSendN_fromSPR_VIF1(ctrl, DRAWCTRL_QWC + 1);
}

// CT3 0x018a22c8: a fog packet -- FOGCOL (r, g, b added at their bytes, unmasked) and, kept after the
// sent qwords, the three fog floats.
void gaSetupFogCtrlData(GA_FOGCTRL_DATA *fog, float f0, float f1, float f2, int r, int g, int b)
{
    fog->fog[0] = f0;
    fog->fog[1] = f1;
    fog->fog[2] = f2;
    SetPacketHead(&fog->head, FOGCTRL_QWC, GA_PACKET_FOGCTRL, 1);
    fog->fogcol.data = (u_long)(u_int)r + ((u_long)(u_int)g << 8) + ((u_long)(u_int)b << 16);
    fog->fogcol.addr = SCE_GS_FOGCOL;
}

// CT3 0x018a2358
void gaBGT_ExecFogCtrlData_VIF1(GA_FOGCTRL_DATA *fog)
{
    gaBGT_DmaSendN_fromSPR_VIF1(fog, FOGCTRL_QWC + 1);
}

// CT3 0x018a2960 (no caller in the ROM)
void gaBGT_ExecTBufferData_VIF1(void *tbuf)
{
    gaBGT_DmaSendN_fromSPR_VIF1(tbuf, TBUFFER_SEND_QWC);
}

// CT3 0x018a2e70: a draw command packet's length is its DMAtag's qwc.
void gaBGT_ExecDrawCommandData_VIF1(GA_DRAWCOMMAND_DATA *com)
{
    gaBGT_DmaSendN_fromSPR_VIF1(com, com->qwc + 1);
}

// CT3 0x018a2e90: queue a VIF MARK in the BGT ring (no caller in the ROM).
void gaBGT_PutMark_VIF1(u_int mark)
{
    GA_PACKET_HEAD *packet = (GA_PACKET_HEAD *)SCRATCHPAD(GA_SPR_MARK_PACKET);

    gaDmaSync_fromSPR();
    packet->vifcode[3] = GA_VIFCODE_MARK(mark);
    packet->dmatag = GA_DMATAG_CNT(1);
    packet->mark = GA_PACKET_MARK(GA_PACKET_GSINIT);
    packet->vifcode[0] = VC_NOP;
    packet->vifcode[1] = VC_NOP;
    packet->vifcode[2] = VC_NOP;
    gaBGT_DmaSendN_fromSPR_VIF1(packet, MARK_PACKET_QWC);
}

// CT3 0x018a2f10 (no caller in the ROM)
void gaPutMark_VIF1(u_int mark)
{
    REG_VIF1_MARK = mark;
}

// CT3 0x018a2f20 (no caller in the ROM)
u_int gaGetMark_VIF1(void)
{
    return REG_VIF1_MARK;
}

// CT3 0x018a2f30: open the BGT ring (VIF1 fed from the MFIFO) and point the packet area at the scratchpad.
void gaBGT_Start(void *ringbuffer, int size)
{
    SPR_WORK->bgtPacketEE = SPR_EE(GA_SPR_BGT_PACKET);
    gaDmaOpen_MFIFO_VIF1(ringbuffer, size);
}

// CT3 0x018a2f58: 1 while VIF1 works or the ring still holds packets (fromSPR's MADR, the ring's write
// end, has not met VIF1's TADR, its read end).
int gaBGT_Status(void)
{
    u_int stat = REG_VIF1_STAT;
    u_int head = REG_DMAC_8_SPR_FROM_MADR;
    u_int tail = REG_DMAC_1_VIF1_TADR;
    int busy = 1;

    if ((stat & VIF1_STAT_BGT_BUSY) == 0) {
        busy = 0;
        if (head != tail)
            busy = 1;
    }
    return busy;
}

// CT3 0x018a2fa8: push an empty REF tag through the ring, then wait until VIF1 has drained it.
void gaBGT_Sync(void)
{
    GA_PACKET_HEAD *packet = (GA_PACKET_HEAD *)SCRATCHPAD(GA_SPR_MARK_PACKET);

    gaDmaSync_fromSPR();
    packet->dmatag = GA_DMATAG_REF_EMPTY;
    packet->mark = 0;
    gaBGT_DmaSendN_fromSPR_VIF1(packet, SYNC_PACKET_QWC);
    gaDmaSync_fromSPR();
    while (gaBGT_Status() != 0)
        ;
}

// CT3 0x018a3000
void gaBGT_End(void)
{
    gaBGT_Sync();
    gaDmaClose_MFIFO_VIF1();
}

// CT3 0x018a3020: an ST register value from two floats (no caller in the ROM).
u_long DR_ST(float s, float t)
{
    u_int sBits;
    u_int tBits;

    memcpy(&sBits, &s, sizeof sBits);
    memcpy(&tBits, &t, sizeof tBits);
    return (u_long)sBits | ((u_long)tBits << 32);
}

// CT3 0x018a3058: Q's bits (no caller in the ROM).
u_long DR_Q(float q)
{
    u_int qBits;

    memcpy(&qBits, &q, sizeof qBits);
    return qBits;
}
