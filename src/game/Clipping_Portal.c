// game/Clipping_Portal.c -- Clock Tower 3: the ti* effects module (wind, vibration, fades, light belts,
// lens flares, pendulums, particles, translucence; symtab local chunk 88). CodeWarrior C. Slice 1
// (2026-11-11): vibration, wind, tiLightBelt2Init, tiFade_Start / _Draw, tiFactorial; the rest are stubs until reconstructed.

#include "common/alyssapan_stub.h"
#include "game/Clipping_Portal.h"
#include "game/LinkMatrixTree.h"
#include "game/ct3_types.h"
#include "game/main.h"
#include "game/unassigned_data.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaInitDrawObject.h"
#include "sdk/eekernel.h"
#include "ga/pad_button_read.h"
#include "alyssapan/rendering/alyssapan_drawga.h"

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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x01893850
void tiBumpSTValueUpdate(void)
{
    ALYSSAPAN_STUB("tiBumpSTValueUpdate", 0x01893850);
}

// stub of CT3 0x0188b790
void tiCharaFootAdjust(void)
{
    ALYSSAPAN_STUB("tiCharaFootAdjust", 0x0188b790);
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

// stub of CT3 0x018931f0
void tiHurikoHitUpdate(void)
{
    ALYSSAPAN_STUB("tiHurikoHitUpdate", 0x018931f0);
}

// stub of CT3 0x01892ee0
void tiHurikoUpdate(void)
{
    ALYSSAPAN_STUB("tiHurikoUpdate", 0x01892ee0);
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

// stub of CT3 0x01890070
void tiParticleUpdate(void)
{
    ALYSSAPAN_STUB("tiParticleUpdate", 0x01890070);
}

// stub of CT3 0x0188bb60
void tiPipeJointUpdate(void)
{
    ALYSSAPAN_STUB("tiPipeJointUpdate", 0x0188bb60);
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

// stub of CT3 0x01889cd0
void Clipping_Portal(void *work, void *screen)
{
    ALYSSAPAN_STUB("Clipping_Portal", 0x01889cd0);
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

// stub of CT3 0x018933f0
void tiHurikoInit(int scene)
{
    ALYSSAPAN_STUB("tiHurikoInit", 0x018933f0);
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

// stub of CT3 0x0188bf30
void tiPipeJointInit(int bank)
{
    ALYSSAPAN_STUB("tiPipeJointInit", 0x0188bf30);
}

// stub of CT3 0x0188ddc0
void tiSet_CharaTranslucenceOFF(int chara, int flag)
{
    ALYSSAPAN_STUB("tiSet_CharaTranslucenceOFF", 0x0188ddc0);
}

// stub of CT3 0x0188ded0
void tiSet_CharaTranslucenceON(int chara, int level, int flag)
{
    ALYSSAPAN_STUB("tiSet_CharaTranslucenceON", 0x0188ded0);
}

// stub of CT3 0x0188d6e0
void tiXFadeOut_Start(float time)
{
    ALYSSAPAN_STUB("tiXFadeOut_Start", 0x0188d6e0);
}

// stub of CT3 0x018949c0
void tiGetTransRotMatrix(float (*dst)[4], float *pos, float (*src)[4])
{
    ALYSSAPAN_STUB("tiGetTransRotMatrix", 0x018949c0);
}

// stub of CT3 0x01890ea0
void tiHurikoReset_On(int bank, int id, int byChara)
{
    ALYSSAPAN_STUB("tiHurikoReset_On", 0x01890ea0);
}

// stub of CT3 0x0188d5e0
void tiXFadeOut_Start2(float time, u_char kind)
{
    ALYSSAPAN_STUB("tiXFadeOut_Start2", 0x0188d5e0);
}
