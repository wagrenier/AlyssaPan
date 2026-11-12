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
void ssAI_BrainEnd(struct CHARACTER *chara, int brain);                                  // CT3 0x01865e20 (main2.c)
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
} HIDE_POLE_LIST;
static_assert(sizeof(HIDE_POLE_LIST) == 8, "HIDE_POLE_LIST");
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
