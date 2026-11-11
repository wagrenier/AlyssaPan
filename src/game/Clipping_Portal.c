// game/Clipping_Portal.c -- Clock Tower 3: the ti* effects module (wind, vibration, fades, light belts,
// lens flares, pendulums, particles, translucence; symtab local chunk 88). CodeWarrior C. Slice 1
// (2026-11-11): vibration, wind, tiLightBelt2Init, tiFade_Start / _Draw, tiFactorial; the rest are stubs until reconstructed.

#include "common/alyssapan_stub.h"
#include "game/Clipping_Portal.h"
#include "game/LinkMatrixTree.h"
#include "game/ct3_types.h"
#include "game/main.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/tiXcmd_TransItem.h"
#include "game/unassigned_data.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaInitDrawObject.h"
#include "sdk/eekernel.h"
#include "ga/pad_button_read.h"
#include "alyssapan/rendering/alyssapan_drawga.h"

#include <math.h>
#include <string.h>

WIND *WindList[WIND_MAX];                       /* sbss 0x0193d290 */
int VibTimeBaseTicks;                           /* sbss 0x0193f770 */
static u_char Fade_Type;                        /* sbss 0x0193d350 (local): FADE_TYPE_*, tiFade_Draw's */

// ---- vibration ----

// CT3 0x0188cd70: once a frame (main, FSMltLoadEndWaitFile): count the vibration's ticks and stop it once
// they pass VibTimeBaseTicks. The ROM counts whether or not a timed vibration runs.
void tiVibrationTimer(void)
{
    VibTimeTicks += gGlobalNextTicksInterval;
    if (VibTimeBaseTicks < VibTimeTicks) {
        tiVibrationOff(0);
        fVibTimer = 0;
    }
}

// CT3 0x0188cdc0: stop both motors of pad `port` (only while the vibration option is on).
void tiVibrationOff(int port)
{
    if (fVibration) {
        act_small = 0;
        act_large = 0;
        gaPadSetVibration(&pad[(u_char)port], 0, 0);
        fVibTimer = 0;
    }
}

// CT3 0x0188ce10: start the motors of pad `port`: the large motor's level accumulates (0.75 of `large`
// on top of what runs, capped at 255; a level of 1 counts as 2); time != 0 stops it after `time` frames.
// Nothing while the option is off or a demo is being skipped.
void tiVibrationOn(int port, u_char small, int large, int time)
{
    int level;

    if (fVibration && DemoSkip == 0) {
        if (large == 1) {
            large = 2;
        }
        level = act_large + (int)((float)large * VIB_LARGE_SCALE);
        if (level > VIB_LARGE_MAX) {
            level = VIB_LARGE_MAX;
        }
        act_large = (u_char)level;
        act_small = small;
        gaPadSetVibration(&pad[(u_char)port], small, act_large);
        if (time != 0) {
            VibTimeBaseTicks = time * VIB_TICKS_PER_FRAME;
            fVibTimer = 1;
            VibTimeTicks = 0;
        }
    }
}

// ---- wind ----

// PORT helper: out = m * v, the VU0 sequence vmulax / vmadday / vmaddaz / vmaddw (m's rows are the
// columns of the transform; the sum in the ROM's order). The wind and the light belts use it.
static void PortApplyMatrix(float *out, float (*m)[4], const float *v)
{
    int j;

    for (j = 0; j < 4; j++) {
        out[j] = m[0][j] * v[0] + m[1][j] * v[1] + m[2][j] * v[2] + m[3][j] * v[3];
    }
}

// CT3 0x0188cb10: drop the winds bank `scene` registered (scene_end).
void tiWindEnd(int scene)
{
    int i;

    for (i = 0; i < WIND_MAX; i++) {
        if (WindList[i] != NULL && WindList[i]->bank == scene) {
            WindList[i] = NULL;
        }
    }
}

// CT3 0x0188cb60: drop every wind (system_end).
void tiWindEnd_All(void)
{
    int i;

    for (i = 0; i < WIND_MAX; i++) {
        WindList[i] = NULL;
    }
}

// CT3 0x0188cbe0: once a frame (main): carry each wind's three local points into its part's frame.
void tiWindUpdate(void)
{
    int i;

    for (i = 0; i < WIND_MAX; i++) {
        WIND *wind = WindList[i];
        float (*m)[4];

        if (wind == NULL) {
            continue;
        }
        m = BANK_PTR(float (*)[4], wind->bank, wind->matrix);
        PortApplyMatrix(wind->world30, m, wind->local30);
        PortApplyMatrix(wind->world40, m, wind->local40);
        PortApplyMatrix(wind->world10, m, wind->local10);
    }
}

// CT3 0x0188ccb0: register bank `scene`'s winds (scene_init): each record learns its bank and the matrix
// it follows, and takes the first free WindList slot (none free: not registered).
void tiWindInit(int scene)
{
    u_int *list = BANK_PTR(u_int *, scene, All_table_TBL[scene]->winds);
    int i, slot;

    if (list == NULL) {
        return; // PORT: the ROM reads the list at EE address 0 (no bank seen so far lacks one)
    }
    for (i = 0; list[i] != 0; i++) {
        WIND *wind = BANK_PTR(WIND *, scene, list[i]);
        MODEL_PARTS *part = &((MODEL_PARTS *)pModelPartsHeader[scene])[wind->part];

        if (part->kind == PARTS_KIND_WIND_ON_ROOT) {
            part = (MODEL_PARTS *)pModelPartsHeader[scene];
        }
        wind->matrix = BankEeAddr(scene, part->matrix200);
        wind->bank = scene;
        for (slot = 0; slot < WIND_MAX; slot++) {
            if (WindList[slot] == NULL) {
                WindList[slot] = wind;
                break;
            }
        }
    }
}

// ---- light belts ----

// CT3 0x0188b050: set up bank `scene`'s light belts (scene_init): a belt on one part puts every point at
// the part's position; a belt between two parts keeps half the span in each part's own frame.
void tiLightBelt2Init(int scene)
{
    u_int *list = BANK_PTR(u_int *, scene, All_table_TBL[scene]->lightBelts);
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[scene];
    float inv[4][4];
    int i, k;

    if (list == NULL) {
        return;
    }
    for (i = 0; list[i] != 0; i++) {
        LIGHT_BELT2 *belt = BANK_PTR(LIGHT_BELT2 *, scene, list[i]);
        MODEL_PARTS *a = &parts[belt->partA];
        MODEL_PARTS *b;
        float v[4];

        if (belt->link == 0) {
            float (*points)[4] = BANK_PTR(float (*)[4], scene, belt->points);

            for (k = 0; k < belt->count; k++) {
                memcpy(points[k], a->pos, sizeof(points[k]));
            }
            continue;
        }
        b = &parts[belt->partB];
        for (k = 0; k < 3; k++) {
            belt->halfA[k] = (b->pos[k] - a->pos[k]) * 0.5f;
        }
        belt->halfA[3] = 0.0f;
        for (k = 0; k < 3; k++) {
            belt->halfB[k] = belt->halfA[k] * -1.0f;
        }
        belt->halfB[3] = 0.0f;
        gaInversMatrix(inv, a->matrix);
        memcpy(v, belt->halfA, sizeof(v));
        PortApplyMatrix(belt->halfA, inv, v);
        gaInversMatrix(inv, b->matrix);
        memcpy(v, belt->halfB, sizeof(v));
        PortApplyMatrix(belt->halfB, inv, v);
    }
}

// ---- fade ----

// CT3 0x0188d3c0: start a fade of kind `type` (FADE_TYPE_*) lasting `time` seconds; tiFade_Draw runs it.
void tiFade_Start(float time, int type)
{
    Fade_Type = (u_char)type;
    fFade = 1;
    FadeStartTicks = 0;
    FadeEndTicks = (int)(time * FADE_TICKS_PER_SECOND);
}

// The fade's full-screen sprites (data 0x01938c80 init_black, 0x01938ce0 init_white; 0x60 bytes, the
// last 0x10 zero): a DMAtag of qwc 4, a GIFtag of 3 A+D registers (PRIM sprite, RGBAQ, XYZ2 x2), the
// colour with A 0x80 and Q 1.0, and the GS window (0x7ff8, 0x7ff8) .. (0x9ff8, 0x9bf8): 512 x 448.
// The alpha is the draw control's ALPHA_1 FIX, not the colour's A.
// PORT: the packets are not sent (no GS emulation): the host draw module covers the whole window with the
// sprite's colour at the same FIX (alyssapan_drawga.h AlyssaPan_DrawGaFill), at any aspect.
#define FADE_RGBA_BLACK 0x80000000u     /* init_black's RGBAQ: r g b 0, a 0x80 */
#define FADE_RGBA_WHITE 0x80ffffffu     /* init_white's RGBAQ: r g b 0xff, a 0x80 */

// The fade's draw control: flat, untextured, UV, alpha blended; alpha test ATST 1 (always) AFAIL 1, Z test
// ZTST 1 (always; ZTE 0); ALPHA_1 (Cs - Cd) * FIX + Cd; TEX1 LCM 1 MXL 2 MMAG 1 MMIN 1.
#define FADE_DRAWCTRL_ARGS(alphaFix) {                                          0, 0, 0, 1, 0, 1, 0, 0,      /* PRMODE: ABE, FST */                         1, 1, 0, 1, 0, 0, 0, 1,      /* TEST_1: ATE, ATST 1, AFAIL 1, ZTST 1 */     0, 1, 2, 1, (u_int)(alphaFix), /* ALPHA_1: A Cs, B Cd, C FIX, D Cd */       1,                           /* FBA_1 */                                    1, 2, 1, 1, 0, 0,            /* TEX1_1 */                                   0}                           /* TEXA */
#define FADE_ALPHA_FULL 128.0f          /* ALPHA_1 FIX of a fully covered screen */

// CT3 0x0188d160: once a frame while fFade: cover the screen with black or white at the fade's alpha
// (fades in go from covered to clear, fades out from clear to covered); the IN2 kinds hold a pause while
// they run; when the time is up the fade ends, except the OUT2 kinds, which stay covered.
void tiFade_Draw(void)
{
    alignas(16) static GA_DRAWCTRL_DATA drawctrl; /* sbss 0x0193d2c0 drawctrl$1812 */
    int left = FadeEndTicks - FadeStartTicks;
    float f = 0.0f;

    if (left < 0) {
        left = 0;
    }
    switch (Fade_Type) {
    case FADE_TYPE_IN_BLACK:
    case FADE_TYPE_IN_WHITE:
    case FADE_TYPE_IN2_BLACK:
    case FADE_TYPE_IN2_WHITE:
        f = FadeEndTicks == 0 ? 0.0f : (float)left / (float)FadeEndTicks;
        break;
    case FADE_TYPE_OUT_BLACK:
    case FADE_TYPE_OUT_WHITE:
    case FADE_TYPE_OUT2_BLACK:
    case FADE_TYPE_OUT2_WHITE:
        f = FadeEndTicks == 0 ? 1.0f : 1.0f - (float)left / (float)FadeEndTicks;
        break;
    }
    {
        GA_DRAWCTRL_ARGS args = FADE_DRAWCTRL_ARGS((int)(FADE_ALPHA_FULL * f));

        gaSetupDrawCtrlData(&drawctrl, args);
    }
    FlushCache(0);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl);
    switch (Fade_Type) {
    case FADE_TYPE_IN_BLACK:
    case FADE_TYPE_OUT_BLACK:
    case FADE_TYPE_OUT2_BLACK:
    case FADE_TYPE_IN2_BLACK:
        AlyssaPan_DrawGaFill(FADE_RGBA_BLACK, (int)(drawctrl.alpha.data >> 32 & 0xff));
        break;
    case FADE_TYPE_IN_WHITE:
    case FADE_TYPE_OUT_WHITE:
    case FADE_TYPE_OUT2_WHITE:
    case FADE_TYPE_IN2_WHITE:
        AlyssaPan_DrawGaFill(FADE_RGBA_WHITE, (int)(drawctrl.alpha.data >> 32 & 0xff));
        break;
    }
    if (left <= 0) {
        if (Fade_Type == FADE_TYPE_IN2_BLACK || Fade_Type == FADE_TYPE_IN2_WHITE) {
            pause_reservation = 0;
        }
        if (Fade_Type != FADE_TYPE_OUT2_BLACK && Fade_Type != FADE_TYPE_OUT2_WHITE) {
            fFade = 0;
        }
    } else {
        if (Fade_Type == FADE_TYPE_IN2_BLACK || Fade_Type == FADE_TYPE_IN2_WHITE) {
            pause_reservation = 1;
        }
        FadeStartTicks += gGlobalNextTicksInterval;
    }
    // callcheck: gaBGT_ExecDrawCommandData_VIF1 (PORT: the sprite packet's send; AlyssaPan_DrawGaFill draws it)
}

// ---- maths ----

// CT3 0x01894870: base ** n by repeated multiplication (the ROM unrolls the loop by eight).
int tiFactorial(int base, u_int n)
{
    int result = 1;
    u_int i;

    for (i = 0; i < n; i++) {
        result *= base;
    }
    return result;
}

// ---- portal clipping ----

// CT3 0x01889cd0: main_rtn and its variants, while Clipping_System is 1: narrow the 3dlib work's side
// planes to the portal rectangle (Clipping_X1..Y2_pos, screen pixels). Only the tangents follow the
// rectangle; the secants keep the screen's own spans, and the left and bottom secants use the centre
// alone (CX / sx, CY / sy) -- all as the ROM computes them.
void Clipping_Portal(void *work, float *screen)
{
    GA_3DLIB_WORK *w = (GA_3DLIB_WORK *)work;
    float cx = screen[GA_SCREEN_CX];
    float cy = screen[GA_SCREEN_CY];
    float sx = screen[GA_SCREEN_SCALE_X];
    float sy = screen[GA_SCREEN_SCALE_Y];
    float p = screen[GA_SCREEN_PROJ];
    float t;

    t = cx / sx;
    w->planeSec[GA_PLANE_LEFT] = sqrtf(t * t + p * p) / p;
    w->planeTan[GA_PLANE_LEFT] = -((cx - Clipping_X1_pos) / sx) / p;
    t = (screen[GA_SCREEN_W] - cx) / sx;
    w->planeSec[GA_PLANE_RIGHT] = sqrtf(t * t + p * p) / p;
    w->planeTan[GA_PLANE_RIGHT] = ((Clipping_X2_pos - cx) / sx) / p;
    t = (screen[GA_SCREEN_H] - cy) / sy;
    w->planeSec[GA_PLANE_TOP] = sqrtf(t * t + p * p) / p;
    w->planeTan[GA_PLANE_TOP] = -((Clipping_Y2_pos - cy) / sy) / p;
    t = cy / sy;
    w->planeSec[GA_PLANE_BOTTOM] = sqrtf(t * t + p * p) / p;
    w->planeTan[GA_PLANE_BOTTOM] = ((cy - Clipping_Y1_pos) / sy) / p;
}

// ---- feet ----
//
// The leg parts are matrix-tree records (CHARACTER.legParts: part + CHARA_LEG_PART_OFFSET), so these are
// LinkMatrixTree.c's record offsets.
#define LEG_REC_BITS 0x3                /* u_char: LEG_REC_BIT_DIRTY makes BuildMatrixTree rebuild the part */
#define LEG_REC_BIT_DIRTY 0x1
#define LEG_REC_PIVOT 0x20              /* float[4]: the joint's point in its own world frame (MODEL_PARTS +0x60;
                                           named from this use) */
#define LEG_REC_PARENT 0x8c             /* u_int: EE address of the parent's matrix */
#define LEG_REC_POS 0xf0                /* float[4]: MODEL_PARTS.pos */
#define LEG_REC_MATRIX 0x180            /* float[4][4]: the world matrix (MODEL_PARTS.matrix) */
#define LEG_REC_MATRIX_Y 0x190          /* its row 1: the thigh's bend axis */
#define LEG_REC_OFFSET 0x240            /* float[4][4]: applied after the local matrix */
#define LEG_REC_BASE 0x280              /* float[4][4]: the part's own transform (MODEL_PARTS.placeRot) */
#define LEG_F(rec, off) ((float *)((rec) + (off)))
#define LEG_M(rec, off) ((float (*)[4])((rec) + (off)))

#define FOOT_LOS_ABOVE 50.0f            /* the floor probe starts this far above the foot ... */
#define FOOT_LOS_BELOW 1.0f             /* ... and ends this far below it */

// PORT: the record's REC_PARENT word as a host matrix (BuildMatrixTree's PartsParentMatrix).
static float (*FootParentMatrix(int bank, const u_char *rec))[4]
{
    u_int ee = *(const u_int *)(rec + LEG_REC_PARENT);

    if (ee == MATUNIT_EE)
        return matUnit;
    return BANK_PTR(float (*)[4], bank, ee);
}

// out.xyz = (a - b).xyz / |(a - b).xyz| (VU0 vrsqrt), out.w = (a - b).w; answers the length (1 / rsqrt).
static float FootDirLen(float out[4], const float *a, const float *b)
{
    float d[4], q;
    int k;

    for (k = 0; k < 4; k++)
        d[k] = a[k] - b[k];
    q = 1.0f / sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    for (k = 0; k < 3; k++)
        out[k] = d[k] * q;
    out[3] = d[3];
    return 1.0f / q;
}

// The turn that brings the joint's direction to `cur` onto `want` (both seen from its pivot in the
// inverse frame `inv`): the axis want x cur (VU0 vopmula / vopmsub, not normalised: the ROM's), the angle
// acos(cur . want); `axis` keeps its w (the ROM reuses the vector).
static void FootTurn(float (*m)[4], float (*inv)[4], const float *pivot, const float *cur, const float *want,
                     float axis[4])
{
    float a[4], b[4], u1[4], u2[4];

    PortApplyMatrix(a, inv, cur);
    PortApplyMatrix(b, inv, want);
    FootDirLen(u1, a, pivot);
    FootDirLen(u2, b, pivot);
    u1[3] = a[3]; // vsub.xyz: w stays the transformed point's
    u2[3] = b[3];
    axis[0] = u2[1] * u1[2] - u1[1] * u2[2];
    axis[1] = u2[2] * u1[0] - u1[2] * u2[0];
    axis[2] = u2[0] * u1[1] - u1[0] * u2[1];
    asMakeRotMatrixByQuartanion(m, axis, ahAcos(gaInnerProductXYZ(u1, u2)));
}

// CT3 0x0188b240: two-bone IK for one leg: bend thigh and calf (their LEG_REC_BASE) so the foot's IK point
// reaches `target`. Out of reach (thigh-to-target longer than the two bones): nothing turns, *footY = the
// IK point's height, answers 0. Else the knee goes where the law of cosines puts it (turned about the
// thigh's Y row), the thigh turns to it, the chain is rebuilt down to the IK point, the calf turns that
// point onto the target, *footY = target's height, answers 1.
// PORT: the bank is added in front (the records' EE words resolve in it).
int tiCalc_FootPos(int bank, u_char *thigh, u_char *calf, u_char *foot, u_char *ik, float *target, float *footY)
{
    alignas(16) float pivot[4] = {0}, axis[4] = {0}, m[4][4], chain[4][4], tip[4][4];
    float thighPos[4], knee[4], ikPos[4], dir[4], unused[4], ikAt[4];
    float reach, thighLen, calfLen;
    int k;

    memcpy(thighPos, LEG_F(thigh, LEG_REC_POS), sizeof(thighPos));
    memcpy(knee, LEG_F(calf, LEG_REC_POS), sizeof(knee));
    memcpy(ikPos, LEG_F(ik, LEG_REC_POS), sizeof(ikPos));
    reach = FootDirLen(dir, target, thighPos);
    thighLen = FootDirLen(unused, knee, thighPos);
    calfLen = FootDirLen(unused, ikPos, knee);
    if (thighLen + calfLen < reach) {
        *footY = ikPos[1];
        return 0;
    }

    // the knee: the thigh-to-target direction turned about the thigh's Y row, thighLen out
    asMakeRotMatrixByQuartanion(m, LEG_F(thigh, LEG_REC_MATRIX_Y),
                                ahAcos((reach * reach + thighLen * thighLen - calfLen * calfLen) /
                                       (reach * 2.0f * thighLen)));
    PortApplyMatrix(axis, m, dir);
    for (k = 0; k < 4; k++)
        knee[k] = thighPos[k] + axis[k] * thighLen;

    // the thigh: turn the calf's position onto the knee
    gaInversMatrix(chain, LEG_M(thigh, LEG_REC_MATRIX));
    memcpy(pivot, LEG_F(thigh, LEG_REC_PIVOT), sizeof(pivot));
    FootTurn(m, chain, pivot, LEG_F(calf, LEG_REC_POS), knee, axis);
    gaMulMatrix(LEG_M(thigh, LEG_REC_BASE), LEG_M(thigh, LEG_REC_BASE), m);

    // the chain down to the IK point, with the thigh's new base
    gaMulMatrix(chain, FootParentMatrix(bank, thigh), LEG_M(thigh, LEG_REC_BASE));
    gaMulMatrix(chain, chain, LEG_M(calf, LEG_REC_BASE));
    gaMulMatrix(tip, chain, LEG_M(foot, LEG_REC_BASE));
    gaMulMatrix(tip, tip, LEG_M(ik, LEG_REC_BASE));
    gaMulMatrix(tip, tip, LEG_M(ik, LEG_REC_OFFSET));
    PortApplyMatrix(ikAt, tip, LEG_F(ik, LEG_REC_PIVOT));

    // the calf: turn the IK point onto the target
    gaMulMatrix(chain, chain, LEG_M(calf, LEG_REC_OFFSET));
    gaInversMatrix(chain, chain);
    memcpy(pivot, LEG_F(calf, LEG_REC_PIVOT), sizeof(pivot));
    FootTurn(m, chain, pivot, ikAt, target, axis);
    gaMulMatrix(LEG_M(calf, LEG_REC_BASE), LEG_M(calf, LEG_REC_BASE), m);
    *footY = target[1];
    return 1;
    // callcheck: gaInnerProductXYZ (FootTurn makes it)
}

// One leg of tiCharaFootAdjust (leg[0..3]: thigh, calf, foot, IK point): probe the floor under the IK
// point (FOOT_LOS_ABOVE over it to FOOT_LOS_BELOW under it, in the character's lightBank: the scene bank
// it stands in) and, on a floor hit, keep the thigh's and calf's bases in saved[] and bend the leg onto
// the hit point.
static int FootAdjustLeg(CHARACTER *chr, u_char **leg, float (*saved)[4][4], LOS_HIT *hit, float *footY)
{
    float *foot = LEG_F(leg[CHARA_LEG_IK_R - CHARA_LEG_R_THIGH], LEG_REC_POS);

    memcpy(hit->seg.from, foot, sizeof(hit->seg.from));
    memcpy(hit->seg.to, foot, sizeof(hit->seg.to));
    hit->seg.from[1] += FOOT_LOS_ABOVE;
    hit->seg.to[1] -= FOOT_LOS_BELOW;
    if (CheckLOS(chr->lightBank, &hit->seg) == 0)
        return 0;
    if (hit->record == NULL || // PORT: the stub CheckLOS leaves none
        (*(u_int *)((u_char *)hit->record + LOS_HIT_FLAGS) & LOS_HIT_FLAG_FLOOR) == 0)
        return 0;
    memcpy(saved[0], LEG_M(leg[0], LEG_REC_BASE), sizeof(saved[0]));
    memcpy(saved[1], LEG_M(leg[1], LEG_REC_BASE), sizeof(saved[1]));
    return (u_char)tiCalc_FootPos(chr->bank, leg[0], leg[1], leg[2], leg[3], hit->point, footY);
}

// CT3 0x0188b790: once a frame (main): plant the feet of every live character (CHARACTER_FLAG_REFRACTION
// and _SHADOW) that asks for it (CHARACTER.footAdjust) on the floor under them: bend the legs, rebuild
// the bank's matrix tree once with the bent legs (BuildMatrixTree mode 2), then put the thigh and calf
// bases back, so the bend lasts this frame's matrices only.
void tiCharaFootAdjust(void)
{
    alignas(16) float savedR[2][4][4], savedL[2][4][4];
    LOS_HIT hit;
    float footYR, footYL;
    int i, k;

    memset(&hit, 0, sizeof(hit));
    for (i = 0; i < CHARACTER_MAX; i++) {
        CHARACTER *chr = &Character[i];
        u_char *legs[8];
        int right = 0, left = 0;

        if ((chr->flags & CHARACTER_FLAG_REFRACTION) == 0 || (chr->flags & CHARACTER_FLAG_SHADOW) == 0)
            continue;
        for (k = 0; k < 8; k++)
            legs[k] = BANK_PTR(u_char *, chr->bank, chr->legParts[k]);
        if (chr->footAdjust & CHARA_FOOT_ADJUST_R)
            right = FootAdjustLeg(chr, &legs[CHARA_LEG_R_THIGH], savedR, &hit, &footYR);
        if (chr->footAdjust & CHARA_FOOT_ADJUST_L)
            left = FootAdjustLeg(chr, &legs[CHARA_LEG_L_THIGH], savedL, &hit, &footYL);
        if (!right && !left)
            continue;
        if (right) {
            legs[CHARA_LEG_R_THIGH][LEG_REC_BITS] = LEG_REC_BIT_DIRTY;
            legs[CHARA_LEG_R_CALF][LEG_REC_BITS] = LEG_REC_BIT_DIRTY;
        }
        if (left) {
            legs[CHARA_LEG_L_THIGH][LEG_REC_BITS] = LEG_REC_BIT_DIRTY;
            legs[CHARA_LEG_L_CALF][LEG_REC_BITS] = LEG_REC_BIT_DIRTY;
        }
        BuildMatrixTree(chr->bank, 2, 0);
        if (right) {
            memcpy(LEG_M(legs[CHARA_LEG_R_THIGH], LEG_REC_BASE), savedR[0], sizeof(savedR[0]));
            memcpy(LEG_M(legs[CHARA_LEG_R_CALF], LEG_REC_BASE), savedR[1], sizeof(savedR[1]));
        }
        if (left) {
            memcpy(LEG_M(legs[CHARA_LEG_L_THIGH], LEG_REC_BASE), savedL[0], sizeof(savedL[0]));
            memcpy(LEG_M(legs[CHARA_LEG_L_CALF], LEG_REC_BASE), savedL[1], sizeof(savedL[1]));
        }
    }
    // callcheck: CheckLOS, tiCalc_FootPos (FootAdjustLeg makes them)
}

// ---- particles and pendulums (slice 3, 2026-11-11) ----
// The banks tiParticleUpdate and tiHurikoUpdate run: the scene banks (0..5) always, on the global clock;
// the rest only while the world runs (pause 0 or PAUSE_RUNNING), on the scene clock, and not while the
// bank's animation is forced still. A group's later banks are skipped with groupCount.

// CT3 0x0188f9a0: drop expired records off the ring's head. The ROM reads count and head once and walks
// all count records from that head: any expired one (life - age < 1) moves head past it and takes one
// off count, wherever it sits -- kept.
void tiCalc_LifeSpan(PARTICLE2_ENTRY *ent, int bank)
{
    u_int count = ent->count;
    u_int head = ent->head;
    PARTICLE2 *ring = BANK_PTR(PARTICLE2 *, bank, ent->ring);
    u_int i;

    for (i = 0; i < count; i++) {
        int slot = (int)((head + i) % ent->ringSize);

        if (ring[slot].life - ring[slot].age < 1) {
            ent->head = (u_short)((slot + 1) % ent->ringSize);
            ent->count--;
        }
    }
}

// One list of a bank's second particles (ALL_TABLE.particles2: after 0; particles2After: after 1).
static void ParticleList(int bank, u_int listEE, int after, int ticks, CAMERA *cam)
{
    u_int *list = BANK_PTR(u_int *, bank, listEE);
    int i;

    if (list == NULL) {
        return; // PORT: the ROM reads the list at EE address 0
    }
    for (i = 0; list[i] != 0; i++) {
        PARTICLE2_ENTRY *ent = BANK_PTR(PARTICLE2_ENTRY *, bank, list[i]);

        tiCalc_LifeSpan(ent, bank);
        tiGenerateParticle(ticks, ent, ssTrigCheckEnable(ent->parts, bank), bank);
        tiCalc_Orbit(ticks, ent, after, cam, bank);
    }
}

static void ParticleBank(int bank, int ticks, CAMERA *cam)
{
    ParticleList(bank, All_table_TBL[bank]->particles2, 0, ticks, cam);
    ParticleList(bank, All_table_TBL[bank]->particles2After, 1, ticks, cam);
}

// CT3 0x01890070: age, emit and move every loaded bank's second particles (main, once a frame). The
// scene banks take Cam[0] and do not test the bank's type; the rest take Cam[CAMERA_SHARED] and skip a
// behaviour bank.
void tiParticleUpdate(void)
{
    int bank;

    for (bank = 0; bank < BANK_SCENE_MAX; bank++) {
        if (BankInfo[bank].drawOn != 0) {
            ParticleBank(bank, gGlobalNextTicksInterval, &Cam[0]);
            bank += BankInfo[bank].groupCount - 1;
        }
    }
    if (pause != 0 && (pause & PAUSE_RUNNING) == 0) {
        return;
    }
    for (bank = BANK_SCENE_MAX; bank < BANK_MAX; bank++) {
        if (BankInfo[bank].drawOn != 0 && BankInfo[bank].type != BANK_TYPE_BEHAVIOR) {
            if ((BankInfo[bank].flags & BANK_FLAG_ANIM_FORCE_STOP) == 0) {
                ParticleBank(bank, gNextTicksInterval, &Cam[CAMERA_SHARED]);
            }
            bank += BankInfo[bank].groupCount - 1;
        }
    }
    // callcheck: ssTrigCheckEnable, tiCalc_LifeSpan, tiCalc_Orbit, tiGenerateParticle (ParticleList makes them)
}

// One bank's pendulums. A pendulum whose trigger is off only marks itself DISABLED; once on again it
// becomes RESET (a scene bank's still swings that frame, the others wait a frame), and on the next
// enabled frame tiHurikoReset puts it back before it swings.
static void HurikoBank(int bank, int ticks, int sceneBank)
{
    u_int *list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->huriko);
    int i;

    if (list == NULL) {
        return; // PORT: the ROM reads the list at EE address 0
    }
    for (i = 0; list[i] != 0; i++) {
        HURIKO *h = BANK_PTR(HURIKO *, bank, list[i]);
        u_int was = h->flags;

        if (ssTrigCheckEnable(h->trigPart, bank) == 0) {
            h->flags |= HURIKO_FLAG_DISABLED;
            continue;
        }
        if (was & HURIKO_FLAG_RESET) {
            tiHurikoReset(bank, h);
            h->flags &= ~HURIKO_FLAG_RESET;
        }
        if (was & HURIKO_FLAG_DISABLED) {
            h->flags &= ~HURIKO_FLAG_DISABLED;
            h->flags |= HURIKO_FLAG_RESET;
            if (!sceneBank) {
                continue;
            }
        }
        if ((was & HURIKO_FLAG_CLOSED) == 0) {
            tiCalc_HurikoOPEN(h, bank, ticks);
        } else {
            tiCalc_HurikoCLOSED(h, bank, ticks);
        }
        tiGetNewNormalData(h);
    }
}

// CT3 0x01892ee0: swing every loaded bank's pendulums (main, once a frame); the banks are
// tiParticleUpdate's, but the scene banks skip a behaviour bank too.
void tiHurikoUpdate(void)
{
    int bank;

    for (bank = 0; bank < BANK_SCENE_MAX; bank++) {
        if (BankInfo[bank].drawOn != 0 && BankInfo[bank].type != BANK_TYPE_BEHAVIOR) {
            HurikoBank(bank, gGlobalNextTicksInterval, 1);
            bank += BankInfo[bank].groupCount - 1;
        }
    }
    if (pause != 0 && (pause & PAUSE_RUNNING) == 0) {
        return;
    }
    for (bank = BANK_SCENE_MAX; bank < BANK_MAX; bank++) {
        if (BankInfo[bank].drawOn != 0 && BankInfo[bank].type != BANK_TYPE_BEHAVIOR) {
            if ((BankInfo[bank].flags & BANK_FLAG_ANIM_FORCE_STOP) == 0) {
                HurikoBank(bank, gNextTicksInterval, 0);
            }
            bank += BankInfo[bank].groupCount - 1;
        }
    }
    // callcheck: ssTrigCheckEnable, tiCalc_HurikoCLOSED, tiCalc_HurikoOPEN, tiGetNewNormalData, tiHurikoReset (HurikoBank makes them)
}

// CT3 0x01891570: VU0 -- the whole vector (w too) times rsqrt(x*x + y*y + z*z).
void gaNormalizeVectorXYZ(float *dst, const float *src)
{
    float q = 1.0f / sqrtf(src[0] * src[0] + src[1] * src[1] + src[2] * src[2]);

    dst[0] = src[0] * q;
    dst[1] = src[1] * q;
    dst[2] = src[2] * q;
    dst[3] = src[3] * q;
}

// The pendulum record's first word: the ROM reads it whole as tiHurikoInit's part and tiHurikoReset_On's
// id (ssTrigCheckEnable takes its low half, HURIKO.trigPart).
static int HurikoWord0(const HURIKO *h)
{
    int w;

    memcpy(&w, h, sizeof w);
    return w;
}

// The part a pendulum or a hit hangs on: a PARTS_KIND_WIND_ON_ROOT part stands for part 0 (as tiWindInit).
static MODEL_PARTS *HurikoPart(int bank, int part)
{
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];

    if (parts[part].kind == PARTS_KIND_WIND_ON_ROOT) {
        return parts;
    }
    return &parts[part];
}

// A joint's position word: posRef -> an EE word -> the position; this answers that last EE word.
static u_int HurikoJointPosEE(int bank, u_int jointEE)
{
    HURIKO_JOINT *j = BANK_PTR(HURIKO_JOINT *, bank, jointEE);

    return BANK_PTR(u_int *, bank, j->posRef)[0];
}

// CT3 0x018933f0: scene_init -- link each pendulum's joints and take their rest pose from the part's
// placement, then point each hit at its part's flags and second matrix. Every pendulum starts DISABLED,
// so tiHurikoUpdate resets it on its first enabled frames.
void tiHurikoInit(int scene)
{
    ALL_TABLE *tbl = All_table_TBL[scene];
    u_int *list = BANK_PTR(u_int *, scene, tbl->huriko);
    u_int *hits = BANK_PTR(u_int *, scene, tbl->hurikoHits);
    int i;

    for (i = 0; list != NULL && list[i] != 0; i++) { // PORT: the ROM reads a 0 list at EE address 0
        HURIKO *h = BANK_PTR(HURIKO *, scene, list[i]);
        MODEL_PARTS *part;
        u_int *tab;
        float inv[4][4];
        int nc, nj, c, k;

        h->flags |= HURIKO_FLAG_DISABLED;
        part = HurikoPart(scene, HurikoWord0(h));
        tab = BANK_PTR(u_int *, scene, h->jointTable);
        nc = h->chains;
        nj = h->joints;
        gaInversMatrix(inv, part->matrix);

        // the roots keep their local position (all four words)
        for (c = 0; c < nc; c++) {
            HURIKO_JOINT *root = BANK_PTR(HURIKO_JOINT *, scene, tab[c * nj]);

            memcpy(root->pos, BANK_PTR(float *, scene, HurikoJointPosEE(scene, tab[c * nj])), sizeof root->pos);
        }
        // the neighbours: up / down in the chain, left / right in the ring of chains
        for (c = 0; c < nc; c++) {
            for (k = 1; k < nj; k++) {
                HURIKO_JOINT *j = BANK_PTR(HURIKO_JOINT *, scene, tab[k + c * nj]);

                j->up = HurikoJointPosEE(scene, tab[k + c * nj - 1]);
                if (k < nj - 1) {
                    j->down = HurikoJointPosEE(scene, tab[k + c * nj + 1]);
                }
                j->left = HurikoJointPosEE(scene, tab[k + nj * ((nc - 1 + c) % nc)]);
                j->right = HurikoJointPosEE(scene, tab[k + nj * ((c + 1) % nc)]);
            }
        }
        // the rest pose: each joint's offset from where it would hang below the one above, in part space
        for (c = 0; c < nc; c++) {
            float prev[4], cur[4], d[4];

            PortApplyMatrix(prev, part->matrix, BANK_PTR(float *, scene, HurikoJointPosEE(scene, tab[c * nj])));
            for (k = 1; k < nj; k++) {
                HURIKO_JOINT *j = BANK_PTR(HURIKO_JOINT *, scene, tab[k + c * nj]);
                float rest[3];

                PortApplyMatrix(cur, part->matrix, BANK_PTR(float *, scene, HurikoJointPosEE(scene, tab[k + c * nj])));
                gaCopyVectorXYZ(j->pos, cur);
                rest[0] = prev[0] + 0.0f * j->length;
                rest[1] = prev[1] + HURIKO_REST_DOWN_Y * j->length;
                rest[2] = prev[2] + 0.0f * j->length;
                d[0] = cur[0] - rest[0];
                d[1] = cur[1] - rest[1];
                d[2] = cur[2] - rest[2];
                d[3] = 0.0f;
                PortApplyMatrix(j->restDir, inv, d);
                memcpy(prev, cur, sizeof prev);
            }
        }
    }
    for (i = 0; hits != NULL && hits[i] != 0; i++) {
        HURIKO_HIT *hit = BANK_PTR(HURIKO_HIT *, scene, hits[i]);
        MODEL_PARTS *part = HurikoPart(scene, hit->part);

        hit->matrix = BankEeAddr(scene, part->matrix200);
        hit->partFlags = BankEeAddr(scene, &part->flags);
    }
}

// One bank's pendulum hits: both points into the part's second frame, unless the part is hidden.
static void HurikoHitBank(int bank)
{
    u_int *hits = BANK_PTR(u_int *, bank, All_table_TBL[bank]->hurikoHits);
    int i;

    for (i = 0; hits != NULL && hits[i] != 0; i++) { // PORT: the ROM reads a 0 list at EE address 0
        HURIKO_HIT *hit = BANK_PTR(HURIKO_HIT *, bank, hits[i]);
        float (*m)[4];
        float *a;

        if (*BANK_PTR(u_int *, bank, hit->partFlags) & PARTS_FLAG_HIDDEN) {
            continue;
        }
        m = BANK_PTR(float (*)[4], bank, hit->matrix);
        a = BANK_PTR(float *, bank, hit->a);
        PortApplyMatrix(hit->world0, m, a);
        if (hit->pairInA == 0) {
            PortApplyMatrix(hit->world1, m, BANK_PTR(float *, bank, hit->b));
        } else {
            PortApplyMatrix(hit->world1, m, a + 4);
        }
    }
}

// CT3 0x018931f0: once a frame (main): carry every loaded bank's pendulum hits. Paused (and the world
// not running) only the scene banks; unlike tiHurikoUpdate, a forced-stop bank is skipped among them too.
void tiHurikoHitUpdate(void)
{
    int banks = BANK_MAX;
    int bank;

    if (pause != 0 && (pause & PAUSE_RUNNING) == 0) {
        banks = BANK_SCENE_MAX;
    }
    for (bank = 0; bank < banks; bank++) {
        if (BankInfo[bank].drawOn != 0 && BankInfo[bank].type != BANK_TYPE_BEHAVIOR) {
            if ((BankInfo[bank].flags & BANK_FLAG_ANIM_FORCE_STOP) == 0) {
                HurikoHitBank(bank);
            }
            bank += BankInfo[bank].groupCount - 1;
        }
    }
}

// CT3 0x01890ea0: tiXcmd_HurikoReset -- mark a bank's pendulums DISABLED (all, or the one whose first
// word is id), so tiHurikoUpdate resets them; byChara: bank is a character slot, its bank is taken.
void tiHurikoReset_On(int bank, int id, int byChara)
{
    u_int *list;
    int i;

    if (byChara) {
        bank = Character[bank].bank;
    }
    list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->huriko);
    for (i = 0; list != NULL && list[i] != 0; i++) { // PORT: the ROM reads a 0 list at EE address 0
        HURIKO *h = BANK_PTR(HURIKO *, bank, list[i]);

        if (id == 0 || HurikoWord0(h) == id) {
            h->flags |= HURIKO_FLAG_DISABLED;
        }
    }
}

// ---- translucence ----

// CT3 0x0188ddc0: end a character's translucence (ssTrigExecuteActCmd, ssAI_BrainEndSpecialProc,
// tiXcmd_SetCharacterEdgeShading): flag bit 0 clears CHARA_TRANS_TRANSLUCENT, else bit 1 clears
// CHARA_TRANS_GHOST; either way the alpha, the edge level and the edge parameters go to 0.
void tiSet_CharaTranslucenceOFF(int chara, int flag)
{
    CHARACTER *chr = &Character[chara];
    int i;

    if (flag & 1) {
        chr->translucent &= (u_char)~CHARA_TRANS_TRANSLUCENT;
    } else if (flag & 2) {
        chr->translucent &= (u_char)~CHARA_TRANS_GHOST;
    }
    chr->translucentAlpha = 0.0f;       /* the ROM stores the word 0 */
    chr->edgeLevel = 0;
    for (i = 0; i < 6; i++) {
        chr->h536[i] = 0;
    }
}

// CT3 0x0188ded0: start a character's translucence (ssTrigExecuteActCmd, tiXcmd_SetCharacterEdgeShading):
// flag bit 0 sets the state to CHARA_TRANS_TRANSLUCENT, else bit 1 to CHARA_TRANS_GHOST (stored, not
// or-ed, unlike tiSet_CharaTranslucenceOFF's clears); the level becomes the alpha either way.
void tiSet_CharaTranslucenceON(int chara, int level, int flag)
{
    CHARACTER *chr = &Character[chara];

    if (flag & 1) {
        chr->translucent = CHARA_TRANS_TRANSLUCENT;
    } else if (flag & 2) {
        chr->translucent = CHARA_TRANS_GHOST;
    }
    chr->translucentAlpha = (float)level;
}

// ---- the pipe joint (chara 3's tube) ----
//
// Character CHARA_PIPE's bank has a tube from part s_tyubu01 (the base) to s_tyubu02 (the target) with an
// elbow, n_29 (the joint). Each frame the elbow is set by two-bone IK (upper length PIPE_LENGTH1, lower
// PIPE_LENGTH2) and then swung by a damped spring toward the target's REC_PIPE_AIM, sagging under
// PipeGravity; the joint's REC_BASE is set for BuildMatrixTree and put back to its rest pose after.
// The parts are matrix-tree records, so the offsets are LinkMatrixTree.c's (the LEG_REC_* above).
#define CHARA_PIPE 3                    /* Character[3]: main2.c's CHARA_E03 ("E_03"), who calls tiPipeJointInit */
#define PIPE_PART_BASE "s_tyubu01"      /* data 0x019344e8 */
#define PIPE_PART_JOINT "n_29"          /* data 0x019344f8 */
#define PIPE_PART_TARGET "s_tyubu02"    /* data 0x01934500 */
#define PIPE_LENGTH1 35.0f              /* base -> elbow (0x420c0000) */
#define PIPE_LENGTH2 55.0f              /* elbow -> target (0x425c0000) */
#define PIPE_PULL_MAX 20.0f             /* the target's pull is cut to this length */
#define PIPE_PULL_GAIN 1.2f
#define PIPE_SPRING_DAMP 0.5f
#define REC_PIPE_AIM 0x2f0              /* float[4]: the point the target part pulls the elbow toward
                                           (MODEL_PARTS +0x330; named from this use) */
#define REC_MATRIX_T 0x1b0              /* float[4]: the world matrix's row 3 (translation) */

static u_char *pPipe_Target;            /* sbss 0x0193d200 (local) */
static u_char *pPipe_Joint;             /* sbss 0x0193d240 (local) */
static u_char *pPipe_Base;              /* sbss 0x0193d280 (local) */
static u_int matParent;                 /* sbss 0x0193d1c0 (local): the joint's REC_PARENT (an EE word) */
static float prevMatrix[4][4];          /* sbss 0x0193d180 (local): the joint's REC_BASE at init */
static float length1;                   /* sbss 0x0193d288 (local) */
static float length2;                   /* sbss 0x0193d284 (local) */
static float Pipe_Vel[4];               /* data 0x01938d50 (local): the elbow's spring velocity, 0 at load */
static const float PipeDown[4] = { 0.0f, -1.0f, 0.0f, 0.0f };   /* data 0x01938d60 (@2316) */
static const float PipeGravity[4] = { 0.0f, 0.5f, 0.0f, 0.0f }; /* data 0x01938d70 (@2317) */
static int Pipe_Bank = -1;              /* PORT: the bank tiPipeJointInit read, to resolve matParent */

// CT3 0x018922c0: VU0 -- dst.xyz = src.xyz / |src.xyz|, dst.w = src.w; answers the length (1 / rsqrt).
float gaNormalizeVector3XYZ(float *dst, const float *src)
{
    float q = 1.0f / sqrtf(src[0] * src[0] + src[1] * src[1] + src[2] * src[2]);

    dst[0] = src[0] * q;
    dst[1] = src[1] * q;
    dst[2] = src[2] * q;
    dst[3] = src[3];
    return 1.0f / q;
}

// PORT: matParent (an EE word) as a host matrix (FootParentMatrix's rule).
static float (*PipeParentMatrix(void))[4]
{
    if (matParent == MATUNIT_EE)
        return matUnit;
    return BANK_PTR(float (*)[4], Pipe_Bank, matParent);
}

// CT3 0x0188bf30: main2.c, when CHARA_PIPE's shape loads into `bank`: find the three parts, keep the
// joint's parent matrix word and rest pose, set the two lengths.
void tiPipeJointInit(int bank)
{
    u_char *header = pModelPartsHeader[bank];

    pPipe_Base = PARTS_REC(header, asFindModelPartsID(header, ahGetHash(PIPE_PART_BASE)));
    pPipe_Joint = PARTS_REC(header, asFindModelPartsID(header, ahGetHash(PIPE_PART_JOINT)));
    pPipe_Target = PARTS_REC(header, asFindModelPartsID(header, ahGetHash(PIPE_PART_TARGET)));
    matParent = *(u_int *)(pPipe_Joint + LEG_REC_PARENT);
    memcpy(prevMatrix, pPipe_Joint + LEG_REC_BASE, sizeof prevMatrix);
    length1 = PIPE_LENGTH1;
    length2 = PIPE_LENGTH2;
    Pipe_Bank = bank;
}

// CT3 0x0188bb60: once a frame (main), while CHARA_PIPE has both top flag bits (REFRACTION and SHADOW):
// place the elbow, rebuild the bank's tree with it, then put the joint's REC_BASE back.
void tiPipeJointUpdate(void)
{
    CHARACTER *chr = &Character[CHARA_PIPE];
    float dir[4], axis[4], elbow[4], pull[4], pullDir[4], rot[4][4], m[4][4];
    float d[4], dist, len, accel;
    int k;

    if ((chr->flags & CHARACTER_FLAG_REFRACTION) == 0 || (chr->flags & CHARACTER_FLAG_SHADOW) == 0) {
        return;
    }
    pPipe_Joint[LEG_REC_BITS] = LEG_REC_BIT_DIRTY;

    // The elbow: turn the base -> target direction about down x dir by the law of cosines' angle.
    for (k = 0; k < 3; k++)
        d[k] = LEG_F(pPipe_Target, LEG_REC_POS)[k] - LEG_F(pPipe_Base, LEG_REC_POS)[k];
    d[3] = LEG_F(pPipe_Target, LEG_REC_POS)[3]; // vsub.xyz: w stays the target's
    dist = gaNormalizeVector3XYZ(dir, d);
    // The ROM also normalises base -> joint here; the axis overwrites that store before anything reads it.
    axis[0] = PipeDown[1] * dir[2] - dir[1] * PipeDown[2];
    axis[1] = PipeDown[2] * dir[0] - dir[2] * PipeDown[0];
    axis[2] = PipeDown[0] * dir[1] - dir[0] * PipeDown[1];
    axis[3] = 0.0f;
    asMakeRotMatrixByQuartanion(rot, axis,
                                ahAcos(((dist * dist + length1 * length1) - length2 * length2) /
                                       (dist * 2.0f * length1)));
    dir[3] = 0.0f;
    for (k = 0; k < 4; k++)
        elbow[k] = dir[0] * rot[0][k] + dir[1] * rot[1][k] + dir[2] * rot[2][k] + dir[3] * rot[3][k];
    for (k = 0; k < 3; k++)
        elbow[k] = elbow[k] * length1 + LEG_F(pPipe_Base, LEG_REC_POS)[k];
    elbow[3] = 1.0f;

    // The swing: the target's pull (cut to PIPE_PULL_MAX) drives a damped spring that sags by PipeGravity.
    for (k = 0; k < 3; k++)
        pull[k] = LEG_F(pPipe_Target, REC_PIPE_AIM)[k] - LEG_F(pPipe_Target, LEG_REC_POS)[k];
    pull[3] = LEG_F(pPipe_Target, REC_PIPE_AIM)[3];
    len = gaNormalizeVector3XYZ(pullDir, pull);
    if (!(len <= PIPE_PULL_MAX)) {
        for (k = 0; k < 3; k++)
            pull[k] = pullDir[k] * PIPE_PULL_MAX;
    }
    for (k = 0; k < 3; k++) {
        accel = (pull[k] * PIPE_PULL_GAIN + ((pull[k] - Pipe_Vel[k]) - PipeGravity[k])) * PIPE_SPRING_DAMP;
        Pipe_Vel[k] += accel;
        elbow[k] += Pipe_Vel[k];
    }

    // The joint's world matrix with the elbow as its translation (its row 3 cleared first), carried back
    // through the inverse offset and parent into REC_BASE.
    LEG_F(pPipe_Joint, REC_MATRIX_T)[0] = 0.0f;
    LEG_F(pPipe_Joint, REC_MATRIX_T)[1] = 0.0f;
    LEG_F(pPipe_Joint, REC_MATRIX_T)[2] = 0.0f;
    LEG_F(pPipe_Joint, REC_MATRIX_T)[3] = 1.0f;
    memcpy(m, LEG_M(pPipe_Joint, LEG_REC_MATRIX), sizeof(float[3][4]));
    for (k = 0; k < 3; k++)
        m[3][k] = LEG_F(pPipe_Joint, REC_MATRIX_T)[k] + elbow[k];
    m[3][3] = LEG_F(pPipe_Joint, REC_MATRIX_T)[3];
    gaInversMatrix(rot, LEG_M(pPipe_Joint, LEG_REC_OFFSET));
    gaMulMatrix(m, m, rot);
    gaInversMatrix(rot, PipeParentMatrix());
    gaMulMatrix(LEG_M(pPipe_Joint, LEG_REC_BASE), rot, m);
    BuildMatrixTree(chr->bank, 2, 0);
    memcpy(LEG_M(pPipe_Joint, LEG_REC_BASE), prevMatrix, sizeof prevMatrix);
}

// CT3 0x0188d6e0: ssGrpPortalCheck's trigger: start a plain cross-fade out lasting `time` seconds.
void tiXFadeOut_Start(float time)
{
    fXFade = 1;
    XfadeStartTicks = 0;
    XfadeEndTicks = (int)(time * 30.0f);
    XFade_Type = XFADE_TYPE_NONE;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x01893850
void tiBumpSTValueUpdate(void)
{
    ALYSSAPAN_STUB("tiBumpSTValueUpdate", 0x01893850);
}

// stub of CT3 0x0188df70
void tiDrawLensFlares(void)
{
    ALYSSAPAN_STUB("tiDrawLensFlares", 0x0188df70);
}

// stub of CT3 0x0188ced0
void tiDrawShadingOff(void)
{
    ALYSSAPAN_STUB("tiDrawShadingOff", 0x0188ced0);
}

// stub of CT3 0x0188a610
void tiLightBelt2Update(void)
{
    ALYSSAPAN_STUB("tiLightBelt2Update", 0x0188a610);
}

// stub of CT3 0x01889f70
void tiLightBelt2UpdateForALYSSA_Battle(void)
{
    ALYSSAPAN_STUB("tiLightBelt2UpdateForALYSSA_Battle", 0x01889f70);
}

// stub of CT3 0x0188a1f0
void tiLightBelt2UpdateForBOSS_Battle(void)
{
    ALYSSAPAN_STUB("tiLightBelt2UpdateForBOSS_Battle", 0x0188a1f0);
}

// stub of CT3 0x01894230
void tiRefractionMapUpdate(void)
{
    ALYSSAPAN_STUB("tiRefractionMapUpdate", 0x01894230);
}

// stub of CT3 0x0188d3f0
void tiXFadeOut_Draw(void)
{
    ALYSSAPAN_STUB("tiXFadeOut_Draw", 0x0188d3f0);
}

// stub of CT3 0x01893cf0
void ExportDrawToTex(void)
{
    ALYSSAPAN_STUB("ExportDrawToTex", 0x01893cf0);
}

// stub of CT3 0x01889df0
void tiFreeBossData(void)
{
    ALYSSAPAN_STUB("tiFreeBossData", 0x01889df0);
}

// stub of CT3 0x0188dca0
void tiChara_Translucence_End(float alpha, int fbuf)
{
    ALYSSAPAN_STUB("tiChara_Translucence_End", 0x0188dca0);
}

// stub of CT3 0x0188d720
void tiChara_Translucence_End2(int fbuf, int chara)
{
    ALYSSAPAN_STUB("tiChara_Translucence_End2", 0x0188d720);
}

// stub of CT3 0x0188dd40
void tiChara_Translucence_Start(void)
{
    ALYSSAPAN_STUB("tiChara_Translucence_Start", 0x0188dd40);
}

// stub of CT3 0x0188dbd0
void tiChara_Translucence_Start2(void)
{
    ALYSSAPAN_STUB("tiChara_Translucence_Start2", 0x0188dbd0);
}

// stub of CT3 0x0188e4a0
void tiLensFlaresInit(int scene)
{
    ALYSSAPAN_STUB("tiLensFlaresInit", 0x0188e4a0);
}

// stub of CT3 0x01890350
void tiParticleInit(int scene)
{
    ALYSSAPAN_STUB("tiParticleInit", 0x01890350);
}

// stub of CT3 0x018949c0
void tiGetTransRotMatrix(float (*dst)[4], float *pos, float (*src)[4])
{
    ALYSSAPAN_STUB("tiGetTransRotMatrix", 0x018949c0);
}

// stub of CT3 0x0188d5e0
void tiXFadeOut_Start2(float time, u_char kind)
{
    ALYSSAPAN_STUB("tiXFadeOut_Start2", 0x0188d5e0);
}

// stub of CT3 0x0188fba0
void tiGenerateParticle(int ticks, PARTICLE2_ENTRY *ent, int enabled, int bank)
{
    (void)ticks;
    (void)ent;
    (void)enabled;
    (void)bank;
    ALYSSAPAN_STUB("tiGenerateParticle", 0x0188fba0);
}

// stub of CT3 0x0188f2b0
void tiCalc_Orbit(int ticks, PARTICLE2_ENTRY *ent, int after, CAMERA *cam, int bank)
{
    (void)ticks;
    (void)ent;
    (void)after;
    (void)cam;
    (void)bank;
    ALYSSAPAN_STUB("tiCalc_Orbit", 0x0188f2b0);
}

// stub of CT3 0x01890c00
void tiHurikoReset(int bank, HURIKO *h)
{
    (void)bank;
    (void)h;
    ALYSSAPAN_STUB("tiHurikoReset", 0x01890c00);
}

// stub of CT3 0x01892310
void tiCalc_HurikoOPEN(HURIKO *h, int bank, int ticks)
{
    (void)h;
    (void)bank;
    (void)ticks;
    ALYSSAPAN_STUB("tiCalc_HurikoOPEN", 0x01892310);
}

// stub of CT3 0x018915a0
void tiCalc_HurikoCLOSED(HURIKO *h, int bank, int ticks)
{
    (void)h;
    (void)bank;
    (void)ticks;
    ALYSSAPAN_STUB("tiCalc_HurikoCLOSED", 0x018915a0);
}

// stub of CT3 0x018912b0
void tiGetNewNormalData(HURIKO *h)
{
    (void)h;
    ALYSSAPAN_STUB("tiGetNewNormalData", 0x018912b0);
}
