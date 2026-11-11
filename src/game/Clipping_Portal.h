// game/Clipping_Portal.h -- CT3 game/Clipping_Portal.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// ---- wind (tiWindInit / tiWindUpdate / tiWindEnd, 2026-11-11) ----
// A bank's wind records (ALL_TABLE.winds, a 0-ended list of EE words). layout: ee -- bank data; matrix
// is a relocated EE address (BankEePtr of the record's bank). Each frame tiWindUpdate carries the three
// local points into the frame of the part's matrix200; the readers of the world points (the particle
// and pendulum code, later slices) name them.
typedef struct WIND {
    int bank;                           /* 0x00: the bank that registered it (tiWindInit); tiWindEnd drops it */
    int part;                           /* 0x04: the model part whose matrix200 carries it */
    u_int pad08;
    u_int matrix;                       /* 0x0c: EE address of that part's matrix200 (part 0's when the
                                           part's kind is PARTS_KIND_WIND_ON_ROOT) */
    float local10[4];                   /* 0x10: a point in the part's frame -> world10 */
    float world10[4];                   /* 0x20 */
    float local30[4];                   /* 0x30 -> world30 */
    float local40[4];                   /* 0x40 -> world40 */
    float world30[4];                   /* 0x50 */
    float world40[4];                   /* 0x60 */
} WIND;
static_assert(offsetof(WIND, matrix) == 0x0c, "WIND.matrix");
static_assert(offsetof(WIND, world10) == 0x20, "WIND.world10");
static_assert(offsetof(WIND, world40) == 0x60, "WIND.world40");
#define WIND_MAX 10                     /* WindList's slots */
#define PARTS_KIND_WIND_ON_ROOT 1       /* MODEL_PARTS.kind: the wind takes part 0's frame (= PARTS_KIND_ITEM_LOCKED;
                                           named from this use) */
extern WIND *WindList[WIND_MAX];                /* sbss 0x0193d290 */

// ---- light belts (tiLightBelt2Init, 2026-11-11; tiLightBelt2Update reads them, a later slice) ----
// A bank's light belts (ALL_TABLE.lightBelts, a 0-ended list of EE words). layout: ee -- bank data;
// points is a relocated EE address. A belt either sits on one part (link 0: every point at the part's
// position) or spans two parts: halfA / halfB are half the span, each in its own part's frame (w 0).
typedef struct LIGHT_BELT2 {
    int partA;                          /* 0x00: the model part it starts at */
    u_short pad04;
    u_short count;                      /* 0x06: points[]'s length */
    u_int pad08;
    int link;                           /* 0x0c: 0 = on partA alone; else spans partA .. partB */
    u_int points;                       /* 0x10: EE address of float[count][4] */
    int partB;                          /* 0x14: the part it ends at (link != 0) */
    u_int pad18[2];
    float halfA[4];                     /* 0x20: (B.pos - A.pos) / 2 in A's frame (inverse of A's matrix), w 0 */
    float halfB[4];                     /* 0x30: (A.pos - B.pos) / 2 in B's frame, w 0 */
} LIGHT_BELT2;
static_assert(offsetof(LIGHT_BELT2, points) == 0x10, "LIGHT_BELT2.points");
static_assert(offsetof(LIGHT_BELT2, halfA) == 0x20, "LIGHT_BELT2.halfA");
static_assert(sizeof(LIGHT_BELT2) == 0x40, "LIGHT_BELT2");

// ---- vibration ----
// Ticks run 5 a frame (LinkMatrixTree.c): tiVibrationOn's time is in frames.
#define VIB_TICKS_PER_FRAME 5
#define VIB_LARGE_SCALE 0.75f           /* tiVibrationOn: the large motor adds level * 0.75, capped at 255 */
#define VIB_LARGE_MAX 0xff
extern int VibTimeBaseTicks;                    /* sbss 0x0193f770: the vibration stops once VibTimeTicks passes it */

// ---- fade ----
#define FADE_TICKS_PER_SECOND 30.0f     /* tiFade_Start: FadeEndTicks = seconds * 30 */

void tiLightBelt2UpdateForALYSSA_Battle(void);                                           // CT3 0x01889f70
void tiLightBelt2UpdateForBOSS_Battle(void);                                             // CT3 0x0188a1f0
void tiLightBelt2Update(void);                                                           // CT3 0x0188a610
void tiCharaFootAdjust(void);                                                            // CT3 0x0188b790
void tiPipeJointUpdate(void);                                                            // CT3 0x0188bb60
void tiWindEnd(int scene);                                                               // CT3 0x0188cb10
void tiWindUpdate(void);                                                                 // CT3 0x0188cbe0
void tiVibrationTimer(void);                                                             // CT3 0x0188cd70
void tiVibrationOff(int port);                                                           // CT3 0x0188cdc0
// small: the small motor on (1) or off; large: the large motor's level (1 counts as 2; adds 0.75 of it to
// act_large); time: frames until tiVibrationTimer stops it, 0 = until tiVibrationOff.
void tiVibrationOn(int port, u_char small, int large, int time);                          // CT3 0x0188ce10
// base ** n (the name says factorial; the code is a power). tiDrawShadingOff's.
int tiFactorial(int base, u_int n);                                                       // CT3 0x01894870
void tiDrawShadingOff(void);                                                             // CT3 0x0188ced0
// time: seconds (f12; x30 frames), type: Fade_Type (a0, a byte). MemorycardUpdate leaves a0 unset (open-items.md).
#define FADE_TYPE_MC_LOAD 4             /* mc_load_set (name from that use; = FADE_TYPE_OUT2_BLACK) */
// tiXcmd_FadeStart's kinds (2026-11-11): direction by twos, white the odd one.
#define FADE_TYPE_IN_BLACK 0
#define FADE_TYPE_OUT_BLACK 1
#define FADE_TYPE_IN_WHITE 2
#define FADE_TYPE_OUT_WHITE 3
#define FADE_TYPE_OUT2_BLACK 4
#define FADE_TYPE_OUT2_WHITE 5
#define FADE_TYPE_IN2_BLACK 6
#define FADE_TYPE_IN2_WHITE 7
void tiFade_Start(float time, int type);                                                 // CT3 0x0188d3c0
void tiFade_Draw(void);                                                                // CT3 0x0188d160
void tiXFadeOut_Draw(void);                                                              // CT3 0x0188d3f0
void tiDrawLensFlares(void);                                                             // CT3 0x0188df70
void tiParticleUpdate(void);                                                             // CT3 0x01890070
void tiHurikoUpdate(void);                                                               // CT3 0x01892ee0
void tiHurikoHitUpdate(void);                                                            // CT3 0x018931f0
void tiBumpSTValueUpdate(void);                                                          // CT3 0x01893850
void tiRefractionMapUpdate(void);                                                        // CT3 0x01894230
void tiFreeBossData(void);                                                               // CT3 0x01889df0
void tiWindEnd_All(void);                                                                // CT3 0x0188cb60
void ExportDrawToTex(void);                                                              // CT3 0x01893cf0
void Clipping_Portal(void *work, void *screen);                                         // CT3 0x01889cd0
void tiChara_Translucence_End2(int fbuf, int chara);                                     // CT3 0x0188d720
void tiChara_Translucence_Start2(void);                                                  // CT3 0x0188dbd0
void tiChara_Translucence_End(float alpha, int fbuf);                                    // CT3 0x0188dca0
void tiChara_Translucence_Start(void);                                                   // CT3 0x0188dd40
void tiLightBelt2Init(int scene);                                                        // CT3 0x0188b050
void tiWindInit(int scene);                                                              // CT3 0x0188ccb0
void tiLensFlaresInit(int scene);                                                        // CT3 0x0188e4a0
void tiParticleInit(int scene);                                                          // CT3 0x01890350
void tiHurikoInit(int scene);                                                            // CT3 0x018933f0
void tiPipeJointInit(int bank);                                                          // CT3 0x0188bf30 (main2.c: chara 3's shape)

// From ssTrigExecuteActCmd's commands (2026-11-11): the arguments the command passes; the last is 1 there.
void tiSet_CharaTranslucenceON(int chara, int level, int flag);                          // CT3 0x0188ded0
void tiSet_CharaTranslucenceOFF(int chara, int flag);                                    // CT3 0x0188ddc0
void tiXFadeOut_Start(float time);                                                       // CT3 0x0188d6e0
// From tiXcmd_XFadeOut2Start (2026-11-11): time in seconds, kind a byte.
void tiXFadeOut_Start2(float time, u_char kind);                                         // CT3 0x0188d5e0

// From tiXcmd_TransItem.c (2026-11-11). dst = src turned about the point pos (pos's translation out,
// src, then back: two gaMulMatrix with matUnit + -pos / + pos rows).
void tiGetTransRotMatrix(float (*dst)[4], float *pos, float (*src)[4]);                  // CT3 0x018949c0
// Sets bit 2 of the pendulums (huriko) of bank `bank` whose id is `id` (0: all); byChara: `bank` is a
// Character[] index and its bank is taken.
void tiHurikoReset_On(int bank, int id, int byChara);                                    // CT3 0x01890ea0
