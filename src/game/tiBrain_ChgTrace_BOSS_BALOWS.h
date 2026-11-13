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
// ---- slice 4: the AXE's defense, the special motion pickers ----
#define BOSS_HIT_DIR 0x20               /* float[4]: the blow's direction (the AXE turns from it) */
#define AXE_GUARD_DOT 0.3f              /* facing . blow above it: from behind (no guard outside the special) */
#define AXIS_Y 2                        /* tiGetDegFromVec's axis argument the bosses pass */
#define ANGLE_UNITS_PER_DEGREE 0xb6     /* CHARACTER.rot units: 65536 / 360 */
// The AXE's motions a blow lands in unguarded before the game was cleared (tiDefenseMode_AXE): its three
// strikes and two more (0x2c, 0x30: their starters unread).
#define AXE_MOTION_OPEN_2C 0x2c
#define AXE_MOTION_OPEN_30 0x30
#define AXE_MOTION_DOWN 0x33            /* attack state 3's strike (Alyssa down): end 0x35, landed 0x34 */
#define AXE_MOTION_DOWN_END 0x35
#define AXE_MOTION_DOWN_HIT 0x34
#define AXE_MOTION_ATK_0 0x44           /* attack state 0's: end 0x2e, landed 0x2d */
#define AXE_MOTION_ATK_0_END 0x2e
#define AXE_MOTION_ATK_0_HIT 0x2d
#define AXE_MOTION_ATK_1 0x45           /* attack state 1's: end 0x32, landed 0x31 */
#define AXE_MOTION_ATK_1_END 0x32
#define AXE_MOTION_ATK_1_HIT 0x31
#define AXE_ATK_0 0
#define AXE_ATK_1 1
#define AXE_ATK_DOWN 3
#define BOSS_SP_BEHIND_DOT 0.5f         /* the pickers' "behind Alyssa" test (both dots above it) */
#define SCISSORS_M_SP_CHAIN_FAR 300.0f  /* a holding chain's first part farther from Alyssa: SP_PICK_CHAIN */
#define BOSS_VITRIOL_CHARA 3            /* tiSelSPMotion_VITRIOL reads Character[3]'s hp (the ROM's fixed address) */
// The special motion picks (*pick); what each starts is read where the pick is used.
#define SP_PICK_1 1
#define SP_PICK_BEHIND 2
#define SP_PICK_VITRIOL 4
#define SP_PICK_CHAIN 7
// SCISSORS_W's attack (tiBrain_Attack_BOSS_SCISSORS_W): strike, vanish, warp, reappear.
#define SCISSORS_W_MOTION_ATK_0 0x28    /* attack state 0's strike, then SCISSORS_W_MOTION_ATK_0_END on a landed blow */
#define SCISSORS_W_MOTION_ATK_0_END 0x29
#define SCISSORS_W_MOTION_ATK_1 0x2b    /* attack state 1's */
#define SCISSORS_W_MOTION_ATK_1_END 0x2c
#define SCISSORS_W_MOTION_DOWN 0x2e     /* attack state 3's: Alyssa down */
#define SCISSORS_W_MOTION_DOWN_END 0x2f
#define SCISSORS_W_MOTION_VANISH BOSS_SCISSORS_W_MOTION_APPROACH /* 0x38: every strike ends in it */
#define SCISSORS_W_MOTION_HIDDEN 0x39   /* out of sight: the warp, then the attack motion or the special */
#define SCISSORS_W_MOTION_SPECIAL 0x3a
#define SCISSORS_W_MOTION_SPECIAL_WAIT 0x3f /* waits for TRIG_VAR_BALOWS_52 SCISSORS_W_VAR_52_DONE */
#define SCISSORS_W_MOTION_SPECIAL_END 0x40
#define SCISSORS_W_ATK_0 0
#define SCISSORS_W_ATK_1 1
#define SCISSORS_W_ATK_DOWN 3
#define SCISSORS_W_PICK_STRIKE 1        /* BRAIN_ATK_PICK: a strike runs (the vanish requested from afar too) */
#define SCISSORS_W_PICK_ENDED 2         /*   its end motion played (the voice said) */
#define SCISSORS_W_PICK_HIDDEN 3        /*   vanished: the warp is due */
#define SCISSORS_W_REACH 150.0f         /* nearer and within state 0's angle: a strike */
#define SCISSORS_W_EYE_Y 30.0f          /* the warp's look beside Alyssa starts this far above her feet */
#define SCISSORS_W_SIDE_LOOK 200.0f     /*   and runs this far; the boss lands SCISSORS_W_SIDE_LAND along it */
#define SCISSORS_W_SIDE_LAND 120.0f
#define SCISSORS_W_WARP_NEAR_HITS 6     /* BOSS_INFO_HITS_70 from here on: the warps seek Alyssa */
#define SCISSORS_W_SPECIAL_FAR 300.0f   /* the special only farther than this from Alyssa */
#define SCISSORS_W_STILL 60.0f          /*   and Alyssa moved less than this since the last warp */
#define SCISSORS_W_SPECIAL_SHOWN 0x385  /*   and the boss was seen at least this long */
#define SCISSORS_W_SHOWN_FOREVER 99999999 /* the special's end: the next pick vanishes at once */
#define SCISSORS_W_VAR_52_DONE 999      /* TRIG_VAR_BALOWS_52's value that ends the special's wait */
#define SCISSORS_W_VOICE_BANK SCISSORS_M_VOICE_BANK /* "B_4B02" (0x01934138) */
#define SCISSORS_W_VOICE_END 0x0f       /* a strike's end motion */
#define SCISSORS_W_VOICE_SPECIAL 0x11
#define SCISSORS_W_VOICE_WARP 0x12      /* on 3 of 10 warps */
#define SCISSORS_W_SE_BANK "E_SWO"      /* 0x01934168 */
#define SCISSORS_W_SE_WARP 0x1c         /* on 3 more of 10 warps */
#define BOSS_STATE_SCISSORS_W_SPECIAL_DONE 0x10000 /* ends SCISSORS_W_MOTION_SPECIAL_WAIT (its setter unread) */
#define BOSS_STATE_SCISSORS_W_SPECIAL 0x20000 /* set as the special starts, cleared at its end motion */
#define BOSS_INFO_SHOWN_TICKS 0x20      /* int: SCISSORS_W: time seen since reappearing (its counter unread) */
#define BOSS_INFO_HIDDEN_TICKS BOSS_INFO_HAMM_28 /* int 0x28: SCISSORS_W: time hidden (its counter unread) */
#define BOSS_INFO_HIDDEN_DELAY 0x38     /* int: SCISSORS_W: hidden longer than this: she reappears */
#define BOSS_INFO_SHOWN_PICK 0x6c       /* int: tnRand(7): the BOSS_INFO_THRESHOLDS slot of her time seen */
#define BOSS_INFO_WARP_TIMES 0xc8       /* int: warps since the last special */
#define BOSS_INFO_WARP_INDEX 0xcc       /* int: the warp point she stands at */
// The AXE's trace (its special: run where Alyssa stood, turn, strike).
#define AXE_MOTION_SPECIAL_2B 0x2b      /* a special strike motion (its starter unread) */
#define AXE_MOTION_SPECIAL_HIT 0x2f     /* the special's strike, then the search motion */
#define AXE_MOTION_SPECIAL_HIT_VOICED 0x37 /* the same with AXE_VOICE_SPECIAL (BOSS_STATE_BIT_04, odds by hp) */
#define AXE_RUN_NEAR 500.0f             /* the run goes on within this of BOSS_INFO_SPECIAL_POS */
#define AXE_RUN_LOOK 100.0f             /*   until a wall this far ahead */
#define AXE_TURN_AIM_DOT 0.99f          /* facing . to-Alyssa at least this: the strike */
#define AXE_TURN_BACK_DOT -0.8f         /*   below this: BOSS_AXE_MOTION_BACKSTEP (the turn-about) */
#define AXE_VAR_52_END 1                /* TRIG_VAR_BALOWS_52's value that ends the AXE's special (set back to -1) */
#define AXE_VOICE_BANK "B_3D03"         /* 0x01934190 */
#define AXE_VOICE_SPECIAL 0x0a
// VITRIOL's trace (the special: roar, then up to five acid sprays). BRAIN_ATK_PICK is the step.
#define VITRIOL_MOTION_SPRAY 0x3b       /* a spray: on a hit VITRIOL_MOTION_SPRAY_HIT, else the attack motion */
#define VITRIOL_MOTION_RETREAT 0x3d     /* ends the special (then the search motion) */
#define VITRIOL_MOTION_SPRAY_HIT 0x3e   /* after a landed spray (VITRIOL_SE_SPRAY_HIT) */
#define VITRIOL_MOTION_ROAR 0x3f        /* step 0's, turning to Alyssa */
#define VITRIOL_ATK_SPRAY 2             /* the attack state whose timer and .panic the spray uses */
#define VITRIOL_STEP_DONE 99            /* the step after a landed spray or the retreat (then none) */
#define VITRIOL_SPRAY_REACH 200.0f      /* in line: nearer, facing her (VITRIOL_SPRAY_DOT), in sight */
#define VITRIOL_SPRAY_DOT 0.99f
#define VITRIOL_SPRAY_NEAR 75.0f        /* steps 1..3 spray this near whatever the line */
#define VITRIOL_ROAR_DOT 0.95f          /* step 0 roars once facing her this well */
#define VITRIOL_94_ROAR 0x14            /* BOSS_INFO_HAMM_94 while VITRIOL roars (0x95 restores it) */
#define BOSS_INFO_VITRIOL_TURN 0xac     /* int[], by step (2..6: 0xb4 = BOSS_INFO_B4 ...): a spray's turning time */
#define VITRIOL_SE_BANK "E_RYUSAN"      /* 0x01934198 */
#define VITRIOL_SE_SPRAY_HIT 0x1b
#define VITRIOL_VOICE_BANK "B_2D01"     /* 0x019341a8 */
#define VITRIOL_VOICE_ROAR 0x0a
int tiDefenseMode_AXE(struct CHARACTER *chr, const u_char *hit, u_char *react);          // CT3 0x01871c70
int tiSelSPMotion_BALOWS(u_char *pick);                                                  // CT3 0x01872240
int tiSelSPMotion_SCISSORS_M(u_char *pick);                                              // CT3 0x01872250
int tiSelSPMotion_AXE2(u_char *pick);                                                    // CT3 0x018724a0
int tiSelSPMotion_AXE1(u_char *pick);                                                    // CT3 0x018725d0
int tiSelSPMotion_VITRIOL(u_char *pick);                                                 // CT3 0x01872750
int tiSelSPMotion_HAMM(u_char *pick);                                                    // CT3 0x01872880
void tiSetChainData(int i, int setB);                                                    // CT3 0x01875930
void tiSetChainFlag(int i, int on, int setB);                                            // CT3 0x01875630

// ---- slice 5 (2026-11-13): VITRIOL's and HAMM's attacks, the mode setters ----
// VITRIOL's attack (tiBrain_Attack_BOSS_VITRIOL): three strikes (each with its landed motion) and the acid.
#define VITRIOL_MOTION_ATK_0 0x28       /* attack state 0's strike: landed VITRIOL_MOTION_ATK_0_HIT */
#define VITRIOL_MOTION_ATK_0_HIT 0x29
#define VITRIOL_MOTION_ATK_0_2A 0x2a    /* in the busy list only (meaning unread) */
#define VITRIOL_MOTION_ATK_1 0x2b       /* attack state 1's */
#define VITRIOL_MOTION_ATK_1_HIT 0x2c
#define VITRIOL_MOTION_ATK_1_2D 0x2d
#define VITRIOL_MOTION_DOWN 0x2e        /* attack state 3's: Alyssa down */
#define VITRIOL_MOTION_DOWN_HIT 0x2f
#define VITRIOL_MOTION_DOWN_30 0x30
#define VITRIOL_MOTION_ACID 0x3a        /* attack state 4's: past VITRIOL_ACID_TICKS the acid SE */
#define VITRIOL_ATK_0 0
#define VITRIOL_ATK_1 1
#define VITRIOL_ATK_DOWN 3
#define VITRIOL_ATK_ACID 4
#define VITRIOL_ACID_TICKS 0x19
#define VITRIOL_SE_ACID 0x1c            /* VITRIOL_SE_BANK "E_RYUSAN" */
#define VITRIOL_ATK_NEAR 100.0f         /* nearer: a strike at even odds (state 3 when Alyssa is down) */
#define VITRIOL_ATK_MID 200.0f          /* the far weights (of 16): strikes 9 / 7 / 2 / 0 by these bands, */
#define VITRIOL_ATK_FAR 300.0f          /*   the acid 0 / 0 / 1 / 2 */
#define VITRIOL_ATK_FARTHEST 600.0f
// HAMM's attack (tiBrain_Attack_BOSS_HAMM): two strikes, the down strike and the grab.
#define HAMM_MOTION_ATK_0 0x1d          /* attack state 0's strike: landed HAMM_MOTION_ATK_0_HIT, missed _END */
#define HAMM_MOTION_ATK_0_HIT 0x58
#define HAMM_MOTION_ATK_0_END 0x59
#define HAMM_MOTION_ATK_1 0x27          /* attack state 1's */
#define HAMM_MOTION_ATK_1_HIT 0x5a
#define HAMM_MOTION_ATK_1_END 0x5b
#define HAMM_MOTION_DOWN 0x5c           /* attack state 3's: Alyssa down */
#define HAMM_MOTION_DOWN_HIT 0x5d
#define HAMM_MOTION_DOWN_END 0x5e
#define HAMM_MOTION_GRAB_TURN 0x65      /* the grab, Alyssa standing: turn, reach (HAMM_MOTION_GRAB_REACH), hold */
#define HAMM_MOTION_GRAB_REACH 0x66
#define HAMM_MOTION_GRAB 0x67           /* holding her: the rattle runs, she bleeds (state 4's .panic) */
#define HAMM_MOTION_GRAB_DROP 0x68      /* she fell or lost the rattle: a bite on a timer */
#define HAMM_MOTION_GRAB_MISS 0x69      /* the reach found her out of reach (or bending) */
#define HAMM_MOTION_GRAB_FREE 0x6a      /* she won the rattle */
#define HAMM_MOTION_LOW_GRAB_TURN 0x55  /* the same with Alyssa bending */
#define HAMM_MOTION_LOW_GRAB_REACH 0x56
#define HAMM_MOTION_LOW_GRAB_MISS 0x57
#define HAMM_ATK_0 0
#define HAMM_ATK_1 1
#define HAMM_ATK_DOWN 3
#define HAMM_ATK_GRAB 4                 /* the grab's angle, damage (.panic) and rattle (.wFar) */
#define HAMM_PICK_STRUCK 2              /* BRAIN_ATK_PICK: the blow landed / the reach caught her */
#define HAMM_GRAB_NEAR 75.0f            /* nearer: the grab (12 of 16) or strike 1; farther strike 0 / 1 */
#define HAMM_ATK_MID 200.0f
#define HAMM_GRAB_REACH 130.0f          /* the reach misses farther */
#define HAMM_GRAB_ANGLE 0x3ffc          /*   or outside this angle */
#define HAMM_LOCK 0x100                 /* asAlyssaLockedByAnything: kept byte 1 | LOCKED_SET_* (BALOWS_LOCK's) */
#define HAMM_BITE_TICKS 0xfa            /* BOSS_INFO_BITE_TICKS past it: the bite lands */
#define HAMM_BITE_DAMAGE 0x14
#define HAMM_SE_BANK "E_HAMMER"         /* 0x019341b0 */
#define HAMM_SE_GRAB 0x21
#define HAMM_VOICE_GRAB 0x0c            /* BOSS_HAMM_SE_BANK "B_1A04": the grab's voice */
#define HAMM_HIT_SE_JLS 6               /* SdReqPlaySE(6, 2, 1) as a strike lands (a2 = 1 from the pick test) */
#define HAMM_HIT_SE 2
// The mode setters (tiSetMode*: the boss's next BRAIN_SUBMODE by distance and hp; called through data).
#define BOSS_SUB_PLAIN 1                /* the setters' default */
#define BOSS_BALOWS_CHARA 32            /* tiSetModeChain_BALOWS reads Character[32]'s hp (the ROM's fixed address) */
#define BOSS_MODE_FAR 200.0f            /* farther: BOSS_SUB_HOLD_3 on 1 of 16 */
#define BOSS_MODE_NEAR 100.0f           /* BALOWS before the second phase: the same from here; AXE2's bands */
#define BOSS_STATE_ARROW_10000000 0x10000000 /* after the clear, arrow level >= 2, 6 of 10: meaning unread */
#define BOSS_STATE_ARROW_20000000 0x20000000 /*   and on 5 of 10 more */
#define BALOWS_CHAIN_D4_MAX 0x5dc       /* BOSS_INFO_BALOWS_D4 below it: BOSS_STATE_BALOWS_CHAIN */
#define BALOWS_VAR_52_FACING_A 4        /* TRIG_VAR_BALOWS_52 4 or 0xe: BOSS_SUB_BALOWS_FACING */
#define BALOWS_VAR_52_FACING_B 0x0e
void tiSetModeChain_BALOWS(float dist, u_char *mode);                                    // CT3 0x01872890
void tiSetModeNormal_BALOWS(float dist, u_char *mode);                                   // CT3 0x01872a20
void tiSetModeNormal_AXE2(float dist, u_char *mode);                                     // CT3 0x01873760
// Slice 6: the other setters (band tables in the .c: distance bands 1000 .. 100, draws of 16).
#define BOSS_INFO_PULL_HITS 0x78        /* int: hits the boss took while a pull was due (BOSS_STATE_PULL_DUE; Damage_BOSS1 counts) */
#define BOSS_PULL_HITS_MAX 2            /* tiSetModeChain_AXE2: from this many, no more pulls (BOSS_SUB_HOLD_5) */
void tiSetModeNormal_HAMM(float dist, u_char *mode);                                     // CT3 0x018745e0
void tiSetModeNormal_VITRIOL(float dist, u_char *mode);                                  // CT3 0x01873f80
void tiSetModeNormal_AXE1(float dist, u_char *mode);                                     // CT3 0x01873b50
void tiSetModeNormal_SCISSORS_M(float dist, u_char *mode);                               // CT3 0x01872f70
void tiSetModeChain_HAMM(float dist, u_char *mode);                                      // CT3 0x018744a0
void tiSetModeChain_VITRIOL(float dist, u_char *mode);                                   // CT3 0x01873ce0
void tiSetModeChain_AXE1(float dist, u_char *mode);                                      // CT3 0x01873960
void tiSetModeChain_AXE2(float dist, u_char *mode);                                      // CT3 0x01873220
void tiSetModeChain_SCISSORS_M(float dist, u_char *mode);                                // CT3 0x01872c90
// Slice 6: Damage_BOSS1's checks.
#define TRIG_EVENT_BOSS_ARROW_ALWAYS 0x89 /* global: every arrow staggers the boss (its setter unread) */
#define ARROW_LEVEL_STAGGER_MIN 3       /* asGetTheArrowLevel below this never staggers; 3: power 10, 4: 20, more: 100 */
#define BOSS_INFO_ARROW_GUARD BOSS_INFO_HAMM_94 /* u_char 0x94: the boss's toughness against arrows (Damage_BOSS1 sets 12, 15, 100) */
#define BOSS_INFO_CHAIN_SLACK 0x98      /* float: a holding set A chain farther from Alyssa than this is taut */
#define BOSS_CHAINS_ALL 6               /* tiCheckPlayerINChains: this many holding chains catch her outright */
#define BOSS_CHAINS_MAX 15              /* the ROM's stack arrays (0xf0 bytes of vectors each) */
int tiCheckArrowLevel(float dist, u_char level);                                         // CT3 0x01875460
int tiCheckPlayerINChains(CHARACTER *chr);                                               // CT3 0x01875210
// Slice 6: tiBrain_Damage_BOSS1 (names from this use unless another TU's).
#define BOSS_CHARA_SCISSORS_W 6         /* the SCISSORS_W bits (0x10000 / 0x20000) clear for it; its voices are B_4B02's */
#define TRIG_VAR_CHAIN_A_BREAK TRIG_VAR_TBREAK_SHORT /* 0x53: set A's break: -1 none, 0 Alyssa caught (C_0000 0x4b),
                                           1 sound.c's T_BREAK counts (set A's time stops), 4 the break lands */
#define CHAIN_A_BREAK_NONE (-1)
#define CHAIN_A_BREAK_CAUGHT 0
#define CHAIN_A_BREAK_COUNTING 1
#define CHAIN_A_BREAK_LANDS 4
#define TRIG_VAR_CHAIN_B_BREAK TRIG_VAR_BALOWS_54 /* 0x54: == 1 set B's time stops (sound.c's TBREAK_LONG) */
#define BOSS_CHAIN_SHOT_DIST 200.0f     /* a new chain's first part: this far from the set's source along the shot */
#define BOSS_CHAIN_B_TIME_MAX 0x1194    /* set B lets go after this long holding */
#define BOSS_CHAIN_PULL_FAR 300.0f      /* a holding set A chain farther from the boss: it is pulled back (BRAIN_BOSS_M_248) */
#define BOSS_SHOWN_PICK_MAX 4           /* set A's release moves BOSS_INFO_SHOWN_PICK up to here (BALOWS: 5) */
#define BOSS_SHOWN_PICK_MAX_BALOWS 5
#define CHARA_LANDED_BLOW 0x51c         /* CHARACTER int (pad): non-zero: BOSS_INFO_APPROACH_84 is dealt to Alyssa */
#define BRAIN_BOSS_STRIKE_DAMAGE 0x0d4  /* short: what TRIG_EVENT_BOSS_STRUCK deals */
#define BRAIN_BOSS_GRABBED_TICKS 0x100  /* int: BOSS_STATE_BALOWS_GRABBED lasts while not above 0x104 (BALOWS, HAMM) */
#define BRAIN_BOSS_GRABBED_MAX 0x104
#define BRAIN_BALOWS_ATK_MAX 0x208      /* int: BOSS_STATE_BALOWS_ATTACKING lasts while BRAIN_BALOWS_ATK_TICKS is not above it */
#define BRAIN_DAMAGE_MOTION_BACK 0x266  /* short: struck from behind (BRAIN_DAMAGE_MOTION: from the front) */
#define BOSS_INFO_RECOVER_MAX 0x14      /* int: BOSS_STATE_BALOWS_RECOVERED ends once BOSS_INFO_SP_COOL passes it */
#define BOSS_INFO_SCISSORS_END_TICKS 0x30 /* int: SCISSORS_M's special at step 2: BOSS_STATE_SCISSORS_M_END past _AT */
#define BOSS_INFO_SCISSORS_END_AT 0x34  /* int: 150 .. 270 by distance (tiScissorsEndAt) */
#define BOSS_INFO_ARROW_COUNT_LEVEL 0x8e /* u_char: arrows from this level count in BOSS_INFO_HITS_74 */
#define BOSS_INFO_ARROW_STAGGER_LEVEL 0x8f /* u_char: arrows from this level stagger */
#define BOSS_INFO_ROAR_TICKS 0xdc       /* int: BALOWS staggered: E_BAL BALOWS_SE_ROAR_LATE once past 0xe0 (150) */
#define BOSS_INFO_ROAR_AT 0xe0
#define BOSS_ROAR_AT 150
#define BOSS_INFO_BREAK_DAMAGE 0x144    /* int: set A's break landing (CHAIN_A_BREAK_LANDS) deals this */
#define BOSS_INFO_SHRUG_DAMAGE 0x148    /* int: a hit with no arrow, outside the look motions, deals this */
#define BOSS_INFO_DEFENSE_KIND 0x15c    /* int: tiDefenseMode's slot (with BOSS_STATE_BIT_04) */
#define BOSS_STATE_AXE_BACKSTAB 0x8000  /* the AXE struck from behind: damage x3, and it staggers */
#define BOSS_STATE_BALOWS_52_ODD 0x1000000 /* TRIG_VAR_BALOWS_52 4 / 0xe: every other frame Alyssa loses 1 */
#define BOSS_STATE_BALOWS_ROAR_DUE 0x4000000 /* BOSS_INFO_ROAR_TICKS counts while the damage motion plays */
#define BALOWS_SE_ROAR_LATE 0x18        /* E_BAL */
#define BOSS_VOICE_ARG BOSS_HAMM_SE_ARG /* 0x3e0: every boss voice here */
#define BALOWS_CHAIN_HIT_MAX 5          /* TRIG_VAR_HIT_ATTACK below this: BALOWS's hit is a set B chain */
#define BALOWS_CHAIN_HIT_DAMAGE 0x32
#define BOSS_HIT_DAMAGE_HEAVY 150       /* Alyssa's loss to a boss blow (tiCalcPlayerHitPoint) */
#define BOSS_HIT_DAMAGE_LIGHT 100
#define SCISSORS_HIT_HEAVY_BELOW 0x12   /* SCISSORS_M's attacks below this: heavy, below 0x26 light, else none */
#define SCISSORS_HIT_LIGHT_BELOW 0x26
#define SCISSORS_M_MOTION_3E 0x3e       /* SCISSORS_M: BOSS_STATE_BALOWS_RECOVERED does not run out during it */
#define AXE_MOTION_GUARD_51 0x51        /* the AXE (BOSS_STATE_BIT_04): BOSS_INFO_ARROW_GUARD 12 in these */
#define AXE_MOTION_GUARD_52 0x52
#define AXE_ARROW_GUARD_MOTION 12
#define AXE_ARROW_GUARD_FREE 100        /* no set A chain holding */
#define AXE_ARROW_GUARD_CHAINED 15
#define BOSS_ARROW_PUSH_KIND1 150.0f    /* ARROW kind 1 pushes this far (else BOSS_INFO_HIT_BACKS[level]) */
#define BOSS_STRUCK_EXTRA 100           /* TRIG_EVENT_BOSS_ARROW_ALWAYS: every hit deals this more */
enum BOSS_REACT {                       /* how a hit was taken (the voice picks by it) */
    BOSS_REACT_NONE = 0,
    BOSS_REACT_CHAINS_CUT = 1,          /* set A's break landed */
    BOSS_REACT_STAGGER = 2,
    BOSS_REACT_SHRUG = 3,               /* a hit with no arrow */
};
extern int (*const tiDefenseMode[3])(struct CHARACTER *chr, const u_char *hit, u_char *react); // .data 0x01938658
// The boss's damage step, every frame: the chain sets' times, what the boss's blows dealt Alyssa, the timers,
// then this frame's hits: arrows chain it (set A), stagger it or only hurt; the voices; the mode it returns
// to. 1 while the damage motion plays or the frame is taken.
int tiBrain_Damage_BOSS1(struct CHARACTER *chr);                                         // CT3 0x0186ec40
// The setters' tables, by BOSS_INFO_NORMAL_KIND / _CHAIN_KIND / _SP_KIND (the same order as tiSelSpecialMode).
#define BOSS_MODE_KIND_COUNT 6
extern void (*const tiSetNormalMode[BOSS_MODE_KIND_COUNT])(float dist, u_char *mode); // .data 0x019385e0
extern void (*const tiSetChainMode[BOSS_MODE_KIND_COUNT])(float dist, u_char *mode);  // .data 0x01938600
extern int (*const tiSelSPMotion[BOSS_MODE_KIND_COUNT])(u_char *pick);                // .data 0x01938620

// ---- slice 5: the approach (tiBrain_Approach_BOSS1) ----
// BOSS_INFO_CHARA identifies the boss (the voices and the fixed-address reads agree): the approach's per-boss
// voices key on it, so tiSelSPMotion_VITRIOL's Character[3] and tiSetModeChain_BALOWS's Character[32] are
// the bosses themselves.
#define BOSS_CHARA_HAMM 2
#define BOSS_CHARA_VITRIOL 3            /* = BOSS_VITRIOL_CHARA */
#define BOSS_CHARA_AXE 4
#define BOSS_CHARA_SCISSORS 5
#define BOSS_CHARA_BALOWS 32            /* = BOSS_BALOWS_CHARA */
#define BOSS_INFO_SP_COOL 0x10          /* int: cleared whenever BOSS_STATE_BALOWS_RECOVERED (0x2) is set (= BOSS_INFO_BALOWS_10) */
#define BOSS_INFO_WAIT_TICKS BOSS_INFO_SHOWN_TICKS /* int 0x20: submode 2 lasts while below 0x24 */
#define BOSS_INFO_WAIT_MAX 0x24
#define BOSS_INFO_SIDE_TICKS BOSS_INFO_HAMM_28 /* int 0x28: submode 4's side-step lasts while below 0x38 */
#define BOSS_INFO_SIDE_BASE 0x2c        /* int: submode 4's length (+- 100 with BOSS_STATE_BIT_2000) */
#define BOSS_INFO_SIDE_MAX BOSS_INFO_HIDDEN_DELAY /* int 0x38 */
#define BOSS_INFO_CHAIN_PULL_ADD 0x68   /* int: added to BOSS_INFO_CHAIN_A_TIME when submode 5 ends (BOSS_STATE_PULL_DUE) */
#define BOSS_INFO_LAST_SHOUT 0xe4       /* short: the AXE's last submode-3 motion (it alternates) */
#define BOSS_INFO_NORMAL_KIND 0x14c     /* int: tiSetNormalMode's slot */
#define BOSS_INFO_CHAIN_KIND 0x150      /* int: tiSetChainMode's slot (with set A up) */
#define BOSS_INFO_SP_KIND 0x154         /* int: tiSelSPMotion's slot */
#define BOSS_STATE_SIDE_LEFT 0x0001     /* submode 4 steps toward 270 degrees (else 90) */
#define BOSS_STATE_PULL_DUE 0x0400      /* submode 5 started: its end adds BOSS_INFO_CHAIN_PULL_ADD */
#define BRAIN_BOSS_SUB1_TICKS BRAIN_HIDE_ATK_TIMER /* int 0x20c: submode 1 lasts while not below 0x214 */
#define BRAIN_BOSS_SUB1_MAX 0x214
#define BRAIN_BOSS_M_248 0x248          /* short: busy motions the approach waits out (meaning unread) */
#define BRAIN_BOSS_M_24A 0x24a
#define BRAIN_BOSS_SUB5_MOTION 0x24c    /* short: submode 5's motion (a voice, then search) */
#define BRAIN_BOSS_SUB3_MOTION_A 0x250  /* short: submode 3's two motions (a voice; = BRAIN_BALOWS_FOLLOW_MOTION) */
#define BRAIN_BOSS_SUB3_MOTION_B 0x252
#define BRAIN_BOSS_STEP_LEFT 0x260      /* short: submode 4's side-steps */
#define BRAIN_BOSS_STEP_RIGHT 0x262
#define BRAIN_BOSS_M_268 0x268
#define BRAIN_BOSS_M_26A 0x26a
#define BOSS_MOTION_APPROACH_29 0x29    /* busy too; VITRIOL in submode 4 plays it on TRIG_EVENT_BOSS_65 */
#define TRIG_EVENT_BOSS_65 0x65         /* global: removed every approach frame (its setter unread) */
#define BOSS_DEAD_FACE_DOT 0.9f         /* Alyssa down: turn until facing her this well, then motion 0 */
#define BOSS_SUB2_NEAR 100.0f           /* submode 2 ends nearer */
#define BOSS_SUB2_HIT_DAMAGE 0x14       /* VITRIOL's landed blow in submode 2 */
#define BOSS_ARROW_NEAR 120.0f          /* the arrow bits clear nearer; farther the approach angles off 45 degrees */
#define BOSS_AVOID_FAR 75.0f            /* the approach avoids obstacles only farther */
#define BOSS_ANGLE_45 0x1ffe            /* gaSetRotMatrixY units (GHOST_DEGREE 0xb6 a degree) */
#define BOSS_ANGLE_90 0x3ffc
#define BOSS_ANGLE_270 0xbff4
#define BOSS_ANGLE_315 0xdff2
#define BOSS_VOICE_AXE1_BANK "B_0A04"   /* 0x019341c0: the AXE without BOSS_STATE_BIT_04 */
#define BALOWS_VOICE_SHOUT 0x0d         /* BALOWS_VOICE_BANK "B_5A09" */
#define BALOWS_SE_SHOUT 0x1f            /* BALOWS_SE_BANK "E_BAL" */
void tiBrain_Approach_BOSS1(struct CHARACTER *chr);                                      // CT3 0x01870e00
