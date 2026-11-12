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
#define DOOR_FLAG_OPEN 500              /* + SCN_DOOR.flag */
#define DOOR_FLAG_600 600               /* names wait for the door actors that set them */
#define DOOR_FLAG_700 700
#define DOOR_FLAG_COUNT 99

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
typedef struct DTRG_PRA_DOR {
    u_short state;                      /* 0x00: 0 at init */
    u_char b2;
    u_char next;                        /* 0x03: the next record (free list or a head's chain), DTRG_END ends */
    u_char pad04[0xc];
    u_int w10;                          /* 0x10..0x18: cleared by tkXDTrg_CloseCancel */
    u_int w14;
    u_int w18;
    u_char pad1c[6];
    u_short h22;                        /* 0x22..0x26: cleared by tkXDTrg_CloseCancel */
    u_short h24;
    u_short h26;
    u_char pad28[3];
    u_char b2b;                         /* 0x2b..0x2d: cleared by tkXDTrg_CloseCancel */
    u_char b2c;
    u_char b2d;
    u_char pad2e[0x84 - 0x2e];
} DTRG_PRA_DOR;
static_assert(sizeof(DTRG_PRA_DOR) == 0x84, "DTRG_PRA_DOR");
static_assert(offsetof(DTRG_PRA_DOR, w10) == 0x10, "DTRG_PRA_DOR.w10");
static_assert(offsetof(DTRG_PRA_DOR, h22) == 0x22, "DTRG_PRA_DOR.h22");
static_assert(offsetof(DTRG_PRA_DOR, b2b) == 0x2b, "DTRG_PRA_DOR.b2b");
#define DTRG_PRA_DOR_MAX 32
#define DTRG_END 0xff                   /* end of a chain / the free list */
extern DTRG_PRA_DOR DTrgPraDor[DTRG_PRA_DOR_MAX];                                        // CT3 0x01c8c530
extern u_char DTrgPraDorTop;                                                             // CT3 0x01944900

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
