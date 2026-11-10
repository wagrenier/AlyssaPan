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
int asCheckAnimationExecutable_Character(int chara);                                      // CT3 0x01842290
void asShadowDrawing_Enable(int scene);                                                  // CT3 0x018474f0
void asShadowDrawing_SetTransOrderBW(char on);                                           // CT3 0x01847610
// One animation history record (lpEAS). The ROM's are 0x10 bytes {anim, tbl, sign, step}; PORT: host
// layout, with the bank whose EE words anim and tbl's fields are (open-items.md (ca)). Nothing writes
// them yet: the history adder's TU fills them, _SunAlloc'd from gHeapJunc.
typedef struct EAS_RECORD {             /* layout: host */
    u_int anim;                         /* EE word of the bank: ExecuteAnimation's animation */
    ALL_TABLE *tbl;                     /* the bank's table */
    int sign;                           /* AbsSignI32 -> parts header PARTS_HDR_ANIM_ABS / _SIGN */
    u_int step;                         /* -> parts header PARTS_HDR_ANIM_STEP while it plays */
    int bank;                           /* PORT */
} EAS_RECORD;
// The parts header's words Execute_ExecuteAnimationHistory sets around ExecuteAnimation (names from
// this use only).
#define PARTS_HDR_ANIM_ABS 0x20         /* int: |sign| */
#define PARTS_HDR_ANIM_SIGN 0x24        /* int: -1, 0, 1 */
#define PARTS_HDR_ANIM_STEP 0x28        /* u_int: the record's step, 0 after */
#define EAS_GROW 0x10                   /* SetExecuteAnimationHistory: records added at a time */
// PORT: the bank in front (anim is its EE word), as ExecuteAnimation.
int asGetBehaviorPlayableTicks(int bank, u_int anim);                                     // CT3 0x0184a520
// PORT: the bank in front; the ROM's (anim, tbl, a, b, step) store sign = a * b.
void SetExecuteAnimationHistory(int bank, u_int anim, ALL_TABLE *tbl, int a, int b, u_int step); // CT3 0x0184ed90
#define EAS_REC_PARENT 0x8c             /* a part record's parent word (REC_PARENT) */
// ALL_TABLE.ctrl's first list: the attached parts (0x20-byte entries; names from this use only).
#define OBJ_CTRL_ATTACH_LIST 0x0        /* u_int: the entries */
#define OBJ_CTRL_ATTACH_COUNT 0xc       /* u_char: their count */
#define OBJ_CTRL_ATTACH_SIZE 0x20
#define OBJ_CTRL_ATTACH_PARENT 0x8      /* u_int: the part's parent word, refreshed by an all-ones animation */
#define OBJ_CTRL_ATTACH_PART 0xe        /* u_short: the part (record index of the bank) */
extern int FrameCounter;                /* 0x0194494c: Execute_ExecuteAnimationHistory's calls */
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
void Shadow_OnceInit(void);                                                              // CT3 0x01847c30
void ExecuteAnimationHistory_init(void);                                                 // CT3 0x0184ef00
void asCopyDoubleBuffer(void);                                                           // CT3 0x01851150
void asCopyDoubleBufferL2V(u_char *partsHeader, int mode);                              // CT3 0x018513b0
void ExecuteAnimationHistory_Free(void);                                                 // CT3 0x0184eed0
// (scene, mode, pass): (0, 1) BMT_01, (2, 0) BMT_20; mode 1 rebuilds every part, pass != 0 the view
// matrices of what moved; a negative scene also keeps this frame's matrices as the previous ones.
void BuildMatrixTree(int scene, int mode, int pass);                                     // CT3 0x0184fd30
void BMT_20(u_char *partsHeader, int bank);                                              // CT3 0x01850770
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
// asSeatArrange_FlameInit's (names from this use only): a row's grid of bytes, a cell's places in it.
#define SEAT_ROW_GRID 0x0               /* u_int: the grid's bytes (row 0's is the one marked) */
#define SEAT_ROW_WIDTH 0xa              /* u_char: the grid's width (row 0's is used) */
#define SEAT_ROW_GRID_QWORDS 0xc        /* u_short: the grid's size in 16-byte units */
#define SEAT_CELL_X 0x4                 /* u_char: the cell's place (its home, bytes 2..3, at init) */
#define SEAT_CELL_Y 0x5
#define SEAT_CELL_X2 0x6                /* u_char: its second place */
#define SEAT_CELL_Y2 0x7
#define SEAT_CELL_PLACES 0xc            /* char: SEAT_CELL_TWO_PLACES marks the second place too */
#define SEAT_CELL_TWO_PLACES 1
#define SEAT_GRID_KEEP 0x3              /* the grid bits the frame keeps */
#define SEAT_GRID_TAKEN 0x4             /* a cell stands here */
#define SEAT_ROW_PART 0x8               /* u_short: the row's part (the grid's origin and axes) */
#define SEAT_CELL_PART 0x0              /* u_short: the cell's part (asSeatArrange_ObjectMatchGlid moves it) */
#define SEAT_SQUARE 25.0f               /* a grid square's size */

// ALL_TABLE.search (layout: ee): {int count; u_int SEARCH_OBJ[]}. A searching part (an eye, a lamp)
// follows the nearest visible target from its owner character's eye part; names from asSearchObject.
typedef struct SEARCH_OBJ {             /* 0x50 bytes, layout: ee */
    u_int parts;                        /* 0x00: short[]: target parts of this bank, -1 ends */
    u_int charas;                       /* 0x04: short[]: target characters (Character[]), -1 ends */
    u_int hashes;                       /* 0x08: u_long[]: each target character's part */
    u_short part;                       /* 0x0c: the part moved (this bank's) */
    u_short chara;                      /* 0x0e: the owner character */
    u_long eyeHash;                     /* 0x10: the owner's part the search starts from */
    u_long restHash;                    /* 0x18: the owner's part that sets the axis, and the goal
                                           when nothing is seen */
    u_short range;                      /* 0x20: how far it sees (0: any distance) */
    u_short halfAngle;                  /* 0x22: the cone about the axis (0: any direction) */
    u_short turn;                       /* 0x24: SEARCH_MOVE_STRAIGHT, else it turns about the eye */
    u_short turnStep;                   /* 0x26: the most it turns in a frame (an angle) */
    float speed;                        /* 0x28: SEARCH_MOVE_STRAIGHT: units per 5 ticks */
    u_int candidates;                   /* 0x2c: float[n][4] scratch: the targets this frame */
    float pos[4];                       /* 0x30: where the part is (copied to its MODEL_PARTS.pos) */
    float goal[4];                      /* 0x40: where it is going */
} SEARCH_OBJ;
static_assert(sizeof(SEARCH_OBJ) == 0x50, "SEARCH_OBJ");
#define SEARCH_MOVE_STRAIGHT 0
#define SEARCH_CANDIDATE_W 0x322bcc77   /* the w a candidate's position is stored with (1e-8f) */
#define SEARCH_IN_CONE 0x1
#define SEARCH_IN_RANGE 0x2
#define SEARCH_SEEN (SEARCH_IN_CONE | SEARCH_IN_RANGE)

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

// ---- slice 4 (2026-11-10) ----
// A behaviour (ALL_TABLE.behavior, BHVR entries; ahRelocateBehavior walks it). layout: ee -- bank data,
// the tracks word an EE address (BankEePtr). Names from asBehaviorStopAnimation /
// asSceneAutoStartAnimation / asStartAnimationFromTop; the rest are ahRelocateBehavior's.
typedef struct BEHAVIOR_TRACK {         /* 0x20 bytes */
    u_int keys;                         /* 0x00: the keys (BEHAVIOR_KEY_SIZE each) */
    u_int length;                       /* 0x04: the keys' total length ('=': low 25 bits) */
    u_char playing;                     /* 0x08: 1 from the top, 0 stopped */
    char kind;                          /* 0x09: BEHAVIOR_TRACK_* ('H', '>', '=') */
    short step;                         /* 0x0a: 1 from the top */
    u_short runs;                       /* 0x0c */
    u_char value0e;                     /* 0x0e: TRACK_VALUE0E_START at scene start */
    u_char flags;                       /* 0x0f: TRACK_FLAG_* */
    int playable;                       /* 0x10: group 0 track 0's: asGetBehaviorPlayableTicks */
    u_int pad14;
    int time;                          /* 0x18: 0 from the top */
    int time1c;                         /* 0x1c: 0 from the top when asked */
} BEHAVIOR_TRACK;
static_assert(sizeof(BEHAVIOR_TRACK) == 0x20, "BEHAVIOR_TRACK");
static_assert(offsetof(BEHAVIOR_TRACK, playing) == 0x08, "BEHAVIOR_TRACK.playing");
static_assert(offsetof(BEHAVIOR_TRACK, step) == 0x0a, "BEHAVIOR_TRACK.step");
static_assert(offsetof(BEHAVIOR_TRACK, value0e) == 0x0e, "BEHAVIOR_TRACK.value0e");
static_assert(offsetof(BEHAVIOR_TRACK, time) == 0x18, "BEHAVIOR_TRACK.time");
typedef struct BEHAVIOR_GROUP {         /* 8 bytes, from BEHAVIOR +0x10 */
    short pad0;
    short trackCount;                   /* 0x2 */
    u_int tracks;                       /* 0x4: BEHAVIOR_TRACK[trackCount] */
} BEHAVIOR_GROUP;
static_assert(sizeof(BEHAVIOR_GROUP) == 8, "BEHAVIOR_GROUP");
static_assert(offsetof(BEHAVIOR_GROUP, tracks) == 4, "BEHAVIOR_GROUP.tracks");
typedef struct BEHAVIOR {
    u_char pad00[8];
    short groupCount;                   /* 0x08 */
    u_char pad0a[6];
    BEHAVIOR_GROUP groups[1];           /* 0x10: groupCount of them */
} BEHAVIOR;
static_assert(offsetof(BEHAVIOR, groupCount) == 0x08, "BEHAVIOR.groupCount");
static_assert(offsetof(BEHAVIOR, groups) == 0x10, "BEHAVIOR.groups");
#define TRACK_FLAG_STARTED 0x2          /* asSceneAutoStartAnimation sets it on an auto-start track */
#define TRACK_FLAG_AUTO_START 0x4       /* the track plays from the top at scene start */
#define TRACK_VALUE0E_START 100         /* asSceneAutoStartAnimation's value for every track's +0xe */

void asStartAnimationFromTop(BEHAVIOR_TRACK *track, int resetTime);                      // CT3 0x018497b0
int asGetForceMatrixOperationFlag(void);                                                 // CT3 0x01851370
#define EV_FORCE_MATRIX_OPERATION 0x68  /* ssTrigCheckEvent: matrices forced, no shadow volumes */

// asShadowDrawing_CheckTransMode's modes, as asDrawShadowVolumeLoop calls it (named from that use).
#define SHADOW_TRANS_SORT 0             /* sort the shadowed banks; 0: start a pass */
#define SHADOW_TRANS_FIN 1              /* 0: the pass ends here (asShadowEnd) */
#define SHADOW_TRANS_NEXT 2             /* the next bank, SHADOW_TRANS_END after the last */
#define SHADOW_TRANS_END (-1)
int asShadowDrawing_CheckTransMode(int mode);                                            // CT3 0x01847660
void asDrawShadowVolumeInit(int a0);                                                     // CT3 0x018461f0
void asShadowStart(void);                                                                // CT3 0x018471c0
void asDrawShadowVolume(int bank);                                                       // CT3 0x018444b0
void asShadowEnd(int bank);                                                              // CT3 0x01846360
// scene_end: the shadow block back to gHeapJunc, shadow70 cleared.
void asShadowDrawing_Fin(int scene);                                                     // CT3 0x01847bd0
// PORT: BankInfo +0x58 is the bank's _SunAlloc'd shadow block; a host pointer does not fit the word.
extern void *ShadowBlock[BANK_MAX];
#define POLY_DISPLAY_ROTATE 0x1         /* a ga draw object's first byte: the winding a mirror flips */
extern float vPVbk[4];                  /* 0x01937740 */
extern float vPPbk[4];                  /* 0x01937750 */
