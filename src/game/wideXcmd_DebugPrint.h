// game/wideXcmd_DebugPrint.h -- CT3 game/wideXcmd_DebugPrint.c (0x018f3f60..0x018fe500): the wide xcmd debug
// print and the player character's operation (as*: hits, motion requests, the key tables, the pad move).
// Slice 1 (2026-11-10): the per-frame operation, the vibration helpers, the key enable and the motion
// request queue. Slice 2 (2026-11-10): positioning, the pause review, the panic / barrier / lock
// states, the lever mash, the monster distance stack and the behaviour controller's init. Prototypes
// not yet written come from the call sites; their slices confirm them.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

// ---- CHARACTER fields this TU reads (the struct's owner is game/Character_Init.c) ----------------------
#define CHARACTER_FLAG_PLAYER 0x20000000u /* asCharacterOperate: operated in the player pass (keys, arrow, moth);
                                           asSetCharacterIdentity sets / clears it */
#define CHARACTER_FLAG_BUILD_NEG 0x02000000u /* as1CharacterPositioning: build the tree with -bank once, then cleared */
#define CHARA_BASE_ROT 0x334            /* CHARACTER: int, subtracted from rot when the shape has a BasePoint part */
// CHARACTER_FLAG_REFRACTION (0x80000000, named from main's draw use) is also asCharacterOperate's gate:
// set, the character runs the whole operation; clear, only its behaviour requests (open-items.md 1 u).
#define CHARA_BEHAVIOR_CHANGE 0x3aa     /* CHARACTER: u_char[6], cleared each frame (asClearBehaviorChangeFlag) */
#define CHARA_BEHAVIOR_CHANGE_COUNT 6
#define CHARA_TARGET 0x300              /* CHARACTER: w300[0], EE address of the object it faces (lock-on?) */
#define CHARA_TARGET_FLAGS 0x194        /* target: u_int flags */
#define CHARA_TARGET_FLAG_F4 0x100000u  /* target flag copied to CHARACTER.targetState bit 0 */
#define CHARA_TARGET_VEC 0x90           /* target: the vector asCharacterOperate dots with the facing */
#define CHARA_TARGET2 0x304             /* CHARACTER: w300[1], a second such object (asCheckAlyssaOnStairway) */

// ---- the motion table's records (CHARA_MOTION_TABLE, CHARA_MOTION_SIZE bytes; names from these uses) ----
#define MOTION_REC_NEXT 0x10            /* short: asChangeMortionInfo_NextMortion's motion */
#define MOTION_REC_NEXT_MODE 0x14       /* short: and its mode */
#define MOTION_REC_KIND 0x1c            /* u_char: MOTION_KIND_* */
#define MOTION_KIND_STAIRS 3            /* asCheckAlyssaOnStairway: kinds 3 and 1 are the stair motions */
#define MOTION_KIND_STAIRS2 1
#define MOTION_REC_RUN 0x1f             /* u_char: MOTION_RUN_FAST makes asPIA0_KEY_TBL_PRUN answer 2 */
#define MOTION_RUN_FAST 2
#define PIA_KEY_H0C 0x0c                /* asPIA0_KEY_TBL_HAIHAI's second argument: short set to 1 */

// ---- the motion request queue (CHARA_MOTION_QUEUE / _MODE, ahRelocateSunPs2ObjectArry.h) ---------------
#define CHARA_MOTION_REQ_PLAY 0         /* asCharacterMortionChangeRequestHash's mode */
#define CHARA_MOTION_REQ_STOP 2         /* asCharacterMortionStopRequest's mode (CHARA_MOTION_QUEUE_STOP) */
#define CHARA_MOTION_KAKOKYU 0xb1       /* motion index: Alyssa's breathing (sndKAKOKYU's breath SE plays) */
#define CHARA_MOTION_BREATH_STOP 0xb    /* motion index of characters 29..31 that silences the breath */
#define CHARA_BREATH_STOP_FIRST 29      /* the characters asCharacterOperate checks: 29, 30, 31 */
#define CHARA_BREATH_STOP_COUNT 3

// ---- vibration (the triggers' strength variables; TRIG_EVENT_PANIC_VIBRATE is the "vibration on" event)
#define TRIG_VAR_VIB_LIGHT 0x46         /* asDualShockVibrationOn_Light's strength */
#define TRIG_VAR_VIB_MIDIUM 0x47        /* asDualShockVibrationOn_Midium's strength */
/* TRIG_VAR_PANIC_VIBRATE (0x49, QuatSlerp.h) is asDualShockVibrationOn_SuperLarge's strength */
#define SE_KAKOKYU 0x3d                 /* SSPlaySE / SSStopSE's id for the breath ("c_0000") */

// ---- Alyssa's hit reactions (asChangeAlyssaMortionByHiting / _ByHirumi) ------------------------------
#define CAMERA_MODE_PLAY 4             /* GlobalCameraMode: the reactions run only in it (name from this use) */
#define ALYSSA_HIT_LETHAL 0            /* Hiting's mode: with the panic barrier gone, a fatal blow (names from */
#define ALYSSA_HIT_PULL 1              /* ... this use): pulls her 10 toward the hit, never the barrier */
#define ALYSSA_HIT_PLAIN 2             /* ... the plain flinch whatever the panic state */
#define ALYSSA_HIT_PUSH 10.0f          /* the step along the hit direction (PULL: negated) */
#define TRIG_EVENT_NO_FATAL_HIT 0x1e   /* set: ALYSSA_HIT_LETHAL does not call PlayerDamage */
#define ALYSSA_FATAL_DAMAGE 100000000  /* PlayerDamage of a lethal hit */
#define ALYSSA_HIT_VIBRATE_TIME 60
// Motion indices: each reaction has a front one (the hit within 90 degrees of her facing) and a back one.
#define MOTION_FLINCH_FRONT 0x5f
#define MOTION_FLINCH_BACK 0x60
#define MOTION_FATAL_FRONT 0x66
#define MOTION_FATAL_BACK 0x67
#define MOTION_FLINCH2_FRONT 0xd5      /* the flinch while targetState bit 0 is clear */
#define MOTION_FLINCH2_BACK 0xd6

// ---- asInitPlayerBehaviorControler's fields (CHARACTER, the behaviour block 0x310..0x51c) -------------
#define CHARA_BHV_Q310 0x310            /* two qwords: 0 at init */
#define CHARA_BHV_BYTES 0x354           /* u_char[8]: 0, 0, 0, 0, 1, -1, 0, 6 at init */
#define CHARA_BHV_SLOT_COUNT 6          /* the per-slot arrays (CHARA_MOTION_SLOT_COUNT) */
#define CHARA_BHV_QUEUE_PER_SLOT 7      /* 6 * 7 = CHARA_MOTION_QUEUE_COUNT */
#define CHARA_BHV_W35C 0x35c            /* int[6]: 0 at init */
#define CHARA_BHV_W374 0x374            /* int[6]: 0 at init */
#define CHARA_BHV_H38C 0x38c            /* short[6]: -1 at init */
#define CHARA_BHV_B3A4 0x3a4            /* u_char[6]: 0 at init */
#define CHARA_BHV_W458 0x458            /* int[6]: 0 at init */
#define CHARA_BHV_W470 0x470            /* int[6]: 0 at init */
#define CHARA_AIM_POINT 0x500           /* qword: asCharacterAutoAiming turns toward it while its w (CHARA_AIM_W) is 1.0 */
#define CHARA_AIM_W 0x50c               /* float: the point's w, 1.0 = aim at the point (asPIA0_KEY_TBL_PULLED sets it) */
#define CHARA_AIM_SPEED 0x510           /* u_short: turn per global tick (PULLED: 0x6d) */
#define CHARA_AIM_BANK 0x512            /* u_short: the bank of CHARA_AIM_PART */
#define CHARA_AIM_PART 0x514            /* int: part index to aim at (its pos), 0 = none; 0 at init */
#define MOTION_REC_FACING 0x1d          /* motion record u_char: MOTION_FACING_BACK_* kinds aim with rot + 0x8000 */
#define MOTION_FACING_BACK_MIN 0x0f     /* 0x0f..0x12: the motion faces backward */
#define MOTION_FACING_BACK_MAX 0x12
#define AIM_BACK_ROT 0x8000
#define CHARA_BHV_W518 0x518            /* int: -1 at init */
#define CHARA_NO_MOTION_TABLE 0x22      /* CHARACTER.id that gets no motion table (the last slot, 34) */
#define MOTION_TABLE_H12 0x12           /* motion table: short, cleared for the CALH.BIN shape */

// ---- asPanicStatusCheck / asAlyssaLockedByAnything / asMonsterDistanceStack_inPanic modes -------------
#define PANIC_STATUS_RESET 0            /* clear the stack */
#define PANIC_STATUS_READ 1
#define PANIC_STATUS_PUSH 2             /* shift in this frame's PANIC_MODE_FULL bit */
#define LOCKED_SET_1 0                  /* asAlyssaLockedByAnything: low byte 0 / 3 / 4 store 1 / 2 / 3 */
#define LOCKED_QUERY 1                  /* the state, or 0 while TRIG_EVENT_BOSS_ALYS_DEAD (no high byte) */
#define LOCKED_TAKE 2                   /* the state, then cleared */
#define LOCKED_SET_2 3
#define LOCKED_SET_3 4
#define LOCKED_KEEP_MASK 0xff00         /* the part of mode the state keeps */
#define MDS_PUSH 0                      /* asMonsterDistanceStack_inPanic: append (id, distance) */
#define MDS_FREE 1                      /* free the stack */
#define MDS_NEAREST 2                   /* the last entry within MDS_FAR (its id), -1 for none */
#define MDS_FORGET -1                   /* drop the stack without freeing (main's reset) */
#define MDS_GROW 16                     /* entries per growth */
#define MDS_FAR 1e25f
#define RLC_SET 0                       /* asRapidLeverChangeOperate: start (once) and poll the mash */
#define RLC_FREE 1                      /* drop it; 1 when one ran */

#define CONTROLLER_TYPE_DEFAULT 2       /* ChrMoveControllerMode_init's controller_type */

extern int MsgCnt;                      /* sbss 0x01944958: FrameCounter while TRIG_VAR_PANIC_OFF is set */
extern int sndKAKOKYU;                  /* sbss 0x01944950: the breath SE plays */
extern int bCharKeyEnable;              /* sbss 0x01944a04: the player's keys are enabled */
extern u_int pDstMPstock;               /* sbss 0x01944a40: EE word, cleared after the player pass */
extern u_int pSrcMPstock;               /* sbss 0x01944a80: EE word, cleared after the player pass */
// Statics (symtab name$n): iLocked$529 0x01944970, pMDS$545 0x01944978, iMaxSize$546 0x0194497c,
// iUseSize$547 0x01944980, iRLC_ID$667 0x0193bad4 (sdata: -1), iPanicStack$2856 0x019449c4,
// LastTicks$2901 0x019449c8.

int wideXcmd_DebugPrint(void);                                                           // CT3 0x018f3f60
void WideErrorMsgDebug(void);                                                            // CT3 0x018f3f90
void asDualShockVibrationOff(void);                                                      // CT3 0x018f3fa0
void asDualShockVibrationOn_Light(int time);                                             // CT3 0x018f3fb0
void asDualShockVibrationOn_Midium(int time);                                            // CT3 0x018f4010
void asDualShockVibrationOn_SuperLarge(int time);                                        // CT3 0x018f4070
// Alyssa (Character[0]) hit from `hit` (a position): steps 10 along the hit line, vibrates and, unless the
// motion already plays, turns to face the hit and queues the reaction's front or back motion.
void asChangeAlyssaMortionByHiting(const float *hit, int mode);                          // CT3 0x018f43d0
// The flinch alone: no step, front / back by the hit's side.
void asChangeAlyssaMortionByHirumi(const float *hit);                                    // CT3 0x018f4ab0
// 1 unless `ticks` have passed since the last check and the barrier is gone (Kekkai_Check != 0: then 0
// and the time is not kept); ticks < 0 resets and answers 0.
int asKatchuKekkaiCheck(int ticks);                                                      // CT3 0x018f40d0
// The panic history: one bit per PANIC_STATUS_PUSH, newest lowest; answers the stack.
u_int asPanicStatusCheck(int mode);                                                      // CT3 0x018f42e0
void asSetCharacterIdentity(CHARACTER *ch, int player);                                  // CT3 0x018f4340
// Sets record `index` of the character's motion table's next motion and its mode.
void asChangeMortionInfo_NextMortion(int chara, int index, short next, short nextMode);  // CT3 0x018f4380
// While unpaused (or PAUSE_RUNNING) with a pause reserved: every operated character back to posBefore.
void asCharacterPositionReviewInPausing(CHARACTER *chr);                                 // CT3 0x018f4bd0
void asCharacterForceMoveOperate(CHARACTER *chr);                                        // CT3 0x018f4c60
// One frame of every character of chr[0..CHARACTER_MAX): `others` the non-player pass, `player` the
// player pass (CHARACTER_FLAG_PLAYER), `animate` the animation and motion functions.
void asCharacterOperate(CHARACTER *chr, int others, int player, int animate);            // CT3 0x018f4fc0
// 1 when Alyssa plays a stair motion (MOTION_KIND_STAIRS*) and her targets' flags say she is on the
// stairs: then she is put back to posBefore.
int asCheckAlyssaOnStairway(void);                                                       // CT3 0x018f5440
void asCharacterMortionFunctionOperation(CHARACTER *ch);                                 // CT3 0x018f55d0
void asMotionAdderByNowMotion(CHARACTER *ch);                                            // CT3 0x018f6b50
// as1CharacterPositioning for every operated character of the pass (asCharacterOperate's passes).
// Moves the bank's placement (part 0's drawOnPos) by delta's xyz, unless Alyssa is on the stairs.
void asCharacterPositioningOnlyDeltaPos(int bank, const float *delta);                   // CT3 0x018f6f00
void asCharacterPositioning(CHARACTER *chr, int others, int player);                     // CT3 0x018f6f80
// Places the character's shape (part 0's placement) at pos / rot, its BasePoint part on pos, and builds
// the bank's matrix tree.
void as1CharacterPositioning(CHARACTER *chr);                                            // CT3 0x018f7040
// Turns every record of type 1 of `recs` (part records, PARTS_REC) by `rot` about Y; NULL: nothing.
void asResetCharacterRotation(int unused, u_char *recs, int rot);                        // CT3 0x018f73b0
int asCharacterAnimationExecute(CHARACTER *ch);                                          // CT3 0x018f7430
void asPlayerInputAnalyze(CHARACTER *ch);                                                // CT3 0x018f79f0
void asCharacterAutoAiming(CHARACTER *ch);                                               // CT3 0x018f7cd0
void asPIA1_KEY_TBL_PULLED(void);                                                        // CT3 0x018f7ec0
void asPIA0_KEY_TBL_HAIHAI(CHARACTER *ch, u_char *key);                                  // CT3 0x018f8020
void asPIA1_KEY_TBL_WATER(void);                                                         // CT3 0x018f8030
int asPIA0_KEY_TBL_WATER(CHARACTER *ch);                                                 // CT3 0x018f8040
int asPIA0_KEY_TBL_LOCK(CHARACTER *ch);                                                  // CT3 0x018f8600
void asPIA1_KEY_TBL_PMOVEEND(void);                                                      // CT3 0x018f8610
int asPIA0_KEY_TBL_PMOVEEND(CHARACTER *ch);                                              // CT3 0x018f8620
// 2 when the current motion's record is a fast run (MOTION_RUN_FAST), else 1.
int asPIA0_KEY_TBL_PRUN(CHARACTER *ch);                                                  // CT3 0x018f86d0
// The lever mash (ControllerClattering*): RLC_SET starts one with the four settings (< 0: default) and
// answers its state (> 0: over, forgotten); RLC_FREE drops it.
int asRapidLeverChangeOperate(int mode, int time, int decay, int w08, int goal);         // CT3 0x018fb4c0
int asMonsterDistanceStack_inPanic(int mode, int *id, float *dist);                      // CT3 0x018fb940
// Alyssa's lock state (low byte of mode: LOCKED_*; the high byte is kept with the state).
int asAlyssaLockedByAnything(int mode);                                                  // CT3 0x018fbb50
void asRequestExecute(CHARACTER *ch);                                                    // CT3 0x018fbc20
void asCheckCharacterHited(int chara);                                                   // CT3 0x018fc2f0
void asClearBehaviorChangeFlag(CHARACTER *ch);                                           // CT3 0x018fe0c0
// Stops the character's motion slot (asSceneAnimationStop_S's call: chara, slot 0..5): queues the
// slot's motion with CHARA_MOTION_REQ_STOP.
void asCharacterMortionStopRequest(int chara, int slot);                                 // CT3 0x018fe1f0
// Queues the motion `hash` names (asFindMotionID); 1 when there is none, else 0.
u_long asCharacterMortionChangeRequestHash(int chara, int mode, u_long hash);            // CT3 0x018fe230
// Appends (index, mode) to the character's request queue; 1 when the queue is full, else 0.
u_long asCharacterMortionChangeRequest(int chara, int mode, int index);                  // CT3 0x018fe290
// 0 while main skips frames (SkipMain) or a demo skips (DemoSkip), else bCharKeyEnable.
int asCharacterKeyEnable(void);                                                          // CT3 0x018fe330
void asSetCharacterKeyEnable(int enable);                                                // CT3 0x018fe370
// Clears the behaviour block and the motion slots / queue, and takes the motion table (CHARA_MOTION_TABLE)
// from the bank's ALL_TABLE +0xb8.
void asInitPlayerBehaviorControler(CHARACTER *chr);                                      // CT3 0x018fe380
void ChrMoveControllerMode_init(void);                                                   // CT3 0x018fe4b0
