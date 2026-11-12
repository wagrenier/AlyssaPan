// game/tkXDTrg_GetSceneHash.h -- CT3 game/tkXDTrg_GetSceneHash.c: the door triggers (tkXDTrg_*), the
// branch scene tables (ScnTbl) and the door flags. Slice 1 (2026-11-11) read the tables and the small
// functions; the actor procs and tkBri_DoorDraw are later slices.
#pragma once

#include <stddef.h>

#include "sdk/scetypes.h"

// ---- the branch scene tables (ScnTbl, .rodata 0x0193a970..0x0193b908) ------------------------------------
// The game is cut into branches (TRIG_VAR_BRANCH, 1..5; tk_GetBranchNo); each has its scenes (rooms),
// the doors between them, the scenes two doors away (Nx2: tkBri_DoorDraw drops them) and tables a later
// slice names (Lnk; branch 1 also DorExt / DorPol).
// A scene's entry indexes runs of those tables. Field names from their readers; the rest raw.
typedef struct SCN_ENTRY {
    u_int hash;                         /* 0x0: the room's hash (GetRoomName_file: its files' first 4 chars) */
    u_char lnk;                         /* 0x4: first entry of the branch's Lnk table (read by no slice yet) */
    u_char lnkCount;                    /* 0x5 */
    u_char b6;                          /* 0x6: 0x28 in every entry seen */
    u_char b7;                          /* 0x7: 0x03 in every entry seen */
    u_char door;                        /* 0x8: first door of the branch's door table (SceneDoorFlagClear) */
    u_char doorCount;                   /* 0x9 */
    u_char nx2;                         /* 0xa: first byte of the branch's Nx2 table: scene indices (tkBri_DoorDraw) */
    u_char nx2Count;                    /* 0xb */
    u_char bc[4];                       /* 0xc: zero except in branch 3 */
} SCN_ENTRY;
static_assert(sizeof(SCN_ENTRY) == 0x10, "SCN_ENTRY");

// One door of a branch's door table (2 bytes).
typedef struct SCN_DOOR {
    u_char flag;                        /* 0x0: the door's flag: global event DOOR_FLAG_OPEN + flag */
    u_char scene;                       /* 0x1: the scene behind the door (index into the branch's scenes) */
} SCN_DOOR;
static_assert(sizeof(SCN_DOOR) == 2, "SCN_DOOR");

// One branch of ScnTbl. layout: host (0x1c bytes on the EE; host pointers here: the table is .rodata,
// never written, and only this TU's code reads it).
typedef struct SCN_BRANCH {
    const SCN_ENTRY *scn;               /* EE 0x00: ScnBr<n>Tbl */
    const u_char *lnk;                  /* EE 0x04: Br<n>LnkTbl */
    const SCN_DOOR *door;               /* EE 0x08: Br<n>DorTbl */
    const u_char *doorExt;              /* EE 0x0c: Br1DorExtTbl (branch 1 only, not in entry 0) */
    const u_char *doorPol;              /* EE 0x10: Br1DorPolTbl (the same) */
    const u_char *nx2;                  /* EE 0x14: Br<n>Nx2Tbl */
    u_short scnCount;                   /* EE 0x18 */
    u_short pad;                        /* EE 0x1a */
} SCN_BRANCH;                           /* layout: host (EE 0x1c bytes) */
#define SCN_BRANCH_MAX 6                /* entries 0..5: tk_GetBranchNo answers 1..5; entry 0 is branch 1 without Ext / Pol */
extern const SCN_BRANCH ScnTbl[SCN_BRANCH_MAX];                                          // CT3 0x0193b860

#define TRIG_VAR_BRANCH 0x1d            /* the branch the player is in (global variable; < 1 reads as 1) */
#define SCN_NONE (-1)                   /* tkScn_GetId: the room is not a scene of the branch */

// ---- the door flags (global events) ------------------------------------------------------------------------
// Three runs of DOOR_FLAG_COUNT global events, one per door flag; tkXcmd_DoorFlagClear clears all three in a
// save's flags, SceneDoorFlagClear the OPEN run for one scene's doors.
// The actor procs (slice 2) give the runs their meaning: an opening door registers OPEN at once (its
// neighbours drawn), MOVING with its sound, OPENED when the animation ends; closing removes MOVING and
// OPENED first and OPEN when it has shut.
#define DOOR_FLAG_OPEN 500              /* + SCN_DOOR.flag: the door is open (or opening) */
#define DOOR_FLAG_OPENED 600            /* its opening animation has finished */
#define DOOR_FLAG_MOVING 700            /* its opening has begun (the sound played) */
#define DOOR_FLAG_COUNT 99
#define DOOR_EVENT_CAMERA 599           /* DOOR_FLAG_OPEN + DOOR_FLAG_COUNT: tkXDTrg_ActorProc_AlsCam cut to the door's camera */

// ---- the door triggers' state (bss) ------------------------------------------------------------------------
// DTrgHed (0x019448a0, 96 bytes): eight heads; each names a scene bank (bank) and the chain of DTrgPraDor
// records working on it (first; DTRG_END ends a chain). Fields from tkXcmd_DoorTrigger_Init and
// tkXDTrg_CloseCancel; the words at +0 / +8 wait for the actor procs.
typedef struct DTRG_HED {
    u_int w0;                           /* 0x0: 0 at init */
    short bank;                         /* 0x4: the scene bank the chain works in, -1 = free (tkBri_DoorDraw passes the bank to tkXDTrg_CloseCancel) */
    u_char first;                       /* 0x6: the first DTrgPraDor record of the chain, DTRG_END = none */
    u_char b7;                          /* 0x7: 0xff at init */
    u_int w8;                           /* 0x8: 0 at init */
} DTRG_HED;
static_assert(sizeof(DTRG_HED) == 0xc, "DTRG_HED");
#define DTRG_HED_MAX 8
#define DTRG_BANK_NONE (-1)
extern DTRG_HED DTrgHed[DTRG_HED_MAX];                                                   // CT3 0x019448a0

// DTrgPraDor (0x01c8c530, 32 records of 0x84 bytes): a pool linked by next (free list head DTrgPraDorTop).
// One record is one door actor. Its procs (tkXDTrg_ActorProc_*) each own a slot: a wait (a countdown, or
// ssTrigWaitByAnimFrame's state, -1 = none), its frames and the proc's step; a proc answers 1 when done and
// then zeroes its wait and step. Fields from the actor procs (slice 2, 2026-11-11); the rest raw.
enum {
    DTRG_SLOT_0 = 0,                    /* AxeOpn / HamOpn set it to -1 (a later slice names it) */
    DTRG_SLOT_CAMERA = 1,               /* tkXDTrg_ActorProc_AlsCam */
    DTRG_SLOT_2 = 2,                    /* AxeOpn / HamOpn clear it */
    DTRG_SLOT_OPEN = 3,                 /* the kinds' Opn / OpnOut */
    DTRG_SLOT_CLOSE = 4,                /* the kinds' Clo */
    DTRG_SLOT_CLOSE_COMMON = 5,         /* tkXDTrg_ActorProc_CmnClo */
    DTRG_SLOT_COUNT = 6,
};
// The door kinds: each has its key entry (DTRG_PRA_DOR.kind) and two opening speeds (DTrgPraMon).
enum {
    DTRG_KIND_0 = 0,                    /* no proc of slice 2 is its own */
    DTRG_KIND_HAM = 1,                  /* tkXDTrg_ActorProc_Ham*: the hammer's door (SE bank "E_HAMMER") */
    DTRG_KIND_VIT = 2,                  /* tkXDTrg_ActorProc_Vit* */
    DTRG_KIND_AXE = 3,                  /* tkXDTrg_ActorProc_Axe*: the axe's door (SE bank "E_ONO") */
    DTRG_KIND_SCM = 4,                  /* tkXDTrg_ActorProc_ScM* */
    DTRG_KIND_SCW = 5,                  /* tkXDTrg_ActorProc_ScW* */
    DTRG_KIND_COUNT = 6,
};
// One kind's entry: an event that keeps the door from closing while set (the door's own kind excepted)
// and the sound object its opening sound is heard from.
typedef struct DTRG_KIND_KEY {
    short event;                        /* 0x0: -1 = none */
    short seObj;                        /* 0x2: SdSetParamSE's sound object */
    short h4;
    short h6;
} DTRG_KIND_KEY;
static_assert(sizeof(DTRG_KIND_KEY) == 8, "DTRG_KIND_KEY");

#define DTRG_FLAG_SIDE 0x2              /* flags: the door is worked from its other side: the kind's other speed;
                                           at Axe / Ham doors the weapon's sound (the side the enemy comes from?) */
#define DTRG_FLAG_SE_ALT 0x8            /* flags: the second door sound of each pair (DOOR_SE_*_ALT) */
typedef struct DTRG_PRA_DOR {
    u_short flags;                      /* 0x00: DTRG_FLAG_*; 0 at init */
    u_char doorFlag;                    /* 0x02: the door's flag (DOOR_FLAG_* + doorFlag) */
    u_char next;                        /* 0x03: the next record (free list or a head's chain), DTRG_END ends */
    int wait[DTRG_SLOT_COUNT];          /* 0x04 */
    short frames[DTRG_SLOT_COUNT];      /* 0x1c: ssTrigWaitByAnimFrame's frames (AlsCam) */
    u_char step[DTRG_SLOT_COUNT];       /* 0x28 */
    char camRoom[0xa];                  /* 0x2e: AlsCam's camera room; only its first 4 chars are read (in a
                                           camera record the name may run on over the next fields) */
    short otherBank;                    /* 0x38: a second scene bank animated with the door, -1 = none */
    u_short part;                       /* 0x3a: the door's model part (its animation) */
    u_short trigPortal;                 /* 0x3c: HitPtl: the part enabled while the door is open */
    short camCut;                       /* 0x3e: AlsCam's camera cut */
    u_char pad40[2];
    u_short trigHitClosed;              /* 0x42: HitPtl: the part enabled while the door does not move */
    u_short trigHitOpen;                /* 0x44: HitPtl: the part enabled once it moves */
    short seObjClose;                   /* 0x46: CmnClo's sound object */
    u_char pad48[2];
    short seObjOpen;                    /* 0x4a: Axe / Ham doors' opening sound object without DTRG_FLAG_SIDE */
    u_char pad4c[2];
    u_short trigUse;                    /* 0x4e: enabled when the door opens, disabled by AlsCam */
    short event;                        /* 0x50: keeps the door from closing while set */
    DTRG_KIND_KEY kind[DTRG_KIND_COUNT]; /* 0x52 */
    u_char pad82[2];
} DTRG_PRA_DOR;
static_assert(sizeof(DTRG_PRA_DOR) == 0x84, "DTRG_PRA_DOR");
static_assert(offsetof(DTRG_PRA_DOR, wait) == 0x04, "DTRG_PRA_DOR.wait");
static_assert(offsetof(DTRG_PRA_DOR, frames) == 0x1c, "DTRG_PRA_DOR.frames");
static_assert(offsetof(DTRG_PRA_DOR, step) == 0x28, "DTRG_PRA_DOR.step");
static_assert(offsetof(DTRG_PRA_DOR, camRoom) == 0x2e, "DTRG_PRA_DOR.camRoom");
static_assert(offsetof(DTRG_PRA_DOR, otherBank) == 0x38, "DTRG_PRA_DOR.otherBank");
static_assert(offsetof(DTRG_PRA_DOR, camCut) == 0x3e, "DTRG_PRA_DOR.camCut");
static_assert(offsetof(DTRG_PRA_DOR, trigHitClosed) == 0x42, "DTRG_PRA_DOR.trigHitClosed");
static_assert(offsetof(DTRG_PRA_DOR, seObjClose) == 0x46, "DTRG_PRA_DOR.seObjClose");
static_assert(offsetof(DTRG_PRA_DOR, seObjOpen) == 0x4a, "DTRG_PRA_DOR.seObjOpen");
static_assert(offsetof(DTRG_PRA_DOR, trigUse) == 0x4e, "DTRG_PRA_DOR.trigUse");
static_assert(offsetof(DTRG_PRA_DOR, kind) == 0x52, "DTRG_PRA_DOR.kind");
#define DTRG_PRA_DOR_MAX 32
#define DTRG_END 0xff                   /* end of a chain / the free list */
#define DTRG_WAIT_NONE (-1)
extern DTRG_PRA_DOR DTrgPraDor[DTRG_PRA_DOR_MAX];                                        // CT3 0x01c8c530
extern u_char DTrgPraDorTop;                                                             // CT3 0x01944900

// DTrgPraMon (bss 0x01944910, 24 bytes): two opening speeds per door kind (percent of the animation's
// rate; only the low byte is used): OpnOut takes [kind][0] with DTRG_FLAG_SIDE and [kind][1] without, Opn
// the other way round. Written by code no slice has read yet (tkXcmd_DoorTrigger?).
extern u_short DTrgPraMon[DTRG_KIND_COUNT][2];                                           // CT3 0x01944910

// The door sounds (SSPlaySE ids). Bank SE_BANK_COMMON ("C_0000", 0x01936e38 in this TU's .rodata).
#define DOOR_SE_OPEN 0xb
#define DOOR_SE_CLOSE 0xc
#define DOOR_SE_OPEN_ALT 0xe            /* DTRG_FLAG_SE_ALT */
#define DOOR_SE_CLOSE_ALT 0xf
#define DOOR_SE_BANK_AXE "E_ONO"        /* 0x01936e40 */
#define DOOR_SE_AXE 0x1b
#define DOOR_SE_AXE_ALT 0x1c
#define DOOR_SE_BANK_HAMMER "E_HAMMER"  /* 0x01936e48 */
#define DOOR_SE_HAMMER 0x4
#define DOOR_SE_HAMMER_ALT 0x3
#define DOOR_SPEED_PERCENT 100.0f       /* the anim procs' speed argument: percent */
#define DOOR_ANIM_FRAME 10              /* the anim procs' frames: tenths of the animation's keys */

// ---- functions -----------------------------------------------------------------------------------------
u_long tkXDTrg_GetSceneHash(const char *name);                                           // CT3 0x018e90a0
void tkXDTrg_CloseCancel(short bank);                                                    // CT3 0x018ecfd0
char tk_GetBranchNo(void);                                                               // CT3 0x018ee560
short tkScn_GetId(u_long roomHash);                                                      // CT3 0x018ee4c0
// tkBri_DoorDraw's mode: the x command SceneDraw's first argument's first character.
#define DOOR_DRAW_CLOSE '0'             /* drop the drawn scenes no open door of the room leads to */
#define DOOR_DRAW_OPEN '1'              /* draw the scenes behind the room's open doors */
#define DOOR_DRAW_BOTH '2'              /* OPEN, then CLOSE */
#define DOOR_DRAW_NEIGHBOURS 8          /* distinct scenes behind one room's doors */
#define DOOR_DRAW_PART_PREFIX "LP_"     /* @2371: the current scene's part a drawn neighbour hangs on: "LP_" + its name, lower case */
int tkBri_DoorDraw(char mode);                                                           // CT3 0x018edf40

void tkXcmd_DoorTrigger_Init(void);                                                      // CT3 0x018ed880
// From data_save_set / data_load_set (Character_Init.c, 2026-11-10): a save's global event flags.
void tkXcmd_DoorFlagClear(u_char *eventFlags);                                           // CT3 0x018ed320
void SceneDoorFlagClear(u_long roomHash);                                                // CT3 0x018eda60
// From tkXcmd_CharSceneChange (tiXcmd_TransItem.c, 2026-11-11): a room's hash -> the bank that draws it
// (stored as CHARACTER.lightBank, a short). The ROM returns an int; -1 = none.
short tkBri_DoorDraw_GetBnkId(u_long roomHash);                                          // CT3 0x018ede90

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int tkXcmd_AiSoundPosition(int argc, char **argv, int bank);                             // CT3 0x018ee5e0
int tkXcmd_SceneDraw(int argc, char **argv, int bank);                                   // CT3 0x018ee5b0
int tkXcmd_WaitUntil(int argc, char **argv, int bank);                                   // CT3 0x018ee5a0

// WaitUntil's operands and operator (tkXcmdS_WaitUntil_Value / tkXcmdS_WaitUntil; scopes: XCMD_SCOPE_*).
enum {
    WAIT_OPERAND_UNSET = -2,            /* no kind letter: the number itself */
    WAIT_OPERAND_LITERAL = -1,          /* "n" / "N" */
    WAIT_OPERAND_FLAG = 0,              /* "f" / "F": an event (ssTrigCheckEvent, 0 / 1) */
    WAIT_OPERAND_VARIABLE = 1,          /* "v" / "V": a variable (ssTrigGetVariable) */
};
#define WAIT_SCOPE_UNSET (-2)           /* no "g" / "t": the accessors read it as global (non-zero) */
#define WAIT_OP_EQ 1                    /* '=' */
#define WAIT_OP_NOT 2                   /* '!' */
#define WAIT_OP_LT 4                    /* '<' */
#define WAIT_OP_GT 8                    /* '>' */
int tkXcmdS_WaitUntil_Value(char **arg, int bank);                                       // CT3 0x018edb40
int tkXcmdS_WaitUntil(int argc, char **argv, int bank);                                  // CT3 0x018edcd0
int tkXcmd_DoorTrigger(int argc, char **argv, int bank);                                 // CT3 0x018ed9e0

// ---- the door actor procs (slice 2, 2026-11-11) ----
// Animate a door's part (and, with otherBank != -1, the same-named part of a second bank) from frame `from`
// to `to` (DOOR_ANIM_FRAME units) at `speed` percent; ToFrame runs from wherever the part is.
void tkXDTrg_ActorProc_AnimToFrameDoor(int bank, u_short part, int frame, u_int speed, int toEnd,
                                       int stepFromRuns, short otherBank);               // CT3 0x018e90e0
void tkXDTrg_ActorProc_AnimRangeDoor(int bank, u_short part, int from, int to, u_int speed, int started,
                                     int stepFromRuns, short otherBank);                 // CT3 0x018e9240
void tkXDTrg_ActorProc_ChangeCamera(int bank, const char *room, short cut, u_char mode, int ms); // CT3 0x018e93b0
int tkXDTrg_ActorProc_CmnClo(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018e9480
int tkXDTrg_ActorProc_ScWClo(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018e9710
int tkXDTrg_ActorProc_ScWOpnOut(int bank, DTRG_PRA_DOR *door);                           // CT3 0x018e98c0
int tkXDTrg_ActorProc_ScWOpn(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018e9ae0
int tkXDTrg_ActorProc_ScMClo(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018e9d00
int tkXDTrg_ActorProc_ScMOpnOut(int bank, DTRG_PRA_DOR *door);                           // CT3 0x018e9eb0
int tkXDTrg_ActorProc_ScMOpn(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018ea0d0
int tkXDTrg_ActorProc_AxeClo(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018ea2f0
int tkXDTrg_ActorProc_AxeOpnOut(int bank, DTRG_PRA_DOR *door);                           // CT3 0x018ea4a0
int tkXDTrg_ActorProc_AxeOpn(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018ea720
int tkXDTrg_ActorProc_VitClo(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018ea9c0
int tkXDTrg_ActorProc_VitOpnOut(int bank, DTRG_PRA_DOR *door);                           // CT3 0x018eab70
int tkXDTrg_ActorProc_VitOpn(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018ead90
int tkXDTrg_ActorProc_HamClo(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018eafb0
int tkXDTrg_ActorProc_HamOpnOut(int bank, DTRG_PRA_DOR *door);                           // CT3 0x018eb160
int tkXDTrg_ActorProc_HamOpn(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018eb3e0
int tkXDTrg_ActorProc_Sht(int bank, DTRG_PRA_DOR *door);                                 // CT3 0x018eb680
int tkXDTrg_ActorProc_HitPtl(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018eb730
int tkXDTrg_ActorProc_AlsCam(int bank, DTRG_PRA_DOR *door);                              // CT3 0x018eb840
void tkXDTrg_ActorProc_Ini_Error(void);                                                  // CT3 0x018ebf80
void tkXDTrg_MonDraw(int draw);                                                          // CT3 0x018ed090
