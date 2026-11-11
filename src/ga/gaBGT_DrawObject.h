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
#define GA_OBJ_REG70 0x70               /* u_long: a GS register value (to the scratchpad's 0x78) */
#define GA_OBJ_REG78 0x78               /* u_long: a GS register value (to the scratchpad's 0xc0) */

#define GA_OBJ_FLAG_VARIANT 0x03        /* picks the VU1 program: MSCAL 0x1a / 0x22 + 2 x this */
#define GA_OBJ_FLAG_LIGHT 0x1c          /* the light kind: 0 light0; 8, 0x10 light1 (the clipped path traps
                                           on anything else) */
#define GA_OBJ_FLAG_BILLBOARD 0x20      /* = DRAW_OBJ_FLAG_CLIP: the clipped / billboard vertex path */

// The VU1 programs the set-ups start (MSCAL addresses, before + 2 x the variant).
#define GA_VU1_PROG_LIGHT0 0x1a
#define GA_VU1_PROG_LIGHT1 0x22

void gaBGT_DrawObject(u_char *obj, int attr);                                            // CT3 0x018a3150
void gaBGT_DrawObject_NoClippingCheck2(u_char *obj, int attr);                           // CT3 0x018a3d20
