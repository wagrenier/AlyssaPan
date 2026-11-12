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
        if (max->mode != PART_BLUR_MODE_TRAIL) {
            int j;

            for (j = 0; j < max->count; j++) {
                DRAW_OBJ_ENTRY *ent = objs[j].entry;
                GA_DRAWCTRL_ARGS *args = BANK_PTR(GA_DRAWCTRL_ARGS *, scene, ent->drawCtrlArgs);

                *(u_int *)(BANK_PTR(u_char *, scene, ent->obj) + DRAW_OBJ_POS) = max->ambPos;
                *args = max->args;
                gaSetupDrawCtrlData(BANK_PTR(GA_DRAWCTRL_DATA *, scene, ent->drawCtrl), *args);
            }
        }
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

// stub of CT3 0x01896720 -- copies each blurred entry (its draw object, matrices and header) into the ring's
// tail slot; needs the ring copies' layout, which ssPartMotionBlurStart builds.
void ssPartMotionBlurPush_Back(PART_BLUR_MAX_OBJ *max)
{
    (void)max;
    ALYSSAPAN_STUB("ssPartMotionBlurPush_Back", 0x01896720);
}

// stub of CT3 0x01897430 -- fills every ring copy with a matrix between prevMatrix and the part's own.
void tiOneFrameMotionBlurUpdate(int bank, PART_BLUR_MAX_OBJ *max)
{
    (void)bank;
    (void)max;
    ALYSSAPAN_STUB("tiOneFrameMotionBlurUpdate", 0x01897430);
}

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

// stub of CT3 0x0189cd60
void ss2DParticle_main(void)
{
    ALYSSAPAN_STUB("ss2DParticle_main", 0x0189cd60);
}

// stub of CT3 0x0189d940
void ssBGT_BlurDrawSprite(int level)
{
    ALYSSAPAN_STUB("ssBGT_BlurDrawSprite", 0x0189d940);
}

// stub of CT3 0x0189d9b0
void ssBGT_BlurSetDrawEnvPostRend(void)
{
    ALYSSAPAN_STUB("ssBGT_BlurSetDrawEnvPostRend", 0x0189d9b0);
}

// stub of CT3 0x0189db60
void ssBGT_DOFDrawSprite(int pass, int level)
{
    ALYSSAPAN_STUB("ssBGT_DOFDrawSprite", 0x0189db60);
}

// stub of CT3 0x0189dc40
void ssBGT_DOFSetDrawEnvPostRend(int pass, int level)
{
    ALYSSAPAN_STUB("ssBGT_DOFSetDrawEnvPostRend", 0x0189dc40);
}

// stub of CT3 0x0189dd70
void ssBGT_DOFSetDrawEnvPreRend(int pass, int level)
{
    ALYSSAPAN_STUB("ssBGT_DOFSetDrawEnvPreRend", 0x0189dd70);
}

// stub of CT3 0x0189d330
void ssBGT_HazyDrawSprite(int level, int transTime)
{
    ALYSSAPAN_STUB("ssBGT_HazyDrawSprite", 0x0189d330);
}

// stub of CT3 0x0189d560
void ssBGT_HazySetDrawEnvPostRend(void)
{
    ALYSSAPAN_STUB("ssBGT_HazySetDrawEnvPostRend", 0x0189d560);
}

// stub of CT3 0x0189d640
void ssBGT_UltimaDrawSprite(int *frame)
{
    ALYSSAPAN_STUB("ssBGT_UltimaDrawSprite", 0x0189d640);
}

// stub of CT3 0x0189d820
void ssBGT_UltimaSetDrawEnvPostRend(void)
{
    ALYSSAPAN_STUB("ssBGT_UltimaSetDrawEnvPostRend", 0x0189d820);
}

// stub of CT3 0x0189b7e0
void ssBGT_VaselineDrawDiamond(void)
{
    ALYSSAPAN_STUB("ssBGT_VaselineDrawDiamond", 0x0189b7e0);
}

// stub of CT3 0x0189b840
void ssBGT_VaselineSetDrawEnvPostRend(void)
{
    ALYSSAPAN_STUB("ssBGT_VaselineSetDrawEnvPostRend", 0x0189b840);
}

// stub of CT3 0x0189bff0
void ssBGT_WaveDrawDiamond(void)
{
    ALYSSAPAN_STUB("ssBGT_WaveDrawDiamond", 0x0189bff0);
}

// stub of CT3 0x0189c070
void ssBGT_WaveSetDrawEnvPostRend(void)
{
    ALYSSAPAN_STUB("ssBGT_WaveSetDrawEnvPostRend", 0x0189c070);
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

// stub of CT3 0x0189b510
void ssBGT_HQDOF_SetDrawEnvPreRend(void)
{
    ALYSSAPAN_STUB("ssBGT_HQDOF_SetDrawEnvPreRend", 0x0189b510);
}

// stub of CT3 0x0189b390
void ssBGT_HQDOF_WriteBackFB(int alpha)
{
    ALYSSAPAN_STUB("ssBGT_HQDOF_WriteBackFB", 0x0189b390);
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
    ParticleObject[scene] = (u_char **)_SunAlloc(&gHeapJunc, count * 4, 0x10);
    ParticleData[scene] = (u_char **)_SunAlloc(&gHeapJunc, count * 4, 0x10);
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

// stub of CT3 0x01897140
void ssPartMotionBlurEnd(int part, int bank)
{
    ALYSSAPAN_STUB("ssPartMotionBlurEnd", 0x01897140);
}

// stub of CT3 0x018976d0
void ssPartMotionBlurStart(int part, int a, int bank, int b, int c)
{
    ALYSSAPAN_STUB("ssPartMotionBlurStart", 0x018976d0);
}

// CT3 0x0189b580
// The trigger command's HQDOF end. ROM: it ends DRAW_MODE_VASELINE (7), not the DRAW_MODE_HQDOF (8)
// ssTrigHQDOFStart sets -- kept (hazards.md, 2026-11-12).
void ssTrigHQDOFEnd(void)
{
    if (DrawMode == DRAW_MODE_VASELINE)
        DrawMode = DRAW_MODE_NORMAL;
}

// stub of CT3 0x0189b5a0
void ssTrigHQDOFStart(int a, int b)
{
    ALYSSAPAN_STUB("ssTrigHQDOFStart", 0x0189b5a0);
}

// CT3 0x0189d2f0
// The trigger command's hazy screen off: back to the normal pass, if the hazy pass is the one on.
void ssTrigHazyScreenEnd(void)
{
    if (DrawMode == DRAW_MODE_HAZY)
        DrawMode = DRAW_MODE_NORMAL;
}

// stub of CT3 0x0189d310
void ssTrigHazyScreenStart(int level, int time)
{
    ALYSSAPAN_STUB("ssTrigHazyScreenStart", 0x0189d310);
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

// stub of CT3 0x018988b0
void ssTrigParticleStart(int particle, int bank, int arg)
{
    ALYSSAPAN_STUB("ssTrigParticleStart", 0x018988b0);
}

// CT3 0x018988a0
// The trigger command's particle stop: the particle's trigger part disabled at once.
void ssTrigParticleStop(int particle, int bank)
{
    ssTrigSetEnable(particle, 0, 0, bank);
}

// stub of CT3 0x0189d920
void ssTrigScreenBlurStart(int level)
{
    ALYSSAPAN_STUB("ssTrigScreenBlurStart", 0x0189d920);
}

// stub of CT3 0x0189b920
void ssTrigVaselineEnd(void)
{
    ALYSSAPAN_STUB("ssTrigVaselineEnd", 0x0189b920);
}

// stub of CT3 0x0189bd40
void ssTrigVaselineStart(int a, int b, int c, int d, int e, int f, int g, int h, int i)
{
    ALYSSAPAN_STUB("ssTrigVaselineStart", 0x0189bd40);
}

// stub of CT3 0x0189ce30
void ssTrigWaveEnd(void)
{
    ALYSSAPAN_STUB("ssTrigWaveEnd", 0x0189ce30);
}

// stub of CT3 0x0189ce60
void ssTrigWaveStart(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int bank,
                     int k)
{
    ALYSSAPAN_STUB("ssTrigWaveStart", 0x0189ce60);
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

// stub of CT3 0x01899320 -- orbit TWIST: the plain step, then the position turned about the direction
// (asMakeRotMatrixByQuartanion by twistAngle) around a centre twistRadius from the start point; the VU0
// sequence still to read.
void tiParticleSetPosition_Twist(const PARTICLE_STATE *st, u_char *obj, u_char *rec, int now)
{
    (void)st;
    (void)obj;
    (void)rec;
    (void)now;
    ALYSSAPAN_STUB("tiParticleSetPosition_Twist", 0x01899320);
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
