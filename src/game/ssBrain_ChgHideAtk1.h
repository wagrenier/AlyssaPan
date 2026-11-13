// game/ssBrain_ChgHideAtk1.h -- CT3 game/ssBrain_ChgHideAtk1.c: the declarations other TUs call
// (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that writes
// these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
// Brain initialiser: slot 0 of the ssAI_BrainInit table (0x01938580).
int ssBrain_Init1(struct CHARACTER *chr);                                                // CT3 0x01880b60

// The brain's mode functions (tiAvoidObject.c slice 3, 2026-11-12): entries of ssAI_charaMain's tables
// (ssAI_<Mode>Mode run the mode, ssAI_Chg<Mode>Mode pick the next; ssAI_DamageMode answers non-zero when
// the damage handling took the frame). Prototypes from that call site: chr is the character.
void ssBrain_ChgHideAtk1(struct CHARACTER *chr);                                         // CT3 0x0187b160
void ssBrain_ChgReturn1(struct CHARACTER *chr);                                          // CT3 0x0187b260
void ssBrain_ChgAttack1(struct CHARACTER *chr);                                          // CT3 0x0187b270
void ssBrain_ChgTrace1(struct CHARACTER *chr);                                           // CT3 0x0187b640
void ssBrain_ChgApproach1(struct CHARACTER *chr);                                        // CT3 0x0187ba60
void ssBrain_ChgSearch1(struct CHARACTER *chr);                                          // CT3 0x0187be60
int ssBrain_Damage1(struct CHARACTER *chr);                                              // CT3 0x0187c4a0
void ssBrain_HideAtk1(struct CHARACTER *chr);                                            // CT3 0x0187d310
void ssBrain_Return1(struct CHARACTER *chr);                                             // CT3 0x0187d6e0
void ssBrain_Attack1(struct CHARACTER *chr);                                             // CT3 0x0187d770
void ssBrain_Trace1(struct CHARACTER *chr);                                              // CT3 0x0187e3d0
void ssBrain_Approach1(struct CHARACTER *chr);                                           // CT3 0x0187f000
void ssBrain_Search1(struct CHARACTER *chr);                                             // CT3 0x0187fbe0
// Plans a trace of target (Character[Player_BankID] from the mode changes; ssAI_SetMonsterPos passes NULL):
// -1 when no trace could be planned (slice 1 of this TU: the argument is a CHARACTER address in the ROM).
int ssAI_SetTraceMode(struct CHARACTER *chr, struct CHARACTER *target);                  // CT3 0x018817b0

// ---- slice 1 (2026-11-12): the mode changes (ssBrain_Chg*1), the transitions, ssAI_SetHideAtkMode ----
// Brain words this slice reads (names from these uses; tiAvoidObject.h has the rest).
#define BRAIN_SHOWN 0x002              /* u_char: the stalker is shown (ssBrain_Return1 hides it: ssAI_MonsterDraw 0) */
#define BRAIN_TRACE_POINT 0x010         /* float[4]: where a trace ends (ssAI_IsGetToPoint in ChgTrace1) */
#define BRAIN_TRACE_ROOM 0x038          /* u_int: the room a trace ends in; with the bank undrawn, a trace whose
                                           BRAIN_GOAL_ROOM differs turns to search (guess from that one test) */
#define BRAIN_HIDE_ATK 0x09c            /* HIDE_ATK records of BRAIN_HIDE_ATK_SIZE: +0 weight (of
                                           AI_HIDE_ATK_WEIGHT), +1 the motion they skip while waiting in it */
#define BRAIN_HIDE_ATK_SIZE 0x20
#define BRAIN_HIDE_ATK_COUNT 0x180      /* u_char: HIDE_ATK records */
#define BRAIN_HIDE_ATK_PICK 0x181       /* u_char: the record ssAI_SetHideAtkMode picked, BRAIN_HIDE_ATK_NONE */
#define BRAIN_HIDE_ATK_NONE 0xff
#define BRAIN_ATTACK_ON 0x184           /* u_char: cleared with TRIG_EVENT_AI_ATTACK when the attack ends */
#define BRAIN_ATTACK_RANGE 0x1a4        /* float: approach -> attack within it (|dy| < AI_ATTACK_HEIGHT too) */
#define BRAIN_ATTACK_LEAVE 0x1a8        /* float: attack -> approach beyond it */
#define BRAIN_SIGHT 0x1ac               /* float: ssAI_SerchCheck's distance (+- AI_SIGHT_MARGIN) */
#define BRAIN_LOSE_RANGE 0x1b8          /* float: an approach beyond it checks it still sees Alyssa */
#define BRAIN_FOV 0x1c8                 /* int: ssAI_SerchCheck's angle (rot units; +- AI_FOV_MARGIN) */
#define BRAIN_APPROACH_TIMER 0x1e8      /* int: cleared on TRIG_EVENT_AI_APPEAR_NOW in an approach */
#define BRAIN_TRACE_TIMER 0x1f4         /* int: cleared on TRIG_EVENT_AI_APPEAR_NOW in a trace */
#define BRAIN_TRACE_PLAN_TICKS 0x200    /* int: BRAIN_TICKS when the trace was last re-planned */
#define BRAIN_HIDE_ATK_TIMER 0x20c      /* int: cleared when a hide attack starts */
#define BRAIN_ATTACK_RATE 0x22e         /* u_char: the attack motion's play rate */
#define BRAIN_TRACE_RATE 0x22f          /* u_char: the trace motion's play rate */
#define BRAIN_SEARCH_MOTION 0x240       /* short: the search's motion (after BRAIN_TRACE_TO_SEARCH_MOTION) */
#define BRAIN_APPROACH_MOTION 0x242     /* short */
#define BRAIN_TRACE_MOTION 0x244        /* short */
#define BRAIN_ATTACK_MOTION 0x246       /* short */
#define BRAIN_HIDE_ATK_MOTION 0x252     /* short: into a hide attack (then BRAIN_ATTACK_MOTION) */
#define BRAIN_FIND_MOTION 0x254         /* short: search -> trace when found (then BRAIN_TRACE_MOTION) */
#define BRAIN_RETURN_MOTION 0x256       /* short: search -> return */
#define BRAIN_TRACE_TO_SEARCH_MOTION 0x25a /* short */
#define BRAIN_SUBMODE_1 1               /* BRAIN_SUBMODE: with SCN_LINK, no sight checks (meaning open) */
#define BRAIN_SUBMODE_HIDE_POLE 3       /* ssAI_SetHideAtkMode: going to its hide pole (BRAIN_LINK_POLE) */
#define TRIG_EVENT_AI_ATTACK 0xd6       /* removed when an attack ends (who sets it: the attack TU) */
#define AI_TRACE_REPLAN_TICKS 1500      /* a trace re-plans after more than 1499 ticks (5 units of 300) */
#define AI_ATTACK_HEIGHT 100.0f         /* |dy| between chr and Alyssa an attack allows */
#define AI_SIGHT_MARGIN 100.0f
#define AI_FOV_MARGIN 0x71c             /* 10 degrees in rot units */
#define AI_TRACE_ARRIVE 120.0f          /* ChgTrace1: at the trace's end (Alyssa not hiding) */
#define AI_TRACE_ARRIVE_HIDING 400.0f   /* the same while she hides */
#define AI_HIDE_ATK_WEIGHT 16           /* the HIDE_ATK weights' total */
#define SET_HIDE_ATK_POLE 0             /* ssAI_SetHideAtkMode: pick a hide pole */
#define SET_HIDE_ATK_PICK 1             /* pick a HIDE_ATK record */

// The modes' setters (-1 when the mode could not be planned). chara is a Character[] index.
int ssAI_SetHideAtkMode(struct CHARACTER *chr, int what);                                // CT3 0x01880e10
int ssAI_SetAttackMode(struct CHARACTER *chr, int chara, int fromSearch);                // CT3 0x01881280
int ssAI_SetApproachMode(struct CHARACTER *chr, int chara);                              // CT3 0x01881b10
int ssAI_SetSearchMode(struct CHARACTER *chr, int a1);                                   // CT3 0x01881f90
// The transitions: 0, or -1 when the next mode could not be set (the caller then ends the brain).
int ssAI_ApproachToHideAtkMode(struct CHARACTER *chr);                                   // CT3 0x01882290
int ssAI_AttackToHideAtkMode(struct CHARACTER *chr);                                     // CT3 0x018822a0
int ssAI_HideAtkToAttackMode(struct CHARACTER *chr);                                     // CT3 0x018823b0
int ssAI_HideAtkToTraceMode(struct CHARACTER *chr);                                      // CT3 0x018823f0
int ssAI_AttackToTraceMode(struct CHARACTER *chr);                                       // CT3 0x01882400
int ssAI_AttackToApproachMode(struct CHARACTER *chr);                                    // CT3 0x01882440
int ssAI_TraceToApproachMode(struct CHARACTER *chr);                                     // CT3 0x018824c0
int ssAI_TraceToSearchMode(struct CHARACTER *chr);                                       // CT3 0x01882520
int ssAI_ApproachToAttackMode(struct CHARACTER *chr);                                    // CT3 0x01882590
int ssAI_ApproachToTraceMode(struct CHARACTER *chr);                                     // CT3 0x01882600
int ssAI_SearchToReturnMode(struct CHARACTER *chr);                                      // CT3 0x01882690
int ssAI_SearchToTraceMode(struct CHARACTER *chr, int found);                            // CT3 0x018826d0
int ssAI_SearchToAttackMode(struct CHARACTER *chr);                                      // CT3 0x01882790
