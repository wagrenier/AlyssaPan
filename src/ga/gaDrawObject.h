// ga/gaDrawObject.h -- CT3 ga/gaDrawObject.c (0x018a4fd8..0x018a6b70, an asm object): the EE side of the ga
// draw object's clipped path -- vertices to the local view on VU0, the index record remade with the
// clipping flags, the vertex reference table, the big / near polygon lists.
//
// Declared from gaBGT_DrawObject.c's call sites (2026-11-10, slice 3); not written yet. These are hand-written
// functions that keep their state in the scratchpad (t9 = 0x70000000, GA_SPR_WORK): obj in a0, the packet
// position (an EE scratchpad address) in a2, nothing returned -- they leave their results in GA_SPR_WORK
// (stPacketEE 0xf8, drawPrimCount 0xe0, tailPacketEE 0x100, the div cursors 0x20..0x2c).
#pragma once
#include "sdk/scetypes.h"

// GA_OBJ_FLAGS bit 1 (part of GA_OBJ_FLAG_VARIANT, the VU1 program choice): the index passes skip the VU0
// facing test (their accept mask is 0x3f0 instead of 0x7f0), and MakeVertexReferenceTableNoClip references
// every vertex. Named from those two uses (slice 1, 2026-11-12).
#define GA_OBJ_FLAG_DOUBLE_SIDED 0x02

// The vertex block in the BGT packet (GA_SPR_WORK.vertexPacketEE): a 0x20-byte head (DMAtag CNT 1 + n,
// the VIF qword with UNPACK V4-32 n), then n qwords; stPacketEE = the head + n qwords, rounded to 4.
#define GA_VERTEX_BLOCK_HEAD 0x20
#define GA_VERTEX_USED 0xff     /* +0xe: a kept polygon uses the vertex (the index passes' sb 0xff) */

// One local-view vertex (the VU0 micro routines' sqi of vf24..vf31; their mfir.w puts the clip flags in w).
// After MakeVertexReferenceTable, a slot's w word is a vertex index instead.
typedef struct GA_LOCAL_VERTEX {
    float xyz[3];               /* 0x0 */
    union {
        struct {
            u_short clip;       /* 0xc: the clip flags the index passes AND over a polygon */
            u_char used;        /* 0xe: GA_VERTEX_USED */
            u_char pad;         /* 0xf */
        } f;
        u_int ref;              /* 0xc: MakeVertexReferenceTable*: the vertex this slot sends */
    } w;
} GA_LOCAL_VERTEX;
static_assert(sizeof(GA_LOCAL_VERTEX) == 0x10, "GA_LOCAL_VERTEX");

// obj +0x54 (u_int EE): a head qword, then a table SetBig points each divided polygon into: + 0x10 + the
// index entry's fourth halfword (a byte offset). What it holds is the redraw's (Redraw_Polygon, unread).
#define GA_OBJ_PRIM_TABLE 0x54

// The redraw buffers SetBig / SetNear fill (GA_SPR_WORK 0x24 / 0x20 walk them; Redraw_Polygon reads them).
// One vertex record per corner of a divided polygon (SetBig's sqc2 at +0x00 / 0x10 / 0x20 / 0x30).
typedef struct GA_REDRAW_VERTEX {
    float view[4];              /* 0x00: the local-view qword (xyz, w = the clip flags as a word) */
    float normal[4];            /* 0x10: obj +0x64's, when it has normals */
    float st[4];                /* 0x20: s, t; the high half is the staging qword's leftovers */
    float colour[4];            /* 0x30: the V4-8 colour, sign-extended, as floats */
} GA_REDRAW_VERTEX;
static_assert(sizeof(GA_REDRAW_VERTEX) == 0x40, "GA_REDRAW_VERTEX");

// One primitive record: its three vertex records' EE addresses and its GA_OBJ_PRIM_TABLE entry.
typedef struct GA_REDRAW_PRIM {
    u_int vertexEE[3];          /* 0x0 */
    u_int primTableEE;          /* 0xc */
} GA_REDRAW_PRIM;
static_assert(sizeof(GA_REDRAW_PRIM) == 0x10, "GA_REDRAW_PRIM");

#define GA_DIV_REDRAW_VERTICES (0x4000 / 0x40)  /* ga_div_redraw_vertex_work: 0x4000 bytes */
#define GA_DIV_REDRAW_PRIMS (0x800 / 0x10)      /* ga_div_redraw_index_work: 0x800 bytes */
extern GA_REDRAW_VERTEX ga_div_redraw_vertex_work[GA_DIV_REDRAW_VERTICES];  // CT3 0x01a2c640
extern GA_REDRAW_PRIM ga_div_redraw_index_work[GA_DIV_REDRAW_PRIMS];        // CT3 0x01a2be40

void gaDrawObject_VertexLocalViewWithClippingFlagSet(u_char *obj, u_int packetEE);           // CT3 0x018a4fe0
void gaDrawObject_VertexLocalView(u_char *obj, u_int packetEE);                              // CT3 0x018a5100
void gaDrawObject_VertexLocalViewWithClippingFlagSet_Billboard(u_char *obj, u_int packetEE); // CT3 0x018a51e8
void gaDrawObject_VertexLocalView_Billboard(u_char *obj, u_int packetEE);                    // CT3 0x018a52f0
void gaDrawObject_RemakeIndexWithClippingCheck(u_char *obj, u_int packetEE);                 // CT3 0x018a53c8
void gaDrawObject_RemakeIndexWithClippingCheck_WithNear(u_char *obj, u_int packetEE);        // CT3 0x018a5730
void gaDrawObject_RemakeIndex(u_char *obj, u_int packetEE);                                  // CT3 0x018a5b88
void gaDrawObject_MakeVertexReferenceTable(u_char *obj);                                     // CT3 0x018a5fe0
void gaDrawObject_MakeVertexReferenceTableNoClip(u_char *obj);                               // CT3 0x018a6038
void gaDrawObject_SetBig(u_char *obj);                                                       // CT3 0x018a6080
void gaDrawObject_SetNear(u_char *obj);                                                      // CT3 0x018a621c
