// game/ssSetTexUVforCubeMap.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Reconstructed: the ambient colour, the part motion blur's counters, init, update and end, the particle
// tables' end, the screen blur off and the Z clear (2026-11-12); the rest are stubs (tools/reverse/stage.py).

#include <stddef.h>
#include <string.h>

#include "common/alyssapan_stub.h"
#include "game/ssSetTexUVforCubeMap.h"
#include "ga/gaInitDrawObject.h"
#include "game/ahalloc.h"
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
        memset(&CharAmbChangeFlag[i], 0, offsetof(CHAR_AMB_CHANGE, pad18));
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

// stub of CT3 0x01895c10
void ReflectMapUpdate(void)
{
    ALYSSAPAN_STUB("ReflectMapUpdate", 0x01895c10);
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

// stub of CT3 0x01895fb0
void ssChangeCharAmbientColor(void)
{
    ALYSSAPAN_STUB("ssChangeCharAmbientColor", 0x01895fb0);
}

// stub of CT3 0x0189ae30
void ssParticle_main(void)
{
    ALYSSAPAN_STUB("ssParticle_main", 0x0189ae30);
}

// stub of CT3 0x01896a90
void ssPartMotionBlurInsertObjToDrawTable(int bank, void *table, u_int *list, int count, int mode)
{
    ALYSSAPAN_STUB("ssPartMotionBlurInsertObjToDrawTable", 0x01896a90);
}

// stub of CT3 0x0189da90
int ssDOF_ClipObjectFromZRange(u_char *obj, u_int mode)
{
    ALYSSAPAN_STUB("ssDOF_ClipObjectFromZRange", 0x0189da90);
    return {};
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

// stub of CT3 0x0189a9d0
void ssParticleInit(int scene)
{
    ALYSSAPAN_STUB("ssParticleInit", 0x0189a9d0);
}

// stub of CT3 0x018962a0
void tnChangeCharAmbientColorSet(short *set)
{
    ALYSSAPAN_STUB("tnChangeCharAmbientColorSet", 0x018962a0);
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

// stub of CT3 0x0189b580
void ssTrigHQDOFEnd(void)
{
    ALYSSAPAN_STUB("ssTrigHQDOFEnd", 0x0189b580);
}

// stub of CT3 0x0189b5a0
void ssTrigHQDOFStart(int a, int b)
{
    ALYSSAPAN_STUB("ssTrigHQDOFStart", 0x0189b5a0);
}

// stub of CT3 0x0189d2f0
void ssTrigHazyScreenEnd(void)
{
    ALYSSAPAN_STUB("ssTrigHazyScreenEnd", 0x0189d2f0);
}

// stub of CT3 0x0189d310
void ssTrigHazyScreenStart(int level, int time)
{
    ALYSSAPAN_STUB("ssTrigHazyScreenStart", 0x0189d310);
}

// stub of CT3 0x01898710
void ssTrigParticleAllOffImmediately(int particle, int bank)
{
    ALYSSAPAN_STUB("ssTrigParticleAllOffImmediately", 0x01898710);
}

// stub of CT3 0x018988b0
void ssTrigParticleStart(int particle, int bank, int arg)
{
    ALYSSAPAN_STUB("ssTrigParticleStart", 0x018988b0);
}

// stub of CT3 0x018988a0
void ssTrigParticleStop(int particle, int bank)
{
    ALYSSAPAN_STUB("ssTrigParticleStop", 0x018988a0);
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

// stub of CT3 0x01899070
void ssParticleCashClear(void)
{
    ALYSSAPAN_STUB("ssParticleCashClear", 0x01899070);
}

// stub of CT3 0x01898a60
void tiParticleGetSizeValue(int age, float *scale, void *curve, short *cache)
{
    (void)age;
    (void)scale;
    (void)curve;
    (void)cache;
    ALYSSAPAN_STUB("tiParticleGetSizeValue", 0x01898a60);
}

// stub of CT3 0x01898980
void tiParticleGetRefractive_IndexValue(int age, float *index, void *curve, short *cache)
{
    (void)age;
    (void)index;
    (void)curve;
    (void)cache;
    ALYSSAPAN_STUB("tiParticleGetRefractive_IndexValue", 0x01898980);
}

// stub of CT3 0x01898bc0
void ssParticleGetVRGBValue(int age, s_char *r, s_char *g, s_char *b, void *curve, short *cache)
{
    (void)age;
    (void)r;
    (void)g;
    (void)b;
    (void)curve;
    (void)cache;
    ALYSSAPAN_STUB("ssParticleGetVRGBValue", 0x01898bc0);
}

// stub of CT3 0x01898e60
void ssParticleGetAlphaValue(int age, short *alpha, void *curve, short *cache)
{
    (void)age;
    (void)alpha;
    (void)curve;
    (void)cache;
    ALYSSAPAN_STUB("ssParticleGetAlphaValue", 0x01898e60);
}

// stub of CT3 0x01898d20
void ssParticleGetUVValue(int age, void *u, void *v, void *curve, short *cache)
{
    (void)age;
    (void)u;
    (void)v;
    (void)curve;
    (void)cache;
    ALYSSAPAN_STUB("ssParticleGetUVValue", 0x01898d20);
}
