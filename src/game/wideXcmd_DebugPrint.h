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
#define MOTION_HAIHAI 0x30              /* motion index: crawling (asPIA1_KEY_TBL_HAIHAI's gate; names from this use) */
#define MOTION_HAIHAI_STEP 0x31         /* ... its step with PIA_KEY.h0c clear */
#define MOTION_HAIHAI_STEP2 0x32        /* ... with PIA_KEY.h0c set */

// The frame's key record asPlayerInputAnalyze builds on its stack (sp+0x60) and hands to every key-table
// entry (layout: EE, the stack slot's; field names from their uses so far).
typedef struct PIA_KEY {
    u_int cnt;                          /* 0x0: pad_cnt_button[0] (held); cleared for the no-key kinds */
    u_int trg;                          /* 0x4: pad_trg_button[0] (pressed this frame); cleared with cnt */
    short h08;                          /* 0x8: 0 */
    short h0a;                          /* 0xa: 0; the pad handlers' result (asPIA1_KEY_TBL_HAIHAI steps at 1) */
    short h0c;                          /* 0xc: 0; set to 1 by most asPIA0 entries */
    short pad0e;
} PIA_KEY;
static_assert(sizeof(PIA_KEY) == 0x10, "PIA_KEY");
static_assert(offsetof(PIA_KEY, h0a) == 0xa, "PIA_KEY.h0a");
static_assert(offsetof(PIA_KEY, h0c) == 0xc, "PIA_KEY.h0c");
// asPIA0's answer: what asPlayerInputAnalyze does before asPIA1.
#define PIA_NONE 0                      /* nothing more: asPIA1 is not called either */
#define PIA_MOVE 1                      /* the pad moves the character (asPIAK by the record's kind) */
#define PIA_HOLD 2                      /* no move (h0a cleared), then asPIA1 */
#define TRIG_EVENT_NO_PAD_MOVE 0x91     /* set: PIA_MOVE does not move (asPlayerInputAnalyze; name from this use) */
#define PIAK_KIND_COUNT 19              /* asPIAK's entries: MOTION_REC_FACING 0..18 */
#define CHARA_TARGET_COPY 0x490         /* CHARACTER: qword the target's vec, +0x10 its flags, +0x14 short part (-1 none) */
#define CHARA_TARGET2_COPY 0x4b0        /* CHARACTER: the same for CHARA_TARGET2 (0x4b0 is also refractAxis) */
#define CHARA_TARGET2_POS 0x4e0         /* CHARACTER: qword pos when CHARA_TARGET2 was copied (= CHARA_BOSS_STRIKE) */
#define CHARA_TARGET2_ROT 0x4d0         /* CHARACTER: int, rot then */
#define CHARA_TARGET_COPY_FLAGS 0x10    /* within a copy: u_int the target's flags */
#define CHARA_TARGET_COPY_PART 0x14     /* within a copy: short the target's part, -1 no target */
#define CHARA_TARGET_PART 0x198         /* target: short part index */
#define CHARA_MOTION_NONE_GROUP (-1)    /* CHARA_MOTION_GROUP with no motion: the pad drives the player directly */

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
// asChangeAlyssaMortionByDamaging (slice 4, 2026-11-10; names from this use):
#define MOTION_DAMAGE_FRONT 0xda       /* the damage motion: barrier down, targetState bit 0 clear */
#define MOTION_DAMAGE_BACK 0xdb
#define MOTION_DAMAGE_EF 0xef          /* in this motion, damage plays MOTION_DAMAGE_EF_NEXT instead */
#define MOTION_DAMAGE_EF_NEXT 0x65
#define MOTION_DAMAGE_68 0x68          /* in this motion, damage plays MOTION_DAMAGE_68_NEXT instead */
#define MOTION_DAMAGE_68_NEXT 0x6e
#define DAMAGE_JITTER 1u               /* flags: the turn is off by DAMAGE_JITTER_ROT, + on odd FrameCounter */
#define DAMAGE_JITTER_ROT 0x2000

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

// ---- asRequestExecute / asResetCharacterPosition (slice 3, 2026-11-10) --------------------------------
// The per-slot arrays (CHARA_BHV_SLOT_COUNT) by their use: slot 0 the body motion, CHARA_SLOT_BLEND (5)
// the motion slot 0 blends out of (mode 5 moves slot 0's motion there and fades it over the record's
// blend ticks; modes 4 / 6 stop / start it alone).
#define CHARA_SLOT_BODY 0
#define CHARA_SLOT_BLEND 5
#define CHARA_SLOT_PANIC 4              /* Alyssa starts its motions only while she has hp and
                                           TRIG_EVENT_BOSS_ALYS_DEAD is not set */
#define CHARA_SLOT_FADE CHARA_BHV_W35C  /* int[6]: the fade left (ticks * 100); = CHARA_SLOT_FADE_LEN at start */
#define CHARA_SLOT_FADE_LEN CHARA_BHV_W374 /* int[6]: the fade's length (the record's blend * 100), 0 none */
#define CHARA_SLOT_PREV CHARA_BHV_H38C  /* short[6]: the motion the slot played before */
#define CHARA_SLOT_PLAYABLE CHARA_BHV_W470 /* int[6]: the motion's playable ticks (asGetBehaviorPlayableTicks) */
#define CHARA_SLOT_TIME CHARA_BHV_W458  /* int[6]: the slot's time (mode 5 carries slot 0's to the blend slot) */
#define CHARA_SLOT_CHANGE_START 1       /* CHARA_BEHAVIOR_CHANGE: the slot's motion started */
#define CHARA_SLOT_CHANGE_RESUME 3      /* ... resumed where it stood (CHARA_MOTION_REQ_RESUME) */
#define CHARA_SLOT_CHANGE_AT_KEY 2      /* bit: asCharacterAnimationExecute plays the slot at its key (atKey) */
// asCharacterAnimationExecute (slice 4, 2026-11-10):
#define CHARA_SLOT_ENDED CHARA_BHV_B3A4 /* u_char[6]: 1 the frame the slot's motion ended */
#define CHARA_ROOT_PREV CHARA_BHV_Q310  /* qword: last frame's CHARA_MOTION_ROOT */
#define CHARA_BASE_ROT_PREV 0x330       /* int: last frame's CHARA_BASE_ROT */
#define CHARA_ROOT_MOVE 0x358           /* u_char (CHARA_BHV_BYTES[4], 1 at init): root motion on, times ... */
#define MOTION_REC_ROOT_MOVE 0x1a       /* u_char: ... the slot 0 record's: the root's move moves the character */
#define CHARA_MOTION_GROUP 0x359        /* s_char (CHARA_BHV_BYTES[5]): slot 0's record MOTION_REC_GROUP, -1 none */
#define CHARA_BHV_W51C 0x51c            /* int: cleared when a record of MOTION_REC_KIND2_RESET starts */
#define CHARA_MOTION_ROOT 0x320         /* qword: the BasePoint's root vector through the motion's matrix
                                           (asResetCharacterPosition); w = 1.0 (CHARA_MOTION_ROOT_W) */
#define CHARA_MOTION_ROOT_W 0x32c
// The motion record's fields asRequestExecute reads (names from this use).
#define MOTION_REC_BLEND_IN 0x12        /* short: fade ticks of a start (modes 0, 5) */
#define MOTION_REC_BLEND_RESTART 0x14   /* short: fade ticks of a restart (mode 1) */
#define MOTION_REC_GROUP 0x16           /* u_char: copied to CHARA_MOTION_GROUP */
#define MOTION_REC_SLOT 0x17            /* u_char: the slot the motion plays in */
#define MOTION_REC_PLAY_RATE 0x18       /* u_char: asChangeBehaviorAnimationPlayRate's rate */
#define MOTION_REC_KIND2 0x1e           /* u_char: MOTION_REC_KIND2_RESET clears CHARA_BHV_W51C */
#define MOTION_REC_KIND2_RESET 10
// The queue's modes (CHARA_MOTION_QUEUE_MODE); CHARA_MOTION_REQ_PLAY (0) and _STOP (2) above.
#define CHARA_MOTION_REQ_RESTART 1      /* from the top, the time kept (resetTime 0) */
#define CHARA_MOTION_REQ_RESUME 3       /* = CHARA_MOTION_REQUEST_INDEX: where it stands, no fade */
#define CHARA_MOTION_REQ_STOP_BLEND 4   /* stop the blend slot */
#define CHARA_MOTION_REQ_BLEND 5        /* slot 0 crossfades from its motion (or swaps with the blend slot) */
#define CHARA_MOTION_REQ_BLEND_ONLY 6   /* start the motion in the blend slot alone */
#define PARTS_REC_ROOT_VEC 0x20         /* part record: the vector asResetCharacterPosition carries through
                                           the BasePoint's matrix */
#define EA_DONE_ROT_BYTES(w) (((w) & 0xff) + (((w) >> 16) & 0xff)) /* the matrix's +0x3c done bytes 0 and 2 */
#define ROT_FULL_TURN 65536.0f          /* rot units per turn */
// asCatWalkEventOperation's modes and its times (@683, sdata 0x0193bad8: frames 900..2700 every 300).
#define CATWALK_RESET 0
#define CATWALK_TICK 1
#define CATWALK_CHECK 2
#define CATWALK_TIMES 7
// asCharacterForceMoveOperate: the part a character rides (names from this use).
#define FORCE_MOVE_ROOM_HASH 0x64c20b5ull /* CHARACTER.roomHash of the one room it runs in */
#define FORCE_MOVE_TARGET_PART 0x198    /* CHARA_TARGET object: short, the ridden part's index in lightBank */
#define PARTS_KIND_FORCE_MOVE 9         /* = PARTS_KIND_NO_DOF: the kind the ridden part must have */
#define PARTS_PREV_MATRIX 0x200         /* MODEL_PARTS: float[4][4], last frame's world matrix (pad200) */
#define CHARA_MOVE_DELTA 0x340          /* CHARACTER: float[3], this frame's own move; all 0 = standing */
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

// ---- asCheckCharacterHited (slice 5, 2026-11-10; names from this use) ----------------------------------
// What touched a character this frame: ALL_TABLE.hits of its bank (EE words of that bank: BANK_PTR).
// The writer (the collision TU) is not read yet: the record's fields are named from this reader.
typedef struct HIT_TABLE {
    u_char pad00[0x24];
    int count;                          /* 0x24: records in recs */
    u_int recs;                         /* 0x28: HIT_REC[count] (EE word of the bank) */
} HIT_TABLE;
typedef struct HIT_REC {
    u_int attacker;                     /* 0x00: EE word: the attacking object (HIT_ATTACKER_KIND at +0x1a2) */
    short attack;                       /* 0x04: the attack (a character's: its motion; an object's:
                                           TRIG_VAR_HIT_ATTACK) */
    short who;                          /* 0x08: the attacker's Character index, or its part in bank */
    u_char isObject;                    /* 0x0a: 0 = a character, else a part of bank `bank` */
    u_char bank;                        /* 0x0b: the object's bank (BankInfo index) */
} HIT_REC;
static_assert(sizeof(HIT_REC) == 0x0c, "HIT_REC");
static_assert(offsetof(HIT_TABLE, recs) == 0x28, "HIT_TABLE.recs");
#define HIT_ATTACKER_KIND 0x1a2         /* attacker: u_char, HIT_KIND_* */
#define HIT_KIND_PANIC 3                /* hurts in a boss battle or while PANIC_MODE_FULL */
#define HIT_KIND_NORMAL 4               /* hurts out of both */
#define CHARA_HIT_LANDED 0x51c          /* CHARACTER: int, set to 1 when its blow reached Alyssa */
#define CHARA_BOSS_STRIKE 0x4e0         /* CHARACTER: qword, the point a boss's scripted strike comes from */
// The attackers (Character indices), named by the SE bank whose hit sound they play (sound.c's banks).
#define HIT_CHARA_HAMMER 2              /* "E_HAMMER", SE 2 */
#define HIT_CHARA_RYUSAN 3              /* no hit SE (E_RYUSAN's stage) */
#define HIT_CHARA_ONO 4                 /* "E_ONO", SE 6 */
#define HIT_CHARA_SMA 5                 /* "E_SMA", SE 6 */
#define HIT_CHARA_SWO 6                 /* "E_SWO", SE 6 */
#define HIT_CHARA_32 32                 /* hurts only as HIT_KIND_PANIC; no hit SE */
#define HIT_SE_ID 6                     /* SSPlaySE's id of the attackers' hit sound ... */
#define HIT_SE_ID_HAMMER 2              /* ... E_HAMMER's */
#define ALYSSA_SCREAM_SE 6              /* SdReqPlaySE(PANIC_SE_JLS, 6..8, ALYSSA_SCREAM_ARG): her cry */
#define ALYSSA_SCREAM_COUNT 3
#define ALYSSA_SCREAM_ARG 10
// The object attackers: the first four bytes of BankInfo[].name, compared as one word.
#define HIT_BANK_E_93 0x33395f45u       /* "E_93" */
#define HIT_BANK_E_14 0x34315f45u       /* "E_14" (lands as HIT_CHARA_ONO's) */
#define HIT_BANK_E_95 0x35395f45u       /* "E_95" */
#define HIT_BANK_SCSR 0x52534353u       /* "SCSR" */
#define HIT_BANK_E_97 0x37395f45u       /* "E_97": the flinch from HIT_CHARA_32's position */
#define TRIG_VAR_HIT_ATTACK 5           /* an object's hit stores its attack here */
// The triggers (names from this use).
#define TRIG_EVENT_MUTEKI 0x95          /* registered while vsBossMuteki runs: no hits */
#define TRIG_EVENT_BOSS_STRIKE 0x46     /* a boss's scripted strike (stage 1 and 4: Hiting; 5: a motion) */
#define TRIG_EVENT_BOSS_STRUCK 0x50     /* registered when that strike hit */
#define TRIG_VAR_BOSS_ACTION 0x52       /* stage 5's strike needs it at 4 or 0x0e */
#define BOSS_ACTION_4 4
#define BOSS_ACTION_E 0x0e
#define MOTION_BOSS5_STRUCK 0xe8        /* the motion stage 5's strike plays */
#define TRIG_EVENT_NO_HIT_B0 0xb0       /* any of these set: no hits (with TRIG_EVENT_PANIC_HIDING and */
#define TRIG_EVENT_NO_HIT_6F 0x6f       /* ... TRIG_EVENT_PANIC_ON_4D / _8A / _257) */
#define TRIG_EVENT_NO_HIT_88 0x88
#define MOTION_NO_HIT 0x89              /* slot 0 playing it: no hits */
#define CHARACTER_FLAG_NO_HIT 0x10000000u /* Alyssa's flags: no hits */
#define MUTEKI_MAX 450                  /* vsBossMuteki / AfterPanicStartMuteki clamp (ticks) */
#define MOTION_REC_B16 0x16             /* motion record u_char: MOTION_B16_NO_MUTEKI ends vsBossMuteki */
#define MOTION_B16_NO_MUTEKI 0x17

#define CONTROLLER_TYPE_DEFAULT 2       /* ChrMoveControllerMode_init's controller_type */

extern int MsgCnt;                      /* sbss 0x01944958: FrameCounter while TRIG_VAR_PANIC_OFF is set */
extern int sndKAKOKYU;                  /* sbss 0x01944950: the breath SE plays */
extern int bCharKeyEnable;              /* sbss 0x01944a04: the player's keys are enabled */
extern int MagicWaterCount;             /* sbss 0x01944954: the holy water count (asWater_Run_Check) */
extern int iGacha;                      /* sbss 0x01944a08: cleared while no motion group (asPlayerInputAnalyze) */
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
// Slot 0's motion at its key (ExecuteAnimation's matrix pass): the BasePoint's root vector through it
// into CHARA_MOTION_ROOT and, when the matrix turns, CHARA_BASE_ROT from it (the parts turned by the change).
void asResetCharacterPosition(CHARACTER *ch);                                            // CT3 0x018f7210
void asResetCharacterRotation(int unused, u_char *recs, int rot);                        // CT3 0x018f73b0
// Every slot's motion one frame; queues what follows a motion that ended; moves the character by the
// BasePoint root's move when root motion is on. 1 when a motion ended this frame.
int asCharacterAnimationExecute(CHARACTER *ch);                                          // CT3 0x018f7430
void asPlayerInputAnalyze(CHARACTER *ch);                                                // CT3 0x018f79f0
void asCharacterAutoAiming(CHARACTER *ch);                                               // CT3 0x018f7cd0
// Alyssa damaged at `hit` (a qword); flags DAMAGE_JITTER.
void asChangeAlyssaMortionByDamaging(const float *hit, u_long flags);                    // CT3 0x018f46e0

// ---- the key tables (asPlayerInputAnalyze's dispatch; slice 6, 2026-11-10) -----------------------------
// Every entry takes the character and the frame's key record. asPIA0[CHARA_MOTION_GROUP] answers whether
// the pad moves the character (PIA_MOVE_*); asPIAK[the motion record's MOTION_REC_FACING] moves it;
// asPIA1[CHARA_MOTION_GROUP] then reacts to the keys. All three are .data tables of the ROM (asPIA0 sdata
// 0x0193baf0, asPIA1 0x0193bb60, asPIAK 0x0193bbd0).
void asPIA1_KEY_TBL_PULLED(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f7ec0
int asPIA0_KEY_TBL_PULLED(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f7ed0
void asPIA1_KEY_TBL_HAIHAI(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f7fa0
int asPIA0_KEY_TBL_HAIHAI(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f8020
void asPIA1_KEY_TBL_WATER(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f8030
int asPIA0_KEY_TBL_WATER(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f8040
void asPIA1_KEY_TBL_CATWALK(CHARACTER *ch, PIA_KEY *key);                                // CT3 0x018f8110
int asPIA0_KEY_TBL_CATWALK(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f81e0
void asPIA1_KEY_TBL_LOCK(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f8350
int asPIA0_KEY_TBL_LOCK(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018f8600
void asPIA1_KEY_TBL_PMOVEEND(CHARACTER *ch, PIA_KEY *key);                               // CT3 0x018f8610
int asPIA0_KEY_TBL_PMOVEEND(CHARACTER *ch, PIA_KEY *key);                                // CT3 0x018f8620
void asPIA1_KEY_TBL_PRUN(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f8630
int asPIA0_KEY_TBL_PRUN(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018f86d0
void asPIA1_KEY_TBL_PSITGO(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f8710
int asPIA0_KEY_TBL_PSITGO(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f8720
void asPIA1_KEY_TBL_PSIT(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f8730
int asPIA0_KEY_TBL_PSIT(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018f8740
void asPIA1_KEY_TBL_PSTAND(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f8750
int asPIA0_KEY_TBL_PSTAND(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f8850
void asPIA1_KEY_TBL_SITBEHIND(CHARACTER *ch, PIA_KEY *key);                              // CT3 0x018f8860
int asPIA0_KEY_TBL_SITBEHIND(CHARACTER *ch, PIA_KEY *key);                               // CT3 0x018f8a00
void asPIA1_KEY_TBL_BEHIND(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f8af0
int asPIA0_KEY_TBL_BEHIND(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f8c90
void asPIA1_KEY_TBL_KAIDAN(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f8d70
int asPIA0_KEY_TBL_KAIDAN(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f8fb0
void asPIA1_KEY_TBL_DOOR(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f8fc0
int asPIA0_KEY_TBL_DOOR(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018f8fd0
void asPIA1_KEY_TBL_YUMI(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f8fe0
int asPIA0_KEY_TBL_YUMI(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018f91d0
void asPIA1_KEY_TBL_DOWN(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f9540
int asPIA0_KEY_TBL_DOWN(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018f9590
void asPIA1_KEY_TBL_SITGONOUP(CHARACTER *ch, PIA_KEY *key);                              // CT3 0x018f95c0
int asPIA0_KEY_TBL_SITGONOUP(CHARACTER *ch, PIA_KEY *key);                               // CT3 0x018f96b0
void asPIA1_KEY_TBL_SITNOUP(CHARACTER *ch, PIA_KEY *key);                                // CT3 0x018f96c0
int asPIA0_KEY_TBL_SITNOUP(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f9720
void asPIA1_KEY_TBL_GOUP1M(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f9730
int asPIA0_KEY_TBL_GOUP1M(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f9740
void asPIA1_KEY_TBL_JUMPDOWN(CHARACTER *ch, PIA_KEY *key);                               // CT3 0x018f9750
int asPIA0_KEY_TBL_JUMPDOWN(CHARACTER *ch, PIA_KEY *key);                                // CT3 0x018f9760
void asPIA1_KEY_TBL_HASHIGO(CHARACTER *ch, PIA_KEY *key);                                // CT3 0x018f9770
int asPIA0_KEY_TBL_HASHIGO(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f9780
void asPIA1_KEY_TBL_CATCH(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018f9790
int asPIA0_KEY_TBL_CATCH(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f97a0
void asPIA1_KEY_TBL_MOVEEND(CHARACTER *ch, PIA_KEY *key);                                // CT3 0x018f97b0
int asPIA0_KEY_TBL_MOVEEND(CHARACTER *ch, PIA_KEY *key);                                 // CT3 0x018f9980
void asPIA1_KEY_TBL_RUN(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018f9b00
int asPIA0_KEY_TBL_RUN(CHARACTER *ch, PIA_KEY *key);                                     // CT3 0x018f9ee0
void asPIA1_KEY_TBL_WALK(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018f9f30
int asPIA0_KEY_TBL_WALK(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018fa1e0
void asPIA1_KEY_TBL_SITGO(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018fa230
int asPIA0_KEY_TBL_SITGO(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018fa380
void asPIA1_KEY_TBL_SIT(CHARACTER *ch, PIA_KEY *key);                                    // CT3 0x018fa390
int asPIA0_KEY_TBL_SIT(CHARACTER *ch, PIA_KEY *key);                                     // CT3 0x018fa570
void asPIA1_KEY_TBL_STAND(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018fa640
int asPIA0_KEY_TBL_STAND(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018fa8e0
void asPIAK_BHVC1_MoveBLR(CHARACTER *ch, PIA_KEY *key);                                  // CT3 0x018fa900
void asPIAK_BHVC1_Stop(CHARACTER *ch, PIA_KEY *key);                                     // CT3 0x018fa910
void asPIAK_BHVC1_MoveUD(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018fa920
void asPIAK_BHVC1_MoveLR(CHARACTER *ch, PIA_KEY *key);                                   // CT3 0x018fa930
// The chains holding Alyssa: -1 none; 1..2 the farthest one's index (query) or distance (and the pull);
// 3+ 0 (query) or CHAIN_PULL_LENGTH (she is held).
int asChainLengthCheck(int query);                                                       // CT3 0x018f4140
// She drinks holy water when the triggers allow it; 1 when she does.
int asWater_Run_Check(CHARACTER *ch);                                                    // CT3 0x018f8050
void asCharacterMoveByPadUD(PIA_KEY *key);                                               // CT3 0x018fa940
// Moves the character by the pad; dir 1 forward (MoveLR / Stop), -1 backward (MoveBLR).
void asCharacterMoveByPad(CHARACTER *ch, PIA_KEY *key, int dir);                         // CT3 0x018faa80

// The lever mash (ControllerClattering*): RLC_SET starts one with the four settings (< 0: default) and
// answers its state (> 0: over, forgotten); RLC_FREE drops it.
// The cat walk's random events: CATWALK_RESET / _TICK (the time on by this frame's ticks) / _CHECK (1
// when one of the times was passed this frame, at a random chance, or the last one is past).
int asCatWalkEventOperation(int mode);                                                   // CT3 0x018fb3b0
int asRapidLeverChangeOperate(int mode, int time, int decay, int w08, int goal);         // CT3 0x018fb4c0
int asMonsterDistanceStack_inPanic(int mode, int *id, float *dist);                      // CT3 0x018fb940
// Alyssa's lock state (low byte of mode: LOCKED_*; the high byte is kept with the state).
int asAlyssaLockedByAnything(int mode);                                                  // CT3 0x018fbb50
void asRequestExecute(CHARACTER *ch);                                                    // CT3 0x018fbc20
// Alyssa's (chara 0) hits this frame: the boss battle's invulnerability and scripted strikes, then each
// record of her bank's hit table: the reaction (Hiting / Damaging / Hirumi), the attacker's SE, her cry.
// Other characters: nothing.
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
