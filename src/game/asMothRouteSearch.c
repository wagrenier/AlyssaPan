// game/asMothRouteSearch.c -- Clock Tower 3: MothInfo's life (init / destroy) and the moth's route search
// around scene collision (symtab local chunk 83). CodeWarrior C. Written whole on 2026-11-11.
// The moths are parts of the bank asXcmd_I_have_x_Mothes armed (MothInfo.bank); asMothOperate (its own
// TU) flies them and asks asMothRouteSearch for a clear line when the straight one is blocked.

#include "game/asMothRouteSearch.h"
#include "game/ahalloc.h"
#include "game/hhXcmd_DressUp.h"
#include "game/LinkMatrixTree.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/tiXcmd_TransItem.h"
#include "game/unassigned_data.h"
#include "ga/gaInit3dlib.h"

#include <math.h>
#include <string.h>

MOTH_INFO MothInfo;                     /* sbss 0x0193ced0 */

#define MOTH_TURN 0x10000               /* asMakeRotMatrixByQuartanion's angle unit: a full turn */
#define MOTH_HALF_TURN 0x8000
#define MOTH_STEEP_DOT 0.9f             /* the hit direction this close to up: cross with x instead */

static const float MothUp[4] = {0.0f, 1.0f, 0.0f, 1.0f};    /* @328 (rodata 0x019386c0) */
static const float MothSide[4] = {1.0f, 0.0f, 0.0f, 1.0f};  /* @329 (rodata 0x019386d0) */

// CT3 0x01882c10
void asInitMothInfo(void)
{
    MothInfo.list = NULL;
    MothInfo.active = 0;
    MothInfo.bank = 0;
    MothInfo.count = 0;
    MothInfo.endTicks[0] = -1;
    MothInfo.endTicks[1] = -1;
    MothInfo.w24 = 0;
}

// CT3 0x01882c40: frees every moth's block and the list, drops the pad rumble and the global event
// TRIG_EVENT_MOTHS_GONE, and stamps the stage's first end; reloadSE loads the SE bank MOTH_SE_BANK_GONE.
void asDestroyMothInfo(int reloadSE)
{
    int i;

    if (!MothInfo.active)
        return;
    if (MothInfo.list != NULL) {
        for (i = 0; i < MothInfo.count; i++)
            _SunFree(&gHeapJunc, &MothInfo.list[i].mem);
        _SunFree(&gHeapJunc, (void **)&MothInfo.list);
    }
    if (MothInfo.clatter != -1)
        ControllerClatteringFree(MothInfo.clatter);
    ssTrigRemoveEvent(TRIG_EVENT_MOTHS_GONE, XCMD_SCOPE_GLOBAL, 0);
    MothInfo.clatter = -1;
    MothInfo.active = 0;
    MothInfo.bank = 0;
    MothInfo.count = 0;
    MothInfo.w24 = 0;
    if (MothInfo.endTicks[MothInfo.stage] == 0)
        MothInfo.endTicks[MothInfo.stage] = gTotalTicks;
    if (reloadSE) {
        // The ROM copies the 6-byte name to the stack and passes argv = {name}.
        char name[8];
        char *argv[1];
        strcpy(name, MOTH_SE_BANK_GONE);
        argv[0] = name;
        hhXcmd_LoadSE(1, argv, 0);
    }
}

// out = m[0] * v.x + m[1] * v.y + m[2] * v.z + m[3] * v.w (VU0 vmulabc / vmaddabc / vmaddbc).
static void MothMulMatVec(float *out, float (*m)[4], const float *v)
{
    float r[4];
    int i;

    for (i = 0; i < 4; i++)
        r[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
    memcpy(out, r, sizeof(r));
}

// v *= 1 / |v.xyz| over all four lanes (vrsqrt with vf0.w).
static void MothNormalize(float *v)
{
    float q = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    int i;

    for (i = 0; i < 4; i++)
        v[i] *= q;
}

// out.xyz = a x b (vopmula / vopmsub); w 0 (the ROM leaves the register's w, which no reader uses).
static void MothCross(float *out, const float *a, const float *b)
{
    out[0] = a[1] * b[2] - b[1] * a[2];
    out[1] = a[2] * b[0] - b[2] * a[0];
    out[2] = a[0] * b[1] - b[0] * a[1];
    out[3] = 0.0f;
}

// The candidate end's offset from `from`: the straight offset tilted by `tilt` about tiltAxis, then
// turned by `around` about the hit direction.
static void MothCandidate(float *v, float (*tiltRot)[4], const float *straight, float *hitDir, int around)
{
    float turn[4][4];

    MothMulMatVec(v, tiltRot, straight);
    asMakeRotMatrixByQuartanion(turn, hitDir, around);
    MothMulMatVec(v, turn, v);
}

// CT3 0x01882890 (asMothOperate calls it with stepAround 0x5ed0, stepTilt 0x1555 and shortens `to`
// towards `from` while it answers 1).
int asMothRouteSearch(int bank, float *from, float *to, float *out, int stepAround, int stepTilt)
{
    LOS_HIT los;
    float straight[4], hitDir[4], tiltAxis[4], v[4];
    float tiltRot[4][4];
    u_int tilt, around;
    int blocked;
    int i;

    memcpy(los.seg.from, from, sizeof(los.seg.from));
    memcpy(los.seg.to, to, sizeof(los.seg.to));
    if (CheckLOS(bank, &los.seg) == 0) {
        memcpy(out, to, 4 * sizeof(float));
        return 0;
    }
    memcpy(out, los.point, 4 * sizeof(float));

    // The tilt axis: perpendicular to the direction of the hit, from up (or x when that is nearly up).
    for (i = 0; i < 4; i++)
        hitDir[i] = los.point[i] - from[i];
    MothNormalize(hitDir);
    {
        float dot = gaInnerProductXYZ(hitDir, (float *)MothUp);
        MothCross(tiltAxis, hitDir, (dot >= MOTH_STEEP_DOT || dot <= -MOTH_STEEP_DOT) ? MothSide : MothUp);
    }
    MothNormalize(tiltAxis);
    // (The ROM takes the hit offset's squared length here and drops it.)

    for (i = 0; i < 4; i++)
        straight[i] = to[i] - from[i];
    blocked = 1;
    around = 0;
    for (tilt = stepTilt & 0xffff; blocked && tilt < MOTH_HALF_TURN; tilt += stepTilt & 0xffff) {
        asMakeRotMatrixByQuartanion(tiltRot, tiltAxis, (int)tilt);
        MothCandidate(v, tiltRot, straight, hitDir, (int)around);
        while (blocked && around <= MOTH_TURN) {
            for (i = 0; i < 4; i++)
                los.seg.to[i] = v[i] + from[i];
            if (CheckLOS(bank, &los.seg) == 0) {
                blocked = 0;
            } else {
                // As the ROM: the next candidate is built before the step, so each ring's first end is
                // tested twice.
                MothCandidate(v, tiltRot, straight, hitDir, (int)around);
                around += stepAround & 0xffff;
            }
        }
        around -= MOTH_TURN;
    }
    if (blocked)
        return 1;
    memcpy(out, los.seg.to, 4 * sizeof(float));
    return 0;
}
