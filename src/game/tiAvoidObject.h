// game/tiAvoidObject.h -- CT3 game/tiAvoidObject.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <assert.h>
#include <stddef.h>

// ---- slice 1 (2026-11-12): the timers, the save image's part, the stalkers' kinds and motions ----
// The timers count gNextTicksInterval ticks; the setters take units of 300 ticks (the scripts' numbers:
// the global init's first wait is 0x78 of them). Which unit that is in seconds is open (open-items.md).
#define AI_TICKS_PER_UNIT 300
#define AI_FIRST_APPEARANCE 0x78        /* ssAI_GlobalInit: the first wait */
#define AI_REAPPEARANCE 0x3c            /* ssAI_CountAppearanceTimer: the wait after an appearance */
extern int ssNoReactionTimer;           /* sbss 0x0193ce90 (symtab local) */
extern int AppearanceTimer;             /* sbss 0x0193ce94 (symtab local) */
// The save image's words (Character_Init.h's MC image: inside pade5cc).
#define MC_DATA_AI_APPEARANCE_TIMER 0xe5cc
#define MC_DATA_AI_NO_REACTION_TIMER 0xe5d0
// Trigger events of the AI (ssTrigCheckEvent / _Register / _Remove, global scope).
#define TRIG_EVENT_AI_APPEAR 0xc8       /* set when AppearanceTimer runs out: a stalker may come; GlobalInit clears it */
#define TRIG_EVENT_AI_1D 0xcd           /* removed when an AI_ID_1D brain ends (set by its brain set: guess) */
#define TRIG_EVENT_AI_1E 0xce
#define TRIG_EVENT_AI_1F 0xcf
#define TRIG_EVENT_AI_APPEAR_NOW 0xd1   /* with the no-reaction time out, zeroes AppearanceTimer; ssAI_main
                                           removes it (and 0xd2, 0xd3) every frame */
// CHARACTER.id of the AI's kinds: 2..6 are the stalkers (the chase BGM's, TRIG_EVENT_CHASE); 0x1d..0x1f
// have an event each. Names from these uses; the brain TUs say who they are.
#define STALKER_ID_2 0x02
#define STALKER_ID_3 0x03
#define STALKER_ID_4 0x04
#define STALKER_ID_5 0x05
#define STALKER_ID_6 0x06
#define AI_ID_1D 0x1d
#define AI_ID_1E 0x1e
#define AI_ID_1F 0x1f
// ssAI_MemLoadInit parks a stalker: pos and posBefore (1e7, -1e7, 0), h100 -2, this flag set.
#define AI_PARK_X 10000000.0f           /* 0x4b189680 */
#define AI_PARK_Y (-10000000.0f)        /* 0xcb189680 */
#define AI_H100_NONE (-2)               /* CHARACTER.h100 of a parked stalker (ssAI_main: -2 -> -3) */
#define CHARACTER_FLAG_AI_PARKED 0x02000000u /* name from MemLoadInit / BrainEnd, which set it */
// Motion table indices (CHARA_MOTION_SLOTS[0]) the predicates test.
#define MOTION_HAIHAI_C0 0xc0           /* Alyssa's crawl motions (ssAI_IsHaiHaiPlaying; with wideXcmd_DebugPrint.h's MOTION_HAIHAI_STEP / _STEP2) */
#define MOTION_HAIHAI_B6 0xb6
#define MOTION_HAIHAI_11 0x11
#define MOTION_HAIHAI_10 0x10
#define MOTION_DOOR_PUSH_ALYSSA 0x3c
#define MOTION_DOOR_PULL_ALYSSA 0x3f
#define MOTION_DOOR_PUSH_2 0x11
#define MOTION_DOOR_PULL_2 0x12
#define MOTION_DOOR_PUSH_346 0x20
#define MOTION_DOOR_PULL_346 0x21
#define MOTION_DOOR_PUSH_5 0x1e
#define MOTION_DOOR_PULL_5 0x1f

void ssAI_main(void);                                                                    // CT3 0x01863ed0
void ssAI_GlobalInit(void);                                                              // CT3 0x01866490
void ssAI_SceneInit(int scene);                                                          // CT3 0x018660a0
// The AI's part of a save image (Character_Init.c's data_save_set / data_load_set pass the image).
void ssAI_MemSaveData(u_char *data);                                                     // CT3 0x018662a0
void ssAI_MemLoadInit(u_char *data);                                                     // CT3 0x018662d0
struct CHARACTER;
// Ends chr's brain: chr goes into motion and is parked out of the world, hidden, its brain freed.
void ssAI_BrainEnd(struct CHARACTER *chr, int motion);                                   // CT3 0x01865e20
// 1 while the character plays a door push or pull opening (ssAI_IsDoorPushOpenPlaying / _Pull).
int ssAI_IsDoorOpenPlaying(struct CHARACTER *chr);                                       // CT3 0x01860d00
int ssAI_IsDoorPullOpenPlaying(struct CHARACTER *chr);                                   // CT3 0x01860d40
int ssAI_IsDoorPushOpenPlaying(struct CHARACTER *chr);                                   // CT3 0x01860e40
void ssAI_ForceBrainEnd(struct CHARACTER *chr, int motion);                              // CT3 0x01865df0
void ssAI_CharaLoadInit(void);                                                           // CT3 0x01866090
void ssAI_CountAppearanceTimer(void);                                                    // CT3 0x01866530
// From tkXDTrg_MonDraw (tkXDTrg_GetSceneHash.c, 2026-11-11): shows (1) or hides (0) a stalker.
void ssAI_MonsterDraw(struct CHARACTER *chr, int draw);                                  // CT3 0x0185f710
// From asArrowOperate (asUseArrow.c, 2026-11-11): on stage 3, an arrow first pointing up.
void ssAI_AxeThrowReset(void);                                                           // CT3 0x018642b0
// From ssCollisionCheck / ssColCheckChar_Chara (ssHitCheckAttackToScn.c, 2026-11-12): 1 while Alyssa
// (id 0) crawls (her motion short at +0x398 is one of the crawl motions; read from the ROM, not written).
int ssAI_IsHaiHaiPlaying(struct CHARACTER *chr);                                         // CT3 0x01860860
// From ssXcmd_SetNoReactionTimer / _SetAppearanceTimer / _SetChaseFlagAndTimer (wideXcmd_TriggerInit.c,
// 2026-11-12): the script's number passed on (the chase flag's timers are 4..10).
void ssAI_SetNoReactionTimer(int time);                                                  // CT3 0x018664f0
void ssAI_SetAppearanceTimer(int time);                                                  // CT3 0x01866510
// From test_ssXcmd_BrainSet4 (wideXcmd_TriggerInit.c, 2026-11-12). BrainFree is _SunFree(&gHeapJunc,
// &chr->brain) in the ROM: on the host it frees CharaBrainHost[slot] and sets the handle to 0 (CharaBrainSet).
void ssAI_BrainFree(struct CHARACTER *chr);                                              // CT3 0x01863c20
// The first bank of BankInfo in state != 0, type 1, whose GetRoomName_file is roomHash (the low word of a
// room hash, zero-extended); -1 for none.
int ssAI_GetScnBankID(u_int roomHash);                                                   // CT3 0x01862770

// ---- slice 2 (2026-11-12): the hide poles and the scene init ----
// A scene bank's hide poles (ALL_TABLE.hidePoles -> HIDE_POLE_LIST; layout: ee, bank data): the places a
// stalker hides at and comes out of. Each pole is a point, or a segment (HIDE_POLE_FLAG_SEGMENT) whose
// middle is its position; ssAI_TransAllPoleData moves both ends into the world with the part's matrix.
typedef struct HIDE_POLE {
    float localA[4];                    /* 0x00: end A in the part's space */
    float localB[4];                    /* 0x10: end B */
    float posA[4];                      /* 0x20: end A in the world (ssAI_TransAllPoleData) */
    float posB[4];                      /* 0x30: end B in the world */
    u_int room;                         /* 0x40: the room the pole leads to (a GetRoomName_file word:
                                           ssAI_GetPoleScnName, ssAI_GetPoleScnBnkID) */
    short pair;                         /* 0x44: the pole at the other end of a scene link */
    short group;                        /* 0x46: ssAI_GetPoleGroupID */
    u_char flags;                       /* 0x48: HIDE_POLE_FLAG_* */
    u_char shadowPol;                   /* 0x49: the shadow polygon's id (HIDE_POLE_FLAG_SHADOW) */
    short parts;                        /* 0x4a: its model part (the matrix); SetHideAttackPole's number */
    u_char pad4c[4];
} HIDE_POLE;
static_assert(sizeof(HIDE_POLE) == 0x50, "HIDE_POLE");
static_assert(offsetof(HIDE_POLE, room) == 0x40, "HIDE_POLE.room");
static_assert(offsetof(HIDE_POLE, flags) == 0x48, "HIDE_POLE.flags");
static_assert(offsetof(HIDE_POLE, parts) == 0x4a, "HIDE_POLE.parts");
typedef struct HIDE_POLE_LIST {
    u_int poles;                        /* 0x0: HIDE_POLE[count] (EE address in the bank) */
    int count;                          /* 0x4 */
    u_int groups;                       /* 0x8: HIDE_POLE_GROUP[groupCount] (slice 4, ssAI_IsNextGroup) */
    int groupCount;                     /* 0xc: also the route ring's length */
    u_int route;                        /* 0x10: short[groupCount]: the route ring (EE address; ssAI_GRS_*) */
    short routeHead;                    /* 0x14: the ring's front (PopFront / PushFront) */
    short routeTail;                    /* 0x16: one past its back (PushRear); head == tail: empty */
} HIDE_POLE_LIST;
static_assert(sizeof(HIDE_POLE_LIST) == 0x18, "HIDE_POLE_LIST");
static_assert(offsetof(HIDE_POLE_LIST, routeTail) == 0x16, "HIDE_POLE_LIST.routeTail");
// A group of hide poles and the groups it links to (ssAI_IsNextGroup; layout: ee, bank data). visited,
// prev and cost are the route search's work fields (ssAI_GRS_InitParam clears them; slice 6).
typedef struct HIDE_POLE_GROUP {
    u_int links;                        /* 0x0: short[linkCount]: indices of linked records (EE address) */
    u_char linkCount;                   /* 0x4 */
    u_char visited;                     /* 0x5: 1 while the search recurses through this group */
    u_char pad06[2];
    short id;                           /* 0x8: the group id (HIDE_POLE.group) */
    short prev;                         /* 0xa: the record the cheapest route came from; -1 the start */
    u_int linkDist;                     /* 0xc: float[linkCount][linkCount] (EE address): [in][out] the
                                           distance between the link poles to links[in] and links[out] */
    float cost;                         /* 0x10: the cheapest route's length so far; 0 = none yet */
} HIDE_POLE_GROUP;
static_assert(sizeof(HIDE_POLE_GROUP) == 0x14, "HIDE_POLE_GROUP");
static_assert(offsetof(HIDE_POLE_GROUP, id) == 0x8, "HIDE_POLE_GROUP.id");
static_assert(offsetof(HIDE_POLE_GROUP, cost) == 0x10, "HIDE_POLE_GROUP.cost");
#define HIDE_POLE_FLAG_ATTACK 0x01      /* SetHideAttackPole: a stalker may strike from this pole */
#define HIDE_POLE_FLAG_SEGMENT 0x02     /* the position is the middle of A and B (ssAI_GetPolePos) */
#define HIDE_POLE_FLAG_SHADOW 0x10      /* a shadow polygon's pole: shadowPol is valid */
#define HIDE_POLE_FLAG_SCN_LINK 0x20    /* a scene link: pair is valid (ssAI_GetPairScnLinkPoleID) */
#define AI_BANK_NONE 0xfe               /* ssAI_IsGetShadowPol: a bank at or above it has no poles */
// MODEL_PARTS +0xcc (the w of initRot's first row): the EE address of the matrix a group portal's part
// moves its quad with (ssAI_SceneInit). The ROM reads it as a float word; name from that one use.
#define PARTS_PORTAL_MATRIX 0xcc

void ssAI_TransAllPoleData(int bank);                                                    // CT3 0x018641e0
void ssAI_GetPolePos(float *pos, int pole, int bank);                                    // CT3 0x01863010
short ssAI_GetPoleGroupID(int pole, int bank);                                           // CT3 0x01862f20
u_int ssAI_GetPoleScnName(int pole, int bank);                                           // CT3 0x01862830
s_char ssAI_GetPoleScnBnkID(int pole, int bank);                                         // CT3 0x01862860
short ssAI_GetShadowPolID(s_char shadowPol, int bank);                                   // CT3 0x018628b0
short ssAI_GetPairScnLinkPoleID(short pole, int bank);                                   // CT3 0x01862c40
int ssAI_CheckPoleFlg(short pole, int bank, u_char flag);                                // CT3 0x018600a0
int ssAI_IsGetShadowPol(u_char shadowPol, int bank);                                     // CT3 0x0185ff40

// ---- slice 3 (2026-11-12): the frame's driver, the mode dispatch, the facing, the panic, the predicates ----
// The brain's modes (BRAIN_MODE, its first byte): the symtab names each by its table (ssAI_<Mode>Mode,
// ssAI_Chg<Mode>Mode). 0xff = halted (ssAI_main, when Alyssa is down). wideXcmd_TriggerInit.c's AI_MODE_*
// are the same values under older names (AI_MODE_CHASE = SS_AI_APPROACH, AI_MODE_NO_CHASE = SS_AI_RETURN).
enum SS_AI_MODE {
    SS_AI_SEARCH = 0,
    SS_AI_APPROACH = 1,
    SS_AI_TRACE = 2,
    SS_AI_ATTACK = 3,
    SS_AI_RETURN = 4,
    SS_AI_HIDE_ATK = 5,
    SS_AI_MODE_COUNT = 6,
    SS_AI_HALTED = 0xff,
};
// The brain's fields this TU reads (the 0x280-byte brain of test_ssXcmd_BrainSet4; names from these uses).
#define BRAIN_MODE 0x000                /* u_char: SS_AI_* */
#define BRAIN_STOP_MOTION 0x004         /* short: -1 while the brain runs; while TRG_CHAR_STOP_CHAR holds the
                                           characters, the motion it was at when they stopped */
#define BRAIN_PANIC_NEAR 0x188          /* float: within it the distance rise is BRAIN_PANIC_RISE_NEAR */
#define BRAIN_PANIC_MID 0x18c           /* float: within it BRAIN_PANIC_RISE_MID */
#define BRAIN_PANIC_FAR 0x190           /* float: within it BRAIN_PANIC_RISE_FAR; beyond it no rise */
#define BRAIN_PANIC_RISE_NEAR 0x194     /* float */
#define BRAIN_PANIC_RISE_MID 0x198      /* float */
#define BRAIN_PANIC_RISE_FAR 0x19c      /* float */
#define BRAIN_PANIC_SCALE 0x1a0         /* float: ssAI_PanicSet multiplies every rise by it */
#define BRAIN_HEAR_RATE 0x1cc           /* u_char: per cent per unit of noise (ssAI_CheckHearingRate) */
#define BRAIN_HEAR_RATE_INVISIBLE 0x1cd /* u_char: the same while TRIG_EVENT_INVISIBLE */
#define BRAIN_TICKS 0x1d0               /* int: gNextTicksInterval summed while the brain runs */
#define BRAIN_NO_REACTION_TIME 0x21c  /* int: ssAI_BrainEnd sets the no-reaction timer from it */
#define BRAIN_DAMAGE_FN 0x231           /* u_char: the ssAI_DamageMode entry; 0x232..0x237 the entries of
                                           the six modes (BRAIN_MODE_FN + mode) */
#define BRAIN_MODE_FN 0x232
#define BRAIN_CHG_MODE_FN 0x238         /* u_char[6]: the ssAI_Chg<Mode>Mode entry of each mode */
#define BRAIN_END_MOTION 0x24e          /* short: the motion after the halt (main2.c's BRAIN_ID: ssAI_BrainEnd's
                                           second argument, ssAI_ForceBrainEnd's motion) */
#define BRAIN_TURN_MOTION_BACK 0x268    /* short: the turn motion for a turn of a half to three quarters */
#define BRAIN_TURN_MOTION 0x26a         /* short: the turn motion for a quarter to a half turn */
#define BRAIN_HALT_MOTION 0x26c         /* short: the motion a halted stalker plays (ssAI_main) */
// CHARACTER.h100 of a stalker that left: AI_H100_NONE (-2) becomes this, and the next frame drops its light
// bank, room and shadow flags.
#define AI_H100_GONE (-3)
#define AI_ID_20 0x20                   /* CHARACTER.id of slot 32 (BALOWS_CHARA): the predicates' boss kind */
#define AI_TURN_FULL 0x10000            /* CHARACTER.rot units per turn */
#define AI_FACE_TURN_SPEED 1000         /* ssAI_main: a halted stalker turns towards Alyssa this fast */
// Trigger events (global) this slice tests.
#define TRIG_EVENT_AI_NO_MODE_CHANGE 0xd0 /* set: no brain changes its mode (ssAI_charaMain) */
#define TRIG_EVENT_AI_D2 0xd2           /* removed every frame by ssAI_main (with 0xd1, 0xd3) */
#define TRIG_EVENT_DOOR_CLOSED 500      /* + door (0..AI_DOOR_MAX): the door is closed (ssAI_IsDoorClose) */
#define TRIG_EVENT_DOOR_OPEN_FULL 600   /* + door: the door is open all the way (ssAI_IsDoorOpenFull) */
#define AI_DOOR_MAX 0x62
// ssAI_CheckHideAreaAtkRate: the chance (per cent) of a strike at each hiding level (TRIG_VAR_HIDE_POINT;
// 4 and above always, 0 never).
#define AI_HIDE_ATK_RATE_1 0x25
#define AI_HIDE_ATK_RATE_2 0x32
#define AI_HIDE_ATK_RATE_3 0x4b
#define AI_HIDE_LEVEL_SURE 4

void ssAI_charaMain(struct CHARACTER *chr);                                              // CT3 0x01863c30
// Turns chr's rot towards the direction dir (a unit vector) by at most speed (rot units); a turn of a
// quarter to three quarters plays the turn motion unless noMotion or a turn already plays.
void ssAI_CompCharaDir(struct CHARACTER *chr, const float *dir, int speed, int noMotion); // CT3 0x01863980
void ssAI_SetPanicGaugeAndBGMVol(struct CHARACTER *chr);                                 // CT3 0x0185f470
// PanicSet(rise * the brain's BRAIN_PANIC_SCALE, source), except in SS_AI_RETURN.
void ssAI_PanicSet(float rise, struct CHARACTER *chr, int source);                       // CT3 0x0185f930
int ssAI_CheckHearingRate(struct CHARACTER *chr, int noise);                             // CT3 0x0185fd40
int ssAI_CheckHideAreaAtkRate(void);                                                     // CT3 0x0185fe10
int ssAI_IsDoorOpenFull(int door);                                                       // CT3 0x0185feb0
int ssAI_IsDoorClose(int door);                                                          // CT3 0x0185fef0
int ssAI_IsBendPlaying(struct CHARACTER *chr);                                           // CT3 0x018608e0
int ssAI_IsDownPlaying(struct CHARACTER *chr);                                           // CT3 0x01860910
int ssAI_IsReturnPlaying(struct CHARACTER *chr);                                         // CT3 0x01860940
int ssAI_IsDamagePlaying(struct CHARACTER *chr);                                         // CT3 0x01860a70
int ssAI_IsRotationPlaying(struct CHARACTER *chr);                                       // CT3 0x01861830
// A stalker's effects put back at its brain's end: its own model's weapons and attack shapes, its scene bank's
// effects, omni light, translucence, ambient and fade-away (slice 4).
void ssAI_BrainEndSpecialProc(struct CHARACTER *chr);                                    // CT3 0x01864950

// ---- slice 4 (2026-11-12): the obstacle steering, the frame and group / scene predicates, the axe reset ----
// tiAvoidObject: chr steers along dir (a unit vector, rewritten) around what blocks its line towards Alyssa.
// Probes start AVOID_EYE_HEIGHT above chr's feet; the first reaches as far as Alyssa (at most
// AVOID_PROBE_MAX), the side probes AVOID_SIDE_PROBE. *lastHit is the hit polygon (HIT_DATA) chr follows,
// NULL for none; a host pointer (the ROM's EE word). Answers 1 when the line was blocked or it turned.
#define AVOID_EYE_HEIGHT 30.0f
#define AVOID_PROBE_MAX 200.0f
#define AVOID_SIDE_PROBE 60.0f
#define AVOID_GOAL_OFFSET 100.0f        /* the goal stands this far off the followed polygon's corner */
#define AVOID_ANGLE_15 0x0aaa           /* CHARACTER.rot units (AI_TURN_FULL a turn): the probe fan */
#define AVOID_ANGLE_M15 0xf546
#define AVOID_ANGLE_30 0x1554
#define AVOID_ANGLE_M30 0xea9c
#define AVOID_ANGLE_45 0x1ffe           /* the turn taken when a probe is clear */
#define AVOID_ANGLE_M45 0xdff2
// CHARACTER +0xd0 (pad0d0): the float the goal takes as its y (tiAvoidObject); its meaning is open.
#define CHARA_AVOID_GOAL_Y 0xd0
int tiAvoidObject(float *dir, struct CHARACTER *chr, void **lastHit);                    // CT3 0x0185e9d0
// 1 when motion `index` of chr's table has passed `frame` since the last frame (its behaviour's first
// track's TRACK_FRAMES word: low half the frame now, high half the frame before; a wrap counts).
#define TRACK_FRAMES 0x1c               /* BEHAVIOR_TRACK.time1c read as two u_shorts */
#define TRACK_FLAG_NO_FRAMES 0x1        /* BEHAVIOR_TRACK.flags (= TRACK_FLAG_STEP_FROM_RUNS): no frame passes */
int ssAI_IsMortionFramePassed(struct CHARACTER *chr, int index, u_int frame);            // CT3 0x0185ffd0
// 1 when hide-pole group `group` links to group `other` (bank's HIDE_POLE_GROUPs).
int ssAI_IsNextGroup(short group, short other, int bank);                                // CT3 0x018603d0
// 1 when a shown shadow pole of bank leads to room.
int ssAI_IsNextScene(int bank, u_int room);                                              // CT3 0x01860490

// ---- slice 5 (2026-11-12): the kinds' motion predicates, the group links, the route ring, the arrivals ----
// Brain words read by slice 5 (named from these readers only: open-items.md tiAvoidObject.c slice 5).
#define BRAIN_SUBMODE 0x001             /* u_char: BRAIN_SUBMODE_* (ssAI_IsWarpChara) */
#define BRAIN_SUBMODE_SCN_LINK 2        /* crossing a scene link: BRAIN_LINK_ROOM and BRAIN_ROOM both count */
#define BRAIN_ROOM 0x02c                /* u_int: the brain's room (a GetRoomName_file word) */
#define BRAIN_SCN_NEXT_POINT 0x050      /* float[4]: the next point in the next scene */
#define BRAIN_NEXT_POINT 0x060          /* float[4]: the next point */
#define BRAIN_LINK_ROOM 0x078           /* u_int: the room on the scene link's other side */
#define BRAIN_SCN_END_POLE 0x07c        /* short: the pole the scene ends at (in chr->lightBank) */
#define AI_ARRIVE_DIST 20.0f            /* the IsGetTo* predicates: arrived within this distance */
#define AI_RANDOM_POLE_MAX 256          /* GetNextTargPoleRandom's candidates (a short[256] on the stack) */
// A group link pole joins two hide-pole groups (its pair lies in the other group); the flag is the
// segment flag: a link's position is its middle.
#define HIDE_POLE_FLAG_GRP_LINK HIDE_POLE_FLAG_SEGMENT
int ssAI_IsForwardPlaying(struct CHARACTER *chr);                                        // CT3 0x018606c0
int ssAI_IsWaitPlaying(struct CHARACTER *chr);                                           // CT3 0x01860f40
int ssAI_IsFarAttackPlaying(struct CHARACTER *chr);                                      // CT3 0x01861070
int ssAI_IsPostAttackPlaying(struct CHARACTER *chr);                                     // CT3 0x01861110
int ssAI_IsAttackPlaying(struct CHARACTER *chr);                                         // CT3 0x018612d0
int ssAI_IsPreAttackPlaying(struct CHARACTER *chr);                                      // CT3 0x01861510
int ssAI_IsFindPlaying(struct CHARACTER *chr);                                           // CT3 0x018616b0
int ssAI_IsActionPlaying(struct CHARACTER *chr);                                         // CT3 0x01861a00
int ssAI_IsWarpChara(struct CHARACTER *target, struct CHARACTER *chr);                   // CT3 0x01860540
int ssAI_IsGetToPoint(float dist, struct CHARACTER *chr, const float *pos);              // CT3 0x01861e70
int ssAI_IsGetToPole(struct CHARACTER *chr, short pole, int bank);                       // CT3 0x01861ef0
int ssAI_IsGetToScnEndPole(struct CHARACTER *chr);                                       // CT3 0x01861fa0
int ssAI_IsGetToScnNextPoint(struct CHARACTER *chr);                                     // CT3 0x01862050
int ssAI_IsGetToNextPoint(struct CHARACTER *chr);                                        // CT3 0x018620e0
short ssAI_GetGLinkPolID(short group, short other, int bank);                            // CT3 0x01862940
short ssAI_GetPairGrpLinkPoleID(short pole, int bank);                                   // CT3 0x01862bd0
short ssAI_GetNextTargPoleRandom2(int bank, short except, u_char flags);                 // CT3 0x01862cb0
short ssAI_GetNextTargPoleRandom(int bank, short except, u_char flags);                  // CT3 0x01862dd0
u_char ssAI_GetPoleDoorNo(int pole, int bank);                                           // CT3 0x01862ef0
int ssAI_GetGLinkPolPos(float *pos, short group, short other, int bank);                 // CT3 0x01862f50
short ssAI_GRS_PopFront(int bank);                                                       // CT3 0x01863080
int ssAI_GRS_PushRear(short v, int bank);                                                // CT3 0x018630f0
int ssAI_GRS_PushFront(short v, int bank);                                               // CT3 0x01863160
float ssAI_GRS_GetTargDistance(short from, short to, const float *pos, int bank);        // CT3 0x01863330

// ---- slice 6 (2026-11-12): the shadow entry, the placement, the sight checks, the group route search ----
// Brain words read by slice 6 (named from these readers only: open-items.md tiAvoidObject.c slice 6).
#define BRAIN_GOAL_ROOM 0x024           /* u_int: SetMonsterPos re-plans a trace when BRAIN_ROOM leaves it */
#define BRAIN_POLE_GROUP 0x028          /* short: the hide-pole group chr stands in (CHARACTER.h100 too) */
#define BRAIN_BANK 0x02a                /* u_char: the scene bank of BRAIN_ROOM */
#define BRAIN_LINK_POLE 0x03e           /* short: the scene-link pole chr goes through */
#define BRAIN_TARG_POLE 0x070           /* short: the pole BRAIN_NEXT_POINT stands at */
#define BRAIN_LINK_BANK 0x074           /* s_char: the bank on the scene link's other side */
#define BRAIN_DOOR_MOTION_ALT 0x248     /* short: entering through a closed door with HIDE_POLE_FLAG_DOOR_ALT */
#define BRAIN_DOOR_MOTION 0x24a         /* short: entering through a closed door otherwise */
#define BRAIN_DOOR_MOTION_END 0x24c     /* short: played after either door motion */
// Hide-pole flags read by LetMonsterInFromShadow (named from that one use: guesses).
#define HIDE_POLE_FLAG_OWN_GROUP 0x04   /* the entry takes this pole's group, not the other bank's shadow's */
#define HIDE_POLE_FLAG_CROSS_BANK 0x08  /* the stalker enters the other bank's room (with a door check) */
#define HIDE_POLE_FLAG_DOOR_ALT 0x80    /* the closed door plays BRAIN_DOOR_MOTION_ALT */
#define AI_BARRIER_LIFT 10.0f           /* ssAI_BarrierCheck: both ends of its probes are lifted this much */
#define AI_SEARCH_EYE 180.0f            /* ssAI_SerchCheck: the searcher's eye above its feet */
#define AI_SEARCH_HEAD 160.0f           /* the target's head above its feet (standing) */
#define AI_SEARCH_HEAD_LOW 30.0f        /* the same while it bends or is down */
#define AI_ROUTE_NONE (-1.0f)           /* ssAI_GrpRouteSearch: a group is not in the bank */
// Puts a stalker into its room through the shadow polygon shadowPol of `bank` (its scene-link pair is the
// way on): 0, or -1 when the shadow has no pole or pair, the other bank is not loaded, or the placement
// failed. motion follows the door motions.
int ssAI_LetMonsterInFromShadow(struct CHARACTER *chr, u_char otherBank, u_char bank, s_char shadowPol,
                                short motion);                                           // CT3 0x0185f970
// Moves chr to pos (both positions) in pole group `group`; room != 0 also moves it into that room of bank.
// endProc first ends the brain's effects and plays its end motion. -1 when a trace could not be re-planned.
int ssAI_SetMonsterPos(struct CHARACTER *chr, const float *pos, short group, u_char bank, u_int room,
                       int endProc);                                                     // CT3 0x01862170
// 1 when a probe from one of the four lowest-y corners of chr's box to pos hits bank's collision.
int ssAI_BarrierCheck(struct CHARACTER *chr, const float *pos, int bank);                // CT3 0x018600e0
// 1 when chr sees target: within dist, within fov (rot units) of its facing, and a line from its eye to
// target's feet or head is clear in bank.
int ssAI_SerchCheck(float dist, struct CHARACTER *chr, struct CHARACTER *target, int fov,
                    int bank);                                                           // CT3 0x01860250
// 1 when the direction dir lies within fov / 2 of chr's facing (rot units).
int ssAI_IsTrgWithinAngle(const float *dir, struct CHARACTER *chr, int fov);             // CT3 0x018605d0
// 1 when chr stands within AI_ARRIVE_DIST of the group-link segment `pole`; -1 when it is no link.
int ssAI_IsGetToGLinkPole(struct CHARACTER *chr, short pole, int bank);                  // CT3 0x01861b90
// The brain's next point: where the line from chr to pos (pos mirrored across the link when both stand on
// one side) crosses the group-link segment `pole` in the XZ plane, else the nearer end; -1 when the
// crossing falls outside the segment.
int ssAI_SetNextGLinkPolePoint(short pole, int bank, struct CHARACTER *chr, const float *pos); // CT3 0x01862290
// The scene-link pole into room nearest pos (the pair of a shown shadow pole leading there); -1 for none.
short ssAI_GetScnLinkPoleID(const float *pos, u_int room, int bank);                     // CT3 0x018629f0
// The route search over the bank's pole groups (HIDE_POLE_GROUP's work fields; route ring of group ids).
float ssAI_GRS_GetNextDistance(short group, int link, const float *pos, int bank);       // CT3 0x018631d0
void ssAI_GRS_RouteGrpRecur(HIDE_POLE_GROUP *groups, const float *pos, const float *targPos, short from,
                            short to, int bank);                                         // CT3 0x01863420
// The length of the shortest route from pos in group `group` to targPos in group `targGroup`, its groups
// pushed on the route ring (the start's excluded); AI_ROUTE_NONE when a group is not in the bank.
float ssAI_GrpRouteSearch(const float *pos, const float *targPos, short group, short targGroup,
                          int bank);                                                     // CT3 0x01863640
void ssAI_GRS_InitParam(int bank);                                                       // CT3 0x01863870
