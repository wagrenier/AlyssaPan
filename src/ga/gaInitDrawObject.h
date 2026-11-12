// ga/gaInitDrawObject.h -- CT3 ga/gaInitDrawObject.c (0x018a1a48..0x018a3078): the ga library's draw
// set-up -- the scratchpad work area the VU1 renderer reads, the GS register packets (GS init, double
// buffer, draw control, fog, texture buffer) and the BGT ("background transfer") MFIFO ring's start, sync
// and end. Slice 1 written 2026-11-09; the layouts below are read from the ROM's stores.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// ---- the packets -----------------------------------------------------------------------------------
// Every ga packet starts the same way, and goes to VIF1 through the BGT ring (gaBGT_DmaSendN_fromSPR_VIF1,
// the whole packet including its DMA tag):
//   +0x00 DMAtag (id CNT, qwc = the qwords after the tag), +0x08 the packet's kind mark (0xeeee000N, in the
//   tag's upper half: the VIF never sees it), +0x10 VIFcodes {NOP, NOP, NOP, DIRECT n}, +0x20 a GIFtag
//   (PACKED, one A+D register, NLOOP = the pairs, EOP), +0x30 the A+D pairs.
#define GA_DMATAG_CNT(qwc) (0x10000000ull | (u_long)(qwc)) /* DMAtag id 1 (CNT): qwc qwords follow */
#define GA_DMATAG_REF_EMPTY 0x30000000ull                  /* DMAtag id 3 (REF), qwc 0, addr 0 */
#define GA_PACKET_MARK(kind) (0xeeee0000ull | (u_long)(kind))
#define GA_VIFCODE_DIRECT(qwc) (0x50000000u | (u_int)(qwc)) /* VIF DIRECT: qwc qwords to the GIF (PATH2) */
#define GA_VIFCODE_MARK(mark) (0x07000000u | ((u_int)(mark) & 0xffff))
#define GA_GIFTAG_AD(nloop) (0x1000000000008000ull | (u_long)(nloop)) /* NREG 1, PACKED, EOP, NLOOP n */
#define GA_GIFTAG_REGS_AD 0xeull                                       /* the one register: A+D */

enum GA_PACKET_KIND {
    GA_PACKET_GSINIT = 0,       /* also gaBGT_PutMark_VIF1's mark packet */
    GA_PACKET_DBUFFER = 1,
    GA_PACKET_DRAWCTRL = 3,
    GA_PACKET_FOGCTRL = 5,
    GA_PACKET_TBUFFER = 6,
};

typedef struct GA_AD {
    u_long data;                        /* 0x00 */
    u_long addr;                        /* 0x08: the GS register */
} GA_AD;
static_assert(sizeof(GA_AD) == 0x10, "GA_AD");

typedef struct GA_PACKET_HEAD {
    u_long dmatag;                      /* 0x00 */
    u_long mark;                        /* 0x08: GA_PACKET_MARK(kind) */
    u_int vifcode[4];                   /* 0x10: NOP, NOP, NOP, DIRECT */
    u_long giftag;                      /* 0x20 */
    u_long giftagRegs;                  /* 0x28 */
} GA_PACKET_HEAD;
static_assert(sizeof(GA_PACKET_HEAD) == 0x30, "GA_PACKET_HEAD");

// gsinit (0x70): COLCLAMP, PRMODECONT, SCANMSK, PABE.
typedef struct GA_GSINIT_DATA {
    GA_PACKET_HEAD head;                /* 0x00 */
    GA_AD ad[4];                        /* 0x30 */
} GA_GSINIT_DATA;
static_assert(sizeof(GA_GSINIT_DATA) == 0x70, "GA_GSINIT_DATA");

// drawbuffer_* (0x80): FRAME_1, ZBUF_1, SCISSOR_1, XYOFFSET_1, DTHE.
typedef struct GA_DBUFFER_DATA {
    GA_PACKET_HEAD head;                /* 0x00 */
    GA_AD frame;                        /* 0x30: FBP rewritten by gaBGT_ExecDBufferData_VIF1 */
    GA_AD zbuf;                         /* 0x40: ZBP likewise */
    GA_AD scissor;                      /* 0x50 */
    GA_AD xyoffset;                     /* 0x60 */
    GA_AD dthe;                         /* 0x70 */
} GA_DBUFFER_DATA;
static_assert(sizeof(GA_DBUFFER_DATA) == 0x80, "GA_DBUFFER_DATA");
static_assert(offsetof(GA_DBUFFER_DATA, frame) == 0x30, "GA_DBUFFER_DATA.frame");
static_assert(offsetof(GA_DBUFFER_DATA, dthe) == 0x70, "GA_DBUFFER_DATA.dthe");

// drawctrl_* (0x90): PRMODE, TEST_1, ALPHA_1, FBA_1, TEX1_1, TEXA.
typedef struct GA_DRAWCTRL_DATA {
    GA_PACKET_HEAD head;                /* 0x00 */
    GA_AD prmode;                       /* 0x30 */
    GA_AD test;                         /* 0x40 */
    GA_AD alpha;                        /* 0x50 */
    GA_AD fba;                          /* 0x60 */
    GA_AD tex1;                         /* 0x70 */
    GA_AD texa;                         /* 0x80 */
} GA_DRAWCTRL_DATA;
static_assert(sizeof(GA_DRAWCTRL_DATA) == 0x90, "GA_DRAWCTRL_DATA");
static_assert(offsetof(GA_DRAWCTRL_DATA, texa) == 0x80, "GA_DRAWCTRL_DATA.texa");

// gaSetupDrawCtrlData's settings, passed by value: the ROM passes each word in its own argument slot (a1..t3,
// then 8-byte stack slots), then TEXA's doubleword (scene_init copies the 0x78 bytes from the draw list).
// Each word is one GS register field; the ROM adds them at its shifts without masking.
typedef struct GA_DRAWCTRL_ARGS {
    // PRMODE (with PRMODECONT 0, gsinit's): the primitive attributes
    u_int iip;                          /* 0x00: Gouraud */
    u_int tme;                          /* 0x04: textured */
    u_int fge;                          /* 0x08: fogged */
    u_int abe;                          /* 0x0c: alpha blended */
    u_int aa1;                          /* 0x10: antialiased */
    u_int fst;                          /* 0x14: UV (not STQ) */
    u_int ctxt;                         /* 0x18: context 2 */
    u_int fix;                          /* 0x1c: fragment value fixed */
    // TEST_1
    u_int ate;                          /* 0x20 */
    u_int atst;                         /* 0x24 */
    u_int aref;                         /* 0x28 */
    u_int afail;                        /* 0x2c */
    u_int date;                         /* 0x30 */
    u_int datm;                         /* 0x34 */
    u_int zte;                          /* 0x38 */
    u_int ztst;                         /* 0x3c */
    // ALPHA_1: ((A - B) * C >> 7) + D
    u_int alphaA;                       /* 0x40 */
    u_int alphaB;                       /* 0x44 */
    u_int alphaC;                       /* 0x48 */
    u_int alphaD;                       /* 0x4c */
    u_int alphaFix;                     /* 0x50 */
    u_int fba;                          /* 0x54: FBA_1 */
    // TEX1_1
    u_int lcm;                          /* 0x58 */
    u_int mxl;                          /* 0x5c */
    u_int mmag;                         /* 0x60 */
    u_int mmin;                         /* 0x64 */
    u_int tex1Bit15;                    /* 0x68: the ROM shifts it to bit 15, where TEX1 has nothing (L is 19..20) */
    u_int k;                            /* 0x6c */
    u_long texa;                        /* 0x70: TEXA, whole */
} GA_DRAWCTRL_ARGS;
static_assert(sizeof(GA_DRAWCTRL_ARGS) == 0x78, "GA_DRAWCTRL_ARGS");
static_assert(offsetof(GA_DRAWCTRL_ARGS, k) == 0x6c, "GA_DRAWCTRL_ARGS.k");
static_assert(offsetof(GA_DRAWCTRL_ARGS, texa) == 0x70, "GA_DRAWCTRL_ARGS.texa");

typedef struct GA_DRAWCOMMAND_DATA {
    u_short qwc;                        /* 0x00: the DMAtag's qwc (the rest of the packet is unread) */
    u_char head[0x36];
    u_long rgbaq;                       /* 0x38: the frame clear's RGBAQ (Q 1.0, A 0x80, RGB = fog colour) */
    u_char tail[0x10];
} GA_DRAWCOMMAND_DATA;
static_assert(sizeof(GA_DRAWCOMMAND_DATA) == 0x50, "GA_DRAWCOMMAND_DATA");
static_assert(offsetof(GA_DRAWCOMMAND_DATA, rgbaq) == 0x38, "GA_DRAWCOMMAND_DATA.rgbaq");

// One bank's fog packet (fogctrl[86], 0x50 bytes each; the symtab's 6880 / 86): FOGCOL, and after the
// sent qwords the three fog floats gaSetupFogCtrlData was given (for the 3dlib work, not the GS).
typedef struct GA_FOGCTRL_DATA {
    GA_PACKET_HEAD head;                /* 0x00 */
    GA_AD fogcol;                       /* 0x30 */
    float fog[3];                       /* 0x40: not sent (the DMAtag counts 3 qwords) */
    u_int pad4c;
} GA_FOGCTRL_DATA;
static_assert(sizeof(GA_FOGCTRL_DATA) == 0x50, "GA_FOGCTRL_DATA");
static_assert(offsetof(GA_FOGCTRL_DATA, fog) == 0x40, "GA_FOGCTRL_DATA.fog");

// ---- textures (gaSetupTBufferData / gaBGT_LoadGSTexClutData_VIF1; no caller in the ROM: the game uses
// its own sun* copies, BRANCH_CLEAR.c) --------------------------------------------------------------------
#define GA_MIP_LEVELS 7                 /* level 0 and MIPTBP1 / MIPTBP2's six */
#define GA_CLUTS 8

// The texture gaSetupTBufferData is given (layout from its loads; words at +0x18 unread). Addresses are
// GS word addresses (TBP / CBP before >> 6) and EE addresses of the image data, as the ROM keeps them.
typedef struct GA_TEX_DESC {
    u_int width;                        /* 0x00: pixels */
    u_int height;                       /* 0x04 */
    u_int tw;                           /* 0x08: TEX0.TW (log2 width) */
    u_int th;                           /* 0x0c: TEX0.TH */
    u_int psm;                          /* 0x10: SCE_GS_PSM* */
    int mipMax;                         /* 0x14: the last mip level; -1 = no image to load */
    u_int pad18;
    u_int mipData[GA_MIP_LEVELS];       /* 0x1c: EE address of each level's pixels */
    u_int clutCount;                    /* 0x38 */
    u_int cld;                          /* 0x3c: != 0 sets TEX0.CLD = 1 (load the CLUT buffer) */
    u_int clutData[GA_CLUTS];           /* 0x40: EE address of each CLUT */
} GA_TEX_DESC;
static_assert(sizeof(GA_TEX_DESC) == 0x60, "GA_TEX_DESC");
static_assert(offsetof(GA_TEX_DESC, clutCount) == 0x38, "GA_TEX_DESC.clutCount");

// A texture buffer packet (0x130): TEX0_1, CLAMP_1, MIPTBP1_1, MIPTBP2_1, TEXFLUSH (8 qwords sent), then
// what gaBGT_LoadGSTexClutData_VIF1 reads to upload the levels and CLUTs.
typedef struct GA_TBUFFER_DATA {
    GA_PACKET_HEAD head;                /* 0x00 */
    GA_AD tex0;                         /* 0x30 */
    GA_AD clamp;                        /* 0x40 */
    GA_AD miptbp1;                      /* 0x50 */
    GA_AD miptbp2;                      /* 0x60 */
    GA_AD texflush;                     /* 0x70 */
    int mipMax;                         /* 0x80: GA_TEX_DESC.mipMax */
    u_int mipData[GA_MIP_LEVELS];       /* 0x84 */
    u_int mipTbp[GA_MIP_LEVELS];        /* 0xa0: word address of each level in GS memory */
    u_int mipTbw[GA_MIP_LEVELS];        /* 0xbc: each level's buffer width, pixels (TBW << 6) */
    u_int psm;                          /* 0xd8 */
    u_int width;                        /* 0xdc */
    u_int height;                       /* 0xe0 */
    u_int clutCount;                    /* 0xe4 */
    u_int clutData[GA_CLUTS];           /* 0xe8 */
    u_int clutCbp[GA_CLUTS];            /* 0x108: word address of each CLUT in GS memory */
    u_int clutWidth;                    /* 0x128: 16 (PSMT8) or 8 */
    u_int clutHeight;                   /* 0x12c: 16 (PSMT8) or 2 */
} GA_TBUFFER_DATA;
static_assert(offsetof(GA_TBUFFER_DATA, mipMax) == 0x80, "GA_TBUFFER_DATA.mipMax");
static_assert(offsetof(GA_TBUFFER_DATA, mipTbw) == 0xbc, "GA_TBUFFER_DATA.mipTbw");
static_assert(offsetof(GA_TBUFFER_DATA, clutData) == 0xe8, "GA_TBUFFER_DATA.clutData");
static_assert(offsetof(GA_TBUFFER_DATA, clutCbp) == 0x108, "GA_TBUFFER_DATA.clutCbp");
static_assert(sizeof(GA_TBUFFER_DATA) == 0x130, "GA_TBUFFER_DATA");

// An image upload's GS set-up (0x70, gaBGT_SetLoadGSEnv): VIFcodes {NOP, NOP, NOP, DIRECT 6 + image qwc},
// a GIFtag (A+D x 4, no EOP), BITBLTBUF / TRXPOS / TRXREG / TRXDIR, then the IMAGE GIFtag whose data the
// DMA REF tag after it brings.
typedef struct GA_LOADENV {
    u_int vifcode[4];                   /* 0x00 */
    u_long giftag;                      /* 0x10 */
    u_long giftagRegs;                  /* 0x18 */
    GA_AD bitbltbuf;                    /* 0x20 */
    GA_AD trxpos;                       /* 0x30 */
    GA_AD trxreg;                       /* 0x40 */
    GA_AD trxdir;                       /* 0x50 */
    u_long imageTag;                    /* 0x60: IMAGE, EOP, NLOOP = the image's qwords */
    u_long imageTagRegs;                /* 0x68 */
} GA_LOADENV;
static_assert(sizeof(GA_LOADENV) == 0x70, "GA_LOADENV");

// ---- the scratchpad work area (EE 0x70000000..) ----------------------------------------------------
// The VU1 renderer's constants and VIFcode templates. Kept at its EE layout: its address words hold EE
// addresses (u_int), as the ROM wrote them (open-items.md (h)'s rule for relocated bank data); a host reader
// maps a scratchpad one with SCRATCHPAD(ee - SCRATCHPAD_EE_BASE). Fields named from gaInitDrawObject's and
// gaInit3dlibWork's stores; "work[n]" is the n-th float of the 3dlib work (ga3dlibwork, copied to 0x400).
typedef struct GA_SPR_WORK {
    u_char pad000[0x14];
    u_int divRedrawPacketEE;            /* 0x014: ga_div_redraw_gspacket_work (0x01957e40), uncached-accelerated */
    u_char pad018[0x8];
    u_int divRedrawIndexEE;             /* 0x020: gaBGT_DrawObject: ga_div_redraw_index_work (0x01a2be40) */
    u_int divRedrawVertexEE;            /* 0x024: gaBGT_DrawObject: ga_div_redraw_vertex_work (0x01a2c640) */
    u_int bigDivIndexEE;                /* 0x028: gaBGT_DrawObject: the big-polygon cursor, from
                                           ga_bigdiv_index_work (0x0193e6c0); moved = gaDrawObject_SetBig */
    u_int nearDivIndexEE;               /* 0x02c: gaBGT_DrawObject: the near-polygon cursor, from
                                           ga_neardiv_index_work (0x0193e8c0); moved = gaDrawObject_SetNear */
    float viewScaleX;                   /* 0x030: work[2] / work[0] */
    float viewScaleY;                   /* 0x034: work[3] / work[1] */
    float zScale;                       /* 0x038: 1 / (work[11] - work[10]) */
    float work6;                        /* 0x03c: work[6] */
    u_int zero040[2];                   /* 0x040 */
    float zOffset;                      /* 0x048: -(work[11] + work[10]) / 2 / (work[11] - work[10]) */
    float half04c;                      /* 0x04c: 0.5 */
    u_int unpackHead[4];                /* 0x050: NOP, NOP, NOP, UNPACK V4-32 11 qwords to VU1 0x100 */
    float work21;                       /* 0x060: work[21] */
    float work22;                       /* 0x064: work[22] */
    float work23x16;                    /* 0x068: work[23] * 16 */
    u_int zero06c;                      /* 0x06c */
    float work26;                       /* 0x070: work[26] */
    float work27;                       /* 0x074: work[27] */
    u_long objReg70;                    /* 0x078: the draw object's +0x70 GS register value (gaBGT_DrawObject's set-ups) */
    u_int vertexSkip;                   /* 0x080: 0 (the nc2 set-ups); vertexCount - drawVertexCount (the clipped) */
    u_int vertexCount;                  /* 0x084: the object's vertex count (the set-ups) */
    u_int primCount;                    /* 0x088: the object's primitive count (the set-ups) */
    float work6b;                      /* 0x08c: work[6] again */
    u_long giftagTri[2];                /* 0x090: PRE TRIANGLE, 9 regs (ST RGBAQ XYZF2) x 3 */
    u_long giftagTriNop[2];             /* 0x0a0: the same after 3 NOPs, 12 regs */
    float work9;                        /* 0x0b0: work[9] */
    float work25;                       /* 0x0b4: work[25] */
    float colourMax;                    /* 0x0b8: 255.0 */
    float work24;                       /* 0x0bc: work[24] */
    u_long objReg78;                    /* 0x0c0: the draw object's +0x78 GS register value (the set-ups) */
    u_int pad0c8;
    float objLight;                     /* 0x0cc: the draw object's +0x30 (the light1 set-ups) */
    u_char pad0d0[0x10];
    u_int drawPrimCount;                /* 0x0e0: gaBGT_DrawObject*: the object's +0x44 */
    u_int drawVertexCount;              /* 0x0e4: gaBGT_DrawObject*: the object's +0x48 */
    u_int pad0e8;
    u_int work3dlibEE;                 /* 0x0ec: the 3dlib work gaInit3dlibWork was given (EE 0x70000400) */
    u_int bgtPacketEE;                  /* 0x0f0: the BGT packet area (EE 0x70000a00), set by gaBGT_Start */
    u_int vertexPacketEE;               /* 0x0f4: the clipped path: the vertex block in the packet (packet + 0xd0
                                           light0, + 0xc0 light1 / light2) */
    u_int stPacketEE;                   /* 0x0f8: gaDrawObject_VertexLocalView*: the end of the vertex block */
    u_int indexPacketEE;                /* 0x0fc: the clipped path: the index block (0xf8 + 0x10 light0, + 0x170
                                           light1 / light2) */
    u_int tailPacketEE;                 /* 0x100: after the index block (gaDrawObject_RemakeIndex*) */
    u_int packetEndEE;                  /* 0x104: the set-ups: the packet's end (the D8 copy's length) */
    u_char pad108[0x8];
    u_int vifMscal6[4];                 /* 0x110: NOP NOP NOP MSCAL 6 */
    u_int vifFlushMscntFlushe[4];       /* 0x120: FLUSH MSCNT FLUSHE NOP */
    u_int vifFlusheMscntFlushe[4];      /* 0x130: FLUSHE MSCNT FLUSHE NOP */
    u_int vifFlush[4];                  /* 0x140: FLUSH NOP NOP NOP */
    u_int vifMscntFlushe[4];            /* 0x150: MSCNT FLUSHE NOP NOP */
    u_int vifMscal8[4];                 /* 0x160: NOP NOP NOP MSCAL 8 */
    u_int vifMscnt[4];                  /* 0x170: NOP NOP NOP MSCNT */
    u_int vifMscalA[4];                 /* 0x180: NOP NOP NOP MSCAL 0xa */
    u_char pad190[0x90];
    u_int drawAttr;                     /* 0x220: gaBGT_DrawObject*: the attr argument (a GS upload or 0) */
    u_char pad224[0xc];                 /* 0x224: the caller's ra; 0x240.. s0..s7 */
    float minusHalf230;                 /* 0x230: -0.5 */
    float half234;                      /* 0x234: 0.5 */
    u_int pad238;
    float half23c;                      /* 0x23c: 0.5 */
} GA_SPR_WORK;
static_assert(offsetof(GA_SPR_WORK, divRedrawPacketEE) == 0x14, "GA_SPR_WORK.divRedrawPacketEE");
static_assert(offsetof(GA_SPR_WORK, viewScaleX) == 0x30, "GA_SPR_WORK.viewScaleX");
static_assert(offsetof(GA_SPR_WORK, zOffset) == 0x48, "GA_SPR_WORK.zOffset");
static_assert(offsetof(GA_SPR_WORK, work21) == 0x60, "GA_SPR_WORK.work21");
static_assert(offsetof(GA_SPR_WORK, work6b) == 0x8c, "GA_SPR_WORK.work6b");
static_assert(offsetof(GA_SPR_WORK, giftagTri) == 0x90, "GA_SPR_WORK.giftagTri");
static_assert(offsetof(GA_SPR_WORK, work9) == 0xb0, "GA_SPR_WORK.work9");
static_assert(offsetof(GA_SPR_WORK, work3dlibEE) == 0xec, "GA_SPR_WORK.work3dlibEE");
static_assert(offsetof(GA_SPR_WORK, objReg70) == 0x78, "GA_SPR_WORK.objReg70");
static_assert(offsetof(GA_SPR_WORK, primCount) == 0x88, "GA_SPR_WORK.primCount");
static_assert(offsetof(GA_SPR_WORK, objReg78) == 0xc0, "GA_SPR_WORK.objReg78");
static_assert(offsetof(GA_SPR_WORK, objLight) == 0xcc, "GA_SPR_WORK.objLight");
static_assert(offsetof(GA_SPR_WORK, drawPrimCount) == 0xe0, "GA_SPR_WORK.drawPrimCount");
static_assert(offsetof(GA_SPR_WORK, vifMscal6) == 0x110, "GA_SPR_WORK.vifMscal6");
static_assert(offsetof(GA_SPR_WORK, minusHalf230) == 0x230, "GA_SPR_WORK.minusHalf230");
static_assert(offsetof(GA_SPR_WORK, nearDivIndexEE) == 0x2c, "GA_SPR_WORK.nearDivIndexEE");
static_assert(offsetof(GA_SPR_WORK, packetEndEE) == 0x104, "GA_SPR_WORK.packetEndEE");
static_assert(offsetof(GA_SPR_WORK, drawAttr) == 0x220, "GA_SPR_WORK.drawAttr");
static_assert(sizeof(GA_SPR_WORK) == 0x240, "GA_SPR_WORK");

#define GA_SPR_WORK_OFFSET 0x000
#define GA_SPR_3DLIB_WORK 0x400         /* main copies ga3dlibwork here each frame */
#define GA_SPR_MARK_PACKET 0x800        /* gaBGT_PutMark_VIF1 / gaBGT_Sync build their packet here */
#define GA_SPR_BGT_PACKET 0xa00         /* gaBGT_Start: where the BGT's packets are built */

// The VU1 microprogram gaInitDrawObject loads (.vutext 0x018ff430, 0x335 qwords of MPG).
#define VU1_OBJECT_DRAW_PROGRAM_QWC 0x335
extern u_int vu1_object_draw_program[VU1_OBJECT_DRAW_PROGRAM_QWC * 4]; /* CT3 0x018ff430 */

void gaInitDrawObject(void);                                                             // CT3 0x018a1a48
void gaInit3dlibWork(float *work);                                                       // CT3 0x018a1c40
void gaSetupGSInitData(GA_GSINIT_DATA *gsinit);                                          // CT3 0x018a1d58
void gaBGT_ExecGSInitData_VIF1(GA_GSINIT_DATA *gsinit);                                  // CT3 0x018a1de0
void gaSetupDBufferData(GA_DBUFFER_DATA *dbuf, u_int fbAddr, u_int fbWidth, u_int fbPsm, u_int fbMsk,
                        u_int zbAddr, u_int zPsm, u_int zMsk, u_int scissorX0, u_int scissorY0,
                        u_int scissorX1, u_int scissorY1, u_int offsetX, u_int offsetY,
                        u_int dthe);                                                     // CT3 0x018a1df8
void gaBGT_ExecDBufferData_VIF1(GA_DBUFFER_DATA *dbuf);                                  // CT3 0x018a1f60
void gaSetupDrawCtrlData(GA_DRAWCTRL_DATA *ctrl, GA_DRAWCTRL_ARGS args);                 // CT3 0x018a1fd8 (args by value)
void gaBGT_ExecDrawCtrlData_VIF1(GA_DRAWCTRL_DATA *ctrl);                                // CT3 0x018a22b0
void gaSetupFogCtrlData(GA_FOGCTRL_DATA *fog, float f0, float f1, float f2, int r, int g, int b); // CT3 0x018a22c8
void gaBGT_ExecFogCtrlData_VIF1(GA_FOGCTRL_DATA *fog);                                  // CT3 0x018a2358
void gaSetupTBufferData(GA_TBUFFER_DATA *tbuf, const GA_TEX_DESC *tex, u_int tbp, u_int cbp, u_int clut,
                        u_int wms, u_int wmt, u_int minu, u_int maxu, u_int minv, u_int maxv,
                        u_int tfx);                                                      // CT3 0x018a2370
void gaBGT_ExecTBufferData_VIF1(void *tbuf);                                             // CT3 0x018a2960
int gaBGT_SetLoadGSEnv(GA_LOADENV *env, u_int dbp, u_int dbw, u_int dpsm, u_int dsax, u_int dsay, int rrw,
                       int rrh);                                                         // CT3 0x018a2978
void gaBGT_LoadGSTexClutData_VIF1(GA_TBUFFER_DATA *tbuf);                                // CT3 0x018a2ac8
void gaBGT_ExecDrawCommandData_VIF1(GA_DRAWCOMMAND_DATA *com);                           // CT3 0x018a2e70
void gaBGT_PutMark_VIF1(u_int mark);                                                     // CT3 0x018a2e90
void gaPutMark_VIF1(u_int mark);                                                         // CT3 0x018a2f10
u_int gaGetMark_VIF1(void);                                                              // CT3 0x018a2f20
void gaBGT_Start(void *ringbuffer, int size);                                            // CT3 0x018a2f30
int gaBGT_Status(void);                                                                  // CT3 0x018a2f58
void gaBGT_Sync(void);                                                                   // CT3 0x018a2fa8
void gaBGT_End(void);                                                                    // CT3 0x018a3000
u_long DR_ST(float s, float t);                                                          // CT3 0x018a3020
u_long DR_Q(float q);                                                                    // CT3 0x018a3058
