// game/tkXDTrg_GetSceneHash.c -- Clock Tower 3, 0x018e90a0..0x018ee5f0: the door triggers (tkXDTrg_*: the
// door actors' procs, close / cancel), the door bridge draw (tkBri_*), the branch scene tables (ScnTbl and
// the Br<n>* tables) and the tk x commands. 50 functions; tumap.py keyed the file by its first function.
//
// Slice 1 (2026-11-11): the branch scene tables (dumped from the ELF), tk_GetBranchNo, tkScn_GetId,
// tkXDTrg_GetSceneHash, the door-trigger state's init (tkXcmd_DoorTrigger_Init), the door flags
// (tkXcmd_DoorFlagClear, SceneDoorFlagClear), tkXDTrg_CloseCancel, the door bridge draw (tkBri_DoorDraw:
// the scenes behind a room's open doors drawn, the rest dropped; tkBri_DoorDraw_GetBnkId) with its x
// command SceneDraw, tkXcmd_AiSoundPosition and tkXcmd_WaitUntil (tkXcmdS_WaitUntil_Value).
// Slice 2 (2026-11-11): the door actor procs (AnimToFrameDoor .. AlsCam, Ini_Error): the record's slots,
// the kinds' open / close and the door flags' three runs. Left: tkXDTrg_ActorProc (the dispatcher), _Ini,
// _AlsOpn, tkXDTrg_MonDraw, tkXcmd_DoorTrigger and the rest of the 50.

#include "game/tkXDTrg_GetSceneHash.h"

#include "common/alyssapan_stub.h"
#include "game/Character_Init.h"
#include "game/QuatSlerp.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/ct3_types.h"
#include "game/fileio.h"
#include "game/main2.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/tiAvoidObject.h"
#include "game/tiXcmd_TransItem.h"
#include "game/unassigned_data.h"

#include <stdlib.h>
#include <string.h>

DTRG_HED DTrgHed[DTRG_HED_MAX];                 /* bss  0x019448a0 */
u_char DTrgPraDorTop;                           /* bss  0x01944900 */
u_short DTrgPraMon[DTRG_KIND_COUNT][2];         /* bss  0x01944910 */
DTRG_PRA_DOR DTrgPraDor[DTRG_PRA_DOR_MAX];      /* bss  0x01c8c530 */

#define SCENE_HASH_LEN 4                /* a scene is named by its first four characters */

// CT3 0x018e90a0
// A scene's hash from a name: its first four characters, upper-cased (the same hash GetRoomName_file gives).
u_long tkXDTrg_GetSceneHash(const char *name)
{
    char buf[SCENE_HASH_LEN + 1];

    memcpy(buf, name, SCENE_HASH_LEN);
    buf[SCENE_HASH_LEN] = '\0';
    UpperCase(buf);
    return ahGetHash(buf);
}

// CT3 0x018ecfd0
// Cancels the door motions in a scene bank: every DTrgPraDor record on its head's chain loses its motion state.
void tkXDTrg_CloseCancel(short bank)
{
    int i;
    u_char r;

    for (i = 0; i < DTRG_HED_MAX && bank != DTrgHed[i].bank; i++) {
    }
    if (i >= DTRG_HED_MAX) {
        return;
    }
    for (r = DTrgHed[i].first; r != DTRG_END; r = DTrgPraDor[r].next) {
        DTRG_PRA_DOR *p = &DTrgPraDor[r];
        p->wait[DTRG_SLOT_OPEN] = 0;
        p->wait[DTRG_SLOT_CLOSE] = 0;
        p->wait[DTRG_SLOT_CLOSE_COMMON] = 0;
        p->frames[DTRG_SLOT_OPEN] = 0;
        p->frames[DTRG_SLOT_CLOSE] = 0;
        p->frames[DTRG_SLOT_CLOSE_COMMON] = 0;
        p->step[DTRG_SLOT_OPEN] = 0;
        p->step[DTRG_SLOT_CLOSE] = 0;
        p->step[DTRG_SLOT_CLOSE_COMMON] = 0;
    }
}

// CT3 0x018ed320
// Clears the three door-flag runs in a set of global event flags (a save's, on save and load).
static inline void DoorFlagBitClear(u_char *flags, u_int n)
{
    n &= 0xffff;
    flags[n >> 3] &= ~(u_char)(1 << (n & 7));
}

void tkXcmd_DoorFlagClear(u_char *eventFlags)
{
    int i;

    for (i = 0; i < DOOR_FLAG_COUNT; i++) {
        DoorFlagBitClear(eventFlags, DOOR_FLAG_OPEN + i);
        DoorFlagBitClear(eventFlags, DOOR_FLAG_OPENED + i);
        DoorFlagBitClear(eventFlags, DOOR_FLAG_MOVING + i);
    }
}

// CT3 0x018ed880
// The door triggers' state at boot: every head free, the record pool one free list (0 -> 1 -> ... -> 31).
void tkXcmd_DoorTrigger_Init(void)
{
    int i;

    for (i = 0; i < DTRG_HED_MAX; i++) {
        DTrgHed[i].roomHash = 0;
        DTrgHed[i].bank = DTRG_BANK_NONE;
        DTrgHed[i].first = DTRG_END;
        DTrgHed[i].last = DTRG_END;
        DTrgHed[i].count = 0;
    }
    for (i = 0; i < DTRG_PRA_DOR_MAX; i++) {
        DTrgPraDor[i].flags = 0;
        DTrgPraDor[i].next = (u_char)(i + 1);
    }
    DTrgPraDor[DTRG_PRA_DOR_MAX - 1].next = DTRG_END;
    DTrgPraDorTop = 0;
}

// CT3 0x018eda60
// Closes every door of a scene: its doors' OPEN flags (global events) are removed.
void SceneDoorFlagClear(u_long roomHash)
{
    char branch = tk_GetBranchNo();
    short id = tkScn_GetId(roomHash);
    const SCN_ENTRY *scn;
    const SCN_DOOR *door;
    int i;

    if (id == SCN_NONE) {
        return;
    }
    scn = &ScnTbl[branch].scn[id];
    door = &ScnTbl[branch].door[scn->door];
    for (i = 0; i < (int)scn->doorCount; i++, door++) {
        ssTrigRemoveEvent(door->flag + DOOR_FLAG_OPEN, TRIG_SET_GLOBAL, -1);
    }
}

// CT3 0x018ede90
// The bank that holds a room: the first loaded scene bank (BANK_TYPE_SHADOW) whose file names the room.
short tkBri_DoorDraw_GetBnkId(u_long roomHash)
{
    int i;

    for (i = 0; i < BANK_MAX; i++) {
        if (BankInfo[i].state != BANK_STATE_FREE && BankInfo[i].type == BANK_TYPE_SHADOW &&
            roomHash == GetRoomName_file(BankInfo[i].name)) {
            return (short)i;
        }
        if (BankInfo[i].state != BANK_STATE_FREE) {
            i += BankInfo[i].groupCount - 1;  // skip the rest of the bank's group
        }
    }
    return -1;
}

// A scene's name (asConvertHash2String of its hash) into name, and the name's first word, the key
// ssTrigCheckSceneDraw takes.
static int DoorDrawSceneName(const SCN_ENTRY *scn, char *name)
{
    int word;

    strcpy(name, asConvertHash2String(scn->hash));
    memcpy(&word, name, sizeof(word));
    return word;
}

// Drops a drawn neighbour scene: its bank stops drawing and any door motion on it is cancelled.
static void DoorDrawDrop(const SCN_ENTRY *scn, char *name)
{
    short bank = tkBri_DoorDraw_GetBnkId(scn->hash);

    TrgSceneDraw(0, 0, 0, name, bank);
    tkXDTrg_CloseCancel(bank);
}

// The neighbour slot of a scene, or DOOR_DRAW_NEIGHBOURS. PORT: the ROM's search has no bound (the
// door's scene is always in the list it built); the host stops at the list's end.
static int DoorDrawFind(u_char nb[][2], u_char scene)
{
    int k;

    for (k = 0; k < DOOR_DRAW_NEIGHBOURS && nb[k][0] != scene; k++) {
    }
    return k;
}

// CT3 0x018edf40
// The scenes around the player's room (Character[0]'s), by its doors: OPEN draws each neighbour behind
// an open door (its DOOR_FLAG_OPEN event set) that is not drawn yet, hung on the current scene's part
// "LP_<name>"; CLOSE drops each drawn neighbour no open door leads to; both drop the scenes two doors away
// (the Nx2 run). Always 1 (an x command's "done").
int tkBri_DoorDraw(char mode)
{
    const SCN_BRANCH *br = &ScnTbl[(int)tk_GetBranchNo()];
    short id = tkScn_GetId(Character[0].roomHash);
    const SCN_ENTRY *scnTbl;
    const SCN_ENTRY *scn;
    const SCN_DOOR *door;
    const u_char *nx2;
    u_char nb[DOOR_DRAW_NEIGHBOURS][2];  // (scene, open) pairs; 0xff ends the list
    char name[16];                       // PORT: the ROM's 8-byte buffers, widened
    char part[16];
    int count, i, k;

    // callcheck: asConvertHash2String, tkXDTrg_CloseCancel (through DoorDrawSceneName / DoorDrawDrop, the ROM's inlined repeats)
    if (id == SCN_NONE) {
        return 1;
    }
    scnTbl = br->scn;
    scn = &scnTbl[id];
    count = scn->doorCount;
    if (count != 0) {
        memset(nb, 0, sizeof(nb));
        for (k = 0; k < DOOR_DRAW_NEIGHBOURS; k++) {
            nb[k][0] = 0xff;
        }
        // The distinct scenes behind the doors (at most 8: the ROM's search stops at slot 7).
        door = &br->door[scn->door];
        for (i = 0; i < count; i++, door++) {
            for (k = 0; k < DOOR_DRAW_NEIGHBOURS - 1 && nb[k][0] != 0xff && door->scene != nb[k][0]; k++) {
            }
            if (nb[k][0] == 0xff) {
                nb[k][0] = door->scene;
            }
        }
        if (mode == DOOR_DRAW_OPEN || mode == DOOR_DRAW_BOTH) {
            for (k = 0; k < DOOR_DRAW_NEIGHBOURS && nb[k][0] != 0xff; k++) {
                nb[k][1] = 0;
            }
            door = &br->door[scn->door];
            for (i = 0; i < count; i++, door++) {
                k = DoorDrawFind(nb, door->scene);
                if (k < DOOR_DRAW_NEIGHBOURS && nb[k][1] == 0 &&
                    (char)ssTrigCheckEvent(door->flag + DOOR_FLAG_OPEN, TRIG_SET_GLOBAL, -1) == 1) {
                    nb[k][1] = 1;
                }
            }
            for (k = 0; k < DOOR_DRAW_NEIGHBOURS && nb[k][0] != 0xff; k++) {
                if (nb[k][1] != 1) {
                    continue;
                }
                if ((char)ssTrigCheckSceneDraw(DoorDrawSceneName(&scnTbl[nb[k][0]], name)) == 0) {
                    const ALL_TABLE *tbl = All_table_TBL[idCurrentScene];
                    int partId;
                    short bank;

                    strcpy(part, DOOR_DRAW_PART_PREFIX);
                    strcat(part, name);
                    LowerCase(part + sizeof(DOOR_DRAW_PART_PREFIX) - 1);
                    partId = asFindModelPartsID(BANK_PTR(u_char *, idCurrentScene, tbl->partsHeader),
                                                ahGetHash(part));
                    bank = tkBri_DoorDraw_GetBnkId(scnTbl[nb[k][0]].hash);
                    TrgSceneDraw(1, idCurrentScene, (short)partId, name, bank);
                }
            }
        }
        if (mode == DOOR_DRAW_CLOSE || mode == DOOR_DRAW_BOTH) {
            for (k = 0; k < DOOR_DRAW_NEIGHBOURS; k++) {
                nb[k][1] = 1;
            }
            door = &br->door[scn->door];
            for (i = 0; i < count; i++, door++) {
                k = DoorDrawFind(nb, door->scene);
                if (k < DOOR_DRAW_NEIGHBOURS && nb[k][1] == 1 &&
                    (char)ssTrigCheckEvent(door->flag + DOOR_FLAG_OPEN, TRIG_SET_GLOBAL, -1) == 1) {
                    nb[k][1] = 0;
                }
            }
            for (k = 0; k < DOOR_DRAW_NEIGHBOURS && nb[k][0] != 0xff; k++) {
                if (nb[k][1] == 1 &&
                    (char)ssTrigCheckSceneDraw(DoorDrawSceneName(&scnTbl[nb[k][0]], name)) == 1) {
                    DoorDrawDrop(&scnTbl[nb[k][0]], name);
                }
            }
        }
    }
    // The scenes two doors away are never drawn from here.
    nx2 = br->nx2 + scn->nx2;
    for (i = 0; i < (int)scn->nx2Count; i++, nx2++) {
        if ((char)ssTrigCheckSceneDraw(DoorDrawSceneName(&scnTbl[*nx2], name)) == 1) {
            DoorDrawDrop(&scnTbl[*nx2], name);
        }
    }
    return 1;
}

// CT3 0x018ee4c0
// A room's index among the scenes of the current branch, SCN_NONE when the branch has no such scene.
short tkScn_GetId(u_long roomHash)
{
    const SCN_BRANCH *br = &ScnTbl[(int)tk_GetBranchNo()];
    int i;

    for (i = 0; i < (int)br->scnCount && roomHash != br->scn[i].hash; i++) {
    }
    if (i >= (int)br->scnCount) {
        i = SCN_NONE;
    }
    return (short)i;
}

// CT3 0x018ee560
// The branch the player is in (TRIG_VAR_BRANCH), at least 1.
char tk_GetBranchNo(void)
{
    char branch = (char)ssTrigGetVariable(TRIG_VAR_BRANCH, TRIG_SET_GLOBAL, -1);

    if (branch < 1) {
        branch = 1;
    }
    return branch;
}

// CT3 0x018ee5e0
// An empty x command: always done.
int tkXcmd_AiSoundPosition(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return 1;
}

// CT3 0x018ee5b0
// x command SceneDraw <mode>: tkBri_DoorDraw with the argument's first character (DOOR_DRAW_*).
int tkXcmd_SceneDraw(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    return (u_char)tkBri_DoorDraw(argv[0][0]);
}

// CT3 0x018edb40
// One WaitUntil operand: arg[0] the number, arg[1] up to two letters for what it names (WAIT_OPERAND_*
// kinds, XCMD_SCOPE_* scopes). 'n' / 'N' stops the reading: a literal. Without a kind letter the number
// is the value itself; without a scope letter the scope stays WAIT_SCOPE_UNSET (global to the accessors).
int tkXcmdS_WaitUntil_Value(char **arg, int bank)
{
    const char *p = arg[1];
    int kind = WAIT_OPERAND_UNSET;
    int scope = WAIT_SCOPE_UNSET;
    int value;
    int i;

    for (i = 0; i < 2 && *p != '\0' && kind != WAIT_OPERAND_LITERAL; i++, p++) {
        switch (*p) {
        case 'n': case 'N': kind = WAIT_OPERAND_LITERAL; break;
        case 'g': case 'G': scope = XCMD_SCOPE_GLOBAL; break;
        case 't': case 'T': scope = XCMD_SCOPE_BANK; break;
        case 'v': case 'V': kind = WAIT_OPERAND_VARIABLE; break;
        case 'f': case 'F': kind = WAIT_OPERAND_FLAG; break;
        default: break;
        }
    }
    value = atoi(arg[0]);
    if (kind == WAIT_OPERAND_VARIABLE) {
        value = ssTrigGetVariable(value, scope, bank);
    } else if (kind == WAIT_OPERAND_FLAG) {
        value = ssTrigCheckEvent(value, scope, bank) != 0;
    }
    return value;
}

// CT3 0x018edcd0
// x command WaitUntil <a> <a kind> <op> <b> <b kind>: compares two operands (tkXcmdS_WaitUntil_Value) by
// the operator's up to two characters. The ROM answers 1 (done) when the comparison it names FAILS
// (">" is done once a <= b, "==" once a != b...): read so from the decompiler and kept. An operator it
// does not know answers the number of characters read.
int tkXcmdS_WaitUntil(int argc, char **argv, int bank)
{
    int a = tkXcmdS_WaitUntil_Value(argv, bank);
    int b = tkXcmdS_WaitUntil_Value(argv + 3, bank);
    const char *p = argv[2];
    u_int op = 0;
    u_int n;

    (void)argc;
    for (n = 0; n < 2 && *p != '\0'; n++, p++) {
        if (*p == '>') {
            op |= WAIT_OP_GT;
        } else if (*p == '<') {
            op |= WAIT_OP_LT;
        } else if (*p == '!') {
            op |= WAIT_OP_NOT;
        } else if (*p == '=') {
            op |= WAIT_OP_EQ;
        }
    }
    switch (op) {
    case WAIT_OP_GT:              n = !(b < a); break;
    case WAIT_OP_LT:              n = b <= a; break;
    case WAIT_OP_GT | WAIT_OP_EQ: n = a < b; break;
    case WAIT_OP_LT | WAIT_OP_EQ: n = b < a; break;
    case WAIT_OP_NOT | WAIT_OP_EQ: n = a == b; break;
    case WAIT_OP_EQ:              n = a != b; break;
    default: break;
    }
    return n & 0xff;
}

// CT3 0x018ee5a0
// x command WaitUntil: a jump to tkXcmdS_WaitUntil.
int tkXcmd_WaitUntil(int argc, char **argv, int bank)
{
    return tkXcmdS_WaitUntil(argc, argv, bank);
}

// ---- the door actor procs (slice 2, 2026-11-11) ----
// Each proc runs one step per frame for one door record (DTRG_PRA_DOR) in its slot and answers 1 when its
// work is done; the actor (tkXDTrg_ActorProc, a later slice) calls them. The ROM repeats the Opn / OpnOut /
// Clo bodies per door kind; here the kinds' copies share DoorKindOpen / DoorKindClose, with the kind, the
// direction and the weapon sound as arguments (the bodies are the same instruction for instruction but for
// those constants).

#define DOOR_SPEED_MASK 0xff            /* DTrgPraMon: only the low byte is the speed */
#define DOOR_CAMERA_MS_TICKS 300        /* ChangeCamera: CamChange_S's ticks per 1000 ms */

// The model part's hash in a bank's parts header (MODEL_PARTS.hash), for the second bank's same-named part.
static u_long DoorPartHash(int bank, u_short part)
{
    ALL_TABLE *tbl = All_table_TBL[bank];

    return ((MODEL_PARTS *)BANK_PTR(u_char *, bank, tbl->partsHeader))[part].hash;
}

// CT3 0x018e90e0
// Animates the door's part from where it is to `frame` (and the second bank's part of the same name).
void tkXDTrg_ActorProc_AnimToFrameDoor(int bank, u_short part, int frame, u_int speed, int toEnd,
                                       int stepFromRuns, short otherBank)
{
    float rate = (float)speed / DOOR_SPEED_PERCENT;

    asSceneAnimationStart(rate, 0, All_table_TBL[bank], -1, frame * DOOR_ANIM_FRAME, part, 1, 0, 0, 0, toEnd,
                          stepFromRuns, SCENE_ANIM_SCENE, 0);
    if (otherBank != -1) {
        asSceneAnimationStart(rate, DoorPartHash(bank, part), All_table_TBL[otherBank], -1,
                              frame * DOOR_ANIM_FRAME, 0, 1, 0, 0, 0, toEnd, stepFromRuns, SCENE_ANIM_SCENE, 0);
    }
}

// CT3 0x018e9240
// Animates the door's part from `from` to `to` (and the second bank's part of the same name).
void tkXDTrg_ActorProc_AnimRangeDoor(int bank, u_short part, int from, int to, u_int speed, int started,
                                     int stepFromRuns, short otherBank)
{
    float rate = (float)speed / DOOR_SPEED_PERCENT;

    asSceneAnimationStart(rate, 0, All_table_TBL[bank], from * DOOR_ANIM_FRAME, to * DOOR_ANIM_FRAME, part, 1, 0,
                          0, started, 0, stepFromRuns, SCENE_ANIM_SCENE, 0);
    if (otherBank != -1) {
        asSceneAnimationStart(rate, DoorPartHash(bank, part), All_table_TBL[otherBank], from * DOOR_ANIM_FRAME,
                              to * DOOR_ANIM_FRAME, 0, 1, 0, 0, started, 0, stepFromRuns, SCENE_ANIM_SCENE, 0);
    }
}

// CT3 0x018e93b0
// Cuts to a camera of the door's bank: shadows off, the bank becomes the camera's scene (its room's doors
// redrawn) when it was not, then CamChange_S with the move time in ticks.
void tkXDTrg_ActorProc_ChangeCamera(int bank, const char *room, short cut, u_char mode, int ms)
{
    asShadowAllDisable(SHADOW_DISABLE_BANKS | SHADOW_DISABLE_CHARACTERS);
    if (idCameraScene != bank) {
        idCameraScene = bank;
        haCameraRoom = (u_int)tkXDTrg_GetSceneHash(room);
        SwapDrawingDoor();
    }
    CamChange_S(bank, cut, mode, (ms * DOOR_CAMERA_MS_TICKS) / 1000);
}

static int DoorAnimPlaying(int bank, const DTRG_PRA_DOR *door)
{
    return asCheckAnimationState_S(0, All_table_TBL[bank], door->part, 1, 0, 0, SCENE_ANIM_SCENE, 0)
        == ANIM_STATE_PLAYING;
}

// Plays a door sound heard from the door's sound object.
static void DoorPlaySE(int bank, const char *seBank, int se, short seObj)
{
    int handle = SSPlaySE(seBank, se, 1);

    SdSetParamSE(handle, SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK | SD_SE_PARAM_CALLBACK, seObj, bank,
                 SetPositionalParam_Alys);
    SetPositionalParam_Alys(handle);
}

// CT3 0x018e9480
// The close every kind ends with: the door swings back in three eases (10 -> 5 -> 2 -> 0), then the close
// sound, the door's OPEN flag removed and the scenes behind it dropped.
int tkXDTrg_ActorProc_CmnClo(int bank, DTRG_PRA_DOR *door)
{
    int *wait = &door->wait[DTRG_SLOT_CLOSE_COMMON];
    u_char *step = &door->step[DTRG_SLOT_CLOSE_COMMON];
    int done = 0;

    if (*wait != 0) {
        (*wait)--;
        return 0;
    }
    switch (*step) {
    case 0:
        ssTrigRemoveEvent(door->doorFlag + DOOR_FLAG_OPENED, 1, bank);
        ssTrigRemoveEvent(door->doorFlag + DOOR_FLAG_MOVING, 1, bank);
        tkXDTrg_ActorProc_AnimRangeDoor(bank, door->part, 10, 5, 200, 0, 1, door->otherBank);
        (*step)++;
        // fall through
    case 1:
        if (DoorAnimPlaying(bank, door)) {
            break;
        }
        tkXDTrg_ActorProc_AnimRangeDoor(bank, door->part, 5, 2, 80, 0, 1, door->otherBank);
        (*step)++;
        // fall through
    case 2:
        if (DoorAnimPlaying(bank, door)) {
            break;
        }
        tkXDTrg_ActorProc_AnimRangeDoor(bank, door->part, 2, 0, 30, 0, 1, door->otherBank);
        (*step)++;
        // fall through
    case 3:
        if (DoorAnimPlaying(bank, door)) {
            break;
        }
        DoorPlaySE(bank, SE_BANK_COMMON, (door->flags & DTRG_FLAG_SE_ALT) ? DOOR_SE_CLOSE_ALT : DOOR_SE_CLOSE,
                   door->seObjClose);
        ssTrigRemoveEvent(door->doorFlag + DOOR_FLAG_OPEN, 1, bank);
        tkBri_DoorDraw(DOOR_DRAW_CLOSE);
        done = 1;
        break;
    default:
        break;
    }
    if (done) {
        *wait = 0;
        *step = 0;
    }
    return done;
}

// A kind's Clo: a door that is not open and moving, or that a key event holds (the record's own, or any
// other kind's), is done at once; otherwise CmnClo closes it.
static int DoorKindClose(int bank, DTRG_PRA_DOR *door, int kind)
{
    int *wait = &door->wait[DTRG_SLOT_CLOSE];
    u_char *step = &door->step[DTRG_SLOT_CLOSE];
    int held = 1;
    int done = 0;
    int i;

    if (*wait != 0) {
        (*wait)--;
        return 0;
    }
    if (*step != 1) {
        if (*step != 0) {
            goto end;
        }
        if (ssTrigCheckEvent(door->doorFlag + DOOR_FLAG_OPEN, 1, bank) == 1
            && ssTrigCheckEvent(door->doorFlag + DOOR_FLAG_MOVING, 1, bank) == 1) {
            held = ssTrigCheckEvent(door->event, 0, bank) != 0;
            for (i = 0; i < DTRG_KIND_COUNT && !held; i++) {
                if (door->kind[i].event != -1 && i != kind && ssTrigCheckEvent(door->kind[i].event, 0, bank) == 1) {
                    held = 1;
                }
            }
            if (!held) {
                (*step)++;
            }
        }
        done = 1;
        if (held) {
            goto end;
        }
    }
    done = tkXDTrg_ActorProc_CmnClo(bank, door);
end:
    if (done) {
        *wait = 0;
        *step = 0;
    }
    return done;
}

// A kind's Opn / OpnOut: the OPEN flag and the scenes behind the door at once (one frame's wait), then the
// opening sound, MOVING and the swing (0 -> 10, the kind's speed), OPENED when it ends. A door already
// OPENED is done. weaponBank: the Axe / Ham doors' weapon sound, played from the side `weaponSide` says
// (OpnOut: with DTRG_FLAG_SIDE, Opn: without); the other side plays the common opening sound.
static int DoorKindOpen(int bank, DTRG_PRA_DOR *door, int kind, int out, const char *weaponBank, int weaponSe,
                        int weaponSeAlt)
{
    int *wait = &door->wait[DTRG_SLOT_OPEN];
    u_char *step = &door->step[DTRG_SLOT_OPEN];
    int side = (door->flags & DTRG_FLAG_SIDE) != 0;
    int alt = (door->flags & DTRG_FLAG_SE_ALT) != 0;
    int done = 0;
    u_short speed;

    if (*wait != 0) {
        (*wait)--;
        return 0;
    }
    switch (*step) {
    case 0:
        if (weaponBank != NULL && !out) {
            // AxeOpn / HamOpn: the record's other slots start over.
            door->step[DTRG_SLOT_ALS_OPEN] = 0;
            door->wait[DTRG_SLOT_ALS_OPEN] = DTRG_WAIT_NONE;
            door->step[DTRG_SLOT_CAMERA] = 0;
            door->wait[DTRG_SLOT_CAMERA] = DTRG_WAIT_NONE;
            door->step[DTRG_SLOT_ALS_CLOSE] = 0;
            door->wait[DTRG_SLOT_ALS_CLOSE] = 0;
            door->step[DTRG_SLOT_CLOSE_COMMON] = 0;
            door->wait[DTRG_SLOT_CLOSE_COMMON] = 0;
        }
        ssTrigRegisterEvent(door->doorFlag + DOOR_FLAG_OPEN, 1, bank);
        tkBri_DoorDraw(DOOR_DRAW_OPEN);
        ssTrigSetEnable(door->trigUse, 1, 0, bank);
        *wait = 1;
        (*step)++;
        goto end;
    case 1:
        if (ssTrigCheckEvent(door->doorFlag + DOOR_FLAG_OPENED, 1, bank) != 0) {
            done = 1;
            goto end;
        }
        if (weaponBank != NULL && side == out) {
            DoorPlaySE(bank, weaponBank, alt ? weaponSeAlt : weaponSe, door->kind[kind].seObj);
        } else {
            DoorPlaySE(bank, SE_BANK_COMMON, alt ? DOOR_SE_OPEN_ALT : DOOR_SE_OPEN,
                       weaponBank != NULL ? door->seObjOpen : door->kind[kind].seObj);
        }
        ssTrigRegisterEvent(door->doorFlag + DOOR_FLAG_MOVING, 1, bank);
        speed = DTrgPraMon[kind][side == out ? 0 : 1];
        tkXDTrg_ActorProc_AnimRangeDoor(bank, door->part, 0, 10, speed & DOOR_SPEED_MASK, 0, 0, door->otherBank);
        (*step)++;
        // fall through
    case 2:
        if (!DoorAnimPlaying(bank, door)) {
            ssTrigRegisterEvent(door->doorFlag + DOOR_FLAG_OPENED, 1, bank);
            done = 1;
        }
        break;
    default:
        break;
    }
end:
    if (done) {
        *wait = 0;
        *step = 0;
    }
    return done;
}

// CT3 0x018e9710
int tkXDTrg_ActorProc_ScWClo(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindClose(bank, door, DTRG_KIND_SCW);
}

// CT3 0x018e98c0
int tkXDTrg_ActorProc_ScWOpnOut(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_SCW, 1, NULL, 0, 0);
}

// CT3 0x018e9ae0
int tkXDTrg_ActorProc_ScWOpn(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_SCW, 0, NULL, 0, 0);
}

// CT3 0x018e9d00
int tkXDTrg_ActorProc_ScMClo(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindClose(bank, door, DTRG_KIND_SCM);
}

// CT3 0x018e9eb0
int tkXDTrg_ActorProc_ScMOpnOut(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_SCM, 1, NULL, 0, 0);
}

// CT3 0x018ea0d0
int tkXDTrg_ActorProc_ScMOpn(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_SCM, 0, NULL, 0, 0);
}

// CT3 0x018ea2f0
int tkXDTrg_ActorProc_AxeClo(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindClose(bank, door, DTRG_KIND_AXE);
}

// CT3 0x018ea4a0
int tkXDTrg_ActorProc_AxeOpnOut(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_AXE, 1, DOOR_SE_BANK_AXE, DOOR_SE_AXE, DOOR_SE_AXE_ALT);
}

// CT3 0x018ea720
int tkXDTrg_ActorProc_AxeOpn(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_AXE, 0, DOOR_SE_BANK_AXE, DOOR_SE_AXE, DOOR_SE_AXE_ALT);
}

// CT3 0x018ea9c0
int tkXDTrg_ActorProc_VitClo(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindClose(bank, door, DTRG_KIND_VIT);
}

// CT3 0x018eab70
int tkXDTrg_ActorProc_VitOpnOut(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_VIT, 1, NULL, 0, 0);
}

// CT3 0x018ead90
int tkXDTrg_ActorProc_VitOpn(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_VIT, 0, NULL, 0, 0);
}

// CT3 0x018eafb0
int tkXDTrg_ActorProc_HamClo(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindClose(bank, door, DTRG_KIND_HAM);
}

// CT3 0x018eb160
int tkXDTrg_ActorProc_HamOpnOut(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_HAM, 1, DOOR_SE_BANK_HAMMER, DOOR_SE_HAMMER, DOOR_SE_HAMMER_ALT);
}

// CT3 0x018eb3e0
int tkXDTrg_ActorProc_HamOpn(int bank, DTRG_PRA_DOR *door)
{
    return DoorKindOpen(bank, door, DTRG_KIND_HAM, 0, DOOR_SE_BANK_HAMMER, DOOR_SE_HAMMER, DOOR_SE_HAMMER_ALT);
}

// CT3 0x018eb680
// Shuts the door at once: the open / close procs stop, its flags go and the part jumps to frame 0.
int tkXDTrg_ActorProc_Sht(int bank, DTRG_PRA_DOR *door)
{
    door->step[DTRG_SLOT_OPEN] = 0;
    door->wait[DTRG_SLOT_OPEN] = 0;
    door->step[DTRG_SLOT_CLOSE] = 0;
    door->wait[DTRG_SLOT_CLOSE] = 0;
    door->step[DTRG_SLOT_CLOSE_COMMON] = 0;
    door->wait[DTRG_SLOT_CLOSE_COMMON] = 0;
    ssTrigRemoveEvent(door->doorFlag + DOOR_FLAG_OPEN, 1, bank);
    ssTrigRemoveEvent(door->doorFlag + DOOR_FLAG_OPENED, 1, bank);
    ssTrigRemoveEvent(door->doorFlag + DOOR_FLAG_MOVING, 1, bank);
    tkXDTrg_ActorProc_AnimToFrameDoor(bank, door->part, 0, 100, 1, 0, door->otherBank);
    return 1;
}

// CT3 0x018eb730
// The door's collision and portal parts follow its flags: the closed hit until it moves, the open hit
// after; the portal while it is OPEN. Never done (runs every frame).
int tkXDTrg_ActorProc_HitPtl(int bank, DTRG_PRA_DOR *door)
{
    if (ssTrigCheckEvent(door->doorFlag + DOOR_FLAG_MOVING, 1, bank) == 0) {
        ssTrigSetEnable(door->trigHitClosed, 1, 0, bank);
        ssTrigSetEnable(door->trigHitOpen, 0, 0, bank);
    } else {
        ssTrigSetEnable(door->trigHitClosed, 0, 0, bank);
        ssTrigSetEnable(door->trigHitOpen, 1, 0, bank);
    }
    if (ssTrigCheckEvent(door->doorFlag + DOOR_FLAG_OPEN, 1, bank) == 0) {
        ssTrigSetEnable(door->trigPortal, 0, 0, bank);
    } else {
        ssTrigSetEnable(door->trigPortal, 1, 0, bank);
    }
    return 0;
}

// CT3 0x018eb840
// Cuts to the door's camera (camRoom / camCut, at once), turns its use trigger off and registers
// DOOR_EVENT_CAMERA; after ten anim frames the monsters are hidden (tkXDTrg_MonDraw(0)) and it is done.
int tkXDTrg_ActorProc_AlsCam(int bank, DTRG_PRA_DOR *door)
{
    int *wait = &door->wait[DTRG_SLOT_CAMERA];
    u_char *step = &door->step[DTRG_SLOT_CAMERA];
    int done = 0;

    if (*wait != DTRG_WAIT_NONE && !ssTrigWaitByAnimFrame(wait, door->frames[DTRG_SLOT_CAMERA])) {
        return 0;
    }
    switch (*step) {
    case 0:
        tkXDTrg_ActorProc_ChangeCamera(bank, door->camRoom, door->camCut, 1, 0);
        ssTrigSetEnable(door->trigUse, 0, 0, bank);
        ssTrigRegisterEvent(DOOR_EVENT_CAMERA, 1, bank);
        (*step)++;
        door->frames[DTRG_SLOT_CAMERA] = 1;
        if (!ssTrigWaitByAnimFrame(wait, (u_short)door->frames[DTRG_SLOT_CAMERA] * DOOR_ANIM_FRAME)) {
            break;
        }
        // fall through
    case 1:
        tkXDTrg_MonDraw(0);
        done = 1;
        break;
    default:
        break;
    }
    if (done) {
        *wait = DTRG_WAIT_NONE;
        *step = 0;
    }
    return done;
}

// CT3 0x018ebf80
// tkXDTrg_ActorProc_Ini's answer to an actor kind it does not know: nothing.
void tkXDTrg_ActorProc_Ini_Error(void)
{
}

// ---- the door trigger x command (slice 3, 2026-11-11) ----

DTRG_PRA_ALS DTrgPraAls;                        /* bss  0x01944928 */

#define DTRG_EVENT_NO_OPEN 0x32         /* AlsOpn: while set, Alyssa does not open the door (named from this use) */
#define DTRG_EVENT_DOOR_SOUND 0xd2      /* registered with AlsOpn's opening sound and AlsClo's close (named from this use) */
#define DOOR_ALS_MOTION "m_a_001"       /* @1678 0x01936e60: Alyssa's door motion */
#define DOOR_ALS_MOTION_SIDE "m_a_004"  /* @1677 0x01936e58: the same from the door's other side (DTRG_FLAG_SIDE) */
#define DOOR_ALS_LEAD_FRAMES 6          /* AlsOpn: frames of her motion before the latch sound (plain side) */
#define DOOR_ALS_LEAD_FRAMES_SIDE 1
#define DOOR_PERCENT 100                /* the speeds and anim waits: percent */
#define CHARACTER_FLAG_DOOR 0x10000000u /* AlsOpn sets it on Character[0] with her door motion (named from this use) */
#define DOOR_ALS_STEP_NONE 5            /* AlsOpn: a step that skips the slot's wait (no step reaches it) */
#define DOOR_ALS_STEP_WAIT_DRAW 10      /* AlsOpn: waiting for tkBri_DoorDraw to draw the scenes behind the door */

// CT3 0x018ec870
// The opening code of a weapon door's actor code: 'O' at 6 (OpnOut when 9 is 'O' too, Opn when the code
// ends there) or 'C' (Clo); -1 otherwise.
short tkXDTrg_ActorCodeAnalyzeMonster(const char *code)
{
    if (code[6] == 'O') {
        if (code[9] == '\0') {
            return DTRG_MON_OPN;
        }
        if (code[9] == 'O') {
            return DTRG_MON_OPN_OUT;
        }
        return -1;
    }
    if (code[6] == 'C') {
        return DTRG_MON_CLO;
    }
    return -1;
}

// CT3 0x018ec8c0
// An actor code (three characters, then the proc's name: "Ini", "IniMon", "AlsOpn", "AlsClo", "AlsCam",
// "Hit", "Sht", "HamOpn", "VitClo", "AxeOpnOut", "ScMOpn", "ScWClo"...) -> DTRG_CODE_*, DTRG_CODE_NONE when
// unknown. A weapon kind with an unknown opening code lands one below its base (the ROM's arithmetic).
short tkXDTrg_ActorCodeAnalyze(const char *code)
{
    switch (code[3]) {
    case 'V':
        return (short)(tkXDTrg_ActorCodeAnalyzeMonster(code) + DTRG_CODE_VIT);
    case 'S':
        if (code[5] == 'W') {
            return (short)(tkXDTrg_ActorCodeAnalyzeMonster(code) + DTRG_CODE_SCW);
        }
        if (code[5] == 'M') {
            return (short)(tkXDTrg_ActorCodeAnalyzeMonster(code) + DTRG_CODE_SCM);
        }
        if (code[5] == 't') {
            return DTRG_CODE_SHT;
        }
        break;
    case 'H':
        if (code[4] == 'i') {
            return DTRG_CODE_HIT_PTL;
        }
        if (code[4] == 'a') {
            return (short)(tkXDTrg_ActorCodeAnalyzeMonster(code) + DTRG_CODE_HAM);
        }
        break;
    case 'A':
        if (code[4] == 'l') {
            switch (code[7]) {
            case 'a':
                return DTRG_CODE_ALS_CAM;
            case 'l':
                return DTRG_CODE_ALS_CLO;
            case 'p':
                return DTRG_CODE_ALS_OPN;
            }
        } else if (code[4] == 'x') {
            return (short)(tkXDTrg_ActorCodeAnalyzeMonster(code) + DTRG_CODE_AXE);
        }
        break;
    case 'I':
        return code[6] == 'M' ? DTRG_CODE_INI_MON : DTRG_CODE_INI;
    }
    return DTRG_CODE_NONE;
}

// Puts a head's chain back on the free list.
static void DoorHeadFree(DTRG_HED *head)
{
    u_char r;

    while ((r = head->first) != DTRG_END) {
        DTRG_PRA_DOR *p = &DTrgPraDor[r];
        p->flags = 0;
        p->doorFlag = DTRG_END;
        head->first = p->next;
        p->next = DTrgPraDorTop;
        head->count--;
        DTrgPraDorTop = r;
    }
    head->roomHash = 0;
    head->bank = DTRG_BANK_NONE;
    head->first = DTRG_END;
    head->last = DTRG_END;
    head->count = 0;
}

// CT3 0x018ecb60
// The head of a scene bank (the bank and the room its file holds). A bank without one takes a free head,
// after every head whose bank was unloaded or now holds another room has been freed with its records.
// PORT: the ROM's function is void and its callers use what v0 holds: the head found, or the one taken.
// PORT: the ROM takes the first head that was free *before* the clean-up (and writes DTrgHed[-1] when
// none was); the host takes a head the clean-up freed in that case.
DTRG_HED *tkXDTrg_ActorGetSceneHead(int bank)
{
    u_int hash = (u_int)GetRoomName_file(BankInfo[bank].name);
    int slot = -1;
    int i;

    for (i = 0; i < DTRG_HED_MAX && (DTrgHed[i].bank != bank || DTrgHed[i].roomHash != hash); i++) {
        if (slot == -1 && DTrgHed[i].bank == DTRG_BANK_NONE) {
            slot = i;
        }
    }
    if (i < DTRG_HED_MAX) {
        return &DTrgHed[i];
    }
    for (i = 0; i < DTRG_HED_MAX; i++) {
        DTRG_HED *h = &DTrgHed[i];
        if (h->bank != DTRG_BANK_NONE && (BankInfo[h->bank].state != BANK_STATE_LOADED ||
                                          h->roomHash != (u_int)GetRoomName_file(BankInfo[h->bank].name))) {
            DoorHeadFree(h);
        }
    }
    for (i = 0; slot == -1 && i < DTRG_HED_MAX; i++) {
        if (DTrgHed[i].bank == DTRG_BANK_NONE) {
            slot = i;
        }
    }
    if (slot == -1) {
        slot = 0;
    }
    DTrgHed[slot].roomHash = hash;
    DTrgHed[slot].bank = (short)bank;
    DTrgHed[slot].first = DTRG_END;
    DTrgHed[slot].last = DTRG_END;
    DTrgHed[slot].count = 0;
    return &DTrgHed[slot];
}

// CT3 0x018eca60
// The record of a door (by its flag) on a head's chain; a door not on it takes the free list's first
// record, appended to the chain. The pool is never exhausted in play (32 records, chains freed per room).
DTRG_PRA_DOR *tkXDTrg_ActorGetDoorPara(u_short doorFlag, DTRG_HED *head)
{
    u_char top = DTrgPraDorTop;
    u_char r;
    DTRG_PRA_DOR *p;

    for (r = head->first; r != DTRG_END && doorFlag != DTrgPraDor[r].doorFlag; r = DTrgPraDor[r].next) {
    }
    if (r != DTRG_END) {
        return &DTrgPraDor[r];
    }
    p = &DTrgPraDor[top];
    DTrgPraDorTop = p->next;
    p->flags = 0;
    p->doorFlag = (u_char)doorFlag;
    p->next = DTRG_END;
    if (head->first == DTRG_END) {
        head->first = top;
    } else {
        DTrgPraDor[head->last].next = top;
    }
    head->last = top;
    head->count++;
    return p;
}

// The slots' state at an Ini: Alyssa's open and the camera wait on nothing (-1), the rest count from 0;
// no kind holds the door.
static void DoorRecordReset(DTRG_PRA_DOR *door)
{
    int i;

    for (i = 0; i < DTRG_SLOT_COUNT; i++) {
        door->step[i] = 0;
        door->wait[i] = (i == DTRG_SLOT_ALS_OPEN || i == DTRG_SLOT_CAMERA) ? DTRG_WAIT_NONE : 0;
    }
    for (i = 0; i < DTRG_KIND_COUNT; i++) {
        door->kind[i].event = -1;
    }
}

// CT3 0x018ec600
// A door record from the trigger's arguments (the form with fewer than 15): the code gives the flags
// ('u' at 1: DTRG_FLAG_SIDE; a 7th character: 'W' DTRG_FLAG_W, else DTRG_FLAG_SE_ALT), argv[2..8] the
// part, the portal, the camera cut, the warp part, the two hit parts and the close sound object, then
// by count the rest. otherBank is the bank of the bank's detector on the portal part.
int tkXDTrg_ActorProc_Ini_New(int argc, char **argv, int bank, DTRG_PRA_DOR *door)
{
    const char *code = argv[1];
    u_int *det;
    u_short kindFlag;

    DoorRecordReset(door);
    kindFlag = code[6] == '\0' ? 0 : code[6] == 'W' ? DTRG_FLAG_W : DTRG_FLAG_SE_ALT;
    door->flags = (u_short)((code[1] == 'u' ? DTRG_FLAG_SIDE : 0) | DTRG_FLAG_USED | kindFlag);
    door->part = (u_short)atoi(argv[2]);
    door->trigPortal = (u_short)atoi(argv[3]);
    door->camCut = (short)atoi(argv[4]);
    door->warpPart = (u_short)atoi(argv[5]);
    door->trigHitClosed = (u_short)atoi(argv[6]);
    door->trigHitOpen = (u_short)atoi(argv[7]);
    door->seObjClose = (short)atoi(argv[8]);
    if (argc == 13) {
        door->seObjOpen = (short)atoi(argv[9]);
        door->h4c = (short)atoi(argv[10]);
        door->trigUse = (u_short)atoi(argv[11]);
        door->event = (short)atoi(argv[12]);
    } else if (argc == 12) {
        door->seObjOpen = door->seObjClose;
        door->h4c = (short)atoi(argv[9]);
        door->trigUse = (u_short)atoi(argv[10]);
        door->event = (short)atoi(argv[11]);
    } else if (argc == 11) {
        door->seObjOpen = door->seObjClose;
        door->trigUse = (u_short)atoi(argv[9]);
        door->event = (short)atoi(argv[10]);
    } else {
        door->h48 = (short)atoi(argv[9]);
        door->seObjOpen = (short)atoi(argv[10]);
        door->h4c = (short)atoi(argv[11]);
        door->trigUse = (u_short)atoi(argv[12]);
        door->event = (short)atoi(argv[13]);
    }
    // PORT: the ROM reads a missing detector's bank through NULL; the host leaves otherBank as it was.
    for (det = BANK_PTR(u_int *, bank, All_table_TBL[bank]->detectors); *det != 0; det++) {
        const TRIG_DETECTOR *d = BANK_PTR(const TRIG_DETECTOR *, bank, *det);
        if ((u_short)d->part == door->trigPortal) {
            door->otherBank = d->bank;
            break;
        }
    }
    return 1;
}

// CT3 0x018ec540
// A door kind's entry: argv[2] the kind (1..6), argv[3] its event, argv[4..] the entry's next shorts.
int tkXDTrg_ActorProc_IniMon(int argc, char **argv, int bank, DTRG_PRA_DOR *door)
{
    short *entry = &door->kind[(short)atoi(argv[2]) - 1].event;
    u_int i;

    (void)bank;
    entry[0] = (short)atoi(argv[3]);
    for (i = 0; i < (u_int)argc - 4; i++) {
        entry[1 + i] = (short)atoi(argv[4 + i]);
    }
    return 1;
}

// CT3 0x018ebad0
// Alyssa opens the door: she is warped to its warp part and plays her door motion while the scenes behind
// are drawn and the camera cuts (AlsCam); after her motion's lead frames the latch sounds (unless the
// door's event is unset: then she gives up), then the door swings open at DTrgPraAls's speed. Done when
// its animation ends (OPENED registered, the keys back).
int tkXDTrg_ActorProc_AlsOpn(int bank, DTRG_PRA_DOR *door)
{
    int *wait = &door->wait[DTRG_SLOT_ALS_OPEN];
    short *frames = &door->frames[DTRG_SLOT_ALS_OPEN];
    u_char *step = &door->step[DTRG_SLOT_ALS_OPEN];
    int side = (door->flags & DTRG_FLAG_SIDE) != 0;
    int done = 0;

    if (*step != DOOR_ALS_STEP_NONE && *wait != DTRG_WAIT_NONE && !ssTrigWaitByAnimFrame(wait, (u_short)*frames)) {
        return 0;
    }
    switch (*step) {
    case 0:
        if ((char)ssTrigCheckEvent(DTRG_EVENT_NO_OPEN, TRIG_SET_GLOBAL, bank) == 1) {
            done = 1;
            break;
        }
        *step = DOOR_ALS_STEP_WAIT_DRAW;
        // fall through
    case DOOR_ALS_STEP_WAIT_DRAW: {
        char room[8];

        PlayerControlUpdate(1);
        ssTrigRegisterEvent(TRIG_EVENT_PANIC_KEEP_PLAYER, TRIG_SET_GLOBAL, bank);
        ssTrigRegisterEvent(door->doorFlag + DOOR_FLAG_OPEN, TRIG_SET_GLOBAL, bank);
        if (tkBri_DoorDraw(DOOR_DRAW_OPEN) == 0) {
            ssTrigRemoveEvent(door->doorFlag + DOOR_FLAG_OPEN, TRIG_SET_GLOBAL, bank);
            break;
        }
        *step = 0;
        room[0] = '\0';
        WarpObject(room, 1, 0, door->warpPart, bank);
        asSceneAnimationStart(1.0f, ahGetHash(side ? DOOR_ALS_MOTION_SIDE : DOOR_ALS_MOTION), All_table_TBL[bank], 0, 0,
                              0, 1, 0, 0, 0, 0, 0, SCENE_ANIM_CHARACTER, 0);
        Character[0].flags |= CHARACTER_FLAG_DOOR;
        (*step)++;
        *frames = 1;
        if (!ssTrigWaitByAnimFrame(wait, (u_short)*frames * DOOR_ANIM_FRAME)) {
            break;
        }
    }
        // fall through
    case 1:
        if ((char)tkXDTrg_ActorProc_AlsCam(bank, door) == 0) {
            break;
        }
        (*step)++;
        *frames = (short)(side ? DTrgPraAls.framesSide - 1 : DTrgPraAls.frames - DOOR_ALS_LEAD_FRAMES);
        *frames = (short)(*frames - 1);
        *frames = (short)(*frames * DOOR_ANIM_FRAME);
        *frames = (short)((int)((u_short)*frames * DOOR_PERCENT) / anim_play_slow);
        if (!ssTrigWaitByAnimFrame(wait, (u_short)*frames)) {
            break;
        }
        // fall through
    case 2:
        if ((char)ssTrigCheckEvent(door->event, TRIG_SET_TEMP, bank) == 0) {
            done = 1;
            break;
        }
        SSPlaySE(SE_BANK_COMMON, (door->flags & DTRG_FLAG_SE_ALT) ? DOOR_SE_OPEN_ALT : DOOR_SE_OPEN, 1);
        ssTrigRegisterEvent(DTRG_EVENT_DOOR_SOUND, TRIG_SET_GLOBAL, bank);
        (*step)++;
        *frames = side ? DOOR_ALS_LEAD_FRAMES_SIDE : DOOR_ALS_LEAD_FRAMES;
        *frames = (short)(*frames * DOOR_ANIM_FRAME);
        *frames = (short)((int)((u_short)*frames * DOOR_PERCENT) / anim_play_slow);
        if (!ssTrigWaitByAnimFrame(wait, (u_short)*frames)) {
            break;
        }
        // fall through
    case 3:
        ssTrigRegisterEvent(door->doorFlag + DOOR_FLAG_MOVING, TRIG_SET_GLOBAL, bank);
        tkXDTrg_ActorProc_AnimRangeDoor(bank, door->part, 0, 10,
                                        (side ? DTrgPraAls.speedSide : DTrgPraAls.speed) & DOOR_SPEED_MASK, 0, 0,
                                        door->otherBank);
        (*step)++;
        // fall through
    case 4:
        if (!DoorAnimPlaying(bank, door)) {
            ssTrigRegisterEvent(door->doorFlag + DOOR_FLAG_OPENED, TRIG_SET_GLOBAL, bank);
            ssTrigRemoveEvent(TRIG_EVENT_PANIC_KEEP_PLAYER, TRIG_SET_GLOBAL, bank);
            PlayerControlUpdate(0);
            done = 1;
        }
        break;
    default:
        break;
    }
    if (done) {
        *wait = DTRG_WAIT_NONE;
        *step = 0;
    }
    return done;
}

// CT3 0x018eb960
// Alyssa closes the door: the stalkers shown again, the camera event removed, the door's use trigger back;
// a kind's event that is set holds the door open (done at once); else the common close runs.
int tkXDTrg_ActorProc_AlsClo(int bank, DTRG_PRA_DOR *door)
{
    int *wait = &door->wait[DTRG_SLOT_ALS_CLOSE];
    u_char *step = &door->step[DTRG_SLOT_ALS_CLOSE];
    int done = 0;
    int i;

    if (*wait != 0) {
        (*wait)--;
        return 0;
    }
    if (*step == 0) {
        int held = 0;

        tkXDTrg_MonDraw(1);
        ssTrigRemoveEvent(DOOR_EVENT_CAMERA, TRIG_SET_GLOBAL, bank);
        ssTrigSetEnable(door->trigUse, 1, 0, bank);
        for (i = 0; i < DTRG_KIND_COUNT; i++) {
            if (door->kind[i].event != -1 && (char)ssTrigCheckEvent(door->kind[i].event, TRIG_SET_TEMP, bank) == 1) {
                held = 1;
                break;
            }
        }
        if (!held) {
            (*step)++;
        }
        done = 1;
        if (held) {
            goto end;
        }
    } else if (*step != 1) {
        goto end;
    }
    done = (char)tkXDTrg_ActorProc_CmnClo(bank, door);
    ssTrigRegisterEvent(DTRG_EVENT_DOOR_SOUND, TRIG_SET_GLOBAL, bank);
end:
    if (done) {
        *wait = 0;
        *step = 0;
    }
    return done;
}

// CT3 0x018ecd40
// The door trigger's actor: argv[0] the door's flag, argv[1] the actor code; finds (or makes) the door's
// record in the bank's head and runs the code's proc. Unknown codes answer 1 (done).
int tkXDTrg_ActorProc(u_int argc, char **argv, int bank)
{
    short doorFlag = (short)atoi(argv[0]);
    DTRG_HED *head = tkXDTrg_ActorGetSceneHead(bank);
    DTRG_PRA_DOR *door = tkXDTrg_ActorGetDoorPara((u_short)doorFlag, head);

    switch (tkXDTrg_ActorCodeAnalyze(argv[1])) {
    case DTRG_CODE_INI:
        if (argc < DTRG_INI_NEW_ARGC) {
            return tkXDTrg_ActorProc_Ini_New((int)argc, argv, bank, door);
        }
        return tkXDTrg_ActorProc_Ini((int)argc, argv, bank, door);
    case DTRG_CODE_INI_MON:
        return tkXDTrg_ActorProc_IniMon((int)argc, argv, bank, door);
    case DTRG_CODE_ALS_OPN:
        return tkXDTrg_ActorProc_AlsOpn(bank, door);
    case DTRG_CODE_ALS_CLO:
        return tkXDTrg_ActorProc_AlsClo(bank, door);
    case DTRG_CODE_ALS_CAM:
        return tkXDTrg_ActorProc_AlsCam(bank, door);
    case DTRG_CODE_HIT_PTL:
        return tkXDTrg_ActorProc_HitPtl(bank, door);
    case DTRG_CODE_SHT:
        return tkXDTrg_ActorProc_Sht(bank, door);
    case DTRG_CODE_HAM + DTRG_MON_OPN:
        return tkXDTrg_ActorProc_HamOpn(bank, door);
    case DTRG_CODE_HAM + DTRG_MON_CLO:
        return tkXDTrg_ActorProc_HamClo(bank, door);
    case DTRG_CODE_HAM + DTRG_MON_OPN_OUT:
        return tkXDTrg_ActorProc_HamOpnOut(bank, door);
    case DTRG_CODE_VIT + DTRG_MON_OPN:
        return tkXDTrg_ActorProc_VitOpn(bank, door);
    case DTRG_CODE_VIT + DTRG_MON_CLO:
        return tkXDTrg_ActorProc_VitClo(bank, door);
    case DTRG_CODE_VIT + DTRG_MON_OPN_OUT:
        return tkXDTrg_ActorProc_VitOpnOut(bank, door);
    case DTRG_CODE_AXE + DTRG_MON_OPN:
        return tkXDTrg_ActorProc_AxeOpn(bank, door);
    case DTRG_CODE_AXE + DTRG_MON_CLO:
        return tkXDTrg_ActorProc_AxeClo(bank, door);
    case DTRG_CODE_AXE + DTRG_MON_OPN_OUT:
        return tkXDTrg_ActorProc_AxeOpnOut(bank, door);
    case DTRG_CODE_SCM + DTRG_MON_OPN:
        return tkXDTrg_ActorProc_ScMOpn(bank, door);
    case DTRG_CODE_SCM + DTRG_MON_CLO:
        return tkXDTrg_ActorProc_ScMClo(bank, door);
    case DTRG_CODE_SCM + DTRG_MON_OPN_OUT:
        return tkXDTrg_ActorProc_ScMOpnOut(bank, door);
    case DTRG_CODE_SCW + DTRG_MON_OPN:
        return tkXDTrg_ActorProc_ScWOpn(bank, door);
    case DTRG_CODE_SCW + DTRG_MON_CLO:
        return tkXDTrg_ActorProc_ScWClo(bank, door);
    case DTRG_CODE_SCW + DTRG_MON_OPN_OUT:
        return tkXDTrg_ActorProc_ScWOpnOut(bank, door);
    default:
        return 1;
    }
}

// CT3 0x018ed090
// Shows (1) or hides (0) the stalkers: ssAI_MonsterDraw on each with a brain; unless a panic keeps them
// running their AI pauses while hidden, and unless TRIG_EVENT_PANIC_NO_RELOAD is set the hide registers
// TRIG_EVENT_MC_RELOADED (the event full panic uses the same way).
void tkXDTrg_MonDraw(int draw)
{
    int i;

    for (i = 0; i < ATK_CHARA_LIST2_COUNT; i++) {
        CHARACTER *chr = &Character[ssAtkCharaList2[i]];
        if (chr->brain != 0) {
            ssAI_MonsterDraw(chr, draw);
        }
    }
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MONSTERS, TRIG_SET_GLOBAL, -1) == 0) {
        MonsterAIUpdate(draw != 1);
    }
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_NO_RELOAD, TRIG_SET_GLOBAL, -1) == 0) {
        if (draw == 1) {
            ssTrigRemoveEvent(TRIG_EVENT_MC_RELOADED, TRIG_SET_GLOBAL, -1);
        } else {
            ssTrigRegisterEvent(TRIG_EVENT_MC_RELOADED, TRIG_SET_GLOBAL, -1);
        }
    }
}

// newlib's atoff (float atof), which the ROM links; the host libc has atof.
static float atoff(const char *s)
{
    return (float)atof(s);
}

// CT3 0x018ed190
// Door 99's trigger: argv[1] 0 sets Alyssa's door opening (DTrgPraAls: frames, speed, side frames, side
// speed), 1..6 a door kind's two opening speeds (DTrgPraMon). Speeds are given as rates (1.0 = 100).
int tkXDTrg_CharacterInit(int argc, char **argv)
{
    short kind = (short)atoi(argv[1]);

    (void)argc;
    if (kind == DTRG_CHARACTER_ALYSSA) {
        DTrgPraAls.frames = (u_short)atoi(argv[2]);
        DTrgPraAls.speed = (u_short)(u_int)(atoff(argv[3]) * 100.0f);
        DTrgPraAls.framesSide = (u_short)atoi(argv[4]);
        DTrgPraAls.speedSide = (u_short)(u_int)(atoff(argv[5]) * 100.0f);
    } else {
        DTrgPraMon[kind - 1][0] = (u_short)(int)(atoff(argv[2]) * 100.0f);
        DTrgPraMon[kind - 1][1] = (u_short)(int)(atoff(argv[3]) * 100.0f);
    }
    return 1;
}

// CT3 0x018ed9e0
// The x command DoorTrigger: door DTRG_DOOR_CHARACTER sets the opening speeds, any other door runs its actor.
// PORT: the ROM's function is void; the x command's answer is its callee's (v0 passes through).
int tkXcmd_DoorTrigger(int argc, char **argv, int bank)
{
    if ((short)atoi(argv[0]) == DTRG_DOOR_CHARACTER) {
        return tkXDTrg_CharacterInit(argc, argv);
    }
    return tkXDTrg_ActorProc((u_int)argc, argv, bank);
}

// ---- stubs ----

// stub of CT3 0x018ebf90 (slice 4: the old-form Ini builds part names from the scene's name). Answers 1
// like the ROM so the trigger moves on; the record keeps DoorRecordReset's state only.
int tkXDTrg_ActorProc_Ini(int argc, char **argv, int bank, DTRG_PRA_DOR *door)
{
    (void)argc;
    (void)argv;
    (void)bank;
    DoorRecordReset(door);
    ALYSSAPAN_STUB("tkXDTrg_ActorProc_Ini", 0x018ebf90);
    return 1;
}


// ---- the branch scene tables (.rodata, dumped from the ELF 2026-11-11) ----
static const u_char Br1LnkTbl[102] = { /* 0x0193a970 */
    0x01, 0x05, 0x07, 0x08, 0x09, 0x12, 0x1a, 0x00, 0x0f, 0x12, 0x0a, 0x0e, 0x0c, 0x11, 0x00, 0x09,
    0x0a, 0x10, 0x00, 0x00, 0x00, 0x05, 0x0a, 0x0d, 0x19, 0x03, 0x06, 0x09, 0x15, 0x12, 0x13, 0x14,
    0x17, 0x04, 0x09, 0x13, 0x16, 0x18, 0x03, 0x11, 0x02, 0x06, 0x04, 0x0e, 0x00, 0x02, 0x0b, 0x0b,
    0x0d, 0x0b, 0x0a, 0x0d, 0x0b, 0x0d, 0x09, 0x00, 0x1c, 0x1d, 0x1e, 0x1a, 0x1d, 0x1b, 0x1c, 0x2a,
    0x1b, 0x21, 0x27, 0x21, 0x27, 0x21, 0x25, 0x28, 0x1e, 0x1f, 0x20, 0x26, 0x26, 0x27, 0x26, 0x28,
    0x20, 0x29, 0x22, 0x23, 0x24, 0x1e, 0x1f, 0x23, 0x20, 0x24, 0x25, 0x1d, 0x2b, 0x2f, 0x2a, 0x2c,
    0x2d, 0x2b, 0x2b, 0x2f, 0x2a, 0x2e,
};
static const SCN_DOOR Br1DorTbl[125] = { /* 0x0193a9e0 */
    {0, 1}, {1, 5}, {2, 7}, {3, 8}, {4, 9}, {5, 18}, {6, 26}, {0, 0},
    {7, 15}, {8, 18}, {9, 10}, {10, 14}, {11, 12}, {12, 17}, {1, 0}, {13, 9},
    {14, 10}, {15, 16}, {2, 0}, {3, 0}, {4, 0}, {13, 5}, {16, 10}, {17, 13},
    {70, 25}, {9, 3}, {14, 6}, {16, 9}, {18, 21}, {19, 18}, {20, 19}, {21, 20},
    {22, 23}, {23, 23}, {24, 23}, {25, 23}, {11, 4}, {17, 9}, {26, 19}, {27, 22},
    {28, 24}, {29, 24}, {30, 24}, {10, 3}, {31, 17}, {7, 2}, {15, 6}, {12, 4},
    {31, 14}, {5, 0}, {8, 2}, {19, 11}, {20, 11}, {26, 13}, {21, 11}, {18, 10},
    {27, 13}, {22, 11}, {23, 11}, {24, 11}, {25, 11}, {28, 13}, {29, 13}, {30, 13},
    {70, 9}, {6, 0}, {32, 28}, {33, 29}, {34, 30}, {32, 26}, {36, 29}, {33, 27},
    {36, 28}, {39, 42}, {34, 27}, {41, 33}, {42, 33}, {69, 39}, {43, 33}, {44, 39},
    {45, 33}, {46, 37}, {47, 40}, {41, 30}, {42, 30}, {43, 31}, {45, 32}, {48, 33},
    {48, 38}, {49, 33}, {49, 38}, {56, 33}, {56, 38}, {57, 39}, {58, 33}, {58, 38},
    {59, 40}, {46, 32}, {60, 33}, {60, 41}, {48, 34}, {49, 34}, {56, 35}, {58, 36},
    {69, 30}, {44, 31}, {57, 35}, {47, 32}, {59, 36}, {60, 37}, {39, 29}, {50, 43},
    {51, 43}, {52, 43}, {52, 47}, {50, 42}, {51, 42}, {53, 44}, {54, 45}, {53, 43},
    {54, 43}, {55, 43}, {55, 47}, {52, 42}, {55, 46},
};
static const u_char Br1Nx2Tbl[171] = { /* 0x0193aae0 */
    0x02, 0x0a, 0x0b, 0x0d, 0x19, 0x1c, 0x05, 0x07, 0x08, 0x09, 0x12, 0x1a, 0x00, 0x0b, 0x06, 0x09,
    0x0e, 0x01, 0x07, 0x08, 0x0a, 0x0d, 0x12, 0x19, 0x1a, 0x03, 0x09, 0x15, 0x01, 0x05, 0x08, 0x09,
    0x12, 0x1a, 0x01, 0x05, 0x07, 0x09, 0x12, 0x1a, 0x01, 0x03, 0x06, 0x07, 0x08, 0x12, 0x13, 0x15,
    0x16, 0x18, 0x1a, 0x00, 0x05, 0x0d, 0x0e, 0x10, 0x19, 0x00, 0x02, 0x0d, 0x11, 0x00, 0x05, 0x0a,
    0x0b, 0x19, 0x04, 0x0a, 0x12, 0x0a, 0x03, 0x0c, 0x01, 0x05, 0x07, 0x08, 0x09, 0x0f, 0x13, 0x14,
    0x17, 0x1a, 0x09, 0x12, 0x14, 0x16, 0x17, 0x18, 0x12, 0x13, 0x17, 0x03, 0x06, 0x09, 0x09, 0x13,
    0x18, 0x12, 0x13, 0x14, 0x09, 0x13, 0x16, 0x00, 0x05, 0x0a, 0x0d, 0x01, 0x05, 0x07, 0x08, 0x09,
    0x12, 0x1d, 0x1c, 0x21, 0x26, 0x27, 0x00, 0x1b, 0x2a, 0x1a, 0x1e, 0x2b, 0x2f, 0x1d, 0x1f, 0x20,
    0x23, 0x1e, 0x20, 0x23, 0x1e, 0x1f, 0x24, 0x29, 0x1b, 0x25, 0x27, 0x28, 0x23, 0x24, 0x1e, 0x1f,
    0x22, 0x24, 0x20, 0x22, 0x23, 0x28, 0x27, 0x28, 0x1b, 0x21, 0x26, 0x21, 0x25, 0x26, 0x20, 0x1c,
    0x1b, 0x2c, 0x2d, 0x2e, 0x1d, 0x2a, 0x2d, 0x2a, 0x2c, 0x2a, 0x1d,
};
static const u_char Br1DorPolTbl[208] = { /* 0x0193ab90 */
    0xb0, 0x60, 0x4c, 0xf6, 0x6d, 0x42, 0xa9, 0x33, 0xb0, 0xa0, 0xac, 0xf6, 0x6d, 0x42, 0xa9, 0x30,
    0xb0, 0x60, 0x4c, 0xf6, 0x6d, 0x42, 0xa9, 0x34, 0xb0, 0xa0, 0xac, 0xf6, 0x6d, 0x42, 0xa9, 0x31,
    0xb0, 0x60, 0x4c, 0xf6, 0x6d, 0x42, 0xa9, 0x35, 0xb0, 0xa0, 0xac, 0xf6, 0x6d, 0x42, 0xa9, 0x32,
    0xb0, 0x60, 0x4c, 0xf6, 0x6d, 0x42, 0xa9, 0x36, 0xb0, 0xa0, 0xac, 0xf6, 0x6d, 0x42, 0xa9, 0x33,
    0xb0, 0x60, 0x8c, 0xf6, 0x6d, 0x42, 0xa9, 0x33, 0xb0, 0xa0, 0xcc, 0xf6, 0x6d, 0x42, 0xa9, 0x30,
    0xb0, 0x60, 0x8c, 0xf6, 0x6d, 0x42, 0xa9, 0x34, 0xb0, 0xa0, 0xcc, 0xf6, 0x6d, 0x42, 0xa9, 0x31,
    0xb0, 0x60, 0x8c, 0xf6, 0x6d, 0x42, 0xa9, 0x35, 0xb0, 0xa0, 0xcc, 0xf6, 0x6d, 0x42, 0xa9, 0x32,
    0x31, 0x31, 0x2c, 0xf6, 0x6d, 0x42, 0xa9, 0x32, 0x31, 0x31, 0x8c, 0xf6, 0x6d, 0x42, 0xa9, 0x31,
    0x31, 0x31, 0x2c, 0xf6, 0x6d, 0x42, 0xa9, 0x31, 0x31, 0x31, 0x8c, 0xf6, 0x6d, 0x42, 0xa9, 0x30,
    0x31, 0x31, 0xac, 0xf6, 0x6d, 0x42, 0xa9, 0x30, 0x31, 0x31, 0x2c, 0xf7, 0x6d, 0x42, 0xa9, 0x30,
    0x31, 0x31, 0xac, 0xf6, 0x6d, 0x42, 0xa9, 0x31, 0x31, 0x31, 0x2c, 0xf7, 0x6d, 0x42, 0xa9, 0x31,
    0xb1, 0x31, 0x2c, 0xf6, 0x6d, 0x42, 0xa9, 0x31, 0xb1, 0x31, 0x4c, 0xf6, 0x6d, 0x42, 0xa9, 0x30,
    0xb1, 0x31, 0x2c, 0xf6, 0x6d, 0x42, 0xa9, 0x32, 0xb1, 0x31, 0x4c, 0xf6, 0x6d, 0x42, 0xa9, 0x31,
};
static const u_char Br1DorExtTbl[71] = { /* 0x0193ac60 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x03, 0xff, 0xff, 0x04, 0x05, 0x06, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x08, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x09, 0x0a, 0x0b, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};
static const SCN_ENTRY ScnBr1Tbl[49] = { /* 0x0193acb0 */
    {0x062c20b0, 0, 7, 0x28, 0x03, 0, 7, 0, 6, {0x00, 0x00, 0x00, 0x00}},
    {0x064c20b0, 7, 1, 0x28, 0x03, 7, 1, 6, 6, {0x00, 0x00, 0x00, 0x00}},
    {0x066c20b0, 8, 2, 0x28, 0x03, 8, 2, 12, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x068c20b0, 10, 2, 0x28, 0x03, 10, 2, 14, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x06ac20b0, 12, 2, 0x28, 0x03, 12, 2, 16, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc20b0, 14, 2, 0x28, 0x03, 14, 2, 17, 8, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec20b0, 16, 2, 0x28, 0x03, 16, 2, 25, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x070c20b0, 18, 1, 0x28, 0x03, 18, 1, 28, 6, {0x00, 0x00, 0x00, 0x00}},
    {0x072c20b0, 19, 1, 0x28, 0x03, 19, 1, 34, 6, {0x00, 0x00, 0x00, 0x00}},
    {0x060c60b0, 20, 5, 0x28, 0x03, 20, 5, 40, 11, {0x00, 0x00, 0x00, 0x00}},
    {0x062c60b0, 25, 4, 0x28, 0x03, 25, 4, 51, 6, {0x00, 0x00, 0x00, 0x00}},
    {0x064c60b0, 29, 4, 0x28, 0x03, 29, 7, 57, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x066c60b0, 33, 1, 0x28, 0x03, 36, 1, 60, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x068c60b0, 34, 4, 0x28, 0x03, 37, 6, 61, 5, {0x00, 0x00, 0x00, 0x00}},
    {0x06ac60b0, 38, 2, 0x28, 0x03, 43, 2, 66, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc60b0, 40, 1, 0x28, 0x03, 45, 1, 68, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec60b0, 41, 1, 0x28, 0x03, 46, 1, 69, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x070c60b0, 42, 2, 0x28, 0x03, 47, 2, 70, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x072c60b0, 44, 3, 0x28, 0x03, 49, 3, 72, 10, {0x00, 0x00, 0x00, 0x00}},
    {0x062ca0b0, 47, 2, 0x28, 0x03, 52, 2, 82, 6, {0x00, 0x00, 0x00, 0x00}},
    {0x064ca0b0, 49, 1, 0x28, 0x03, 54, 1, 88, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x066ca0b0, 50, 1, 0x28, 0x03, 55, 1, 91, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x068ca0b0, 51, 1, 0x28, 0x03, 56, 1, 94, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x06aca0b0, 52, 1, 0x28, 0x03, 57, 4, 97, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x06cca0b0, 53, 1, 0x28, 0x03, 61, 3, 100, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x06eca0b0, 54, 1, 0x28, 0x03, 64, 1, 103, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x064c20b1, 55, 2, 0x28, 0x03, 65, 2, 107, 7, {0x00, 0x00, 0x00, 0x00}},
    {0x068c20b1, 57, 2, 0x28, 0x03, 67, 2, 114, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x060c60b1, 59, 2, 0x28, 0x03, 69, 2, 118, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x062c60b1, 61, 3, 0x28, 0x03, 71, 3, 121, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x062c2131, 64, 3, 0x28, 0x03, 74, 4, 125, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x064c2131, 67, 2, 0x28, 0x03, 78, 2, 129, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x066c2131, 69, 3, 0x28, 0x03, 80, 3, 132, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x068c2131, 72, 3, 0x28, 0x03, 83, 4, 136, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x06ac2131, 75, 1, 0x28, 0x03, 87, 4, 140, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc2131, 76, 2, 0x28, 0x03, 91, 3, 142, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec2131, 78, 2, 0x28, 0x03, 94, 3, 146, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x070c2131, 80, 2, 0x28, 0x03, 97, 3, 149, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x072c2131, 82, 3, 0x28, 0x03, 100, 4, 150, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x060c6131, 85, 3, 0x28, 0x03, 104, 3, 152, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x062c6131, 88, 2, 0x28, 0x03, 107, 2, 155, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x064c6131, 90, 1, 0x28, 0x03, 109, 1, 158, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x062c21b1, 91, 3, 0x28, 0x03, 110, 5, 159, 5, {0x00, 0x00, 0x00, 0x00}},
    {0x064c21b1, 94, 3, 0x28, 0x03, 115, 4, 164, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x066c21b1, 97, 1, 0x28, 0x03, 119, 1, 165, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x068c21b1, 98, 1, 0x28, 0x03, 120, 1, 167, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x06ac21b1, 99, 1, 0x28, 0x03, 121, 2, 169, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc21b1, 100, 2, 0x28, 0x03, 123, 2, 170, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x062c6a42, 102, 0, 0x28, 0x03, 125, 0, 171, 0, {0x00, 0x00, 0x00, 0x00}},
};
static const u_char Br2LnkTbl[68] = { /* 0x0193afc0 */
    0x01, 0x03, 0x05, 0x09, 0x00, 0x07, 0x09, 0x00, 0x05, 0x06, 0x08, 0x00, 0x03, 0x06, 0x0b, 0x0c,
    0x0f, 0x04, 0x05, 0x0a, 0x10, 0x02, 0x0e, 0x04, 0x10, 0x00, 0x02, 0x06, 0x05, 0x05, 0x0d, 0x0f,
    0x11, 0x0c, 0x07, 0x11, 0x05, 0x0c, 0x06, 0x08, 0x0c, 0x0e, 0x14, 0x15, 0x14, 0x17, 0x19, 0x1a,
    0x1b, 0x12, 0x13, 0x15, 0x16, 0x18, 0x12, 0x14, 0x14, 0x13, 0x14, 0x13, 0x1a, 0x13, 0x19, 0x13,
    0x1d, 0x1d, 0x1b, 0x1c,
};
static const SCN_DOOR Br2DorTbl[68] = { /* 0x0193b010 */
    {0, 1}, {1, 3}, {4, 5}, {5, 9}, {0, 0}, {7, 7}, {8, 9}, {1, 0},
    {13, 5}, {14, 6}, {15, 8}, {4, 0}, {13, 3}, {16, 6}, {70, 11}, {36, 12},
    {41, 15}, {14, 4}, {16, 5}, {18, 10}, {42, 16}, {7, 2}, {39, 14}, {15, 4},
    {43, 16}, {5, 0}, {8, 2}, {18, 6}, {70, 5}, {36, 5}, {32, 13}, {33, 15},
    {37, 17}, {32, 12}, {39, 7}, {40, 17}, {41, 5}, {33, 12}, {42, 6}, {43, 8},
    {37, 12}, {40, 14}, {45, 20}, {46, 21}, {47, 20}, {48, 23}, {49, 25}, {50, 26},
    {51, 27}, {45, 18}, {47, 19}, {53, 21}, {54, 22}, {55, 24}, {46, 18}, {53, 20},
    {54, 20}, {48, 19}, {55, 20}, {49, 19}, {57, 26}, {50, 19}, {57, 25}, {51, 19},
    {58, 29}, {59, 29}, {58, 27}, {59, 28},
};
static const u_char Br2Nx2Tbl[98] = { /* 0x0193b0a0 */
    0x02, 0x06, 0x0b, 0x05, 0x03, 0x09, 0x00, 0x01, 0x06, 0x09, 0x0b, 0x05, 0x0a, 0x01, 0x04, 0x09,
    0x0a, 0x0d, 0x10, 0x11, 0x00, 0x03, 0x08, 0x0b, 0x0c, 0x0f, 0x09, 0x11, 0x06, 0x01, 0x03, 0x05,
    0x07, 0x04, 0x05, 0x10, 0x00, 0x03, 0x06, 0x0c, 0x0f, 0x06, 0x0b, 0x0e, 0x05, 0x0f, 0x11, 0x0c,
    0x06, 0x0b, 0x0d, 0x11, 0x05, 0x0a, 0x05, 0x07, 0x0d, 0x0f, 0x13, 0x16, 0x18, 0x12, 0x15, 0x16,
    0x18, 0x1d, 0x17, 0x19, 0x1a, 0x1b, 0x13, 0x16, 0x18, 0x12, 0x13, 0x15, 0x18, 0x14, 0x19, 0x1a,
    0x1b, 0x12, 0x13, 0x15, 0x16, 0x14, 0x17, 0x1b, 0x14, 0x17, 0x1b, 0x14, 0x17, 0x19, 0x1a, 0x1c,
    0x1b, 0x13,
};
static const SCN_ENTRY ScnBr2Tbl[31] = { /* 0x0193b110 */
    {0x062c20b0, 0, 4, 0x28, 0x03, 0, 4, 0, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x064c20b0, 4, 1, 0x28, 0x03, 4, 1, 3, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x066c20b0, 5, 2, 0x28, 0x03, 5, 2, 6, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc20b0, 7, 2, 0x28, 0x03, 7, 2, 7, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec20b0, 9, 2, 0x28, 0x03, 9, 2, 11, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x060c60b0, 11, 6, 0x28, 0x03, 11, 6, 13, 7, {0x00, 0x00, 0x00, 0x00}},
    {0x062c60b0, 17, 4, 0x28, 0x03, 17, 4, 20, 6, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc60b0, 21, 2, 0x28, 0x03, 21, 2, 26, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec60b0, 23, 2, 0x28, 0x03, 23, 2, 28, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x072c60b0, 25, 2, 0x28, 0x03, 25, 2, 29, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x066ca0b0, 27, 1, 0x28, 0x03, 27, 1, 33, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x06eca0b0, 28, 1, 0x28, 0x03, 28, 1, 36, 5, {0x00, 0x00, 0x00, 0x00}},
    {0x062c2130, 29, 4, 0x28, 0x03, 29, 4, 41, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x064c2130, 33, 1, 0x28, 0x03, 33, 1, 44, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x066c2130, 34, 2, 0x28, 0x03, 34, 2, 47, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc2130, 36, 2, 0x28, 0x03, 36, 2, 48, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec2130, 38, 2, 0x28, 0x03, 38, 2, 52, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x072c6130, 40, 2, 0x28, 0x03, 40, 2, 54, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x062c20b2, 42, 2, 0x28, 0x03, 42, 2, 58, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x064c20b2, 44, 5, 0x28, 0x03, 44, 5, 61, 5, {0x00, 0x00, 0x00, 0x00}},
    {0x062c2132, 49, 5, 0x28, 0x03, 49, 5, 66, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x066c2132, 54, 2, 0x28, 0x03, 54, 2, 70, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x068c2132, 56, 1, 0x28, 0x03, 56, 1, 73, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x06ac2132, 57, 1, 0x28, 0x03, 57, 1, 77, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc2132, 58, 1, 0x28, 0x03, 58, 1, 81, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec2132, 59, 2, 0x28, 0x03, 59, 2, 85, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x070c2132, 61, 2, 0x28, 0x03, 61, 2, 88, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x062c2232, 63, 2, 0x28, 0x03, 63, 2, 91, 5, {0x00, 0x00, 0x00, 0x00}},
    {0x064c2232, 65, 1, 0x28, 0x03, 65, 1, 96, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x066c2232, 66, 2, 0x28, 0x03, 66, 2, 97, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x062caa42, 68, 0, 0x28, 0x03, 68, 0, 98, 0, {0x00, 0x00, 0x00, 0x00}},
};
static const u_char Br3LnkTbl[34] = { /* 0x0193b300 */
    0x02, 0x03, 0x00, 0x03, 0x00, 0x02, 0x04, 0x03, 0x0c, 0x06, 0x05, 0x0b, 0x08, 0x0c, 0x07, 0x09,
    0x0b, 0x08, 0x0b, 0x06, 0x09, 0x0a, 0x04, 0x07, 0x0e, 0x0f, 0x0d, 0x0d, 0x11, 0x12, 0x12, 0x0f,
    0x0f, 0x10,
};
static const SCN_DOOR Br3DorTbl[36] = { /* 0x0193b330 */
    {1, 2}, {4, 3}, {1, 0}, {13, 3}, {4, 0}, {13, 2}, {16, 4}, {16, 3},
    {34, 12}, {35, 6}, {35, 5}, {36, 11}, {37, 11}, {38, 8}, {39, 12}, {38, 7},
    {40, 9}, {41, 11}, {40, 8}, {42, 11}, {36, 6}, {37, 6}, {41, 9}, {42, 10},
    {34, 4}, {39, 7}, {43, 14}, {44, 15}, {43, 13}, {44, 13}, {45, 17}, {46, 18},
    {47, 18}, {45, 15}, {46, 15}, {47, 16},
};
static const u_char Br3Nx2Tbl[32] = { /* 0x0193b380 */
    0x04, 0x04, 0x0c, 0x00, 0x02, 0x07, 0x0b, 0x09, 0x0a, 0x04, 0x09, 0x0b, 0x0c, 0x06, 0x07, 0x0a,
    0x06, 0x09, 0x05, 0x08, 0x08, 0x03, 0x11, 0x12, 0x0f, 0x0e, 0x10, 0x0f, 0x0d, 0x12, 0x0d, 0x11,
};
static const SCN_ENTRY ScnBr3Tbl[20] = { /* 0x0193b3a0 */
    {0x062c20b0, 0, 2, 0x28, 0x03, 0, 2, 0, 1, {0x01, 0x00, 0x00, 0x00}},
    {0x068c2c30, 2, 0, 0x28, 0x03, 2, 0, 1, 0, {0x01, 0x00, 0x00, 0x00}},
    {0x06cc20b0, 2, 2, 0x28, 0x03, 2, 2, 1, 1, {0x01, 0x00, 0x00, 0x00}},
    {0x060c60b0, 4, 3, 0x28, 0x03, 4, 3, 2, 1, {0x01, 0x00, 0x00, 0x00}},
    {0x062c60b0, 7, 2, 0x28, 0x03, 7, 2, 3, 3, {0x01, 0x00, 0x00, 0x00}},
    {0x062c20b3, 9, 1, 0x28, 0x03, 9, 1, 6, 1, {0x01, 0x00, 0x00, 0x00}},
    {0x064c20b3, 10, 2, 0x28, 0x03, 10, 3, 7, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x062c21b3, 12, 2, 0x28, 0x03, 13, 2, 9, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x064c21b3, 14, 2, 0x28, 0x03, 15, 2, 11, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x066c21b3, 16, 2, 0x28, 0x03, 17, 2, 13, 3, {0x01, 0x00, 0x00, 0x00}},
    {0x06ac21b3, 18, 1, 0x28, 0x03, 19, 1, 16, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x06cc21b3, 19, 3, 0x28, 0x03, 20, 4, 18, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x06ec21b3, 22, 2, 0x28, 0x03, 24, 2, 20, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x062c2233, 24, 2, 0x28, 0x03, 26, 2, 22, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x064c2233, 26, 1, 0x28, 0x03, 28, 1, 24, 1, {0x01, 0x00, 0x00, 0x00}},
    {0x066c2233, 27, 3, 0x28, 0x03, 29, 3, 25, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x06ac2233, 30, 1, 0x28, 0x03, 32, 1, 27, 1, {0x01, 0x00, 0x00, 0x00}},
    {0x062c22b3, 31, 1, 0x28, 0x03, 33, 1, 28, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x062c2333, 32, 2, 0x28, 0x03, 34, 2, 30, 2, {0x01, 0x00, 0x00, 0x00}},
    {0x062cea42, 34, 0, 0x28, 0x03, 36, 0, 32, 0, {0x01, 0x00, 0x00, 0x00}},
};
static const u_char Br4LnkTbl[60] = { /* 0x0193b4e0 */
    0x01, 0x10, 0x00, 0x0e, 0x03, 0x04, 0x02, 0x05, 0x02, 0x09, 0x03, 0x07, 0x08, 0x0a, 0x0b, 0x07,
    0x05, 0x06, 0x0c, 0x0d, 0x05, 0x04, 0x05, 0x05, 0x18, 0x07, 0x07, 0x01, 0x16, 0x10, 0x17, 0x00,
    0x0f, 0x14, 0x1c, 0x15, 0x1c, 0x19, 0x1a, 0x1b, 0x11, 0x1a, 0x12, 0x19, 0x0e, 0x17, 0x1b, 0x0f,
    0x16, 0x0b, 0x1c, 0x13, 0x15, 0x13, 0x14, 0x13, 0x16, 0x11, 0x12, 0x18,
};
static const SCN_DOOR Br4DorTbl[60] = { /* 0x0193b520 */
    {29, 1}, {1, 16}, {29, 0}, {0, 14}, {2, 3}, {3, 4}, {2, 2}, {4, 5},
    {3, 2}, {5, 9}, {4, 3}, {6, 7}, {7, 8}, {8, 10}, {9, 11}, {10, 7},
    {6, 5}, {10, 6}, {11, 12}, {12, 13}, {7, 5}, {5, 4}, {8, 5}, {9, 5},
    {13, 24}, {11, 7}, {12, 7}, {0, 1}, {14, 22}, {15, 16}, {16, 23}, {1, 0},
    {15, 15}, {17, 20}, {18, 28}, {19, 21}, {20, 28}, {21, 25}, {22, 26}, {23, 27},
    {17, 17}, {24, 26}, {19, 18}, {25, 25}, {14, 14}, {27, 23}, {26, 27}, {16, 15},
    {27, 22}, {13, 11}, {28, 28}, {21, 19}, {25, 21}, {22, 19}, {24, 20}, {23, 19},
    {26, 22}, {18, 17}, {20, 18}, {28, 24},
};
static const u_char Br4Nx2Tbl[84] = { /* 0x0193b5a0 */
    0x0e, 0x0f, 0x16, 0x10, 0x05, 0x09, 0x04, 0x07, 0x08, 0x0a, 0x0b, 0x03, 0x02, 0x06, 0x0c, 0x0d,
    0x18, 0x05, 0x0c, 0x0d, 0x03, 0x08, 0x0a, 0x0b, 0x07, 0x0a, 0x0b, 0x02, 0x03, 0x07, 0x08, 0x0b,
    0x03, 0x07, 0x08, 0x0a, 0x1c, 0x05, 0x06, 0x0d, 0x05, 0x06, 0x0c, 0x00, 0x17, 0x1b, 0x00, 0x16,
    0x17, 0x12, 0x18, 0x1a, 0x11, 0x18, 0x19, 0x14, 0x15, 0x16, 0x13, 0x1c, 0x13, 0x1c, 0x01, 0x0f,
    0x13, 0x0e, 0x10, 0x1b, 0x05, 0x11, 0x12, 0x12, 0x1a, 0x1b, 0x11, 0x19, 0x1b, 0x0e, 0x17, 0x19,
    0x1a, 0x0b, 0x14, 0x15,
};
static const SCN_ENTRY ScnBr4Tbl[30] = { /* 0x0193b600 */
    {0x06cc60b0, 0, 2, 0x28, 0x03, 0, 2, 0, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x060ce0b0, 2, 2, 0x28, 0x03, 2, 2, 2, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x062c20b4, 4, 2, 0x28, 0x03, 4, 2, 4, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x064c20b4, 6, 2, 0x28, 0x03, 6, 2, 6, 5, {0x00, 0x00, 0x00, 0x00}},
    {0x066c20b4, 8, 2, 0x28, 0x03, 8, 2, 11, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x068c20b4, 10, 5, 0x28, 0x03, 10, 5, 12, 5, {0x00, 0x00, 0x00, 0x00}},
    {0x06ac20b4, 15, 1, 0x28, 0x03, 15, 1, 17, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc20b4, 16, 4, 0x28, 0x03, 16, 4, 20, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec20b4, 20, 1, 0x28, 0x03, 20, 1, 24, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x070c20b4, 21, 1, 0x28, 0x03, 21, 1, 27, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x072c20b4, 22, 1, 0x28, 0x03, 22, 1, 28, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x060c60b4, 23, 2, 0x28, 0x03, 23, 2, 32, 5, {0x00, 0x00, 0x00, 0x00}},
    {0x062c60b4, 25, 1, 0x28, 0x03, 25, 1, 37, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x064c60b4, 26, 1, 0x28, 0x03, 26, 1, 40, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x062c2134, 27, 2, 0x28, 0x03, 27, 2, 43, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x064c2134, 29, 2, 0x28, 0x03, 29, 2, 46, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x066c2134, 31, 2, 0x28, 0x03, 31, 2, 48, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x068c2134, 33, 2, 0x28, 0x03, 33, 2, 49, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x06ac2134, 35, 2, 0x28, 0x03, 35, 2, 52, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x06cc2134, 37, 3, 0x28, 0x03, 37, 3, 55, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x06ec2134, 40, 2, 0x28, 0x03, 40, 2, 58, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x070c2134, 42, 2, 0x28, 0x03, 42, 2, 60, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x072c2134, 44, 3, 0x28, 0x03, 44, 3, 62, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x060c6134, 47, 2, 0x28, 0x03, 47, 2, 65, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x062c21b4, 49, 2, 0x28, 0x03, 49, 2, 68, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x062c2734, 51, 2, 0x28, 0x03, 51, 2, 71, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x064c2734, 53, 2, 0x28, 0x03, 53, 2, 75, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x066c2734, 55, 2, 0x28, 0x03, 55, 2, 78, 4, {0x00, 0x00, 0x00, 0x00}},
    {0x062c20b5, 57, 3, 0x28, 0x03, 57, 3, 82, 3, {0x00, 0x00, 0x00, 0x00}},
    {0x062d2a42, 60, 0, 0x28, 0x03, 60, 0, 85, 0, {0x00, 0x00, 0x00, 0x00}},
};
static const u_char Br5LnkTbl[8] = { /* 0x0193b7e0 */
    0x02, 0x03, 0x04, 0x00, 0x03, 0x01, 0x02, 0x01,
};
static const SCN_DOOR Br5DorTbl[8] = { /* 0x0193b7f0 */
    {0, 2}, {1, 3}, {2, 4}, {0, 0}, {3, 3}, {1, 1}, {3, 2}, {2, 1},
};
static const u_char Br5Nx2Tbl[6] = { /* 0x0193b800 */
    0x03, 0x02, 0x01, 0x00, 0x04, 0x03,
};
static const SCN_ENTRY ScnBr5Tbl[5] = { /* 0x0193b810 */
    {0x062c20b5, 0, 1, 0x28, 0x03, 0, 1, 0, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x064c20b5, 1, 2, 0x28, 0x03, 1, 2, 1, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x066c20b5, 3, 2, 0x28, 0x03, 3, 2, 2, 1, {0x00, 0x00, 0x00, 0x00}},
    {0x068c20b5, 5, 2, 0x28, 0x03, 5, 2, 3, 2, {0x00, 0x00, 0x00, 0x00}},
    {0x072e60b5, 7, 1, 0x28, 0x03, 7, 1, 5, 1, {0x00, 0x00, 0x00, 0x00}},
};
const SCN_BRANCH ScnTbl[SCN_BRANCH_MAX] = { /* 0x0193b860 */
    { ScnBr1Tbl, Br1LnkTbl, Br1DorTbl, NULL, NULL, Br1Nx2Tbl, 49, 0 },  /* 0x0193b860 */
    { ScnBr1Tbl, Br1LnkTbl, Br1DorTbl, Br1DorExtTbl, Br1DorPolTbl, Br1Nx2Tbl, 49, 0 },  /* 0x0193b87c */
    { ScnBr2Tbl, Br2LnkTbl, Br2DorTbl, NULL, NULL, Br2Nx2Tbl, 31, 0 },  /* 0x0193b898 */
    { ScnBr3Tbl, Br3LnkTbl, Br3DorTbl, NULL, NULL, Br3Nx2Tbl, 20, 0 },  /* 0x0193b8b4 */
    { ScnBr4Tbl, Br4LnkTbl, Br4DorTbl, NULL, NULL, Br4Nx2Tbl, 30, 0 },  /* 0x0193b8d0 */
    { ScnBr5Tbl, Br5LnkTbl, Br5DorTbl, NULL, NULL, Br5Nx2Tbl, 5, 0 },  /* 0x0193b8ec */
};
