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
#define BRAIN_GHOST_W204 0x204          /* int: Init3 clears it (reader not read yet) */
#define BRAIN_GHOST_W220 0x220          /* int: Init3 sets 5 (reader not read yet) */
#define BRAIN_GHOST_CLATTER 0x274       /* int: the ghost's pad rattle slot (ControllerClatteringSet), -1 none */
#define BRAIN_GHOST_ALPHA_STEP 0x1b0    /* float: Search_Ghost2's fade-in a frame (ssBrain's BRAIN_APPROACH_NEAR) */
#define BRAIN_GHOST_ALPHA_MAX 0x1b4     /* float: the fade's ceiling (Init3: the ghost's alpha at start) */
#define GHOST_FADE_STEP 1.0f            /* the fade-out a frame (Trace_Ghost1, Return_Ghost1) */
#define GHOST_SEARCH_TIME_BASE 300      /* Init3's BRAIN_SEARCH_TIME_BASE */
#define GHOST_TRACE_RATE 100            /* Init3's BRAIN_TRACE_RATE */
#define GHOST_SE_BANK "Z_REI"           /* @<n> rodata 0x019342a8: the ghost's sound bank (Init3) */
#define GHOST_MOTION_RETURN2_ATTACK 1   /* ChgReturn2's motion index (into an attack) */
#define GHOST_MOTION_RETURN1_SEARCH 3   /* ChgReturn1's motion index (into a search) */

struct CHARA_JUNC;
// The ghost pole of junc (CHARACTER.junc: tiXcmd_SetGhostPole's) nearest to pos (a qword position).
int tiGetNearPole(const float *pos, const struct CHARA_JUNC *junc);                      // CT3 0x0187afd0
// 1 when the ghost leaves Alyssa alone (TRIG_EVENT_PANIC_OFF_63, no hp, or TRIG_EVENT_PANIC_ON_4D).
int tiCheckAlysStatusForGhost(void);                                                     // CT3 0x0187b0e0
