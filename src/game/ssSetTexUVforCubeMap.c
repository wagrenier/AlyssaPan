// game/ssSetTexUVforCubeMap.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Reconstructed: the ambient colour, the part motion blur's counters, init, update and end, the particle
// tables' end, the screen blur off and the Z clear (2026-11-12); the reflection maps, a character's ambient
// change, the particle init and stop, HQDOF end, DOF clip range and the curve cache clear (2026-11-12, slice 2); the
// particle curve readers, the per-slot setters and the ring life helpers (2026-11-12, slice 3); the particle
// main loop, emission and push-backs, the blur draw-table insert, the DOF slice clip, particle all-off and
// hazy end (2026-11-12, slice 4); the
// rest are stubs (tools/reverse/stage.py).

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "common/alyssapan_stub.h"
#include "game/ssSetTexUVforCubeMap.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaInitDrawObject.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/LinkMatrixTree.h"
#include "game/Clipping_Portal.h"
#include "game/ahalloc.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ct3_types.h"
#include "game/main.h"
#include "game/unassigned_data.h"

// The three draw lists ssSetAmbColorToAllObj walks, in its order (ALL_TABLE +0x18, +0x1c, +0x60).
#define AMB_LIST_COUNT 3
#define PARTS_POS_STRIDE 0x10           /* ALL_TABLE.partsPos: one float[4] qword per part */

// The bank's draw list `which` (0 opaque, 1 translucent, 2 after the particles).
static u_int AmbList(const ALL_TABLE *tbl, int which)
{
    switch (which) {
    case 0:
        return tbl->objOpaque;
    case 1:
        return tbl->objTrans;
    default:
        return tbl->objAfter;
    }
}

// CT3 0x01895f00
// Every draw object of the scene's three lists gets the EE address of its part's ALL_TABLE.partsPos
// qword at DRAW_OBJ_POS: the ambient RGB (alpha in w) the VU1 light programs add to each vertex colour
// (scene_init, once per loaded scene). The word stays an EE address (BankEePtr reads it; open-items 1 (h)).
void ssSetAmbColorToAllObj(int scene)
{
    const ALL_TABLE *tbl = All_table_TBL[scene];
    u_int partsPos = tbl->partsPos;
    int which;

    for (which = 0; which < AMB_LIST_COUNT; which++) {
        const u_int *list = BANK_PTR(const u_int *, scene, AmbList(tbl, which));
        int i;

        if (list == NULL)
            continue;               // PORT: a 0 list word (the ROM reads EE address 0)
        for (i = 0; list[i] != 0; i++) {
            const DRAW_OBJ_ENTRY *ent = BANK_PTR(const DRAW_OBJ_ENTRY *, scene, list[i]);
            u_char *obj = BANK_PTR(u_char *, scene, ent->obj);

            *(u_int *)(obj + DRAW_OBJ_POS) = partsPos + (u_int)ent->parts * PARTS_POS_STRIDE;
        }
    }
}

// ---- ambient colour ---------------------------------------------------------------------------------------

#define AMB_KEEP (-1.0f)                /* ssSetAmbientColor: leave this channel */

// CT3 0x01896530
// Every part of the bank gets the ambient RGBA (ALL_TABLE.partsPos, baseobjCount qwords); AMB_KEEP keeps a
// channel (Character_Init, ssChangeCharAmbientColor and tnChangeCharAmbientColorSet).
void ssSetAmbientColor(float r, float g, float b, float a, int bank)
{
    const ALL_TABLE *tbl = All_table_TBL[bank];
    int count = tbl->baseobjCount;
    float *pos = BANK_PTR(float *, bank, tbl->partsPos);
    int i;

    for (i = 0; i < count; i++, pos += 4) {
        if (r != AMB_KEEP)
            pos[0] = r;
        if (g != AMB_KEEP)
            pos[1] = g;
        if (b != AMB_KEEP)
            pos[2] = b;
        if (a != AMB_KEEP)
            pos[3] = a;
    }
}

// ---- part motion blur -------------------------------------------------------------------------------------

PART_BLUR_MAX_OBJ *PartBlurMaxObj[BANK_MAX][PART_BLUR_MAX_OBJ_MAX]; /* bss 0x01a42280 */

// CT3 0x018965e0
// The copies' colour: the part's RGB, its alpha lowered by PART_BLUR_FADE * i / (ringMax + 1) for the i-th
// copy back from the newest, clamped to +-255. Every pass writes the one shared colour, so the oldest
// copy in use (or the ringMax-th) decides it (ROM).
void ssPartMotionBlurUpdateAlpha(PART_BLUR_MAX_OBJ *max)
{
    const float *src = max->ambColor;
    float r = src[0], g = src[1], b = src[2], a = src[3];
    u_char idx = max->tail;
    int i;

    for (i = 1; i <= max->ringMax && idx != max->head; i++) {
        float alpha;

        if (idx == 0)
            idx = (u_char)(max->ringMax + 1);
        idx--;
        alpha = a - ((float)i * PART_BLUR_FADE) / ((float)max->ringMax + 1.0f);
        if (alpha < -255.0f)
            alpha = -255.0f;
        else if (alpha > 255.0f)
            alpha = 255.0f;
        max->color[0] = r;
        max->color[1] = g;
        max->color[2] = b;
        max->color[3] = alpha;
    }
}

// CT3 0x018968d0
// The copies in use in the record's ring (head..tail, wrapping at ringMax + 1).
int ssGetNumBlurDrawObj(const PART_BLUR_MAX_OBJ *max)
{
    u_int head = max->head;
    u_int tail = max->tail;

    if (head < tail)
        return (int)(tail - head);
    if (head == tail)
        return 0;
    return (int)(tail + ((u_int)(max->ringMax + 1) & 0xff) - head);
}

// CT3 0x01896920
// The draw objects the scene's blur objects of `mode` add to its draw table (main_rtn's passes size their
// tables with it).
int ssGetNumBlurDrawObjForScene(int scene, int mode)
{
    int n = NumPartBlurObj[mode][scene];
    int total = 0;
    int i;

    for (i = 0; i < n; i++)
        total += ssGetNumBlurDrawObj(ALLpartBlurObj[mode][scene][i]->max);
    return total;
}

// One ring copy of the blur object's entry (Push_Back and tiOneFrameMotionBlurUpdate share the body): the
// two matrices, the draw object's first 0x80 bytes with its matrix words (and, given colorEE, its POS word)
// pointing into the copy, and the entry with its object and draw control redirected to the copy's.
static void BlurFillCopy(u_char *copy, const PART_BLUR_OBJ *o, const float (*m58)[4], const float (*m5c)[4],
                         u_int colorEE, int bank)
{
    const DRAW_OBJ_ENTRY *src = o->entry;
    DRAW_OBJ_ENTRY *ent = (DRAW_OBJ_ENTRY *)(copy + BLUR_COPY_ENTRY);
    u_char *obj = copy + BLUR_COPY_OBJ;
    u_int ee;

    memcpy(copy + BLUR_COPY_MAT58, m58, 16 * sizeof(float));
    memcpy(copy + BLUR_COPY_MAT5C, m5c, 16 * sizeof(float));
    memcpy(obj, BANK_PTR(const u_char *, bank, src->obj), BLUR_COPY_OBJ_SIZE);
    if (colorEE != 0)
        memcpy(obj + DRAW_OBJ_POS, &colorEE, sizeof(colorEE));
    ee = BankEeAddr(bank, copy + BLUR_COPY_MAT58);
    memcpy(obj + DRAW_OBJ_W58, &ee, sizeof(ee));
    ee = BankEeAddr(bank, copy + BLUR_COPY_MAT5C);
    memcpy(obj + DRAW_OBJ_W5C, &ee, sizeof(ee));
    *ent = *src;
    ent->obj = BankEeAddr(bank, obj);
    ent->drawCtrlArgs = BankEeAddr(bank, o->head + PART_BLUR_OBJ_ARGS);
    ent->drawCtrl = BankEeAddr(bank, o->head + PART_BLUR_OBJ_DRAWCTRL);
}

// The matrix a draw object's +0x58 / +0x5c word points at.
static const float (*BlurObjMatrix(int bank, const DRAW_OBJ_ENTRY *ent, int word))[4]
{
    const u_char *obj = BANK_PTR(const u_char *, bank, ent->obj);
    u_int ee;

    memcpy(&ee, obj + word, sizeof(ee));
    return BANK_PTR(const float (*)[4], bank, ee);
}

// CT3 0x01896720
// TRAIL mode, per frame: each blur object's entry, as it is drawn now, becomes the ring's newest copy (the
// copy's POS word points at the record's shared colour); a full ring drops its oldest.
void ssPartMotionBlurPush_Back(PART_BLUR_MAX_OBJ *max)
{
    int bank = max->bank;                               /* PORT: the ROM's words are EE addresses */
    u_int colorEE = BankEeAddr(bank, max->color);
    u_int ring = (u_int)(max->ringMax + 1) & 0xff;
    u_char tail = max->tail;
    int i;

    for (i = 0; i < max->count; i++) {
        const PART_BLUR_OBJ *o = &max->objs[i];

        BlurFillCopy(o->copies + tail * PART_BLUR_COPY_SIZE, o, BlurObjMatrix(bank, o->entry, DRAW_OBJ_W58),
                     BlurObjMatrix(bank, o->entry, DRAW_OBJ_W5C), colorEE, bank);
    }
    max->tail = (u_char)((tail + 1) % ring);
    if (max->tail == max->head)
        max->head = (u_char)((max->head + 1) % ring);
}

// CT3 0x01897430
// ONE_FRAME mode, per frame: the copies' colour is the part's with its alpha scaled by alphaPercent around
// -128; copy j of ringMax gets the part's matrix j / ringMax of the way from last frame's to this frame's
// (both of its matrices), so the ring smears one frame's motion. This frame's matrix becomes prevMatrix.
void tiOneFrameMotionBlurUpdate(int bank, PART_BLUR_MAX_OBJ *max)
{
    const MODEL_PARTS *part = &((const MODEL_PARTS *)pModelPartsHeader[bank])[max->parts];
    float cur[4][4];
    float m[4][4];
    int i, j, r, c;

    memcpy(max->color, max->ambColor, sizeof(max->color));
    max->color[3] = (max->color[3] + 128.0f) * (max->alphaPercent / 100.0f) - 128.0f;
    memcpy(cur, part->matrix340, sizeof(cur));
    for (i = 0; i < max->count; i++) {
        const PART_BLUR_OBJ *o = &max->objs[i];

        for (j = 0; j < max->ringMax; j++) {
            float f = (float)j / (float)max->ringMax;

            for (r = 0; r < 4; r++)
                for (c = 0; c < 4; c++)
                    m[r][c] = max->prevMatrix[r][c] + (cur[r][c] - max->prevMatrix[r][c]) * f;
            BlurFillCopy(o->copies + j * PART_BLUR_COPY_SIZE, o, m, m, 0, bank);
        }
    }
    memcpy(max->prevMatrix, cur, sizeof(cur));
}

// The record's entries get back the POS word and draw control they had before the blur (a mode other
// than TRAIL changed them; ssPartMotionBlurEnd and _AllEnd).
static void BlurRestoreEntries(const PART_BLUR_MAX_OBJ *max, int bank)
{
    int j;

    for (j = 0; j < max->count; j++) {
        DRAW_OBJ_ENTRY *ent = max->objs[j].entry;
        GA_DRAWCTRL_ARGS *args = BANK_PTR(GA_DRAWCTRL_ARGS *, bank, ent->drawCtrlArgs);

        memcpy(BANK_PTR(u_char *, bank, ent->obj) + DRAW_OBJ_POS, &max->ambPos, sizeof(max->ambPos));
        *args = max->args;
        gaSetupDrawCtrlData(BANK_PTR(GA_DRAWCTRL_DATA *, bank, ent->drawCtrl), *args);
    }
}

// CT3 0x01896d50
// Per frame (main): the blurred parts of the drawn global banks always; of the other drawn banks (not
// behaviour banks) unless paused (PAUSE_RUNNING still runs). A group of banks counts once. A record in
// ONE_FRAME mode spreads its copies over the frame's motion; the others push this frame's copy and fade.
// The ROM reads each bank's records to the 0 slot; PORT: and stops at the bank's last.
void ssPartMotionBlurUpdate(void)
{
    PART_BLUR_MAX_OBJ *max;
    int bank;
    int i;

    for (bank = 0; bank < BANK_SCENE_MAX; bank++) {
        if (BankInfo[bank].drawOn == 0)
            continue;
        for (i = 0; i < PART_BLUR_MAX_OBJ_MAX && (max = PartBlurMaxObj[bank][i]) != NULL; i++) {
            if (max->mode == PART_BLUR_MODE_ONE_FRAME) {
                tiOneFrameMotionBlurUpdate(bank, max);
            } else {
                ssPartMotionBlurPush_Back(max);
                ssPartMotionBlurUpdateAlpha(max);
            }
        }
        bank += BankInfo[bank].groupCount - 1;
    }
    if (pause != 0 && (pause & PAUSE_RUNNING) == 0)
        return;
    for (bank = BANK_SCENE_MAX; bank < BANK_MAX; bank++) {
        if (BankInfo[bank].drawOn == 0 || BankInfo[bank].type == BANK_TYPE_BEHAVIOR)
            continue;
        for (i = 0; i < PART_BLUR_MAX_OBJ_MAX && (max = PartBlurMaxObj[bank][i]) != NULL; i++) {
            if (max->mode == PART_BLUR_MODE_ONE_FRAME) {
                tiOneFrameMotionBlurUpdate(bank, max);
            } else {
                ssPartMotionBlurPush_Back(max);
                ssPartMotionBlurUpdateAlpha(max);
            }
        }
        bank += BankInfo[bank].groupCount - 1;
    }
}

// CT3 0x01896f00
// scene_end: every blur record of the scene gives its entries back their draw state (only a mode other
// than TRAIL does, ROM) -- the part's POS word, the draw control settings and the GS data built from them
// -- and its three blocks go back to gHeapJunc. The scene's tables start empty again.
void ssPartMotionBlurAllEnd(int scene)
{
    int i;

    for (i = 0; i < PART_BLUR_MAX_OBJ_MAX; i++) {        // PORT: the ROM reads to the 0 slot only
        PART_BLUR_MAX_OBJ *max = PartBlurMaxObj[scene][i];
        PART_BLUR_OBJ *objs;
        u_char *copies;

        if (max == NULL)
            break;
        objs = max->objs;
        copies = objs[0].copies;
        if (max->mode != PART_BLUR_MODE_TRAIL)
            BlurRestoreEntries(max, scene);
        _SunFree(&gHeapJunc, (void **)&copies);
        _SunFree(&gHeapJunc, (void **)&objs);
        _SunFree(&gHeapJunc, (void **)&max);
    }
    PartBlurMaxObj[scene][0] = NULL;
    ALLpartBlurObj[0][scene][0] = NULL;
    ALLpartBlurObj[1][scene][0] = NULL;
    NumPartBlurObj[0][scene] = 0;
    NumPartBlurObj[1][scene] = 0;
}

// CT3 0x01897e30
// system_init: no bank has blur objects or records, and no character's ambient colour is changing.
void ssPartMotionBlurGlobalInit(void)
{
    int bank;
    int i;

    for (bank = 0; bank < BANK_MAX; bank++) {
        ALLpartBlurObj[0][bank][0] = NULL;
        ALLpartBlurObj[1][bank][0] = NULL;
        NumPartBlurObj[0][bank] = 0;
        NumPartBlurObj[1][bank] = 0;
        PartBlurMaxObj[bank][0] = NULL;
    }
    for (i = 0; i < CHAR_AMB_CHANGE_MAX; i++) {
        memset(&CharAmbChangeFlag[i], 0, offsetof(CHAR_AMB_CHANGE, shadow));
        CharAmbChangeColor[i] = 0;
    }
}

// ---- particles ----------------------------------------------------------------------------------------------

u_char **ParticleData[BANK_MAX];        /* sbss 0x01940a10 */

// CT3 0x0189a8d0
// The bank's particle emitters: the length of its ALL_TABLE.particles list (0 without one).
int ssGetNumParticleEmit(int bank)
{
    const u_int *list = BANK_PTR(const u_int *, bank, All_table_TBL[bank]->particles);
    int n = 0;

    if (list == NULL)
        return 0;
    while (list[n] != 0)
        n++;
    return n;
}

// CT3 0x0189a920
// scene_end: the scene's per-emitter draw object and record pointers are cleared and both tables go
// back to gHeapJunc (ssParticleInit allocated them).
void ssParticleDeInit(int scene)
{
    const u_int *list;
    int i;

    if (ssGetNumParticleEmit(scene) == 0)
        return;
    list = BANK_PTR(const u_int *, scene, All_table_TBL[scene]->particles);
    if (list == NULL)
        return;
    for (i = 0; list[i] != 0; i++) {
        ParticleObject[scene][i] = NULL;
        ParticleData[scene][i] = NULL;
    }
    _SunFree(&gHeapJunc, (void **)&ParticleObject[scene]);
    _SunFree(&gHeapJunc, (void **)&ParticleData[scene]);
}

// ---- screen effects -----------------------------------------------------------------------------------------

// CT3 0x0189d900
// The trigger script's screen blur off: back to the normal pass, if the blur pass is the one on.
void ssTrigScreenBlurEnd(void)
{
    if (DrawMode == DRAW_MODE_BLUR)
        DrawMode = DRAW_MODE_NORMAL;
}

// The Z clear's draw control (gaSetupDrawCtrlData's 29 argument slots from the disassembly at 0x0189dec0):
// flat, untextured, alpha test NEVER with AFAIL 2 (ZB_ONLY), depth test ALWAYS: the frame-clear sprite
// writes its depth and no colour.
static const GA_DRAWCTRL_ARGS ZBUF_CLEAR_DRAWCTRL_ARGS = {
    0, 0, 0, 0, 0, 0, 0, 0, /* PRMODE: all off */
    1, 0, 0, 2, 0, 0, 1, 1, /* TEST_1: ATE, ATST 0 (NEVER), AFAIL 2 (ZB_ONLY), ZTE, ZTST 1 (ALWAYS) */
    0, 1, 0, 1, 0,          /* ALPHA_1: A 0 (Cs), B 1 (Cd), C 0 (As), D 1 (Cd) */
    0,                      /* FBA_1 */
    0, 0, 1, 1, 0, 0,       /* TEX1_1: MMAG 1, MMIN 1 */
    0};                     /* TEXA */

// CT3 0x0189dec0
// GlobalDrawObject, before the global banks: the Z buffer cleared by drawcom_frameclear's sprite drawn
// depth-only.
void ssBGT_ZBufferClear(void)
{
    alignas(16) static GA_DRAWCTRL_DATA drawctrl; /* bss 0x0193e280: drawctrl$91 */

    gaSetupDrawCtrlData(&drawctrl, ZBUF_CLEAR_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl);
    gaBGT_ExecDrawCommandData_VIF1(&drawcom_frameclear);
}

// ---- stubs (tools/reverse/stubgen.py) ----

// ---- reflection maps --------------------------------------------------------------------------------------

#define CUBE_CELL_U 0.25f               /* one face's cell in the cube texture */
#define CUBE_CELL_V 0.2f
#define REFLECT_SCREEN_MARGIN 100.0f    /* ssPointCheckWithinScreen's margin for the part's pos */

// v.xyz scaled to length 1 (VU0: vrsqrt Q, vf0w, sum; vmulq.xyz); w kept.
static void NormalizeXYZ(float *v)
{
    float q = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= q;
    v[1] *= q;
    v[2] *= q;
}

// out = m * v, the four rows of m weighted by v's x, y, z, w (VU0 vmula / vmadda / vmaddw).
static void ApplyMatrix4(float *out, float (*m)[4], const float *v)
{
    float r[4];
    int i;

    for (i = 0; i < 4; i++)
        r[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
    memcpy(out, r, sizeof(r));
}

// The unit vector from the eye to p (xyz; w is p's).
static void EyeToPoint(float *out, const float *p, const float *eye)
{
    out[0] = p[0] - eye[0];
    out[1] = p[1] - eye[1];
    out[2] = p[2] - eye[2];
    out[3] = p[3];
    NormalizeXYZ(out);
}

// CT3 0x018951d0
// The texture coordinates of a point (u, v in 0..1 on its face) in the cube texture: the face's cell of the
// cross (CUBE_CELL_U x CUBE_CELL_V), mapped into the map's rectangle; st: the vertex's ST pair.
void ssSetTexUVforCubeMap(float u, float v, float u0, float v0, float w, float h, float *st, int face)
{
    float cu, cv;

    switch (face) {
    case CUBE_FACE_NEG_Z:
        cu = 0.25f;
        cv = 0.4f;
        break;
    case CUBE_FACE_NEG_X:
        cu = 0.5f;
        cv = 0.4f;
        break;
    case CUBE_FACE_POS_Y:
        cu = 0.25f;
        cv = 0.6f;
        break;
    case CUBE_FACE_POS_X:
        cu = 0.0f;
        cv = 0.4f;
        break;
    case CUBE_FACE_NEG_Y:
        cu = 0.25f;
        cv = 0.2f;
        break;
    case CUBE_FACE_POS_Z:
        cu = 0.75f;
        cv = 0.4f;
        break;
    default:
        cu = 0.0f;
        cv = 0.0f;
        break;
    }
    st[0] = u0 + w * (cu + u * CUBE_CELL_U);
    st[1] = v0 + h * (cv + v * CUBE_CELL_V);
}

// CT3 0x018952f0
// Where the reflected vector r meets its face, 0..1 each way: the two other axes over the face's axis.
void ssCalcCubeMapUVfromFace(const float *r, float *u, float *v, int face)
{
    float s;

    switch (face) {
    case CUBE_FACE_NEG_Z:
        s = 0.5f / fabsf(r[2]);
        *u = 0.5f - s * r[0];
        *v = 0.5f + s * r[1];
        break;
    case CUBE_FACE_NEG_X:
        s = 0.5f / fabsf(r[0]);
        *u = 0.5f + s * r[2];
        *v = 0.5f + s * r[1];
        break;
    case CUBE_FACE_POS_Y:
        s = 0.5f / fabsf(r[1]);
        *u = 0.5f - s * r[0];
        *v = 0.5f + s * r[2];
        break;
    case CUBE_FACE_POS_X:
        s = 0.5f / fabsf(r[0]);
        *u = 0.5f - s * r[2];
        *v = 0.5f + s * r[1];
        break;
    case CUBE_FACE_NEG_Y:
        s = 0.5f / fabsf(r[1]);
        *u = 0.5f - s * r[0];
        *v = 0.5f - s * r[2];
        break;
    case CUBE_FACE_POS_Z:
        s = 0.5f / fabsf(r[2]);
        *u = 0.5f + s * r[0];
        *v = 0.5f + s * r[1];
        break;
    default:
        break;                          // the ROM leaves u, v as they were
    }
}

// CT3 0x018954a0
// The face the vector r points at: its longest axis (ties go to z, then y), by its sign.
int ssGetReflectFace(const float *r)
{
    float ax = fabsf(r[0]);
    float ay = fabsf(r[1]);
    float az = fabsf(r[2]);

    if (ay < ax) {
        if (az < ax)
            return r[0] < 0.0f ? CUBE_FACE_NEG_X : CUBE_FACE_POS_X;
        return r[2] < 0.0f ? CUBE_FACE_NEG_Z : CUBE_FACE_POS_Z;
    }
    if (az < ay)
        return r[1] < 0.0f ? CUBE_FACE_NEG_Y : CUBE_FACE_POS_Y;
    return r[2] < 0.0f ? CUBE_FACE_NEG_Z : CUBE_FACE_POS_Z;
}

// CT3 0x018955c0
// The reflection of the eye ray `incident` (unit, eye to surface) about `normal`: out.xyz = unit(normal *
// 2(normal . incident) - incident), out.w = normal.w. Only for a surface facing the eye (the dot <= 0);
// returns 0, out untouched, otherwise.
int ssGetReflectVector(float *out, float *normal, float *incident)
{
    float d = gaInnerProductXYZ(normal, incident) * 2.0f;
    float r[4];

    if (!(d <= 0.0f))
        return 0;
    r[0] = normal[0] * d - incident[0];
    r[1] = normal[1] * d - incident[1];
    r[2] = normal[2] * d - incident[2];
    r[3] = normal[3];
    NormalizeXYZ(r);
    memcpy(out, r, sizeof(r));
    return 1;
}

// CT3 0x01895680
// A plane map: every vertex facing the eye (in the part's frame) gets ST from its reflected vector's x and y,
// mapped from -1..1 into the map's rectangle (x mirrored). PORT: bank, to read the entry's EE words.
void ssCalcReflectPlaneMap(int bank, const REFLECT_ENTRY *ent, const float *eye)
{
    const REFLECT_MAP *maps = BANK_PTR(const REFLECT_MAP *, bank, ent->maps);
    int count = ent->count;
    int i;

    for (i = 0; i < count; i++) {
        const REFLECT_MAP *map = &maps[i];
        u_char *obj = BANK_PTR(u_char *, bank, map->obj);
        float (*pos)[4] = BANK_PTR(float (*)[4], bank, *(u_int *)(obj + REFLECT_OBJ_POS));
        float (*nrm)[4] = (float (*)[4])(BANK_PTR(u_char *, bank, *(u_int *)(obj + REFLECT_OBJ_NORMAL))
                                         + REFLECT_PACKET_ST);
        float (*st)[2] = (float (*)[2])(BANK_PTR(u_char *, bank, *(u_int *)(obj + DRAW_OBJ_PACKET))
                                        + REFLECT_PACKET_ST);
        int verts = *(int *)(obj + REFLECT_OBJ_VERTS);
        int v;

        for (v = 0; v < verts; v++) {
            float inc[4], r[4];

            EyeToPoint(inc, pos[v], eye);
            if (!ssGetReflectVector(r, nrm[v], inc))
                continue;
            st[v][0] = map->u0 + (1.0f - r[0]) * 0.5f * map->w;
            st[v][1] = map->v0 + (r[1] + 1.0f) * 0.5f * map->h;
        }
    }
}

// CT3 0x01895820
// A cube map: each triangle facing the eye picks its face from the reflection at its centre (turned into the
// world by m, the part's matrix); its three vertices facing the eye get ST on that face. PORT: bank.
void ssCalcReflectCubeMap(int bank, const REFLECT_ENTRY *ent, const float *eye, float (*m)[4])
{
    const REFLECT_MAP *maps = BANK_PTR(const REFLECT_MAP *, bank, ent->maps);
    int count = ent->count;
    int i;

    for (i = 0; i < count; i++) {
        const REFLECT_MAP *map = &maps[i];
        u_char *obj = BANK_PTR(u_char *, bank, map->obj);
        REFLECT_TRI *tris = BANK_PTR(REFLECT_TRI *, bank, map->tris);
        float (*pos)[4] = BANK_PTR(float (*)[4], bank, *(u_int *)(obj + REFLECT_OBJ_POS));
        float (*nrm)[4] = (float (*)[4])(BANK_PTR(u_char *, bank, *(u_int *)(obj + REFLECT_OBJ_NORMAL))
                                         + REFLECT_PACKET_ST);
        float (*st)[2] = (float (*)[2])(BANK_PTR(u_char *, bank, *(u_int *)(obj + DRAW_OBJ_PACKET))
                                        + REFLECT_PACKET_ST);
        int t;

        for (t = 0; t < map->triCount; t++) {
            REFLECT_TRI *tri = &tris[t];
            float inc[4], r[4];
            int face;
            int k;

            EyeToPoint(inc, tri->center, eye);
            if (!ssGetReflectVector(r, tri->normal, inc))
                continue;
            ApplyMatrix4(r, m, r);
            face = ssGetReflectFace(r);
            for (k = 0; k < 3; k++) {
                u_int vi = tri->vert[k];
                float u, v;

                EyeToPoint(inc, pos[vi], eye);
                if (!ssGetReflectVector(r, nrm[vi], inc))
                    continue;
                ApplyMatrix4(r, m, r);
                ssCalcCubeMapUVfromFace(r, &u, &v, face);
                ssSetTexUVforCubeMap(u, v, map->u0, map->v0, map->w, map->h, st[vi], face);
            }
        }
    }
}

// CT3 0x01895c10
// Per frame (main): the reflective parts of every drawn bank (only the global ones while paused, unless
// PAUSE_RUNNING), not behaviour banks, a group once. A part enabled by its trigger and on screen (or
// REFLECT_FLAG_ALWAYS) has its maps re-mapped from the eye in its frame: the bank's own camera for a type 0
// bank, the shared camera's for the others.
void ReflectMapUpdate(void)
{
    float sharedEye[4], bankEye[4], localEye[4];
    float inv[4][4];
    int banks;
    int bank;

    banks = (pause == 0 || (pause & PAUSE_RUNNING) != 0) ? BANK_MAX : BANK_SCENE_MAX;
    asViewMatrix2PosRUF(&Cam[CAMERA_SHARED].view[0][0], sharedEye, NULL, NULL, NULL);
    for (bank = 0; bank < banks; bank++) {
        const u_int *list;
        const float *eye;
        CAMERA *cam;
        int i;

        if (BankInfo[bank].drawOn == 0 || BankInfo[bank].type == BANK_TYPE_BEHAVIOR)
            continue;
        list = BANK_PTR(const u_int *, bank, All_table_TBL[bank]->reflects);
        cam = &Cam[asCheckBankID(bank)];
        if (BankInfo[bank].type == BANK_TYPE_0) {
            asViewMatrix2PosRUF(&cam->view[0][0], bankEye, NULL, NULL, NULL);
            eye = bankEye;
        } else {
            eye = sharedEye;
        }
        for (i = 0; list != NULL && list[i] != 0; i++) {   // PORT: a 0 list word (the ROM reads EE 0)
            const REFLECT_ENTRY *ent = BANK_PTR(const REFLECT_ENTRY *, bank, list[i]);
            u_char *header = pModelPartsHeader[bank];
            MODEL_PARTS *part = (MODEL_PARTS *)(header + ent->part * (int)sizeof(MODEL_PARTS));
            float (*m)[4];

            if (!ssTrigCheckEnable(ent->part, bank))
                continue;
            if ((ent->flags & REFLECT_FLAG_ALWAYS) == 0
                && !ssPointCheckWithinScreen(REFLECT_SCREEN_MARGIN, part->pos, cam->view))
                continue;
            if (part->kind == PARTS_KIND_REFLECT_ROOT)
                m = ((MODEL_PARTS *)header)->matrix200;
            else
                m = part->matrix200;
            gaInversMatrix(inv, m);
            ApplyMatrix4(localEye, inv, eye);
            if (ent->flags & REFLECT_FLAG_PLANE)
                ssCalcReflectPlaneMap(bank, ent, localEye);
            else
                ssCalcReflectCubeMap(bank, ent, localEye, m);
        }
        bank += BankInfo[bank].groupCount - 1;
    }
}

// ---- the 2D particles (2026-11-12) ----

SS_2DP_INFO ss2DpInfo;                  /* data 0x01939c90 */

// data 0x01938de0: the packet each record starts as (ss2DParticlePush_Back copies it). Its vertices'
// UV and XY are overwritten; RGBAQ and the XYZ2 high words (Z, the ADC bit of the fan's first two) stay.
u_int SS_draw_diamond[SS_DIAMOND_PACKET_QW * 4] = {
    0x10000014, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x50000013,
    0x00008006, 0x3002c000, 0x00000431, 0x00000000,
    0x00000080, 0x00000080, 0x00000080, 0x00000040,
    0x00001000, 0x00000e00, 0x00000000, 0x00000000,
    0x00009000, 0x00008e00, 0x0fffffe0, 0x00008000,
    0x00000080, 0x00000080, 0x00000080, 0x00000000,
    0x00001000, 0x00001c00, 0x00000000, 0x00000000,
    0x00009000, 0x00009000, 0x0fffffe0, 0x00008000,
    0x00000080, 0x00000080, 0x00000080, 0x00000000,
    0x00000010, 0x00000e00, 0x00000000, 0x00000000,
    0x00008e00, 0x00008e00, 0x0fffffe0, 0x00000000,
    0x00000080, 0x00000080, 0x00000080, 0x00000000,
    0x00001000, 0x00000010, 0x00000000, 0x00000000,
    0x00009000, 0x00008c00, 0x0fffffe0, 0x00000000,
    0x00000080, 0x00000080, 0x00000080, 0x00000000,
    0x00002000, 0x00000e00, 0x00000000, 0x00000000,
    0x00009200, 0x00008e00, 0x0fffffe0, 0x00000000,
    0x00000080, 0x00000080, 0x00000080, 0x00000000,
    0x00001000, 0x00001c00, 0x00000000, 0x00000000,
    0x00009000, 0x00009000, 0x0fffffe0, 0x00000000,
};

// CodeWarrior's float -> unsigned conversion (values from 2^31 lose the bias and get bit 31 back).
static u_int ss2DpFloatToU(float f)
{
    if (f < 2147483648.0f)
        return (u_int)(int)f;
    return (u_int)(int)(f - 2147483648.0f) | 0x80000000u;
}

// __fixsfdi to the 16 bits a GS coordinate keeps.
static u_int ss2DpFix16(float f)
{
    return (u_int)(long long)f & 0xffff;
}

// CT3 0x0189cff0
// Projects the object's vertices through its matrix (column vectors, VU0's mula / madd chain) and the
// screen (centre + scale * d * x / z; y flipped), and writes their bounding rectangle, clamped to
// 0..512 x 0..448. From the disassembly (Ghidra's decompilation garbles the clamps): a minimum that
// reached 0 and a maximum that reached the edge stop moving. Returns the first vertex's view z.
// PORT: the object's EE words resolve in info->bank. The 512 x 448 space is the ROM's; the host's
// aspect comes from the renderer, not from here.
float ssGetScreenRange(SS_WAVE_OBJ *obj, SS_2DP_INFO *info)
{
    const float (*m)[4] = (const float (*)[4])BankEePtr(info->bank, obj->matrix);
    const float (*p)[4] = (const float (*)[4])BankEePtr(info->bank, obj->verts);
    float minX = 513.0f, minY = 449.0f, maxX = -1.0f, maxY = -1.0f;
    float depth = 0.0f, v[4], sx, sy;
    int i, j;
    for (i = 0; i < (int)obj->vertCount; i++) {
        for (j = 0; j < 4; j++)
            v[j] = m[0][j] * p[i][0] + m[1][j] * p[i][1] + m[2][j] * p[i][2] + m[3][j] * p[i][3];
        if (i == 0)
            depth = v[2];
        sx = screen_0[GA_SCREEN_CX] + screen_0[GA_SCREEN_SCALE_X] * ((screen_0[GA_SCREEN_PROJ] * v[0]) / v[2]);
        sy = screen_0[GA_SCREEN_CY] - screen_0[GA_SCREEN_SCALE_Y] * ((screen_0[GA_SCREEN_PROJ] * v[1]) / v[2]);
        if (minX != 0.0f && sx < minX)
            minX = sx < 0.0f ? 0.0f : sx;
        if (minY != 0.0f && sy < minY)
            minY = sy < 0.0f ? 0.0f : sy;
        if (maxX != 512.0f && !(sx <= maxX))
            maxX = sx <= 512.0f ? sx : 512.0f;
        if (maxY != 448.0f && !(sy <= maxY))
            maxY = sy <= 448.0f ? sy : 448.0f;
    }
    info->x = (short)ss2DpFloatToU(minX);
    info->y = (short)ss2DpFloatToU(minY);
    info->w = (short)ss2DpFloatToU(maxX - minX);
    info->h = (short)ss2DpFloatToU(maxY - minY);
    return depth;
}

// CT3 0x0189c150
// Record i's Y and V for this frame: the diamond's centre at its y, the bottom and top size pixels away;
// the V's follow y + dv, and near the screen's top edge they are clamped to the first texels.
void ss2DParticleSetDrawData(int i)
{
    SS_2DP_REC *r = &ss2DpData[i];
    int y = r->y, s = r->size;
    u_int yc = ss2DpFix16((float)(y + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    float vc, vb, vt;
    r->vtx[0].y = yc;
    r->vtx[1].y = ss2DpFix16((float)(y + s + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    r->vtx[2].y = yc;
    r->vtx[3].y = ss2DpFix16((float)(y - s + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    r->vtx[4].y = yc;
    r->vtx[5].x = r->vtx[1].x;
    r->vtx[5].y = r->vtx[1].y;
    vc = ((float)y + r->dv) * SS_2DP_SUBPIXEL;
    vb = vc + (float)s * SS_2DP_SUBPIXEL;
    if (vb < SS_2DP_V_FLOOR) {
        r->vtx[0].v = SS_2DP_V_CENTRE_TOP;
        r->vtx[1].v = SS_2DP_V_BOTTOM_TOP;
        r->vtx[2].v = SS_2DP_V_CENTRE_TOP;
        r->vtx[3].v = SS_2DP_V_TOP_TOP;
        r->vtx[4].v = SS_2DP_V_CENTRE_TOP;
    } else {
        vt = vc - (float)s * SS_2DP_SUBPIXEL;
        if (vc < SS_2DP_V_FLOOR) {
            r->vtx[0].v = SS_2DP_V_CENTRE_TOP;
            r->vtx[1].v = ss2DpFix16(vb);
            r->vtx[2].v = SS_2DP_V_CENTRE_TOP;
            r->vtx[3].v = SS_2DP_V_TOP_TOP;
            r->vtx[4].v = SS_2DP_V_CENTRE_TOP;
        } else {
            u_int v = ss2DpFix16(vc);
            r->vtx[0].v = v;
            r->vtx[1].v = ss2DpFix16(vb);
            r->vtx[2].v = v;
            r->vtx[3].v = vt < SS_2DP_V_FLOOR ? SS_2DP_V_TOP_TOP : ss2DpFix16(vt);
            r->vtx[4].v = v;
        }
    }
    r->vtx[5].u = r->vtx[1].u;
    r->vtx[5].v = r->vtx[1].v;
}

// One record's step (ssCalc2DParticleOrbit's three loops have the same body): y rises from the
// rectangle's bottom by speed / 10 a frame of age.
static void ss2DParticleStep(int i, short bottom)
{
    SS_2DP_REC *r = &ss2DpData[i];
    r->y = (short)(bottom - (short)(((u_int)r->speed * (u_int)r->age) / 10));
    r->age++;
    ss2DParticleSetDrawData(i);
}

// CT3 0x0189c4b0
// Moves every live record, head to tail (the ring may wrap).
void ssCalc2DParticleOrbit(void)
{
    int i = ss2DpInfo.head, tail = ss2DpInfo.tail, max = ss2DpInfo.max;
    short bottom = (short)(ss2DpInfo.y + ss2DpInfo.h);
    if (i < tail) {
        for (; i < tail; i++)
            ss2DParticleStep(i, bottom);
    } else if (tail < i) {
        for (; i < max; i++)
            ss2DParticleStep(i, bottom);
        for (i = 0; i < tail; i++)
            ss2DParticleStep(i, bottom);
    }
}

// CT3 0x0189c6b0
// The oldest record, dropped from the ring; NULL when it is empty.
SS_2DP_REC *ss2DParticlePop_Front(SS_2DP_REC *data)
{
    SS_2DP_REC *r;
    if (ss2DpInfo.tail == ss2DpInfo.head)
        return NULL;
    r = &data[ss2DpInfo.head];
    ss2DpInfo.head = (u_short)((ss2DpInfo.head + 1) % (u_int)ss2DpInfo.max);
    return r;
}

// CT3 0x0189c700
// 1: record i is live (between head and tail) and older than the life.
int ss2DParticleLifeCheck(u_short i)
{
    u_short head = ss2DpInfo.head, tail = ss2DpInfo.tail;
    if (head == tail)
        return 0;
    if (head < tail) {
        if (i < head || tail <= i)
            return 0;
    } else if (tail < head && tail <= i && i < head) {
        return 0;
    }
    return ss2DpInfo.life < ss2DpData[i].age;
}

// CT3 0x0189c7a0
// Pops the expired records from the head.
void ssCalc2DParticleLifeSpan(void)
{
    while (ss2DParticleLifeCheck(ss2DpInfo.head) && ss2DParticlePop_Front(ss2DpData) != NULL) {
    }
}

// A randomised byte: base * (1 +- rand percent), as the ROM converts it (float -> unsigned, low byte).
static u_int ss2DpRandomise(u_int base, u_int percent)
{
    int r = tnRand((int)(percent << 1));
    float f = (float)base * ((float)(int)(r - percent) / 100.0f + 1.0f);
    return ss2DpFloatToU(f) & 0xff;
}

// CT3 0x0189c7f0
// A new diamond at the tail: x random across the rectangle, at its bottom; speed and size randomised by
// their percents and clamped; the UV shifts random within +-sway / 10 pixels. The packet is the template
// with X and U written (Y and V come each frame). 0: the ring is full.
int ss2DParticlePush_Back(SS_2DP_REC *data)
{
    u_short tail = ss2DpInfo.tail;
    u_int next = (tail + 1u) % (u_int)ss2DpInfo.max;
    SS_2DP_REC *r;
    short x;
    u_int speed, size, sway;
    float du, dv;
    if (next == ss2DpInfo.head)
        return 0;
    x = (short)(ss2DpInfo.x + tnRand(ss2DpInfo.w));
    speed = ss2DpInfo.speed;
    if (ss2DpInfo.speedRand != 0) {
        speed = ss2DpRandomise(speed, ss2DpInfo.speedRand);
        if (speed < SS_2DP_SPEED_MIN)
            speed = SS_2DP_SPEED_MIN;
        else if (speed > SS_2DP_SPEED_MAX)
            speed = SS_2DP_SPEED_MAX;
    }
    size = ss2DpInfo.size;
    if (ss2DpInfo.sizeRand != 0) {
        size = ss2DpRandomise(size, ss2DpInfo.sizeRand);
        if (size < SS_2DP_SIZE_MIN)
            size = SS_2DP_SIZE_MIN;
        else if (size > SS_2DP_SIZE_MAX)
            size = SS_2DP_SIZE_MAX;
    }
    sway = ss2DpInfo.sway;
    du = (float)(int)(sway * (u_int)(tnRand(SS_2DP_SWAY_RAND) - 100)) * SS_2DP_SWAY_SCALE;
    dv = (float)(int)(sway * (u_int)(tnRand(SS_2DP_SWAY_RAND) - 100)) * SS_2DP_SWAY_SCALE;
    r = &data[tail];
    memcpy(r, SS_draw_diamond, sizeof(SS_draw_diamond));
    r->du = du;
    r->dv = dv;
    r->x = x;
    r->y = (short)(ss2DpInfo.y + ss2DpInfo.h);
    r->age = 0;
    r->speed = (u_char)speed;
    r->size = (u_char)size;
    /* XYZ2 x (y = 0 until ss2DParticleSetDrawData): centre, bottom and top share it */
    r->vtx[0].x = ss2DpFix16((float)(x + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    r->vtx[0].y = 0;
    r->vtx[2].x = ss2DpFix16((float)((int)x - (int)size + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    r->vtx[2].y = 0;
    r->vtx[4].x = ss2DpFix16((float)((int)x + (int)size + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    r->vtx[4].y = 0;
    r->vtx[1].x = r->vtx[3].x = r->vtx[5].x = r->vtx[0].x;
    r->vtx[1].y = r->vtx[3].y = r->vtx[5].y = 0;
    /* UV u (v = 0 until then): the screen x shifted by du */
    r->vtx[0].u = ss2DpFix16(((float)x + du) * SS_2DP_SUBPIXEL);
    r->vtx[0].v = 0;
    r->vtx[2].u = ss2DpFix16((du + (float)((int)x - (int)size)) * SS_2DP_SUBPIXEL);
    r->vtx[2].v = 0;
    r->vtx[4].u = ss2DpFix16((du + (float)((int)x + (int)size)) * SS_2DP_SUBPIXEL);
    r->vtx[4].v = 0;
    r->vtx[1].u = r->vtx[3].u = r->vtx[5].u = r->vtx[0].u;
    r->vtx[1].v = r->vtx[3].v = r->vtx[5].v = 0;
    ss2DpInfo.tail = (u_short)next;
    return 1;
}

// CT3 0x0189cc10
// This frame's emission: max / life records a frame on average (the whole part of the running total's
// step), until the ring is full.
void ssGenerate2DParticle(void)
{
    float rate = (float)ss2DpInfo.max / (float)ss2DpInfo.life;
    int old = ss2DpInfo.frame;
    int n = (int)(rate * (float)(old + 1)) - (int)(rate * (float)old), i;
    ss2DpInfo.frame = old + 1;
    for (i = 0; i < n; i++) {
        if (!ss2DParticlePush_Back(ss2DpData))
            return;
    }
}

// CT3 0x0189cd60
// The wave pass's frame (ssWaveDrawAllObject): unless paused, the rectangle follows the object and the
// size its depth (sizeScale / depth), then emission, expiry and the move.
void ss2DParticle_main(void)
{
    float depth;
    if (pause != 0 && !(pause & PAUSE_RUNNING))
        return;
    if (ss2DpInfo.obj != NULL) {
        depth = ssGetScreenRange(ss2DpInfo.obj, &ss2DpInfo);
        if (0.0f < depth)
            ss2DpInfo.size = (u_char)ss2DpFloatToU(ss2DpInfo.sizeScale / depth);
    }
    ssGenerate2DParticle();
    ssCalc2DParticleLifeSpan();
    ssCalc2DParticleOrbit();
}

// ---- the screen passes' sprite (2026-11-12) ----

SS_SPRITE_PACKET SS_draw_sprite = {     /* data 0x01938d80 */
    {0x10000005, 0x00000000, 0x00000000, 0x00000000,
     0x00000000, 0x00000000, 0x00000000, 0x50000004,
     0x00008001, 0x64000000, 0x00434310, 0x00000000},
    {0x00000006, 0x00000000},
    {0x80808080, 0x3f800000},
    {0x000f000f, 0x00000000},
    {0x7ff87ff8, 0x00fffffe},
    {0x1c0f200f, 0x00000000},
    {0x9bf89ff8, 0x00fffffe},
};

// CT3 0x0189dfc0
// The sprite's colour (Q 1.0).
void ssSetSpriteCol(int r, int g, int b, int a)
{
    SS_draw_sprite.rgbaq[0] = (u_int)a << 24 | (u_int)b << 16 | (u_int)r | (u_int)g << 8;
    SS_draw_sprite.rgbaq[1] = SS_SPRITE_Q_ONE;
}

// CT3 0x0189df70
// The sprite's texels, corner to corner, each sampled at its centre (* 16 + 15).
void ssSetSpriteUV(int u0, int v0, int u1, int v1)
{
    SS_draw_sprite.uv0[0] = ((u_int)(v0 * 16 + SS_SPRITE_UV_CENTRE) & 0xffff) << 16
                          | ((u_int)(u0 * 16 + SS_SPRITE_UV_CENTRE) & 0xffff);
    SS_draw_sprite.uv0[1] = 0;
    SS_draw_sprite.uv1[0] = ((u_int)(v1 * 16 + SS_SPRITE_UV_CENTRE) & 0xffff) << 16
                          | ((u_int)(u1 * 16 + SS_SPRITE_UV_CENTRE) & 0xffff);
    SS_draw_sprite.uv1[1] = 0;
}

// The GS XY word of a screen point: (x + 0.45, y + 0.4) * 16 in doubles (the ROM's fptodp / dpmul),
// truncated, offset by 2048.
static u_int ssSpriteXY(float x, float y)
{
    long long fx = (long long)(((double)x + SS_SPRITE_X_BIAS) * 16.0);
    long long fy = (long long)(((double)y + SS_SPRITE_Y_BIAS) * 16.0);
    return ((u_int)(fx + SS_SPRITE_XY_OFFSET) & 0xffff) | ((u_int)(fy + SS_SPRITE_XY_OFFSET) & 0xffff) << 16;
}

// CT3 0x0189e010
// The sprite's corners in screen pixels and its Z (both corners). PORT: a negative z (undefined in the
// ROM's __fixunssfdi) gives 0.
void ssSetSpritePos(float x0, float y0, float x1, float y1, float z)
{
    u_int zw = z > 0.0f ? (u_int)((unsigned long long)z & SS_SPRITE_Z_MASK) : 0;
    SS_draw_sprite.xyz0[0] = ssSpriteXY(x0, y0);
    SS_draw_sprite.xyz0[1] = zw;
    SS_draw_sprite.xyz1[0] = ssSpriteXY(x1, y1);
    SS_draw_sprite.xyz1[1] = zw;
}

// ---- the diamond passes' draw environment (2026-11-12) ----

alignas(16) static u_char ssTexbuffer[0x200];           /* bss 0x01941900: the screen passes' texture buffer */
alignas(16) static GA_DRAWCTRL_DATA drawctrl_ssSprite;  /* 0x01941bc0: the screen passes' draw control */

// PORT: ssDOFtexture_attr (data 0x01938f80, 0x80 bytes, the first 0x34 a SUN_TEX_ATTR): the frame buffer
// read as a texture, 512 x 448 32-bit, log2 9 x 9, no mips (the same record as tex_attr_A$2750).
static const SUN_TEX_ATTR ssDOFtexture_attr = {512, 448, 9, 9, SCE_GS_PSMCT32, 0, 0x000e0000u, {0, 0, 0, 0}, 0, 0};
#define SS_DOF_TEXTURE_CBP 0xfa800u     /* the ROM's TEX_INFO.cbp for it (no CLUT is read for PSMCT32) */

// The diamonds' draw control: Gouraud, textured, alpha blended, UV; alpha test always passes, depth
// test on (ZTST 1 always); ALPHA_1 (Cs - Cd) * As + Cd; FBA 1; TEX1 LCM 1, bilinear both ways.
static const GA_DRAWCTRL_ARGS SS_DIAMOND_DRAWCTRL_ARGS = {
    1, 1, 0, 1, 0, 1, 0, 0, /* PRMODE: IIP, TME, ABE, FST */
    0, 1, 0, 0, 0, 0, 1, 1, /* TEST_1: ATST 1, ZTE, ZTST 1 */
    0, 1, 0, 1, 0,          /* ALPHA_1: A 0 (Cs), B 1 (Cd), C 0 (As), D 1 (Cd) */
    1,                      /* FBA_1 */
    1, 0, 1, 1, 0, 0,       /* TEX1_1: LCM 1, MMAG 1, MMIN 1 */
    0};                     /* TEXA */

// The frame buffer at fbuf becomes the screen passes' texture (ssTexbuffer from ssDOFtexture_attr; the
// ROM repeats this set-up in every pass). PORT: the record is the host copy.
static void ssSetScreenTexture(u_int fbuf)
{
    TEX_INFO tex;
    memset(&tex, 0, sizeof(tex));
    tex.tim = 0x01938f80u;              /* ssDOFtexture_attr */
    tex.tbp = fbuf;
    tex.cbp = SS_DOF_TEXTURE_CBP;
    sunSetupTBufferDataFrom(ssTexbuffer, tex, &ssDOFtexture_attr);
    sunBGT_ExecTBufferData_VIF1(ssTexbuffer);
}

// The PostRend set-up both diamond passes share: the frame just drawn (ga_draw_fbuf_address) becomes the
// texture, then the draw control.
static void ssDiamondSetDrawEnv(void)
{
    ssSetScreenTexture((u_int)ga_draw_fbuf_address);
    gaSetupDrawCtrlData(&drawctrl_ssSprite, SS_DIAMOND_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_ssSprite);
}

// CT3 0x0189b7e0
// The vaseline diamonds: every record's packet, in order (they are not a ring).
void ssBGT_VaselineDrawDiamond(void)
{
    int i, n = ss2DpInfo.max;
    for (i = 0; i < n; i++)
        gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&ss2DpData[i]);
}

// CT3 0x0189b840
// The vaseline pass's draw environment after the scene (ssDiamondSetDrawEnv).
void ssBGT_VaselineSetDrawEnvPostRend(void)
{
    ssDiamondSetDrawEnv();
}

// CT3 0x0189bff0
// The wave diamonds: the live records' packets, head to tail round the ring.
void ssBGT_WaveDrawDiamond(void)
{
    u_int i = ss2DpInfo.head, tail = ss2DpInfo.tail, max = ss2DpInfo.max;
    while (i != tail) {
        gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&ss2DpData[i]);
        i++;
        if (i == max)
            i = 0;
    }
}

// CT3 0x0189c070
// The wave pass's draw environment after the scene (ssDiamondSetDrawEnv).
void ssBGT_WaveSetDrawEnvPostRend(void)
{
    ssDiamondSetDrawEnv();
}

// ---- the full-screen sprite passes: HQDOF write-back, hazy, ultima, blur, DOF (2026-11-12) ----

alignas(16) static GA_DRAWCTRL_DATA drawctrl_ssSprite2;  /* 0x01941b00: the HQDOF / DOF sprites' draw control */

#define SS_HQDOF_FBUF 0xa8000u          /* main.c's HQDOF_FBUF: the frame the HQDOF samples are drawn into */
#define SS_SCREEN_W 512.0f              /* the ROM's frame, in pixels */
#define SS_SCREEN_H 448.0f
#define SS_SPRITE_GREY 0x80             /* RGB 0x80: the texture unmodulated */
#define SS_SPRITE_ALPHA_QUARTER 0x20    /* As 0x20 of 0x80: a quarter of the source over the frame */

// The sprites' draw control (the disassembly's 29 slots): flat, textured, alpha blended, UV; alpha test
// always passes, depth test on (ZTST 1 always); ALPHA_1 (Cs - Cd) * As + Cd; FBA 1; TEX1 LCM 1, bilinear.
static const GA_DRAWCTRL_ARGS SS_SPRITE_DRAWCTRL_ARGS = {
    0, 1, 0, 1, 0, 1, 0, 0, /* PRMODE: TME, ABE, FST */
    0, 1, 0, 0, 0, 0, 1, 1, /* TEST_1: ATST 1, ZTE, ZTST 1 */
    0, 1, 0, 1, 0,          /* ALPHA_1: A 0 (Cs), B 1 (Cd), C 0 (As), D 1 (Cd) */
    1,                      /* FBA_1 */
    1, 0, 1, 1, 0, 0,       /* TEX1_1: LCM 1, MMAG 1, MMIN 1 */
    0};                     /* TEXA */

// The ultima strips' draw control: as SS_SPRITE_DRAWCTRL_ARGS without blending (ABE 0): the strips
// replace the frame.
static const GA_DRAWCTRL_ARGS SS_OPAQUE_SPRITE_DRAWCTRL_ARGS = {
    0, 1, 0, 0, 0, 1, 0, 0, /* PRMODE: TME, FST */
    0, 1, 0, 0, 0, 0, 1, 1, /* TEST_1: ATST 1, ZTE, ZTST 1 */
    0, 1, 0, 1, 0,          /* ALPHA_1 (unused) */
    1,                      /* FBA_1 */
    1, 0, 1, 1, 0, 0,       /* TEX1_1: LCM 1, MMAG 1, MMIN 1 */
    0};                     /* TEXA */

// drawbuffer_0 over the whole frame: FRAME at the draw buffer (gaBGT_ExecDBufferData_VIF1 fills FBP),
// 512 wide, PSMCT32; ZBUF PSMZ32 with its write mask zMsk; scissor 0..511 x 0..447; no offset, no dither.
static void ssSetFrameDrawBuffer(u_int zMsk)
{
    gaSetupDBufferData(&drawbuffer_0, 0, (u_int)SS_SCREEN_W, SCE_GS_PSMCT32, 0, 0, 0, zMsk, 0, 0,
                       (u_int)SS_SCREEN_W - 1, (u_int)SS_SCREEN_H - 1, 0, 0, 0);
    gaBGT_ExecDBufferData_VIF1(&drawbuffer_0);
}

// CT3 0x0189b510
// main_rtn's HQDOF pass, before its samples: drawbuffer_0 over the whole frame, Z written.
void ssBGT_HQDOF_SetDrawEnvPreRend(void)
{
    ssSetFrameDrawBuffer(0);
}

// CT3 0x0189b390
// After the HQDOF samples: the frame they were drawn into (SS_HQDOF_FBUF) laid over the draw buffer as
// one full-screen sprite at alpha (0..128). The sprite keeps the UVs its last user left (no ssSetSpriteUV).
void ssBGT_HQDOF_WriteBackFB(int alpha)
{
    ssSetFrameDrawBuffer(0);
    ssSetScreenTexture(SS_HQDOF_FBUF);
    gaSetupDrawCtrlData(&drawctrl_ssSprite2, SS_SPRITE_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_ssSprite2);
    ssSetSpriteCol(SS_SPRITE_GREY, SS_SPRITE_GREY, SS_SPRITE_GREY, alpha);
    ssSetSpritePos(0.0f, 0.0f, SS_SCREEN_W, SS_SCREEN_H, 0.0f);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
}

// CT3 0x0189d560
// The hazy pass's draw environment: the frame just drawn (ga_draw_fbuf_address) as the texture, the
// blended sprite control.
void ssBGT_HazySetDrawEnvPostRend(void)
{
    ssSetScreenTexture((u_int)ga_draw_fbuf_address);
    gaSetupDrawCtrlData(&drawctrl_ssSprite, SS_SPRITE_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_ssSprite);
}

#define HAZY_SHIFT_X 2.0f               /* level * 2: the left and right copies' sideways shift */
#define HAZY_SHIFT_Y_SIDE 1.16f         /* level * 1.16: the side copies raised */
#define HAZY_SHIFT_Y_DOWN 1.6f          /* level * 1.6: the third copy lowered */
#define HAZY_TICKS_SCALE 300           /* the fade lasts transTime * 300 / 10 ticks */
#define HAZY_ROUNDS 3

// CT3 0x0189d330
// The haze: the frame laid over itself three times a round, three rounds, at a quarter alpha -- shifted
// left and up, right and up, and down, by amounts proportional to level. With transTime the shift fades
// to 0 over transTime * 30 ticks (passTicks$274 counts them), then the haze ends.
void ssBGT_HazyDrawSprite(int level, int transTime)
{
    static int passTicks;               /* sbss 0x0193e010: passTicks$274 */
    float shiftX, shiftSide, shiftDown;
    int i;

    if (transTime == 0) {
        float l = (float)level;
        shiftX = l * HAZY_SHIFT_X;
        shiftSide = l * HAZY_SHIFT_Y_SIDE;
        shiftDown = l * HAZY_SHIFT_Y_DOWN;
    } else {
        int span;
        float l, t, n, x, side, down;
        passTicks += gNextTicksInterval;
        span = transTime * HAZY_TICKS_SCALE / 10;
        if (span < passTicks) {
            passTicks = 0;
            ssTrigHazyScreenEnd();
            return;
        }
        l = (float)level;
        t = (float)passTicks;
        n = (float)span;
        x = l * HAZY_SHIFT_X;
        side = l * HAZY_SHIFT_Y_SIDE;
        down = l * HAZY_SHIFT_Y_DOWN;
        shiftX = x - x * t / n;
        shiftSide = side - side * t / n;
        shiftDown = down - down * t / n;
    }
    ssSetSpriteCol(SS_SPRITE_GREY, SS_SPRITE_GREY, SS_SPRITE_GREY, SS_SPRITE_ALPHA_QUARTER);
    ssSetSpriteUV(0, 0, (int)SS_SCREEN_W, (int)SS_SCREEN_H);
    for (i = 0; i < HAZY_ROUNDS; i++) {
        ssSetSpritePos(-shiftX, -shiftSide, SS_SCREEN_W - shiftX, SS_SCREEN_H - shiftSide, 0.0f);
        gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
        ssSetSpritePos(shiftX, -shiftSide, shiftX + SS_SCREEN_W, SS_SCREEN_H - shiftSide, 0.0f);
        gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
        ssSetSpritePos(0.0f, shiftDown, SS_SCREEN_W, shiftDown + SS_SCREEN_H, 0.0f);
        gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
    }
}

// CT3 0x0189d820
// The ultima pass's draw environment: the frame on display (ga_disp_fbuf_address, last frame's) as the
// texture, the opaque control.
void ssBGT_UltimaSetDrawEnvPostRend(void)
{
    ssSetScreenTexture((u_int)ga_disp_fbuf_address);
    gaSetupDrawCtrlData(&drawctrl_ssSprite, SS_OPAQUE_SPRITE_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_ssSprite);
}

#define ULTIMA_STRIPS 128               /* 4-pixel columns over the 512-wide frame */
#define ULTIMA_STRIP_W 4
#define ULTIMA_CENTRE_STRIP 80          /* the strip whose wave is fastest (x 320) */
#define ULTIMA_FIRST_FRAME 6            /* *frame on the pass's first sprite frame (5 plain frames first) */
#define ULTIMA_SPRITE_FRAMES 256        /* sprite frames before *frame restarts */
#define ULTIMA_SPEED 64.0f              /* the centre strip's angle step per frame */
#define ULTIMA_SPEED_FALLOFF 0.7f       /* less per strip away from the centre */
#define ULTIMA_AMPLITUDE 1000.0f        /* each strip's offset: sin * 1000 pixels */

// CT3 0x0189d640
// The ultima effect: last frame redrawn as 128 vertical strips, each moved up by how far its wave
// (dy$243, sin of (frame - 5) times a speed that falls off away from strip 80, * 1000) rose since the
// previous frame -- the picture tears into columns sliding at their own pace. The first sprite frame
// clears the waves; past frame 262 *frame goes back to 0 (main's counter restarts the 5 plain frames).
void ssBGT_UltimaDrawSprite(int *frame)
{
    static int dy[ULTIMA_STRIPS];       /* sbss 0x0193e050: dy$243 */
    int prev[ULTIMA_STRIPS];
    int f = *frame;
    int i, x;

    if (f == ULTIMA_FIRST_FRAME) {
        for (i = 0; i < ULTIMA_STRIPS; i++) {
            prev[i] = 0;
            dy[i] = 0;
        }
    }
    for (i = 0, x = 0; i < ULTIMA_STRIPS; i++, x += ULTIMA_STRIP_W) {
        int d = i - ULTIMA_CENTRE_STRIP;
        float s;
        prev[i] = dy[i];
        if (d < 0)
            d = -d;
        s = ahSin((int)((float)(f - 5) * (ULTIMA_SPEED - ULTIMA_SPEED_FALLOFF * (float)d)));
        dy[i] = (int)(ULTIMA_AMPLITUDE * s);
        ssSetSpriteUV(x, 0, x + ULTIMA_STRIP_W, (int)SS_SCREEN_H);
        ssSetSpritePos((float)x, -(float)(dy[i] - prev[i]), (float)(x + ULTIMA_STRIP_W), SS_SCREEN_H, 0.0f);
        gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
    }
    if (f - ULTIMA_FIRST_FRAME > ULTIMA_SPRITE_FRAMES)
        *frame = 0;
}

// CT3 0x0189d9b0
// The blur pass's draw environment: the frame on display (last frame's) as the texture, the blended
// sprite control.
void ssBGT_BlurSetDrawEnvPostRend(void)
{
    ssSetScreenTexture((u_int)ga_disp_fbuf_address);
    gaSetupDrawCtrlData(&drawctrl_ssSprite, SS_SPRITE_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_ssSprite);
}

#define BLUR_ALPHA_BASE 0x30            /* As = level * 5 + 0x30 */
#define BLUR_ALPHA_STEP 5

// CT3 0x0189d940
// Motion blur: last frame laid over this one, whole-screen, at alpha level * 5 + 48.
void ssBGT_BlurDrawSprite(int level)
{
    ssSetSpriteCol(SS_SPRITE_GREY, SS_SPRITE_GREY, SS_SPRITE_GREY, level * BLUR_ALPHA_STEP + BLUR_ALPHA_BASE);
    ssSetSpriteUV(0, 0, (int)SS_SCREEN_W, (int)SS_SCREEN_H);
    ssSetSpritePos(0.0f, 0.0f, SS_SCREEN_W, SS_SCREEN_H, 0.0f);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
}

// The DOF passes' depth slices (ssSetDOFClipRange's near and far, view z): pass 4 is the farthest; the
// focus pass's near is DOF_NEAR_ALL, so it takes everything in front of its far as well.
#define DOF_NEAR_ALL (-10000.0f)
#define DOF_SLICE_1_FAR 500.0f
#define DOF_SLICE_2_FAR 1000.0f
#define DOF_SLICE_3_FAR 1500.0f
#define DOF_SLICE_4_FAR 10000.0f
#define DOF_PASS_FAR 4

// CT3 0x0189dd70
// Before each DOF slice (pass 4 down to level): drawbuffer_0 over the whole frame, Z written, and the
// slice's depth range -- 1: (-10000, 500), 2: (500, 1000), 3: (1000, 1500), 4: (1500, 10000), any other
// pass (0, 0); the focus pass (pass == level) starts at -10000.
void ssBGT_DOFSetDrawEnvPreRend(int pass, int level)
{
    float nearClip, farClip;

    if (pass == 1) {
        nearClip = DOF_NEAR_ALL;
        farClip = DOF_SLICE_1_FAR;
    } else if (pass == 2) {
        nearClip = DOF_SLICE_1_FAR;
        farClip = DOF_SLICE_2_FAR;
    } else if (pass == 3) {
        nearClip = DOF_SLICE_2_FAR;
        farClip = DOF_SLICE_3_FAR;
    } else if (pass == DOF_PASS_FAR) {
        nearClip = DOF_SLICE_3_FAR;
        farClip = DOF_SLICE_4_FAR;
    } else {
        nearClip = 0.0f;
        farClip = 0.0f;
    }
    if (pass == level)
        nearClip = DOF_NEAR_ALL;
    ssSetFrameDrawBuffer(0);
    ssSetDOFClipRange(nearClip, farClip);
}

#define SS_ZBUF_NO_WRITE 1              /* ZBUF ZMSK: depth tested, not written */

// CT3 0x0189dc40
// After a slice behind the focus: drawbuffer_0 with Z writes masked, the frame just drawn as the texture,
// and drawctrl_ssSprite2 set up -- ssBGT_DOFDrawSprite sends it.
void ssBGT_DOFSetDrawEnvPostRend(void)
{
    ssSetFrameDrawBuffer(SS_ZBUF_NO_WRITE);
    ssSetScreenTexture((u_int)ga_draw_fbuf_address);
    gaSetupDrawCtrlData(&drawctrl_ssSprite2, SS_SPRITE_DRAWCTRL_ARGS);
}

// CT3 0x0189db60
// The slice blurred: the frame laid over itself three times at a quarter alpha -- 2 px left and 2 px
// right (both 1.16 px up), then 1.6 px down. The UVs are whatever the last pass left (no ssSetSpriteUV).
void ssBGT_DOFDrawSprite(void)
{
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_ssSprite2);
    ssSetSpriteCol(SS_SPRITE_GREY, SS_SPRITE_GREY, SS_SPRITE_GREY, SS_SPRITE_ALPHA_QUARTER);
    ssSetSpritePos(-2.0f, -1.16f, 510.0f, 446.84f, 0.0f);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
    ssSetSpritePos(2.0f, -1.16f, 514.0f, 446.84f, 0.0f);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
    ssSetSpritePos(0.0f, 1.6f, 512.0f, 449.6f, 0.0f);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&SS_draw_sprite);
}

// CT3 0x01895fb0
// Per frame (main): each ambient change in progress moves its character's bank 1/frames of the way from its
// part 0's ambient to the target (and, with CHAR_AMB_FLAG_SHADOW, its shadow value), then counts a frame.
// A character without a bank ends its change; an unloaded bank (its own or the room's) ends it and the
// whole pass for this frame (ROM: return, not continue).
void ssChangeCharAmbientColor(void)
{
    int i;

    for (i = 0; i < CHAR_AMB_CHANGE_MAX; i++) {
        CHAR_AMB_CHANGE *c = &CharAmbChangeFlag[i];
        const float *from;
        const float *to;
        float cur[4];
        int bank;
        int k;

        if (c->frames == 0)
            continue;
        bank = Character[c->chara].bank;
        if (bank < 0) {
            c->frames = 0;
            continue;
        }
        if (BankInfo[bank].state < 1 || BankInfo[c->bank].state < 1) {
            c->frames = 0;
            return;
        }
        from = BANK_PTR(const float *, bank, All_table_TBL[bank]->partsPos);
        to = (c->flags & CHAR_AMB_FLAG_COLOR) ? c->color : All_table_TBL[c->bank]->ambient;
        for (k = 0; k < 4; k++)
            cur[k] = from[k] + (to[k] - from[k]) / (float)c->frames;
        ssSetAmbientColor(cur[0], cur[1], cur[2], cur[3], bank);
        if (c->flags & CHAR_AMB_FLAG_SHADOW) {
            s_char now = BankInfo[bank].shadowOrder;

            asShadowDrawing_SetShadowValue(bank, (s_char)(now + (s_char)(((int)c->shadow - (int)now)
                                                                         / (int)c->frames)));
        }
        c->frames--;
    }
}

#define CAM_EE 0x01ae7c40u              /* Cam[]'s EE address (bss), for the words that hold one */
#define PARTICLE_TICKS_PER_TENTH 30     /* 300 ticks a second (the ROM's x * 300 / 10) */
#define PARTICLE_IDLE_INIT (-100)       /* PARTICLE_STATE.idleTicks at init */

int ParticlePassTicks[BANK_MAX];        /* sbss 0x019408b0 */

static float DOF_NearClip;              /* sbss 0x01940e3c */
static float DOF_FarClip;               /* sbss 0x01940e40 */

// One frame of bank `bank`'s first-kind particle systems at tick `total`: each emits, drops its expired
// slots and moves the rest. A system with run-ahead ticks left runs passes 5 ticks apart until they are
// used up (the frame it starts); afterwards one pass a frame.
static void ParticleBankPass(int bank, int total)
{
    const u_int *list = BANK_PTR(const u_int *, bank, All_table_TBL[bank]->particles);
    int i;

    if (list == NULL)                   /* PORT: the ROM reads the list unchecked */
        return;
    for (i = 0; list[i] != 0; i++) {
        const PARTICLE_ENTRY *ent = BANK_PTR(const PARTICLE_ENTRY *, bank, list[i]);
        PARTICLE_STATE *st = BANK_PTR(PARTICLE_STATE *, bank, ent->state);
        u_short part = (u_short)ent->parts;     /* lhu +0x10 */

        if (st->prerollTicks == 0) {
            int now = total - ParticlePassTicks[bank];
            int enabled = ssTrigCheckEnable(part, bank);

            ssGenerateParticle(ent, ParticleObject[bank][i], ParticleData[bank][i], enabled, now, bank);
            ssCalcParticleLifeSpan(ent, ParticleObject[bank][i], ParticleData[bank][i], now, bank);
            ssCalcParticleOrbit(ent, ParticleObject[bank][i], ParticleData[bank][i], now, bank);
            continue;
        }
        while (st->prerollTicks > 0) {
            int now = total - st->prerollTicks - ParticlePassTicks[bank];
            int enabled = ssTrigCheckEnable(part, bank);

            ssGenerateParticle(ent, ParticleObject[bank][i], ParticleData[bank][i], enabled, now, bank);
            ssCalcParticleLifeSpan(ent, ParticleObject[bank][i], ParticleData[bank][i], now, bank);
            ssCalcParticleOrbit(ent, ParticleObject[bank][i], ParticleData[bank][i], now, bank);
            st->prerollTicks -= PARTICLE_PREROLL_STEP;
        }
    }
}

// CT3 0x0189ae30
// Per frame (main): the global banks' particles on the global clock, the scene banks' on the scene clock
// (not behaviour banks, and not while paused unless PAUSE_RUNNING). A bank not drawn or forced still adds
// the frame's ticks to its ParticlePassTicks instead, so its systems' time stands still. A group of banks
// counts once.
void ssParticle_main(void)
{
    int bank;

    for (bank = 0; bank < BANK_SCENE_MAX; bank++) {
        if (BankInfo[bank].drawOn == 0 || (BankInfo[bank].flags & BANK_FLAG_ANIM_FORCE_STOP))
            ParticlePassTicks[bank] += gGlobalNextTicksInterval;
        else
            ParticleBankPass(bank, gGlobalTotalTicks);
        if (BankInfo[bank].groupCount != 0)
            bank += BankInfo[bank].groupCount - 1;
    }
    if (pause != 0 && (pause & PAUSE_RUNNING) == 0)
        return;
    for (bank = BANK_SCENE_MAX; bank < BANK_MAX; bank++) {
        if (BankInfo[bank].drawOn == 0 || BankInfo[bank].type == BANK_TYPE_BEHAVIOR
            || (BankInfo[bank].flags & BANK_FLAG_ANIM_FORCE_STOP))
            ParticlePassTicks[bank] += gNextTicksInterval;
        else
            ParticleBankPass(bank, gTotalTicks);
        if (BankInfo[bank].groupCount != 0)
            bank += BankInfo[bank].groupCount - 1;
    }
}

// CT3 0x0189a750
// The slots the system emits this pass: emitRate per 5 ticks since its last emission (the first pass only
// starts the clock). Disabled, it emits none but its clock still moves on. A mesh emitter takes its slots'
// places from the mesh's vertices in turn.
void ssGenerateParticle(const PARTICLE_ENTRY *ent, u_char *objs, u_char *data, int enabled, int now,
                        int bank)
{
    PARTICLE_STATE *st = BANK_PTR(PARTICLE_STATE *, bank, ent->state);
    int n;
    int i;

    if (st->idleTicks == PARTICLE_IDLE_INIT) {
        st->idleTicks = now;
        return;
    }
    n = (int)((st->emitRate * (float)(now - st->idleTicks)) / PARTICLE_EMIT_STEP);
    if (n == 0)
        return;
    if (enabled) {
        if (st->emitter == 0) {
            for (i = 0; i < n; i++)
                if (ssParticlePush_Back(objs, data, ent->obj, st, ent, now, bank) == 0)
                    break;
        } else {
            const u_char *em = BANK_PTR(const u_char *, bank, st->emitter);
            u_short count;
            u_int verts;
            u_int vert = 0;

            memcpy(&count, em + PARTICLE_EMITTER_VERT_COUNT, sizeof(count));
            memcpy(&verts, em + PARTICLE_EMITTER_VERTS, sizeof(verts));
            for (i = 0; i < n; i++)
                if (ssMeshParticleGetNextVertID2(&vert, count, verts, objs, data, ent, st, now, bank) == 0)
                    break;
        }
    }
    st->idleTicks = now;
}

// CT3 0x018969f0
// The ring's copies from the newest back to the oldest (at most ringMax), as draw list words.
int ssAddBlurObjToDrawTableInOrder(u_int *out, const PART_BLUR_OBJ *obj, int bank)
{
    const PART_BLUR_MAX_OBJ *max = obj->max;
    u_int idx = max->tail;
    int n = 0;
    int i;

    for (i = 0; i < max->ringMax; i++) {
        if (idx == max->head)
            break;
        if (idx == 0)
            idx = (u_int)(max->ringMax + 1) & 0xff;
        idx = (idx - 1) & 0xff;
        out[n++] = BankEeAddr(bank, obj->copies + idx * PART_BLUR_COPY_SIZE);
    }
    return n;
}

// CT3 0x01896a90
// main_rtn's passes: the bank's blur copies go into its draw list `table`. Sorted in place (table ==
// list, objSortWithBlurIdx's order in aftSortALLpartBlurObj), each object's copies follow its sorted row,
// the rows after it moved up to make room; otherwise list's count - 1 entries are copied and every blur
// object's copies appended, then the 0 end.
void ssPartMotionBlurInsertObjToDrawTable(int bank, void *table, u_int *list, int count, int mode)
{
    u_int *out = (u_int *)table;
    int blur = ssGetNumBlurDrawObjForScene(bank, mode);
    int last = count - 1;
    int n;
    int k;

    if (out == list) {
        for (k = NumPartBlurObj[mode][bank] - 1; k >= 0; k--) {
            const PART_BLUR_OBJ *obj = aftSortALLpartBlurObj[k];

            for (; obj->sortedIndex < last; last--)
                out[last + blur] = list[last];
            blur -= ssGetNumBlurDrawObj(obj->max);
            ssAddBlurObjToDrawTableInOrder(&out[obj->sortedIndex + blur + 1], obj, bank);
        }
        return;
    }
    for (n = 0; n < count - 1; n++)
        out[n] = list[n];
    for (k = 0; k < NumPartBlurObj[mode][bank]; k++)
        n += ssAddBlurObjToDrawTableInOrder(&out[n], ALLpartBlurObj[mode][bank][k], bank);
    out[n] = 0;
}

#define DOF_MODE_SLICE_MAX 2            /* dofMode below: the object's depth decides */
#define DOF_NEAR_CLIP_ALL (-10000.0f)   /* DOF_NearClip of the pass that draws the other objects */

// CT3 0x0189da90
// Whether the object is drawn in the current depth-of-field slice: by its centre's view depth (the
// matrix at +0x58 applied to DRAW_OBJ_CENTER, VU0), near <= z < far; an object of another mode only in
// the pass whose near clip is -10000.
int ssDOF_ClipObjectFromZRange(u_char *obj, u_int mode, int bank)
{
    u_int matEE;
    const float (*m)[4];
    float c[4];
    float z;

    if (mode >= DOF_MODE_SLICE_MAX)
        return DOF_NearClip == DOF_NEAR_CLIP_ALL;
    memcpy(&matEE, obj + DRAW_OBJ_W58, sizeof(matEE));
    m = BANK_PTR(const float (*)[4], bank, matEE);
    memcpy(c, obj + DRAW_OBJ_CENTER, sizeof(c));
    z = m[0][2] * c[0] + m[1][2] * c[1] + m[2][2] * c[2] + m[3][2] * c[3];
    if (z < DOF_NearClip)
        return 0;
    return z < DOF_FarClip;
}


// The EE address of a field of bank `scene`'s part record `part` (tiParticleInit's PartFieldEE).
static u_int ParticlePartEE(int scene, int part, size_t offset)
{
    return BankEeAddr(scene, pModelPartsHeader[scene] + part * (int)sizeof(MODEL_PARTS) + offset);
}

// CT3 0x0189a9d0
// scene_init: bank `scene`'s particle list (ALL_TABLE.particles) gets its per-entry tables (ParticleObject,
// ParticleData, from gHeapJunc) and each entry's state its times in ticks, its default emit rate and the EE
// addresses of its parts; its draw object views through its camera's view2 (+0x58, +0x5c), its draw control
// and texture are set up.
// PORT: the draw object's two view words hold Cam's EE address (CAM_EE + ...): no reader is written yet.
void ssParticleInit(int scene)
{
    int count = ssGetNumParticleEmit(scene);
    const u_int *list;
    int i;

    if (count == 0)
        return;
    ParticlePassTicks[scene] = 0;
    // PORT: host pointers are 8 bytes; the ROM asks count * 4 (two 12-byte asks for 0A02's three
    // entries overran into the next block headers and cut gHeapJunc short, hazards.md).
    ParticleObject[scene] = (u_char **)_SunAlloc(&gHeapJunc, count * (int)sizeof(u_char *), 0x10);
    ParticleData[scene] = (u_char **)_SunAlloc(&gHeapJunc, count * (int)sizeof(u_char *), 0x10);
    list = BANK_PTR(const u_int *, scene, All_table_TBL[scene]->particles);
    if (list == NULL)
        return;
    for (i = 0; list[i] != 0; i++) {
        PARTICLE_ENTRY *ent = BANK_PTR(PARTICLE_ENTRY *, scene, list[i]);
        PARTICLE_STATE *st = BANK_PTR(PARTICLE_STATE *, scene, ent->state);
        u_char *obj = BANK_PTR(u_char *, scene, ent->obj);
        GA_DRAWCTRL_ARGS *args = BANK_PTR(GA_DRAWCTRL_ARGS *, scene, ent->drawCtrlId);
        TEX_INFO *tex = BANK_PTR(TEX_INFO *, scene, ent->tex);
        u_int view;

        st->head = 0;
        st->tail = 0;
        st->idleTicks = PARTICLE_IDLE_INIT;
        st->lifeTicks = (int)st->lifeTenths * (PARTICLE_TICKS_PER_TENTH * 10) / 10;
        st->prerollTicks = st->prerollTicks * (PARTICLE_TICKS_PER_TENTH * 10) / 10;
        if (st->emitRate == 0.0f)
            st->emitRate = (float)st->ringSize / ((float)st->lifeTenths * 6.0f);
        st->partRec = ParticlePartEE(scene, ent->parts, PARTS_RECORDS);
        if (st->emitter != 0) {
            u_char *em = BANK_PTR(u_char *, scene, st->emitter);
            u_int m = ParticlePartEE(scene, *(u_short *)(em + 0xc), offsetof(MODEL_PARTS, matrix200));

            memcpy(em, &m, sizeof(m));
        }
        view = CAM_EE + (u_int)asCheckBankID(scene) * (u_int)sizeof(CAMERA) + offsetof(CAMERA, view2);
        memcpy(obj + DRAW_OBJ_W58, &view, sizeof(view));
        view = CAM_EE + (u_int)asCheckBankID(scene) * (u_int)sizeof(CAMERA) + offsetof(CAMERA, view2);
        memcpy(obj + DRAW_OBJ_W5C, &view, sizeof(view));
        if (st->part2 != 0)
            st->partPos = ParticlePartEE(scene, st->part2, offsetof(MODEL_PARTS, pos));
        view = All_table_TBL[scene]->partsPos + (u_int)ent->parts * 0x10;
        memcpy(obj + DRAW_OBJ_POS, &view, sizeof(view));
        if (args != NULL)
            gaSetupDrawCtrlData(BANK_PTR(GA_DRAWCTRL_DATA *, scene, ent->drawCtrl), *args);
        if (tex != NULL)
            sunSetupTBufferData(BANK_PTR(void *, scene, ent->tbuffer), *tex);
        ParticleObject[scene][i] = BANK_PTR(u_char *, scene, ent->objs);
        ParticleData[scene][i] = BANK_PTR(u_char *, scene, ent->data);
    }
}

// tnChangeCharAmbientColorSet's set: CHAR_AMB_CHANGE's fields as shorts (the colour's floats two each).
#define AMB_SET_CHARA 0
#define AMB_SET_BANK 1
#define AMB_SET_FRAMES 2
#define AMB_SET_FLAGS 3
#define AMB_SET_COLOR 4
#define AMB_SET_ALPHA 10
#define AMB_SET_SHADOW 12

// The float as the ROM's float -> u_int -> float conversion leaves it (truncated; a negative wraps).
static float TruncToUnsigned(float f)
{
    u_int u;

    if (f < 2147483648.0f)
        u = (u_int)(int)f;
    else
        u = (u_int)(int)(f - 2147483648.0f) | 0x80000000u;
    return (float)u;
}

// CT3 0x018962a0
// A character's ambient changes (the warps, ssScnPortalCheck, the xcmd): with frames, it is queued in the
// first free slot (or the character's own) for ssChangeCharAmbientColor; without (or with no slot free),
// the character's bank takes the target at once. A CHAR_AMB_FLAG_COLOR target with a negative alpha takes
// the character's light bank's ambient alpha, written back into set. The shadow value goes to set's bank.
void tnChangeCharAmbientColorSet(short *set)
{
    u_short chara = (u_short)set[AMB_SET_CHARA];
    u_short flags = (u_short)set[AMB_SET_FLAGS];
    float color[4];
    int now = 1;
    int i;

    memcpy(color, &set[AMB_SET_COLOR], sizeof(color));
    if ((flags & CHAR_AMB_FLAG_COLOR) && color[3] < 0.0f) {
        short lb = Character[chara].lightBank;

        if (lb >= 0) {
            color[3] = TruncToUnsigned(All_table_TBL[lb]->ambient[3]);
            memcpy(&set[AMB_SET_ALPHA], &color[3], sizeof(float));
        }
    }
    if (set[AMB_SET_FRAMES] != 0) {
        for (i = 0; i < CHAR_AMB_CHANGE_MAX; i++) {
            CHAR_AMB_CHANGE *c = &CharAmbChangeFlag[i];

            if (c->frames == 0 || c->chara == chara) {
                c->chara = chara;
                c->bank = (u_short)set[AMB_SET_BANK];
                c->frames = (u_short)set[AMB_SET_FRAMES];
                c->flags = flags;
                memcpy(c->color, color, sizeof(color));
                c->shadow = (s_char)set[AMB_SET_SHADOW];
                now = 0;
                break;
            }
        }
    }
    if (now) {
        int bank = Character[chara].bank;
        const float *to = (flags & CHAR_AMB_FLAG_COLOR) ? color
                                                        : All_table_TBL[(u_short)set[AMB_SET_BANK]]->ambient;

        ssSetAmbientColor(to[0], to[1], to[2], to[3], bank);
        if (flags & CHAR_AMB_FLAG_SHADOW)
            asShadowDrawing_SetShadowValue((u_short)set[AMB_SET_BANK], (s_char)set[AMB_SET_SHADOW]);
    }
}

// CT3 0x01897140
// The trigger command's motion blur off for the bank's part: its record leaves PartBlurMaxObj (the list
// closes up), its blur objects leave both ALLpartBlurObj lists, its entries get their draw state back
// (a mode other than TRAIL) and its three blocks go back to gHeapJunc.
void ssPartMotionBlurEnd(int part, int bank)
{
    PART_BLUR_MAX_OBJ **slots = PartBlurMaxObj[bank];
    PART_BLUR_MAX_OBJ *max = NULL;
    PART_BLUR_OBJ *objs;
    u_char *copies;
    int i, j, mode;

    /* ROM: the copy runs one slot ahead past each match; PORT: bounded at the bank's last slot */
    for (i = 0, j = 0; i < PART_BLUR_MAX_OBJ_MAX && slots[i] != NULL; i++, j++) {
        if (slots[i]->parts == (u_short)part) {
            max = slots[i];
            j++;
        }
        slots[i] = j < PART_BLUR_MAX_OBJ_MAX ? slots[j] : NULL;
        if (slots[i] == NULL)
            break;
    }
    if (max == NULL)
        return;
    for (mode = 0; mode < PART_BLUR_MODES; mode++) {
        PART_BLUR_OBJ **list = ALLpartBlurObj[mode][bank];
        int k;

        for (k = NumPartBlurObj[mode][bank] - 1; k >= 0; k--) {
            int n = NumPartBlurObj[mode][bank];

            if (list[k]->max->parts != (u_short)part)
                continue;
            /* ROM: list[x] = list[x + 1] up to n - 1, reading the 0 end; PORT: the end written, never read */
            for (i = k; i < n - 1; i++)
                list[i] = list[i + 1];
            list[n - 1] = NULL;
            NumPartBlurObj[mode][bank]--;
        }
    }
    objs = max->objs;
    copies = objs[0].copies;
    if (max->mode != PART_BLUR_MODE_TRAIL)
        BlurRestoreEntries(max, bank);
    _SunFree(&gHeapJunc, (void **)&copies);
    _SunFree(&gHeapJunc, (void **)&objs);
    _SunFree(&gHeapJunc, (void **)&max);
}

#define PART_BLUR_LISTS 2               /* ALL_TABLE.objOpaque (ALLpartBlurObj[0]), objTrans ([1]) */

// The bank's draw list `list` (0 objOpaque, 1 objTrans) as host words.
static const u_int *BlurDrawList(int bank, int list)
{
    const ALL_TABLE *tbl = All_table_TBL[bank];

    return BANK_PTR(const u_int *, bank, list == 0 ? tbl->objOpaque : tbl->objTrans);
}

// CT3 0x018976d0
// The trigger command's motion blur on for the bank's part (once: a part with a record is left alone).
// Every entry of the two draw lists drawn at the part gets a blur object (its draw control with alpha
// blending and the alpha test on) and a ring of ringMax + 1 copies; the objects join ALLpartBlurObj[list]
// in their list order. TRAIL starts with an empty ring; ONE_FRAME with a full one (tail = ringMax), and
// redirects the entries themselves: their POS word to the record's colour, their draw control blended
// (the record keeps the old settings for the end).
void ssPartMotionBlurStart(int part, int ringMax, int bank, int mode, int alphaPercent)
{
    PART_BLUR_MAX_OBJ **slots = PartBlurMaxObj[bank];
    PART_BLUR_MAX_OBJ *max;
    PART_BLUR_OBJ *objs;
    u_char *copies;
    int count = 0;
    int slot;
    int list, i, k;

    for (slot = 0; slot < PART_BLUR_MAX_OBJ_MAX && slots[slot] != NULL; slot++)
        if (slots[slot]->parts == (u_short)part)
            return;
    if (slot == PART_BLUR_MAX_OBJ_MAX)
        return;                                         /* PORT: the ROM writes past the bank's slots */
    for (list = 0; list < PART_BLUR_LISTS; list++) {
        const u_int *words = BlurDrawList(bank, list);

        for (i = 0; words[i] != 0; i++)
            if (BANK_PTR(const DRAW_OBJ_ENTRY *, bank, words[i])->parts == part)
                count++;
    }
    objs = (PART_BLUR_OBJ *)_SunAlloc(&gHeapJunc, count * (int)sizeof(PART_BLUR_OBJ), 0x40);
    copies = (u_char *)_SunAlloc(&gHeapJunc, count * (ringMax + 1) * PART_BLUR_COPY_SIZE, 0x40);
    max = (PART_BLUR_MAX_OBJ *)_SunAlloc(&gHeapJunc, (int)sizeof(PART_BLUR_MAX_OBJ), 0x40);
    slots[slot] = max;
    if (slot + 1 < PART_BLUR_MAX_OBJ_MAX)
        slots[slot + 1] = NULL;
    max->mode = (u_char)mode;
    max->parts = (u_short)part;
    max->ringMax = (u_char)ringMax;
    max->head = 0;
    if (mode == PART_BLUR_MODE_ONE_FRAME) {
        max->tail = (u_char)ringMax;
        max->alphaPercent = (float)(u_char)alphaPercent;
    } else {
        max->tail = 0;
    }
    max->count = (u_char)count;
    max->objs = objs;
    max->ambPos = All_table_TBL[bank]->partsPos + (u_int)part * 0x10;
    max->ambColor = BANK_PTR(const float *, bank, max->ambPos);
    max->bank = bank;
    memcpy(max->prevMatrix, ((const MODEL_PARTS *)pModelPartsHeader[bank])[part].matrix340,
           sizeof(max->prevMatrix));
    k = 0;
    for (list = 0; list < PART_BLUR_LISTS; list++) {
        const u_int *words = BlurDrawList(bank, list);
        PART_BLUR_OBJ **blur = ALLpartBlurObj[list][bank];

        for (i = 0; words[i] != 0; i++) {
            DRAW_OBJ_ENTRY *ent = BANK_PTR(DRAW_OBJ_ENTRY *, bank, words[i]);
            PART_BLUR_OBJ *o = &objs[k];
            GA_DRAWCTRL_ARGS *args = (GA_DRAWCTRL_ARGS *)(o->head + PART_BLUR_OBJ_ARGS);
            u_int n;

            if (ent->parts != part)
                continue;
            *args = *BANK_PTR(const GA_DRAWCTRL_ARGS *, bank, ent->drawCtrlArgs);
            args->abe = 1;
            args->ate = 1;
            o->listIndex = i;
            o->entry = ent;
            o->copies = copies + k * (ringMax + 1) * PART_BLUR_COPY_SIZE;
            o->max = max;
            if (mode == PART_BLUR_MODE_ONE_FRAME) {
                GA_DRAWCTRL_ARGS *own = BANK_PTR(GA_DRAWCTRL_ARGS *, bank, ent->drawCtrlArgs);
                u_int colorEE = BankEeAddr(bank, max->color);

                memcpy(BANK_PTR(u_char *, bank, ent->obj) + DRAW_OBJ_POS, &colorEE, sizeof(colorEE));
                max->args = *own;
                own->abe = 1;
                own->ate = 1;
                gaSetupDrawCtrlData(BANK_PTR(GA_DRAWCTRL_DATA *, bank, ent->drawCtrl), *own);
            }
            gaSetupDrawCtrlData((GA_DRAWCTRL_DATA *)(o->head + PART_BLUR_OBJ_DRAWCTRL), *args);
            /* in list order: the objects after it move up one */
            for (n = NumPartBlurObj[list][bank]; n != 0; n--) {
                if (blur[n - 1]->listIndex < i)
                    break;
                blur[n] = blur[n - 1];
            }
            blur[n] = o;
            NumPartBlurObj[list][bank]++;
            k++;
            if (NumPartBlurObj[list][bank] >= PART_BLUR_MAX - 1)
                break;                                  /* PORT: room for the 0 end; the ROM has no check */
        }
        blur[NumPartBlurObj[list][bank]] = NULL;
    }
    max->count = (u_char)k;                             /* PORT: == count unless a list was full */
}

// CT3 0x0189b580
// The trigger command's HQDOF end. ROM: it ends DRAW_MODE_VASELINE (7), not the DRAW_MODE_HQDOF (8)
// ssTrigHQDOFStart sets -- kept (hazards.md, 2026-11-12).
void ssTrigHQDOFEnd(void)
{
    if (DrawMode == DRAW_MODE_VASELINE)
        DrawMode = DRAW_MODE_NORMAL;
}

#define HQDOF_PERCENT_MAX 100           /* ssTrigHQDOFStart: both settings in 1..100 (0 reads as 1) */

// A command byte as a 1..100 setting.
static u_char HQDOFClamp(u_char v)
{
    if (v == 0)
        return 1;
    return v < HQDOF_PERCENT_MAX + 1 ? v : HQDOF_PERCENT_MAX;
}

// CT3 0x0189b5a0
// The trigger command's HQ depth of field on (when no screen pass is): the eye jitter and each sample's
// weight, each clamped to 1..100.
void ssTrigHQDOFStart(int cameraOffset, int opacity)
{
    if (DrawMode == DRAW_MODE_NORMAL) {
        HQDOFCameraOffset = HQDOFClamp((u_char)cameraOffset);
        HQDOFOpacity = HQDOFClamp((u_char)opacity);
        DrawMode = DRAW_MODE_HQDOF;
    }
}

// CT3 0x0189d2f0
// The trigger command's hazy screen off: back to the normal pass, if the hazy pass is the one on.
void ssTrigHazyScreenEnd(void)
{
    if (DrawMode == DRAW_MODE_HAZY)
        DrawMode = DRAW_MODE_NORMAL;
}

// CT3 0x0189d310
// The trigger command's hazy screen on (when no screen pass is): DRAW_MODE_HAZY at `level`, fading in over
// `time` (HazeTransTime).
void ssTrigHazyScreenStart(int level, int time)
{
    if (DrawMode == DRAW_MODE_NORMAL) {
        DrawMode = DRAW_MODE_HAZY;
        HazeTransTime = time;
        ScreenHazyLevel = level;
    }
}

// CT3 0x01898710
// The trigger command's particle all-off: the system whose trigger part is `particle` is disabled and its
// ring emptied at once (head = tail; a second-kind ring's count 0 too). The part's kind says which list
// holds it; the first system that matches is the one.
void ssTrigParticleAllOffImmediately(int particle, int bank)
{
    u_short part = (u_short)particle;
    u_char kind = ((MODEL_PARTS *)pModelPartsHeader[bank])[part].kind;
    const ALL_TABLE *tbl = All_table_TBL[bank];
    int l;
    int i;

    if (kind == PARTS_KIND_PARTICLE) {
        const u_int *list = BANK_PTR(const u_int *, bank, tbl->particles);

        for (i = 0; list != NULL && list[i] != 0; i++) {        /* PORT: NULL checked */
            const PARTICLE_ENTRY *ent = BANK_PTR(const PARTICLE_ENTRY *, bank, list[i]);

            if ((u_int)ent->parts == part) {
                PARTICLE_STATE *st = BANK_PTR(PARTICLE_STATE *, bank, ent->state);

                ssTrigSetEnable(particle, 0, 0, bank);
                st->head = st->tail;
                return;
            }
        }
    } else if (kind == PARTS_KIND_PARTICLE2) {
        const u_int lists[2] = {tbl->particles2, tbl->particles2After};

        for (l = 0; l < 2; l++) {
            const u_int *list = BANK_PTR(const u_int *, bank, lists[l]);

            for (i = 0; list != NULL && list[i] != 0; i++) {
                PARTICLE2_ENTRY *ent = BANK_PTR(PARTICLE2_ENTRY *, bank, list[i]);

                if (ent->parts == part) {
                    ssTrigSetEnable(particle, 0, 0, bank);
                    ent->head = ent->tail;
                    ent->count = 0;
                    return;
                }
            }
        }
    }
}

#define PARTICLE_PREROLL_TICKS_PER_TENTH 30     /* the command's tenths of a second, as ticks: x 300 / 10 */

// CT3 0x018988b0
// The trigger command's particle start: the particle's trigger part enabled at once, and the first system
// of the bank's list drawn at that part runs prerollTenths ahead (ssParticle_main's preroll).
void ssTrigParticleStart(int particle, int bank, int prerollTenths)
{
    const u_int *list;
    int i;

    ssTrigSetEnable(particle, 1, 0, bank);
    list = BANK_PTR(const u_int *, bank, All_table_TBL[bank]->particles);
    for (i = 0; list != NULL && list[i] != 0; i++) {            /* PORT: NULL checked */
        const PARTICLE_ENTRY *ent = BANK_PTR(const PARTICLE_ENTRY *, bank, list[i]);

        if ((u_int)ent->parts == ((u_int)particle & 0xffff)) {
            BANK_PTR(PARTICLE_STATE *, bank, ent->state)->prerollTicks =
                (prerollTenths * (PARTICLE_PREROLL_TICKS_PER_TENTH * 10)) / 10;
            return;
        }
    }
}

// CT3 0x018988a0
// The trigger command's particle stop: the particle's trigger part disabled at once.
void ssTrigParticleStop(int particle, int bank)
{
    ssTrigSetEnable(particle, 0, 0, bank);
}

// CT3 0x0189d920
// The trigger command's screen blur on (when no screen pass is): DRAW_MODE_BLUR at `level`.
void ssTrigScreenBlurStart(int level)
{
    if (DrawMode == DRAW_MODE_NORMAL) {
        DrawMode = DRAW_MODE_BLUR;
        ScreenBlurLevel = level;
    }
}

SS_2DP_REC *ss2DpData;                  /* 0x0193f778: the 2D particles' records (vaseline, wave), gHeapJunc */

// CT3 0x0189b920
// The trigger command's vaseline off: the 2D particle records go back to gHeapJunc and the normal pass is
// on, whichever pass was (ROM: no DrawMode test, unlike ssTrigWaveEnd).
void ssTrigVaselineEnd(void)
{
    _SunFree(&gHeapJunc, (void **)&ss2DpData);
    DrawMode = DRAW_MODE_NORMAL;
}

static const SS_SCREEN_RECT ssScreenRectZero; /* sbss 0x0193e018 (symtab __529): the result's start */

// CT3 0x0189b640
// A random screen point for a vaseline diamond. inside: uniform over the rectangle. Otherwise uniform over
// the rest of the 512 x 448 screen, counted in three bands: the rows above it (row-major), the rows beside
// it (column-major, height h, the columns left of it then right of it), the rows below it (row-major).
// ROM: a beside-column equal to range.x is not shifted past the rectangle, so it lands in its left column
// (and column x + w is never drawn); kept. PORT: an empty rectangle with inside set returns its corner
// (the ROM divides by 0 w).
SS_SCREEN_RECT ssVaselineGetScreenPos(SS_SCREEN_RECT range, int inside)
{
    SS_SCREEN_RECT p = ssScreenRectZero;
    int x = range.x, y = range.y, w = range.w, h = range.h, r, beside, below;
    if (inside) {
        if (w == 0) {
            p.x = (u_short)x;
            p.y = (u_short)y;
            return p;
        }
        r = tnRand(w * h);
        p.x = (u_short)(x + r % w);
        p.y = (u_short)(y + r / w);
        return p;
    }
    r = tnRand(SS_2DP_SCREEN_W * SS_2DP_SCREEN_H - w * h);
    beside = r - y * SS_2DP_SCREEN_W;
    if (beside < 0) {
        p.x = (u_short)(r % SS_2DP_SCREEN_W);
        p.y = (u_short)(r / SS_2DP_SCREEN_W);
        return p;
    }
    below = beside - (SS_2DP_SCREEN_W - w) * h;
    if (below < 0) {
        p.x = (u_short)(beside / h);
        p.y = (u_short)(y + beside % h);
        if (x < (int)p.x)
            p.x = (u_short)(p.x + w);
        return p;
    }
    p.x = (u_short)(below % SS_2DP_SCREEN_W);
    p.y = (u_short)(y + h + below / SS_2DP_SCREEN_W);
    return p;
}

// One axis of ss2DParticleSetDrawData2's UVs (lo < mid < hi, 16 * size apart): one that comes within a
// texel of the texture's edge is pushed in, with those beyond it, to 1, 2, 3 texels from that edge.
static void ss2DpClampUV(float *lo, float *mid, float *hi, float max)
{
    if (*hi < SS_2DP_UV_EDGE) {
        *hi = 3.0f * SS_2DP_UV_EDGE;
        *mid = 2.0f * SS_2DP_UV_EDGE;
        *lo = SS_2DP_UV_EDGE;
    } else if (*mid < SS_2DP_UV_EDGE) {
        *mid = 2.0f * SS_2DP_UV_EDGE;
        *lo = SS_2DP_UV_EDGE;
    } else if (*lo < SS_2DP_UV_EDGE) {
        *lo = SS_2DP_UV_EDGE;
    }
    if (!(*lo <= max)) {
        *mid = max - SS_2DP_UV_EDGE;
        *lo = max - 2.0f * SS_2DP_UV_EDGE;
        *hi = max;
    } else if (!(*mid <= max)) {
        *mid = max - SS_2DP_UV_EDGE;
        *hi = max;
    } else if (!(*hi <= max)) {
        *hi = max;
    }
}

// CT3 0x0189b950
// A vaseline diamond's packet, written once (it does not move): the fan about (x, y) with the
// half-diagonal size, and its UVs the same points shifted by (du, dv), kept a texel inside the 512 x 448
// frame on all four sides (ss2DParticleSetDrawData clamps only the top). From the disassembly.
void ss2DParticleSetDrawData2(SS_DIAMOND_VERTEX *vtx, int x, int y, int size, float du, float dv)
{
    u_int xc = ss2DpFix16((float)(x + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    u_int yc = ss2DpFix16((float)(y + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    float uc = SS_2DP_SUBPIXEL * ((float)x + du), vc = SS_2DP_SUBPIXEL * ((float)y + dv);
    float s = SS_2DP_SUBPIXEL * (float)size;
    float ul = uc - s, ur = uc + s, vt = vc - s, vb = vc + s;
    u_int u, v;

    vtx[0].x = xc;
    vtx[0].y = yc;
    vtx[1].x = xc;
    vtx[1].y = ss2DpFix16((float)(y + size + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    vtx[2].x = ss2DpFix16((float)(x - size + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    vtx[2].y = yc;
    vtx[3].x = xc;
    vtx[3].y = ss2DpFix16((float)(y - size + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    vtx[4].x = ss2DpFix16((float)(x + size + SS_2DP_XYZ_OFFSET) * SS_2DP_SUBPIXEL);
    vtx[4].y = yc;
    vtx[5].x = vtx[1].x;
    vtx[5].y = vtx[1].y;

    ss2DpClampUV(&vt, &vc, &vb, SS_2DP_V_MAX);
    ss2DpClampUV(&ul, &uc, &ur, SS_2DP_U_MAX);
    u = ss2DpFix16(uc);
    v = ss2DpFix16(vc);
    vtx[0].u = u;
    vtx[0].v = v;
    vtx[1].u = u;
    vtx[1].v = ss2DpFix16(vb);
    vtx[2].u = ss2DpFix16(ul);
    vtx[2].v = v;
    vtx[3].u = u;
    vtx[3].v = ss2DpFix16(vt);
    vtx[4].u = ss2DpFix16(ur);
    vtx[4].v = v;
    vtx[5].u = vtx[1].u;
    vtx[5].v = vtx[1].v;
}

// CT3 0x0189bd40
// The trigger command's vaseline on, if no other pass is: count still diamonds from gHeapJunc, each at a
// random point (ssVaselineGetScreenPos), its size randomised by sizeRand percent and clamped, its UV shift
// random within +-sway / 1000 pixels, its packet written once. Unlike the wave, nothing moves or expires:
// the records are not a ring (ss2DpInfo keeps only max).
void ssTrigVaselineStart(int x, int y, int w, int h, int size, int sizeRand, int sway, int count, int inside)
{
    SS_SCREEN_RECT range, p;
    int i;
    if (DrawMode != DRAW_MODE_NORMAL)
        return;
    ss2DpData = (SS_2DP_REC *)_SunAlloc(&gHeapJunc, count * (int)sizeof(SS_2DP_REC), 0x10);
    range.x = (u_short)x;
    range.y = (u_short)y;
    range.w = (u_short)w;
    range.h = (u_short)h;
    for (i = 0; i < count; i++) {
        SS_2DP_REC *r;
        u_int s = (u_int)size;
        float du, dv;
        p = ssVaselineGetScreenPos(range, inside);
        if (sizeRand != 0) {
            s = ss2DpRandomise(s, (u_int)sizeRand);
            if (s < SS_2DP_SIZE_MIN)
                s = SS_2DP_SIZE_MIN;
            else if (s > SS_2DP_SIZE_MAX)
                s = SS_2DP_SIZE_MAX;
        }
        du = (float)(sway * (tnRand(SS_2DP_SWAY_RAND) - 100)) * SS_2DP_SWAY_SCALE;
        dv = (float)(sway * (tnRand(SS_2DP_SWAY_RAND) - 100)) * SS_2DP_SWAY_SCALE;
        ss2DpInfo.max = (u_short)count;
        r = &ss2DpData[i];
        memcpy(r, SS_draw_diamond, sizeof(SS_draw_diamond));
        r->du = du;
        r->dv = dv;
        r->x = (short)p.x;
        r->y = (short)p.y;
        r->size = (u_char)s;
        ss2DParticleSetDrawData2(r->vtx, p.x, p.y, (int)s, du, dv);
    }
    DrawMode = DRAW_MODE_VASELINE;
}

// CT3 0x0189ce30
// The trigger command's wave off, if the wave pass is the one on: its 2D particle records go back to
// gHeapJunc.
void ssTrigWaveEnd(void)
{
    if (DrawMode == DRAW_MODE_WAVE) {
        _SunFree(&gHeapJunc, (void **)&ss2DpData);
        DrawMode = DRAW_MODE_NORMAL;
    }
}

// CT3 0x0189ce60
// The trigger command's wave on, if no other pass is: count records from gHeapJunc and ss2DpInfo set
// up. With a part, the rectangle is the bounds of the bank's wave object (its matrix pointed at the
// part's first) and sizeScale = size * its depth, so the diamonds shrink as it recedes; without one
// (SS_2DP_NO_OBJECT) the rectangle is the arguments'. A diamond lives h * 10 / speed frames: the time
// it takes to cross the rectangle.
void ssTrigWaveStart(int x, int y, int w, int h, int size, int sizeRand, int speed, int speedRand,
                     int sway, int count, int bank, int part)
{
    if (DrawMode != DRAW_MODE_NORMAL)
        return;
    ss2DpData = (SS_2DP_REC *)_SunAlloc(&gHeapJunc, count * (int)sizeof(SS_2DP_REC), 0x10);
    ss2DpInfo.bank = bank;
    if ((u_char)part == SS_2DP_NO_OBJECT) {
        ss2DpInfo.obj = NULL;
        ss2DpInfo.sizeScale = 0.0f;
        ss2DpInfo.x = (short)x;
        ss2DpInfo.y = (short)y;
        ss2DpInfo.w = (short)w;
        ss2DpInfo.h = (short)h;
    } else {
        const u_int *waves = BANK_PTR(const u_int *, bank, All_table_TBL[bank]->waves);
        SS_WAVE_OBJ *obj = BANK_PTR(SS_WAVE_OBJ *, bank, waves[(u_char)part]);
        ss2DpInfo.obj = obj;
        obj->matrix = BankEeAddr(bank, pModelPartsHeader[bank] + obj->part * PARTS_REC_SIZE + SS_WAVE_OBJ_MATRIX);
        ss2DpInfo.sizeScale = (float)size * ssGetScreenRange(obj, &ss2DpInfo);
    }
    ss2DpInfo.max = (u_short)count;
    ss2DpInfo.size = (u_char)size;
    ss2DpInfo.frame = 0;
    ss2DpInfo.speed = (u_char)speed;
    ss2DpInfo.head = 0;
    ss2DpInfo.tail = 0;
    DrawMode = DRAW_MODE_WAVE;
    ss2DpInfo.sway = (u_char)sway;
    ss2DpInfo.life = (u_short)((int)((u_int)(u_short)h * 10) / speed);
    ss2DpInfo.speedRand = (u_char)speedRand;
    ss2DpInfo.sizeRand = (u_char)sizeRand;
}

// ---- the particle curve readers (declared from tiCalc_Orbit, 2026-11-11) ----

// The curve readers' cache: the last values they computed (ssParticleCashClear empties it before each pass
// over a ring). Names and addresses are the symtab's; their roles come with the readers.
static float cashVECval0[4];            /* sbss 0x0193e020 */
static float cashFval1;                 /* sbss 0x0193e030 */
static float cashFval0;                 /* sbss 0x0193e034 */
static s_char cashIval3;                /* sbss 0x0193e038 */
static s_char cashIval2;                /* sbss 0x0193e03c */
static s_char cashIval1;                /* sbss 0x0193e040 */
static short cashIval0;                 /* sbss 0x0193e044 */
static int cashTicks;                   /* sbss 0x0193e048: -1 = nothing cached */

// CT3 0x01899070
void ssParticleCashClear(void)
{
    cashIval0 = 0;
    cashTicks = -1;
    cashIval1 = 0;
    cashIval2 = 0;
    cashIval3 = 0;
    cashFval0 = 0.0f;
    cashFval1 = 0.0f;
    cashVECval0[0] = 0.0f;              // vf0: (0, 0, 0, 1)
    cashVECval0[1] = 0.0f;
    cashVECval0[2] = 0.0f;
    cashVECval0[3] = 1.0f;
}

// The depth-of-field slice main_rtn_DOF draws (ssBGT_DOFSetDrawEnvPreRend sets it per pass).

// CT3 0x0189deb0
void ssSetDOFClipRange(float nearClip, float farClip)
{
    DOF_NearClip = nearClip;
    DOF_FarClip = farClip;
}

// The curves' keys: a tick (the record's age, ascending; the last key's tick lies past the record's life)
// and the value at that tick. A reader interpolates linearly between the key *cache and the next one,
// first stepping *cache forward past every key the age has reached (ages only grow, so the cache word
// saves the walk). Age 0 answers key 0 and resets the cache word.
typedef struct CURVE_KEY_F {            /* the refractive index */
    int tick;
    float value;
} CURVE_KEY_F;
typedef struct CURVE_KEY_S16 {          /* the alpha, percent */
    int tick;
    short value;
    u_char pad6[2];
} CURVE_KEY_S16;
typedef struct CURVE_KEY_RGB {          /* the vertex colour offsets, signed */
    int tick;
    s_char rgb[3];
    u_char pad7;
} CURVE_KEY_RGB;
typedef struct CURVE_KEY_F2 {           /* UV, the 2D size */
    int tick;
    float value[2];
} CURVE_KEY_F2;
typedef struct CURVE_KEY_V4 {           /* the 3D scale */
    int tick;
    u_char pad04[0xc];
    float value[4];
} CURVE_KEY_V4;
static_assert(sizeof(CURVE_KEY_F) == 0x08, "CURVE_KEY_F");
static_assert(sizeof(CURVE_KEY_S16) == 0x08, "CURVE_KEY_S16");
static_assert(sizeof(CURVE_KEY_RGB) == 0x08, "CURVE_KEY_RGB");
static_assert(sizeof(CURVE_KEY_F2) == 0x0c, "CURVE_KEY_F2");
static_assert(sizeof(CURVE_KEY_V4) == 0x20, "CURVE_KEY_V4");

// The key-walk every reader inlines: *cache past the keys age has reached; t0 / t1 the ticks around age.
#define CURVE_WALK(keys, age, cache, t0, t1)                \
    do {                                                    \
        (t0) = (keys)[*(cache)].tick;                       \
        (t1) = (keys)[*(cache) + 1].tick;                   \
        while ((t1) <= (age)) {                             \
            (*(cache))++;                                   \
            (t0) = (t1);                                    \
            (t1) = (keys)[*(cache) + 1].tick;               \
        }                                                   \
    } while (0)

// CT3 0x01898a60
// The 3D scale at `age` (tiCalc_Orbit: PARTICLE2.scale). The ROM interpolates on VU0 (vmulbc, vadd)
// and multiplies by the reciprocal of the span.
void tiParticleGetSizeValue(int age, float *scale, void *curve, short *cache)
{
    const CURVE_KEY_V4 *keys = (const CURVE_KEY_V4 *)curve;
    int t0, t1, i;
    float w0, w1, inv;

    if (age == 0) {
        memcpy(scale, keys[0].value, sizeof(keys[0].value));
        *cache = 0;
        return;
    }
    if (age == cashTicks) {
        memcpy(scale, cashVECval0, sizeof(cashVECval0));
        return;
    }
    CURVE_WALK(keys, age, cache, t0, t1);
    w0 = (float)(t1 - age);
    w1 = (float)(age - t0);
    inv = 1.0f / (float)(t1 - t0);
    for (i = 0; i < 4; i++)
        scale[i] = (keys[*cache].value[i] * w0 + keys[*cache + 1].value[i] * w1) * inv;
    memcpy(cashVECval0, scale, sizeof(cashVECval0));
    cashTicks = age;
}

// CT3 0x01898980
void tiParticleGetRefractive_IndexValue(int age, float *index, void *curve, short *cache)
{
    const CURVE_KEY_F *keys = (const CURVE_KEY_F *)curve;
    int t0, t1;

    if (age == 0) {
        *index = keys[0].value;
        *cache = 0;
        return;
    }
    if (age == cashTicks) {
        *index = cashFval0;
        return;
    }
    CURVE_WALK(keys, age, cache, t0, t1);
    cashFval0 = (keys[*cache + 1].value * (float)(age - t0) + keys[*cache].value * (float)(t1 - age))
                / (float)(t1 - t0);
    *index = cashFval0;
    cashTicks = age;
}

// CT3 0x01898bc0
// Integer interpolation of signed bytes (the ROM's div truncates toward 0, like C's).
void ssParticleGetVRGBValue(int age, s_char *r, s_char *g, s_char *b, void *curve, short *cache)
{
    const CURVE_KEY_RGB *keys = (const CURVE_KEY_RGB *)curve;
    const CURVE_KEY_RGB *k;
    int t0, t1;

    if (age == 0) {
        *r = keys[0].rgb[0];
        *g = keys[0].rgb[1];
        *b = keys[0].rgb[2];
        *cache = 0;
        return;
    }
    if (age == cashTicks) {
        *r = cashIval1;
        *g = cashIval2;
        *b = cashIval3;
        return;
    }
    CURVE_WALK(keys, age, cache, t0, t1);
    k = &keys[*cache];
    cashIval1 = (s_char)((k[0].rgb[0] * (t1 - age) + k[1].rgb[0] * (age - t0)) / (t1 - t0));
    *r = cashIval1;
    cashIval2 = (s_char)((k[0].rgb[1] * (t1 - age) + k[1].rgb[1] * (age - t0)) / (t1 - t0));
    *g = cashIval2;
    cashIval3 = (s_char)((k[0].rgb[2] * (t1 - age) + k[1].rgb[2] * (age - t0)) / (t1 - t0));
    *b = cashIval3;
    cashTicks = age;
}

// CT3 0x01898e60
void ssParticleGetAlphaValue(int age, short *alpha, void *curve, short *cache)
{
    const CURVE_KEY_S16 *keys = (const CURVE_KEY_S16 *)curve;
    const CURVE_KEY_S16 *k;
    int t0, t1;

    if (age == 0) {
        *alpha = keys[0].value;
        *cache = 0;
        return;
    }
    if (age == cashTicks) {
        *alpha = cashIval0;
        return;
    }
    CURVE_WALK(keys, age, cache, t0, t1);
    k = &keys[*cache];
    cashIval0 = (short)((k[0].value * (t1 - age) + k[1].value * (age - t0)) / (t1 - t0));
    *alpha = cashIval0;
    cashTicks = age;
}

// The two-float readers (UV and the 2D size) are the same code twice in the ROM.
static void CurveGetF2(int age, float *a, float *b, const void *curve, short *cache)
{
    const CURVE_KEY_F2 *keys = (const CURVE_KEY_F2 *)curve;
    const CURVE_KEY_F2 *k;
    int t0, t1;

    if (age == 0) {
        *a = keys[0].value[0];
        *b = keys[0].value[1];
        *cache = 0;
        return;
    }
    if (age == cashTicks) {
        *a = cashFval0;
        *b = cashFval1;
        return;
    }
    CURVE_WALK(keys, age, cache, t0, t1);
    k = &keys[*cache];
    cashFval0 = (k[0].value[0] * (float)(t1 - age) + k[1].value[0] * (float)(age - t0)) / (float)(t1 - t0);
    *a = cashFval0;
    cashFval1 = (k[1].value[1] * (float)(age - t0) + k[0].value[1] * (float)(t1 - age)) / (float)(t1 - t0);
    *b = cashFval1;
    cashTicks = age;
}

// CT3 0x01898d20
// u, v: floats (the ROM stores with swc1; tiCalc_Orbit passes the draw object's DRAW_OBJ_UV_U / _V).
void ssParticleGetUVValue(int age, void *u, void *v, void *curve, short *cache)
{
    CurveGetF2(age, (float *)u, (float *)v, curve, cache);
}

// CT3 0x01898f30
void ssParticleGetSizeValue(int age, float *w, float *h, void *curve, short *cache)
{
    CurveGetF2(age, w, h, curve, cache);
}

// ---- the first particle kind's rings (PARTICLE_ENTRY) ------------------------------------------------------

// CT3 0x01897fd0
// The oldest live slot's draw object, the head moved past it; NULL when the ring is empty.
u_char *ssParticlePop_Front(u_char *objs, PARTICLE_STATE *st)
{
    u_short head = st->head;

    if (st->tail == head)
        return NULL;
    st->head = (u_short)((head + 1) % st->ringSize);
    return objs + head * PARTICLE_OBJ_SIZE;
}

// CT3 0x0189a510
// Whether slot `slot` is live (between head and tail, wrapping) and its life is over at tick `now`.
int ssParticleLifeCheck(const PARTICLE_STATE *st, u_short slot, const u_char *data, int now)
{
    u_short head = st->head;
    u_short tail = st->tail;
    int born;

    if (head == tail)
        return 0;
    if (head < tail) {
        if (slot < head || slot >= tail)
            return 0;
    } else if (tail < head && slot >= tail && slot < head) {
        return 0;
    }
    memcpy(&born, data + slot * PARTICLE_REC_SIZE + PARTICLE_REC_BORN, sizeof(born));
    return st->lifeTicks + born <= now;
}

// CT3 0x0189a5b0
// Drops the expired slots from the front of the entry's ring (ssParticle_main, once per entry and frame).
// PORT: the ROM's first ssParticleLifeCheck passes data / now through untouched registers.
void ssCalcParticleLifeSpan(const PARTICLE_ENTRY *ent, u_char *objs, const u_char *data, int now, int bank)
{
    PARTICLE_STATE *st = BANK_PTR(PARTICLE_STATE *, bank, ent->state);

    while (ssParticleLifeCheck(st, st->head, data, now) && ssParticlePop_Front(objs, st) != NULL) {
    }
}

// ---- the first particle kind's per-slot curves (ssCalcParticleOrbit calls them) ----------------------------

// A slot draw object's word ssParticleSetVRGB / _SetAlpha write through: the EE address of its four vertex
// colours (RGBA bytes from +0x10, one word per vertex; signed, 0 = the 0x80 midpoint).
#define PARTICLE_OBJ_COLORS 0x6c
#define PARTICLE_COLOR_BASE 0x10        /* the first vertex colour's offset in that block */
#define PARTICLE_VERTS 4                /* a slot is one quad */
#define PARTICLE_ALPHA_PERCENT 100      /* the alpha curve's full scale */
#define PARTICLE_COLOR_BIAS 0x80        /* signed colour bytes centre on 0x80 */
#define PARTICLE_REC_SIZE_W 0x50        /* float: the slot's size this frame, w and h */
#define PARTICLE_REC_BASE_W 0xa0        /* float: its base size, w and h */

static int ParticleRecInt(const u_char *rec, size_t off)
{
    int v;

    memcpy(&v, rec + off, sizeof(v));
    return v;
}

// CT3 0x018990a0
// PORT: bank resolves the object's EE colour word.
void ssParticleSetVRGB(void *curve, u_char *obj, u_char *rec, int now, int bank)
{
    s_char r, g, b;
    u_int colorsEE;
    u_char *colors;
    int v;

    ssParticleGetVRGBValue(now - ParticleRecInt(rec, PARTICLE_REC_BORN), &r, &g, &b, curve,
                           (short *)(rec + PARTICLE_REC_VRGB_CACHE));
    memcpy(&colorsEE, obj + PARTICLE_OBJ_COLORS, sizeof(colorsEE));
    colors = BANK_PTR(u_char *, bank, colorsEE) + PARTICLE_COLOR_BASE;
    for (v = 0; v < PARTICLE_VERTS; v++) {
        colors[v * 4 + 0] = (u_char)r;
        colors[v * 4 + 1] = (u_char)g;
        colors[v * 4 + 2] = (u_char)b;
    }
}

// CT3 0x01899150
void ssParticleSetUV(void *curve, u_char *obj, u_char *rec, int now)
{
    float u, v;

    ssParticleGetUVValue(now - ParticleRecInt(rec, PARTICLE_REC_BORN), &u, &v, curve,
                         (short *)(rec + PARTICLE_REC_UV_CACHE));
    memcpy(obj + DRAW_OBJ_UV_U, &u, sizeof(u));
    memcpy(obj + DRAW_OBJ_UV_V, &v, sizeof(v));
}

// CT3 0x018991a0
// Each vertex's alpha: the source colour's alpha (src: four RGBA words, signed) scaled by the curve's
// percent. PORT: bank resolves the object's EE colour word.
void ssParticleSetAlpha(void *curve, u_char *obj, u_char *rec, const s_char *src, int now, int bank)
{
    short alpha;
    u_int colorsEE;
    u_char *colors;
    int v;

    ssParticleGetAlphaValue(now - ParticleRecInt(rec, PARTICLE_REC_BORN), &alpha, curve,
                            (short *)(rec + PARTICLE_REC_ALPHA_CACHE));
    memcpy(&colorsEE, obj + PARTICLE_OBJ_COLORS, sizeof(colorsEE));
    colors = BANK_PTR(u_char *, bank, colorsEE) + PARTICLE_COLOR_BASE;
    for (v = 0; v < PARTICLE_VERTS; v++)
        colors[v * 4 + 3] = (u_char)((s_char)((alpha * (src[v * 4 + 3] + PARTICLE_COLOR_BIAS))
                                               / PARTICLE_ALPHA_PERCENT) - PARTICLE_COLOR_BIAS);
}

// CT3 0x018992c0
void ssParticleSetSize(void *curve, u_char *rec, int now)
{
    float w, h, base;

    ssParticleGetSizeValue(now - ParticleRecInt(rec, PARTICLE_REC_BORN), &w, &h, curve,
                           (short *)(rec + PARTICLE_REC_SIZE_CACHE));
    memcpy(&base, rec + PARTICLE_REC_BASE_W, sizeof(base));
    w *= base;
    memcpy(rec + PARTICLE_REC_SIZE_W, &w, sizeof(w));
    memcpy(&base, rec + PARTICLE_REC_BASE_W + 4, sizeof(base));
    h *= base;
    memcpy(rec + PARTICLE_REC_SIZE_W + 4, &h, sizeof(h));
}

// ---- the first particle kind's emission (ssGenerateParticle calls them) ------------------------------------

// A new slot's record: the template object's four qwords (+0x60: pos, size, two unread) copied to
// PARTICLE_REC_POS, the slot object's +0x60 then points at the record's copy.
#define PARTICLE_OBJ_BLOCK 0x60         /* u_int: EE address of the object's four qwords (pos, size, ..) */
#define PARTICLE_REC_POS 0x40           /* float[4]: the slot's position (world, after emission) */
#define PARTICLE_REC_BLOCK_SIZE 0x40    /* the four qwords */
#define PARTICLE_REC_DIR 0x80           /* float[4]: PARTICLE_STATE.dir through the part's matrix200 */
#define PARTICLE_REC_START 0x90         /* float[4]: the position it was emitted at */
#define PARTICLE_REC_TICK 0xac          /* int: the tick of its last move (the motions' dt; emission sets it) */
#define PARTICLE_COLORS_STRIDE 0x20     /* PARTICLE_ENTRY.colors: one slot's colour block */
#define PARTICLE_JITTER_PERCENT 100.0f

// The part's matrix200 through PARTICLE_STATE.partRec (the record's EE address: header + 0x40 + part * 0x380).
static float (*ParticlePartMatrix(const PARTICLE_STATE *st, int bank))[4]
{
    MODEL_PARTS *part = (MODEL_PARTS *)(BANK_PTR(u_char *, bank, st->partRec) - PARTS_RECORDS);

    return part->matrix200;
}

// ssParticlePush_Back and ssMeshParticlePush_Back (the ROM has the body twice): the template object
// (ent->obj, 0x80 bytes) goes to the tail slot with the record's block, birth, size and direction set.
// meshPos: NULL -- the template's position through the part's matrix200; else the mesh vertex through the
// emitter's matrix (its word 0). 0 when the ring is full.
static int ParticlePushSlot(u_char *objs, u_char *data, u_int objEE, PARTICLE_STATE *st,
                            const PARTICLE_ENTRY *ent, const float *meshPos, const float (*meshMat)[4],
                            int now, int bank)
{
    u_char copy[PARTICLE_OBJ_SIZE];
    u_short tail = st->tail;
    u_short next = (u_short)((tail + 1u) % st->ringSize);
    u_char *rec;
    float dir[4];
    float (*m)[4];
    u_int ee;
    int i;

    memcpy(copy, BANK_PTR(const u_char *, bank, objEE), sizeof(copy));
    if (next == st->head)
        return 0;
    memcpy(dir, st->dir, sizeof(dir));
    if (st->dirJitter != 0) {
        float len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);

        for (i = 0; i < 3; i++)
            dir[i] += (len * (float)(tnRand(st->dirJitter * 2) - st->dirJitter)) / PARTICLE_JITTER_PERCENT;
    }
    rec = data + tail * PARTICLE_REC_SIZE;
    memcpy(rec + PARTICLE_REC_BORN, &now, sizeof(now));
    memcpy(rec + PARTICLE_REC_TICK, &now, sizeof(now));
    memcpy(&ee, copy + PARTICLE_OBJ_BLOCK, sizeof(ee));
    memcpy(rec + PARTICLE_REC_POS, BANK_PTR(const u_char *, bank, ee), PARTICLE_REC_BLOCK_SIZE);
    ee = BankEeAddr(bank, rec + PARTICLE_REC_POS);
    memcpy(copy + PARTICLE_OBJ_BLOCK, &ee, sizeof(ee));
    if (st->sizeJitter != 0) {
        float scale = (float)(tnRand(st->sizeJitter * 2) - st->sizeJitter) / PARTICLE_JITTER_PERCENT + 1.0f;
        float *size = (float *)(rec + PARTICLE_REC_SIZE_W);

        size[0] *= scale;
        size[1] *= scale;
    }
    memcpy(rec + PARTICLE_REC_BASE_W, rec + PARTICLE_REC_SIZE_W, 2 * sizeof(float));
    if (ent->colors != 0) {
        ee = ent->colors + tail * PARTICLE_COLORS_STRIDE;
        memcpy(copy + PARTICLE_OBJ_COLORS, &ee, sizeof(ee));
    }
    m = ParticlePartMatrix(st, bank);
    ApplyMatrix4((float *)(rec + PARTICLE_REC_DIR), m, dir);
    if (meshPos == NULL)
        ApplyMatrix4((float *)(rec + PARTICLE_REC_POS), m, (const float *)(rec + PARTICLE_REC_POS));
    else
        ApplyMatrix4((float *)(rec + PARTICLE_REC_POS), (float (*)[4])meshMat, meshPos);
    memcpy(rec + PARTICLE_REC_START, rec + PARTICLE_REC_POS, 4 * sizeof(float));
    memcpy(objs + tail * PARTICLE_OBJ_SIZE, copy, sizeof(copy));
    st->tail = next;
    return 1;
}

// CT3 0x018983a0
int ssParticlePush_Back(u_char *objs, u_char *data, u_int obj, PARTICLE_STATE *st,
                        const PARTICLE_ENTRY *ent, int now, int bank)
{
    return ParticlePushSlot(objs, data, obj, st, ent, NULL, NULL, now, bank);
}

// CT3 0x01898020
// The slot placed at mesh vertex `vert` (the emitter's vertices through its matrix, word 0).
int ssMeshParticlePush_Back(u_char *objs, u_char *data, u_int obj, PARTICLE_STATE *st,
                            const PARTICLE_ENTRY *ent, u_int vert, int now, int bank)
{
    const u_char *em = BANK_PTR(const u_char *, bank, st->emitter);
    u_int matEE;
    u_int vertsEE;

    memcpy(&matEE, em, sizeof(matEE));
    memcpy(&vertsEE, em + PARTICLE_EMITTER_VERTS, sizeof(vertsEE));
    return ParticlePushSlot(objs, data, obj, st, ent,
                            BANK_PTR(const float *, bank, vertsEE) + vert * 4,
                            BANK_PTR(const float (*)[4], bank, matEE), now, bank);
}

// CT3 0x0189a640
// The mesh's vertices are drawn without repeats: the cursor counts down from count, a random one of the
// first cursor + 1 is emitted from and swapped to the cursor's place (a Fisher-Yates shuffle, in place in
// the bank's vertex list); at 0 the cursor starts over.
int ssMeshParticleGetNextVertID2(u_int *vert, u_short count, u_int verts, u_char *objs, u_char *data,
                                 const PARTICLE_ENTRY *ent, PARTICLE_STATE *st, int now, int bank)
{
    float (*v)[4] = BANK_PTR(float (*)[4], bank, verts);
    u_int pick;

    if (*vert == 0)
        *vert = count;
    pick = (u_int)tnRand((int)*vert);
    *vert -= 1;
    if (ssMeshParticlePush_Back(objs, data, ent->obj, st, ent, pick, now, bank) == 0)
        return 0;
    if (pick != *vert) {
        float t[4];

        memcpy(t, v[*vert], sizeof(t));
        memcpy(v[*vert], v[pick], sizeof(t));
        memcpy(v[pick], t, sizeof(t));
    }
    return 1;
}

// ---- the first particle kind's motions (ssCalcParticleOrbit) -----------------------------------------------

#define PARTICLE_ORBIT_PLAIN 0          /* PARTICLE_STATE.orbit: accel and direction (ssParticleSetPosition) */
#define PARTICLE_ORBIT_GOAL 1           /* pulled toward part2's position */
#define PARTICLE_ORBIT_GOAL_STOP 2      /* the same, standing still within the stop distance */
#define PARTICLE_ORBIT_WIND 4           /* tiParticleSetPosition_WIND (Clipping_Portal.c) */
#define PARTICLE_ORBIT_TWIST 6          /* tiParticleSetPosition_Twist */
#define PARTICLE_TICKS_PER_SECOND 300.0f

// The slot's seconds since its last move (PARTICLE_REC_TICK), which becomes now.
static float ParticleStep(u_char *rec, int now)
{
    int last;

    memcpy(&last, rec + PARTICLE_REC_TICK, sizeof(last));
    memcpy(rec + PARTICLE_REC_TICK, &now, sizeof(now));
    return (float)(now - last) / PARTICLE_TICKS_PER_SECOND;
}

// The ROM's step (xyz; w kept): pos += dir * dt + a * dt^2, dir += a * 2dt; the object's centre follows.
static void ParticleIntegrate(u_char *obj, u_char *rec, const float *a, float dt)
{
    float *pos = (float *)(rec + PARTICLE_REC_POS);
    float *dir = (float *)(rec + PARTICLE_REC_DIR);
    int i;

    for (i = 0; i < 3; i++) {
        float move = dir[i] * dt + a[i] * (dt * dt);

        dir[i] += a[i] * (dt * 2.0f);
        pos[i] += move;
    }
    gaCopyVectorXYZ((float *)(obj + DRAW_OBJ_CENTER), pos);
}

// The pull toward the goal (part2's position): its unit vector times accel.x; dist: the distance.
static void ParticleGoalPull(const PARTICLE_STATE *st, const u_char *rec, float *a, float *dist, int bank)
{
    const float *goal = BANK_PTR(const float *, bank, st->partPos);
    const float *pos = (const float *)(rec + PARTICLE_REC_POS);
    float d[3];
    float len;
    int i;

    for (i = 0; i < 3; i++)
        d[i] = goal[i] - pos[i];
    len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    for (i = 0; i < 3; i++)
        a[i] = d[i] / len * st->accel[0];
    *dist = len;
}

// CT3 0x018997f0
// Orbit PLAIN: the slot moves along its direction under the system's acceleration.
void ssParticleSetPosition(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now)
{
    float dt = ParticleStep(rec, now);

    ParticleIntegrate(obj, rec, st->accel, dt);
}

// CT3 0x018994e0
// A goal orbit whose goal part is off (PARTS_FLAG_FOG on part2): the slot drifts along its direction.
void tiParticleSetPositionGoalDisable(u_char *obj, u_char *rec, int now)
{
    float dt = ParticleStep(rec, now);
    float *pos = (float *)(rec + PARTICLE_REC_POS);
    const float *dir = (const float *)(rec + PARTICLE_REC_DIR);
    int i;

    for (i = 0; i < 3; i++)
        pos[i] += dir[i] * dt;
    gaCopyVectorXYZ((float *)(obj + DRAW_OBJ_CENTER), pos);
}

// CT3 0x018996c0
// Orbit GOAL: accelerated toward the goal part's position. PORT: bank resolves partPos.
void tiParticleSetPositionToGoal(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now, int bank)
{
    float dt = ParticleStep(rec, now);
    float a[3];
    float dist;

    ParticleGoalPull(st, rec, a, &dist, bank);
    ParticleIntegrate(obj, rec, a, dt);
}

// CT3 0x01899550
// Orbit GOAL_STOP: as GOAL while farther than accel.y from the goal; within it the slot stands still.
void tiParticleSetPositionToGoal_Stop(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now, int bank)
{
    float dt = ParticleStep(rec, now);
    float a[3];
    float dist;

    ParticleGoalPull(st, rec, a, &dist, bank);
    if (st->accel[1] < dist)
        ParticleIntegrate(obj, rec, a, dt);
}

// CT3 0x01899320
// Orbit TWIST: the plain step, then the position turned by twistAngle about the new direction
// (asMakeRotMatrixByQuartanion), around a centre twistRadius from the start point: the start point less
// the unit vector of (dir + (1,1,1)) x dir times the radius. VU0: the turn is T(-c) R T(c) applied to the
// whole qword (row vectors), and the record's position and the object's centre both get all four words.
void tiParticleSetPosition_Twist(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now)
{
    float *pos = (float *)(rec + PARTICLE_REC_POS);
    float *dir = (float *)(rec + PARTICLE_REC_DIR);
    const float *start = (const float *)(rec + PARTICLE_REC_START);
    alignas(16) float rot[4][4];
    float m[4][4];
    float up[3], n[3], c[3], out[4], q;
    int i, j;

    ParticleIntegrate(obj, rec, st->accel, ParticleStep(rec, now));
    asMakeRotMatrixByQuartanion(rot, dir, st->twistAngle);
    for (i = 0; i < 3; i++)
        up[i] = dir[i] + 1.0f;                          /* vaddw.xyz: + vf0w */
    n[0] = up[1] * dir[2] - dir[1] * up[2];             /* vopmula / vopmsub: up x dir */
    n[1] = up[2] * dir[0] - dir[2] * up[0];
    n[2] = up[0] * dir[1] - dir[0] * up[1];
    q = 1.0f / sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    for (i = 0; i < 3; i++)
        c[i] = start[i] - n[i] * q * (float)st->twistRadius;
    /* T(-c) R: R's rows 0..2, row 3 R[3] - c . R; then (T(-c) R) T(c) adds c * w to each row's xyz */
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 3; i++)
            m[i][j] = rot[i][j];
        m[3][j] = rot[3][j] - c[0] * rot[0][j] - c[1] * rot[1][j] - c[2] * rot[2][j];
    }
    for (i = 0; i < 4; i++)
        for (j = 0; j < 3; j++)
            m[i][j] += c[j] * m[i][3];
    for (j = 0; j < 4; j++)
        out[j] = pos[0] * m[0][j] + pos[1] * m[1][j] + pos[2] * m[2][j] + pos[3] * m[3][j];
    memcpy(pos, out, sizeof(out));
    memcpy(obj + DRAW_OBJ_CENTER, out, sizeof(out));
}

// The live slots head .. tail of a ring (wrapping at ringSize): the first, then each next; -1 at the end.
static int ParticleFirst(const PARTICLE_STATE *st)
{
    return st->head == st->tail ? -1 : st->head;
}

static int ParticleNext(const PARTICLE_STATE *st, int slot)
{
    slot = (slot + 1) % st->ringSize;
    return slot == st->tail ? -1 : slot;
}

#define FOR_LIVE_SLOTS(st, k) for ((k) = ParticleFirst(st); (k) >= 0; (k) = ParticleNext((st), (k)))

// CT3 0x018998e0
// Per pass (ssParticle_main): every live slot moves by the system's orbit kind, then the size, alpha, UV
// and colour curves run over all of them, each pass with a cleared curve cache. A goal orbit whose goal
// part has PARTS_FLAG_FOG set drifts instead. Orbit kinds 3 and 5 move nothing (ROM).
void ssCalcParticleOrbit(const PARTICLE_ENTRY *ent, u_char *objs, u_char *data, int now, int bank)
{
    PARTICLE_STATE *st = BANK_PTR(PARTICLE_STATE *, bank, ent->state);
    int goalOff = 0;
    int k;

    if (st->head == st->tail)
        return;
    if (st->orbit == PARTICLE_ORBIT_GOAL || st->orbit == PARTICLE_ORBIT_GOAL_STOP)
        goalOff = (((MODEL_PARTS *)pModelPartsHeader[bank])[st->part2].flags & PARTS_FLAG_FOG) != 0;
    FOR_LIVE_SLOTS(st, k) {
        u_char *obj = objs + k * PARTICLE_OBJ_SIZE;
        u_char *rec = data + k * PARTICLE_REC_SIZE;

        switch (st->orbit) {
        case PARTICLE_ORBIT_TWIST:
            tiParticleSetPosition_Twist(st, obj, rec, now);
            break;
        case PARTICLE_ORBIT_WIND:
            tiParticleSetPosition_WIND((const float *)st, obj, rec, now);
            break;
        case PARTICLE_ORBIT_GOAL_STOP:
            if (goalOff)
                tiParticleSetPositionGoalDisable(obj, rec, now);
            else
                tiParticleSetPositionToGoal_Stop(st, obj, rec, now, bank);
            break;
        case PARTICLE_ORBIT_GOAL:
            if (goalOff)
                tiParticleSetPositionGoalDisable(obj, rec, now);
            else
                tiParticleSetPositionToGoal(st, obj, rec, now, bank);
            break;
        case PARTICLE_ORBIT_PLAIN:
            ssParticleSetPosition(st, obj, rec, now);
            break;
        default:
            break;
        }
    }
    if (st->sizeCurve != 0) {
        ssParticleCashClear();
        FOR_LIVE_SLOTS(st, k)
            ssParticleSetSize(BANK_PTR(void *, bank, st->sizeCurve), data + k * PARTICLE_REC_SIZE, now);
    }
    if (st->alphaCurve != 0) {
        u_int colorsEE;
        const s_char *src;

        ssParticleCashClear();
        memcpy(&colorsEE, BANK_PTR(const u_char *, bank, ent->obj) + PARTICLE_OBJ_COLORS, sizeof(colorsEE));
        src = BANK_PTR(const s_char *, bank, colorsEE) + PARTICLE_COLOR_BASE;
        FOR_LIVE_SLOTS(st, k)
            ssParticleSetAlpha(BANK_PTR(void *, bank, st->alphaCurve), objs + k * PARTICLE_OBJ_SIZE,
                               data + k * PARTICLE_REC_SIZE, src, now, bank);
    }
    if (st->uvCurve != 0) {
        ssParticleCashClear();
        FOR_LIVE_SLOTS(st, k)
            ssParticleSetUV(BANK_PTR(void *, bank, st->uvCurve), objs + k * PARTICLE_OBJ_SIZE,
                            data + k * PARTICLE_REC_SIZE, now);
    }
    if (st->vrgbCurve != 0) {
        ssParticleCashClear();
        FOR_LIVE_SLOTS(st, k)
            ssParticleSetVRGB(BANK_PTR(void *, bank, st->vrgbCurve), objs + k * PARTICLE_OBJ_SIZE,
                              data + k * PARTICLE_REC_SIZE, now, bank);
    }
}
