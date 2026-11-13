// ga/gaInitDrawObject.c -- Clock Tower 3, the ga library's draw set-up (0x018a1a48..0x018a3078): the
// scratchpad work area the VU1 renderer reads, the GS register packets and the BGT ring's start, sync and
// end. Object extent from the symtab: a .text section symbol at 0x018a1a48 (a GCC-built object, like
// gaVifInit's before it), up to gaObjectClippingCheck's object at 0x018a3078.
//
// Slice 1 (2026-11-09): every function but the texture three; slice 2 (2026-11-09): gaSetupTBufferData,
// gaBGT_SetLoadGSEnv, gaBGT_LoadGSTexClutData_VIF1 (no caller in the ROM: the game uses its sun* copies).
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
#include "alyssapan/rendering/alyssapan_drawga.h"

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
    // PORT: divided by AlyssaPan_GaWiden(): the clip flags' screen (+-0.5) and the redraw cut's guard band
    // reach the wide window's edges instead of 4:3's.
    spr->viewScaleX = work[2] / work[0] / AlyssaPan_GaWiden();
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

// main's double buffer (its boot: ga_disp_fbuf_address 0, ga_draw_fbuf_address 0x38000; gaWaitVSync swaps
// the two). Any other draw buffer is a frame no display shows: the shadow buffer, the effect buffer ..
#define GA_DISPLAY_FBUF_A 0x00000
#define GA_DISPLAY_FBUF_B 0x38000
#define GIFTAG_NLOOP_MASK 0x7fffull

// PORT: no GS takes the packet: its A+D pairs go to the host draw module, which keeps the registers the ga
// objects after it draw under (alyssapan_drawga.h AlyssaPan_DrawGaRegister).
static void HandRegistersToHost(const GA_PACKET_HEAD *head)
{
    const GA_AD *ad = (const GA_AD *)(head + 1);
    u_int n = (u_int)(head->giftag & GIFTAG_NLOOP_MASK);
    u_int i;

    for (i = 0; i < n; i++)
        AlyssaPan_DrawGaRegister((int)(ad[i].addr & 0xff), ad[i].data);
}

// CT3 0x018a1f60: point FBP / ZBP at the current draw and Z buffers, then send the packet.
void gaBGT_ExecDBufferData_VIF1(GA_DBUFFER_DATA *dbuf)
{
    dbuf->frame.data = (dbuf->frame.data & ~GS_FBP_FIELD) | (u_long)((u_int)ga_draw_fbuf_address >> GS_PAGE_SHIFT);
    dbuf->zbuf.data = (dbuf->zbuf.data & ~GS_FBP_FIELD) | (u_long)((u_int)ga_zbuf_address >> GS_PAGE_SHIFT);
    gaBGT_DmaSendN_fromSPR_VIF1(dbuf, DBUFFER_QWC + 1);
    AlyssaPan_DrawGaTarget(ga_draw_fbuf_address == GA_DISPLAY_FBUF_A || ga_draw_fbuf_address == GA_DISPLAY_FBUF_B);
    HandRegistersToHost(&dbuf->head);
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
    HandRegistersToHost(&ctrl->head);
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

// The CLUT block per palette format: PSMT8's 16x16 (0x100 words), every other's 8x2 (0x40 words, the
// ROM's step even though an 8x2 CT32 CLUT is 16 words).
#define CLUT8_WORDS 0x100
#define CLUT8_W 16
#define CLUT8_H 16
#define CLUT4_WORDS 0x40
#define CLUT4_W 8
#define CLUT4_H 2

#define GS_BLOCK_SHIFT 6                   /* TBP / CBP: word address >> 6 */
#define MIP_LEVEL_MIN_WORDS 0x100          /* every mip level takes at least 0x100 words */
#define MIP_TBW_MIN_INDEXED 0x80           /* PSMT8 / PSMT4 levels: buffer width >= 128 pixels */
#define MIP_TBW_MIN 0x40
#define TEX0_TCC_RGBA (1ull << 34)
#define TEX0_CLD_SHIFT 61

static int IsPsmIndexed(u_int psm) /* PSMT8, PSMT4 (the ROM's psm - 0x13 < 2) */
{
    return psm - SCE_GS_PSMT8 < 2;
}

// CT3 0x018a2370: a texture buffer -- TEX0 (level 0 at tbp, CLUT number `clut` of the eight laid from cbp),
// CLAMP, the six mip levels packed after level 0 (MIPTBP1 / MIPTBP2) and TEXFLUSH; then, unsent, what
// gaBGT_LoadGSTexClutData_VIF1 needs. Addresses are GS word addresses. No caller in the ROM.
void gaSetupTBufferData(GA_TBUFFER_DATA *tbuf, const GA_TEX_DESC *tex, u_int tbp, u_int cbp, u_int clut,
                        u_int wms, u_int wmt, u_int minu, u_int maxu, u_int minv, u_int maxv, u_int tfx)
{
    u_int width = tex->width;
    u_int height = tex->height;
    u_int psm = tex->psm;
    u_int clutWords;
    u_int clutW;
    u_int clutH;
    u_int tbw;
    u_int bpp;
    u_int levelTbw[GA_MIP_LEVELS];
    u_int levelTbp[GA_MIP_LEVELS];
    u_int minTbw;
    int level;

    SetPacketHead(&tbuf->head, TBUFFER_SEND_QWC - 1, GA_PACKET_TBUFFER, 5);
    if (psm == SCE_GS_PSMT8) {
        clutWords = CLUT8_WORDS;
        clutW = CLUT8_W;
        clutH = CLUT8_H;
    } else {
        clutWords = CLUT4_WORDS;
        clutW = CLUT4_W;
        clutH = CLUT4_H;
    }

    // Level 0's buffer width in 64-pixel units, at least 1, even for the indexed formats.
    tbw = width >> 6;
    if (tbw == 0)
        tbw = 1;
    if (IsPsmIndexed(psm))
        tbw += tbw & 1;
    tbw <<= 6;
    tbuf->tex0.data = (u_long)(tbp >> GS_BLOCK_SHIFT) +
                      ((u_long)((clutWords * clut + cbp) >> GS_BLOCK_SHIFT) << 37) +
                      ((u_long)tex->tw << 26) + TEX0_TCC_RGBA + ((u_long)psm << 20) +
                      ((u_long)((tbw >> 6) & 0x3ffffff) << 14) + ((u_long)tex->th << 30) +
                      ((u_long)tfx << 35) + ((u_long)(tex->cld != 0) << TEX0_CLD_SHIFT);
    tbuf->tex0.addr = SCE_GS_TEX0_1;
    tbuf->clamp.data = (u_long)wms + ((u_long)maxv << 34) + ((u_long)maxu << 14) + ((u_long)minu << 4) +
                       ((u_long)wmt << 2) + ((u_long)minv << 24);
    tbuf->clamp.addr = SCE_GS_CLAMP_1;

    // Bits per pixel as the ROM sizes the levels (PSMCT16 falls to 4 like PSMT4: a ROM slip, kept).
    if (psm == SCE_GS_PSMCT16)
        bpp = 4;
    else if (psm < SCE_GS_PSMCT16)
        bpp = 32;
    else if (psm == SCE_GS_PSMT8)
        bpp = 8;
    else
        bpp = 4;

    // Each level after the one before it, in words (w * h * bpp / 32), at least 0x100.
    levelTbp[0] = tbp;
    for (level = 1; level < GA_MIP_LEVELS; level++) {
        u_int words = (width >> (level - 1)) * (height >> (level - 1)) * bpp >> 5;

        if (words < MIP_LEVEL_MIN_WORDS)
            words = MIP_LEVEL_MIN_WORDS;
        levelTbp[level] = levelTbp[level - 1] + words;
    }
    minTbw = IsPsmIndexed(psm) ? MIP_TBW_MIN_INDEXED : MIP_TBW_MIN;
    levelTbw[0] = tbw;
    for (level = 1; level < GA_MIP_LEVELS; level++) {
        levelTbw[level] = width >> level;
        if (levelTbw[level] < minTbw)
            levelTbw[level] = minTbw;
    }

    tbuf->miptbp1.data = (u_long)(levelTbp[1] >> 6) + ((u_long)(levelTbw[1] >> 6) << 14) +
                         ((u_long)(levelTbp[2] >> 6) << 20) + ((u_long)(levelTbw[2] >> 6) << 34) +
                         ((u_long)(levelTbp[3] >> 6) << 40) + ((u_long)(levelTbw[3] >> 6) << 54);
    tbuf->miptbp1.addr = SCE_GS_MIPTBP1_1;
    tbuf->miptbp2.data = (u_long)(levelTbp[4] >> 6) + ((u_long)(levelTbw[4] >> 6) << 14) +
                         ((u_long)(levelTbp[5] >> 6) << 20) + ((u_long)(levelTbw[5] >> 6) << 34) +
                         ((u_long)(levelTbp[6] >> 6) << 40) + ((u_long)(levelTbw[6] >> 6) << 54);
    tbuf->miptbp2.addr = SCE_GS_MIPTBP2_1;
    tbuf->texflush.data = 0;
    tbuf->texflush.addr = SCE_GS_TEXFLUSH;

    tbuf->mipMax = tex->mipMax;
    for (level = 0; level < GA_MIP_LEVELS; level++) {
        tbuf->mipData[level] = tex->mipData[level];
        tbuf->mipTbp[level] = levelTbp[level];
        tbuf->mipTbw[level] = levelTbw[level];
    }
    tbuf->psm = psm;
    tbuf->width = width;
    tbuf->height = height;
    tbuf->clutCount = tex->clutCount;
    for (level = 0; level < GA_CLUTS; level++) {
        tbuf->clutData[level] = tex->clutData[level];
        tbuf->clutCbp[level] = cbp + clutWords * level;
    }
    tbuf->clutWidth = clutW;
    tbuf->clutHeight = clutH;
}

// CT3 0x018a2960 (no caller in the ROM)
void gaBGT_ExecTBufferData_VIF1(void *tbuf)
{
    gaBGT_DmaSendN_fromSPR_VIF1(tbuf, TBUFFER_SEND_QWC);
}

#define GA_GIFTAG_AD_NOEOP(nloop) (0x1000000000000000ull | (u_long)(nloop))
#define GA_GIFTAG_IMAGE(nloop) (0x0800000000008000ull | (u_long)(nloop)) /* FLG IMAGE, EOP */
#define LOADENV_QWC 6                      /* the A+D qwords DIRECT sends before the image */
#define TEXLOAD_ENV_QWC 7                  /* the CNT part: the packet's GA_LOADENV */
#define TEXLOAD_SEND_QWC 9                 /* tag, GA_LOADENV, REF tag */
#define TEXFLUSH_QWC 3

// Marks in the upload packets' DMAtag upper halves (0xdddd000N: the VIF never sees them).
#define GA_LOAD_MARK(kind) (0xdddd0000ull | (u_long)(kind))
enum GA_LOAD_MARK_KIND {
    GA_LOAD_CLUT_ENV = 0,
    GA_LOAD_CLUT_REF = 1,
    GA_LOAD_MIP_ENV = 2,
    GA_LOAD_MIP_REF = 3,
    GA_LOAD_TEXFLUSH = 0xf,
};

// The upload packet, built at the scratchpad's mark packet area (EE 0x70000800), 9 qwords.
typedef struct GA_TEXLOAD_PACKET {
    u_long dmatag;                      /* 0x00: CNT 7 */
    u_long mark;                        /* 0x08 */
    GA_LOADENV env;                     /* 0x10 */
    u_long refTag;                      /* 0x80: REF, qwc = the image's, addr = the image */
    u_long refMark;                     /* 0x88 */
} GA_TEXLOAD_PACKET;
static_assert(sizeof(GA_TEXLOAD_PACKET) == 0x90, "GA_TEXLOAD_PACKET");

typedef struct GA_TEXFLUSH_PACKET {
    GA_PACKET_HEAD head;                /* 0x00: CNT 3, DIRECT 2, A+D x 1 (no EOP) */
    GA_AD texflush;                     /* 0x30 */
} GA_TEXFLUSH_PACKET;
static_assert(sizeof(GA_TEXFLUSH_PACKET) == 0x40, "GA_TEXFLUSH_PACKET");

// CT3 0x018a2978: an upload's GS set-up -- destination (dbp, dbw in their GS units, dpsm), the rectangle at
// (dsax, dsay) of rrw x rrh, host to local -- and the IMAGE tag. Returns the image's qwords (0xffff for a
// format it does not size).
int gaBGT_SetLoadGSEnv(GA_LOADENV *env, u_int dbp, u_int dbw, u_int dpsm, u_int dsax, u_int dsay, int rrw,
                       int rrh)
{
    int qwc = 0xffff;

    switch (dpsm) {
    case SCE_GS_PSMCT32:
        qwc = rrw * rrh >> 2;
        break;
    case SCE_GS_PSMCT24:
        qwc = rrw * rrh * 3 >> 4;
        break;
    case SCE_GS_PSMCT16:
        qwc = rrw * rrh >> 3;
        break;
    case SCE_GS_PSMT8:
    case SCE_GS_PSMT8H:
        qwc = rrw * rrh >> 4;
        break;
    case SCE_GS_PSMT4:
    case SCE_GS_PSMT4HL:
    case SCE_GS_PSMT4HH:
        qwc = rrw * rrh >> 5;
        break;
    }
    env->vifcode[3] = GA_VIFCODE_DIRECT((qwc + LOADENV_QWC) & 0xffff);
    env->giftag = GA_GIFTAG_AD_NOEOP(4);
    env->giftagRegs = GA_GIFTAG_REGS_AD;
    env->bitbltbuf.data = ((u_long)dbp << 32) | ((u_long)dbw << 48) | ((u_long)dpsm << 56);
    env->bitbltbuf.addr = SCE_GS_BITBLTBUF;
    env->trxpos.data = (u_long)dsax | ((u_long)dsay << 16);
    env->trxpos.addr = SCE_GS_TRXPOS;
    env->trxreg.data = (u_long)(u_int)rrw | ((u_long)(u_int)rrh << 32);
    env->trxreg.addr = SCE_GS_TRXREG;
    env->trxdir.addr = SCE_GS_TRXDIR;
    env->imageTag = GA_GIFTAG_IMAGE((u_int)qwc);
    env->imageTagRegs = 0;
    env->vifcode[0] = VC_NOP;
    env->vifcode[1] = VC_NOP;
    env->vifcode[2] = VC_NOP;
    env->trxdir.data = 0;
    return qwc;
}

// A REF tag's address from an EE address, as the ROM converts it: a scratchpad address loses its base;
// bit 30 moves to bit 31 and flips bit 28 (the ROM's arithmetic, whatever it meant), the rest is masked to
// physical.
static u_int GaRefTagAddr(u_int ee)
{
    u_int addr = ee;
    u_int bit30;

    if (ee >= SCRATCHPAD_EE_BASE)
        addr = ee - SCRATCHPAD_EE_BASE;
    bit30 = addr & 0x40000000u;
    return (bit30 << 1) | ((addr & 0x1fffffffu) ^ (bit30 >> 2));
}

static void SetTexFlush(GA_TEXFLUSH_PACKET *packet)
{
    packet->head.vifcode[3] = GA_VIFCODE_DIRECT(TEXFLUSH_QWC - 1);
    packet->head.dmatag = GA_DMATAG_CNT(TEXFLUSH_QWC);
    packet->head.mark = GA_LOAD_MARK(GA_LOAD_TEXFLUSH);
    packet->head.giftag = GA_GIFTAG_AD_NOEOP(1);
    packet->head.giftagRegs = GA_GIFTAG_REGS_AD;
    packet->texflush.addr = SCE_GS_TEXFLUSH;
    packet->head.vifcode[0] = VC_NOP;
    packet->head.vifcode[1] = VC_NOP;
    packet->head.vifcode[2] = VC_NOP;
    packet->texflush.data = 0;
}

static void SetTexLoadRef(GA_TEXLOAD_PACKET *packet, int envKind, u_int data, int qwc)
{
    packet->refMark = GA_LOAD_MARK(envKind + 1);
    packet->refTag = ((u_long)GaRefTagAddr(data) << 32) | (u_long)(u_int)qwc | GA_DMATAG_REF_EMPTY;
}

// CT3 0x018a2ac8: upload a texture buffer's CLUTs (CT32, to clutCbp) and its levels 0..mipMax (to mipTbp,
// each halving), each through the scratchpad's upload packet with a REF to the data, between TEXFLUSHes.
// No caller in the ROM.
void gaBGT_LoadGSTexClutData_VIF1(GA_TBUFFER_DATA *tbuf)
{
    GA_TEXFLUSH_PACKET *flush = (GA_TEXFLUSH_PACKET *)SCRATCHPAD(GA_SPR_MARK_PACKET);
    GA_TEXLOAD_PACKET *load = (GA_TEXLOAD_PACKET *)SCRATCHPAD(GA_SPR_MARK_PACKET);
    u_int i;
    int qwc;

    gaDmaSync_fromSPR();
    SetTexFlush(flush);
    gaBGT_DmaSendN_fromSPR_VIF1(flush, TEXFLUSH_QWC + 1);
    for (i = 0; i < tbuf->clutCount; i++) {
        gaDmaSync_fromSPR();
        load->dmatag = GA_DMATAG_CNT(TEXLOAD_ENV_QWC);
        load->mark = GA_LOAD_MARK(GA_LOAD_CLUT_ENV);
        qwc = gaBGT_SetLoadGSEnv(&load->env, tbuf->clutCbp[i] >> GS_BLOCK_SHIFT, 1, SCE_GS_PSMCT32, 0, 0,
                                 (int)tbuf->clutWidth, (int)tbuf->clutHeight);
        SetTexLoadRef(load, GA_LOAD_CLUT_ENV, tbuf->clutData[i], qwc);
        gaBGT_DmaSendN_fromSPR_VIF1(load, TEXLOAD_SEND_QWC);
    }
    if (tbuf->mipMax != -1) {
        for (i = 0; i < (u_int)tbuf->mipMax + 1; i++) {
            gaDmaSync_fromSPR();
            load->dmatag = GA_DMATAG_CNT(TEXLOAD_ENV_QWC);
            load->mark = GA_LOAD_MARK(GA_LOAD_MIP_ENV);
            qwc = gaBGT_SetLoadGSEnv(&load->env, tbuf->mipTbp[i] >> GS_BLOCK_SHIFT,
                                     tbuf->mipTbw[i] >> GS_BLOCK_SHIFT, tbuf->psm, 0, 0,
                                     (int)(tbuf->width >> (i & 0x1f)), (int)(tbuf->height >> (i & 0x1f)));
            SetTexLoadRef(load, GA_LOAD_MIP_ENV, tbuf->mipData[i], qwc);
            gaBGT_DmaSendN_fromSPR_VIF1(load, TEXLOAD_SEND_QWC);
        }
    }
    gaDmaSync_fromSPR();
    SetTexFlush(flush);
    gaBGT_DmaSendN_fromSPR_VIF1(flush, TEXFLUSH_QWC + 1);
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
