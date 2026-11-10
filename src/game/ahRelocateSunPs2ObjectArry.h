// game/ahRelocateSunPs2ObjectArry.h -- CT3 game/ahRelocateSunPs2ObjectArry.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

// asForceOperateAnimation_Scene / _Character's modes (BANK_FLAG_ANIM_* in ct3_types.h).
#define ANIM_FORCE_NONE 0               /* release: the bank follows the world's pause */
#define ANIM_FORCE_STOP 1               /* still while the world moves (and BANK_FLAG_ANIM_STOPPED) */
#define ANIM_FORCE_RUN 2                /* runs while the world is paused */
#define SKIN_WORK_SIZE 0x4000           /* asUpDateGAobjectsAnimation's SkinWork block */

int asCheckNowMoving(void);                                                              // CT3 0x01842380
int asCheckAnimationExecutable_Scene(int scene);                                         // CT3 0x018422e0
void asShadowDrawing_DetachLightPosition(int scene);                                     // CT3 0x01847460
void asShadowDrawing_Disable(int scene);                                                 // CT3 0x01847520
void Execute_ExecuteAnimationHistory(void);                                              // CT3 0x0184ec20
void asSeekBarAnimation(int scene);                                                      // CT3 0x01842b20
void asSearchObject(int scene);                                                          // CT3 0x01843950
// PORT: the bank is added in front: straw and partsHeader are the bank's EE words (BANK_PTR).
void asMakeStraw(int bank, u_int straw, u_int partsHeader);                              // CT3 0x018497d0
// PORT: (bank, anim, tbl, ...): anim is an EE word of the bank (ALL_TABLE.behavior from
// asUpDateSceneAnimation); the history records (Execute_ExecuteAnimationHistory) must carry the bank.
void ExecuteAnimation(int bank, u_int anim, ALL_TABLE *tbl, int a2, int a3, int a4, int a5, int a6); // CT3 0x0184a570
// out = a cubic Bezier point: ctrl[0] t^3 + ctrl[1] 3t^2 s + ctrl[2] 3ts^2 + ctrl[3] s^3 (s = 1 - t
// at the callers, asMakeStraw / tiLightBelt2Update). All four lanes.
void asGetBeziePosition(float t, float s, float out[4], const float ctrl[4][4]);         // CT3 0x0184a4b0
extern int iEASnum;                     /* 0x0193f780 */
extern void *lpEAS;                     /* 0x0193f77c */
extern int iEASidx;                     /* 0x0193f784 */
extern char iShadowTransMode;           /* 0x0193fa7c */
extern char iShadowTransModeFin;        /* 0x0193fa78 */
extern int iPleaseShadowDraw;           /* 0x0193fa74 */
extern char iShadowTransOrderBW;        /* 0x0193fa70 */

void asForceOperateAnimation_Scene(int scene, int mode);                                 // CT3 0x01842400
void asUpDateGAobjectsAnimation(void);                                                   // CT3 0x01842540
void asUpDateSceneAnimation(void);                                                       // CT3 0x018426e0
void asUpdateAnimation_Matrix(void);                                                     // CT3 0x018427f0
void asSeekBarFreeForX(int scene);                                                       // CT3 0x01842fe0
void asShadowPolygonPositions(int bank);                                                 // CT3 0x01844df0
void asGetShadowMatrixGlobalView(void);                                                  // CT3 0x01847330
void asShadowDrawing_Fin(int scene);                                                     // CT3 0x01847bd0
void Shadow_OnceInit(void);                                                              // CT3 0x01847c30
void ExecuteAnimationHistory_init(void);                                                 // CT3 0x0184ef00
void asCopyDoubleBuffer(void);                                                           // CT3 0x01851150
void ExecuteAnimationHistory_Free(void);                                                 // CT3 0x0184eed0
void BuildMatrixTree(int scene, int a1, int a2);                                         // CT3 0x0184fd30
// A CT3X file's header (a bank image; FSMltLoadProc reads it, ahRelocateFileObject relocates it).
#define CT3X_MAGIC 0x04                 /* u_int: the file kind */
#define CT3X_BASE 0x08                  /* u_int: the EE address the file's pointers are linked for */
#define CT3X_BHVR_LIST 0x40             /* BHVR: a 0-ended list of behaviour pointers */
#define CT3X_MAGIC_BHVR 0x52564842      /* "BHVR": a behaviour file */
#define CT3X_MAGIC_SHPE 0x45504853      /* "SHPE": a shape file (ALL_TABLE at BANK_ALL_TABLE_OFFSET) */
#define CT3X_MAGIC_ROOM 0x4d4f4f52      /* "ROOM": a scene file (the same) */

// PORT: the list / behaviour and both bases are EE addresses inside the image ahRelocateFileObject is
// relocating (its words stay EE addresses, open-items.md 1 h).
void ahRelocateSunPs2ObjectArry(u_int arry, u_int image, u_int base);                    // CT3 0x0183e280
void ahRelocateBehavior(u_int behavior, u_int image, u_int base);                        // CT3 0x0183e440
// FSMltLoadProc's call: (image, the file is a shape or behaviour file). PORT: the host image; the EE
// address its words are relocated to is BankImageEE[] of the bank holding it.
void ahRelocateFileObject(u_char *image, int persona);                                   // CT3 0x0183e710
void asDrawShadowVolumeLoop(void);                                                       // CT3 0x018443c0
void asBuildMirrorMatrixTreeBack(void *tree);                                            // CT3 0x0184f120
void asBuildMirrorMatrixTree(void *tree);                                                // CT3 0x0184f220
void asGAobjectControlPolygonDisplayRotateFlagReverse(ALL_TABLE *tbl);                   // CT3 0x0184f330
void asMirrorCulling(int on);                                                            // CT3 0x0184f390
int asMakeMirrorMatrixByPlane(float *planeVec, float *planePoint);                       // CT3 0x0184f910
void asMirrorPlaneCalc(float *planeVec, float *planePoint);                              // CT3 0x0184fb20
int asEnumMirror(ALL_TABLE *tbl);                                                        // CT3 0x0184fb40
void asInitMirrorMatrix(void);                                                           // CT3 0x0184fd00
void asBuildDOFMatrixTree(u_char *partsHeader);                                          // CT3 0x0184f020
// ---- seek bars (slice 3, 2026-11-10) ----
// ALL_TABLE.ctrl (bank data, layout: ee): +8 u_int OBJ_CTRL[], +0xe u_char their count.
typedef struct OBJ_CTRL {               /* 8 bytes, layout: ee */
    u_int ends;                         /* 0x0: float[2][4]: the bar's two ends in its part's space */
    short kind;                         /* 0x4: CTRL_KIND_* */
    u_short part;                       /* 0x6: the bar's part (MODEL_PARTS index of the bank) */
} OBJ_CTRL;
static_assert(sizeof(OBJ_CTRL) == 8, "OBJ_CTRL");
#define OBJ_CTRL_LIST 0x8               /* ALL_TABLE.ctrl: u_int OBJ_CTRL[] */
#define OBJ_CTRL_COUNT 0xe              /* ALL_TABLE.ctrl: u_char */
#define CTRL_KIND_SEEKBAR 2             /* asSeekBarInitializeForX's kind (others unread) */

// One frame-range animation a seek bar drives (12 bytes; asSeekBarAnimation).
typedef struct SEEKBAR_ANIM {
    u_short id;                         /* 0x0: asSceneAnimationStart's animation */
    short mode;                         /* 0x2: SEEKBAR_ANIM_* (which start flag) */
    int frameA;                         /* 0x4: the frame at t = 0 */
    int frameB;                         /* 0x8: the frame at t = 1 */
} SEEKBAR_ANIM;
static_assert(sizeof(SEEKBAR_ANIM) == 12, "SEEKBAR_ANIM");
#define SEEKBAR_ANIM_0 0                /* asSceneAnimationStart's 7th argument set */
#define SEEKBAR_ANIM_1 1                /* its 8th */
#define SEEKBAR_ANIM_2 2                /* its 9th */

// A seek bar: a line in a part's space; where a plane (a part's position) crosses it sets t in [0, 1],
// and t picks each animation's frame. 0x40 bytes, _SunAlloc'd (layout: host; EE offsets kept: the
// pointer at 0x30 fits the EE record's size). Who fills hash..anims is not read yet (init zeroes them).
typedef struct SEEKBAR {
    float ends[2][4];                   /* 0x00: from OBJ_CTRL.ends */
    u_long hash;                        /* 0x20: the plane part's hash in the character's bank */
    u_short planePart;                  /* 0x28: chara < 0: the plane is this part of the bar's bank */
    short chara;                        /* 0x2a: the character whose part is the plane; -1: the bank */
    u_short part;                       /* 0x2c: OBJ_CTRL.part */
    u_short animCount;                  /* 0x2e */
    SEEKBAR_ANIM *anims;                /* 0x30: _SunAlloc'd (asSeekBarFreeForX frees it); NULL: idle */
    u_char pad38[8];                    /* the EE record's 0x34..0x40 (host: after the 8-byte pointer) */
} SEEKBAR;
static_assert(offsetof(SEEKBAR, hash) == 0x20, "SEEKBAR.hash");
static_assert(offsetof(SEEKBAR, anims) == 0x30, "SEEKBAR.anims");
static_assert(sizeof(SEEKBAR) == 0x40, "SEEKBAR");
// layout: host (EE: 8 bytes, _SunAlloc'd; host-only memory, nothing else reads it).
typedef struct SEEKBAR_X {
    int count;
    SEEKBAR *bars;                      /* _SunAlloc'd count * 0x40, align 0x10 */
} SEEKBAR_X;
// PORT: ALL_TABLE.seekBarX on the host (a host pointer does not fit the EE word).
extern SEEKBAR_X *SeekBarX[BANK_MAX];

// ALL_TABLE.seat (layout: ee): {int count; u_int rows}, rows 0x10 bytes: +4 u_int cells, +0xe u_char
// their count; a cell is 0x10 bytes, bytes 2..3 its home position, copied to 4..5 and 6..7 at init.
#define SEAT_ROW_SIZE 0x10
#define SEAT_ROW_CELLS 0x4
#define SEAT_ROW_COUNT 0xe
#define SEAT_CELL_SIZE 0x10

// The model part records (pModelPartsHeader[bank] + 0x40 + i * 0x380: asFindModelParts' answer, the
// records BMT_01 walks). MODEL_PARTS (ct3_types.h) is indexed from the header, so its offsets are the
// record's + 0x40; fields past record 0x340 do not fit it: BMT_01 reads them as record offsets.
#define PARTS_RECORDS 0x40              /* the first record, from the header */
#define PARTS_REC(hdr, i) ((u_char *)(hdr) + PARTS_RECORDS + (i) * 0x380)
#define PARTS_FLAG_HIDDEN 0x1           /* MODEL_PARTS.flags: asSeekBarAnimation skips the bar's part (the
                                           same bit as PARTS_FLAG_FOG) */

u_long asSceneAnimationStart(float speed, int a0, ALL_TABLE *tbl, int a2, int frame, int id, int a6,
                             int a7, int a8, int a9, int a10, int a11, int a12, int a13); // CT3 0x018483d0
void asSeatArrange_ObjectMatchGlid(int row, int cell);                                   // CT3 0x018437c0
void asSeatArrange_FlameInit(int scene);                                                 // CT3 0x01843270
void BMT_01(u_char *partsHeader, int bank);                                              // CT3 0x01850fc0
void asSeekBarInitializeForX(int scene);                                                 // CT3 0x018430d0
void asSeatArrange_Init(int scene);                                                      // CT3 0x018436a0
void asShadowDrawing_Init(int scene);                                                    // CT3 0x01847b10
void asBehaviorStopAnimation(void *behavior);                                            // CT3 0x01847dc0
void asSceneAutoStartAnimation(void *behavior);                                          // CT3 0x01847fd0
void asForceOperateAnimation_Character(int chara, int mode);                              // CT3 0x018423c0 (main2.c)
