// game/tiBrain_ChgTrace_Ghost.h -- CT3 game/tiBrain_ChgTrace_Ghost.c: the declarations other TUs call
// (game/wideXcmd_TriggerInit.c first, 2026-11-12). Prototypes from the call sites; the TU that writes
// these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

struct CHARACTER;
// Brain initialisers: slots of the ssAI_BrainInit table (0x01938580), indexed by the brain's kind byte
// (+0x230); each reads the character's brain (CHARACTER.brain) itself (tiBrain_Init2: a0 only).
int tiBrain_Init1(struct CHARACTER *chr);                                                // CT3 0x0187aeb0
int tiBrain_Init2(struct CHARACTER *chr);                                                // CT3 0x0187ae40
int tiBrain_Init3(struct CHARACTER *chr);                                                // CT3 0x0187adb0

// The brain's mode functions (tiAvoidObject.c slice 3, 2026-11-12): entries of ssAI_charaMain's tables
// (ssAI_<Mode>Mode run the mode, ssAI_Chg<Mode>Mode pick the next; ssAI_DamageMode answers non-zero when
// the damage handling took the frame). Prototypes from that call site: chr is the character.
void tiBrain_ChgTrace_Ghost(struct CHARACTER *chr);                                      // CT3 0x01875b10
void tiBrain_ChgReturn2(struct CHARACTER *chr);                                          // CT3 0x01875b20
void tiBrain_ChgReturn1(struct CHARACTER *chr);                                          // CT3 0x01875b80
void tiBrain_ChgAttack_Ghost4(struct CHARACTER *chr);                                    // CT3 0x01875c20
void tiBrain_ChgAttack_Ghost3(struct CHARACTER *chr);                                    // CT3 0x01875f00
void tiBrain_ChgAttack_Ghost2(struct CHARACTER *chr);                                    // CT3 0x01875f60
void tiBrain_ChgAttack_Ghost1(struct CHARACTER *chr);                                    // CT3 0x01876190
void tiBrain_ChgApproach_Ghost2(struct CHARACTER *chr);                                  // CT3 0x01876400
void tiBrain_ChgApproach_Ghost1(struct CHARACTER *chr);                                  // CT3 0x01876670
void tiBrain_ChgSearch_Ghost3(struct CHARACTER *chr);                                    // CT3 0x018768f0
void tiBrain_ChgSearch_Ghost2(struct CHARACTER *chr);                                    // CT3 0x01876b50
void tiBrain_ChgSearch_Ghost1(struct CHARACTER *chr);                                    // CT3 0x01876d90
void tiBrain_Trace_Ghost1(struct CHARACTER *chr);                                        // CT3 0x01877050
int tiBrain_Damage3(struct CHARACTER *chr);                                              // CT3 0x018770e0
int tiBrain_Damage2(struct CHARACTER *chr);                                              // CT3 0x01877280
int tiBrain_Damage1(struct CHARACTER *chr);                                              // CT3 0x01877610
void tiBrain_Return_Ghost1(struct CHARACTER *chr);                                       // CT3 0x01877950
void tiBrain_Attack_Ghost4(struct CHARACTER *chr);                                       // CT3 0x01877a00
void tiBrain_Attack_Ghost3(struct CHARACTER *chr);                                       // CT3 0x01878280
void tiBrain_Attack_Ghost2(struct CHARACTER *chr);                                       // CT3 0x01878960
void tiBrain_Attack_Ghost1(struct CHARACTER *chr);                                       // CT3 0x01879200
void tiBrain_Approach_Ghost2(struct CHARACTER *chr);                                     // CT3 0x01879a80
void tiBrain_Approach_Ghost1(struct CHARACTER *chr);                                     // CT3 0x01879d70
void tiBrain_Search_Ghost3(struct CHARACTER *chr);                                       // CT3 0x0187a2b0
void tiBrain_Search_Ghost2(struct CHARACTER *chr);                                       // CT3 0x0187a760
void tiBrain_Search_Ghost1(struct CHARACTER *chr);                                       // CT3 0x0187a7b0

// ---- slice 1 (2026-11-13): the small mode functions, tiBrain_Init3, the helpers ----
// The ghost brain's own words (ssBrain_ChgHideAtk1.h names the shared ones; names from these uses only).
#define BRAIN_GHOST_TRACE_REQ 0x260     /* short: non-zero turns an attack (ChgAttack_Ghost3) to a trace; Init3: 0 */
#define BRAIN_GHOST_W220 0x220          /* int (its low byte read): the Damage<n> rate step (Init2 / Init3: 5) */
#define BRAIN_GHOST_CLATTER 0x274       /* int: the ghost's pad rattle slot (ControllerClatteringSet), -1 none */
#define BRAIN_GHOST_ALPHA_STEP 0x1b0    /* float: Search_Ghost2's fade-in a frame (ssBrain's BRAIN_APPROACH_NEAR) */
#define BRAIN_GHOST_ALPHA_MAX 0x1b4     /* float: the fade's ceiling (Init3: the ghost's alpha at start) */
#define GHOST_FADE_STEP 1.0f            /* the fade-out a frame (Trace_Ghost1, Return_Ghost1) */
#define GHOST_SEARCH_TIME_BASE 300      /* Init3's BRAIN_SEARCH_TIME_BASE */
#define GHOST_TRACE_RATE 100            /* Init3's BRAIN_TRACE_RATE */
#define GHOST_SE_BANK "Z_REI"           /* @<n> rodata 0x019342a8: the ghost's sound bank (Init3) */
#define GHOST_MOTION_RETURN2_ATTACK 1   /* ChgReturn2's motion index (into an attack) */
#define GHOST_MOTION_RETURN1_SEARCH 3   /* ChgReturn1's motion index (into a search) */

// ---- slice 2 (2026-11-13): the Chg* mode changes, Damage1..3, Init2 ----
// A ghost's rest after it gives up: no new approach until its ticks pass the limit (ChgSearch_Ghost1 / 3).
#define BRAIN_GHOST_REST 0x181          /* u_char: GHOST_REST_* (ssBrain's BRAIN_ATK_PICK; Init2: 0) */
#define GHOST_REST_NONE 0
#define GHOST_REST_GAVE_UP 1            /* set when an approach is dropped; counts BRAIN_GHOST_REST_TICKS */
#define GHOST_REST_2 2                  /* counts BRAIN_GHOST_REST2_TICKS (setter not read yet) */
#define BRAIN_GHOST_REST_TICKS 0x204    /* int: gNextTicksInterval summed while GAVE_UP (Init3 / the drops: 0) */
#define BRAIN_GHOST_REST_LIMIT 0x208    /* int */
#define BRAIN_GHOST_REST2_TICKS 0x20c   /* int (ssBrain's BRAIN_HIDE_ATK_TIMER) */
#define BRAIN_GHOST_REST2_LIMIT 0x214   /* int (ssBrain's BRAIN_ATK_PERIOD) */
#define BRAIN_GHOST_SE 0x278            /* int: the SE handle of GHOST_SE_LOOP (stopped by a damage) */
#define BRAIN_GHOST_FLAGS 0x183         /* u_char: GHOST_FLAG_* (ssBrain's BRAIN_ATK_EXTRA_RATE) */
#define GHOST_FLAG_HEIGHT 0x02          /* the ghost keeps Alyssa's height (Attack_Ghost4 rises / falls to it;
                                           Approach_Ghost2 holds its own), else it follows the floor (CheckLOS) */
#define GHOST_FLAG_PANIC_RATE 0x04     /* TRIG_EVENT_PANIC_3 speeds BRAIN_TRACE_RATE up by W220, else slows it */
#define GHOST_FLAG_FADE_FAR 0x08        /* farther than GHOST_FADE_FAR from Alyssa it fades a step a frame */
#define BRAIN_GHOST_HIT_FADE 0x1bc      /* float: Damage2 sets 1.0 while the damage plays; its fade a frame
                                           then (ssBrain's BRAIN_HEAR_RANGE) */
#define GHOST_RATE_MIN 100              /* BRAIN_TRACE_RATE's range under GHOST_FLAG_PANIC_RATE */
#define GHOST_RATE_MAX 200
#define GHOST_FADE_FAR 250.0f
#define GHOST_SE_NEAR 1000.0f           /* GHOST_SE_LOOP's volume: 127 at 0, 0 from here on */
#define GHOST_SE_VOLUME 127
// Motion table indices (CHARA_MOTION_SLOTS[0]) of the ghost kinds.
#define GHOST_MOTION_FLOAT 1            /* the loop the one-shot motions go back to */
#define GHOST_MOTION_SEARCH 3
#define GHOST_MOTION_FIND 4             /* ChgSearch_Ghost1: its event in submode 1 / 2 */
#define GHOST_MOTION_DAMAGE 9
#define GHOST_MOTION_VANISH 10          /* out of Alyssa's room: into a return */
#define GHOST_MOTION_APPEAR 0xe         /* ChgSearch_Ghost2: into an attack */
#define GHOST_MOTION_WITHDRAW 0xf       /* ChgAttack_Ghost2: back to a search */
// The two ghost characters and the global events that let each attack (names from these uses).
#define CHARA_ID_GHOST_1D 0x1d
#define CHARA_ID_GHOST_1E 0x1e
#define TRIG_EVENT_GHOST_1D_ON 0x55c
#define TRIG_EVENT_GHOST_1E_ON 0x55d
// The grab (Attack_Ghost4): the ghost's own words of its attack (ssBrain's BRAIN_ATK_STATES region).
#define BRAIN_GHOST_GRAB_TICKS 0x080    /* int: gNextTicksInterval summed while it holds Alyssa (0 at the grab) */
#define BRAIN_GHOST_GRAB_LIMIT 0x084    /* int: held longer -> it lets go */
#define BRAIN_GHOST_GRAB_RANGE 0x088    /* float: a lunge starts nearer than it (and within the angle) */
#define BRAIN_GHOST_GRAB_ANGLE 0x090    /* short: ssAI_IsTrgWithinAngle's */
#define BRAIN_GHOST_GRAB_PANIC 0x094    /* u_short: PanicSet's rise each held frame (and once at the bite) */
#define BRAIN_GHOST_LUNGE_CHANCE 0x09c  /* u_char: of 16, a lunge a frame (16 under the panic rate's event) */
#define BRAIN_GHOST_LUNGE_SPEED BRAIN_TRACE_SPEED /* 0x1f8 float: the lunge's step a frame toward her */
#define BRAIN_GHOST_HOLD_RANGE BRAIN_ATK_STAND /* 0x1c0 float: the lunge within it lands on her */
#define TRIG_EVENT_GHOST_GRAB 0x4c      /* held while the ghost holds Alyssa (name from this use) */
#define GHOST_GRAB_REACH 25.0f          /* the pre-attack grabs nearer than this ... */
#define GHOST_GRAB_HEIGHT 50.0f         /* ... and within this height of her */
#define GHOST_LUNGE_CHANCE_MAX 16
#define GHOST_MOTION_LUNGE 6            /* then GHOST_MOTION_LUNGE2, then the search motion */
#define GHOST_MOTION_GRAB 7             /* after GHOST_MOTION_GRAB_START */
#define GHOST_MOTION_LUNGE2 8           /* playing it: the lunge missed, the ghost rests */
#define GHOST_MOTION_GRAB_START 0xc
// asAlyssaLockedByAnything's modes for the ghost that bites (5..7 by character; meaning open, gh8).
#define GHOST_LOCKED_1D 5
#define GHOST_LOCKED_1E 6
#define GHOST_LOCKED_1F 7
#define CHARA_ID_GHOST_1F 0x1f
// Z_REI's sounds.
#define GHOST_SE_LOOP 0
#define GHOST_SE_CRY_A 3
#define GHOST_SE_CRY_B 4
#define GHOST_SE_APPEAR 6
#define GHOST_SE_LUNGE 1
#define GHOST_SE_GRAB 2

struct CHARA_JUNC;
// The ghost pole of junc (CHARACTER.junc: tiXcmd_SetGhostPole's) nearest to pos (a qword position).
int tiGetNearPole(const float *pos, const struct CHARA_JUNC *junc);                      // CT3 0x0187afd0
// 1 when the ghost leaves Alyssa alone (TRIG_EVENT_PANIC_OFF_63, no hp, or TRIG_EVENT_PANIC_ON_4D).
int tiCheckAlysStatusForGhost(void);                                                     // CT3 0x0187b0e0
