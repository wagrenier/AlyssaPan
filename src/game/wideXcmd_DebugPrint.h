// game/wideXcmd_DebugPrint.h -- CT3 game/wideXcmd_DebugPrint.c (0x018f3f60..0x018fe500): the wide xcmd debug
// print and the player character's operation (as*: hits, motion requests, the key tables, the pad move).
// Slice 1 (2026-11-10): the per-frame operation, the vibration helpers, the key enable and the motion
// request queue. Prototypes not yet written come from the call sites; their slices confirm them.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

// ---- CHARACTER fields this TU reads (the struct's owner is game/Character_Init.c) ----------------------
#define CHARACTER_FLAG_PLAYER 0x20000000u /* asCharacterOperate: operated in the player pass (keys, arrow, moth) */
// CHARACTER_FLAG_REFRACTION (0x80000000, named from main's draw use) is also asCharacterOperate's gate:
// set, the character runs the whole operation; clear, only its behaviour requests (open-items.md 1 u).
#define CHARA_BEHAVIOR_CHANGE 0x3aa     /* CHARACTER: u_char[6], cleared each frame (asClearBehaviorChangeFlag) */
#define CHARA_BEHAVIOR_CHANGE_COUNT 6
#define CHARA_TARGET 0x300              /* CHARACTER: w300[0], EE address of the object it faces (lock-on?) */
#define CHARA_TARGET_FLAGS 0x194        /* target: u_int flags */
#define CHARA_TARGET_FLAG_F4 0x100000u  /* target flag copied to CHARACTER.targetState bit 0 */
#define CHARA_TARGET_VEC 0x90           /* target: the vector asCharacterOperate dots with the facing */

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

#define CONTROLLER_TYPE_DEFAULT 2       /* ChrMoveControllerMode_init's controller_type */

extern int MsgCnt;                      /* sbss 0x01944958: FrameCounter while TRIG_VAR_PANIC_OFF is set */
extern int sndKAKOKYU;                  /* sbss 0x01944950: the breath SE plays */
extern int bCharKeyEnable;              /* sbss 0x01944a04: the player's keys are enabled */
extern u_int pDstMPstock;               /* sbss 0x01944a40: EE word, cleared after the player pass */
extern u_int pSrcMPstock;               /* sbss 0x01944a80: EE word, cleared after the player pass */

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
void asKatchuKekkaiCheck(int a0);                                                        // CT3 0x018f40d0
void asPanicStatusCheck(int mode);                                                       // CT3 0x018f42e0
void asCharacterPositionReviewInPausing(CHARACTER *chr);                                 // CT3 0x018f4bd0
void asCharacterForceMoveOperate(CHARACTER *chr);                                        // CT3 0x018f4c60
// One frame of every character of chr[0..CHARACTER_MAX): `others` the non-player pass, `player` the
// player pass (CHARACTER_FLAG_PLAYER), `animate` the animation and motion functions.
void asCharacterOperate(CHARACTER *chr, int others, int player, int animate);            // CT3 0x018f4fc0
void asCharacterMortionFunctionOperation(CHARACTER *ch);                                 // CT3 0x018f55d0
void asMotionAdderByNowMotion(CHARACTER *ch);                                            // CT3 0x018f6b50
void asCharacterPositioning(CHARACTER *chr, int a1, int a2);                             // CT3 0x018f6f80
void as1CharacterPositioning(CHARACTER *chr);                                            // CT3 0x018f7040
void asResetCharacterRotation(int a0, int a1, int a2);                                   // CT3 0x018f73b0
int asCharacterAnimationExecute(CHARACTER *ch);                                          // CT3 0x018f7430
void asPlayerInputAnalyze(CHARACTER *ch);                                                // CT3 0x018f79f0
void asCharacterAutoAiming(CHARACTER *ch);                                               // CT3 0x018f7cd0
void asPIA1_KEY_TBL_PULLED(void);                                                        // CT3 0x018f7ec0
void asPIA1_KEY_TBL_WATER(void);                                                         // CT3 0x018f8030
void asPIA1_KEY_TBL_PMOVEEND(void);                                                      // CT3 0x018f8610
void asRapidLeverChangeOperate(int a0, int a1, int a2, int a3, int a4);                  // CT3 0x018fb4c0
void asMonsterDistanceStack_inPanic(int a0, int a1, int a2);                             // CT3 0x018fb940
void asAlyssaLockedByAnything(int mode);                                                 // CT3 0x018fbb50
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
void asInitPlayerBehaviorControler(CHARACTER *chr);                                      // CT3 0x018fe380
void ChrMoveControllerMode_init(void);                                                   // CT3 0x018fe4b0
