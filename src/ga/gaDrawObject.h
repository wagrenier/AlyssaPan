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
