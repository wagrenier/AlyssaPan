// ga/gaObjectClippingCheck.c -- Clock Tower 3 module ga/gaObjectClippingCheck.c (0x018a3078..0x018a3150).
// One hand-written function (scratchpad t9 = 0x70000000, VU0 macro mode and the micro routine 0x068), read
// from the disassembly (mipsdis.py 018a3078 018a3150) and the micro (vudis.py 18fe500 f3 68), 2026-11-12.
// The object's tail is `j gaDMAStopMessage_VIF1` after the return: padding the linker filled, never run.

#include "ga/gaObjectClippingCheck.h"
#include "ga/gaBGT_DrawObject.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaInitDrawObject.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "sdk/eekernel.h"

#include <string.h>

#define SPR_WORK ((GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET))

// Where the function leaves the sphere's view-space centre (sqc2 vf8; it reads the z back at 0x2c8).
// gaDrawObject's SetBig / SetNear overwrite the qword later as their staging area (GA_SPR_DIV_STAGE).
#define GA_SPR_CLIP_CENTER 0x2c0

// The micro's FMAND mask (vi1 = 0xf0: the MAC flags' sign bits, x 0x80 y 0x40 z 0x20 w 0x10) and the
// answer both tests compare with: x and z negative, y and w not -- the left and top planes' sign.
#define VU0_MAC_SIGN_X 0x80
#define VU0_MAC_SIGN_Y 0x40
#define VU0_MAC_SIGN_Z 0x20
#define VU0_MAC_SIGN_W 0x10
#define CLIP_PLANES_PASS (VU0_MAC_SIGN_X | VU0_MAC_SIGN_Z)

// The MAC sign bits of tan * z - edge over the four side planes (VU0 micro 0x071 / 0x072's sub).
static int PlaneSigns(const float tanZ[4], const float edge[4])
{
    static const int bit[4] = {VU0_MAC_SIGN_X, VU0_MAC_SIGN_Y, VU0_MAC_SIGN_Z, VU0_MAC_SIGN_W};
    int c, signs = 0;

    for (c = 0; c < 4; c++)
        if (tanZ[c] - edge[c] < 0.0f)
            signs |= bit[c];
    return signs;
}

// CT3 0x018a3078
// The object's bounding sphere against the view frustum. The centre goes to view space by the object's
// matrix (v * m, w taken as 1: vmulax / vmadday / vmaddaz / vmaddw by vf0.w) and is left at the
// scratchpad's 0x2c0. Z against the 3dlib work's near / far; then VU0 micro 0x068 against the side planes
// (GA_PLANE_* order, x left .. w bottom): each plane's offset at the centre's depth (planeTan * z) against
// the centre's x (left, right) or y (top, bottom) moved by the radius times the plane's secant -- outward
// (vf13: the sphere touches the frustum) and inward (vf12: wholly inside). See GA_CLIP_* for the answer.
// callcheck: gaDMAStopMessage_VIF1 (not this function's: the stray j at 0x018a3148 after its jr ra)
int gaObjectClippingCheck(u_char *obj)
{
    GA_SPR_WORK *spr = SPR_WORK;
    const GA_3DLIB_WORK *lib = (const GA_3DLIB_WORK *)SCRATCHPAD(spr->work3dlibEE - SCRATCHPAD_EE_BASE);
    // PORT: the matrix word is a relocated EE address of the object's bank (as gaBGT_DrawObject reads it).
    const float *m = (const float *)BankEePtr(BankOfHostPtr(obj), *(const u_int *)(obj + GA_OBJ_MATRIX));
    const float *p = (const float *)(obj + GA_OBJ_SPHERE_CENTER);
    float r = *(const float *)(obj + GA_OBJ_SPHERE_RADIUS);
    float v[4], tanZ[4], outer[4], inner[4];
    float zFar, zNear;
    int c, touch, inside, clip;

    for (c = 0; c < 4; c++)
        v[c] = p[0] * m[c] + p[1] * m[4 + c] + p[2] * m[8 + c] + m[12 + c];
    memcpy(SCRATCHPAD(GA_SPR_CLIP_CENTER), v, sizeof(v));

    zFar = v[2] + r;
    zNear = v[2] - r;
    if (zFar <= lib->zNear || lib->zFar <= zNear)
        return GA_CLIP_OUTSIDE;

    // micro 0x068: vf1 = (x, x, y, y); vf10 = planeSec * r; vf11 = planeTan * z.
    for (c = 0; c < 4; c++) {
        float centre = c < 2 ? v[0] : v[1];
        float reach = lib->planeSec[c] * r;
        // vf13 (outer) adds on x and z (left, top) and subtracts on y and w (right, bottom); vf12 the reverse.
        int plus = (c & 1) == 0;

        tanZ[c] = lib->planeTan[c] * v[2];
        outer[c] = plus ? centre + reach : centre - reach;
        inner[c] = plus ? centre - reach : centre + reach;
    }
    touch = PlaneSigns(tanZ, outer);    // vi2
    inside = PlaneSigns(tanZ, inner);   // vi3
    if (touch != CLIP_PLANES_PASS)
        return GA_CLIP_OUTSIDE;

    if (lib->zFar < zFar)
        return inside == CLIP_PLANES_PASS ? GA_CLIP_FAR : GA_CLIP_FAR | GA_CLIP_SIDE;
    clip = inside == CLIP_PLANES_PASS ? GA_CLIP_INSIDE : GA_CLIP_SIDE;
    if (zNear < lib->zNear)
        clip |= GA_CLIP_NEAR;
    return clip;
}
