// game/ct3_types.h -- CT3's shared game structs whose owning TU is not written yet (game/main.c first,
// 2026-11-09). Only the fields read so far are named; each struct moves to its owner's header when that TU
// is written (Character_All_Init's game/Character_Init.c, fileio.c's bank table, ...).
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// One character slot of Character[35] (0x01a4bfc0, 0x580 bytes each: the symtab's 0xc080 / 35).
typedef struct CHARACTER {
    s_char id;                          /* 0x000: -1 = slot free */
    u_char pad001;
    short bank;                         /* 0x002: its bank (BankInfo index), < 0 = none */
    short behaviorBank[10];             /* 0x004: the banks of its behaviours ("%s.ANM"), -1 = none
                                           (TrgCharacterBehaviorSet / _Free; Character_All_Init: -1) */
    char shapeName[10];                 /* 0x018: its shape's file ("%s.BIN", TrgCharacterShapeSet) */
    char behaviorName[10][10];          /* 0x022: behaviorBank[i]'s file */
    u_char pad086[2];
    u_int flags;                        /* 0x088: CHARACTER_FLAG_* */
    u_int flagsPrev;                    /* 0x08c: flags as the last asCopyDoubleBuffer left them */
    u_long roomHash;                    /* 0x090: GetRoomName_file of the room it belongs to (CharID_TO_BankID
                                           finds the bank by it); Character_All_Init: 0 */
    u_long roomHashPrev;                /* 0x098: roomHash before the last warp (ExecuteWarpObject);
                                           Character_All_Init: 0 */
    short lightBank;                    /* 0x0a0: the bank whose light main_rtn applies, -1 = none */
    u_char pad0a2[0xe];
    qword posBefore;                   /* 0x0b0: last frame's position (main copies pos here first) */
    qword pos;                          /* 0x0c0 */
    u_char pad0d0[0x18];
    int rotBefore;                      /* 0x0e8: last frame's value of 0x0ec */
    int rot;                            /* 0x0ec */
    float targetDot;                    /* 0x0f0: facing (rot's Y-rotation row 2) . the target's vector
                                           (asCharacterOperate; CHARA_TARGET in wideXcmd_DebugPrint.h) */
    u_char targetState;                 /* 0x0f4: bit 0 = the target's CHARA_TARGET_FLAG_F4; 0 with no target */
    u_char pad0f5[7];
    u_int brain;                        /* 0x0fc: its AI brain (EE address; +0x24e: the brain's id that
                                           ssAI_BrainEnd takes). Character_All_Init: 0. The AI TU decides
                                           the host form (open-items.md 1 am) */
    short h100;                         /* 0x100: Character_All_Init: -1; a warp copies its marker part's
                                           short at +0x54 here (MODEL_PARTS.warpH54) */
    s_char b102;                        /* 0x102: Character_All_Init: -1 */
    u_char pad103[0x2f1 - 0x103];
    u_char b2f1;                        /* 0x2f1: Character_All_Init: 1 */
    u_char b2f2;                        /* 0x2f2: Character_All_Init: 0 */
    u_char pad2f3[0xd];
    int w300[4];                        /* 0x300: Character_All_Init: 0 */
    u_char behavior[0x4b0 - 0x310];     /* 0x310: asInitPlayerBehaviorControler's (0x310..0x488 at least) */
    qword refractAxis;                  /* 0x4b0: asUpdateAnimation_Matrix, for a refracting shadowed
                                           character: row 1 of part CHARA_AXIS_HASH_ROW1's world matrix,
                                           else row 2 of part CHARA_AXIS_HASH_ROW2's, else (0, 0, 0, 1) */
    u_char pad4c0[0x4e0 - 0x4c0];
    qword strikeFrom;                   /* 0x4e0: where a boss's scripted strike comes from (CHARA_BOSS_STRIKE,
                                           wideXcmd_DebugPrint.h; tiXcmd_CheckEarthquake: the source's pos at
                                           the quake's first frame) */
    u_char pad4f0[0x520 - 0x4f0];
    int keyTicks;                       /* 0x520: global ticks summed while the keys are enabled (asCharacterOperate) */
    int hp;                             /* 0x524: Character_All_Init: Char_Init_MAX_HP[i] */
    int hpMax;                          /* 0x528: Character_All_Init: Char_Init_MAX_HP[i] (which of the
                                           two is the maximum is a guess from the table's name) */
    u_char translucent;                 /* 0x52c: CHARA_TRANS_*; non-zero: AllDrawObject's last pass draws it */
    u_char pad52d[3];
    float translucentAlpha;             /* 0x530: tiChara_Translucence_End's first argument */
    u_char edgeLevel;                   /* 0x534: the edge-shading level (tiXcmd_SetCharacterEdgeShading,
                                           _CharaEdgeShadeAnim); Character_All_Init: 0 */
    u_char pad535;
    short h536[6];                      /* 0x536: Character_All_Init: 0; [4] / [5] CHARA_EDGE_PARAM_* */
    u_char pad542[0x54c - 0x542];
    u_int legParts[8];                  /* 0x54c: CHARA_LEG_*: EE addresses (BankEeAddr) of the shape bank's
                                           leg parts + CHARA_LEG_PART_OFFSET (TrgCharacterShapeSet) */
    u_char pad56c[4];
    struct CHARA_JUNC *junc;            /* 0x570: a block of gHeapJunc, freed with its buffer by
                                           Character_All_Init. layout: host (an EE word there) */
    u_char pad578[0x580 - 0x570 - sizeof(void *)];
} CHARACTER;
static_assert(sizeof(CHARACTER) == 0x580, "CHARACTER");
static_assert(offsetof(CHARACTER, flagsPrev) == 0x8c, "CHARACTER.flagsPrev");
static_assert(offsetof(CHARACTER, bank) == 0x002, "CHARACTER.bank");
static_assert(offsetof(CHARACTER, refractAxis) == 0x4b0, "CHARACTER.refractAxis");
static_assert(offsetof(CHARACTER, hp) == 0x524, "CHARACTER.hp");
static_assert(offsetof(CHARACTER, strikeFrom) == 0x4e0, "CHARACTER.strikeFrom");
static_assert(offsetof(CHARACTER, targetDot) == 0x0f0, "CHARACTER.targetDot");
static_assert(offsetof(CHARACTER, targetState) == 0x0f4, "CHARACTER.targetState");
static_assert(offsetof(CHARACTER, keyTicks) == 0x520, "CHARACTER.keyTicks");
static_assert(offsetof(CHARACTER, flags) == 0x088, "CHARACTER.flags");
static_assert(offsetof(CHARACTER, lightBank) == 0x0a0, "CHARACTER.lightBank");
static_assert(offsetof(CHARACTER, posBefore) == 0x0b0, "CHARACTER.posBefore");
static_assert(offsetof(CHARACTER, pos) == 0x0c0, "CHARACTER.pos");
static_assert(offsetof(CHARACTER, rotBefore) == 0x0e8, "CHARACTER.rotBefore");
static_assert(offsetof(CHARACTER, translucent) == 0x52c, "CHARACTER.translucent");
static_assert(offsetof(CHARACTER, translucentAlpha) == 0x530, "CHARACTER.translucentAlpha");
static_assert(offsetof(CHARACTER, behaviorName) == 0x022, "CHARACTER.behaviorName");
static_assert(offsetof(CHARACTER, roomHash) == 0x090, "CHARACTER.roomHash");
static_assert(offsetof(CHARACTER, brain) == 0x0fc, "CHARACTER.brain");
static_assert(offsetof(CHARACTER, shapeName) == 0x018, "CHARACTER.shapeName");
static_assert(offsetof(CHARACTER, legParts) == 0x54c, "CHARACTER.legParts");
static_assert(offsetof(CHARACTER, b2f1) == 0x2f1, "CHARACTER.b2f1");
static_assert(offsetof(CHARACTER, w300) == 0x300, "CHARACTER.w300");
static_assert(offsetof(CHARACTER, hp) == 0x524, "CHARACTER.hp");
static_assert(offsetof(CHARACTER, h536) == 0x536, "CHARACTER.h536");
static_assert(offsetof(CHARACTER, junc) == 0x570, "CHARACTER.junc");

// The gHeapJunc block at CHARACTER.junc: only its buffer pointer (EE +0xc) is read so far (Character_All_Init
// frees it, then the block). layout: host -- the EE's 0xc bytes before it are kept, the pointer is a host one.
typedef struct CHARA_JUNC {
    int poleCount;                      /* EE 0x00: tiXcmd_SetGhostPole's count of poles */
    float ghostRange;                   /* EE 0x04: tiXcmd_SetPointForGhostArea's range */
    u_int ghostPoint;                   /* EE 0x08: EE address (BankEeAddr) of the ghost area's part position */
    u_int *poles;                       /* EE 0x0c: a gHeapJunc block of poleCount EE addresses (BankEeAddr)
                                           of the ghost poles' part positions (tiXcmd_SetGhostPole) */
} CHARA_JUNC;

// CHARACTER.translucent (AllDrawObject): any bit keeps the character out of the opaque and mirror passes.
#define CHARA_TRANS_TRANSLUCENT 0x1     /* drawn last between tiChara_Translucence_Start / _End */
#define CHARA_EDGE_PARAM_A 4            /* CHARACTER.h536[]: the two edge-shading values tiXcmd_CharaEdgeShadeAnim */
#define CHARA_EDGE_PARAM_B 5            /* animates (their reader unread) */
#define CHARA_TRANS_GHOST 0x2           /* (without 0x1) drawn last by main_rtn_GHOST, _Start2 / _End2 */

// CharAmbChangeFlag[3] (0x0193fc00, 0x1c bytes each): a character's ambient colour change in progress
// (ssChangeCharAmbientColor's; a warp into a BANK_TYPE_SHADOW room ends the character's).
#define CHAR_AMB_CHANGE_MAX 3
typedef struct CHAR_AMB_CHANGE {
    u_short chara;                      /* 0x00: the character (0 = none: the player's also clears) */
    u_short h02, h04, h06;
    int w08, w0c, w10, w14;
    u_char pad18[4];
} CHAR_AMB_CHANGE;
static_assert(sizeof(CHAR_AMB_CHANGE) == 0x1c, "CHAR_AMB_CHANGE");

#define CHARACTER_MAX 35                /* 0x23: Character[], AI_flag[], DirectLightCharaCounter[] */
#define CHARACTER_FLAG_REFRACTION 0x80000000u /* DrawObjectRefraction: with _SHADOW, the character's bank
                                             is drawn in the refraction pass (name from that use) */
#define CHARACTER_FLAG_SHADOW 0x40000000u /* main: the character's bank casts a shadow polygon */

// One camera of Cam[7] (0x01ae7c40, 0x1c0 bytes each; indexed by asCheckBankID: scene banks 0..5 and
// the shared one). camera_init0 (asEyeForcusUP2CamMat.c) resets every field named here.
typedef struct CAMERA {
    float view[4][4];                   /* 0x000: the view matrix (asGetShadowMatrixGlobalView copies
                                           Cam[6]'s, the shared one, for the shadows) */
    float view2[4][4];                  /* 0x040: asBuildMirrorMatrixTree multiplies Cam[6]'s onto the
                                           reflected part matrix where the DOF tree has matrix_global_view
                                           (name from that one use) */
    float viewPrev[4][4];               /* 0x080: asCharacterMoveByPad steers by Cam[6]'s rows 0 / 2 of this
                                           one instead of view2 while iCamChangeFlag is set (the cut just
                                           changed): the previous cut's view, a guess from that one use */
    // 0x0c0..0x14c: eye, focus point and up, each live / from / to (camera_init0: eyes (10, 10, 10, 1),
    // focus points (0, 0, 0, 1), ups (0, 1, 0, 0)). A cut (CamChangeSub) copies the live ones into
    // *From and has asGetDestinationEYE / _TRG fill *To (asEyeForcusUP2CamMat.c, 2026-11-10).
    float eye[4];                       /* 0x0c0 */
    float eyeFrom[4];                   /* 0x0d0 */
    float eyeTo[4];                     /* 0x0e0 */
    float focus[4];                    /* 0x0f0: the point looked at (ssHQDOF_DrawAllObject's focusPos) */
    float focusFrom[4];                 /* 0x100 */
    float focusTo[4];                   /* 0x110 */
    float up[4];                        /* 0x120 */
    float upFrom[4];                    /* 0x130 */
    float upTo[4];                      /* 0x140 */
    float focal[4];                     /* 0x150: focal lengths (LEICA mm; 2.88 each at init): [0] live,
                                           [1] from, [2] to (CamChangeSub); asChangeViewMatrix_forTheBank
                                           turns [3] into screen_0's projection distance */
    int moveTicks;                      /* 0x160: CamChange_S's fourth argument: the cut's move in ticks
                                           (0 = at once; camera_view0's blend weight moveTime / moveTicks;
                                           0 at init) (was changeArg, 2026-11-10) */
    int moveTime;                       /* 0x164: ticks since the cut (camera_view0 adds the frame's) */
    int bank;                           /* 0x168: the bank CamChange_S last cut to (-1 at init), written and
                                           compared as an int; asCharacterMoveByPad stocks it as a short
                                           with cut (iCameraBank_Stock / iCameraID_Stock) */
    u_int cuts;                         /* 0x16c: EE address of bank's CAM_CUT records (its control table
                                           +0, CamChangeSub; BankEePtr(bank, ...)); 0 at init */
    u_int targets;                      /* 0x170: ... of its CAM_TRG records (control table +4) */
    u_int ctrlList;                     /* 0x174: ... of its OBJ_CTRL list (control table +8) */
    short cut;                        /* 0x178: the camera's cut, CamChange_S's second argument (a save
                                           keeps the live bank's; name from data_save_set); -1 at init */
    u_char eyeMode;                     /* 0x17a: the eye kind camera_view0 runs; 0xff (at init and on a
                                           cut change) = the cut's move, which asCamModeChange ends by
                                           taking eyeKind / trgKind here */
    u_char eyeKind;                     /* 0x17b: the cut's CAM_CUT_EYE_KIND (asGetDestinationEYE's kind) */
    u_char trgMode;                     /* 0x17c: the target kind beside eyeMode; camera_init0: 4 for a
                                           global bank (0..5), 0xff for a scene bank; 0xff on a cut change */
    u_char trgKind;                     /* 0x17d: asGetDestinationTRG's kind: 0 = the cut's target list,
                                           4 = none (the cut's CAM_CUT_HAS_TARGETS is 0) */
    u_char changeMode;                  /* 0x17e: CamChange_S's third argument */
    u_char focalMoving;                 /* 0x17f: 1 on a cut (CamChangeSub): focal[0] still blends from
                                           focal[1] to focal[2] after the move (camera_view0); 0 at init */
    u_char dofLevel;                    /* 0x180: the DOF level (CamGetNowDOFLevel reads Cam[6]'s); 4 at init */
    u_char moveSpeed;                   /* 0x181: the cut's CAM_CUT_MOVE_SPEED: asCameraPositionTranser's
                                           speed divisor (0 = jump); 0 at init */
    u_char b182;                        /* 0x182: 0 at init; 1 asks camera_view for one camera_view0 while
                                           the scene bank's animation is stopped */
    u_char pad183[0x3d];
} CAMERA;
static_assert(offsetof(CAMERA, eye) == 0x0c0, "CAMERA.eye");
static_assert(offsetof(CAMERA, up) == 0x120, "CAMERA.up");
static_assert(offsetof(CAMERA, focal) == 0x150, "CAMERA.focal");
static_assert(offsetof(CAMERA, moveTicks) == 0x160, "CAMERA.moveTicks");
static_assert(offsetof(CAMERA, cuts) == 0x16c, "CAMERA.cuts");
static_assert(offsetof(CAMERA, ctrlList) == 0x174, "CAMERA.ctrlList");
static_assert(offsetof(CAMERA, eyeKind) == 0x17b, "CAMERA.eyeKind");
static_assert(offsetof(CAMERA, trgKind) == 0x17d, "CAMERA.trgKind");
static_assert(offsetof(CAMERA, trgMode) == 0x17c, "CAMERA.trgMode");
static_assert(offsetof(CAMERA, changeMode) == 0x17e, "CAMERA.changeMode");
static_assert(offsetof(CAMERA, dofLevel) == 0x180, "CAMERA.dofLevel");
static_assert(offsetof(CAMERA, b182) == 0x182, "CAMERA.b182");
static_assert(offsetof(CAMERA, cut) == 0x178, "CAMERA.cut");
static_assert(sizeof(CAMERA) == 0x1c0, "CAMERA");
static_assert(offsetof(CAMERA, focus) == 0x0f0, "CAMERA.focus");
static_assert(offsetof(CAMERA, view2) == 0x040, "CAMERA.view2");
static_assert(offsetof(CAMERA, viewPrev) == 0x080, "CAMERA.viewPrev");
static_assert(offsetof(CAMERA, bank) == 0x168, "CAMERA.bank");

#define CAMERA_MAX 7                    /* Cam[]: 3136 bytes in the symtab */

// One bank of BankInfo[86] (0x01aea360, 0x90 bytes each; fileio.c owns it, fileio.h has its states).
// A file larger than one slot takes a group of banks: the head is BANK_STATE_LOADED, the loops over
// BankInfo skip the rest of the group with groupCount. (fileio.c slice 1, 2026-11-10: state was
// `count`, drawOn `loaded`.)
typedef struct BANK_INFO {
    int state;                          /* 0x00: BANK_STATE_* (fileio.h); a group's later banks: their place */
    int groupCount;                     /* 0x04: banks in the group (added to the index, minus one, to skip it) */
    char name[16];                      /* 0x08: the bank's file ("GAME.BIN", "MENU.BIN", ...) */
    int type;                           /* 0x18: BANK_TYPE_* = the FS_LOAD_* kind it was loaded as */
    u_int image;                        /* 0x1c: the EE address of its memory (host: BankImage[]) */
    int drawOn;                         /* 0x20: FSMltSetDrawBank's mode (DRAW_BANK_ON: drawn) */
    int handle;                         /* 0x24: the FSMltLoad handle that loaded it */
    u_int flags;                        /* 0x28: BANK_FLAG_* */
    u_char pad2c[4];
    // 0x30..0x70: the bank's shadow (ahRelocateSunPs2ObjectArry.c's asShadowDrawing_*; names from
    // asShadowDrawing_Init / _Disable / _CheckTransMode, 2026-11-10)
    float shadowVec[4];                 /* 0x30: (0, 0, 0, 1) at init; w (0x3c) is cleared by
                                           asShadowDrawing_Disable: the shadow's on word */
    float shadowAnchor[4];              /* 0x40: the shadow's anchor in the world (asShadowPolygonPositions);
                                           asShadowEnd projects it: the centre of its light fans */
    int shadowFrame;                    /* 0x50: the FrameCounter the volume was stocked on: drawn only then
                                           (asDrawShadowVolume) */
    u_int shadowLightPos;               /* 0x54: the attached light position (0: detached) */
    int shadow58;                       /* 0x58: the bank's _SunAlloc'd shadow block (asShadowDrawing_Fin frees it);
                                           0 at init. PORT: the host keeps it in ShadowBlock[bank] */
    float shadowLength;                 /* 0x5c: 500 at init (the ROM stores 1000, 250, then 500) */
    u_char shadowColor[4];              /* 0x60: RGBA, 0xff each; _Disable sets alpha 0xff */
    float shadow64;                     /* 0x64: 200 at init; asShadowDrawing_SetShadowAngleLockParameter */
    float shadow68;                     /* 0x68: 100 at init; the same setter */
    signed char shadowOrder;            /* 0x6c: 100 at init; _CheckTransMode sorts banks > 0 to the
                                           front of its order, <= 0 to the back */
    u_char shadow6d;                    /* 0x6d: 0 at init; asShadowDrawing_SetShadowAngleLock (the lock on) */
    u_char shadowDraw;                  /* 0x6e: non-0: _CheckTransMode takes the bank's shadow */
    u_char pad6f;
    u_char shadow70;                    /* 0x70: 0 at init */
    u_char pad71[0xf];
    int drawOnPrev;                     /* 0x80: drawOn as the last asCopyDoubleBuffer left it */
    u_char pad84[0xc];
} BANK_INFO;
static_assert(sizeof(BANK_INFO) == 0x90, "BANK_INFO");
static_assert(offsetof(BANK_INFO, drawOnPrev) == 0x80, "BANK_INFO.drawOnPrev");
static_assert(offsetof(BANK_INFO, name) == 0x08, "BANK_INFO.name");
static_assert(offsetof(BANK_INFO, type) == 0x18, "BANK_INFO.type");
static_assert(offsetof(BANK_INFO, image) == 0x1c, "BANK_INFO.image");
static_assert(offsetof(BANK_INFO, drawOn) == 0x20, "BANK_INFO.drawOn");
static_assert(offsetof(BANK_INFO, handle) == 0x24, "BANK_INFO.handle");
static_assert(offsetof(BANK_INFO, flags) == 0x28, "BANK_INFO.flags");
static_assert(offsetof(BANK_INFO, shadowVec) == 0x30, "BANK_INFO.shadowVec");
static_assert(offsetof(BANK_INFO, shadowLightPos) == 0x54, "BANK_INFO.shadowLightPos");
static_assert(offsetof(BANK_INFO, shadowFrame) == 0x50, "BANK_INFO.shadowFrame");
static_assert(offsetof(BANK_INFO, shadowLength) == 0x5c, "BANK_INFO.shadowLength");
static_assert(offsetof(BANK_INFO, shadowColor) == 0x60, "BANK_INFO.shadowColor");
static_assert(offsetof(BANK_INFO, shadowOrder) == 0x6c, "BANK_INFO.shadowOrder");
static_assert(offsetof(BANK_INFO, shadowDraw) == 0x6e, "BANK_INFO.shadowDraw");
static_assert(offsetof(BANK_INFO, shadow70) == 0x70, "BANK_INFO.shadow70");

#define BANK_MAX 86                     /* 0x56: BankInfo[], the per-scene tables (FogCtrlAddr, ...) */
#define BANK_TYPE_SHADOW 1              /* main: banks of this type cast shadow polygons; scene_init:
                                           their fog colour clears the frame. fileio.c (2026-11-10): a
                                           bank's type is the FS_LOAD_* kind it was loaded as, so 1 is
                                           FS_LOAD_SCENE, the scene bank (rename when its users are next
                                           touched) */
#define BANK_TYPE_0 0                   /* scene_init: takes the table's fog record, like type 1 */
#define BANK_SCENE_MAX 6                /* asCheckBankID: banks 0..5 have their own view matrix, the
                                           rest share bank 6's; GlobalDrawObject draws banks 0..5 */
#define BANK_FLAG_DRAW_FIRST 0x2        /* scene_draw_init: ARRW.BIN, "E_*" and "M<digit>*" banks;
                                           GlobalDrawObject draws them before the other banks
                                           (AllDrawObject's "trans" pass draws them after the rest) */
// asForceOperateAnimation_Scene's bits (2026-11-10): a forced stop / run of the bank's animation.
#define BANK_FLAG_ANIM_FORCE_STOP 0x4   /* asCheckAnimationExecutable_Scene: still while the world moves */
#define BANK_FLAG_ANIM_FORCE_RUN 0x8    /* ... runs while the world is paused */
#define BANK_FLAG_ANIM_STOPPED 0x20     /* set with a forced stop, cleared by a run or a release */
#define BANK_FLAG_NO_MIRROR 0x10       /* AllDrawObject: a character of this bank is not mirrored
                                           (name from that use) */

// A bank's light table (ALL_TABLE.light; Character_Init.c's SceneLightSet / CharLightSet, main_rtn).
// layout: ee -- bank data; the u_int words are relocated EE addresses (BankEePtr, open-items.md 1 h).
// Light sources are model parts of the bank (MODEL_PARTS.lightPos / lightColor).
typedef struct SCENE_LIGHT {
    u_int pairs;                        /* 0x00: short [part, set] pairs, ended by part LIGHT_PAIRS_END:
                                           the lit part and its LIGHT_SET (LIGHT_NONE: unlit) */
    u_int sets;                         /* 0x04: LIGHT_SET[] */
    float charaScale;                   /* 0x08: main_rtn: scales a character's TEX_TFX_HIGHLIGHT2 parts */
    u_int charaDir;                     /* 0x0c: short[LIGHT_SLOTS]: the directional light parts of a
                                           character lit by this bank (CharLightSet) */
    u_int omniList;                     /* 0x10: LIGHT_LIST: the point lights a character lit by this
                                           bank picks its nearest four from (CharLightSet) */
} SCENE_LIGHT;
static_assert(offsetof(SCENE_LIGHT, charaScale) == 0x08, "SCENE_LIGHT.charaScale");
static_assert(offsetof(SCENE_LIGHT, omniList) == 0x10, "SCENE_LIGHT.omniList");

// One light set of SCENE_LIGHT.sets (0x14 bytes): the parts lighting a scene part. layout: ee.
typedef struct LIGHT_SET {
    short dir[4];                       /* 0x00: directional light parts, LIGHT_NONE = slot off */
    short omni[4];                      /* 0x08: point light parts, LIGHT_NONE = off, LIGHT_OMNI_NEAREST =
                                           the next nearest of omniList (SceneLightSet) */
    u_int omniList;                     /* 0x10: LIGHT_LIST, LIGHT_LIST_NONE: omni[] are all parts */
} LIGHT_SET;
static_assert(sizeof(LIGHT_SET) == 0x14, "LIGHT_SET");
static_assert(offsetof(LIGHT_SET, omniList) == 0x10, "LIGHT_SET.omniList");
// A LIGHT_LIST is a short count followed by that many part indices.

#define LIGHT_SLOTS 4                   /* directional and point lights per lit part (gaSetLight) */
#define LIGHT_NONE (-1)                 /* a part / set index: none */
#define LIGHT_PAIRS_END (-2)            /* SCENE_LIGHT.pairs: the end */
#define LIGHT_OMNI_NEAREST (-2)         /* LIGHT_SET.omni: the next of gaSearchNearLight4's answer */
#define LIGHT_LIST_NONE 0xffffffffu     /* LIGHT_SET.omniList: no list */
#define NEAR_LIGHT_NONE 0xff            /* gaSearchNearLight4's answer: no light for this slot */
#define LIGHT_ACTRESS_FALLOFF 300.0f    /* CharLightSet, actress lighting: w' = 1 / (1 / w + 300) */

// The light block gaSetLight reads (MODEL_PARTS.attr's EE address: the part's draw object attribute).
// layout: ee. The fields SceneLightSet / CharLightSet write, and what gaSetLight builds from them
// (gaInit3dlib.c, 2026-11-10); +0x00..0x0f unread so far.
typedef struct GA_LIGHT_WORK {
    u_char pad000[0x10];
    float dirView[4][LIGHT_SLOTS];      /* 0x010: gaSetLight: dirPos[] under the view's rotation, one
                                           column per light, rows x, y, z, -w */
    float omniView[4][LIGHT_SLOTS];     /* 0x050: gaSetLight: omniPos[].xyz under the view, one column
                                           per light, rows x, y, z, then omniPos[].w unchanged */
    float dirColor[LIGHT_SLOTS][4];     /* 0x090: from MODEL_PARTS.lightColor */
    float omniColor[LIGHT_SLOTS][4];    /* 0x0d0 */
    float dirPos[LIGHT_SLOTS][4];       /* 0x110: from MODEL_PARTS.lightPos */
    float omniPos[LIGHT_SLOTS][4];      /* 0x150: off = (0, 0, 0, 1) */
} GA_LIGHT_WORK;
static_assert(offsetof(GA_LIGHT_WORK, dirView) == 0x010, "GA_LIGHT_WORK.dirView");
static_assert(offsetof(GA_LIGHT_WORK, omniView) == 0x050, "GA_LIGHT_WORK.omniView");
static_assert(offsetof(GA_LIGHT_WORK, dirColor) == 0x090, "GA_LIGHT_WORK.dirColor");
static_assert(offsetof(GA_LIGHT_WORK, omniColor) == 0x0d0, "GA_LIGHT_WORK.omniColor");
static_assert(offsetof(GA_LIGHT_WORK, dirPos) == 0x110, "GA_LIGHT_WORK.dirPos");
static_assert(offsetof(GA_LIGHT_WORK, omniPos) == 0x150, "GA_LIGHT_WORK.omniPos");

// A loaded scene's object tables (All_table_TBL[86], a record of the scene's bank): each list is a
// NULL-terminated array of EE words, each the address of a draw list entry. Only the fields the
// written TUs read; fileio.c owns the layout. layout: ee (2026-11-10, open-items.md item 1 bv): the
// table is the bank image + 0x80, so every pointer word is a u_int EE address the relocator
// (ahRelocateFileObject) moved; read it with BANK_PTR(type, bank, word). Only +0x14, +0x38 and +0x3c
// are counts. The other words (0x20..0x24, 0x30..0x50, 0x5c, 0x64..0x6c, 0x74, 0x80..0xb4) are lists
// no written TU reads yet: named by their readers.
typedef struct ALL_TABLE {
    u_char pad00[0x10];
    u_int partsPos;                     /* 0x10: float[parts][4]; main_rtn draws a part only while
                                           its w > PARTS_POS_HIDDEN_W; main_rtn_Particle2 starts from it */
    int baseobjCount;                   /* 0x14: scene_init -> BaseobjCount[bank] */
    u_int objOpaque;                    /* 0x18: list: GlobalDrawObject's first main_rtn list */
    u_int objTrans;                     /* 0x1c: list: main_rtn mode 1; scene_draw_init sorts it */
    u_int hits;                         /* 0x20: HIT_TABLE, what touched the bank's character this frame
                                           (asCheckCharacterHited reads Alyssa's) */
    u_int detectors;                    /* 0x24: list of TRIG_DETECTOR (ssGrpPortalCheck.h): the bank's
                                           trigger detectors and room portals */
    u_int light;                        /* 0x28: SCENE_LIGHT, the bank's light table (charaScale) */
    u_int partsHeader;                  /* 0x2c: the MODEL_PARTS header (scene_init -> pModelPartsHeader;
                                           InitMatrixTree, asBuildMirrorMatrixTree / _Back) */
    u_int copyWords;                    /* 0x30: u_int[count38]: the words TRACK_KIND_COPY keys pick (EA_010) */
    u_int pad34;
    int count38;                        /* 0x38: the +0x30 list's length (the relocator's bound) */
    int count3c;                        /* 0x3c: the +0x34 list's length */
    u_int objGroups;                    /* 0x40: (short first, short count) pairs into objGroupIndex (EA_010's
                                           TRACK_KIND_UV / _COPY keys name one) */
    u_int objGroupIndex;                /* 0x44: short[]: indices into the objOpaque list */
    u_int pad48;
    u_int trigger;                      /* 0x4c: TRIG_DATA (ssGrpPortalCheck.h): the trigger script's
                                           three blocks (Trigger_init) */
    u_int straw;                       /* 0x50: asMakeStraw's first argument (with partsHeader) */
    u_int sound;                       /* 0x54: SOUND_OBJ_TABLE (sound.c's SSFindSoundObj) */
    u_int particles;                    /* 0x58: list of PARTICLE_ENTRY: main_rtn_Particle's */
    u_int pad5c;
    u_int objAfter;                     /* 0x60: list: after the particles; non-empty -> ExportDrawToTex */
    u_int pad64[2];
    u_int shadow;                       /* 0x6c: the shadow table (relocated: 7 words, then 8 key tables);
                                           its word 0 is the shadow volume's ga draw objects, 0x80 apart,
                                           ended by a 0 kind byte (asDrawShadowVolume) */
    u_int partsLight;                   /* 0x70: float[parts]: TEX_TFX_HIGHLIGHT2 parts' DRAW_OBJ_LIGHT */
    u_int losFade;                      /* 0x74: LOS_FADE[] (LosHitFadeUpdate), 0 = none */
    u_int particles2;                   /* 0x78: list of PARTICLE2_ENTRY: main_rtn_Particle2 mode 0 */
    u_int particles2After;              /* 0x7c: the same, mode 1; non-empty -> ExportDrawToTex */
    u_int distAtten;                    /* 0x80: DIST_ATTEN[] (DistanceAttenuationUpdate), 0 = none */
    u_int angleAtten;                   /* 0x84: ANGLE_ATTEN[] (AngleAttenuationUpdate), 0 = none */
    u_int pad88;
    u_int texFileSet;                   /* 0x8c: TEX_FILE_SET_TABLE (QuatSlerp.h: the paged textures), 0 = none */
    u_int pad90[10];
    u_int fog;                         /* 0xb8: FOG_INFO (scene_init -> FogCtrlAddr, bank types 0 / 1) */
    u_int behavior;                     /* 0xbc: scene_init -> pBehaviorHeader[bank]; the relocator reads
                                           words to 0xe8: the table's end is not known */
    u_int ctrl;                         /* 0xc0: OBJ_CTRL_TABLE: the scene's controlled parts
                                           (asSeekBarInitializeForX takes the CTRL_KIND_SEEKBAR ones);
                                           also its cameras: +0 the CAM_CUT records, +4 the CAM_TRG
                                           records, +0xc (u_char) the cut count (CamChangeSub) */
    u_int padc4[2];
    u_int mirror;                       /* 0xcc: MIRROR_LIST (asEnumMirror), 0 = no mirror */
    u_int padd0[2];
    u_int search;                      /* 0xd8: SEARCH_TABLE (asSearchObject), 0 = none */
    u_int seat;                         /* 0xdc: SEAT_TABLE (asSeatArrange_Init), 0 = none */
    u_int groupPortals;                 /* 0xe0: list of GRP_PORTAL (ssGrpPortalCheck.h) */
    u_int pade4;
    u_int seekBarX;                     /* 0xe8: the relocator clears it; the ROM keeps the _SunAlloc'd
                                           SEEKBAR_X here. PORT: a host pointer does not fit the EE word,
                                           so the host keeps it in SeekBarX[bank] (this word stays 0) */
} ALL_TABLE;
static_assert(offsetof(ALL_TABLE, ctrl) == 0xc0, "ALL_TABLE.ctrl");
static_assert(offsetof(ALL_TABLE, mirror) == 0xcc, "ALL_TABLE.mirror");
static_assert(offsetof(ALL_TABLE, search) == 0xd8, "ALL_TABLE.search");
static_assert(offsetof(ALL_TABLE, seat) == 0xdc, "ALL_TABLE.seat");
static_assert(offsetof(ALL_TABLE, seekBarX) == 0xe8, "ALL_TABLE.seekBarX");
static_assert(offsetof(ALL_TABLE, partsPos) == 0x10, "ALL_TABLE.partsPos");
static_assert(offsetof(ALL_TABLE, light) == 0x28, "ALL_TABLE.light");
static_assert(offsetof(ALL_TABLE, count38) == 0x38, "ALL_TABLE.count38");
static_assert(offsetof(ALL_TABLE, sound) == 0x54, "ALL_TABLE.sound");
static_assert(offsetof(ALL_TABLE, objAfter) == 0x60, "ALL_TABLE.objAfter");
static_assert(offsetof(ALL_TABLE, partsLight) == 0x70, "ALL_TABLE.partsLight");
static_assert(offsetof(ALL_TABLE, particles2After) == 0x7c, "ALL_TABLE.particles2After");
static_assert(offsetof(ALL_TABLE, fog) == 0xb8, "ALL_TABLE.fog");

// A bank's sound objects (ALL_TABLE.sound): the sources the trigger actions and room-in start play.
// layout: ee -- bank data; loadSE and objs are relocated words (BankEePtr, open-items.md item 1 h).
typedef struct SOUND_OBJ_TABLE {
    u_int loadSE;                       /* 0x00: EE address of SE_LOAD_POINT[SE_LOAD_POINT_MAX], the SE
                                           banks SSLoadSEfromLoadPoint loads with the bank */
    u_int objs;                         /* 0x04: EE address of SOUND_OBJ[], ended by id 0 */
} SOUND_OBJ_TABLE;
// One row of a bank's SE load list (SOUND_OBJ_TABLE.loadSE). layout: ee.
typedef struct SE_LOAD_POINT {          /* 0x0c bytes */
    u_int name;                         /* 0x00: EE address of the SE bank's file name; 0 ends the list */
    u_char pad04[8];                    /* 0x04: not read by sound.c */
} SE_LOAD_POINT;
static_assert(sizeof(SE_LOAD_POINT) == 0xc, "SE_LOAD_POINT");
#define SE_LOAD_POINT_MAX 5             /* SSLoadSEfromLoadPoint's row bound */
#define BANK_ALL_TABLE_OFFSET 0x80      /* FSMltLoadProc: All_table_TBL[bank] = bank image + 0x80 */
static_assert(sizeof(SOUND_OBJ_TABLE) == 0x8, "SOUND_OBJ_TABLE");
static_assert(offsetof(SOUND_OBJ_TABLE, objs) == 0x4, "SOUND_OBJ_TABLE.objs");
typedef struct SOUND_OBJ {              /* 0x28 bytes */
    int id;                             /* 0x00: the trigger tables' sound object id; 0 ends the list */
    int seNo;                           /* 0x04: the C_0000 SE it plays */
    int autoStart;                      /* 0x08: != 0: plays when the room is entered */
    u_char pad0c[0x18];
    int positional;                     /* 0x24: != 0: SetPositionalParam_Alys pans it every frame */
} SOUND_OBJ;
static_assert(sizeof(SOUND_OBJ) == 0x28, "SOUND_OBJ");
static_assert(offsetof(SOUND_OBJ, positional) == 0x24, "SOUND_OBJ.positional");
#define SOUND_OBJ_MAX 1000              /* SSAutoStartSoundRoomIn's scan bound */

#define PARTS_POS_HIDDEN_W -255.0f      /* ALL_TABLE.partsPos[i][3] at or below: the part is not drawn */

// One model part of a bank's parts header (pModelPartsHeader[bank], 0x380 bytes each). main_rtn reads
// two fields; the rest belongs to the TU that loads the header.
typedef struct MODEL_PARTS {
    u_char pad000[0x42];
    u_char kind;                        /* 0x42: PARTS_KIND_NO_DOF: main_rtn_DOF skips the part */
    u_char flags43;                     /* 0x43: PARTS_FLAG43_* */
    int attr;                           /* 0x44: copied into the part's draw object (+0x3c) every draw:
                                           the EE address of its GA_LIGHT_WORK (BankEePtr) */
    u_long hash;                        /* 0x48: the part's name hash (ahGetHash); TrgSceneDraw joints a
                                           bank to a PARTS_KIND_JOINT_PORTAL part by it */
    u_int flags;                        /* 0x50: PARTS_FLAG_* */
    short warpH54;                      /* 0x54: a warp onto this part (a marker) copies it to
                                           CHARACTER.h100 (ExecuteWarpObject) */
    u_char pad056[0x6a];
    float initRot[3][4];                /* 0xc0: rows 0..2 of the part's initial placement, transInit its row 3
                                           (tiXcmd_SetMapData copies the four rows into placeRot) */
    float transInit[4];                 /* 0xf0: the part's initial translation (asGetObjectTransInitialValue) */
    u_char pad100[0x20];
    float alpha;                        /* 0x120: the part's alpha: ALL_TABLE.partsPos w at AmbientAlphaInit */
    float losAlpha;                     /* 0x124: LosHitFadeUpdate's fade */
    float distAlpha;                    /* 0x128: DistanceAttenuationAlphaSet's: minus the fade, -max..0 */
    float angleAlpha;                   /* 0x12c: AngleAttenuationAlphaSet's: minus the fade, -max..0 */
    float pos[4];                      /* 0x130: the part's position (SSGetPositional: a sound source) */
    u_char pad140[0x80];
    float matrix[4][4];                 /* 0x1c0: the part's world matrix (SelfDiscardSceneLoad; ActressMP's) */
    u_char pad200[0xc0];
    float placeRot[3][4];               /* 0x2c0: rows 0..2 of the part's placement matrix, drawOnPos its
                                           row 3 (asObjectRotate sets or turns them) */
    float drawOnPos[4];                 /* 0x2f0: part 0's: where the bank was placed (a save keeps it;
                                           TrgSceneDraw(.., -2, ..) places it at DrawOnPosSetVectorTemp) */
    float scale[4];                     /* 0x300: the part's scale (LinkMatrixTree's REC_SCALE); tiXcmd_SetScale
                                           multiplies x, y, z by a factor, w kept */
    float lightColor[4];              /* 0x310: as a light source (SceneLightSet, CharLightSet) */
    float lightPos[4];                  /* 0x320: as a light source; w: falloff (1 = Omni0/1_num) */
    u_char pad330[0x50];
} MODEL_PARTS;
static_assert(offsetof(MODEL_PARTS, alpha) == 0x120, "MODEL_PARTS.alpha");
static_assert(offsetof(MODEL_PARTS, transInit) == 0x0f0, "MODEL_PARTS.transInit");
static_assert(offsetof(MODEL_PARTS, warpH54) == 0x54, "MODEL_PARTS.warpH54");
static_assert(offsetof(MODEL_PARTS, losAlpha) == 0x124, "MODEL_PARTS.losAlpha");
static_assert(offsetof(MODEL_PARTS, distAlpha) == 0x128, "MODEL_PARTS.distAlpha");
static_assert(offsetof(MODEL_PARTS, angleAlpha) == 0x12c, "MODEL_PARTS.angleAlpha");
static_assert(offsetof(MODEL_PARTS, scale) == 0x300, "MODEL_PARTS.scale");
static_assert(offsetof(MODEL_PARTS, pos) == 0x130, "MODEL_PARTS.pos");
static_assert(offsetof(MODEL_PARTS, lightColor) == 0x310, "MODEL_PARTS.lightColor");
static_assert(offsetof(MODEL_PARTS, lightPos) == 0x320, "MODEL_PARTS.lightPos");
static_assert(offsetof(MODEL_PARTS, placeRot) == 0x2c0, "MODEL_PARTS.placeRot");
static_assert(offsetof(MODEL_PARTS, drawOnPos) == 0x2f0, "MODEL_PARTS.drawOnPos");
static_assert(sizeof(MODEL_PARTS) == 0x380, "MODEL_PARTS");
static_assert(offsetof(MODEL_PARTS, hash) == 0x48, "MODEL_PARTS.hash");
static_assert(offsetof(MODEL_PARTS, flags43) == 0x43, "MODEL_PARTS.flags43");
#define PARTS_FLAG43_WARPED 0x1         /* asWarpSceneObject placed the part (drawOnPos); its reader unread */
#define PARTS_FLAG43_TRANS 0x2          /* asSetObjectTrans sets it with WARPED (3): its reader unread */
static_assert(offsetof(MODEL_PARTS, matrix) == 0x1c0, "MODEL_PARTS.matrix");
static_assert(offsetof(MODEL_PARTS, initRot) == 0x0c0, "MODEL_PARTS.initRot");

// PanicInfoData (0x01940e50, 48 bytes): the player's panic state a save keeps whole. Fields by offset
// (data_save_set / data_load_set copy them one by one; the load then ends any recovery: item -1, time 0).
// PanicUpdate (2026-11-10) named the rest.
#define PANIC_RISE_COUNT 8
#define PANIC_RISE_THREAT 0             /* 0..2: the first non-zero one counts (times the hiding factor) */
#define PANIC_RISE_THREAT_COUNT 3
#define PANIC_RISE_CONST 4              /* 4..5: summed, the rise when no threat or attack is */
#define PANIC_RISE_ATTACK 6             /* 6..7: summed onto the threat; registers TRIG_EVENT_PANIC_ATTACKED */
typedef struct PANIC_INFO {
    u_char mode;                        /* 0x00: PANIC_MODE_* bits */
    s_char recoveryItem;                /* 0x01: PanicRecoveryTbl row of the calming item in use, -1 none */
    short recoveryTime;                 /* 0x02: its frames left (counted down by PanicUpdate) */
    float rise[PANIC_RISE_COUNT];       /* 0x04: this frame's gauge rises by source (PANIC_RISE_*),
                                           written by their TUs, cleared by PanicUpdate */
    float gauge;                        /* 0x24: the panic gauge (PanicSound: heartbeat from Panicdef.beatFrom) */
    float gaugeMax;                     /* 0x28: its top (PanicModeInit: 10000.0) */
    int filterAlpha;                    /* 0x2c: PanicFilter's vignette alpha (PanicModeInit: -1, off) */
} PANIC_INFO;
static_assert(sizeof(PANIC_INFO) == 0x30, "PANIC_INFO");
static_assert(offsetof(MODEL_PARTS, kind) == 0x42, "MODEL_PARTS.kind");
static_assert(offsetof(MODEL_PARTS, attr) == 0x44, "MODEL_PARTS.attr");
static_assert(offsetof(MODEL_PARTS, flags) == 0x50, "MODEL_PARTS.flags");
static_assert(offsetof(MODEL_PARTS, pos) == 0x130, "MODEL_PARTS.pos");

#define PARTS_FLAG_FOG 0x1              /* a fog volume: never drawn; the bank's fog part sets FOG_mode */
#define PARTS_KIND_NO_DOF 9             /* MODEL_PARTS.kind: not drawn in the depth-of-field slices */
#define PARTS_FLAG_ZONLY_FIRST 0x8      /* main_rtn: OneMaxObjectDrawZOnly runs before the part */
#define PARTS_FLAG_SORT_CENTER 0x4      /* objSort measures the entry's DRAW_OBJ_CENTER through the part's
                                           matrix, not the part's pos */

// A bank's fog record (FogCtrlAddr[bank]): the fog part, the clear colour, and gaSetupFogCtrlData's
// arguments for the bank's own objects (+0x10) and for characters lit in it (+0x28).
typedef struct FOG_INFO {
    int parts;                          /* 0x00: the fog part's index (MODEL_PARTS) */
    int clearColor[3];                  /* 0x04: R, G, B in the low bytes (main.c's FogClearColor) */
    float fog[3];                       /* 0x10: gaSetupFogCtrlData's floats */
    int fogArg[3];                      /* 0x1c: its ints */
    float charaFog[3];                  /* 0x28: the same for main_rtn's character pass */
    int charaFogArg[3];                 /* 0x34 */
} FOG_INFO;
static_assert(sizeof(FOG_INFO) == 0x40, "FOG_INFO");
static_assert(offsetof(FOG_INFO, charaFog) == 0x28, "FOG_INFO.charaFog");

// A texture of a draw list entry (DRAW_OBJ_ENTRY.tex). layout: ee (2026-11-10): bank data, tim an EE
// word (BANK_PTR). tim is a SUN_TEX_ATTR (QuatSlerp.h). The words after it are sunSetupTBufferData's
// arguments, passed by value (2026-11-10, QuatSlerp.c slice 3): where the texture sits in GS memory, its
// CLAMP_1 and TEX0's function.
typedef struct TEX_INFO {
    u_int tim;                          /* 0x00: EE address of the SUN_TEX_ATTR; 0 = none */
    u_int tbp;                          /* 0x04: GS word address of level 0 */
    u_int cbp;                          /* 0x08: GS word address of the CLUT area */
    u_int clut;                         /* 0x0c: which CLUT of that area TEX0.CBP points at */
    u_int wms;                          /* 0x10: CLAMP_1 fields (TEX_WRAP_*) */
    u_int wmt;                          /* 0x14 */
    u_int minu;                         /* 0x18 */
    u_int maxu;                         /* 0x1c */
    u_int minv;                         /* 0x20 */
    u_int maxv;                         /* 0x24 */
    int tfx;                            /* 0x28: TEX0.TFX (TEX_TFX_*) */
} TEX_INFO;
static_assert(sizeof(TEX_INFO) == 0x2c, "TEX_INFO");
static_assert(offsetof(TEX_INFO, tfx) == 0x28, "TEX_INFO.tfx");

#define TEX_WRAP_REPEAT 0               /* CLAMP_1.WMS / WMT */
#define TEX_WRAP_CLAMP 1
#define TEX_TFX_HIGHLIGHT2 3            /* TEX0.TFX 3; main_rtn also takes such a part's DRAW_OBJ_LIGHT
                                           from ALL_TABLE.partsLight */
#define TEX_SMALL_MAX 32                /* both sides <= 32: the BGT ring restarts before the upload */

// One entry of a draw list (ALLobject[bank][i], 0-terminated EE words; objSort's tables). layout: ee
// (2026-11-10): bank data; the pointer words are the ones ahRelocateSunPs2ObjectArry relocates.
typedef struct DRAW_OBJ_ENTRY {
    u_int obj;                          /* 0x00: the ga draw object (DRAW_OBJ_* offsets below) */
    u_int drawCtrlArgs;                 /* 0x04: GA_DRAWCTRL_ARGS, the entry's draw control settings
                                           (scene_init sets drawCtrl from them); non-0 and new: switch */
    u_int tex;                          /* 0x08: TEX_INFO; 0 = none */
    int parts;                          /* 0x0c: MODEL_PARTS index */
    int dofMode;                        /* 0x10: ssDOF_ClipObjectFromZRange's mode (< 2: test the
                                           object's depth against the slice) */
    u_int tbuffer;                      /* 0x14: the texture's upload packet */
    u_int drawCtrl;                     /* 0x18: GA_DRAWCTRL_DATA */
} DRAW_OBJ_ENTRY;
static_assert(sizeof(DRAW_OBJ_ENTRY) == 0x1c, "DRAW_OBJ_ENTRY");

// The ga draw object's bytes main_rtn touches (gaBGT_DrawObject's TU owns the layout).
#define DRAW_OBJ_FLAGS 0x00             /* u_char: DRAW_OBJ_FLAG_* */
#define DRAW_OBJ_CLIP 0x02              /* u_char: gaObjectClippingCheck's answer */
#define DRAW_OBJ_CENTER 0x10            /* float[4]: a point in the part's frame (objSort, PARTS_FLAG_SORT_CENTER) */
#define DRAW_OBJ_LIGHT 0x30             /* float */
#define DRAW_OBJ_POS 0x38               /* u_int: EE address of the part's ALL_TABLE.partsPos qword (BankEePtr): the
                                           ambient RGB the VU1 light0 program adds (vf20), alpha in w */
#define DRAW_OBJ_PACKET_QWC 0x48        /* u_int: the object's packet is this + 1 qwords (tiXcmd_CopySunPs2Object) */
#define DRAW_OBJ_PACKET 0x68            /* u_int: EE address of the object's packet (relocated) */
#define DRAW_OBJ_ATTR 0x3c              /* int: MODEL_PARTS.attr */
#define DRAW_OBJ_FLAG_CLIP 0x20         /* always drawn through the clipping path */

// A particle system's ring state (PARTICLE_ENTRY.state; the particle TU owns it). main_rtn_Particle
// draws the live slots head .. tail, wrapping at ringSize.
typedef struct PARTICLE_STATE {
    u_char pad00[0x46];
    u_short ringSize;                   /* 0x46 */
    u_short head;                       /* 0x48: the oldest live slot */
    u_short tail;                       /* 0x4a: one past the newest; head == tail: none live */
} PARTICLE_STATE;
static_assert(offsetof(PARTICLE_STATE, ringSize) == 0x46, "PARTICLE_STATE.ringSize");
static_assert(offsetof(PARTICLE_STATE, tail) == 0x4a, "PARTICLE_STATE.tail");

// One entry of a bank's particle list (ALL_TABLE.particles, 0-terminated EE words). layout: ee
// (2026-11-10): the relocator moves +0, +4, +8, +0xc, +0x14..+0x24.
typedef struct PARTICLE_ENTRY {
    u_int obj;                          /* 0x00: a ga draw object (the relocator's GA_OBJ_RELOC) */
    u_int drawCtrlId;                   /* 0x04: non-0: the draw control to switch to (a relocated word:
                                           probably GA_DRAWCTRL_ARGS, open-items.md 1 s) */
    u_int tex;                          /* 0x08: TEX_INFO */
    u_int state;                        /* 0x0c: PARTICLE_STATE */
    u_char pad10[0x10];
    u_int tbuffer;                      /* 0x20: the texture's upload packet */
    u_int drawCtrl;                     /* 0x24: GA_DRAWCTRL_DATA */
} PARTICLE_ENTRY;
static_assert(offsetof(PARTICLE_ENTRY, drawCtrl) == 0x24, "PARTICLE_ENTRY.drawCtrl");

// One entry of a bank's second particle list (ALL_TABLE.particles2 / particles2After, NULL-terminated;
// main_rtn_Particle2). A ring of PARTICLE2 records, each drawn as one ga draw object placed at the
// entry's model part (ALL_TABLE.partsPos[parts]) plus the record's offset. layout: ee (2026-11-10).
typedef struct PARTICLE2 {
    u_char pad00[0x20];
    float offset[4];                    /* 0x20: added to the part's position; w scales its w
                                           around PARTS_POS_W_CENTER */
    u_char pad30[0x38];
    u_int obj;                          /* 0x68: the ga draw object (DRAW_OBJ_* offsets) */
    u_int obj6c;                        /* 0x6c: relocated; not read yet */
    u_char pad70[0x10];
} PARTICLE2;
static_assert(sizeof(PARTICLE2) == 0x80, "PARTICLE2");
static_assert(offsetof(PARTICLE2, offset) == 0x20, "PARTICLE2.offset");
static_assert(offsetof(PARTICLE2, obj) == 0x68, "PARTICLE2.obj");

typedef struct PARTICLE2_ENTRY {
    u_char pad00[0x40];
    u_short parts;                      /* EE 0x40: index into ALL_TABLE.partsPos */
    u_short pad42;
    u_short ringSize;                   /* EE 0x44 */
    u_short count;                      /* EE 0x46: live records; 0 = nothing to draw */
    u_short head;                       /* EE 0x48: the oldest live record */
    u_char pad4a[6];
    u_int drawCtrlId;                   /* 0x50: non-0: the draw control to switch to (relocated) */
    u_int tex;                          /* 0x54: TEX_INFO; uploaded only for particles2 (not ...After) */
    int pad58;
    u_int ring;                         /* 0x5c: PARTICLE2[ringSize] */
    u_char pad60[0x14];
    u_int tbuffer;                      /* 0x74: the texture's upload packet */
    u_int drawCtrl;                     /* 0x78: GA_DRAWCTRL_DATA */
} PARTICLE2_ENTRY;
static_assert(offsetof(PARTICLE2_ENTRY, parts) == 0x40, "PARTICLE2_ENTRY.parts");
static_assert(offsetof(PARTICLE2_ENTRY, drawCtrlId) == 0x50, "PARTICLE2_ENTRY.drawCtrlId");
static_assert(offsetof(PARTICLE2_ENTRY, drawCtrl) == 0x78, "PARTICLE2_ENTRY.drawCtrl");

#define PARTS_POS_W_CENTER 128.0f       /* main_rtn_Particle2: w' = (w + 128) * scale - 128 */

// PORT (2026-11-09, open-items.md 1 h): a bank's relocated data (its ga draw objects first) keeps the
// EE layout, so a pointer word in it stays a u_int EE address -- a host pointer would overlap the next
// field (DRAW_OBJ_POS's word is followed by DRAW_OBJ_ATTR). The bank loader (fileio.c's TU) records each
// bank's host image and the EE address it was relocated to; BankEePtr turns such a word into a host
// pointer (RosePan's SprMan::eePtr is the precedent). Not ROM globals.
extern u_char *BankImage[BANK_MAX];             /* PORT: the bank's data on the host */
extern u_int BankImageEE[BANK_MAX];             /* PORT: the EE address its words were relocated to */
static inline void *BankEePtr(int bank, u_int ee)
{
    return ee ? BankImage[bank] + (int)(ee - BankImageEE[bank]) : NULL;
}
// A relocated word of bank `bank` as a host pointer of type T (ALL_TABLE's fields, the draw lists).
#define BANK_PTR(T, bank, ee) ((T)BankEePtr((bank), (ee)))
// The other way: the EE address a host pointer into bank `bank` had (TrgCharacterShapeSet's leg parts).
static inline u_int BankEeAddr(int bank, const void *host)
{
    return host ? BankImageEE[bank] + (u_int)((const u_char *)host - BankImage[bank]) : 0;
}

#define PARTICLE_OBJ_SIZE 0x80          /* ParticleObject[bank][i]: one ga draw object per ring slot */

// A part motion-blur object (ALLpartBlurObj[mode][bank][i]; the ss part-blur TU, ssSetTexUVforCubeMap.c,
// owns it). layout: host -- only the two words objSortWithBlurIdx touches are named.
typedef struct PART_BLUR_OBJ {
    u_char pad000[0x110];
    int listIndex;                      /* 0x110: its entry's index in the bank's objTrans list */
    int sortedIndex;                    /* 0x114: objSortWithBlurIdx: its row in the sorted table */
} PART_BLUR_OBJ;
static_assert(offsetof(PART_BLUR_OBJ, listIndex) == 0x110, "PART_BLUR_OBJ.listIndex");

#define PART_BLUR_MAX 48                /* ALLpartBlurObj's slots per bank (0xc0 bytes of EE words) */
#define PART_BLUR_MODES 2               /* ALLpartBlurObj / NumPartBlurObj: [0] and [1] (main_rtn's mode 1) */

#define OS_OBJ_TABLE_BYTES 2000         /* osTotalObjTable: one scene's sort table, 3 x 86 of them */

// A screen filter's fade (DemoFilterChangeWork, FilterChangeWork: 0x28 bytes): a full-screen quad of one
// colour, A R G B in [0..3]. Each frame current moves 1/timer of the way to target and timer counts down
// (QuatSlerp.c's *ScreenFilter, 2026-11-10: +0x08 is the target, +0x18 what is drawn).
typedef struct FILTER_CHANGE_WORK {
    short active;                       /* 0x00: FILTER_* bits (0: off) */
    short mode;                         /* 0x02: FILTER_MODE_* */
    int timer;                          /* 0x04: frames left of the fade */
    int target[4];                      /* 0x08: A, R, G, B the fade goes to */
    int current[4];                     /* 0x18: A, R, G, B drawn */
} FILTER_CHANGE_WORK;
#define FILTER_ON 0x1                   /* FILTER_CHANGE_WORK.active: drawn */
#define FILTER_OFF_AT_END 0x2           /* switched off when the fade ends */
#define FILTER_MODE_ADD 0               /* FILTER_CHANGE_WORK.mode: Cs * As + Cd */
#define FILTER_MODE_BLEND 1             /* (Cs - Cd) * As + Cd; other modes keep the last draw control */
static_assert(sizeof(FILTER_CHANGE_WORK) == 0x28, "FILTER_CHANGE_WORK");
static_assert(offsetof(FILTER_CHANGE_WORK, timer) == 0x04, "FILTER_CHANGE_WORK.timer");
static_assert(offsetof(FILTER_CHANGE_WORK, current) == 0x18, "FILTER_CHANGE_WORK.current");

// One fog animation slot of FogAnimeBuff[86] (0x48 bytes): the targets a bank's FOG_INFO eases to
// (FogAnimeUpdate, 2026-11-10: each flagged value moves 1 / time of the way a frame, and takes the target
// when time reaches 0, which ends the animation). system_init clears its head; the script sets it.
#define FOG_ANIME_FOG0 0x01             /* flags: FOG_INFO.fog[0] (charaFlags: charaFog[0]) */
#define FOG_ANIME_FOG1 0x02             /* fog[1] */
#define FOG_ANIME_FOG2 0x04             /* fog[2] */
#define FOG_ANIME_FOG_ARG 0x08          /* fogArg[0..2] */
#define FOG_ANIME_CLEAR_COLOR 0x10      /* clearColor (flags only): also drawcom_frameclear's RGBAQ */
typedef struct FOG_ANIME {
    short time;                         /* 0x00: frames left for flags' values */
    short charaTime;                    /* 0x02: frames left for charaFlags' */
    u_char flags;                       /* 0x04: FOG_ANIME_* animating the bank's own fog */
    u_char charaFlags;                  /* 0x05: FOG_ANIME_FOG* / _FOG_ARG animating the character fog */
    u_char pad06[6];
    int clearColor[3];                  /* 0x0c: the targets, as FOG_INFO's */
    float fog[3];                       /* 0x18 */
    int fogArg[3];                      /* 0x24 */
    float charaFog[3];                  /* 0x30 */
    int charaFogArg[3];                 /* 0x3c */
} FOG_ANIME;
static_assert(offsetof(FOG_ANIME, clearColor) == 0x0c, "FOG_ANIME.clearColor");
static_assert(offsetof(FOG_ANIME, charaFogArg) == 0x3c, "FOG_ANIME.charaFogArg");
static_assert(sizeof(FOG_ANIME) == 0x48, "FOG_ANIME");
