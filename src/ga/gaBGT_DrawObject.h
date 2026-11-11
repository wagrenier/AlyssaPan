// ga/gaBGT_DrawObject.h -- CT3 ga/gaBGT_DrawObject.c: the ga draw object's send to the VU1 renderer.
// Slice 1 (2026-11-10): the no-clip path (gaBGT_DrawObject_NoClippingCheck2 and its nc2 set-ups), read
// from the disassembly (the decompiler mangles these hand-written functions: they keep their state in the
// scratchpad, t9 = 0x70000000). The ROM builds a VIF1 chain of REF tags to the object's blocks; the port
// hands the same blocks to the host draw module (alyssapan_drawga.h) instead.
#pragma once
#include "sdk/scetypes.h"

// The ga draw object (bank data, layout: ee; its pointer words are relocated EE addresses, BankEePtr).
// ct3_types.h's DRAW_OBJ_* name the bytes main_rtn touches; these are the rest the set-ups read.
#define GA_OBJ_FLAGS 0x00               /* u_char: GA_OBJ_FLAG_* */
#define GA_OBJ_LIGHT_SCALE 0x30         /* float: the light1 set-up copies it to the scratchpad (0xcc) */
#define GA_OBJ_POSITION 0x38            /* u_int EE: the object's position qword (= DRAW_OBJ_POS) */
#define GA_OBJ_LIGHT_WORK 0x3c          /* u_int EE: 17 qwords the light1 set-up sends (GA_LIGHT_WORK) */
#define GA_OBJ_PRIM_COUNT 0x44          /* u_int */
#define GA_OBJ_VERTEX_COUNT 0x48        /* u_int */
#define GA_OBJ_INDICES 0x50             /* u_int EE: primCount x 4 halfwords (UNPACK V4-16 to VU 0x17f) */
#define GA_OBJ_MATRIX 0x58              /* u_int EE: 4 qwords (to the header's 0xa0..0xd0) */
#define GA_OBJ_LIGHT_MATRIX 0x5c        /* u_int EE: 3 qwords, light1 only (the header's 0x70..0x90) */
#define GA_OBJ_POSITIONS 0x60           /* u_int EE: vertexCount qwords (UNPACK V4-32 to VU 0) */
#define GA_OBJ_NORMALS 0x64             /* u_int EE: a VIF head qword + vertexCount qwords, light1 only */
#define GA_OBJ_ST 0x68                  /* u_int EE: a VIF head qword (STMASK, UNPACK V2-32) + the STs */
#define GA_OBJ_COLOURS 0x6c             /* u_int EE: a VIF head qword (UNPACK V4-8) + the RGBAs */
#define GA_OBJ_REG70 0x70               /* 2 floats (to the scratchpad's 0x78): the VU1 program's Z offset and
                                           fog base (vf17.zw), not a GS register (slice 2) */
#define GA_OBJ_REG78 0x78               /* 2 floats (to the scratchpad's 0xc0): the ST offset (vf30.xy) */

#define GA_OBJ_FLAG_VARIANT 0x03        /* picks the VU1 program: MSCAL 0x1a / 0x22 + 2 x this */
#define GA_OBJ_FLAG_LIGHT 0x1c          /* the light kind (GA_LIGHT_KIND_*); anything else hits a break */
#define GA_OBJ_FLAG_BILLBOARD 0x20      /* = DRAW_OBJ_FLAG_CLIP: the vertices go to the local view as a
                                           billboard (gaDrawObject_VertexLocalView*_Billboard) */
#define GA_OBJ_FLAGS2 0x02              /* u_char: GA_OBJ_FLAG2_* */
#define GA_OBJ_FLAG2_NEAR 0x04          /* gaBGT_DrawObject: the near-clipping index pass (_WithNear) */

// flags & GA_OBJ_FLAG_LIGHT. gaBGT_DrawObject gives each its own set-up; the no-clip paths send light2 to
// light1's (their light2 calls are never reached).
#define GA_LIGHT_KIND_0 0x00
#define GA_LIGHT_KIND_1 0x08
#define GA_LIGHT_KIND_2 0x10

// The VU1 programs the set-ups start (MSCAL addresses, before + 2 x the variant).
#define GA_VU1_PROG_LIGHT0 0x1a
#define GA_VU1_PROG_LIGHT1 0x22
// The clipped set-ups' programs (the vertices arrive in the local view): light0 puts MSCAL 2 in the vertex
// block's VIF qword; light1 / light2 REF the scratchpad's NOP NOP NOP MSCAL 6 / MSCAL 0xa (GA_SPR_WORK
// vifMscal6 / vifMscalA).
#define GA_VU1_PROG_CLIP_LIGHT0 0x02

// The division buffers gaBGT_DrawObject points GA_SPR_WORK 0x20..0x2c at (symtab objects, EE addresses:
// gaDrawObject.c and gaDrawObject_Redraw_Polygon.c own them; a cursor that moved means polygons were put
// on that list).
#define GA_DIV_REDRAW_INDEX_WORK_EE 0x01a2be40u  /* ga_div_redraw_index_work, 0x800 bytes */
#define GA_DIV_REDRAW_VERTEX_WORK_EE 0x01a2c640u /* ga_div_redraw_vertex_work, 0x4000 bytes */
#define GA_BIGDIV_INDEX_WORK_EE 0x0193e6c0u      /* ga_bigdiv_index_work, 0x200 bytes */
#define GA_NEARDIV_INDEX_WORK_EE 0x0193e8c0u     /* ga_neardiv_index_work, 0x200 bytes */

void gaBGT_DrawObject(u_char *obj, int attr);                                            // CT3 0x018a3150
void gaBGT_DrawObject_NoClippingCheck(u_char *obj, int attr);                            // CT3 0x018a3958
void gaBGT_DrawObject_NoClippingCheck2(u_char *obj, int attr);                           // CT3 0x018a3d20
