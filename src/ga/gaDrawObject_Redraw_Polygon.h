// ga/gaDrawObject_Redraw_Polygon.h -- CT3 ga/gaDrawObject_Redraw_Polygon.c (0x0189f718..0x018a0880, an asm
// object): the clipped path's divided polygons (ga_div_redraw_*), sent through trans_for_gs.
//
// Declared from gaBGT_DrawObject.c's call sites (2026-11-10, slice 3); not written yet. obj in a0, state in
// the scratchpad (GA_SPR_WORK 0x20..0x2c).
#pragma once
#include "sdk/scetypes.h"

void gaDrawObject_Redraw_Polygon(u_char *obj); // CT3 0x0189f720
