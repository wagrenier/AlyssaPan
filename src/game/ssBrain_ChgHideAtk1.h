// game/ssBrain_ChgHideAtk1.h -- CT3 game/ssBrain_ChgHideAtk1.c: the declarations other TUs call
// (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that writes
// these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <assert.h>
#include <stddef.h>

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
#define BRAIN_PLAYER_ROOM 0x038         /* u_int: the target's room when a mode was set (Set{Search,Attack,
                                           Approach}Mode); with the bank undrawn, a trace whose BRAIN_GOAL_ROOM
                                           differs turns to search */
// The brain's attack states (wideXcmd_TriggerInit.c BRAIN_STATES: BrainSet copies them in): a stalker's
// strikes, each with a weight per situation (the weights of one column add up to AI_ATK_WEIGHT; the set
// picks state i with weight[i] / the weights left) and the reach ChgSearch1 strikes from a search with.
// Slice 2 (2026-11-12): the fields the setters and ChgSearch1 read; 0x00..0x07, 0x14, 0x15, 0x18, 0x1e,
// 0x1f not read here.
typedef struct BRAIN_ATK_STATE {
    u_char b00[8];
    float reach;                        /* 0x08: ChgSearch1 strikes from a search within it */
    float reachPanic;                   /* 0x0c: the same with TRIG_EVENT_PANIC_3 */
    short angle;                        /* 0x10: ssAI_IsTrgWithinAngle's angle for reach */
    short anglePanic;                   /* 0x12: for reachPanic */
    u_char b14[2];
    u_char wDown;                       /* 0x16: weight when the target bends or is down (from a search) */
    u_char fromSearch;                  /* 0x17: non-zero: ChgSearch1 may strike with it (the first one) */
    u_char b18;
    u_char wFar;                        /* 0x19: from a search, farther than BRAIN_ATK_NEAR */
    u_char wPanic;                      /* 0x1a: from a search, near, TRIG_EVENT_PANIC_3 */
    u_char wWide;                       /* 0x1b: from a search, near, in a wide room (AI_ATK_WIDE_ROOM) */
    u_char wNear;                       /* 0x1c: from a search, near, a small room; ssAI_SetHideAtkMode's */
    u_char motion;                      /* 0x1d: its motion (skipped while chr waits in BRAIN_END_MOTION) */
    u_char b1e[2];
} BRAIN_ATK_STATE;
static_assert(sizeof(BRAIN_ATK_STATE) == 0x20, "BRAIN_ATK_STATE");
static_assert(offsetof(BRAIN_ATK_STATE, wDown) == 0x16 && offsetof(BRAIN_ATK_STATE, motion) == 0x1d,
              "BRAIN_ATK_STATE fields");
#define BRAIN_ATK_STATES 0x080          /* BRAIN_ATK_STATE[BRAIN_ATK_COUNT] */
#define BRAIN_ATK_COUNT 0x180           /* u_char */
#define BRAIN_ATK_PICK 0x181            /* u_char: the state the setters picked, BRAIN_ATK_NONE */
#define BRAIN_ATK_NONE 0xff
#define BRAIN_ATK_STEP 0x182            /* u_char: cleared with a pick (read by the attack mode) */
#define AI_ATK_WEIGHT 16                /* every weight column's total */
#define AI_ATK_WIDE_ROOM 36000.0f       /* HIT_SCENE_GRID cellW * cellD above it: a wide room */
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
#define SET_HIDE_ATK_POLE 0             /* ssAI_SetHideAtkMode: pick a hide pole */
#define SET_HIDE_ATK_PICK 1             /* pick an attack state by wNear */

// ---- slice 2 (2026-11-12): the setters ssAI_Set{Trace,Attack,Approach,Search}Mode ----
#define BRAIN_SUBMODE_ROUTE 0           /* following a group route: BRAIN_TARG_POLE is the next group link */
#define BRAIN_SUBMODE_DIRECT BRAIN_SUBMODE_HIDE_POLE /* 3: straight to the point (same group; no route) */
#define BRAIN_GOAL_GROUP 0x020          /* short: the trace target's hide-pole group (CHARACTER.h100) */
#define BRAIN_GOAL_BANK 0x022           /* u_char: the trace target's light bank */
#define BRAIN_SHADOW_POL 0x030          /* s_char: the shadow polygon a stalker with no room comes out of
                                           (wideXcmd_TriggerInit.c BRAIN_ROOM_ARG) */
#define BRAIN_PLAYER_GROUP 0x032        /* short: the approach target's CHARACTER.h100 */
#define BRAIN_PLAYER_BANK 0x034         /* u_char: the target's light bank when a mode was set */
#define BRAIN_SEARCH_POLE BRAIN_LINK_POLE /* 0x03e: the pole a search goes to */
#define BRAIN_SEARCH_GROUP 0x040        /* short: its group */
#define BRAIN_SEARCH_BANK 0x042         /* u_char: chr's light bank then */
#define BRAIN_SEARCH_ROOM 0x044         /* u_int: chr's room then */
#define BRAIN_APPROACH_NEAR 0x1b0       /* float: search -> approach within it starts BRAIN_APPROACH_NEAR_MOTION */
#define BRAIN_HEAR_RANGE 0x1bc          /* float: ChgSearch1 hears within it (x AI_HEAR_FAR: a turn only) */
#define BRAIN_ATK_NEAR 0x1c4            /* float: SetAttackMode from a search: farther -> wFar */
#define BRAIN_SEARCH_TIME 0x1d4         /* int: the search's time (BRAIN_SEARCH_TIME_BASE scaled by the room) */
#define BRAIN_SEARCH_TIME2 0x1d8        /* int: a second time scaled the same way (its use: Search1) */
#define BRAIN_SEARCH_TIME_BASE 0x1dc    /* int */
#define BRAIN_SEARCH_TIME2_BASE 0x1e0   /* int */
#define BRAIN_SEARCH_START 0x1e4        /* int: BRAIN_TICKS when the search began */
#define BRAIN_APPROACH_SPEED 0x1ec      /* float: BRAIN_APPROACH_TIMER = distance / (it / AI_SPEED_DIV) */
#define BRAIN_APPROACH_START 0x1f0      /* int: BRAIN_TICKS when the approach was planned */
#define BRAIN_TRACE_SPEED 0x1f8         /* float: BRAIN_TRACE_TIMER = distance / (it / AI_SPEED_DIV) */
#define BRAIN_TRACE_START 0x1fc         /* int: BRAIN_TICKS when the trace's timer was set */
#define BRAIN_START_MOTION 0x23e        /* short: the brain's first motion (wideXcmd_TriggerInit.c BRAIN_MOTION) */
#define BRAIN_APPROACH_NEAR_MOTION 0x250 /* short: search -> approach within BRAIN_APPROACH_NEAR (then
                                           BRAIN_APPROACH_MOTION); farther it is BRAIN_HIDE_ATK_MOTION */
#define AI_SPEED_DIV 5.0f
#define AI_SEARCH_ROOM_BASE 600.0f      /* SetSearchMode: the times scale by ((cellW + cellD) - it) / */
#define AI_SEARCH_ROOM_SCALE 800.0f     /*   it + 1 */
#define SEARCH_POLE_FLAG 0x40           /* SetSearchMode: the hide-pole flag of the poles a search visits */
// ssBrain_ChgSearch1.
#define AI_SEARCH_STRIKE_RATE 10        /* per cent a frame: a strike from the search is considered */
#define AI_MOTION_KIND2_LOUD 2          /* Alyssa's motion record MOTION_REC_KIND2 a search hears (with
                                           TRIG_EVENT_AI_D2; the kind's meaning is a guess: open-items) */
#define AI_NOISE_LOUD 2                 /* ssAI_CheckHearingRate's noise: a search within BRAIN_HEAR_RANGE */
#define AI_NOISE_SOFT 1                 /* a search within BRAIN_HEAR_RANGE * AI_HEAR_FAR; a trace (Damage1)
                                           within BRAIN_HEAR_RANGE */
#define AI_HEAR_FAR 3.0f
// ssBrain_Damage1.
#define BRAIN_STRUCK 0x003              /* u_char: 1 after Alyssa's AI_ALYSSA_STRIKE hit it; the first frame
                                           with no hit registers TRIG_EVENT_PANIC_STARTED and clears it */
#define BRAIN_PLAYER_SHADOW 0x03c       /* s_char: the target's CHARACTER.b102 when noted (a shadow polygon
                                           id, -1 none): where a warped target is followed from */
#define BRAIN_DAMAGE_MOTION 0x264       /* short: played when Alyssa's strike lands (then the mode's motion) */
#define AI_FELL_Y (-30000.0f)           /* below it the stalker fell out of the world: the brain ends */
#define AI_ALYSSA_STRIKE 0x51           /* HIT_REC.attack of Alyssa's (who 0) strike that staggers a stalker */
// SetAttackMode's how.
#define SET_ATK_QUEUED (-1)             /* the state whose motion chr has queued (else BRAIN_START_MOTION) */
#define SET_ATK_FRESH 0                 /* no state */
#define SET_ATK_SEARCH 1                /* by the situation's weight (from a search) */

// The modes' setters (-1 when the mode could not be planned). chara is a Character[] index.
int ssAI_SetHideAtkMode(struct CHARACTER *chr, int what);                                // CT3 0x01880e10
int ssAI_SetAttackMode(struct CHARACTER *chr, int chara, int how);                       // CT3 0x01881280
int ssAI_SetApproachMode(struct CHARACTER *chr, int chara);                              // CT3 0x01881b10
// keepPole: go on to the BRAIN_SEARCH_POLE already chosen (no new pole, the times kept).
int ssAI_SetSearchMode(struct CHARACTER *chr, int keepPole);                             // CT3 0x01881f90
// dist: chr's distance to Alyssa (ChgSearch1 passes it in f12).
int ssAI_SearchToApproachMode(float dist, struct CHARACTER *chr);                        // CT3 0x018827d0
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
