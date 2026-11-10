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
// The flags pick the pass (exactly one set: EA_100 / EA_010 / EA_001; any other mix runs the general
// body): `play` advances the pose tracks by this frame's ticks and records the call for the history,
// `replay` (Execute_ExecuteAnimationHistory) applies the shape tracks, `matOnly` builds `mat` from the
// EA_001_PART_HASH part only, `atKey` reads the keys where the tracks stand (no ticks). `mat` (NULL:
// none) takes base * local of the EA_001_PART_HASH part, its +0x3c word four done bytes.
// PORT: mat is a host pointer (the ROM's a2 word). Returns 1 when no track runs on.
int ExecuteAnimation(int bank, u_int anim, ALL_TABLE *tbl, float (*mat)[4], int play, int replay,
                     int matOnly, int atKey);                                            // CT3 0x0184a570
// ExecuteAnimation's pass for `play` alone (the per-frame scene animation). PORT: (bank, anim) as above.
int EA_100(int bank, u_int anim, ALL_TABLE *tbl, float (*mat)[4], int atKey);            // CT3 0x0184d1a0
// ... for `replay` alone. PORT: (bank, anim).
int EA_010(int bank, u_int anim, ALL_TABLE *tbl);                                        // CT3 0x0184bf60
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
// The two parts whose world matrix gives CHARACTER.refractAxis (asUpdateAnimation_Matrix: row 1 of the
// first, else row 2 of the second); their names are not known yet.
#define CHARA_AXIS_HASH_ROW1 0x61e97f0f3932cf98ull
#define CHARA_AXIS_HASH_ROW2 0x61cb2103161c34f0ull
void asUpdateAnimation_Matrix(void);                                                     // CT3 0x018427f0
void asSeekBarFreeForX(int scene);                                                       // CT3 0x01842fe0
void asShadowPolygonPositions(int bank);                                                 // CT3 0x01844df0
void asGetShadowMatrixGlobalView(void);                                                  // CT3 0x01847330
void Shadow_OnceInit(void);                                                              // CT3 0x01847c30
void ExecuteAnimationHistory_init(void);                                                 // CT3 0x0184ef00
void asCopyDoubleBuffer(void);                                                           // CT3 0x01851150
void asCopyDoubleBufferL2V(u_char *partsHeader, int mode);                              // CT3 0x018513b0
void asCopyDoubleBufferL2W(u_char *partsHeader);                                        // CT3 0x018510d0 (slice 9)
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
void asBuildMirrorMatrixTreeBack(u_char *partsHeader);                                   // CT3 0x0184f120
void asBuildMirrorMatrixTree(u_char *partsHeader);                                       // CT3 0x0184f220
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
#define SEEKBAR_ANIM_0 0                /* asSceneAnimationStart(.. pose ..): the pose and vertex tracks */
#define SEEKBAR_ANIM_1 1                /* (.. other ..): the number and vector tracks */
#define SEEKBAR_ANIM_2 2                /* (.. byPart ..): part 0 groups, copy and UV tracks of the part */

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

// asSceneAnimationStart's target (its 13th argument).
#define SCENE_ANIM_SCENE 1              /* the scene's behaviour tracks */
#define SCENE_ANIM_CHARACTER 2          /* the character's motion (asCharacterMortionChangeRequest*) */
#define CHARA_MOTION_TABLE 0x350        /* CHARACTER: the EE word of its motion table (0x30-byte records) */
#define CHARA_MOTION_SIZE 0x30
#define CHARA_MOTION_REQUEST_INDEX 3    /* asCharacterMortionChangeRequest's mode: by table index */
u_long asSceneAnimationStart(float speed, u_long hash, ALL_TABLE *tbl, int from, int to, int partId,
                             int pose, int other, int byPart, int started, int toEnd, int stepFromRuns,
                             int target, int chara);                                     // CT3 0x018483d0
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
    u_char playRate;                    /* 0x0e: percent of the tick step (IAK: ticks * playRate / 100; 0 jumps
                                           to the end); TRACK_PLAY_RATE_FULL at scene start,
                                           asChangeBehaviorAnimationPlayRate sets it */
    u_char flags;                       /* 0x0f: TRACK_FLAG_* */
    int playable;                       /* 0x10: group 0 track 0's: asGetBehaviorPlayableTicks; the
                                           end the time stops at (* TRACK_TIME_SCALE) */
    int rangeStart;                     /* 0x14: the play range's start (asChangePlayRange; frames): where a
                                           looping track goes on from */
    int time;                          /* 0x18: 0 from the top */
    int time1c;                         /* 0x1c: 0 from the top when asked */
} BEHAVIOR_TRACK;
static_assert(sizeof(BEHAVIOR_TRACK) == 0x20, "BEHAVIOR_TRACK");
static_assert(offsetof(BEHAVIOR_TRACK, playing) == 0x08, "BEHAVIOR_TRACK.playing");
static_assert(offsetof(BEHAVIOR_TRACK, step) == 0x0a, "BEHAVIOR_TRACK.step");
static_assert(offsetof(BEHAVIOR_TRACK, playRate) == 0x0e, "BEHAVIOR_TRACK.playRate");
static_assert(offsetof(BEHAVIOR_TRACK, time) == 0x18, "BEHAVIOR_TRACK.time");
typedef struct BEHAVIOR_GROUP {         /* 8 bytes, from BEHAVIOR +0x10 */
    short part;                         /* 0x0: the MODEL_PARTS index the group animates (EA_001) */
    short trackCount;                  /* 0x2 */
    u_int tracks;                       /* 0x4: BEHAVIOR_TRACK[trackCount] */
} BEHAVIOR_GROUP;
static_assert(sizeof(BEHAVIOR_GROUP) == 8, "BEHAVIOR_GROUP");
static_assert(offsetof(BEHAVIOR_GROUP, tracks) == 4, "BEHAVIOR_GROUP.tracks");
typedef struct BEHAVIOR {
    u_long hash;                        /* 0x00: its name's hash (asFindBehavior's key; an ld) */
    short groupCount;                   /* 0x08 */
    u_char pad0a[6];
    BEHAVIOR_GROUP groups[1];           /* 0x10: groupCount of them */
} BEHAVIOR;
static_assert(offsetof(BEHAVIOR, groupCount) == 0x08, "BEHAVIOR.groupCount");
static_assert(offsetof(BEHAVIOR, groups) == 0x10, "BEHAVIOR.groups");
#define TRACK_FLAG_STARTED 0x2          /* asSceneAutoStartAnimation sets it on an auto-start track */
#define TRACK_FLAG_AUTO_START 0x4       /* the track plays from the top at scene start */
#define TRACK_PLAY_RATE_FULL 100        /* BEHAVIOR_TRACK.playRate: normal speed (asSceneAutoStartAnimation sets it) */
#define TRACK_FLAG_STEP_FROM_RUNS 0x1   /* asStartAnimation: step starts at runs (+0xc), not 1 */
#define TRACK_PLAYING 1                 /* BEHAVIOR_TRACK.playing: running */
#define TRACK_ACT_END 3                 /* ... asBehaviorActEndAnimation's mark on a running track */
#define TRACK_TIME_SCALE 100            /* time counts playable * 100 at the end (asStartAnimation) */

// slice 7 (2026-11-10)
void asStartAnimation(BEHAVIOR_TRACK *track);                                            // CT3 0x01849750
void asBehaviorActEndAnimation(void *behavior);                                          // CT3 0x01847d50
int asGetBehaviorPlayingTicks(int bank, u_int anim);                                     // CT3 0x0184a530
// ExecuteAnimation's control record (its table +0x2c word): a mode byte at +0, ticks left at +0x20.
#define ANIM_CTRL_TICKS 0x20            /* int: asDecInterpolateTicks counts it down to 0 */
void asDecInterpolateTicks(u_char *ctrl, int ticks);                                     // CT3 0x0184a540
void asForceOperateMirroring_Character(int chara, int on);                               // CT3 0x018424d0 (asXcmd.c)
// Moves the track's time on by `dir` ticks (0: as it stands; IAK_0) and writes the key there into
// the track's key 0 (keys + 8...). 1 when the time reached the track's end (clamped there; modes 0
// and 1 of flags & 3), else 0 -- as EA_100 reads it (the decompilation of 0x0184dbc0, 2026-11-10).
// EA_001 sets its done byte when this is 0: open-items.md (dd).
int InterpolateAnimationKey(BEHAVIOR_TRACK *track, int dir);                             // CT3 0x0184dbc0
// slice 9 (2026-11-10): the keys. A track's keys are `length & TRACK_KEY_STRIDE` bytes each, key 0 the
// output and keys 1..runs the frames; a key's first int is its frame (time / TRACK_TIME_SCALE).
// flags & TRACK_FLAG_PLAY_MASK picks the play mode; bit 0 also plays backwards.
#define TRACK_KEY_STRIDE 0x1ffffff      /* BEHAVIOR_TRACK.length: a key's size */
#define TRACK_FLAG_REVERSE 0x1          /* = TRACK_FLAG_STEP_FROM_RUNS: the time runs down */
#define TRACK_FLAG_PLAY_MASK 0x3
#define TRACK_PLAY_ONCE 0               /* forward, stops at playable (1 answered) */
#define TRACK_PLAY_ONCE_REVERSE 1       /* backward, stops at playable */
#define TRACK_PLAY_LOOP 2               /* forward, past playable goes on from rangeStart */
#define TRACK_PLAY_LOOP_REVERSE 3       /* backward, below playable goes on from rangeStart */
#define TRACK_SLOW_STEP 500             /* playRate == anim_play_slow: the time snaps to 5-frame steps */
// The other kinds InterpolateAnimationKey writes (BEHAVIOR_TRACK.kind, the ROM's character constants;
// named from what it calls on them).
#define TRACK_KIND_NUMBER '"'           /* 0x22: one short at key + 8 */
#define TRACK_KIND_FLOAT1_1_14 '!'      /* 0x21 */
#define TRACK_KIND_VECTOR_XYZ ')'       /* 0x29: as TRACK_KIND_VECTOR */
#define TRACK_KIND_FLOAT0_7_9 '*'       /* 0x2a */
#define TRACK_KIND_UV '3'               /* 0x33 */
#define TRACK_KIND_COPY '4'             /* 0x34: the previous key copied whole, never blended */
#define TRACK_KIND_VERTEX '='           /* 0x3d: sub-records of packed positions (TRACK_KEY_RAW: float[4]) */
#define TRACK_KIND_NORMAL '>'           /* 0x3e: sub-records of packed normals */
#define TRACK_KIND_SUB_NUMBER 'H'       /* 0x48: sub-records with a short at +0x14 */
// The vertex kinds' key: sub-records of 0x18 bytes until the key's size is used up.
typedef struct ANIM_SUBKEY {
    int time;                           /* 0x00: the key's frame in the first one */
    int size;                           /* 0x04: the bytes of the key it takes up */
    u_int data;                         /* 0x08: EE address of the packed data (BankEePtr) */
    int count;                          /* 0x0c: the vertices (asContInterpolate*'s n) */
    int obj;                            /* 0x10: the objOpaque index of the ga object it writes (EA_010) */
    short number;                       /* 0x14: TRACK_KIND_SUB_NUMBER's value */
    short pad16;
} ANIM_SUBKEY;
static_assert(sizeof(ANIM_SUBKEY) == 0x18, "ANIM_SUBKEY");
// The key at the track's time: dir 0 of InterpolateAnimationKey (no time step); 1 at the end.
int IAK_0(BEHAVIOR_TRACK *track);                                                        // CT3 0x0184e520
// The play range: from >= 0 starts it (and the time) there; to < 0 opens the end (TRACK_RANGE_OPEN).
#define TRACK_RANGE_OPEN 0x7fffffff
void asChangePlayRange(BEHAVIOR_TRACK *track, int from, int to);                         // CT3 0x01849700
// time1c's two halves (last frame's and this frame's, EA_100) both set to frame (< 0: the newer one).
void asResetPassCheckarameter(BEHAVIOR_TRACK *track, int frame);                         // CT3 0x018496c0
// PORT: host pointers. The behaviour of a BHVR image (its CT3X_BHVR_LIST) with this hash, or NULL.
BEHAVIOR *asFindBehavior(u_char *image, u_long hash);                                    // CT3 0x0184ef10
// The first of a character's behaviour banks (CHARACTER.behaviorBank, loaded) that has it.
BEHAVIOR *asFindBehaviorByCharacter(int chara, u_long hash);                             // CT3 0x0184ef50
// Every track of the behaviour to frame (playable * 100 - 1 when frame is not below playable).
void asChangeBehaviorAnimationCurrent(BEHAVIOR *bhv, int frame);                         // CT3 0x01847c50
void asChangeBehaviorAnimationPlayRate(BEHAVIOR *bhv, u_char rate);                     // CT3 0x01847cf0
// The bank's shadow follows the part's light position (an EE address: PARTS_REC + SHADOW_LIGHT_POS).
#define SHADOW_LIGHT_POS 0xf0
void asShadowDrawing_AttachLightPosition(int bank, ALL_TABLE *tbl, int part);            // CT3 0x01847490
// The ROM takes the two floats in f12 / f13 and the bank in a0: the order of the C arguments is a guess.
void asShadowDrawing_SetShadowAngleLockParameter(int bank, float lock64, float lock68);  // CT3 0x01847550
void asShadowDrawing_SetShadowAngleLock(int bank, u_char on);                            // CT3 0x01847580
void asShadowDrawing_SetShadowValue(int bank, s_char value);                             // CT3 0x018475b0
void asShadowDrawing_SetShadowLevel(int bank, u_char level);                             // CT3 0x018475e0
#define SHADOW_TRANS_MODE_PLAIN 1       /* asShadowDrawing_SetTransMode: no pass split by shadowOrder
                                           (iShadowTransModeFin 2); any other value: the split pass */
void asShadowDrawing_SetTransMode(char mode);                                            // CT3 0x01847630
// Track kinds EA_001 reads (BEHAVIOR_TRACK.kind; named from that one use): a local rotation /
// position, and the base's the local one is multiplied into.
#define TRACK_KIND_ROT 0x0b             /* a packed quaternion: rows 0..2 of the local matrix */
#define TRACK_KIND_POS 0x0c             /* a packed position: row 3 of the local matrix */
#define TRACK_KIND_BASE_ROT 0x1f        /* the same for the base matrix */
#define TRACK_KIND_BASE_POS 0x20
#define TRACK_KEY_RAW 0x4000000         /* BEHAVIOR_TRACK.length: the key is a float[4] at keys + 0x10 */
#define TRACK_KEY_SHIFT(len) ((len) >> 28) /* ... else three shorts at keys + 8, scaled by 1 >> this */
#define TRACK_KIND_VECTOR 0x0d          /* a packed vector (XYZ1) to PARTS_ANIM_VECTOR (EA_100) */
#define EA_001_PART_HASH 0x74dda77d0cbcf0c2ull /* the one part EA_001 animates (MODEL_PARTS.hash) */
// Where EA_100 puts a pose track's key in the part (offsets from the part, as MODEL_PARTS): the local
// matrix (rows 0..2 from TRACK_KIND_ROT, row 3 from _POS), the base one (TRACK_KIND_BASE_ROT, _BASE_POS:
// MODEL_PARTS.drawOnPos) and TRACK_KIND_VECTOR's; each write sets PARTS_ANIM_DIRTY in the byte at 0x43.
#define PARTS_ANIM_ROT 0x280
#define PARTS_ANIM_POS 0x2b0
#define PARTS_ANIM_BASE_ROT 0x2c0
#define PARTS_ANIM_BASE_POS 0x2f0
#define PARTS_ANIM_VECTOR 0x300
#define PARTS_ANIM_FLAGS 0x43
#define PARTS_ANIM_DIRTY 0x1
// The parts header's mode byte (+0), as EA_100 reads it (names from that use): HOLD runs while any
// track plays (a track at its end starts again when TRACK_FLAG_STARTED); else it runs while a track
// has keys left. Either ends with asBehaviorActEndAnimation (HOLD, ONCE) and a return of 1.
#define ANIM_CTRL_HOLD 1
#define ANIM_CTRL_ONCE 2
int EA_001(void *behavior, ALL_TABLE *tbl, float (*out)[4]);                            // CT3 0x0184bc30

// A scene's mirrors (ALL_TABLE.mirror): asEnumMirror takes the first whose part is shown. layout: ee.
typedef struct MIRROR_LIST {            /* 8 bytes */
    u_int mirrors;                      /* 0x0: MIRROR[count], an EE address */
    int count;                          /* 0x4 */
} MIRROR_LIST;
typedef struct MIRROR {                 /* 0x50 bytes */
    int part;                           /* 0x00: the part (MODEL_PARTS index) whose matrix places it */
    u_char pad04[0xc];
    float p[4][4];                      /* 0x10: the mirror's four corners in the part's space (w 1);
                                           asEnumMirror's plane takes the first three, asMirrorCulling
                                           projects all four */
} MIRROR;
static_assert(sizeof(MIRROR_LIST) == 8, "MIRROR_LIST");
static_assert(sizeof(MIRROR) == 0x50, "MIRROR");
static_assert(offsetof(MIRROR, p) == 0x10, "MIRROR.p");
// PORT: host pointers (the ROM's words 0x0193cae0 / 0x0193cb00): the mirror asEnumMirror found and its
// part record (PARTS_REC); asMirrorCulling and the mirror tree read them.
extern MIRROR *mrr_pActvMirrorMO;       /* sbss 0x0193cae0 */
extern u_char *mrr_pActvMirrorMP;       /* sbss 0x0193cb00 */
extern float mrr_MirrorCullingRect[4];  /* sbss 0x0193cad0: asMirrorCulling's saved clip rectangle */
extern int mrr_MirrorClip_bk;           /* sbss 0x0193cac0: asMirrorCulling's saved Clipping_System */

void asStartAnimationFromTop(BEHAVIOR_TRACK *track, int resetTime);                      // CT3 0x018497b0
int asGetForceMatrixOperationFlag(void);                                                 // CT3 0x01851370
#define EV_FORCE_MATRIX_OPERATION 0x68  /* ssTrigCheckEvent: matrices forced, no shadow volumes */

// asShadowDrawing_CheckTransMode's modes, as asDrawShadowVolumeLoop calls it (named from that use).
#define SHADOW_TRANS_SORT 0             /* sort the shadowed banks; 0: start a pass */
#define SHADOW_TRANS_FIN 1              /* 0: the pass ends here (asShadowEnd) */
#define SHADOW_TRANS_NEXT 2             /* the next bank, SHADOW_TRANS_END after the last */
#define SHADOW_TRANS_END (-1)
int asShadowDrawing_CheckTransMode(int mode);                                            // CT3 0x01847660
void asDrawShadowVolumeInit(void);                                                       // CT3 0x018461f0
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

// ---- slice 12 (2026-11-10) ----
// The character's shadow settings copied from another character's bank (the triggers' copy).
void asCopyShadowInformation(int dstChara, int srcChara);                                // CT3 0x018442d0
// asShadowAllDisable's selection bits.
#define SHADOW_DISABLE_BANKS 0x1        /* every loaded BANK_TYPE_SHADOW bank that is drawn */
#define SHADOW_DISABLE_CHARACTERS 0x2   /* every CHARACTER_FLAG_REFRACTION | _SHADOW character's bank */
void asShadowAllDisable(u_int which);                                                    // CT3 0x01847360
// Every track of the behaviour starts (asStartAnimation / _FromTop); flags cleared, and the end and
// the range's start swapped when the end lies before it. `go` 0: nothing (the ROM's only test of it).
void asBehaviorStartAnimation(void *behavior, int go);                                   // CT3 0x01847e20
void asBehaviorStartAnimationFromTop(void *behavior, int go, int resetTime);             // CT3 0x01847ef0
#define TRACK_STOP_REQUEST 2            /* BEHAVIOR_TRACK.playing: asSceneAnimationStop_S's mark */
#define CHARA_MOTION_SLOTS 0x398        /* CHARACTER: short[CHARA_MOTION_SLOT_COUNT], motion table indices, -1 none */
#define CHARA_MOTION_SLOT_COUNT 6
// asSceneAnimationStart's counterpart (the same selection; target SCENE_ANIM_*): the selected running
// tracks get TRACK_STOP_REQUEST; a character's motion slot playing `hash` (hash 0: every slot) is
// stopped by asCharacterMortionStopRequest. 0 when something stopped, 1 otherwise.
u_char asSceneAnimationStop_S(u_long hash, ALL_TABLE *tbl, int partId, int pose, int other, int byPart,
                              int target, int chara);                                    // CT3 0x01848090
// The first selected running track's TRACK_FLAG_STEP_FROM_RUNS bit (1: it plays backwards from its
// end), -1 when none runs. No hash lookup: partId as given.
int asCheckAnimationDirection_X(ALL_TABLE *tbl, int partId, int pose, int other, int byPart); // CT3 0x018487d0

// ---- slice 13 (2026-11-10) ----
// CHARACTER's motion request queue (asCheckAnimationState_S reads it; the motion TU writes it): motion
// table indices until -1, and beside each its request mode.
#define CHARA_MOTION_QUEUE 0x3b0        /* CHARACTER: short[CHARA_MOTION_QUEUE_COUNT], -1 ends */
#define CHARA_MOTION_QUEUE_COUNT 42
#define CHARA_MOTION_QUEUE_MODE 0x404   /* CHARACTER: short per queue entry */
#define CHARA_MOTION_QUEUE_STOP 2       /* the mode that counts as not playing (0, 1, 3, 5 count as playing) */
// asCheckAnimationState_S's answer.
#define ANIM_STATE_NONE 0               /* nothing selected runs */
#define ANIM_STATE_PLAYING 1            /* a selected track is TRACK_PLAYING (a character: the motion plays) */
#define ANIM_STATE_STOPPING 2           /* none plays, one is TRACK_STOP_REQUEST */
// The state of what asSceneAnimationStop_S would stop (the same arguments; the triggers' door waits).
int asCheckAnimationState_S(u_long hash, ALL_TABLE *tbl, int partId, int pose, int other, int byPart,
                            int target, int chara);                                      // CT3 0x018489e0
// The first track of `kind` in the part's groups (asSceneAnimationStop_S's part rule; kinds '3' / '4'
// also part 0's groups when their first key names partId): (time / 100) * 2, + 1 when it runs; -1 none.
int asGetSceneAnimationCurrent(u_long hash, ALL_TABLE *tbl, int partId, int kind);      // CT3 0x01848db0
// asStockShadowVolume's modes (asShadowPolygonPositions calls it).
#define SV_STOCK_BEGIN 0                /* point the stock at the bank's shadow table buffers */
#define SV_STOCK_PLUS 1                 /* a plus quad (gMP; drawn subtracting) */
#define SV_STOCK_MINUS (-1)             /* a minus quad (gMM; drawn adding) */
#define SV_STOCK_FLUSH 2                /* the lone quads left become objects */
// The bank's shadow volume objects, built from quads of 4 position qwords.
void asStockShadowVolume(int bank, float (*quad)[4], int mode);                          // CT3 0x01844650

// ---- slice 14 (2026-11-10) ----
// asCheckAnimationFlamePassing_S's direction filter (its 5th argument: bit 0 forward, bit 1 backward).
#define PASS_DIR_FORWARD 1              /* tracks playing forward (TRACK_FLAG_REVERSE clear) */
#define PASS_DIR_REVERSE 2              /* tracks playing backward */
#define PASS_DIR_EITHER 3
#define PASS_FRAME_ANY (-10)            /* a character: does the motion play at all */
#define CHARA_MOTION_BEHAVIOR 8         /* motion table record: u_long hash of its behaviour */
// Whether a selected track's time passed `frame` this frame (by time1c's halves), 1 or 0.
int asCheckAnimationFlamePassing_S(u_long hash, ALL_TABLE *tbl, u_int frame, int partId, int dir, int target,
                                   int chara);                                           // CT3 0x01848fb0
