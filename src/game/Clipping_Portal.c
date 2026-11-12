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
#include "game/ahalloc.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/ssSetTexUVforCubeMap.h"
#include "game/tiXcmd_TransItem.h"
#include "game/unassigned_data.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaInitDrawObject.h"
#include "ga/gaSinCos.h"
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

// The ga draw object's blocks SetSTValueForRefraction and tiCalc_Leaf_WIND read (alyssapan_drawga.h AlyssaPanGaObject has
// them: ct3_types.h's DRAW_OBJ_PACKET_QWC / DRAW_OBJ_PACKET name the same +0x48 / +0x68 words otherwise).
#define REFR_OBJ_VERTEX_COUNT 0x48      /* int */
#define REFR_OBJ_POSITIONS 0x60         /* EE word: vertexCount x (x y z w) */
#define REFR_OBJ_NORMALS 0x64           /* EE word: a qword of VIF codes, then vertexCount qwords */
#define REFR_OBJ_ST 0x68                /* EE word: a qword of VIF codes, then vertexCount x (s t) */
#define REFR_BLOCK_VIF_BYTES 0x10

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

// ---- emission (slice 10, 2026-11-11) ----

// CT3 0x0188fa40
// The ROM walks from a random vertex (tnRand(count - 1)) to the first one not yet taken; with every
// vertex taken it clears the flags first. The matrix is matUnit with the vertex, moved into the part's
// frame, added to its row 3 (so w is 1 + the moved vertex's w, as the ROM's vadd leaves it).
void tiGetmatL2WFromMeshEmitter(float (*dst)[4], PARTICLE2_EMITTER *em, int bank)
{
    u_char *used = BANK_PTR(u_char *, bank, em->usedFlags);
    float (*verts)[4] = BANK_PTR(float (*)[4], bank, em->verts);
    float (*m)[4] = BANK_PTR(float (*)[4], bank, em->matrix);
    int n = em->count;
    int start, i, v = 0;
    float p[4];

    if (em->used == n) {
        for (i = 0; i < n; i++) {
            used[i] = 0;
        }
        em->used = 0;
    }
    start = tnRand(n - 1);
    for (i = 0; i < n; i++) {
        v = (start + i) % n;
        if (used[v] == 0) {
            used[v] = 1;
            em->used++;
            break;
        }
    }
    // ROM: with no free vertex the index register keeps a stale value (cannot happen once the flags
    // are cleared above); the port takes the last one walked.
    PortApplyMatrix(p, m, verts[v]);
    memcpy(dst, matUnit, sizeof(float[4][4]));
    for (i = 0; i < 4; i++) {
        dst[3][i] += p[i];
    }
}

// CT3 0x0188fba0: emit new records into the entry's ring (tiParticleUpdate, after tiCalc_LifeSpan).
// emitRate * (ticks + idleTicks) / 5 records are due; while `enabled` (ssTrigCheckEnable of the entry's
// part) they start, up to a full ring: at the part's matrix200, or at an emitter vertex turned by the
// part's rotation, with dir turned by that rotation (plus the jitter), the entry's accel, black at
// full alpha, the entry's life and refractive index. With none due the ticks add up, else they restart
// from 0; due records not emitted (disabled, or a full ring) are dropped, as in the ROM.
void tiGenerateParticle(int ticks, PARTICLE2_ENTRY *ent, int enabled, int bank)
{
    float (*part)[4] = BANK_PTR(float (*)[4], bank, ent->partMatrix);
    PARTICLE2 *ring = BANK_PTR(PARTICLE2 *, bank, ent->ring);
    PARTICLE2_EMITTER *em = BANK_PTR(PARTICLE2_EMITTER *, bank, ent->emitter);
    u_int ringSize = ent->ringSize;
    int due, i;

    if (ent->idleTicks < 0) {
        ent->idleTicks = 0;
        return;
    }
    due = (int)(ent->emitRate * (float)(ticks + ent->idleTicks) / PARTICLE_EMIT_DIV);
    if (enabled != 0 && part != NULL && ring != NULL) {
        float rot[4][4];
        u_int tail = ent->tail;

        // the part's rotation: rows 0..2 of its matrix, row 3 (0, 0, 0, 1)
        memcpy(rot, part, sizeof(rot));
        rot[3][0] = rot[3][1] = rot[3][2] = 0.0f;
        rot[3][3] = 1.0f;
        for (i = 0; i < due && ent->count != ringSize; i++) {
            float local[4][4];
            float dir[4];
            int slot = (int)((tail + (u_int)i) % ringSize);
            PARTICLE2 *rec = &ring[slot];
            PARTICLE2_MATS *mats = BANK_PTR(PARTICLE2_MATS *, bank, rec->mats);
            u_char *obj = BANK_PTR(u_char *, bank, rec->obj);

            if (em == NULL) {
                memcpy(local, part, sizeof(local));
            } else {
                tiGetmatL2WFromMeshEmitter(local, em, bank);
                gaMulMatrix(local, local, rot);
            }
            if (ent->chara != -1) {
                float before[4];

                memcpy(before, &Character[(int)ent->chara].posBefore, sizeof(before));
                local[3][0] = before[0];
                local[3][2] = before[2];
            }
            memcpy(mats->local, local, sizeof(local));
            memcpy(rec->startPos, local[3], sizeof(rec->startPos));
            memcpy(dir, ent->dir, sizeof(dir));
            if (ent->dirJitter != 0) {
                float len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
                int k;

                for (k = 0; k < 3; k++) {
                    int r = tnRand(ent->dirJitter * 2) - ent->dirJitter;
                    dir[k] += len * (float)r / PARTICLE_JITTER_PERCENT;
                }
            }
            PortApplyMatrix(rec->vel, rot, dir);
            memcpy(rec->accel, ent->accel, sizeof(rec->accel));
            rec->color[0] = rec->color[1] = rec->color[2] = 0.0f;
            rec->color[3] = 1.0f;
            rec->life = (int)((float)ent->lifeTenths * 0.1f * PARTICLE_LIFE_TICKS);
            rec->age = 0;
            if (ent->scaleJitter == 0) {
                rec->scale[0] = rec->scale[1] = rec->scale[2] = 1.0f;
            } else {
                int r = tnRand(ent->scaleJitter * 2) - ent->scaleJitter;
                float sc = (float)r / PARTICLE_JITTER_PERCENT + 1.0f;

                rec->scale[0] = rec->scale[1] = rec->scale[2] = sc;
            }
            rec->refrIndex = ent->refrIndex;
            if (obj != NULL) {
                memset(obj + DRAW_OBJ_W78, 0, 8); // the draw object's words 0x78 and 0x7c
            }
            rec->sizeCache = 0;
            rec->refrCache = 0;
            rec->alphaCache = 0;
            rec->uvCache = 0;
            rec->vrgbCache = 0;
            ent->count++;
            ent->tail = (u_short)((slot + 1) % (int)ringSize);
        }
    }
    if (due == 0) {
        ent->idleTicks += ticks;
    } else {
        ent->idleTicks = 0;
    }
}

// ---- the records' motions (slice 8, 2026-11-11) ----
// Written from the disassembly (VU0 macro code: every vector operation is .xyz, so a velocity's and a
// matrix row's w stay as they are). dt = ticks / 300. The integration is the ROM's: the step is
// v * dt + a * dt^2 and the velocity then gains a * 2dt (not a * dt) -- kept.

static PARTICLE2_MATS *RecMats(PARTICLE2 *rec, int bank)
{
    return BANK_PTR(PARTICLE2_MATS *, bank, rec->mats);
}

// The step every accelerating motion shares: local's row 3 moves by vel * dt + accel * dt^2 and the
// velocity gains accel * 2dt.
static void OrbitStep(PARTICLE2 *rec, PARTICLE2_MATS *m, const float *accel, float dt)
{
    float step[3];
    int i;

    for (i = 0; i < 3; i++) {
        step[i] = rec->vel[i] * dt + accel[i] * (dt * dt);
        rec->vel[i] = rec->vel[i] + accel[i] * (dt * 2.0f);
    }
    for (i = 0; i < 3; i++) {
        m->local[3][i] += step[i];
    }
}

// The unit vector from local's position toward goal, and the distance (VRSQRT; the distance is 1 / Q
// through VDIV, as the ROM takes it).
static float OrbitGoalDir(const PARTICLE2_MATS *m, const float *goal, float *dir)
{
    float d[3], q;
    int i;

    for (i = 0; i < 3; i++) {
        d[i] = goal[i] - m->local[3][i];
    }
    q = 1.0f / sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    for (i = 0; i < 3; i++) {
        dir[i] = d[i] * q;
    }
    return 1.0f / q;
}

// CT3 0x0188f1c0: ORBIT_TRANS: velocity plus the record's own acceleration.
void tiCalc_Trans(int ticks, PARTICLE2 *rec, int bank)
{
    PARTICLE2_MATS *m = RecMats(rec, bank);
    float dt = (float)ticks / ORBIT_TICKS_PER_SEC;

    rec->age += ticks;
    OrbitStep(rec, m, rec->accel, dt);
}

// CT3 0x0188eeb0: the goal motions while the goal part is off: coast at the current velocity.
void tiCalc_Goal_Disable(int ticks, PARTICLE2 *rec, int bank)
{
    PARTICLE2_MATS *m = RecMats(rec, bank);
    float dt;
    int i;

    rec->age += ticks;
    dt = (float)ticks / ORBIT_TICKS_PER_SEC;
    for (i = 0; i < 3; i++) {
        m->local[3][i] += rec->vel[i] * dt;
    }
}

// CT3 0x0188f090: ORBIT_TO_GOAL: accelerate toward the goal at accel[0], whatever the distance.
void tiCalc_TransToGoal(int ticks, PARTICLE2 *rec, float *goal, int bank)
{
    PARTICLE2_MATS *m = RecMats(rec, bank);
    float dt, dir[3], accel[3];
    int i;

    rec->age += ticks;
    dt = (float)ticks / ORBIT_TICKS_PER_SEC;
    OrbitGoalDir(m, goal, dir);
    for (i = 0; i < 3; i++) {
        accel[i] = dir[i] * rec->accel[0];
    }
    OrbitStep(rec, m, accel, dt);
}

// CT3 0x0188ef30: ORBIT_TO_GOAL_STOP: the same, but only while farther than accel[1] from the goal;
// within it the record stands still (its age still counts).
void tiCalc_TransToGoal_Stop(int ticks, PARTICLE2 *rec, float *goal, int bank)
{
    PARTICLE2_MATS *m = RecMats(rec, bank);
    float dt, dir[3], accel[3], dist;
    int i;

    rec->age += ticks;
    dt = (float)ticks / ORBIT_TICKS_PER_SEC;
    dist = OrbitGoalDir(m, goal, dir);
    if (dist <= rec->accel[1]) {
        return;
    }
    for (i = 0; i < 3; i++) {
        accel[i] = dir[i] * rec->accel[0];
    }
    OrbitStep(rec, m, accel, dt);
}

// CT3 0x0188e7c0: rebuild the record's view matrices from local: rotView = local's rotation (row 3 cut
// to (0, 0, 0, 1)) then the view; localView = the scale (PARTICLE2.scale xyz on the diagonal), then
// that rotation, local's position, then the view.
void tiCalc_matL2V(PARTICLE2 *rec, float (*view)[4], int bank)
{
    PARTICLE2_MATS *m = RecMats(rec, bank);
    float rot[4][4];
    float sc[4][4] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    };
    int i;

    memcpy(rot, m->local, sizeof(rot));
    rot[3][0] = 0.0f;
    rot[3][1] = 0.0f;
    rot[3][2] = 0.0f;
    rot[3][3] = 1.0f;
    gaMulMatrix(m->rotView, view, rot);
    sc[0][0] = rec->scale[0];
    sc[1][1] = rec->scale[1];
    sc[2][2] = rec->scale[2];
    gaMulMatrix(sc, sc, rot);
    for (i = 0; i < 3; i++) {
        sc[3][i] += m->local[3][i];
    }
    gaMulMatrix(m->localView, view, sc);
}

// CT3 0x0188e740: the eye in the record's frame (eye * local^-1, all four lanes) for the refraction's
// texture coordinates.
void tiCalc_Refraction(PARTICLE2 *rec, float *eye, int bank)
{
    PARTICLE2_MATS *m = RecMats(rec, bank);
    float inv[4][4];
    float local[4];
    int i;

    gaInversMatrix(inv, m->local);
    for (i = 0; i < 4; i++) {
        local[i] = inv[0][i] * eye[0] + inv[1][i] * eye[1] + inv[2][i] * eye[2] + inv[3][i] * eye[3];
    }
    SetSTValueForRefraction(BANK_PTR(u_char *, bank, rec->obj), local, m->localView, rec->refrIndex);
}

// The ring's live records in order from the head (tiCalc_Orbit's loops).
#define FOR_EACH_LIVE(i, rec, ent, ring)                                                            \
    for ((i) = 0; (i) < (int)(ent)->count                                                           \
                  && ((rec) = &(ring)[(int)((ent)->head + (i)) % (int)(ent)->ringSize], 1); (i)++)

// CT3 0x0188f2b0: a particle entry's frame: read each curve the entry has for every live record (size,
// refractive index, colour, alpha, UV, each pass after ssParticleCashClear), move the records by the
// entry's motion and rebuild their view matrices; the after-list (after != 0) also takes the eye for
// the refraction.
void tiCalc_Orbit(int ticks, PARTICLE2_ENTRY *ent, int after, CAMERA *cam, int bank)
{
    PARTICLE2 *ring = BANK_PTR(PARTICLE2 *, bank, ent->ring);
    void *sizeCurve = BANK_PTR(void *, bank, ent->sizeCurve);
    void *refrCurve = BANK_PTR(void *, bank, ent->refrCurve);
    void *vrgbCurve = BANK_PTR(void *, bank, ent->vrgbCurve);
    void *alphaCurve = BANK_PTR(void *, bank, ent->alphaCurve);
    void *uvCurve = BANK_PTR(void *, bank, ent->uvCurve);
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    PARTICLE2 *rec;
    int i;

    if (sizeCurve != NULL) {
        ssParticleCashClear();
        FOR_EACH_LIVE(i, rec, ent, ring) {
            tiParticleGetSizeValue(rec->age, rec->scale, sizeCurve, &rec->sizeCache);
        }
    }
    if (refrCurve != NULL) {
        ssParticleCashClear();
        FOR_EACH_LIVE(i, rec, ent, ring) {
            tiParticleGetRefractive_IndexValue(rec->age, &rec->refrIndex, refrCurve, &rec->refrCache);
        }
    }
    if (vrgbCurve != NULL) {
        ssParticleCashClear();
        FOR_EACH_LIVE(i, rec, ent, ring) {
            s_char r, g, b;

            ssParticleGetVRGBValue(rec->age, &r, &g, &b, vrgbCurve, &rec->vrgbCache);
            rec->color[0] = (float)r;
            rec->color[1] = (float)g;
            rec->color[2] = (float)b;
        }
    }
    if (alphaCurve != NULL) {
        ssParticleCashClear();
        FOR_EACH_LIVE(i, rec, ent, ring) {
            short alpha;

            ssParticleGetAlphaValue(rec->age, &alpha, alphaCurve, &rec->alphaCache);
            rec->color[3] = (float)alpha / ORBIT_ALPHA_SCALE;
        }
    }
    if (uvCurve != NULL) {
        ssParticleCashClear();
        FOR_EACH_LIVE(i, rec, ent, ring) {
            u_char *obj = BANK_PTR(u_char *, bank, rec->obj);

            ssParticleGetUVValue(rec->age, obj + DRAW_OBJ_UV_U, obj + DRAW_OBJ_UV_V, uvCurve, &rec->uvCache);
        }
    }

    switch (ent->orbit) {
    case ORBIT_TRANS:
        FOR_EACH_LIVE(i, rec, ent, ring) {
            tiCalc_Trans(ticks, rec, bank);
            tiCalc_matL2V(rec, cam->view, bank);
        }
        break;
    case ORBIT_TO_GOAL:
    case ORBIT_TO_GOAL_STOP:
        // The goal part's flags are read once per frame (the ROM has the two cases apart, alike but
        // for the motion).
        if ((parts[ent->part2].flags & ORBIT_GOAL_PART_OFF) == 0) {
            float *goal = BANK_PTR(float *, bank, ent->partPos);

            FOR_EACH_LIVE(i, rec, ent, ring) {
                if (ent->orbit == ORBIT_TO_GOAL) {
                    tiCalc_TransToGoal(ticks, rec, goal, bank);
                } else {
                    tiCalc_TransToGoal_Stop(ticks, rec, goal, bank);
                }
                tiCalc_matL2V(rec, cam->view, bank);
            }
        } else {
            FOR_EACH_LIVE(i, rec, ent, ring) {
                tiCalc_Goal_Disable(ticks, rec, bank);
                tiCalc_matL2V(rec, cam->view, bank);
            }
        }
        break;
    case ORBIT_LEAF:
        FOR_EACH_LIVE(i, rec, ent, ring) {
            tiCalc_Leaf(ticks, rec, ent->leafArg, bank);
            tiCalc_matL2V(rec, cam->view, bank);
        }
        break;
    case ORBIT_TRANS_WIND:
        FOR_EACH_LIVE(i, rec, ent, ring) {
            tiCalc_Trans_WIND(ticks, rec, bank);
            tiCalc_matL2V(rec, cam->view, bank);
        }
        break;
    case ORBIT_LEAF_WIND:
        FOR_EACH_LIVE(i, rec, ent, ring) {
            tiCalc_Leaf_WIND(ticks, rec, ent->leafArg, bank);
            tiCalc_matL2V(rec, cam->view, bank);
        }
        break;
    case ORBIT_TWIST:
        FOR_EACH_LIVE(i, rec, ent, ring) {
            tiCalc_Twist(ticks, rec, ent, bank);
            tiCalc_matL2V(rec, cam->view, bank);
        }
        break;
    }

    if (after != 0) {
        float eye[4];

        // The ROM hands one buffer for all four answers; the position is written last, so it is the
        // eye (kept as the ROM passes it).
        asViewMatrix2PosRUF(&cam->view[0][0], eye, eye, eye, eye);
        FOR_EACH_LIVE(i, rec, ent, ring) {
            tiCalc_Refraction(rec, eye, bank);
        }
    }
}
#undef FOR_EACH_LIVE

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

// ---- hit tests: a point against a face, a capsule, a wind (slice 12, 2026-11-11) ----

// PORT helper: |v| of the xyz lanes, v scaled to unit length in place, as the ROM's vmul.xyz /
// vaddy.x / vaddz.x / vrsqrt / vmulq.xyz, then vdiv 1 / (1 / len).
static float PortUnitXYZ(float *v)
{
    float q = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= q;
    v[1] *= q;
    v[2] *= q;
    return 1.0f / q;
}

// out.xyz = base.xyz + dir.xyz * scale, out.w = base.w (vmulx.xyz, then vadd.xyz on the base's register).
static void PortPushOut(float *out, const float *base, const float *dir, float scale)
{
    float w = base[3];

    out[0] = base[0] + dir[0] * scale;
    out[1] = base[1] + dir[1] * scale;
    out[2] = base[2] + dir[2] * scale;
    out[3] = w;
}

// The point of the segment a..b nearest p, the unit direction from it to p and the distance: t =
// (p - a) . (b - a) / |b - a|^2; before a, past b, or between (near = a + (b - a) t, w a's).
typedef enum { SEG_BEFORE_A, SEG_PAST_B, SEG_BETWEEN } SEG_SIDE;

static SEG_SIDE PortSegmentNearest(const float *p, const float *a, const float *b, float near[4], float dir[4],
                                   float *dist)
{
    float pa[4], ab[4], t;
    SEG_SIDE side;

    pa[0] = p[0] - a[0];
    pa[1] = p[1] - a[1];
    pa[2] = p[2] - a[2];
    pa[3] = p[3];
    ab[0] = b[0] - a[0];
    ab[1] = b[1] - a[1];
    ab[2] = b[2] - a[2];
    ab[3] = b[3];
    t = gaInnerProductXYZ(pa, ab) / gaInnerProductXYZ(ab, ab);
    if (t < 0.0f) {
        memcpy(near, a, sizeof(float) * 4);
        side = SEG_BEFORE_A;
    } else if (!(t <= 1.0f)) {
        memcpy(near, b, sizeof(float) * 4);
        side = SEG_PAST_B;
    } else {
        PortPushOut(near, a, ab, t);
        side = SEG_BETWEEN;
    }
    dir[0] = p[0] - near[0];
    dir[1] = p[1] - near[1];
    dir[2] = p[2] - near[2];
    dir[3] = p[3];
    *dist = PortUnitXYZ(dir);
    return side;
}

// CT3 0x01890f30: p against the hit's plane (world0 a point on it, world1 its normal): behind or on it
// (d = p . n - world0 . n <= 0), p is pushed back onto the plane along the normal (xyz). Answers that.
// (tiCalc_HurikoCLOSED, tiCalc_HurikoOPEN)
int tiCheckHit_Face(float *p, HURIKO_HIT *hit)
{
    float d = gaInnerProductXYZ(p, hit->world1) - gaInnerProductXYZ(hit->world0, hit->world1);

    if (d <= 0.0f)
        PortPushOut(p, p, hit->world1, -d);
    return d <= 0.0f;
}

// CT3 0x01890fe0: p against the capsule world0..world1 whose radius is the a record's +0x20 float: inside
// (an end: distance <= radius; the side: < radius), p is pushed out to the surface from the nearest point
// of the axis (its w taken from that point: the end's, or world0's along the side). Answers that.
// PORT: bank added (the a word is an EE word of the bank's data).
int tiCheckHit_Capsule(float *p, HURIKO_HIT *hit, int bank)
{
    float radius = BANK_PTR(const float *, bank, hit->a)[HURIKO_HIT_A_RADIUS / 4];
    float near[4], dir[4], dist;
    SEG_SIDE side = PortSegmentNearest(p, hit->world0, hit->world1, near, dir, &dist);

    if (side == SEG_BETWEEN ? !(dist < radius) : !(dist <= radius))
        return 0;
    PortPushOut(p, near, dir, radius);
    return 1;
    // callcheck: gaInnerProductXYZ (PortSegmentNearest makes it)
}

// CT3 0x0188c8b0: is p inside the wind's capsule, world30..world40 of the wind's radius (an end: distance
// <= radius; the side: < radius)? (tiCalc_Leaf_WIND, tiCalc_Trans_WIND, the pendulums)
int tiCheckWindForce(float *p, WIND *wind)
{
    float near[4], dir[4], dist;
    SEG_SIDE side = PortSegmentNearest(p, wind->world30, wind->world40, near, dir, &dist);

    return side == SEG_BETWEEN ? dist < wind->radius : dist <= wind->radius;
    // callcheck: gaInnerProductXYZ (PortSegmentNearest makes it)
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

// A joint's vertex copy n: posRef[n] (or normalRef[n]) -> the EE word of the qword.
static float *HurikoRef(int bank, u_int refEE, int n)
{
    return BANK_PTR(float *, bank, BANK_PTR(u_int *, bank, refEE)[n]);
}

// The joint at chain c, row k of a pendulum's joint table.
static HURIKO_JOINT *HurikoJointAt(int bank, const HURIKO *h, int c, int k)
{
    return BANK_PTR(HURIKO_JOINT *, bank, BANK_PTR(u_int *, bank, h->jointTable)[k + c * h->joints]);
}

// CT3 0x01890c00: tiHurikoUpdate, on the first enabled frame after a pendulum was off -- hang every joint
// back in its rest pose (below the joint above by its length, plus its rest offset, in the part's
// matrix200 frame), keep the world positions in the joints, and leave the vertices part-local again.
// The roots take back the local position tiHurikoInit kept.
void tiHurikoReset(int bank, HURIKO *h)
{
    static const float down[4] = { 0.0f, HURIKO_REST_DOWN_Y, 0.0f, 0.0f }; /* @1109 (0x01938a20) */
    MODEL_PARTS *part = HurikoPart(bank, HurikoWord0(h));
    float (*m)[4] = part->matrix200;
    int nc = h->chains;
    int nj = h->joints;
    float inv[4][4];
    float t[4];
    int c, k, n;

    gaInversMatrix(inv, m);
    // every master vertex into the part's frame (w too)
    for (n = 0; n < nc * nj; n++) {
        float *p = HurikoRef(bank, HurikoJointAt(bank, h, n / nj, n % nj)->posRef, 0);

        PortApplyMatrix(t, m, p);
        memcpy(p, t, sizeof t);
    }
    // the rest pose, from the root down (the joint above is already placed)
    for (c = 0; c < nc; c++) {
        for (k = 1; k < nj; k++) {
            HURIKO_JOINT *j = HurikoJointAt(bank, h, c, k);
            const float *up = BANK_PTR(const float *, bank, j->up);
            float r[4];
            float *p = HurikoRef(bank, j->posRef, 0);
            int e;

            PortApplyMatrix(r, m, j->restDir);
            for (e = 0; e < 4; e++) {
                p[e] = (down[e] * j->length + up[e]) + r[e];
            }
        }
    }
    // keep the world positions, put the vertices (and their copies) back into part space
    for (c = 0; c < nc; c++) {
        HURIKO_JOINT *root = HurikoJointAt(bank, h, c, 0);

        memcpy(HurikoRef(bank, root->posRef, 0), root->pos, sizeof root->pos);
        for (k = 1; k < nj; k++) {
            HURIKO_JOINT *j = HurikoJointAt(bank, h, c, k);
            float *p = HurikoRef(bank, j->posRef, 0);

            memcpy(j->pos, p, sizeof j->pos);
            PortApplyMatrix(t, inv, p);
            memcpy(p, t, sizeof t);
            for (n = 1; n < j->copies; n++) {
                memcpy(HurikoRef(bank, j->posRef, n), p, sizeof t);
            }
            j->swing40[0] = j->swing40[1] = j->swing40[2] = 0.0f; // sqc2 vf0: (0, 0, 0, 1)
            j->swing40[3] = 1.0f;
            j->swing50[0] = j->swing50[1] = j->swing50[2] = 0.0f;
            j->swing50[3] = 1.0f;
        }
    }
}

// One joint's normal: (a - p) x (b - p) through vopmula / vopmsub, normalised, w 0, copied to every copy
// of the vertex.
static void HurikoNormal(int bank, const HURIKO_JOINT *j, const float *a, const float *b)
{
    const float *p = HurikoRef(bank, j->posRef, 0);
    float *nrm = HurikoRef(bank, j->normalRef, 0);
    float u[3], v[3];
    int n;

    u[0] = a[0] - p[0];
    u[1] = a[1] - p[1];
    u[2] = a[2] - p[2];
    v[0] = b[0] - p[0];
    v[1] = b[1] - p[1];
    v[2] = b[2] - p[2];
    nrm[0] = u[1] * v[2] - v[1] * u[2];
    nrm[1] = u[2] * v[0] - v[2] * u[0];
    nrm[2] = u[0] * v[1] - v[0] * u[1];
    nrm[3] = 0.0f; // PORT: the ROM normalises the register's stale w, then stores 0 over it
    gaNormalizeVectorXYZ(nrm, nrm);
    nrm[3] = 0.0f;
    for (n = 1; n < j->copies; n++) {
        memcpy(HurikoRef(bank, j->normalRef, n), nrm, 4 * sizeof(float));
    }
}

// CT3 0x018912b0: tiHurikoUpdate, after each swing -- every non-root joint's vertex normal from its
// neighbours: the joint above it (the chain's root first) and the next chain's joint one row up. The last
// chain looks back instead: at chain nc - 2 (nc - 3 when CLOSED) and at its own joint above (chain 0's
// when CLOSED), as the ROM indexes them.
void tiGetNewNormalData(HURIKO *h)
{
    int bank = BankOfHostPtr(h); // PORT: the ROM's words are absolute EE addresses; the host asks the bank
    int nc = h->chains;
    int nj = h->joints;
    int closed = (h->flags & HURIKO_FLAG_CLOSED) != 0;
    int c, k;

    for (c = 0; c < nc - 1; c++) {
        const float *above = HurikoRef(bank, HurikoJointAt(bank, h, c, 0)->posRef, 0);

        for (k = 1; k < nj; k++) {
            HURIKO_JOINT *j = HurikoJointAt(bank, h, c, k);

            HurikoNormal(bank, j, above, HurikoRef(bank, HurikoJointAt(bank, h, c + 1, k - 1)->posRef, 0));
            above = HurikoRef(bank, j->posRef, 0);
        }
    }
    for (k = 1; k < nj; k++) {
        HURIKO_JOINT *j = HurikoJointAt(bank, h, nc - 1, k);

        HurikoNormal(bank, j, HurikoRef(bank, HurikoJointAt(bank, h, (nc - 2) - closed, k - 1)->posRef, 0),
                     HurikoRef(bank, HurikoJointAt(bank, h, (nc - 1) * (1 - closed), k - 1)->posRef, 0));
    }
    // callcheck: gaNormalizeVectorXYZ (HurikoNormal makes it)
}

// ---- characters ----

// CT3 0x018948e0: a character's facing: its turn (CHARACTER.rot) applied to +z (@123 = (0, 0, 1, 0)).
// The bosses' brains aim with it.
void tiGetCharaDirection(float *dir, const CHARACTER *chr)
{
    static const float ahead[4] = { 0.0f, 0.0f, 1.0f, 0.0f }; /* @123 (0x019389e0) */
    float m[4][4];

    gaSetRotMatrixY(m, chr->rot);
    PortApplyMatrix(dir, m, ahead);
}

// ---- boss data ----

// The boss info's allocations (pBossInfo, from gHeapJunc; tiFreeBossData frees them). Two chain sets of
// four blocks each (set A while BOSS_INFO_CHAIN_A_COUNT != 0, set B while its count at 0x91 != 0), one
// block while the int at 0xc4 is set, and a sub-block at 0x140 holding two more.
#define BOSS_INFO_CHAIN_A_E8 0xe8       /* set A: EE word of a block */
#define BOSS_INFO_CHAIN_A_FC 0xfc
#define BOSS_INFO_CHAIN_A_104 0x104
#define BOSS_INFO_CHAIN_B_COUNT 0x91    /* u_char: set B's chains (= BOSS_INFO_CHAIN_COUNT) */
#define BOSS_INFO_CHAIN_B_EC 0xec       /* set B: EE word of a block */
#define BOSS_INFO_CHAIN_B_100 0x100     /* = BOSS_INFO_CHAIN_POS */
#define BOSS_INFO_CHAIN_B_108 0x108
#define BOSS_INFO_CHAIN_B_ON 0x110      /* = BOSS_INFO_CHAIN_ON */
#define BOSS_INFO_C4 0xc4               /* int: != 0 -> the block at 0x13c is allocated */
#define BOSS_INFO_13C 0x13c
#define BOSS_INFO_SUB 0x140             /* EE word -> a block whose words 0x24 and 0x28 are blocks too */
#define BOSS_SUB_24 0x24
#define BOSS_SUB_28 0x28

// The trigger variables a boss battle leaves set; tiFreeBossData puts each back to -1 (global), in the
// ROM's order.
static const int BossTrigVars[] = { 0x53, 0x54, 0x52, 0x56, 5 };

// _SunFree on the boss info's pointer word at `off` (of `base`). PORT: pBossInfo's pointer words are EE
// words with no host form until the boss TU that allocates them decides it (as BossInfoPtr in
// wideXcmd_DebugPrint.c), so nothing is allocated through them yet and nothing is freed here.
static void BossInfoFree(u_char *base, int off)
{
    (void)base;
    (void)off;
}

// The chain sets' light belts (the boss battle). A set's trigger variable at 2 clears its chains'
// events; at 3 every chain holding (BOSS_INFO_CHAIN_x_ON[i] == 1) registers its event; then each chain
// whose event is set copies the set's source position (EE word 0xf4 / 0xf8) into its belt end (the EE
// word table at 0x104 / 0x108, entry i). PORT: those words have no host form (BossInfoPtr in
// wideXcmd_DebugPrint.c, open-items.md 1 as): BossInfoWord answers none, so only the events run.
#define TRIG_VAR_CHAIN_A 0x54           /* global: set A's state (CHAIN_STATE_*) */
#define TRIG_VAR_CHAIN_B 0x53           /* global: set B's (the chains on Alyssa) */
#define CHAIN_STATE_DROP 2              /* the set's events are cleared */
#define CHAIN_STATE_HOLD 3              /* the holding chains' events are registered */
#define BOSS_INFO_CHAIN_A_SRC 0xf4      /* EE word -> float[4]: set A's belt source */
#define BOSS_INFO_CHAIN_B_SRC 0xf8
#define BOSS_INFO_CHAIN_A_ENDS 0x104    /* EE word -> EE words[count]: each chain's belt end (float[4]) */
#define BOSS_INFO_CHAIN_B_ENDS 0x108
#define CHAIN_ON 1

static void *BossInfoWord(int off)
{
    (void)off;
    return NULL;
}

static void ChainBelts(int var, int countOff, int onOff, int srcOff, int endsOff, int firstEvent, int max)
{
    int count = pBossInfo[countOff];
    const u_char *on = (const u_char *)BossInfoWord(onOff);
    const float *src = (const float *)BossInfoWord(srcOff);
    float **ends = (float **)BossInfoWord(endsOff);
    int state = ssTrigGetVariable(var, XCMD_SCOPE_GLOBAL, 0);
    int i;

    if (state == CHAIN_STATE_DROP) {
        for (i = 0; i < max; i++) {
            ssTrigRemoveEvent(firstEvent + i, XCMD_SCOPE_GLOBAL, 0);
        }
    } else if (state == CHAIN_STATE_HOLD && on != NULL) {
        // the ROM registers only chains 0..max-1 (its switch has no other case)
        for (i = 0; i < count; i++) {
            if (on[i] == CHAIN_ON && i < max) {
                ssTrigRegisterEvent(firstEvent + i, XCMD_SCOPE_GLOBAL, 0);
            }
        }
    }
    for (i = 0; i < max; i++) {
        if (ssTrigCheckEvent(firstEvent + i, XCMD_SCOPE_GLOBAL, 0) != 0 && src != NULL && ends != NULL) {
            memcpy(ends[i], src, sizeof(float[4]));
        }
    }
}

// CT3 0x01889f70: main, in the chain fight -- set B (five chains, events 0x69..0x6d).
void tiLightBelt2UpdateForALYSSA_Battle(void)
{
    if (pBossInfo == NULL) {
        return; // PORT: guard; the ROM reads pBossInfo unchecked (main calls it only with a boss)
    }
    ChainBelts(TRIG_VAR_CHAIN_B, BOSS_INFO_CHAIN_B_COUNT, BOSS_INFO_CHAIN_B_ON, BOSS_INFO_CHAIN_B_SRC,
               BOSS_INFO_CHAIN_B_ENDS, TRIG_EVENT_CHAIN_B_FIRST,
               TRIG_EVENT_CHAIN_B_LAST - TRIG_EVENT_CHAIN_B_FIRST + 1);
    // callcheck: ssTrigGetVariable, ssTrigRemoveEvent, ssTrigRegisterEvent, ssTrigCheckEvent (ChainBelts makes them)
}

// CT3 0x0188a1f0: main, in the chain fight -- set A (ten chains, events 0x3c..0x45).
void tiLightBelt2UpdateForBOSS_Battle(void)
{
    if (pBossInfo == NULL) {
        return; // PORT: guard, as above
    }
    ChainBelts(TRIG_VAR_CHAIN_A, BOSS_INFO_CHAIN_A_COUNT, BOSS_INFO_CHAIN_A_ON, BOSS_INFO_CHAIN_A_SRC,
               BOSS_INFO_CHAIN_A_ENDS, TRIG_EVENT_CHAIN_A_FIRST,
               TRIG_EVENT_CHAIN_A_LAST - TRIG_EVENT_CHAIN_A_FIRST + 1);
    // callcheck: ssTrigGetVariable, ssTrigRemoveEvent, ssTrigRegisterEvent, ssTrigCheckEvent (ChainBelts makes them)
}

// CT3 0x01889df0: system_end, tiXcmd_BOSS_BattleEnd -- free the boss info's blocks and the info itself
// (gHeapJunc), then clear the battle's trigger variables.
void tiFreeBossData(void)
{
    int i;

    if (pBossInfo == NULL) {
        return;
    }
    if (pBossInfo[BOSS_INFO_CHAIN_A_COUNT] != 0) {
        BossInfoFree(pBossInfo, BOSS_INFO_CHAIN_A_E8);
        BossInfoFree(pBossInfo, BOSS_INFO_CHAIN_A_FC);
        BossInfoFree(pBossInfo, BOSS_INFO_CHAIN_A_104);
        BossInfoFree(pBossInfo, BOSS_INFO_CHAIN_A_ON);
    }
    if (pBossInfo[BOSS_INFO_CHAIN_B_COUNT] != 0) {
        BossInfoFree(pBossInfo, BOSS_INFO_CHAIN_B_EC);
        BossInfoFree(pBossInfo, BOSS_INFO_CHAIN_B_100);
        BossInfoFree(pBossInfo, BOSS_INFO_CHAIN_B_108);
        BossInfoFree(pBossInfo, BOSS_INFO_CHAIN_B_ON);
    }
    {
        int c4;

        memcpy(&c4, pBossInfo + BOSS_INFO_C4, sizeof c4);
        if (c4 != 0) {
            BossInfoFree(pBossInfo, BOSS_INFO_13C);
        }
    }
    {
        u_int sub;

        memcpy(&sub, pBossInfo + BOSS_INFO_SUB, sizeof sub);
        if (sub != 0) {
            // PORT: the ROM frees the sub-block's words 0x24 and 0x28 first (BOSS_SUB_24 / _28): no host
            // form, as BossInfoFree
            BossInfoFree(pBossInfo, BOSS_INFO_SUB);
        }
    }
    _SunFree(&gHeapJunc, (void **)&pBossInfo);
    for (i = 0; i < (int)(sizeof BossTrigVars / sizeof BossTrigVars[0]); i++) {
        ssTrigSetVariable(BossTrigVars[i], -1, 1, 0);
    }
    // callcheck: _SunFree (BossInfoFree makes the rest)
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

// The draw control tiXFadeOut_Start2 sets up for the kinds that start from black: untextured, unblended,
// no tests but ZTST 1 / AFAIL 1, ALPHA (Cs, Cd, FIX, Cd) at FIX 0 (the ROM's 22 argument words).
#define INIT_BLACK_DRAWCTRL_ARGS {                                                  0, 0, 0, 0, 0, 1, 0, 0,      /* PRMODE: FST */                                  0, 0, 0, 1, 0, 0, 0, 1,      /* TEST_1: AFAIL 1, ZTST 1 */                      0, 1, 2, 1, 0,               /* ALPHA_1: A Cs, B Cd, C FIX, D Cd, FIX 0 */      1,                           /* FBA_1 */                                        1, 2, 1, 1, 0, 0,            /* TEX1_1 */                                       0}                           /* TEXA */
#define XFADE_TYPE_FROM_BLACK(kind) ((kind) == 2 || (kind) == XFADE_TYPE_PAUSE2 || (kind) == 4)
alignas(16) static GA_DRAWCTRL_DATA init_black_drawctrl;   /* sbss 0x0193d380 (local) */

// CT3 0x0188d5e0: tiXcmd_XFadeOut2Start: start a cross-fade out of kind `kind` lasting `time` seconds;
// kinds 2..4 set up init_black_drawctrl (the cross-fade's draw reads it: not reconstructed yet).
void tiXFadeOut_Start2(float time, u_char kind)
{
    fXFade = 1;
    XfadeStartTicks = 0;
    XfadeEndTicks = (int)(time * 30.0f);
    XFade_Type = kind;
    if (XFADE_TYPE_FROM_BLACK(XFade_Type)) {
        GA_DRAWCTRL_ARGS args = INIT_BLACK_DRAWCTRL_ARGS;

        gaSetupDrawCtrlData(&init_black_drawctrl, args);
    }
}

// ---- light belts: the per-frame ribbon (slice 6) ----

#define SCREEN_CX 4                     /* screen_0[]: the screen centre x (256) */
#define SCREEN_CY 5                     /* ... y (224) */
#define SCREEN_AX 6                     /* ... the x aspect (0.832) */
#define SCREEN_AY 7                     /* ... the y aspect (0.94) */
#define SCREEN_SCRZ 8                   /* ... the projection distance (512) */

// A belt vertex's first slot (the one the ROM computes; the rest copy it).
static float *BeltVertPos(int bank, const LIGHT_BELT2 *belt, int index)
{
    u_int *verts = BANK_PTR(u_int *, bank, belt->verts);
    LIGHT_BELT2_VERT *v = BANK_PTR(LIGHT_BELT2_VERT *, bank, verts[index]);
    u_int *slots = BANK_PTR(u_int *, bank, v->slots);

    return BANK_PTR(float *, bank, slots[0]);
}

static LIGHT_BELT2_VERT *BeltVert(int bank, const LIGHT_BELT2 *belt, int index)
{
    u_int *verts = BANK_PTR(u_int *, bank, belt->verts);

    return BANK_PTR(LIGHT_BELT2_VERT *, bank, verts[index]);
}

// A view-space point on the screen: (cx + scrz ax x / z, cy - scrz ay y / z, z, 1).
static void BeltToScreen(float out[4], const float *p, float cx, float cy, float sx, float sy)
{
    out[0] = cx + sx * (p[0] / p[2]);
    out[1] = cy - sy * (p[1] / p[2]);
    out[2] = p[2];
    out[3] = 1.0f;
}

// An edge vertex: `base` (on the screen) pushed along the turned 2D direction from `from` to `to` by
// width * scrz / to's z, then back into view space at base's z.
static void BeltEdge(float *out, float (*turn)[4], const float base[4], const float from[4], const float to[4],
                     float width, float scrz, float cx, float cy, float invSx, float invSy)
{
    float d[4], side[4], e[4];
    float q, scale = scrz / to[2];
    int j;

    d[0] = to[0] - from[0];
    d[1] = to[1] - from[1];
    d[2] = 0.0f;
    d[3] = 0.0f;
    q = 1.0f / sqrtf(d[0] * d[0] + d[1] * d[1]);
    for (j = 0; j < 4; j++) {
        d[j] *= q;
    }
    PortApplyMatrix(side, turn, d);
    for (j = 0; j < 4; j++) {
        e[j] = base[j] + side[j] * (width * scale);
    }
    out[0] = invSx * e[2] * (e[0] - cx);
    out[1] = invSy * e[2] * (cy - e[1]);
    out[2] = e[2];
    out[3] = 1.0f;
}

// CT3 0x0188a610: once a frame (main), unless paused: move every drawn scene bank's light belts. A belt on
// one part trails it (its points shift down one, point 0 takes the part's position); a belt between two
// parts is a cubic Bezier from A through A + halfA, B + halfB to B. While the part's trigger is enabled the
// ribbon is rebuilt: the centre row is the points in view space, the edge rows are pushed sideways on the
// screen by each vertex's widths (scaled by depth), and every row goes back into the part's draw space.
void tiLightBelt2Update(void)
{
    float cx = screen_0[SCREEN_CX];
    float cy = screen_0[SCREEN_CY];
    float scrz = screen_0[SCREEN_SCRZ];
    float sx = scrz * screen_0[SCREEN_AX];
    float sy = scrz * screen_0[SCREEN_AY];
    float invSx, invSy;
    float turn0[4][4], turn2[4][4], inv[4][4];
    int bank;

    if (pause != 0) {
        return;
    }
    invSx = 1.0f / sx;
    invSy = 1.0f / sy;
    gaSetRotMatrixZ(turn0, LIGHT_BELT2_TURN0);
    gaSetRotMatrixZ(turn2, LIGHT_BELT2_TURN2);
    for (bank = BANK_SCENE_MAX; bank < BANK_MAX; bank++) {
        BANK_INFO *bi = &BankInfo[bank];
        u_int *list;
        int i;

        if (bi->drawOn == 0 || bi->type == BANK_TYPE_BEHAVIOR) {
            continue;
        }
        if (bi->flags & BANK_FLAG_ANIM_FORCE_STOP) {
            bank += bi->groupCount - 1;
            continue;
        }
        list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->lightBelts);
        if (list == NULL) {
            bank += bi->groupCount - 1;
            continue;
        }
        for (i = 0; list[i] != 0; i++) {
            LIGHT_BELT2 *belt = BANK_PTR(LIGHT_BELT2 *, bank, list[i]);
            MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
            MODEL_PARTS *a = &parts[belt->partA];
            float (*points)[4] = BANK_PTR(float (*)[4], bank, belt->points);
            int count = belt->count;
            int k, r;

            if (a->kind == PARTS_KIND_WIND_ON_ROOT) {
                a = &parts[0];
            }
            gaInversMatrix(inv, a->matrix340);
            if (belt->link == 0) {
                for (k = count - 1; k != 0; k--) {
                    memcpy(points[k], points[k - 1], sizeof(points[k]));
                }
                memcpy(points[0], a->pos330, sizeof(points[0]));
            } else {
                MODEL_PARTS *b = &parts[belt->partB];
                float ha[4], hb[4], ctrl[4][4];
                int j;

                a->flags43 = PARTS_FLAG43_BELT;
                a->drawOnPos[3] = 1.0f;
                b->flags43 = PARTS_FLAG43_BELT;
                PortApplyMatrix(ha, a->matrix200, belt->halfA);
                PortApplyMatrix(hb, b->matrix200, belt->halfB);
                memcpy(ctrl[0], a->pos330, sizeof(ctrl[0]));
                for (j = 0; j < 3; j++) {
                    ctrl[1][j] = ha[j] + a->pos330[j];
                    ctrl[2][j] = b->pos330[j] + hb[j];
                }
                ctrl[1][3] = ha[3];
                ctrl[2][3] = b->pos330[3];
                memcpy(ctrl[3], b->pos330, sizeof(ctrl[3]));
                for (k = 0; k < count; k++) {
                    float s = (float)k / (float)(count - 1);

                    asGetBeziePosition(1.0f - s, s, points[k], ctrl);
                    points[k][3] = 1.0f;
                }
            }
            if (!ssTrigCheckEnable((short)belt->partA, bank)) {
                continue;
            }
            for (k = 0; k < count; k++) {
                PortApplyMatrix(BeltVertPos(bank, belt, LIGHT_BELT2_ROW_CENTRE * count + k),
                                Cam[CAMERA_SHARED].view, points[k]);
            }
            {
                float prev[4], cur[4];

                BeltToScreen(prev, BeltVertPos(bank, belt, LIGHT_BELT2_ROW_CENTRE * count), cx, cy, sx, sy);
                for (k = 1; k < count; k++) {
                    LIGHT_BELT2_VERT *v = BeltVert(bank, belt, LIGHT_BELT2_ROW_CENTRE * count + k);

                    BeltToScreen(cur, BeltVertPos(bank, belt, LIGHT_BELT2_ROW_CENTRE * count + k), cx, cy, sx, sy);
                    BeltEdge(BeltVertPos(bank, belt, LIGHT_BELT2_ROW_EDGE0 * count + k - 1), turn0, prev, prev,
                             cur, v->width0, scrz, cx, cy, invSx, invSy);
                    BeltEdge(BeltVertPos(bank, belt, LIGHT_BELT2_ROW_EDGE2 * count + k - 1), turn2, prev, prev,
                             cur, v->width2, scrz, cx, cy, invSx, invSy);
                    memcpy(prev, cur, sizeof(prev));
                }
                // the last vertex: the last segment's direction, from the last point itself
                {
                    LIGHT_BELT2_VERT *v = BeltVert(bank, belt, LIGHT_BELT2_ROW_CENTRE * count + count - 1);

                    BeltToScreen(cur, BeltVertPos(bank, belt, LIGHT_BELT2_ROW_CENTRE * count + count - 1), cx, cy,
                                 sx, sy);
                    BeltToScreen(prev, BeltVertPos(bank, belt, LIGHT_BELT2_ROW_CENTRE * count + count - 2), cx,
                                 cy, sx, sy);
                    BeltEdge(BeltVertPos(bank, belt, LIGHT_BELT2_ROW_EDGE0 * count + count - 1), turn0, cur, prev,
                             cur, v->width0, scrz, cx, cy, invSx, invSy);
                    BeltEdge(BeltVertPos(bank, belt, LIGHT_BELT2_ROW_EDGE2 * count + count - 1), turn2, cur, prev,
                             cur, v->width2, scrz, cx, cy, invSx, invSy);
                }
            }
            for (r = 0; r < belt->rows; r++) {
                for (k = 0; k < count; k++) {
                    LIGHT_BELT2_VERT *v = BeltVert(bank, belt, k + r * count);
                    u_int *slots = BANK_PTR(u_int *, bank, v->slots);
                    float *p0 = BANK_PTR(float *, bank, slots[0]);
                    float t[4];
                    int j;

                    memcpy(t, p0, sizeof(t));
                    PortApplyMatrix(p0, inv, t);
                    for (j = 1; j < v->n; j++) {
                        memcpy(BANK_PTR(float *, bank, slots[j]), p0, 4 * sizeof(float));
                    }
                }
            }
        }
        bank += bi->groupCount - 1;
    }
}

// ---- particles and lens flares: the scene's set-up (slice 7, 2026-11-11) ----

// One relocated word of bank data as a u_long (the packets are EE u_long words; the host image keeps
// the EE layout, not its 8-byte alignment).
static void PutWord64(u_long *words, int index, u_long value)
{
    memcpy(&words[index], &value, sizeof(value));
}

// The EE address of part `part`'s field at `offset` (MODEL_PARTS offsets count from the parts header,
// part 0 included: the record proper starts at +PARTS_HDR_RECORDS).
static u_int PartFieldEE(int scene, int part, size_t offset)
{
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[scene];

    return BankEeAddr(scene, (u_char *)&parts[part] + offset);
}

// One list of tiParticleInit: point each entry (and its emitter) at its part's matrix200 and the
// second part's position, fill in a missing emit rate, set up its draw control and texture, and
// point every ring record's draw object at the record's position and its 0x6c block.
static void ParticleInitList(int scene, u_int listEE)
{
    u_int *list = BANK_PTR(u_int *, scene, listEE);
    int i, k;

    if (list == NULL) {
        return;
    }
    for (i = 0; list[i] != 0; i++) {
        PARTICLE2_ENTRY *ent = BANK_PTR(PARTICLE2_ENTRY *, scene, list[i]);
        PARTICLE2_EMITTER *em = BANK_PTR(PARTICLE2_EMITTER *, scene, ent->emitter);
        GA_DRAWCTRL_ARGS *args = BANK_PTR(GA_DRAWCTRL_ARGS *, scene, ent->drawCtrlId);
        TEX_INFO *tex = BANK_PTR(TEX_INFO *, scene, ent->tex);
        PARTICLE2 *ring = BANK_PTR(PARTICLE2 *, scene, ent->ring);
        int n = ent->ringSize;

        ent->idleTicks = -1;
        ent->partMatrix = PartFieldEE(scene, ent->parts, offsetof(MODEL_PARTS, matrix200));
        if (ent->emitRate < 0.1f) {
            ent->emitRate = (float)ent->ringSize / ((float)ent->lifeTenths * 0.1f * 60.0f);
        }
        if (em != NULL) {
            em->matrix = PartFieldEE(scene, em->parts, offsetof(MODEL_PARTS, matrix200));
            em->used = 0;
        }
        if (ent->part2 != 0) {
            ent->partPos = PartFieldEE(scene, ent->part2, offsetof(MODEL_PARTS, pos));
        }
        if (args != NULL) {
            gaSetupDrawCtrlData(BANK_PTR(GA_DRAWCTRL_DATA *, scene, ent->drawCtrl), *args);
        }
        if (tex != NULL) {
            sunSetupTBufferData(BANK_PTR(void *, scene, ent->tbuffer), *tex);
        }
        for (k = 0; k < n; k++) { // the ROM unrolls this by 8
            u_char *obj = BANK_PTR(u_char *, scene, ring[k].obj);
            u_int pos = BankEeAddr(scene, ring[k].pos);
            u_int w58 = ring[k].mats + offsetof(PARTICLE2_MATS, localView);
            u_int w5c = ring[k].mats + offsetof(PARTICLE2_MATS, rotView);

            memcpy(obj + DRAW_OBJ_POS, &pos, sizeof(pos));
            memcpy(obj + DRAW_OBJ_W58, &w58, sizeof(w58));
            memcpy(obj + DRAW_OBJ_W5C, &w5c, sizeof(w5c));
        }
    }
}

// CT3 0x01890350: set up bank `scene`'s second particles, both lists (scene_init).
void tiParticleInit(int scene)
{
    ParticleInitList(scene, All_table_TBL[scene]->particles2);
    ParticleInitList(scene, All_table_TBL[scene]->particles2After);
    // callcheck: gaSetupDrawCtrlData, sunSetupTBufferData (ParticleInitList makes them)
}

// drawctrl_flare's settings (gaSetupDrawCtrlData's 29 argument slots, from the disassembly): Gouraud,
// textured, alpha blended, UV; alpha test always passing, depth test ALWAYS; Cs * As + Cd * (1 - As);
// TEX1: LCM 1, MXL 2, bilinear both ways.
static const GA_DRAWCTRL_ARGS FLARE_DRAWCTRL_ARGS = {
    1, 1, 0, 1, 0, 1, 0, 0, /* PRMODE: IIP, TME, ABE, FST */
    1, 0, 0, 1, 0, 0, 1, 1, /* TEST_1: ATE, ATST 0 (NEVER), AFAIL 1 (FB_ONLY), ZTE, ZTST 1 (ALWAYS) */
    0, 1, 0, 1, 0,          /* ALPHA_1: A 0 (Cs), B 1 (Cd), C 0 (As), D 1 (Cd) */
    0,                      /* FBA_1 */
    1, 2, 1, 1, 0, 0,       /* TEX1_1: LCM 1, MXL 2, MMAG 1, MMIN 1 */
    0};                     /* TEXA */

alignas(16) GA_DRAWCTRL_DATA drawctrl_flare;    /* bss 0x0193d6c0 */

// CT3 0x0188e4a0: set up bank `scene`'s lens flares (scene_init): each flare learns the EE addresses of
// its partsPos qword and its part's position; each element uploads its texture's set-up and gets the
// fixed words of its sprite packet with the texture's rectangle as UVs; then drawctrl_flare.
void tiLensFlaresInit(int scene)
{
    u_int *list = BANK_PTR(u_int *, scene, All_table_TBL[scene]->lensFlares);
    u_int partsPos = All_table_TBL[scene]->partsPos;
    int i, k;

    for (i = 0; list != NULL && list[i] != 0; i++) { // PORT: the ROM reads a 0 list at EE address 0
        LENS_FLARE *flare = BANK_PTR(LENS_FLARE *, scene, list[i]);
        LENS_FLARE_ELEM *elems = BANK_PTR(LENS_FLARE_ELEM *, scene, flare->elems);

        flare->partsPos = partsPos + (u_int)flare->parts * 0x10;
        flare->partPos = PartFieldEE(scene, flare->parts, offsetof(MODEL_PARTS, pos));
        for (k = 0; k < flare->count; k++) {
            LENS_FLARE_ELEM *e = &elems[k];
            TEX_INFO *tex = BANK_PTR(TEX_INFO *, scene, e->tex);
            u_long *pkt = BANK_PTR(u_long *, scene, e->packet);
            int u0 = 0, u1 = 0, v0 = 0, v1 = 0;

            if (tex != NULL) {
                sunSetupTBufferData(BANK_PTR(void *, scene, e->tbuffer), *tex);
                u0 = (int)tex->maxu;
                v0 = (int)tex->maxv;
                u1 = u0 + (int)tex->minu;
                v1 = v0 + (int)tex->minv;
            }
            PutWord64(pkt, LENS_FLARE_PKT_DMATAG, LENS_FLARE_DMATAG);
            PutWord64(pkt, LENS_FLARE_PKT_DMATAG + 1, 0);
            PutWord64(pkt, LENS_FLARE_PKT_VIFCODES, 0);
            PutWord64(pkt, LENS_FLARE_PKT_VIFCODES + 1, LENS_FLARE_VIF_DIRECT4 << 32);
            PutWord64(pkt, LENS_FLARE_PKT_GIFTAG, LENS_FLARE_GIFTAG);
            PutWord64(pkt, LENS_FLARE_PKT_REGS, LENS_FLARE_GIFREGS);
            PutWord64(pkt, LENS_FLARE_PKT_PRIM, LENS_FLARE_PRIM_SPRITE);
            PutWord64(pkt, LENS_FLARE_PKT_UV0,
                      ((((u_long)(u0 << 4) & 0xffff) | LENS_FLARE_UV_HALF_U)) |
                          ((((u_long)(v0 << 4) & 0xffff) | LENS_FLARE_UV_HALF_V) << 16));
            PutWord64(pkt, LENS_FLARE_PKT_UV1,
                      ((((u_long)(u1 << 4) & 0xffff) | LENS_FLARE_UV_HALF_U)) |
                          ((((u_long)(v1 << 4) & 0xffff) | LENS_FLARE_UV_HALF_V) << 16));
        }
    }
    gaSetupDrawCtrlData(&drawctrl_flare, FLARE_DRAWCTRL_ARGS);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// ---- bump mapping: the ST shift (slice 12, 2026-11-11) ----

// One bank's bump-mapped entries (ALL_TABLE.bumps: objOpaque indices, -1 ended): an entry whose trigger
// part is enabled gets its draw object's ST offset (DRAW_OBJ_UV_U / _V, the VU1 header's vf30.xy) set
// to the unit direction from the part's first omni light (GA_LIGHT_WORK.omniPos[0], through the part's
// attr) to the part, x and y, times bumpScale / 10 texels of its texture (1 / (TEX_INFO +0x18 + 1),
// 1 / (+0x20 + 1): the fields ct3_types.h calls minu / minv, which tiLensFlaresInit reads as sizes).
static void BumpSTValueUpdateBank(int bank)
{
    u_int *objs = BANK_PTR(u_int *, bank, All_table_TBL[bank]->objOpaque);
    short *bumps = BANK_PTR(short *, bank, All_table_TBL[bank]->bumps);
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    int i;

    for (i = 0; bumps != NULL && bumps[i] != -1; i++) { // PORT: the ROM reads a 0 list at EE 0
        DRAW_OBJ_ENTRY *ent = BANK_PTR(DRAW_OBJ_ENTRY *, bank, objs[bumps[i]]);
        TEX_INFO *tex = BANK_PTR(TEX_INFO *, bank, ent->tex);
        u_char *obj = BANK_PTR(u_char *, bank, ent->obj);
        MODEL_PARTS *part = &parts[ent->parts];
        const GA_LIGHT_WORK *light;
        float d[4], q, scale, su, sv;

        if (ssTrigCheckEnable(ent->parts & 0xffff, bank) == 0)
            continue;
        su = 0.0f;
        memcpy(obj + DRAW_OBJ_UV_U, &su, sizeof(su));
        memcpy(obj + DRAW_OBJ_UV_V, &su, sizeof(su));
        light = BANK_PTR(const GA_LIGHT_WORK *, bank, (u_int)part->attr);
        // vsub.xyzw, vmul.xyz, vaddy.x, vaddz.x, vrsqrt, vmulq.xyz
        d[0] = part->pos[0] - light->omniPos[0][0];
        d[1] = part->pos[1] - light->omniPos[0][1];
        d[2] = part->pos[2] - light->omniPos[0][2];
        q = 1.0f / sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        d[0] *= q;
        d[1] *= q;
        scale = (float)part->bumpScale / 10.0f;
        su = scale * (d[0] * (1.0f / (float)(tex->minu + 1)));
        sv = scale * (d[1] * (1.0f / (float)(tex->minv + 1)));
        memcpy(obj + DRAW_OBJ_UV_U, &su, sizeof(su));
        memcpy(obj + DRAW_OBJ_UV_V, &sv, sizeof(sv));
    }
}

// CT3 0x01893850: set the bump-mapped entries' ST shifts (main, each frame): the scene banks 0..5
// always, the banks past them (not BANK_TYPE_BEHAVIOR) only while the world runs (pause 0, or
// PAUSE_RUNNING set).
// PORT: the ROM skips a drawn scene bank's group with groupCount - 1 unconditionally; a 0 there would
// loop forever, so the host takes the guarded form the other loops use.
void tiBumpSTValueUpdate(void)
{
    int bank;

    for (bank = 0; bank < BANK_SCENE_MAX; bank++) {
        if (BankInfo[bank].drawOn != 0) {
            BumpSTValueUpdateBank(bank);
            if (BankInfo[bank].groupCount != 0)
                bank += BankInfo[bank].groupCount - 1;
        }
    }
    if (pause == 0 || (pause & PAUSE_RUNNING) != 0) {
        for (bank = BANK_SCENE_MAX; bank < BANK_MAX; bank++) {
            if (BankInfo[bank].drawOn != 0 && BankInfo[bank].type != BANK_TYPE_BEHAVIOR) {
                BumpSTValueUpdateBank(bank);
                if (BankInfo[bank].groupCount != 0)
                    bank += BankInfo[bank].groupCount - 1;
            }
        }
    }    // callcheck: ssTrigCheckEnable (BumpSTValueUpdateBank makes it)
}

// ---- lens flares: the draw (slice 12, 2026-11-11) ----

#define FLARE_SCREEN_W 512.0f           /* the GS window the flare's point must fall in */
#define FLARE_SCREEN_H 448.0f
#define FLARE_CENTRE_X 256.0f           /* the point the elements line up towards (the stack's constant) */
#define FLARE_CENTRE_Y 224.0f
#define FLARE_ALPHA_BIAS 128.0f         /* the part's partsPos w + 128: the sprites' alpha */
#define FLARE_RGBAQ_BASE 0x3f80000000808080ull /* Q 1.0, RGB 0x80 (the texel as it is); alpha at bit 24 */
#define FLARE_XYZF_Z 0x00fffffe00000000ull     /* Z 0xfffffe, F 0 */
#define FLARE_XY_OFFSET 0x8000          /* the GS primitive offset (2048 << 4) */

// One XYZF2 word: x, y in 12.4 plus the offset, as __fixdfdi truncates the double product.
static u_long FlareXYZF(float x, float y)
{
    u_long ix = (u_long)((long long)((double)x * 16.0) + FLARE_XY_OFFSET) & 0xffff;
    u_long iy = (u_long)((long long)((double)y * 16.0) + FLARE_XY_OFFSET) & 0xffff;

    return ix | iy << 16 | FLARE_XYZF_Z;
}

// CT3 0x0188df70: draw every drawn bank's lens flares (DrawAllObject_Normal, ssDOF_DrawAllObject,
// ssHQDOF_DrawAllObject). A flare whose trigger part is enabled and whose part lies in front of the
// eye and projects inside the 512 x 448 window lays its elements along the line from that point
// towards the screen centre (256, 224): element k sits at point + dir * (len * factor), where dir and
// len are the unit vector and length of (centre - point) taken in x, y and the view z (the ROM's
// vsub.xyz: the z term shortens the on-screen step). Each element is its sprite packet with the
// RGBAQ (alpha = the part's partsPos w + 128) and the two corners (size / 2 about the point)
// rewritten; a texture change uploads its CLUT (restarting the BGT ring) only when the TEX_INFO's
// first word differs from the last uploaded one. drawctrl_flare is sent once, before the first
// drawn bank.
// PORT: the packets are sent as the ROM's; the host's gaBGT_ExecDrawCommandData_VIF1 draws no GIF
// sprites yet (a textured sprite seam in alyssapan_drawga.h is host work: open-items).
void tiDrawLensFlares(void)
{
    TEX_INFO *prevTex = NULL;           // ROM: -1 (an element with a null texture is skipped anyway)
    u_int lastTim = 0xffffffffu;
    int ctrlSent = 0;
    int bank;

    for (bank = BANK_SCENE_MAX; bank < BANK_MAX; bank++) {
        if (BankInfo[bank].drawOn != 0 && BankInfo[bank].type != BANK_TYPE_BEHAVIOR) {
            u_int *list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->lensFlares);
            int i;

            if (!ctrlSent) {
                gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_flare);
                ctrlSent = 1;
            }
            for (i = 0; list != NULL && list[i] != 0; i++) { // PORT: the ROM reads a 0 list at EE 0
                LENS_FLARE *flare = BANK_PTR(LENS_FLARE *, bank, list[i]);
                const float *partPos = BANK_PTR(const float *, bank, flare->partPos);
                const float *partsPos = BANK_PTR(const float *, bank, flare->partsPos);
                LENS_FLARE_ELEM *elems;
                float scr[4], dir[3], len, q;
                int k;

                if (ssTrigCheckEnable((u_short)flare->parts, bank) == 0)
                    continue;
                PortApplyMatrix(scr, matrix_global_view, partPos);
                elems = BANK_PTR(LENS_FLARE_ELEM *, bank, flare->elems);
                if (scr[2] <= 0.0f)
                    continue;
                scr[0] = screen_0[SCREEN_CX] + screen_0[SCREEN_AX] * ((screen_0[SCREEN_SCRZ] * scr[0]) / scr[2]);
                scr[1] = screen_0[SCREEN_CY] - screen_0[SCREEN_AY] * ((screen_0[SCREEN_SCRZ] * scr[1]) / scr[2]);
                if (scr[0] < 0.0f || !(scr[0] <= FLARE_SCREEN_W) || scr[1] < 0.0f || !(scr[1] <= FLARE_SCREEN_H))
                    continue;

                // vsub.xyz, vmul.xyz, vaddy.x, vaddz.x, vrsqrt, vmulq.xyz; then vdiv: len = 1 / (1 / len)
                dir[0] = FLARE_CENTRE_X - scr[0];
                dir[1] = FLARE_CENTRE_Y - scr[1];
                dir[2] = 0.0f - scr[2];
                q = 1.0f / sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
                dir[0] *= q;
                dir[1] *= q;
                dir[2] *= q;
                len = 1.0f / q;

                for (k = 0; k < flare->count; k++) {
                    LENS_FLARE_ELEM *e = &elems[k];
                    TEX_INFO *tex = BANK_PTR(TEX_INFO *, bank, e->tex);
                    u_long *pkt = BANK_PTR(u_long *, bank, e->packet);
                    float half = (float)e->size * 0.5f;
                    float step, x, y;

                    if (tex == NULL)
                        continue;
                    PutWord64(pkt, LENS_FLARE_PKT_RGBAQ,
                              (u_long)(partsPos[3] + FLARE_ALPHA_BIAS) << 24 | FLARE_RGBAQ_BASE);
                    step = len * e->factor;
                    x = scr[0] + dir[0] * step;
                    y = scr[1] + dir[1] * step;
                    PutWord64(pkt, LENS_FLARE_PKT_XYZF0, FlareXYZF(x - half, y - half));
                    PutWord64(pkt, LENS_FLARE_PKT_XYZF1, FlareXYZF(x + half, y + half));
                    FlushCache(0);
                    if (prevTex != tex) {
                        void *tbuffer = BANK_PTR(void *, bank, e->tbuffer);

                        if (tex->tim != lastTim) {
                            sunBGT_LoadGSTexClutData_VIF1(tbuffer);
                            gaBGT_End();
                            gaBGT_Start(bgt_ringbuffer, 0x8000); /* main.c BGT_RINGBUFFER_SIZE */
                            lastTim = tex->tim;
                        }
                        sunBGT_ExecTBufferData_VIF1(tbuffer);
                    }
                    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)pkt);
                    prevTex = tex;
                }
            }
        }
        if (BankInfo[bank].groupCount != 0)
            bank += BankInfo[bank].groupCount - 1;
    }
    // callcheck: __fixunssfdi, fptodp, dpmul, __fixdfdi (the float -> integer / double conversions)
}

// ---- GS frame-buffer copies (slice 11, 2026-11-11): the shade-off and its two helpers ----

// The two sprite packets (0x60 bytes: a DMAtag of qwc 5, NOPs and DIRECT, a GIFtag of NREG 6 in packed
// mode, registers PRIM RGBAQ UV XYZ2 UV XYZ2), as u_long words; the draws rewrite the corners.
#define SPRITE_PACKET_WORDS 12
#define SPRITE_RGBAQ 7                  /* word 7: RGBAQ (Q 1.0 in the high word) */
#define SPRITE_UV0 8                    /* the first corner's UV, then XYZ2 */
#define SPRITE_XYZ0 9
#define SPRITE_UV1 10                   /* the second corner's */
#define SPRITE_XYZ1 11
#define SPRITE_RGBAQ_GREY 0x3f80000080808080ul /* RGBA 0x80 each (the texel as it is), Q 1.0 */
#define GS_XYZ_Z_FAR 0xfffffeul         /* the sprites' Z */
#define GS_WINDOW_ORIGIN 0x8000         /* XYZ2's window origin, 12.4 (2048.0) */
#define GS_UV_HALF 9                    /* UV's 12.4 fraction on the last texel (0.5625) */

// data 0x019387e0 drawprim: CaptureDBuffer's sprite -- the whole 512 x 448 frame read with UV (0.5, 0.5625)
// .. (511.5, 447.5625), from window (-0.5, -0.5); the second corner is written per capture.
alignas(16) static u_long drawprim[SPRITE_PACKET_WORDS] = {
    0x10000005ul, 0, 0, 0x5000000400000000ul, 0x6400000000008001ul, 0x434310ul,
    6, SPRITE_RGBAQ_GREY, 0x90008ul, 0xfffffe7ff87ff8ul, 0x1bf91ff8ul, 0xfffffe9ff89ff8ul};
// data 0x01938980 sprite_vram$61: tiVRAMViewer's sprite; RGBAQ, the second UV and both corners per call.
alignas(16) static u_long sprite_vram_61[SPRITE_PACKET_WORDS] = {
    0x10000005ul, 0, 0, 0x5000000400000000ul, 0x6400000000008001ul, 0x434310ul,
    6, 0, 0x90009ul, 0, 0, 0};
// data 0x01938880 tex_attr_before: the frame buffer read as a texture (512 x 448, CT32, already in GS
// memory: no data words).
static const SUN_TEX_ATTR tex_attr_before = {GS_FRAME_WIDTH, GS_FRAME_HEIGHT, GS_FRAME_LOG2, GS_FRAME_LOG2,
                                             SCE_GS_PSMCT32, 0, 0x000e0000u, {0, 0, 0, 0}, 0, 0};
alignas(16) static GA_DBUFFER_DATA d_buffer_140;    /* sbss 0x0193dc40 d_buffer$140 */
alignas(16) static GA_DRAWCTRL_DATA drawctrl_138;   /* sbss 0x0193ddc0 drawctrl$138 */
alignas(16) static u_char tex_before_139[0x100];    /* sbss 0x0193dcc0 tex_before$139: a SUN_TBUFFER_DATA */
alignas(16) static GA_DRAWCTRL_DATA drawctrl_63;    /* sbss 0x0193df80 drawctrl$63 */
alignas(16) static u_char tex_64[0x100];            /* sbss 0x0193de80 tex$64: a SUN_TBUFFER_DATA */

// A window position (pixels of the frame) as XYZ2's X / Y fields: 12.4 about the window origin, truncated
// as the ROM's __fixdfdi does, Z GS_XYZ_Z_FAR.
static u_long SpriteXyz(float x, float y)
{
    u_long gx = (u_long)((int64_t)((double)x * 16.0) + GS_WINDOW_ORIGIN) & 0xffff;
    u_long gy = (u_long)((int64_t)((double)y * 16.0) + GS_WINDOW_ORIGIN) & 0xffff;

    return gx | gy << 16 | GS_XYZ_Z_FAR << 32;
}

// The capture's draw control: textured, alpha blended, UV; ATE with ATST 0 (never) AFAIL 1 (FB only), ZTE 1
// ZTST 1 (always); ALPHA_1 (Cs - Cd) * 0x80 + Cd (the source alone); FBA 1; bilinear.
#define CAPTURE_DRAWCTRL_ARGS {                                                     0, 1, 0, 1, 0, 1, 0, 0,      /* PRMODE: TME, ABE, FST */                       1, 0, 0, 1, 0, 0, 1, 1,      /* TEST_1: ATE, AFAIL 1, ZTE, ZTST 1 */           0, 1, 2, 1, VRAM_ALPHA_OPAQUE, /* ALPHA_1: A Cs, B Cd, C FIX, D Cd */           1,                           /* FBA_1 */                                       1, 2, 1, 1, 0, 0,            /* TEX1_1 */                                      0}                           /* TEXA */
// tiVRAMViewer's: the same without ZTE and FBA, at the caller's FIX.
#define VRAM_DRAWCTRL_ARGS(alphaFix) {                                              0, 1, 0, 1, 0, 1, 0, 0,      /* PRMODE: TME, ABE, FST */                       1, 0, 0, 1, 0, 0, 0, 1,      /* TEST_1: ATE, AFAIL 1, ZTST 1 */                0, 1, 2, 1, (u_int)(alphaFix), /* ALPHA_1: A Cs, B Cd, C FIX, D Cd */           0,                           /* FBA_1 */                                       1, 2, 1, 1, 0, 0,            /* TEX1_1 */                                      0}                           /* TEXA */

// CT3 0x018945f0: the frame drawn so far (the draw buffer, ga_draw_fbuf_address) drawn into the effect
// buffer, scaled to width x height: the draw buffer moves to EFFECT_FBUF_ADDRESS (width pixels wide,
// scissored to the picture), the frame is bound as a texture and drawprim covers the picture; then
// drawbuffer_0 is sent again. EffectBuffer_Use is held meanwhile.
// PORT: the GS copy has no memory to land in on the host: the packets are built and sent as the ROM does,
// and the host draw module is told the copy (alyssapan_drawga.h AlyssaPan_DrawGaCapture).
void CaptureDBuffer(u_int width, u_int height)
{
    int fbuf = ga_draw_fbuf_address;

    EffectBuffer_Use = 1;
    drawprim[SPRITE_XYZ1] = SpriteXyz((float)width - 0.5f, (float)height - 0.5f);
    gaSetupDBufferData(&d_buffer_140, EFFECT_FBUF_ADDRESS, width, 0, 0, 0, 0, 0, 0, 0, width - 1, height - 1,
                       0, 0, 0);
    {
        GA_DRAWCTRL_ARGS args = CAPTURE_DRAWCTRL_ARGS;

        gaSetupDrawCtrlData(&drawctrl_138, args);
    }
    {
        TEX_INFO tex;

        memset(&tex, 0, sizeof(tex));
        tex.tim = 0x01938880u;                  /* tex_attr_before */
        tex.tbp = (u_int)ga_draw_fbuf_address;
        tex.wms = TEX_WRAP_CLAMP;
        tex.wmt = TEX_WRAP_CLAMP;
        sunSetupTBufferDataFrom(tex_before_139, tex, &tex_attr_before);
    }
    ga_draw_fbuf_address = EFFECT_FBUF_ADDRESS;
    FlushCache(0);
    gaBGT_ExecDBufferData_VIF1(&d_buffer_140);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_138);
    sunBGT_ExecTBufferData_VIF1(tex_before_139);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)drawprim);
    AlyssaPan_DrawGaCapture(EFFECT_FBUF_ADDRESS, (int)width, (int)height);
    ga_draw_fbuf_address = fbuf;
    FlushCache(0);
    gaBGT_ExecDBufferData_VIF1(&drawbuffer_0);
    EffectBuffer_Use = 0;
    // callcheck: sunSetupTBufferData (PORT: sunSetupTBufferDataFrom, the record is host data)
    // callcheck: __fixdfdi, dpmul, fptodp (the corners' double maths: the compiler's runtime, in SpriteXyz)
}

// CT3 0x01894ed0: a GS region drawn as a sprite: fbp (texW x texH pixels; TEX0 tw / th / psm; the record's
// size at bytesPerPixel) bound as a texture, its texels 0.5625 .. size - 0.4375 over the window rectangle
// x, y .. x + w - 1, y + h - 1, grey 0x80 (the texel as it is), blended (Cs - Cd) * alphaFix + Cd.
// The record starts as a copy of @62 (bss 0x0193d0d0, 0x80 zero bytes).
// PORT: as CaptureDBuffer: the packets as the ROM's, the draw named to the host (AlyssaPan_DrawGaCopy).
void tiVRAMViewer(float x, float y, float w, float h, u_int fbp, int texW, int texH, u_int tw, u_int th,
                  u_int psm, int bytesPerPixel, int alphaFix)
{
    SUN_TEX_ATTR attr;

    memset(&attr, 0, sizeof(attr));
    attr.width = (u_int)texW;
    attr.height = (u_int)texH;
    attr.tw = tw;
    attr.th = th;
    attr.psm = psm;
    attr.bytes = (u_int)(bytesPerPixel * texW * texH);
    sprite_vram_61[SPRITE_RGBAQ] = SPRITE_RGBAQ_GREY;
    sprite_vram_61[SPRITE_UV1] = ((u_long)(((texH - 1) * 16 | GS_UV_HALF) & 0xffff) << 16) |
                                 (u_long)(((texW - 1) * 16 | GS_UV_HALF) & 0xffff);
    sprite_vram_61[SPRITE_XYZ0] = SpriteXyz(x, y);
    sprite_vram_61[SPRITE_XYZ1] = SpriteXyz(x + w - 1.0f, y + h - 1.0f);
    {
        GA_DRAWCTRL_ARGS args = VRAM_DRAWCTRL_ARGS(alphaFix);

        gaSetupDrawCtrlData(&drawctrl_63, args);
    }
    {
        TEX_INFO tex;

        memset(&tex, 0, sizeof(tex));
        tex.tbp = fbp;                          /* tim: the stack record (sunSetupTBufferDataFrom) */
        tex.wms = TEX_WRAP_CLAMP;
        tex.wmt = TEX_WRAP_CLAMP;
        sunSetupTBufferDataFrom(tex_64, tex, &attr);
    }
    FlushCache(0);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_63);
    sunBGT_ExecTBufferData_VIF1(tex_64);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)sprite_vram_61);
    AlyssaPan_DrawGaCopy(fbp, texW, texH, x, y, x + w - 1.0f, y + h - 1.0f, alphaFix);
    // callcheck: sunSetupTBufferData (PORT: sunSetupTBufferDataFrom, the record is host data)
    // callcheck: __fixdfdi, dpmul, fptodp (the corners' double maths: the compiler's runtime, in SpriteXyz)
}

// data 0x01938d40 @1854: the shade-off's slide direction (1, 1, 0, 0), scaled by the slide in pixels.
static const float ShadeOffSlide_1854[4] = {1.0f, 1.0f, 0.0f, 0.0f};

#define SHADE_OFF_TURN 0xfff0           /* gaSetRotMatrixZ angle shared out among the copies (0x10000 a turn) */
#define SHADE_OFF_COPY_ALPHA 0x40       /* each turned copy's FIX: half */

// CT3 0x0188ced0: once a frame while fShadeOff (tiXcmd_ShadeOff): the screen captured at 2^CapLevel
// pixels square (512 x 448 at level 9) and drawn back over the whole frame (the blur), then ShadeOff_Level
// copies of the frame at half alpha, each slid by the slide vector turned a share of a full turn. The slide
// grows to ShadeOff_SlidePixel while it counts down (nowShadeOffFrame), or, on release
// (KEEP_SCREEN_RELEASE), shrinks back with it and ends the shade-off. While held (KEEP_SCREEN_HOLD) the
// first capture stays (KEEP_SCREEN_CAPTURED) and the effect buffer stays taken.
// ROM quirk kept: the copies pass x + 512 / y + 448 as their size, so each spans x .. 2x + 511.
void tiDrawShadingOff(void)
{
    float slide[4];
    float matrix[4][4];
    float pos[4];
    float pixels;
    u_int width, height;
    int i, k;

    memcpy(slide, ShadeOffSlide_1854, sizeof(slide));
    if (fShadeOff == 0)
        return;

    if ((fKeepScreen & KEEP_SCREEN_RELEASE) == 0) {
        pixels = ShadeOff_SlidePixel;
        if (nowShadeOffFrame != 0) {
            nowShadeOffFrame--;
            if (nowShadeOffFrame < 1)
                nowShadeOffFrame = 0;
            else
                pixels = ShadeOff_SlidePixel * (1.0f - (float)nowShadeOffFrame / (float)ShadeOffFrame);
        }
    } else {
        if (nowShadeOffFrame < 0) {
            fShadeOff = 0;
            fKeepScreen = 0;
            EffectBuffer_Use = 0;
            return;
        }
        pixels = (float)nowShadeOffFrame;
        nowShadeOffFrame--;
        pixels = (ShadeOff_SlidePixel * pixels) / (float)ShadeOffFrame;
    }

    width = GS_FRAME_WIDTH;
    if (ShadeOff_CapLevel == SHADE_OFF_LEVELS) {
        height = GS_FRAME_HEIGHT;
    } else {
        width = (u_int)tiFactorial(2, (u_int)(u_char)ShadeOff_CapLevel);
        height = width;
    }
    if ((fKeepScreen & KEEP_SCREEN_CAPTURED) == 0) {
        CaptureDBuffer(width, height);
        if ((fKeepScreen & KEEP_SCREEN_HOLD) != 0) {
            EffectBuffer_Use = 1;
            fKeepScreen |= KEEP_SCREEN_CAPTURED;
        }
    }
    // vmulx.xyz: w keeps its 0
    slide[0] *= pixels;
    slide[1] *= pixels;
    slide[2] *= pixels;
    tiVRAMViewer(0.0f, 0.0f, (float)GS_FRAME_WIDTH, (float)GS_FRAME_HEIGHT, EFFECT_FBUF_ADDRESS, (int)width,
                 (int)height, (u_int)(u_char)ShadeOff_CapLevel, (u_int)(u_char)ShadeOff_CapLevel, SCE_GS_PSMCT32,
                 4, VRAM_ALPHA_OPAQUE);
    for (i = 0; i < (int)ShadeOff_Level; i++) {
        gaSetRotMatrixZ(matrix, (SHADE_OFF_TURN / ShadeOff_Level) * i);
        for (k = 0; k < 4; k++) {
            pos[k] = matrix[0][k] * slide[0] + matrix[1][k] * slide[1] + matrix[2][k] * slide[2] +
                     matrix[3][k] * slide[3];
        }
        tiVRAMViewer(pos[0], pos[1], pos[0] + (float)GS_FRAME_WIDTH, pos[1] + (float)GS_FRAME_HEIGHT,
                     (u_int)ga_draw_fbuf_address, GS_FRAME_WIDTH, GS_FRAME_HEIGHT, GS_FRAME_LOG2, GS_FRAME_LOG2,
                     SCE_GS_PSMCT32, 4, SHADE_OFF_COPY_ALPHA);
    }
}

// One bank's refracting entries (ALL_TABLE.objAfter, the list drawn after ExportDrawToTex): an entry whose
// trigger part is enabled gets its ST from SetSTValueForRefraction with the eye taken into the part's
// frame (matrix200 inverted; a PARTS_KIND_WIND_ON_ROOT part stands for part 0, as for the winds) and the
// part's matrix340, strength refrIndex / 16384 (read from the listed part, before that swap).
static void RefractionMapUpdateBank(int bank, const float *eye)
{
    u_int *list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->objAfter);
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    int i;

    for (i = 0; list != NULL && list[i] != 0; i++) { // PORT: the ROM reads a 0 list at EE 0
        DRAW_OBJ_ENTRY *ent = BANK_PTR(DRAW_OBJ_ENTRY *, bank, list[i]);
        MODEL_PARTS *part;
        float inv[4][4], local[4], index;

        if (ssTrigCheckEnable((u_short)ent->parts, bank) == 0)
            continue;
        part = &parts[ent->parts];
        index = (float)part->refrIndex / 16384.0f;
        if ((signed char)part->kind == PARTS_KIND_WIND_ON_ROOT)
            part = parts;
        gaInversMatrix(inv, part->matrix200);
        PortApplyMatrix(local, inv, eye);
        SetSTValueForRefraction(BANK_PTR(u_char *, bank, ent->obj), local, part->matrix340, index);
    }
}

// CT3 0x01894230: the refraction map's texture coordinates (main, after ExportDrawToTex): the scene banks
// 0..5 with their own camera (Cam[bank]), the banks past them (not BANK_TYPE_BEHAVIOR) with the shared
// one, only while the world runs (pause 0, or PAUSE_RUNNING); each bank first sets the projection
// distance from its camera's focal length (Cam[asCheckBankID(bank)].focal[3]).
// PORT: the scene loop's group skip is guarded as in tiBumpSTValueUpdate.
void tiRefractionMapUpdate(void)
{
    float eye[4];
    int bank;

    for (bank = 0; bank < BANK_SCENE_MAX; bank++) {
        if (BankInfo[bank].drawOn != 0) {
            screen_0[SCREEN_SCRZ] = asFocalLength_LEICA2PS2(Cam[asCheckBankID(bank)].focal[3]);
            // one buffer for all four answers: the position is written last (as tiParticleUpdate)
            asViewMatrix2PosRUF(&Cam[bank].view[0][0], eye, eye, eye, eye);
            RefractionMapUpdateBank(bank, eye);
            if (BankInfo[bank].groupCount != 0)
                bank += BankInfo[bank].groupCount - 1;
        }
    }
    if (pause == 0 || (pause & PAUSE_RUNNING) != 0) {
        for (bank = BANK_SCENE_MAX; bank < BANK_MAX; bank++) {
            if (BankInfo[bank].drawOn != 0 && BankInfo[bank].type != BANK_TYPE_BEHAVIOR) {
                asViewMatrix2PosRUF(&Cam[CAMERA_SHARED].view[0][0], eye, eye, eye, eye);
                screen_0[SCREEN_SCRZ] = asFocalLength_LEICA2PS2(Cam[asCheckBankID(bank)].focal[3]);
                RefractionMapUpdateBank(bank, eye);
                if (BankInfo[bank].groupCount != 0)
                    bank += BankInfo[bank].groupCount - 1;
            }
        }
    }    // callcheck: ssTrigCheckEnable, gaInversMatrix, SetSTValueForRefraction (RefractionMapUpdateBank makes them)
}

// stub of CT3 0x0188d3f0
void tiXFadeOut_Draw(void)
{
    ALYSSAPAN_STUB("tiXFadeOut_Draw", 0x0188d3f0);
}

#define EXPORT_FBUF_ADDRESS_SMALL 0xe0000u /* ExportDrawToTex's copy while the effect buffer is taken */
#define EXPORT_TEX_SIZE 512             /* the copy's square side: 512 into the effect buffer ... */
#define EXPORT_TEX_SIZE_SMALL 256       /* ... 256 at EXPORT_FBUF_ADDRESS_SMALL */
#define EXPORT_TEX_LOG2 9
#define EXPORT_TEX_LOG2_SMALL 8

// data 0x01938900 tex_attr_after: the copy read as a texture (CT32, already in GS memory); ExportDrawToTex
// rewrites its size per call. The ROM's bytes: 512 x 512.
static SUN_TEX_ATTR tex_attr_after = {EXPORT_TEX_SIZE, EXPORT_TEX_SIZE, EXPORT_TEX_LOG2, EXPORT_TEX_LOG2,
                                      SCE_GS_PSMCT32, 0, 0x00100000u, {0, 0, 0, 0}, 0, 0};
alignas(16) static GA_DBUFFER_DATA d_buffer_226;    /* sbss 0x0193dbc0 d_buffer$226 */
alignas(16) static GA_DRAWCTRL_DATA drawctrl_227;   /* sbss 0x0193db00 drawctrl$227 */
alignas(16) static u_char tex_before_228[0x100];    /* sbss 0x0193da00 tex_before$228: a SUN_TBUFFER_DATA */
alignas(16) static u_char tex_after_229[0x100];     /* sbss 0x0193d900 tex_after$229: a SUN_TBUFFER_DATA */

// CT3 0x01893cf0: main's passes, before the objects drawn after the particles (ALL_TABLE.objAfter,
// particles2After): the frame drawn so far copied, scaled, into a square texture -- 512 x 512 in the effect
// buffer, or 256 x 256 at EXPORT_FBUF_ADDRESS_SMALL when the effect buffer is taken -- under the capture's
// draw control and drawprim; then drawbuffer_0 is sent again and the copy is bound as the texture, so the
// objects after it draw with the screen (refraction). The effect buffer is held during the copy only.
// PORT: as CaptureDBuffer: the packets as the ROM's, the copy named to the host (AlyssaPan_DrawGaCapture).
// The host has no picture for the bound copy yet: sunBGT_ExecTBufferData_VIF1 finds no pixels and the
// objects after it draw untextured (open-items.md).
void ExportDrawToTex(void)
{
    u_char wasUsed = EffectBuffer_Use;
    int fbuf = ga_draw_fbuf_address;
    u_int dest;

    if (EffectBuffer_Use == 0) {
        dest = EFFECT_FBUF_ADDRESS;
        EffectBuffer_Use = 1;
        tex_attr_after.bytes = EXPORT_TEX_SIZE * EXPORT_TEX_SIZE * 4;
        tex_attr_after.tw = EXPORT_TEX_LOG2;
        tex_attr_after.th = EXPORT_TEX_LOG2;
        tex_attr_after.width = EXPORT_TEX_SIZE;
        tex_attr_after.height = EXPORT_TEX_SIZE;
        drawprim[SPRITE_XYZ1] = SpriteXyz(EXPORT_TEX_SIZE - 0.5f, EXPORT_TEX_SIZE - 0.5f);
        gaSetupDBufferData(&d_buffer_226, dest, EXPORT_TEX_SIZE, 0, 0, 0, 0, 0, 0, 0, EXPORT_TEX_SIZE - 1,
                           EXPORT_TEX_SIZE - 1, 0, 0, 0);
    } else {
        tex_attr_after.bytes = EXPORT_TEX_SIZE_SMALL * EXPORT_TEX_SIZE_SMALL * 4;
        tex_attr_after.tw = EXPORT_TEX_LOG2_SMALL;
        tex_attr_after.th = EXPORT_TEX_LOG2_SMALL;
        dest = EXPORT_FBUF_ADDRESS_SMALL;
        tex_attr_after.width = EXPORT_TEX_SIZE_SMALL;
        tex_attr_after.height = EXPORT_TEX_SIZE_SMALL;
        drawprim[SPRITE_XYZ1] = SpriteXyz(EXPORT_TEX_SIZE_SMALL - 0.5f, EXPORT_TEX_SIZE_SMALL - 0.5f);
        gaSetupDBufferData(&d_buffer_226, dest, EXPORT_TEX_SIZE_SMALL, 0, 0, 0, 0, 0, 0, 0,
                           EXPORT_TEX_SIZE_SMALL - 1, EXPORT_TEX_SIZE_SMALL - 1, 0, 0, 0);
    }
    {
        GA_DRAWCTRL_ARGS args = CAPTURE_DRAWCTRL_ARGS;

        gaSetupDrawCtrlData(&drawctrl_227, args);
    }
    {
        TEX_INFO tex;

        memset(&tex, 0, sizeof(tex));
        tex.tim = 0x01938880u;                  /* tex_attr_before */
        tex.tbp = (u_int)ga_draw_fbuf_address;
        tex.wms = TEX_WRAP_CLAMP;
        tex.wmt = TEX_WRAP_CLAMP;
        sunSetupTBufferDataFrom(tex_before_228, tex, &tex_attr_before);
    }
    ga_draw_fbuf_address = (int)dest;
    FlushCache(0);
    gaBGT_ExecDBufferData_VIF1(&d_buffer_226);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_227);
    sunBGT_ExecTBufferData_VIF1(tex_before_228);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)drawprim);
    AlyssaPan_DrawGaCapture(dest, (int)tex_attr_after.width, (int)tex_attr_after.height);
    ga_draw_fbuf_address = fbuf;
    {
        TEX_INFO tex;

        memset(&tex, 0, sizeof(tex));
        tex.tim = 0x01938900u;                  /* tex_attr_after */
        tex.tbp = dest;
        tex.wms = TEX_WRAP_CLAMP;
        tex.wmt = TEX_WRAP_CLAMP;
        sunSetupTBufferDataFrom(tex_after_229, tex, &tex_attr_after);
    }
    FlushCache(0);
    gaBGT_ExecDBufferData_VIF1(&drawbuffer_0);
    sunBGT_ExecTBufferData_VIF1(tex_after_229);
    if (wasUsed == 0)
        EffectBuffer_Use = 0;
    // callcheck: sunSetupTBufferData (PORT: sunSetupTBufferDataFrom, the records are host data)
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

// stub of CT3 0x018949c0
void tiGetTransRotMatrix(float (*dst)[4], float *pos, float (*src)[4])
{
    ALYSSAPAN_STUB("tiGetTransRotMatrix", 0x018949c0);
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

// stub of CT3 0x0188ea90
void tiCalc_Leaf(int ticks, PARTICLE2 *rec, int arg, int bank)
{
    (void)ticks;
    (void)rec;
    (void)arg;
    (void)bank;
    ALYSSAPAN_STUB("tiCalc_Leaf", 0x0188ea90);
}

// stub of CT3 0x0188c6e0
void tiCalc_Trans_WIND(int ticks, PARTICLE2 *rec, int bank)
{
    (void)ticks;
    (void)rec;
    (void)bank;
    ALYSSAPAN_STUB("tiCalc_Trans_WIND", 0x0188c6e0);
}

#define LEAF_TICKS_PER_SECOND 300.0f   /* dt = ticks / 300 */
#define LEAF_PUSH_DIVISOR 4.0f          /* the face's push: arg * speed * (n . -v) / 4 */
#define LEAF_TURN_MIN 0x16c             /* the turn per step, ahAcos units (65536 a turn): 2 degrees .. */
#define LEAF_TURN_MAX 0x71c             /* .. 10 degrees */

// CT3 0x0188c210: a falling leaf in the winds (tiCalc_Orbit's ORBIT_LEAF_WIND). A random vertex normal of
// the record's object, turned by local's rotation, is the leaf's face n; the air pushes it along n by
// arg * |v| * (n . -v / |v|) / 4; every enabled wind whose capsule holds the leaf (or whose radius is 0)
// adds its world10 vector; with the acceleration that moves local's row 3 by v dt + a dt^2 and makes
// v' = v + a 2dt (dt = ticks / 300; v's w kept). The leaf turns about its own position by the angle
// between v and v' (clamped to 2..10 degrees) three times: about n x v', about v', about n. age += ticks.
void tiCalc_Leaf_WIND(int ticks, PARTICLE2 *rec, int arg, int bank)
{
    PARTICLE2_MATS *m = RecMats(rec, bank);
    u_char *obj = BANK_PTR(u_char *, bank, rec->obj);
    float dt = (float)ticks / LEAF_TICKS_PER_SECOND;
    float rot[4][4], turn[4][4], n[4], vn[4], vu[4], push[4], wind[4], a[4], vNew[4], axis[4], pos[4];
    const float *normals;
    u_int nrmEE;
    int count, i, angle;
    float speed, k;

    rec->age += ticks;
    memcpy(&nrmEE, obj + REFR_OBJ_NORMALS, sizeof(nrmEE));
    memcpy(&count, obj + REFR_OBJ_VERTEX_COUNT, sizeof(count));
    normals = BANK_PTR(const float *, bank, nrmEE + REFR_BLOCK_VIF_BYTES);

    // the face: a random vertex normal through local's rotation (row 3 = (0, 0, 0, 1))
    memcpy(rot, m->local, sizeof(rot));
    rot[3][0] = rot[3][1] = rot[3][2] = 0.0f;
    rot[3][3] = 1.0f;
    PortApplyMatrix(n, rot, &normals[tnRand(count - 1) * 4]);

    // the air's push
    vn[0] = rec->vel[0] * -1.0f;
    vn[1] = rec->vel[1] * -1.0f;
    vn[2] = rec->vel[2] * -1.0f;
    vn[3] = rec->vel[3];
    speed = PortUnitXYZ(vn);
    k = ((float)arg * speed * gaInnerProductXYZ(n, vn)) / LEAF_PUSH_DIVISOR;
    push[0] = n[0] * k; // vmulx.xyz: w n's
    push[1] = n[1] * k;
    push[2] = n[2] * k;
    push[3] = n[3];

    // the winds
    wind[0] = wind[1] = wind[2] = 0.0f;
    wind[3] = 1.0f;
    for (i = 0; i < WIND_MAX; i++) {
        WIND *w = WindList[i];

        if (w != NULL && ssTrigCheckEnable((u_short)w->part, w->bank) != 0 &&
            (w->radius == 0.0f || tiCheckWindForce(m->local[3], w) != 0)) {
            wind[0] += w->world10[0];
            wind[1] += w->world10[1];
            wind[2] += w->world10[2];
        }
    }

    // the step
    for (i = 0; i < 3; i++) {
        a[i] = push[i] + wind[i] + rec->accel[i];
        m->local[3][i] += rec->vel[i] * dt + a[i] * (dt * dt);
    }
    for (i = 0; i < 3; i++) {
        vNew[i] = rec->vel[i] + a[i] * (dt * 2.0f);
    }
    vNew[3] = rec->vel[3];

    // the turn
    memcpy(vn, vNew, sizeof(vn));
    PortUnitXYZ(vn);
    memcpy(vu, rec->vel, sizeof(vu));
    PortUnitXYZ(vu);
    angle = ahAcos(gaInnerProductXYZ(vn, vu));
    if (angle > LEAF_TURN_MAX)
        angle = LEAF_TURN_MAX;
    else if (angle < LEAF_TURN_MIN)
        angle = LEAF_TURN_MIN;
    axis[0] = n[1] * vn[2] - vn[1] * n[2]; // vopmula / vopmsub: n x v'
    axis[1] = n[2] * vn[0] - vn[2] * n[0];
    axis[2] = n[0] * vn[1] - vn[0] * n[1];
    axis[3] = n[3]; // vopmsub.xyz into n's register: w n's
    asMakeRotMatrixByQuartanion(turn, axis, angle);
    memcpy(pos, m->local[3], sizeof(pos));
    memcpy(rot, matUnit, sizeof(rot));
    rot[3][0] += -pos[0];
    rot[3][1] += -pos[1];
    rot[3][2] += -pos[2];
    gaMulMatrix(m->local, rot, m->local);
    gaMulMatrix(m->local, turn, m->local);
    asMakeRotMatrixByQuartanion(turn, vn, angle);
    gaMulMatrix(m->local, turn, m->local);
    asMakeRotMatrixByQuartanion(turn, n, angle);
    gaMulMatrix(m->local, turn, m->local);
    memcpy(rot, matUnit, sizeof(rot));
    rot[3][0] += pos[0];
    rot[3][1] += pos[1];
    rot[3][2] += pos[2];
    gaMulMatrix(m->local, rot, m->local);
    memcpy(rec->vel, vNew, sizeof(vNew));
}

// stub of CT3 0x0188e8c0
void tiCalc_Twist(int ticks, PARTICLE2 *rec, PARTICLE2_ENTRY *ent, int bank)
{
    (void)ticks;
    (void)rec;
    (void)ent;
    (void)bank;
    ALYSSAPAN_STUB("tiCalc_Twist", 0x0188e8c0);
}

// ---- refraction: the per-vertex ST (slice 12, 2026-11-11) ----


// CT3 0x01893f90: refraction texture coordinates. For each vertex: d = unit(vertex - eye) (xyz; the w
// lane is the plain difference), r = normal * (index * d . normal) - d (xyz, w 0), and the point
// eye + r (xyz; w the eye's) through localView, projected as the screen projects (screen_0, read once
// on entry) and scaled to 0..1 by the 512 x 448 window (clamped): the vertex's s, t. A point on the
// eye plane (z 0) keeps its old ST.
// PORT: obj is the host draw object; its block words are EE words of `bank`'s data, so the bank is
// found from the pointer (BankOfHostPtr).
void SetSTValueForRefraction(u_char *obj, float *eye, float (*localView)[4], float index)
{
    const float cx = screen_0[SCREEN_CX], cy = screen_0[SCREEN_CY];
    const float ax = screen_0[SCREEN_AX], ay = screen_0[SCREEN_AY];
    const float scrz = screen_0[SCREEN_SCRZ];
    int bank = BankOfHostPtr(obj);
    int count;
    u_int posEE, nrmEE, stEE;
    const float *pos, *nrm;
    float *st;
    int i;

    memcpy(&count, obj + REFR_OBJ_VERTEX_COUNT, sizeof(count));
    memcpy(&posEE, obj + REFR_OBJ_POSITIONS, sizeof(posEE));
    memcpy(&nrmEE, obj + REFR_OBJ_NORMALS, sizeof(nrmEE));
    memcpy(&stEE, obj + REFR_OBJ_ST, sizeof(stEE));
    pos = BANK_PTR(const float *, bank, posEE);
    nrm = BANK_PTR(const float *, bank, nrmEE + REFR_BLOCK_VIF_BYTES);
    st = BANK_PTR(float *, bank, stEE + REFR_BLOCK_VIF_BYTES);
    for (i = 0; i < count; i++) {
        const float *v = &pos[i * 4];
        const float *n = &nrm[i * 4];
        float d[4], r[4], p[4], out[4], q, k, s, t;

        d[0] = v[0] - eye[0];
        d[1] = v[1] - eye[1];
        d[2] = v[2] - eye[2];
        d[3] = v[3] - eye[3];
        q = 1.0f / sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        d[0] *= q;
        d[1] *= q;
        d[2] *= q;
        k = index * gaInnerProductXYZ(d, (float *)n);
        r[0] = n[0] * k - d[0];
        r[1] = n[1] * k - d[1];
        r[2] = n[2] * k - d[2];
        r[3] = 0.0f;
        p[0] = eye[0] + r[0];
        p[1] = eye[1] + r[1];
        p[2] = eye[2] + r[2];
        p[3] = eye[3];
        PortApplyMatrix(out, localView, p);
        if (out[2] == 0.0f)
            continue;
        s = (cx + ax * ((scrz * out[0]) / out[2])) * (1.0f / 512.0f);
        t = (cy - ay * ((scrz * out[1]) / out[2])) * 0.002232143f; /* 1 / 448 as the ROM rounds it */
        if (s < 0.0f)
            s = 0.0f;
        else if (!(s <= 1.0f))
            s = 1.0f;
        if (t < 0.0f)
            t = 0.0f;
        else if (!(t <= 1.0f))
            t = 1.0f;
        st[i * 2] = s;
        st[i * 2 + 1] = t;
    }
}
