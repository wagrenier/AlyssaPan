// game/tiBrain_ChgTrace_BOSS_BALOWS.h -- CT3 game/tiBrain_ChgTrace_BOSS_BALOWS.c: the declarations other
// TUs call (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that
// writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
// Brain initialisers: slots 3 and 4 of the ssAI_BrainInit table (0x01938580).
int tiBrain_Init_BOSS1(struct CHARACTER *chr);                                           // CT3 0x01874960
int tiBrain_Init_BOSS_SCISSORS_W(struct CHARACTER *chr);                                 // CT3 0x01874830

// The brain's mode functions (tiAvoidObject.c slice 3, 2026-11-12): entries of ssAI_charaMain's tables
// (ssAI_<Mode>Mode run the mode, ssAI_Chg<Mode>Mode pick the next; ssAI_DamageMode answers non-zero when
// the damage handling took the frame). Prototypes from that call site: chr is the character.
void tiBrain_ChgTrace_BOSS_BALOWS(struct CHARACTER *chr);                                // CT3 0x01867fb0
void tiBrain_ChgAttack_BOSS_BALOWS(struct CHARACTER *chr);                               // CT3 0x01868070
void tiBrain_ChgApproach_BOSS_BALOWS(struct CHARACTER *chr);                             // CT3 0x01868200
void tiBrain_ChgTrace_BOSS_SCISSORS_M(struct CHARACTER *chr);                            // CT3 0x01868500
void tiBrain_ChgAttack_BOSS_SCISSORS_M(struct CHARACTER *chr);                           // CT3 0x01868580
void tiBrain_ChgApproach_BOSS_SCISSORS_M(struct CHARACTER *chr);                         // CT3 0x018685f0
void tiBrain_ChgAttack_BOSS_SCISSORS_W(struct CHARACTER *chr);                           // CT3 0x01868860
void tiBrain_ChgApproach_BOSS_SCISSORS_W(struct CHARACTER *chr);                         // CT3 0x01868870
void tiBrain_ChgTrace_BOSS_AXE(struct CHARACTER *chr);                                   // CT3 0x018688c0
void tiBrain_ChgAttack_BOSS_AXE(struct CHARACTER *chr);                                  // CT3 0x01868950
void tiBrain_ChgApproach_BOSS_AXE(struct CHARACTER *chr);                                // CT3 0x01868a00
void tiBrain_ChgTrace_BOSS_VITRIOL(struct CHARACTER *chr);                               // CT3 0x01868db0
void tiBrain_ChgAttack_BOSS_VITRIOL(struct CHARACTER *chr);                              // CT3 0x01868e50
void tiBrain_ChgApproach_BOSS_VITRIOL(struct CHARACTER *chr);                            // CT3 0x01868f00
void tiBrain_ChgTrace_BOSS_HAMM(struct CHARACTER *chr);                                  // CT3 0x01869150
void tiBrain_ChgAttack_BOSS_HAMM(struct CHARACTER *chr);                                 // CT3 0x018691e0
void tiBrain_ChgApproach_BOSS_HAMM(struct CHARACTER *chr);                               // CT3 0x018692f0
void tiBrain_ChgReturn_BOSS1(struct CHARACTER *chr);                                     // CT3 0x01869540
void tiBrain_Trace_BOSS_BALOWS(struct CHARACTER *chr);                                   // CT3 0x01869550
void tiBrain_Attack_BOSS_BALOWS(struct CHARACTER *chr);                                  // CT3 0x01869930
void tiBrain_Trace_BOSS_SCISSORS_M(struct CHARACTER *chr);                               // CT3 0x0186ab60
void tiBrain_Attack_BOSS_SCISSORS_M(struct CHARACTER *chr);                              // CT3 0x0186af60
void tiBrain_Attack_BOSS_SCISSORS_W(struct CHARACTER *chr);                              // CT3 0x0186b7a0
void tiBrain_Trace_BOSS_AXE(struct CHARACTER *chr);                                      // CT3 0x0186c540
void tiBrain_Attack_BOSS_AXE(struct CHARACTER *chr);                                     // CT3 0x0186cac0
void tiBrain_Trace_BOSS_VITRIOL(struct CHARACTER *chr);                                  // CT3 0x0186cf80
void tiBrain_Attack_BOSS_VITRIOL(struct CHARACTER *chr);                                 // CT3 0x0186d750
void tiBrain_Trace_BOSS_HAMM(struct CHARACTER *chr);                                     // CT3 0x0186e0c0
void tiBrain_Attack_BOSS_HAMM(struct CHARACTER *chr);                                    // CT3 0x0186e0d0
void tiBrain_Return_BOSS1(struct CHARACTER *chr);                                        // CT3 0x0186ec10
int tiBrain_Damage_BOSS1(struct CHARACTER *chr);                                         // CT3 0x0186ec40
void tiBrain_Approach_BOSS1(struct CHARACTER *chr);                                      // CT3 0x01870e00

// ---- slice 1 (2026-11-13): the mode changes and the special-mode pickers ----
// The bosses' Chg<Mode> functions pick the next SS_AI_* mode (brain[BRAIN_MODE]) from the motion the boss
// plays (CHARA_MOTION_SLOTS[0]: a one-shot listed per boss is never cut), BRAIN_ATK_PICK (0xff: no attack
// picked, so a trace may end) and the boss info (pBossInfo, tiXcmd_TransItem.h). Names below come from
// these uses only; the Trace / Attack / Approach bodies (later slices) confirm them.

// BRAIN_SUBMODE values the bosses use (the meaning of 2..5 and 8 is read from these tests only).
#define BOSS_SUB_NONE 0                 /* a fresh approach / trace */
#define BOSS_SUB_NEAR_2 2               /* an approach that only attacks within BOSS_NEAR_LIMIT */
#define BOSS_SUB_HOLD_3 3               /* no mode change from an approach */
#define BOSS_SUB_NEAR_4 4               /* as BOSS_SUB_NEAR_2 */
#define BOSS_SUB_HOLD_5 5               /* as BOSS_SUB_HOLD_3 (not BALOWS) */
#define BOSS_SUB_SPECIAL 6              /* the special mode is on: the approach turns into a trace */
#define BOSS_SUB_BALOWS_SPECIAL 7       /* tiSetSpecialMode_BALOWS's: a SCISSORS_M approach -> trace */
#define BOSS_SUB_BALOWS_FACING 8        /* BALOWS: attack only when Alyssa is ahead (BOSS_FACING_*) */
#define BOSS_NEAR_LIMIT 200.0f          /* BOSS_SUB_NEAR_2 / _4: farther, no attack */
#define BOSS_LOS_EYE_Y 30.0f            /* the sight line runs this far above both feet */
#define BOSS_ATTACK_RANGE_SCISSORS_M 400.0f
#define BOSS_ATTACK_RANGE_AXE 100.0f
#define BOSS_ATTACK_RANGE_VITRIOL 1000.0f
#define BOSS_ATTACK_RANGE_HAMM 250.0f
#define BOSS_ATTACK_RANGE_BALOWS_CHAIN 100.0f /* BALOWS_FACING under BOSS_STATE_BALOWS_CHAIN */
#define BOSS_FACING_DOT 0.05f           /* BALOWS_FACING: facing . to-Alyssa above this: she is ahead */
#define BOSS_FACING_DOT_CHAIN 0.5f      /* the same under BOSS_STATE_BALOWS_CHAIN */
#define BOSS_AXE_STEP 150.0f            /* the AXE's special: the step it checks behind / ahead of it */
#define BOSS_AXE_MOTION_BACKSTEP 0x13   /* played before BRAIN_ATTACK_MOTION when Alyssa is ahead */
#define BOSS_SCISSORS_W_MOTION_APPROACH 0x38
#define BOSS_HAMM_MOTION_SPECIAL 0x40   /* then BOSS_HAMM_MOTION_SPECIAL_2, then BRAIN_SEARCH_MOTION */
#define BOSS_HAMM_MOTION_SPECIAL_2 0x4a /* ChgTrace_BOSS_HAMM: playing it sets BRAIN_ATK_PICK 2 */
#define BOSS_HAMM_SE_BANK "B_1A04"      /* 0x01934108 */
#define BOSS_HAMM_SE_SPECIAL 0xb
#define BOSS_HAMM_SE_ARG 0x3e0
#define BOSS_RETURN_MOTION 0            /* tiBrain_Return_BOSS1 holds motion 0 */
#define BOSS_ATK_PICK_FIRST 0           /* BRAIN_ATK_PICK: the first attack (a mode change's pick) */
#define BOSS_ATK_PICK_BLOCKED 1         /* the AXE's special found its step blocked; HAMM's special */
#define BOSS_ATK_PICK_HAMM_SPECIAL 2    /* while HAMM's special motion runs */
#define TRIG_EVENT_BOSS_AXE_NO_STEP 0x0b /* global: Character_Init.h TRIG_EVENT_CLEARED (the game was cleared once): the AXE skips the step check */
#define TRIG_VAR_SCISSORS_M_03 3        /* global variable: below 5 the SCISSORS_M special may start */
#define SCISSORS_M_VAR_03_LIMIT 5
#define BALOWS_VAR_52_CHAIN_A 4         /* TRIG_VAR_BALOWS_52 values that keep BALOWS_FACING under the chain */
#define BALOWS_VAR_52_CHAIN_B 0xe
#define BALOWS_CHAIN_SETS_MIN 2         /* BOSS_INFO_CHAIN_B_ACTIVE above it: BALOWS's special starts */
#define TRIG_EVENT_BALOWS_SPECIAL 0x6e  /* registered (global) as BALOWS's special starts */
#define BOSS_AXE1_CHARA 4               /* tiSetSpecialMode_AXE1 tests Character[4] (the ROM's fixed address) */

// Boss info words (pBossInfo offsets; tiXcmd_TransItem.h has the rest).
#define BOSS_INFO_SPECIAL_COUNT 0x18    /* int: past BOSS_INFO_SPECIAL_AT the special starts */
#define BOSS_INFO_SPECIAL_AT 0x1c       /* int: BOSS_SETUP.w1c */
#define BOSS_INFO_HAMM_28 0x28          /* int: HAMM: cleared while BOSS_INFO_HITS_74 is 0; past 1800 both clear */
#define BOSS_INFO_HAMM_28_MAX 0x708
#define BOSS_INFO_HITS_70 0x70          /* int: a count each special tests (HAMM > 5, AXE > 7, VITRIOL > 9) */
#define BOSS_INFO_HITS_74 0x74          /* int: HAMM: above 2 (within BOSS_INFO_HAMM_28_MAX) the special */
#define BOSS_INFO_APPROACH_84 0x84      /* int: cleared whenever a boss turns to an approach */
#define BOSS_INFO_AXE_88 0x88           /* int: AXE1 > 200, AXE2 > 100: the special starts */
#define BOSS_INFO_COUNT_8D 0x8d         /* u_char: SCISSORS_M > 4, VITRIOL > 2 (3 with byte 0x92): the special */
#define BOSS_INFO_HAMM_94 0x94          /* u_char: halved as HAMM's special starts, the old value kept in 0x95 */
#define BOSS_INFO_HAMM_95 0x95          /*   and put back when the special's trace ends */
#define BOSS_INFO_BALOWS_D4 0xd4        /* int: BALOWS: below 1 the second phase's event may fire */
#define BOSS_INFO_AXE_F0 0xf0           /* tiAvoidObject's followed polygon (BOSS_INFO_AVOID_HIT; a pointer on the PS2): cleared as the AXE's trace ends */
#define BOSS_INFO_SPECIAL_POS 0x120     /* float[4]: Alyssa's position when the AXE's special started */
#define BOSS_INFO_SPECIAL_KIND 0x158    /* int: BOSS_SPECIAL_*, the tiSelSpecialMode slot (BOSS_SETUP.w14c) */
// BOSS_INFO_PHASE (0x0c) bits read here.
#define BOSS_STATE_AXE_SPECIAL 0x0008   /* the AXE's special is on (AXE1 / AXE2 end it) */
#define BOSS_STATE_HAMM_SPECIAL 0x0020  /* HAMM's special motion is playing */
#define BOSS_STATE_BALOWS_2_READY 0x0100 /* BALOWS: the second phase waits (TRIG_EVENT_BALOWS_2) */
#define BOSS_STATE_HOLD_1000 0x1000     /* VITRIOL: the approach attacks at once; AXE2: the special starts */
#define BOSS_STATE_SPECIAL_ON 0x80000   /* a special mode runs (HAMM sets it; every trace's end clears it) */
#define BOSS_STATE_BALOWS_2_DEFER 0x200000 /* BALOWS_2_READY came during a one-shot: fire it after */
#define BOSS_STATE_BALOWS_CHAIN 0x800000 /* BALOWS_FACING: the chain variant (TRIG_VAR_BALOWS_52) */
// BOSS_INFO_SPECIAL_KIND: the slots of tiSelSpecialMode.
enum {
    BOSS_SPECIAL_HAMM = 0,
    BOSS_SPECIAL_VITRIOL = 1,
    BOSS_SPECIAL_AXE1 = 2,
    BOSS_SPECIAL_AXE2 = 3,
    BOSS_SPECIAL_SCISSORS_M = 4,
    BOSS_SPECIAL_BALOWS = 5,
    BOSS_SPECIAL_COUNT = 6
};
// Each answers 1 when the boss's special mode starts now (and resets its counters).
int tiSetSpecialMode_BALOWS(void);                                                       // CT3 0x01871eb0
int tiSetSpecialMode_SCISSORS_M(void);                                                   // CT3 0x01871f20
int tiSetSpecialMode_AXE2(void);                                                         // CT3 0x01871f90
int tiSetSpecialMode_AXE1(void);                                                         // CT3 0x018720b0
int tiSetSpecialMode_VITRIOL(void);                                                      // CT3 0x01872150
int tiSetSpecialMode_HAMM(void);                                                         // CT3 0x018721a0
extern int (*const tiSelSpecialMode[BOSS_SPECIAL_COUNT])(void);                          // CT3 0x01938640

// ---- slice 2 (2026-11-13): the HP, charge, defense and chain helpers, the BALOWS / SCISSORS_M traces ----
// BRAIN_ATK_PICK doubles as a special trace's step (then BRAIN_ATK_NONE: the trace may end).
#define BOSS_TRACE_STEP_START 0
#define BOSS_TRACE_STEP_RUN 1
#define BOSS_TRACE_STEP_WAIT 2          /* SCISSORS_M: its open motion 0x40 holds */
#define BOSS_TRACE_STEP_CLOSING 3       /* SCISSORS_M: closing (motion 0x41) */
// tiCalcChargeTime: value steps of BOSS_CHARGE_STEP -> ticks.
#define BOSS_CHARGE_STEP 300
#define BOSS_CHARGE_TIME_0 0x708
#define BOSS_CHARGE_TIME_1 0xa8c
#define BOSS_CHARGE_TIME_2 0xe10
#define BOSS_CHARGE_TIME_3 0x1194
#define BOSS_CHARGE_TIME_4 0x1518
#define BOSS_CHARGE_TIME_5 0x189c
#define BOSS_CLEARED_DAMAGE_SCALE 1.5f  /* a boss's hit after the game was cleared once (TRIG_EVENT_CLEARED) */
#define BOSS_HP_RATE_HIGH 0.75f         /* tiCalcHPRate bands */
#define BOSS_HP_RATE_MID 0.5f
#define BOSS_HP_RATE_LOW 0.25f
#define BALOWS_RATE_DIST_0 200.0f       /* tiCalcBalowsAttackRate's distance bands */
#define BALOWS_RATE_DIST_1 300.0f
#define BALOWS_RATE_DIST_2 400.0f
// The hit a defense mode sees (its layout unread past these two).
#define BOSS_HIT_KIND 0x01              /* u_char: BALOWS guards kinds 0 / 1, SCISSORS_MAN all but 5 */
#define BOSS_HIT_KIND_5 5
#define BOSS_HIT_DAMAGE 0x08            /* u_short: SCISSORS_MAN takes it off its hp */
// pBossInfo words of the traces.
#define BOSS_INFO_SPECIAL_TIME BOSS_INFO_BALOWS_D4 /* int 0xd4: SCISSORS_M: its special's time (cleared at step 1) */
#define BOSS_INFO_SPECIAL_TIME_MAX 0xd8 /* int: reaching it ends the special; SCISSORS_MAN's guard sets the time to it */
#define BOSS_INFO_AVOID_HIT BOSS_INFO_AXE_F0 /* 0xf0: tiAvoidObject's followed polygon (a pointer on the PS2) */
#define BOSS_STATE_SCISSORS_M_RAGE 0x0010 /* set as the cut starts: at random by hp rate, always once cleared */
#define BOSS_STATE_SCISSORS_M_END 0x40000 /* ends SCISSORS_M's special at step 2 (cleared there and at step 0) */
#define BOSS_STATE_SCISSORS_M_400000 0x400000 /* cleared as the special opens (meaning unread) */
// BALOWS's trace.
#define BALOWS_MOTION_SPECIAL 7         /* the special's roar (step 0) */
#define BALOWS_MOTION_THROW 0x0b        /* the chain throw, once facing Alyssa */
#define BALOWS_MOTION_BREAK 0x1a        /* set A's chains break (hits guarded meanwhile) */
#define BALOWS_FACE_DOT 0.95f
#define BALOWS_SE_BANK "E_BAL"          /* rodata 0x01934110 */
#define BALOWS_SE_ROAR 6
#define BALOWS_SE_BREAK 7
#define BALOWS_SE_CHAIN_LOOP 0x1b       /* sound.c's E_BAL loop */
#define BALOWS_VOICE_BANK "B_5A09"      /* 0x01934118 */
#define BALOWS_VOICE_BREAK 0x10
#define BALOWS_VOICE_THROW 0x13
#define BALOWS_CHAIN_SE_BANK "T_0010"   /* 0x01934120 */
#define BALOWS_CHAIN_SE_HOLD 0x0f
#define BALOWS_SE_ALYSSA_BANK "C_0000"  /* 0x01934128 */
#define BALOWS_SE_ALYSSA_PULLED 0x4b
#define TRIG_VAR_BALOWS_54 0x54         /* global: the throw's progress (2 thrown, 4 Alyssa pulled; -1 after) */
#define BALOWS_VAR_54_STARTED 1
#define BALOWS_VAR_54_THROWN 2
#define BALOWS_VAR_54_PULL 4
#define BALOWS_PULL_DAMAGE 10000
// SCISSORS_M's trace.
#define SCISSORS_M_MOTION_CUT 0x3e
#define SCISSORS_M_MOTION_OPEN 0x3f
#define SCISSORS_M_MOTION_WAIT 0x40
#define SCISSORS_M_MOTION_CLOSE 0x41
#define SCISSORS_M_FACE_DOT 0.98f
#define SCISSORS_M_RAGE_DIE 0x10        /* tnRand(16) below the odds: rage */
#define SCISSORS_M_RAGE_ODDS_MID 4      /* hp rate in (0.5, 0.75] */
#define SCISSORS_M_RAGE_ODDS_LOW 8      /* hp rate 0.5 or less */
#define SCISSORS_M_SE_BANK "E_SMA"      /* 0x01934130 */
#define SCISSORS_M_SE_CUT 0x21
#define SCISSORS_M_VOICE_BANK "B_4B02"  /* 0x01934138 */
#define SCISSORS_M_VOICE_OPEN 0x0b
#define SCISSORS_M_VOICE_CLOSE 0x0c

// ---- slice 3 (2026-11-13): BALOWS's attack (tiBrain_Attack_BOSS_BALOWS) ----
// Its motions (the one-shots its attack body drives; 0x18 is the miss after the grab's lunge).
#define BALOWS_MOTION_GRAB_MISS 0x18    /* the lunge found Alyssa out of reach */
#define BALOWS_MOTION_BLOOD 9           /* attack 2: the blood throw (BOSS_INFO_BLOOD_PART placed) */
#define BALOWS_MOTION_SWING 0x0a        /* attack 1 (brain attack state 0) */
#define BALOWS_MOTION_GRAB_TURN 0x0c    /* the grab: turning in, then BALOWS_MOTION_GRAB_LUNGE */
#define BALOWS_MOTION_GRAB 0x0d         /* holding Alyssa: the rattle runs, she bleeds, it heals */
#define BALOWS_MOTION_GRAB_DROP 0x0e    /* she fell or lost the rattle: a bite on a timer */
#define BALOWS_MOTION_GRAB_FREE 0x0f    /* she won the rattle */
#define BALOWS_MOTION_GRAB_LUNGE 0x1b   /* reaches for her: in reach, BALOWS_MOTION_GRAB follows */
#define BALOWS_MOTION_RECOVER 0x1c      /* after a follow-up (BOSS_STATE_BALOWS_FOLLOW) */
#define BALOWS_MOTION_STAMP 0x1d        /* near (attack state 5): turns while its timer runs */
#define BALOWS_MOTION_DOWN_HIT 0x1e     /* Alyssa down near it (attack state 6) */
#define BALOWS_MOTION_CHARGE 8          /* attack 0 far (attack state 1) */
// The brain's attack states BALOWS uses (BRAIN_ATK_STATES + n * 0x20): .b00 as an int timer, .chain as its
// limit (turning stops past it), .angle for ssAI_IsTrgWithinAngle, .panic as BOSS_INFO_APPROACH_84's value.
#define BALOWS_ATK_SWING 0              /* BALOWS_MOTION_SWING */
#define BALOWS_ATK_CHARGE 1             /* BALOWS_MOTION_CHARGE */
#define BALOWS_ATK_BLOOD 2              /* BALOWS_MOTION_BLOOD (angle only) */
#define BALOWS_ATK_GRAB 4               /* the grab: .b00 cleared at its end, .panic the bite's damage per
                                           frame (and twice that healed), .wFar the rattle's time */
#define BALOWS_ATK_STAMP 5              /* BALOWS_MOTION_STAMP */
#define BALOWS_ATK_DOWN_HIT 6           /* BALOWS_MOTION_DOWN_HIT */
#define BRAIN_BALOWS_ATK_TICKS 0x204    /* int: cleared as an attack starts */
#define BRAIN_BALOWS_FOLLOW_MOTION 0x250 /* short: a landed stamp / charge may chain into it (even odds) */
#define BRAIN_BALOWS_CLATTER 0x274      /* int: the grab's pad rattle slot (ControllerClatteringSet), -1 none */
#define BALOWS_NEAR 100.0f              /* nearer: the grab, the stamp or the down hit */
#define BALOWS_GRAB_REACH 130.0f        /* the lunge holds her within it and BALOWS_GRAB_ANGLE */
#define BALOWS_GRAB_ANGLE 0x3ffc
#define BALOWS_CHARGE_NEAR 200.0f       /* attack 0: the charge's odds (of 16) by distance */
#define BALOWS_CHARGE_MID 300.0f
#define BALOWS_CHARGE_ODDS_NEAR 4
#define BALOWS_CHARGE_ODDS_MID 12
#define BALOWS_CHARGE_ODDS_FAR 16
#define BALOWS_BITE_TICKS 0x113         /* BOSS_INFO_BITE_TICKS past it: the bite lands */
#define BALOWS_BITE_DAMAGE 20
#define BALOWS_VOICE_DELAY 0xfb         /* the swing's voice once its timer passes it (BOSS_STATE_BALOWS_FOLLOW) */
#define BALOWS_LUNGE_HP_RATE 0.6f       /* the lunge's voice: above it 0xe, else 10 */
#define BALOWS_BLOOD_HP_RATE 0.5f       /* the blood lands on Alyssa: odds 14 of 16 above it, else 8 */
#define BALOWS_BLOOD_ODDS_HIGH 0xe
#define BALOWS_BLOOD_ODDS_LOW 8
#define BALOWS_BLOOD_AHEAD 100.0f       /* else this far ahead of her */
#define BALOWS_BLOOD_RAISE 10.0f        /*   and this high */
#define BALOWS_LOCK 0x200               /* asAlyssaLockedByAnything: kept byte 2 | LOCKED_SET_* */
#define ALYSSA_MOTION_KIND2_HELD 4      /* MOTION_REC_KIND2 (asMothOperate.h MOTH_SHAKE_MOTION_KIND2): down */
#define BALOWS_SE_GRAB 1                /* E_BAL, slot 1, while the grab holds */
#define BALOWS_SE_STAMP 0x19
#define BALOWS_SE_RECOVER 0x1f
#define BALOWS_VOICE_CHARGE 0x0b        /* B_5A09 (BALOWS_VOICE_BANK) */
#define BALOWS_VOICE_LUNGE_HIGH 0x0e
#define BALOWS_VOICE_LUNGE_LOW 0x0a
#define BALOWS_VOICE_BLOOD 0x0f
#define BALOWS_VOICE_SWING 0x11
// pBossInfo words of BALOWS's attack.
#define BOSS_INFO_BALOWS_10 0x10        /* int: cleared as the recover starts (meaning unread) */
#define BOSS_INFO_BITE_TICKS 0x3c       /* int: BALOWS_MOTION_GRAB_DROP's bite timer */
#define BOSS_INFO_GRAB_TICKS 0xd0       /* int: the grab's ticks: tiCalcChargeTime of it adds to 0xd4 */
#define BOSS_INFO_BLOOD_NEAR 0x138      /* float: Alyssa this near BOSS_INFO_BLOOD_POS: BOSS_STATE_BALOWS_NEAR_BLOOD */
#define BALOWS_BLOOD_FLAGS 3            /* written to the blood part's PARTS_ANIM_FLAGS (dirty | 2) */
// BOSS_INFO_PHASE bits BALOWS's attack uses.
#define BOSS_STATE_BALOWS_RECOVERED 0x0002 /* set as the recover starts (meaning unread) */
#define BOSS_STATE_BALOWS_ATTACKING 0x0040 /* an attack started; no new one from far until it clears (SCISSORS_M too) */
#define BOSS_STATE_BALOWS_GRAB 0x0080   /* the grab's rattle runs */
#define BOSS_STATE_BALOWS_GRABBED 0x0200 /* the grab happened (no second grab: the stamp instead) */
#define BOSS_STATE_BALOWS_FOLLOWED 0x0800 /* the landed blow's follow-up was decided (SCISSORS_M too) */
#define BOSS_STATE_BALOWS_100000 0x100000 /* cleared as the swing starts (meaning unread) */
#define BOSS_STATE_BALOWS_FOLLOW 0x2000000 /* a follow-up / voice is due */
#define BOSS_STATE_BALOWS_NEAR_BLOOD 0x8000000 /* Alyssa near the blood: the throw sets TRIG_VAR_BALOWS_52 0xb */
// Triggers.
#define TRIG_EVENT_BALOWS_NO_GRAB 0x55c /* global: no grab (the number tiBrain_ChgTrace_Ghost.h's TRIG_EVENT_GHOST_1D_ON has) */
#define TRIG_VAR_BALOWS_08 8            /* global: -1 as the swing starts (meaning unread) */
#define TRIG_VAR_BALOWS_56 0x56         /* global: 0 / 1 / 2 by the phase, hp and the cleared flag at the swing */
#define BALOWS_VAR_52_BLOOD 1           /* TRIG_VAR_BALOWS_52 after the blood throw */
#define BALOWS_VAR_52_BLOOD_NEAR 0xb    /*   with the second phase waiting and Alyssa near the blood */

// ---- slice 3: SCISSORS_M's attack (tiBrain_Attack_BOSS_SCISSORS_M) ----
#define SCISSORS_M_MOTION_SLASH 0x29    /* attack state 0: the slash (a combo of 1..6, BOSS_INFO_COMBO_*) */
#define SCISSORS_M_MOTION_SLASH_END 0x2a /*   after a landed slash, then the search motion */
#define SCISSORS_M_MOTION_THRUST 0x2c   /* attack state 1: BOSS_SUB_NEAR_2's attack from afar */
#define SCISSORS_M_MOTION_THRUST_END 0x2d
#define SCISSORS_M_MOTION_STAB 0x2f     /* attack state 3: Alyssa down within SCISSORS_M_NEAR */
#define SCISSORS_M_MOTION_STAB_END 0x30
#define SCISSORS_M_ATK_SLASH 0
#define SCISSORS_M_ATK_THRUST 1
#define SCISSORS_M_ATK_STAB 3
#define SCISSORS_M_NEAR 100.0f          /* nearer: the slash (or the stab on Alyssa down) */
#define SCISSORS_M_DIST_1 200.0f        /* farther: the slash's odds (of 16) by distance and set A's chains */
#define SCISSORS_M_DIST_2 300.0f
#define SCISSORS_M_DIST_3 400.0f
#define BOSS_INFO_COMBO_DONE 0x7c       /* int: SCISSORS_M: slashes of this combo so far */
#define BOSS_INFO_COMBO_LEN 0x80        /* int: SCISSORS_M: the combo's slashes (tnRand(8): 1 1 2 2 3 4 5 6), 0 none */

float tiCalcHPRate(struct CHARACTER *chr);                                               // CT3 0x01875060
int tiCalcChargeTime(int value);                                                         // CT3 0x01875090
void tiCalcPlayerHitPoint(int damage, struct CHARACTER *chr);                            // CT3 0x01875100
void tiCalcBalowsAttackRate(float dist, int *rate0, int *rate1, int *rate2);             // CT3 0x01874a70
int tiDefenseMode_BALOWS(struct CHARACTER *chr, const u_char *hit, u_char *react);       // CT3 0x01871b70
int tiDefenseMode_SCISSORS_MAN(struct CHARACTER *chr, const u_char *hit, u_char *react); // CT3 0x01871be0
int tiSelSPMotion_BALOWS(void);                                                          // CT3 0x01872240
int tiSelSPMotion_HAMM(void);                                                            // CT3 0x01872880
void tiSetChainData(int i, int setB);                                                    // CT3 0x01875930
void tiSetChainFlag(int i, int on, int setB);                                            // CT3 0x01875630
