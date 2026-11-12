// game/tiXcmd_TransItem.h -- CT3 game/tiXcmd_TransItem.c: the declarations other TUs call (ssGrpPortalCheck.c, 2026-11-11).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// Slice 1 (2026-11-11): the patterns its commands use.
#define PARTS_KIND_ITEM_LOCKED 1        /* MODEL_PARTS.kind: the item commands leave the part alone (the
                                           kind's own meaning unread) */
#define TI_ITEM_TRANS_MAX 100.0f        /* trans_add_value's top: the item comes this far toward the eye */
#define TI_ITEM_CAMERA 0                /* tiXcmd_ItemCheckInit takes Cam[0]'s eye */
#define CHARA_QUAKE_SOURCE 2            /* Character[2]: where tiXcmd_CheckEarthquake's quake starts */
#define TI_QUAKE_SPEED_DIV 5.0f         /* the quake spreads argv[0] / 5 per tick */
#define TI_QUAKE_RANGE_MAX 1000.0f      /* past this radius the quake is over unseen */
#define BOSS_INFO_HIT_BACKS 0x9c        /* pBossInfo: float[6], tiXcmd_BOSS_SetHitBacks */
#define BOSS_INFO_HIT_BACK_COUNT 6
// Slice 9 (2026-11-11): tiXcmd_Balows2th's boss info (names from this use; the boss TU confirms them).
#define BOSS_INFO_PHASE 0x0c            /* int: Balows2th sets BOSS_PHASE_BALOWS_2 */
#define BOSS_PHASE_BALOWS_2 4
#define BOSS_INFO_CHAIN_A_TIME 0x40     /* int: chain set A's pull time (set B's is 0x44, BOSS_INFO_PULL_TIME) */
#define BOSS_INFO_CHAIN_B_TIME 0x44
#define BOSS_INFO_CHAIN_A_COUNT 0x90    /* u_char: set A's chains (set B's count is 0x91) */
#define BOSS_INFO_CHAIN_B_COUNT 0x91    /* u_char: set B's chains (main.c's light belt test) */
#define BOSS_INFO_CHAIN_A_ACTIVE 0x92   /* u_char: set A is up (events 0x3c..0x45, SE slot 1) */
#define BOSS_INFO_CHAIN_B_ACTIVE 0x93   /* u_char: set B is up (events 0x69..0x6d, SE slot 2) = sound.h's
                                           BOSS_INFO_BAL_LOOP */
#define BOSS_INFO_CHAIN_A_ON 0x10c      /* ptr -> u_char[count]: set A's chains holding (set B: 0x110) */
// Slice 11 (2026-11-12): the boss info's host form (open-items.md 1 as / kh / jr, decided here: this TU
// allocates it). pBossInfo stays the ROM's 0x160-byte gHeapJunc block, every scalar at its EE offset
// (main.c, sound.c, Clipping_Portal.c and wideXcmd_DebugPrint.c index its bytes). Its pointer words
// (0xe8..0x140) cannot hold a host pointer in 4 bytes: the host keeps each in a side table
// (BossInfoGetPtr / BossInfoSetPtr) and the EE word holds BOSS_INFO_PTR_SET or 0, so the ROM's "!= 0"
// tests still hold. The blocks they point to are host layout: lists of host pointers, u_char flags.
#define BOSS_INFO_SIZE 0x160            /* _SunAlloc(gHeapJunc, 0x160, 0x10) */
#define BOSS_INFO_CHARA 0x00            /* u_char: the boss's Character[] index (tiXcmd_BOSS_BattleStart) */
#define BOSS_INFO_CHAIN_A_BANK 0x04     /* int: set A's chain bank (tiXcmd_BOSS_SetChain's) */
#define BOSS_INFO_CHAIN_B_BANK 0x08     /* int: set B's (tiXcmd_BOSS_SetChain_Alyssa's) */
#define BOSS_INFO_THRESHOLDS 0x48       /* int[8]: per boss (BOSS_SETUP.thresholds; meaning unread) */
#define BOSS_INFO_B4 0xb4               /* int[4]: per boss (50, 75, 100, 125 for Character[3]) */
#define BOSS_INFO_CHAIN_A_BELTS 0xe8    /* ptr -> LIGHT_BELT2 *[count]: each set A chain's light belt */
#define BOSS_INFO_CHAIN_B_BELTS 0xec    /* ptr -> LIGHT_BELT2 *[count]: set B's */
#define BOSS_INFO_CHAIN_A_SRC 0xf4      /* ptr -> float[4]: the boss's "Bip01 Spine1" position */
#define BOSS_INFO_CHAIN_B_SRC 0xf8      /* ptr -> float[4]: the player's "Bip01 Spine1" position */
#define BOSS_INFO_CHAIN_A_FROM 0xfc     /* ptr -> float *[count]: each chain's first part, drawOnPos */
#define BOSS_INFO_CHAIN_B_FROM 0x100    /*   (wideXcmd_DebugPrint.c's BOSS_INFO_CHAIN_POS) */
#define BOSS_INFO_CHAIN_A_TO 0x104      /* ptr -> float *[count]: each chain's last part, drawOnPos (the */
#define BOSS_INFO_CHAIN_B_TO 0x108      /*   belt end Clipping_Portal.c's ChainBelts moves to the source) */
#define BOSS_INFO_CHAIN_B_ON 0x110      /* ptr -> u_char[count]: set B's chains holding */
#define BOSS_INFO_BLOOD_POS 0x130       /* ptr -> float[4]: GetBloodAttackObject's part position (mode 1) */
#define BOSS_INFO_BLOOD_PART 0x134      /* ptr -> a part's record (MODEL_PARTS + 0x40; mode 0) */
#define BOSS_INFO_WARP_COUNT 0xc4       /* int: tiXcmd_BOSS_SetWarp's points (!= 0: the list is allocated) */
#define BOSS_INFO_WARP_POS 0x13c        /* ptr -> float *[count]: the warp points' part positions */
#define BOSS_INFO_SPATTACK 0x140        /* ptr -> SPATTACK_SCISSORS: the SCISSORS_MAN attack record */
#define BOSS_INFO_PTR_FIRST 0xe8
#define BOSS_INFO_PTR_LAST 0x140
#define BOSS_INFO_PTR_SET 1u            /* PORT: the EE word of a set pointer (the host pointer is aside) */
#define BOSS_STATE_BIT_04 0x0004        /* BOSS_INFO_PHASE bits BattleStart sets per boss (meaning unread) */
#define BOSS_STATE_BIT_2000 0x2000
#define BOSS_STATE_TWIN_ATTACK 0x0010   /* the SCISSORS_MAN attack runs two streams (B at +spread, A at
                                           -spread) and turns twice as fast; cleared as it ends */
#define BOSS_STATE_SPATTACK 0x4000      /* the SCISSORS_MAN attack has started (its first call set it) */

// Slice 12 (2026-11-12): the SCISSORS_MAN attack record (pBossInfo word BOSS_INFO_SPATTACK;
// tiXcmd_Init_SPAttack_SCISSORS_MAN allocates it, 0x70 bytes on the EE). Streams of parts walk from the
// boss toward the player: each step moves a stream `speed` along its heading, turning it toward the
// player (at most turnDeg a step) while the player is within SPATTACK_NEAR and ahead; parts[cur] and
// tips[cur] are placed at the stream's new position and one step past it. layout: host (the lists hold
// host pointers; only this TU and Clipping_Portal.c's tiFreeBossData touch it).
typedef struct SPATTACK_SCISSORS {
    int count;                          /* 0x00: parts in each list */
    int cur;                            /* 0x04: the list slot the last step placed */
    int elapsedTicks;                   /* 0x08 */
    int durationTicks;                  /* 0x0c: argv[3] * SPATTACK_TICKS_PER_DURATION */
    int sinceStepTicks;                 /* 0x10 */
    int stepTicks;                      /* 0x14: argv[2] * SPATTACK_TICKS_PER_STEP */
    float speed;                        /* 0x18: distance per step */
    float turnDeg;                      /* 0x1c: the most a step turns (doubled in a twin attack) */
    int spreadDeg;                      /* 0x20: a twin attack's streams start this far apart (+/-) */
    struct MODEL_PARTS **parts;         /* 0x24: [count] each step's part at the stream's position */
    struct MODEL_PARTS **tips;          /* 0x28: [count] each step's part one step further */
    float posA[4];                      /* 0x30: stream A's position */
    float posB[4];                      /* 0x40 */
    float dirA[4];                      /* 0x50: stream A's heading (unit, w 0) */
    float dirB[4];                      /* 0x60 */
} SPATTACK_SCISSORS;
#define SPATTACK_TICKS_PER_STEP 30      /* argv[2]'s unit */
#define SPATTACK_TICKS_PER_DURATION 300 /* argv[3]'s unit */
#define SPATTACK_NEAR 300.0f            /* the player within this: the streams home in */
#define SPATTACK_HIDDEN_Y 1.0e9f        /* Init parks every parts[] part this high */
#define SPATTACK_BOSS_CHARA 5           /* the streams start at Character[5].pos (the ROM's fixed address) */
#define TRIG_EVENT_SPATTACK_FIRST 0x71  /* + cur: registered (global) as a step places slot cur */
#define BOSS_PLAYER_HP 800              /* BattleStart: the player's hp and hpMax (BOSS_PLAYER_HP_LAST for */
#define BOSS_PLAYER_HP_LAST 1200        /*   the last boss, Character[0x20]) */
#define CHAIN_SOURCE_PART "Bip01 Spine1" /* the chains' source part (the boss's, the player's) */
void *BossInfoGetPtr(int off);          /* PORT: the host pointer of pointer word `off`, NULL if none */
void BossInfoSetPtr(int off, void *p);  /* PORT: sets it, and the EE word to BOSS_INFO_PTR_SET or 0 */
#define BALOWS_CHARA 32                /* Character[32]: the boss Balows2th resets (hp, brain) */
#define BALOWS_2_HP 10
#define TRIG_EVENT_BALOWS_2 0x55e       /* registered as the second phase starts */
#define TRIG_EVENT_CHAINS_CUT 0x85      /* registered when a chain set is dropped */
#define TRIG_EVENT_CHAIN_A_FIRST 0x3c   /* set A's events 0x3c..0x45 */
#define TRIG_EVENT_CHAIN_A_LAST 0x45
#define TRIG_EVENT_CHAIN_B_FIRST 0x69   /* set B's events 0x69..0x6d */
#define TRIG_EVENT_CHAIN_B_LAST 0x6d
#define TRIG_VAR_BALOWS_52 0x52         /* global variables Balows2th sets to -1 (meaning unread) */
#define TRIG_VAR_BALOWS_05 0x05
#define SE_BOSS_LOOP_T 0xf              /* "T_0010" 15 and "C_0000" 0x4b: sound.h's boss loops */
#define SE_BOSS_LOOP_C 0x4b
// wideXcmd_Delay's third argument: "0" waits animation ticks (value * 1000 / anim_play_slow), anything
// else counts frames.
#define DELAY_MODE_TICKS '0'
#define DELAY_TICK_SCALE 1000
#define SHADE_OFF_LEVELS 9              /* ShadeOff_CapLevel = 9 - the level argument */
#define SHADE_OFF_ON_ARGC_VAR 7         /* "ON" with 7 arguments: the frames are a variable */
#define SHADE_OFF_OFF_ARGC_VAR 3        /* "OFF" with 3 arguments: the frames are a variable */
#define KEEP_SCREEN_HOLD 0x1            /* fKeepScreen: "ON .. KEEP": the shaded screen is held */
#define KEEP_SCREEN_CAPTURED 0x2        /* fKeepScreen: the held screen is captured (tiDrawShadingOff) */
#define KEEP_SCREEN_RELEASE 0x4         /* fKeepScreen: "OFF": the shade-off ends */
#define XFADE_TYPE_NONE 0               /* XFade_Type */
#define XFADE_TYPE_PAUSE 1              /* the two kinds that reserved a pause (pause_reservation) */
#define XFADE_TYPE_PAUSE2 3
#define TRIG_EVENT_CAM_RESERVED 0x9b    /* global: a camera change is reserved (ChangeCam_Execute removes it) */

// Slice 2 (2026-11-11). tiXcmd_OperateItem's controls (pad_*_button[0], sdk/libpad.h's bits).
#define TI_ITEM_EXIT_BUTTON SCE_PADRdown   /* cross (pressed): the examination ends */
#define TI_ITEM_TOWARD_BUTTON SCE_PADRup   /* triangle (held): the item comes toward the eye */
#define TI_ITEM_AWAY_BUTTON SCE_PADRleft   /* square (held): away */
#define TI_ITEM_STICK_TURN_ID 0         /* asGetAnalogStickInformation: pad 0's left stick turns the item */
#define TI_ITEM_STICK_MOVE_ID 1         /* its right stick moves it */
#define TI_ITEM_STICK_DEAD 0x28         /* the dead zone both reads pass */
#define TI_ITEM_STICK_TURN 7.0f         /* degrees a frame at full tilt */
#define TI_ITEM_PAD_TURN 0x444          /* the d-pad's turn a frame (6 degrees) */
#define TI_ITEM_STEP 5.0f               /* the move a frame (of TI_ITEM_TRANS_MAX) */
#define TI_ITEM_STICK_TOWARD_LO 90.0f   /* the right stick within 90 degrees of up: toward the eye */
#define TI_ITEM_STICK_TOWARD_HI 270.0f
// tiXcmd_FadeStart.
#define FADE_START_ARGC_TIME 3          /* 3 arguments: the time is argv[0]; else the variable argv[0] / argv[3] */
#define TRIG_EVENT_FADED_OUT 0x4a       /* global: registered by the OUT fades, removed by the IN fades */
// The letters (LetterList[LETTER_CATEGORIES][LETTER_LIST_SLOTS], LetterData's records).
#define LETTER_CATEGORIES 5
#define LETTER_LIST_SLOTS 10            /* also "not found" / "all full" */
#define LETTER_DATA_FIELDS 5            /* CATEGORY, NUMBER, PAGE_NUMBER, START_PAGE, END_PAGE: bytes 0..4 */
// KEY_MAP: [0..34] the player's assignment, [35..69] the defaults tiXcmd_ResetKeyMap copies back.
#define KEY_MAP_ACTIONS 35
#define KEY_MAP_DEFAULTS 35
// tiXcmd_SetKeyToKeyMap: the actions a new assignment may swap with (KEY_MAP[16..34], the ROM's loop),
// minus the pair at 19 / 20 (the pair system_data_load_set copies from 54 / 55: never swapped).
#define KEY_MAP_CONFIG_FIRST 16
#define KEY_MAP_CONFIG_LAST 34
#define KEY_MAP_LINKED_FIRST 19
// tiXcmd_KeyNumToFrame: the key-config screen's button picture frames (named from that one use).
enum {
    KEY_FRAME_CIRCLE = 0,               /* also anything not listed */
    KEY_FRAME_CROSS = 1,
    KEY_FRAME_SQUARE = 2,
    KEY_FRAME_TRIANGLE = 3,
    KEY_FRAME_L1 = 4,
    KEY_FRAME_L2 = 5,
    KEY_FRAME_R1 = 6,
    KEY_FRAME_R2 = 7,
    KEY_FRAME_SELECT = 8,
    KEY_FRAME_START = 9,
};
// tiXcmd_SetStatusBar / tiXcmd_GetItemListLastIndex.
#define STATUS_BAR_FROM_NOW (-1)        /* asSceneAnimationStart's from: the current frame */
#define ITEM_LIST_SLOTS 12              /* ItemList[] */
#define ITEM_LIST_LAST_A 1              /* GetItemListLastIndex's argv[2]: both run the same loop */
#define ITEM_LIST_LAST_B 0

// tiXcmd_SetItemData / tiXcmd_GetItemData: argv[5] picks the table, argv[4] the field of its 0x18-byte
// row (ItemData / KeyItemData; the fields' meanings are not read yet, so they keep their offsets).
enum {
    ITEM_TABLE_ITEMS = 0,               /* ItemData */
    ITEM_TABLE_KEY_ITEMS = 1,           /* KeyItemData */
    ITEM_TABLE_ARROWS = 3,              /* ArrowData (GetItemData, field ITEM_FIELD_HELD only) */
};
enum {
    ITEM_FIELD_KIND = 0,                /* byte +0 */
    ITEM_FIELD_HELD = 1,                /* byte +1 (held count) */
    ITEM_FIELD_04 = 2,                  /* int +4 (SetItemData stores the value & 0xff) */
    ITEM_FIELD_08 = 3,                  /* int +8 */
    ITEM_FIELD_0C = 4,                  /* int +0xc */
    ITEM_FIELD_10 = 5,                  /* int +0x10 */
};

// Slice 5 (2026-11-11): tiXcmd_GetItem / tiXcmd_GetIDfromList. The kinds are ITEM_TABLE_* plus
// ITEM_TABLE_LETTERS; the list sizes are the symtab's (ArrowList 200 bytes, KeyItemList 96, ArrowData 12).
#define ITEM_TABLE_LETTERS 2            /* LetterData / LetterList (between the key items and the arrows) */
#define ITEM_ROW_EVENT 0x14             /* the item row's int: the event registered on pick-up */
#define LETTER_DATA_EVENT 8             /* LetterData's int: the same for a letter */
#define ITEM_FIXED_SLOTS 5              /* items 1..4 own ItemList[id - 1]; the others list from slot 4 */
#define KEY_ITEM_LIST_SLOTS 24          /* KeyItemList[] */
#define ARROW_LIST_SLOTS 50             /* ArrowList[] */
#define ARROW_KINDS 3                   /* ArrowData[]: the held count of each arrow kind */
#define ARROW_UNLISTED 3                /* GetItem's arrow id that is counted only, never listed */
#define TRIG_VAR_GOT_ITEM_KIND 0x23     /* global: the kind of the last item picked up (GetItem) */

// Slice 10 (2026-11-11): tiXcmd_UseItem. argv[4] says how the slot is found or what the use does; the
// row's byte +0 (ITEM_FIELD_KIND) says what using the item does. A use that cannot happen sets the item
// variable to -1. Named from this one function: the scripts that pass them confirm them.
enum {
    ITEM_USE_SLOT_FROM_VAR = 0,         /* the slot is the variable argv[2] (global argv[3]); use the item */
    ITEM_USE_DROP = 1,                  /* ... and only empty the slot (no use, no compaction) */
    ITEM_USE_FIND = 2,                  /* the slot is the one holding the item; use it */
};
enum {
    ITEM_KIND_GIVE_TAKE = 0,            /* register +4, remove +0x14, consume */
    ITEM_KIND_GIVE_TAKE_IF = 1,         /* the same when the event +0x10 is set, else refused */
    ITEM_KIND_SIGNAL = 2,               /* register +4 only (kept) */
    ITEM_KIND_SIGNAL_IF = 3,            /* register +4 when the event +0x10 is set (kept), else refused */
    ITEM_KIND_HEAL = 4,                 /* Alyssa's hp += the int +4, up to hpMax; consume */
    ITEM_KIND_CALM = 5,                 /* PanicDownSet(the int +4) while she panics; consume */
    ITEM_KIND_INVISIBLE = 6,            /* sets TRIG_EVENT_INVISIBLE unless it is set; consume */
    ITEM_KIND_COUNT_ONLY = 0xff,        /* (-1) consume only; a key item of this kind just leaves the list */
};
#define ITEM_ROW_USE_EVENT 4            /* the row's int: the event a use registers (HEAL / CALM: the amount) */
#define ITEM_ROW_NEED_EVENT 0x10        /* the row's int: the event the _IF kinds need */
#define TRIG_EVENT_CALM_ALT_READY 0x44b /* set: a CALM item registers TRIG_EVENT_CALM_ALT_USED instead */
#define TRIG_EVENT_CALM_ALT_USED 0x44a
#define TRIG_EVENT_NO_CALM 0x1a         /* set: a CALM item is refused (also cleared by a load) */

// Slice 6 (2026-11-11): tiXcmd_VibrationStart's argument counts (argv[0] the port, then the motors).
#define VIBRATION_ARGC_LITERAL 3        /* port, motor 1, motor 2: the numbers themselves */
#define VIBRATION_ARGC_LITERAL_TIME 4   /* ... and the fourth tiVibrationOn argument */
#define VIBRATION_ARGC_VARS 5           /* port, (event, global) for motor 1, (variable, global) for motor 2 */
#define VIBRATION_ARGC_VARS_TIME 6      /* ... and the fourth argument as a number */
// tiXcmd_CheckMCFile: mc_Getinfo_time counts frames (30 a second); the play time goes out as digits.
#define MC_TIME_FRAMES_PER_HOUR 108000  /* 0x1a5e0 */
#define MC_TIME_FRAMES_PER_MINUTE 1800  /* 0x708 */
#define MC_TIME_FRAMES_PER_SECOND 30    /* 0x1e */

// tiXcmd_CompareVariable: an operand whose global argument is XCMD_SCOPE_LITERAL is the number itself
// (tkXcmd_GetValueType answers 2 for the same); the answer goes to a variable as COMPARE_*.
// Slice 8 (2026-11-11): the scope argument of ssTrigRegisterEvent / ssTrigGetVariable and the rest, as
// tkXcmd_GetValueType reads it from a letter or a digit.
#define XCMD_SCOPE_BANK 0               /* "t" / "T" / "0": the running bank's own events and variables */
#define XCMD_SCOPE_GLOBAL 1             /* "g" / "G" / "1" */
#define XCMD_SCOPE_LITERAL 2            /* "n" / "N" / "2": the argument is the number itself */
int tkXcmd_GetValueType(const char *arg);                                                // CT3 0x018e8e70
// tkXcmd_CaptionFlag's optional third argument: which way OMAKE_2 and the event go.
enum {
    CAPTION_FLAG_TO_EVENT = 0,          /* no third argument, "g" / "G" / "0": the event follows OMAKE_2 */
    CAPTION_FLAG_FROM_EVENT = 1,        /* "s" / "S" / "1": OMAKE_2 follows the event */
    CAPTION_FLAG_IGNORED = 2,           /* any other third argument: the command does nothing */
};
// tiXcmd_SetRotation's axis argument.
enum {
    ROTATION_AXIS_X = 0,
    ROTATION_AXIS_Y = 1,
    ROTATION_AXIS_Z = 2,
};
#define DEGREES_TO_GA_ANGLE(deg) ((int)(((deg) * 65536.0f) / 360.0f) & 0xffff) /* gaSin's 16-bit turn */
// tiXcmd_CopySunPs2Object copies this much of a texture upload packet (SUN_TBUFFER_DATA is 0xd8; the ROM
// copies 0x100, as tex_A$2748 holds).
#define TI_TBUFFER_COPY_SIZE 0x100
enum {
    COMPARE_EQUAL = 0,
    COMPARE_GREATER = 1,                /* the first operand is the greater */
    COMPARE_LESS = 2,
};

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int tkXcmd_CharSceneChange(int argc, char **argv, int bank);                             // CT3 0x018e8d20
int tiXcmd___________Free0(int argc, char **argv, int bank);                             // CT3 0x018e20b0
int tiXcmd_SetRotation(int argc, char **argv, int bank);                                 // CT3 0x018e89d0
int tiXcmd_SetScale(int argc, char **argv, int bank);                                    // CT3 0x018e8930
int tiXcmd_VibrationON_OFF(int argc, char **argv, int bank);                             // CT3 0x018e7810
int tiXcmd_VibrationCheckFlg(int argc, char **argv, int bank);                           // CT3 0x018e77b0
int tiXcmd_VibrationStart(int argc, char **argv, int bank);                              // CT3 0x018e75e0
int tiXcmd_VibrationEnd(int argc, char **argv, int bank);                                // CT3 0x018e75b0
int tiXcmd___________Free1(int argc, char **argv, int bank);                             // CT3 0x018e2020
int tiXcmd___________Free2(int argc, char **argv, int bank);                             // CT3 0x018e1fc0
int tiXcmd_StereoON_OFF(int argc, char **argv, int bank);                                // CT3 0x018e75a0
int tiXcmd_StereoCheckFlg(int argc, char **argv, int bank);                              // CT3 0x018e7590
int tiXcmd_SetParaDefault(int argc, char **argv, int bank);                              // CT3 0x018e7580
int tiXcmd_CheckMCFile(int argc, char **argv, int bank);                                 // CT3 0x018e71f0
int tiXcmd_CopyVariable(int argc, char **argv, int bank);                                // CT3 0x018e88a0
int tiXcmd_SetReset(int argc, char **argv, int bank);                                    // CT3 0x018e8890
int tiXcmd_GetMCflag(int argc, char **argv, int bank);                                   // CT3 0x018e7130
int tiXcmd_SetmcMain_ID(int argc, char **argv, int bank);                                // CT3 0x018e70d0
int tiXcmd_SetmcSlot_ID(int argc, char **argv, int bank);                                // CT3 0x018e7070
int tiXcmd_SetmcFile_ID(int argc, char **argv, int bank);                                // CT3 0x018e7010
int tiXcmd_SetmcErr_ID(int argc, char **argv, int bank);                                 // CT3 0x018e6fb0
int tiXcmd_SetmcAccess_ID(int argc, char **argv, int bank);                              // CT3 0x018e6f50
int tiXcmd___________Free3(int argc, char **argv, int bank);                             // CT3 0x018e1fb0
int tiXcmd_SetPlayerChara(int argc, char **argv, int bank);                              // CT3 0x018e7d10
int tiXcmd_GetItem(int argc, char **argv, int bank);                                     // CT3 0x018e6bf0
int tiXcmd_UseItem(int argc, char **argv, int bank);                                     // CT3 0x018e6330
int tiXcmd_GetIDfromList(int argc, char **argv, int bank);                               // CT3 0x018e6160
int tiXcmd_SetStatusBar(int argc, char **argv, int bank);                                // CT3 0x018e57b0
int tiXcmd_SceneAnimationSkipToVariable(int argc, char **argv, int bank);                // CT3 0x018e86d0
int tiXcmd_AddVariable(int argc, char **argv, int bank);                                 // CT3 0x018e8640
int tiXcmd_SubVariable(int argc, char **argv, int bank);                                 // CT3 0x018e85b0
int tiXcmd_GetItemData(int argc, char **argv, int bank);                                 // CT3 0x018e5ce0
int tiXcmd_SetItemData(int argc, char **argv, int bank);                                 // CT3 0x018e5a50
int tiXcmd_GetItemListLastIndex(int argc, char **argv, int bank);                        // CT3 0x018e5930
int tiXcmd_CopySunPs2Object(int argc, char **argv, int bank);                            // CT3 0x018e83d0
int tiXcmd_CompareVariable(int argc, char **argv, int bank);                             // CT3 0x018e8260
int tiXcmd_MoveItem(int argc, char **argv, int bank);                                    // CT3 0x018e5870
int tiXcmd_SetKeyToKeyMap(int argc, char **argv, int bank);                              // CT3 0x018e5590
int tiXcmd_GetKeyFromKeyMap(int argc, char **argv, int bank);                            // CT3 0x018e54f0
int tiXcmd_ResetKeyMap(int argc, char **argv, int bank);                                 // CT3 0x018e54c0
int tiXcmd_ItemCheckInit(int argc, char **argv, int bank);                               // CT3 0x018e1550
int tiXcmd_TransItem(int argc, char **argv, int bank);                                   // CT3 0x018e0e80
int tiXcmd_SetSceneID(int argc, char **argv, int bank);                                  // CT3 0x018e1610
int tiXcmd_CheckLife(int argc, char **argv, int bank);                                   // CT3 0x018e5700
int tiXcmd_XFadeOut2Start(int argc, char **argv, int bank);                              // CT3 0x018e4c00
int tiXcmd_SetDrawMode_Global(int argc, char **argv, int bank);                          // CT3 0x018e8210
int tiXcmd_GetAlyssaLife(int argc, char **argv, int bank);                               // CT3 0x018e56a0
int tiXcmd_OperateItem(int argc, char **argv, int bank);                                 // CT3 0x018e0fa0
int tiXcmd___________Free4(int argc, char **argv, int bank);                             // CT3 0x018e1fa0
int tiXcmd___________Free5(int argc, char **argv, int bank);                             // CT3 0x018e1f90
int tiXcmd___________Free6(int argc, char **argv, int bank);                             // CT3 0x018e1f80
int tkXcmd_CaptionFlag(int argc, char **argv, int bank);                                 // CT3 0x018e8bc0
int tiXcmd_SetPointForGhostArea(int argc, char **argv, int bank);                        // CT3 0x018e1ed0
int tiXcmd_Balows2th(int argc, char **argv, int bank);                                   // CT3 0x018e1bc0
int tiXcmd_TEST00(int argc, char **argv, int bank);                                      // CT3 0x018e8b70
int tiXcmd_SetMapData(int argc, char **argv, int bank);                                  // CT3 0x018e1810
int tiXcmd_CompareVar2(int argc, char **argv, int bank);                                 // CT3 0x018e7eb0
int tiXcmd_CheckCurrentScene(int argc, char **argv, int bank);                           // CT3 0x018e7e20
int tiXcmd_SetCharacterEdgeShading(int argc, char **argv, int bank);                     // CT3 0x018e4b40
int tiXcmd_LightBeltStart(int argc, char **argv, int bank);                              // CT3 0x018e4b30
int tiXcmd_LightBeltEnd(int argc, char **argv, int bank);                                // CT3 0x018e4b20
int tiXcmd_TranslucenceAlphaAnim(int argc, char **argv, int bank);                       // CT3 0x018e4a20
int tiXcmd_HurikoReset(int argc, char **argv, int bank);                                 // CT3 0x018e4990
int tiXcmd_KeyNumToFrame(int argc, char **argv, int bank);                               // CT3 0x018e5360
int tiXcmd_GetLetterData(int argc, char **argv, int bank);                               // CT3 0x018e5120
int tiXcmd_FadeStart(int argc, char **argv, int bank);                                   // CT3 0x018e47b0
int tiXcmd_BOSS_BattleStart(int argc, char **argv, int bank);                            // CT3 0x018e3460
int tiXcmd_BOSS_BattleEnd(int argc, char **argv, int bank);                              // CT3 0x018e2e70
int tiXcmd_CharaEdgeShadeAnim(int argc, char **argv, int bank);                          // CT3 0x018e43d0
int tiXcmd_GetINDEX_FromLetterList(int argc, char **argv, int bank);                     // CT3 0x018e5030
int tiXcmd_BOSS_SetChain(int argc, char **argv, int bank);                               // CT3 0x018e31d0
int tiXcmd_XFade_Break(int argc, char **argv, int bank);                                 // CT3 0x018e4390
int tiXcmd_ChangeCam_Reserve(int argc, char **argv, int bank);                           // CT3 0x018e4c90
int tiXcmd_ChangeCam_Execute(int argc, char **argv, int bank);                           // CT3 0x018e4c50
int wideXcmd_GetNumberOfBeam(int argc, char **argv, int bank);                           // CT3 0x018e7d60
int tiXcmd_CheckEarthquake(int argc, char **argv, int bank);                             // CT3 0x018e2d10
int wideXcmd_Delay(int argc, char **argv, int bank);                                     // CT3 0x018e8f00
int tiXcmd_BOSS_SetWarp(int argc, char **argv, int bank);                                // CT3 0x018e2e90
int tiXcmd_ShadeOff(int argc, char **argv, int bank);                                    // CT3 0x018e41f0
int tiXcmd_Init_SPAttack_SCISSORS_MAN(int argc, char **argv, int bank);                  // CT3 0x018e2b30
int tiXcmd_End_SPAttack_SCISSORS_MAN(int argc, char **argv, int bank);                   // CT3 0x018e2ae0
int tiXcmd_SPAttack_SCISSORS_MAN(int argc, char **argv, int bank);                       // CT3 0x018e22f0
int tiXcmd_CheckinLetterList(int argc, char **argv, int bank);                           // CT3 0x018e4cf0
int tiXcmd_FreeGhostPole(int argc, char **argv, int bank);                               // CT3 0x018e7b70
int tiXcmd_SetGhostPole(int argc, char **argv, int bank);                                // CT3 0x018e7bf0
int tiXcmd_BOSS_SetHitBacks(int argc, char **argv, int bank);                            // CT3 0x018e33d0
int wideXcmd_GetRate(int argc, char **argv, int bank);                                   // CT3 0x018e79d0
int wideXcmd_GetCharaData(int argc, char **argv, int bank);                              // CT3 0x018e78c0
int tiXcmd_WaitUntilBrainFree(int argc, char **argv, int bank);                          // CT3 0x018e7870
int tiXcmd_BOSS_SetChain_Alyssa(int argc, char **argv, int bank);                        // CT3 0x018e2f70
int tiXcmd_CheckRange(int argc, char **argv, int bank);                                  // CT3 0x018e2170
int tiXcmd_GetBloodAttackObject(int argc, char **argv, int bank);                        // CT3 0x018e20c0
