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

// ---- pendulums (huriko; tiHurikoUpdate, 2026-11-11 slice 3) ----
// A bank's pendulums (ALL_TABLE.huriko, a 0-ended list of EE words). layout: ee -- bank data; only the
// trigger part and the flags are read so far (tiCalc_HurikoOPEN / _CLOSED, tiHurikoReset name the rest).
typedef struct HURIKO {
    short trigPart;                     /* 0x00: ssTrigCheckEnable's part: the pendulum swings while enabled;
                                           tiHurikoInit reads the word as its model part (the high half 0),
                                           tiHurikoReset_On compares the word with its id */
    u_char pad02[2];
    u_short chains;                     /* 0x04: chains of joints (tiHurikoInit) */
    u_short joints;                     /* 0x06: joints per chain, the root first */
    u_char pad08[8];
    u_int jointTable;                   /* 0x10: EE word -> u_int[chains * joints]: HURIKO_JOINT EE words,
                                           chain-major */
    u_char pad14[4];
    u_int flags;                        /* 0x18: HURIKO_FLAG_* */
} HURIKO;
static_assert(offsetof(HURIKO, chains) == 0x04, "HURIKO.chains");
static_assert(offsetof(HURIKO, jointTable) == 0x10, "HURIKO.jointTable");
static_assert(offsetof(HURIKO, flags) == 0x18, "HURIKO.flags");

// One joint of a pendulum chain (tiHurikoInit, 2026-11-11 slice 4). layout: ee -- bank data.
typedef struct HURIKO_JOINT {
    u_int posRef;                       /* 0x00: EE word -> an EE word -> the joint's part-local position */
    u_char pad04[8];
    float length;                       /* 0x0c: the rest offset from the joint above, straight down */
    u_char pad10[0x10];
    u_int up;                           /* 0x20: EE address of the joint above's position (k >= 1) */
    u_int down;                         /* 0x24: ... of the joint below's (not set on the last joint) */
    u_int left;                         /* 0x28: ... of the same joint in the previous chain (cyclic) */
    u_int right;                        /* 0x2c: ... in the next chain (cyclic) */
    float restDir[4];                   /* 0x30: part-local: world pos - (up's world pos + (0,-1,0) * length),
                                           w 0 (k >= 1) */
    u_char pad40[0x20];
    float pos[4];                       /* 0x60: k >= 1: the world position's xyz; the root: its local
                                           position (all four words) */
} HURIKO_JOINT;
static_assert(offsetof(HURIKO_JOINT, length) == 0x0c, "HURIKO_JOINT.length");
static_assert(offsetof(HURIKO_JOINT, up) == 0x20, "HURIKO_JOINT.up");
static_assert(offsetof(HURIKO_JOINT, restDir) == 0x30, "HURIKO_JOINT.restDir");
static_assert(offsetof(HURIKO_JOINT, pos) == 0x60, "HURIKO_JOINT.pos");
#define HURIKO_REST_DOWN_Y -1.0f        /* @338 (0x019389f0) = (0, -1, 0, 0): a joint hangs straight down */

// A pendulum's hit (ALL_TABLE.hurikoHits, a 0-ended list of EE words): two part-local points carried
// into the part's frame each frame (tiHurikoHitUpdate). layout: ee -- bank data.
typedef struct HURIKO_HIT {
    int part;                           /* 0x00: its model part (PARTS_KIND_WIND_ON_ROOT: part 0's) */
    short pairInA;                      /* 0x04: non-zero: both points at a (a[0], a[1]); 0: a and b */
    u_char pad06[2];
    u_int partFlags;                    /* 0x08: EE address of the part's MODEL_PARTS.flags (tiHurikoInit);
                                           PARTS_FLAG_HIDDEN: not carried */
    u_int a;                            /* 0x0c: EE word -> the first point (two with pairInA) */
    u_int b;                            /* 0x10: EE word -> the second point */
    u_int matrix;                       /* 0x14: EE address of the part's MODEL_PARTS.matrix200 (tiHurikoInit) */
    u_char pad18[8];
    float world0[4];                    /* 0x20 */
    float world1[4];                    /* 0x30 */
} HURIKO_HIT;
static_assert(offsetof(HURIKO_HIT, matrix) == 0x14, "HURIKO_HIT.matrix");
static_assert(offsetof(HURIKO_HIT, world0) == 0x20, "HURIKO_HIT.world0");
static_assert(sizeof(HURIKO_HIT) == 0x40, "HURIKO_HIT");
#define HURIKO_FLAG_CLOSED 0x1          /* tiCalc_HurikoCLOSED swings it, else tiCalc_HurikoOPEN */
#define HURIKO_FLAG_DISABLED 0x4        /* its trigger was off (set each frame it is off) */
#define HURIKO_FLAG_RESET 0x8           /* re-enabled: tiHurikoReset on the next enabled frame */

// ---- particles (tiParticleUpdate, 2026-11-11 slice 3) ----
// PORT: the bank is added to the helpers that read a PARTICLE2_ENTRY's ring (an EE word in the bank).
struct PARTICLE2_ENTRY;
struct CAMERA;
void tiCalc_LifeSpan(struct PARTICLE2_ENTRY *ent, int bank);                            // CT3 0x0188f9a0
// ROM (ticks, entry, enabled); PORT: bank added.
void tiGenerateParticle(int ticks, struct PARTICLE2_ENTRY *ent, int enabled, int bank);  // CT3 0x0188fba0
// after: 0 for ALL_TABLE.particles2, 1 for particles2After; cam: Cam[0] for the scene banks, else Cam[6].
void tiCalc_Orbit(int ticks, struct PARTICLE2_ENTRY *ent, int after, struct CAMERA *cam, int bank); // CT3 0x0188f2b0
void tiHurikoReset(int bank, HURIKO *h);                                                 // CT3 0x01890c00
void tiCalc_HurikoOPEN(HURIKO *h, int bank, int ticks);                                  // CT3 0x01892310
void tiCalc_HurikoCLOSED(HURIKO *h, int bank, int ticks);                                // CT3 0x018915a0
void tiGetNewNormalData(HURIKO *h);                                                      // CT3 0x018912b0
// dst.xyz = src.xyz / |src.xyz|; w scaled the same (VU0: the whole vector times rsqrt(x2+y2+z2)).
void gaNormalizeVectorXYZ(float *dst, const float *src);                                 // CT3 0x01891570
float gaNormalizeVector3XYZ(float *dst, const float *src);                              // CT3 0x018922c0

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
// work: the 3dlib work (GA_3DLIB_WORK, ga3dlibwork); screen: screen_0's twelve floats (GA_SCREEN_*).
void Clipping_Portal(void *work, float *screen);                                        // CT3 0x01889cd0
// thigh..ik: CHARACTER.legParts' records (host pointers); target: the floor point; footY: out.
// PORT: the bank is added in front. Answers 1 when the leg was bent.
int tiCalc_FootPos(int bank, u_char *thigh, u_char *calf, u_char *foot, u_char *ik, float *target,
                   float *footY);                                                       // CT3 0x0188b240
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
