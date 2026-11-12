// ga/gaDrawObject_Redraw_Polygon.h -- CT3 ga/gaDrawObject_Redraw_Polygon.c (0x0189f718..0x018a0880, an asm
// object: a .text section symbol starts it; its first two words, j gaDMAStopMessage_VIF1 / nop, are the
// previous object's tail): the clipped path's divided polygons, redrawn by the EE straight to the GS.
//
// The flow (all of it reads and writes the scratchpad, t9 = 0x70000000, GA_SPR_WORK):
//   trans_for_gs     the redraw vertex records (ga_div_redraw_vertex_work, SetBig / SetNear's) to screen
//                    space on VU0 macro code -> ga_div_redraw_gsvertex_work; the primitive records rebased
//                    onto them -> ga_div_redraw_gsindex_work (GA_SPR_WORK 0x18 / 0x1c: the two ends).
//   *_Redraw_Polygon each primitive: dropped when its three vertices are outside one guard plane, cut by
//                    _DivSub when some are (new vertices and primitives appended, cut again later), else
//                    three GS vertices (ST, RGBAQ, XYZF2) into ga_div_redraw_gspacket_work.
//   *_Output         the packet's VIF DIRECT head and GIFtag, then the fromSPR / MFIFO kick.
// gaDrawObject1_* differ only in a 3-qword GS header per primitive, copied from its GA_OBJ_PRIM_TABLE entry
// (GIFtag GA_SPR_WORK 0xa0 instead of 0x90); the symtab has no caller for them.
#pragma once
#include "ga/gaDrawObject.h"
#include "sdk/scetypes.h"

// The symtab's work buffers (bss), EE addresses as the ROM keeps them in GA_SPR_WORK.
#define GA_DIV_REDRAW_GSINDEX_WORK_EE 0x01a07e40u    /* ga_div_redraw_gsindex_work, 0x4000 bytes */
#define GA_DIV_REDRAW_GSVERTEX_WORK_EE 0x01a0be40u   /* ga_div_redraw_gsvertex_work, 0x20000 bytes */
#define GA_DIV_REDRAW_GSPACKET_WORK_EE 0x01957e40u   /* ga_div_redraw_gspacket_work, 0xb0000 bytes */
#define GA_GSPACKET_WRAP 0x9a000u       /* the cursor goes back to the start once it reaches this offset */
#define GA_EE_UNCACHED_ACCEL 0x20000000u    /* the packet cursor's segment (written through, not cached) */

// One screen-space vertex (trans_for_gs's sqc2 vf5 / vf6 / vf7 / vf3, the clipper's vf8 / vf13 / vf18 /
// vf23): what a GS vertex needs, and the guard-band position whose clip flags drive the cutting.
typedef struct GA_GS_VERTEX {
    float xyzf[4];              /* 0x00: screen x, y, z; w the fog value (FTOI0 -> XYZF2) */
    float stq[4];               /* 0x10: s q, t q, q (-> ST, and Q); w unused */
    float rgba[4];              /* 0x20: the colour, 0..255 (FTOI0 -> RGBAQ) */
    union {
        float xyz[4];           /* 0x30: the view position's x, y scaled to the guard band (+-0.5), z */
        struct {
            float pad[3];
            u_int flags;        /* 0x3c: GA_GUARD_* (VCLIPw against 0.5): the planes still outside */
        } f;
    } clip;
} GA_GS_VERTEX;
static_assert(sizeof(GA_GS_VERTEX) == 0x40, "GA_GS_VERTEX");

// The clip flags (VCLIP's low bits); the clipper cuts against them in this order, -y first.
#define GA_GUARD_POS_X 0x01
#define GA_GUARD_NEG_X 0x02
#define GA_GUARD_POS_Y 0x04
#define GA_GUARD_NEG_Y 0x08
#define GA_GUARD_ALL 0x0f

#define GA_DIV_REDRAW_GSINDEX_PRIMS (0x4000 / 0x10)
#define GA_DIV_REDRAW_GSVERTICES (0x20000 / 0x40)
#define GA_DIV_REDRAW_GSPACKET_SIZE 0xb0000
extern GA_REDRAW_PRIM ga_div_redraw_gsindex_work[GA_DIV_REDRAW_GSINDEX_PRIMS]; // CT3 0x01a07e40
extern GA_GS_VERTEX ga_div_redraw_gsvertex_work[GA_DIV_REDRAW_GSVERTICES];     // CT3 0x01a0be40
extern u_char ga_div_redraw_gspacket_work[GA_DIV_REDRAW_GSPACKET_SIZE];        // CT3 0x01957e40

// layout: host (the ROM's registers across the hand-written routines: a1, v1, s3, s6)
typedef struct GA_REDRAW_STATE {
    u_int listEndEE;            /* a1: the primitive list's end; _DivSub appends here */
    u_int packetHeadEE;         /* v1: the packet's head (VIF qword, GIFtag) */
    u_int packetEE;             /* s3: the next GS vertex */
    int count;                  /* s6: primitives written */
    int bank;                   /* PORT: the object's bank (gaDrawObject1_*'s GA_OBJ_PRIM_TABLE reads) */
} GA_REDRAW_STATE;

void gaDrawObject_Redraw_Polygon(u_char *obj);                                      // CT3 0x0189f720
void gaDrawObject1_Redraw_Polygon(u_char *obj);                                     // CT3 0x0189f84c
void gaDrawObject_Redraw_Polygon_DivSub(GA_REDRAW_STATE *st, const GA_REDRAW_PRIM *prim, u_int orFlags); // CT3 0x0189f994
void gaDrawObject_Redraw_Polygon_Output(GA_REDRAW_STATE *st);                       // CT3 0x018a00f8
void gaDrawObject1_Redraw_Polygon_Output(GA_REDRAW_STATE *st);                      // CT3 0x018a026c
void trans_for_gs(u_char *obj, int flags);                                          // CT3 0x018a03e0
