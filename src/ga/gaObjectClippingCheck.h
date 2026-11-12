// ga/gaObjectClippingCheck.h -- CT3 ga/gaObjectClippingCheck.c: the ga draw object's bounding-sphere test
// against the view frustum (one hand-written function and VU0 micro 0x068; 2026-11-12).
#pragma once
#include "sdk/scetypes.h"

// The ga draw object's bounding sphere (bank data, layout: ee; the rest is gaBGT_DrawObject.h's GA_OBJ_*).
#define GA_OBJ_SPHERE_CENTER 0x10       /* float[4]: the centre in the object's frame (= DRAW_OBJ_CENTER; w unread) */
#define GA_OBJ_SPHERE_RADIUS 0x20       /* float: the radius (the qword's x; the rest unread) */

// gaObjectClippingCheck's answer, which the callers store in the object's GA_OBJ_FLAGS2 byte
// (DRAW_OBJ_CLIP). Outside: not drawn. 0: wholly inside, the no-clip path may draw it. Otherwise bits:
#define GA_CLIP_OUTSIDE (-1)
#define GA_CLIP_INSIDE 0x00
#define GA_CLIP_SIDE 0x01               /* crosses a side plane (left, right, top or bottom) */
#define GA_CLIP_FAR 0x02                /* crosses the far plane (then GA_CLIP_SIDE is the only other bit) */
#define GA_CLIP_NEAR 0x06               /* crosses the near plane: 4 = GA_OBJ_FLAG2_NEAR, with 2 */

int gaObjectClippingCheck(u_char *obj);                                                  // CT3 0x018a3078
