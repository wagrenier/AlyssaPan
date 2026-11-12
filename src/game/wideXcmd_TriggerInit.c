// game/wideXcmd_TriggerInit.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// ssXcmd_SetAllControlDisable is reconstructed (2026-11-11); slice 2 (2026-11-12): the wide triggers,
// the axe's state, the stalkers' AI commands and the test commands. The rest are stubs until reconstructed.

#include "common/alyssapan_stub.h"
#include "game/wideXcmd_TriggerInit.h"
#include "game/Character_Init.h"
#include "game/BRANCH_CLEAR.h"
#include "game/asMothPause.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/xcmd_tn.h"
#include "game/tiAvoidObject.h"
#include "game/tiXcmd_TransItem.h"
#include "ga/gaSinCos.h"

#include <stdint.h>
#include <stdlib.h>

// The AI brain's bytes these x commands read (CHARACTER.brain; the AI TU, tiAvoidObject.c, owns the
// record: 2026-11-12, from these uses only).
#define BRAIN_AI_MODE 0x000             /* u_char: AI_MODE_* */
#define BRAIN_AXE_181 0x181             /* u_char: AXE_ThrowEnd sets 0xff (the axe stalker's) */
#define BRAIN_AXE_182 0x182             /* u_char: AXE_ThrowEnd sets 0 */
#define BRAIN_RETURN_TIMER 0x218        /* int: frames (SetMonReturnTimer) */

// BRAIN_AI_MODE's values, named from these commands (GetChaseAIState: 1 and 3 chase; IsMonAI_SearchMode:
// 0 searches; SetChaseFlagAndTimer: 4 takes the chase flag away).
enum {
    AI_MODE_SEARCH = 0,
    AI_MODE_CHASE = 1,
    AI_MODE_2 = 2,
    AI_MODE_CHASE_3 = 3,
    AI_MODE_NO_CHASE = 4,
    AI_MODE_5 = 5,
};

#define TRIG_EVENT_CHASE 0xcb           /* global: a stalker with a brain chases (SetChaseFlagAndTimer) */

// ---- x commands (2026-11-11) ----

// ControlDisableflag: ssXcmd_SetAllControlDisable's state.
#define CONTROL_DISABLE_OFF 0           /* controls as they were */
#define CONTROL_DISABLE_ON 1            /* everything stopped; the command waits for the hold event */
#define CONTROL_DISABLE_PARTIAL 2       /* bank script: stalkers and moths running again, the hold continues */
#define SET_ALL_CONTROL_ARGC_MOTION 2   /* "0 <motion>": Alyssa takes that motion when the hold ends */

// CT3 0x018de9d0: "SetAllControlDisable <on> [motion]". on != 0: the stalkers' AI, the menu and the
// player's keys back on (each unless its panic-keep event is set), TRIG_EVENT_MC_RELOADED removed unless
// TRIG_EVENT_PANIC_NO_RELOAD, the moths resumed: done. on == 0: the first run stops them all (registering
// TRIG_EVENT_MC_RELOADED instead) and waits; the next runs wait while the hold event 0x49 is set -- a bank's
// script (bank != 0) then lets the stalkers and moths run again once (CONTROL_DISABLE_PARTIAL); the global
// script's run ends at once -- and when the event clears the command ends, starting Alyssa's motion argv[1]
// when given. The player's keys and the menu stay off after the hold until the "1" form.
// The title script's first command after START (2026-11-11 probe): as a stub it answered XCMD_WAIT for good.
int ssXcmd_SetAllControlDisable(int argc, char **argv, int bank)
{
    if (atoi(argv[0]) != 0) {
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MONSTERS, 1, -1) == 0)
            MonsterAIUpdate(0);
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MENU, 1, -1) == 0)
            ssMenuUpdate(0);
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_PLAYER, 1, -1) == 0)
            PlayerControlUpdate(0);
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_NO_RELOAD, 1, -1) == 0)
            ssTrigRemoveEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
        asMothPause(0);
        return XCMD_DONE;
    }

    if (ControlDisableflag == CONTROL_DISABLE_OFF) {
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MONSTERS, 1, -1) == 0)
            MonsterAIUpdate(1);
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MENU, 1, -1) == 0)
            ssMenuUpdate(1);
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_PLAYER, 1, -1) == 0)
            PlayerControlUpdate(1);
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_NO_RELOAD, 1, -1) == 0)
            ssTrigRegisterEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
        asMothPause(1);
    }
    if (ControlDisableflag == CONTROL_DISABLE_OFF) {
        ControlDisableflag = CONTROL_DISABLE_ON;
        return XCMD_WAIT;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) == 0) {
        ControlDisableflag = CONTROL_DISABLE_OFF;
        if (argc == SET_ALL_CONTROL_ARGC_MOTION)
            asCharacterMortionChangeRequest(0, 0, atoi(argv[1]));
        return XCMD_DONE;
    }
    if (bank == 0)
        return XCMD_DONE;
    if (ControlDisableflag == CONTROL_DISABLE_ON) {
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MONSTERS, 1, -1) == 0)
            MonsterAIUpdate(0);
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_NO_RELOAD, 1, -1) == 0)
            ssTrigRemoveEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
        asMothPause(0);
        ControlDisableflag = CONTROL_DISABLE_PARTIAL;
    }
    return XCMD_WAIT;
}

// ---- the wide triggers (2026-11-12, slice 2) ----

#define GLOBAL_SET_ENABLE_ARGS 2        /* "<bank> <op> <part>...": the parts follow two arguments */

// CT3 0x018dd550: "TriggerInit <bank>": the named bank's triggers back to their start (Trigger_reset).
int wideXcmd_TriggerInit(int argc, char **argv, int bank)
{
    int id = ssGetBankID(argv[0]);
    if (id != -1)
        Trigger_reset(id);
    return XCMD_DONE;
}

// CT3 0x018dd590: "GlobalSetEnable <bank> <op> <part>...": ssTrigSetEnable(op) at once on each named
// part of the named bank; a name the bank lacks is skipped.
int wideXcmd_GlobalSetEnable(int argc, char **argv, int bank)
{
    int id = ssGetBankID(argv[0]);
    if (id != -1) {
        u_int op = (u_int)atoi(argv[1]);
        u_char *header = pModelPartsHeader[id];
        for (u_int i = 0; i < (u_int)(argc - GLOBAL_SET_ENABLE_ARGS); i++) {
            int part = asFindModelPartsID(header, ahGetHash(argv[GLOBAL_SET_ENABLE_ARGS + i]));
            if (part != -1)
                ssTrigSetEnable(part & 0xffff, op & 0xff, 0, id);
        }
    }
    return XCMD_DONE;
}

// ---- the axe (2026-11-12, slice 2) ----

#define AXE_ANGLE_FULL 0x10000          /* gaSetRotMatrix* angles: a turn */
#define AXE_DROP_PERIOD 5               /* AXE_SetPath: the fall grows each 5 ticks */

static AXE_INFO AxeInfo;                /* CT3 0x019447f0 (AxeInfo, a local of this object) */

// One wrap of a turn angle into 0 .. 0xffff (the ROM's, applied once).
static int AxeWrapAngle(int angle)
{
    if (angle < 0)
        angle += AXE_ANGLE_FULL;
    else if (angle >= AXE_ANGLE_FULL)
        angle -= AXE_ANGLE_FULL;
    return angle;
}

// newlib's atoff (float atof), which the ROM links; the host libc has atof (asXcmd.c's too).
static float atoff(const char *s)
{
    return (float)atof(s);
}

// CT3 0x018dd680: the axe stalker's throw in flight (0 for any other character).
int ssAI_AXE_IsThrowing(CHARACTER *chr)
{
    if (chr->id == HIT_CHARA_ONO)
        return AxeInfo.throwing;
    return 0;
}

// CT3 0x018dd6b0: "AXE_ThrowEnd": the hidden parts back, the axe stalker's blow marked landed (its
// CHARA_HIT_LANDED word), the throw over; the stalker's brain bytes +0x181 / +0x182 set to -1 / 0.
int ssXcmd_AXE_ThrowEnd(int argc, char **argv, int bank)
{
    CHARACTER *axe = &Character[HIT_CHARA_ONO];

    for (int i = 0; i < AXE_HIT_MAX; i++) {        // the parts the axe hid shown again (op 1, at once)
        if (AxeInfo.hits[i].bank != 0) {
            ssTrigSetEnable((u_short)AxeInfo.hits[i].part, 1, 0, AxeInfo.hits[i].bank);
            AxeInfo.hits[i].bank = 0;
        }
    }
    *(int *)((u_char *)axe + CHARA_HIT_LANDED) = 1;
    AxeInfo.speed = 0.0f;
    AxeInfo.throwing = 0;
    if (axe->brain != 0) {
        // PORT: brain is an EE word nothing on the host sets yet (open-items.md 1 am).
        u_char *brain = (u_char *)(uintptr_t)axe->brain;
        brain[BRAIN_AXE_181] = 0xff;
        brain[BRAIN_AXE_182] = 0;
    }
    return XCMD_DONE;
}

// CT3 0x018de260: "AXE_InitParam <speed> <drop> <pitch> <yaw>": the throw's direction is the axe
// stalker's facing turned by yaw (row 2 of the Y rotation), then pitched by the X rotation when pitch
// is given; the speed and the fall per AXE_DROP_PERIOD ticks set, the clock at 0, the hidden parts back.
int ssXcmd_AXE_InitParam(int argc, char **argv, int bank)
{
    float m[4][4];
    int yaw = Character[HIT_CHARA_ONO].rot;
    int turn = atoi(argv[3]);

    if (turn != 0)
        yaw = AxeWrapAngle(yaw + turn);
    gaSetRotMatrixY(m, yaw);
    for (int k = 0; k < 4; k++)
        AxeInfo.dir[k] = m[2][k];
    int pitch = atoi(argv[2]);
    if (pitch != 0) {
        float v[4];
        gaSetRotMatrixX(m, AxeWrapAngle(pitch));
        for (int k = 0; k < 4; k++)
            v[k] = m[0][k] * AxeInfo.dir[0] + m[1][k] * AxeInfo.dir[1] + m[2][k] * AxeInfo.dir[2] +
                   m[3][k] * AxeInfo.dir[3];
        for (int k = 0; k < 4; k++)
            AxeInfo.dir[k] = v[k];
    }
    AxeInfo.ticks = 0;
    AxeInfo.speed = atoff(argv[0]);
    AxeInfo.drop = atoff(argv[1]);
    AxeInfo.throwing = 0;
    for (int i = 0; i < AXE_HIT_MAX; i++) {        // the parts the axe hid shown again (op 1, at once)
        if (AxeInfo.hits[i].bank != 0) {
            ssTrigSetEnable((u_short)AxeInfo.hits[i].part, 1, 0, AxeInfo.hits[i].bank);
            AxeInfo.hits[i].bank = 0;
        }
    }
    return XCMD_DONE;
}

// CT3 0x018de3e0: the axe at rest: direction (0, 0, 0, 1) (vf0), no hidden parts, no throw.
void ssAI_AXE_Init(void)
{
    AxeInfo.dir[0] = AxeInfo.dir[1] = AxeInfo.dir[2] = 0.0f;
    AxeInfo.dir[3] = 1.0f;
    for (int i = 0; i < AXE_HIT_MAX; i++)
        AxeInfo.hits[i].bank = 0;
    AxeInfo.ticks = 0;
    AxeInfo.speed = 0.0f;
    AxeInfo.drop = 0.0f;
    AxeInfo.throwing = 0;
}

// ---- the stalkers' AI (2026-11-12, slice 2) ----

// The script's bank's hide poles (ALL_TABLE +0xc4: { records, count }, 0x50 bytes each).
typedef struct HIDE_POLE {
    u_char pad00[0x48];
    u_char flags;                       /* 0x48: HIDE_POLE_FLAG_* */
    u_char pad49;
    short id;                           /* 0x4a: the number SetHideAttackPole names */
    u_char pad4c[4];
} HIDE_POLE;
static_assert(sizeof(HIDE_POLE) == 0x50, "HIDE_POLE");
static_assert(offsetof(HIDE_POLE, id) == 0x4a, "HIDE_POLE.id");
typedef struct HIDE_POLE_LIST {
    u_int poles;                        /* 0x0: HIDE_POLE[count] (EE address in the bank) */
    int count;                          /* 0x4 */
} HIDE_POLE_LIST;
static_assert(sizeof(HIDE_POLE_LIST) == 8, "HIDE_POLE_LIST");
#define HIDE_POLE_FLAG_ATTACK 0x01      /* SetHideAttackPole: a stalker may strike from this pole */

// CT3 0x018de410: "SetHideAttackPole <id>": every hide pole of the script's bank with that id flagged
// HIDE_POLE_FLAG_ATTACK.
int ssXcmd_SetHideAttackPole(int argc, char **argv, int bank)
{
    int id = atoi(argv[0]);
    HIDE_POLE_LIST *list = BANK_PTR(HIDE_POLE_LIST *, bank, All_table_TBL[bank]->hidePoles);
    HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);

    for (int i = 0; i < list->count; i++) {
        if (poles[i].id == id)
            poles[i].flags |= HIDE_POLE_FLAG_ATTACK;
    }
    return XCMD_DONE;
}

// CT3 0x018de4a0: "SetAdvantageousTime <seconds>": the panic's advantageous time in frames, and the
// global event 0x99 (the one Character_Init.h names TRIG_EVENT_MC_LOADED) registered.
int ssXcmd_SetAdvantageousTime(int argc, char **argv, int bank)
{
    AdvantageousTimeTime = atoi(argv[0]) * MC_TIME_FRAMES_PER_SECOND;
    ssTrigRegisterEvent(TRIG_EVENT_MC_LOADED, XCMD_SCOPE_GLOBAL, -1);
    return XCMD_DONE;
}

// A character's AI brain, NULL when it has none.
static u_char *CharaBrain(int chara)
{
    // PORT: brain is an EE word nothing on the host sets yet (open-items.md 1 am); main2.c reads it so.
    return (u_char *)(uintptr_t)Character[chara].brain;
}

// CT3 0x018de4e0: "GetMonAImode <chara> <var>": the bank variable var = the character's AI mode
// (BRAIN_AI_MODE), -1 without a brain.
int ssXcmd_GetMonAImode(int argc, char **argv, int bank)
{
    u_int var = (u_int)atoi(argv[1]);
    u_char *brain = CharaBrain(atoi(argv[0]));
    if (brain == NULL)
        ssTrigSetVariable(var & 0xffff, -1, XCMD_SCOPE_BANK, bank);
    else
        ssTrigSetVariable(var & 0xffff, brain[BRAIN_AI_MODE], XCMD_SCOPE_BANK, bank);
    return XCMD_DONE;
}

// CT3 0x018de580: "MonsterSetDraw <chara> <draw>": ssAI_MonsterDraw.
int ssXcmd_MonsterSetDraw(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);
    ssAI_MonsterDraw(&Character[chara], atoi(argv[1]));
    return XCMD_DONE;
}

// CT3 0x018de670: "SetMonReturnTimer <chara> <op> <seconds>": the brain's return timer (frames) set,
// divided, multiplied, lowered or raised by seconds * 30; another op, or no brain, does nothing.
int ssXcmd_SetMonReturnTimer(int argc, char **argv, int bank)
{
    u_char *brain = CharaBrain(atoi(argv[0]));
    if (brain != NULL) {
        int *timer = (int *)(brain + BRAIN_RETURN_TIMER);
        switch (argv[1][0]) {
        case '=':
            *timer = atoi(argv[2]) * MC_TIME_FRAMES_PER_SECOND;
            break;
        case '/':
            *timer = *timer / (atoi(argv[2]) * MC_TIME_FRAMES_PER_SECOND);
            break;
        case '*':
            *timer = *timer * atoi(argv[2]) * MC_TIME_FRAMES_PER_SECOND;
            break;
        case '-':
            *timer = *timer - atoi(argv[2]) * MC_TIME_FRAMES_PER_SECOND;
            break;
        case '+':
            *timer = *timer + atoi(argv[2]) * MC_TIME_FRAMES_PER_SECOND;
            break;
        default:
            break;
        }
    }
    return XCMD_DONE;
}

// CT3 0x018de7e0: "IsMonAI_SearchMode <chara> <event>": the bank event set while the character's
// brain is in AI_MODE_SEARCH, removed otherwise (or without a brain).
int ssXcmd_IsMonAI_SearchMode(int argc, char **argv, int bank)
{
    u_char *brain = CharaBrain(atoi(argv[0]));
    if (brain == NULL || brain[BRAIN_AI_MODE] != AI_MODE_SEARCH)
        ssTrigRemoveEvent(atoi(argv[1]) & 0xffff, XCMD_SCOPE_BANK, bank);
    else
        ssTrigRegisterEvent(atoi(argv[1]) & 0xffff, XCMD_SCOPE_BANK, bank);
    return XCMD_DONE;
}

#define STAY_GROUP_ARGS 3               /* "<chara> <event> <scope> <room>...": the rooms follow three */

// CT3 0x018de890: "StayGroup <chara> <event> <scope> <room>...": the event set when the character's
// room id (CHARACTER.h100, the portals' room) is one of the listed rooms, removed otherwise.
int ssXcmd_StayGroup(int argc, char **argv, int bank)
{
    short room = Character[atoi(argv[0])].h100;
    for (u_int i = STAY_GROUP_ARGS; i < (u_int)argc; i++) {
        if (room == atoi(argv[i])) {
            ssTrigRegisterEvent(atoi(argv[1]), atoi(argv[2]), bank);
            return XCMD_DONE;
        }
    }
    ssTrigRemoveEvent(atoi(argv[1]), atoi(argv[2]), bank);
    return XCMD_DONE;
}

// CT3 0x018de9a0: "SetNoReactionTimer <n>".
int ssXcmd_SetNoReactionTimer(int argc, char **argv, int bank)
{
    ssAI_SetNoReactionTimer(atoi(argv[0]));
    return XCMD_DONE;
}

// CT3 0x018dec30: "SetAppearanceTimer <n>".
int ssXcmd_SetAppearanceTimer(int argc, char **argv, int bank)
{
    ssAI_SetAppearanceTimer(atoi(argv[0]));
    return XCMD_DONE;
}

// SetChaseFlagAndTimer: the appearance timer each AI mode gets (AI_MODE_NO_CHASE removes the flag).
#define CHASE_TIMER_MODE_0 10
#define CHASE_TIMER_MODE_1 6
#define CHASE_TIMER_MODE_2 8
#define CHASE_TIMER_MODE_3 4
#define CHASE_TIMER_MODE_5 4

// CT3 0x018dec60: "SetChaseFlagAndTimer <chara>": with a brain, the global chase event 0xcb set and the
// appearance timer chosen by its AI mode (mode 4 removes the event again); without one, the event removed.
int ssXcmd_SetChaseFlagAndTimer(int argc, char **argv, int bank)
{
    u_char *brain = CharaBrain(atoi(argv[0]));
    if (brain == NULL) {
        ssTrigRemoveEvent(TRIG_EVENT_CHASE, XCMD_SCOPE_GLOBAL, -1);
        return XCMD_DONE;
    }
    ssTrigRegisterEvent(TRIG_EVENT_CHASE, XCMD_SCOPE_GLOBAL, -1);
    switch (brain[BRAIN_AI_MODE]) {
    case AI_MODE_SEARCH:
        ssAI_SetAppearanceTimer(CHASE_TIMER_MODE_0);
        break;
    case AI_MODE_CHASE:
        ssAI_SetAppearanceTimer(CHASE_TIMER_MODE_1);
        break;
    case AI_MODE_2:
        ssAI_SetAppearanceTimer(CHASE_TIMER_MODE_2);
        break;
    case AI_MODE_CHASE_3:
        ssAI_SetAppearanceTimer(CHASE_TIMER_MODE_3);
        break;
    case AI_MODE_NO_CHASE:
        ssTrigRemoveEvent(TRIG_EVENT_CHASE, XCMD_SCOPE_GLOBAL, -1);
        break;
    case AI_MODE_5:
        ssAI_SetAppearanceTimer(CHASE_TIMER_MODE_5);
        break;
    default:
        break;
    }
    return XCMD_DONE;
}

// CT3 0x018ded70: "GetChaseAIState <chara> <who> <event>": the bank event set while the character's
// brain chases (AI_MODE_CHASE or AI_MODE_CHASE_3) and Player_BankID is who; removed otherwise.
int ssXcmd_GetChaseAIState(int argc, char **argv, int bank)
{
    int player = Player_BankID;
    u_char *brain = CharaBrain(atoi(argv[0]));
    if (brain == NULL || (brain[BRAIN_AI_MODE] != AI_MODE_CHASE && brain[BRAIN_AI_MODE] != AI_MODE_CHASE_3) ||
        player != atoi(argv[1]))
        ssTrigRemoveEvent(atoi(argv[2]) & 0xffff, XCMD_SCOPE_BANK, bank);
    else
        ssTrigRegisterEvent(atoi(argv[2]) & 0xffff, XCMD_SCOPE_BANK, bank);
    return XCMD_DONE;
}

// ---- test commands (2026-11-12, slice 2) ----

// CT3 0x018dee50: "test_SetGroupID <chara> <room>": the room id set when the character has none (-1).
int test_ssXcmd_SetGroupID(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);
    if (Character[chara].h100 == -1)
        Character[chara].h100 = (short)(atoi(argv[1]) & 0xff);
    return XCMD_DONE;
}

// CT3 0x018deed0: "test_BrainEnd <chara>": ssAI_BrainEnd(chara, 0) when it has a brain.
int test_ssXcmd_BrainEnd(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);
    if (Character[chara].brain != 0)
        ssAI_BrainEnd(&Character[chara], 0);
    return XCMD_DONE;
}

// CT3 0x018df5d0, 0x018df5e0, 0x018df5f0, 0x018df600: emptied test commands.
int test_ssXcmd_BrainSet3(int argc, char **argv, int bank)
{
    return XCMD_DONE;
}

int test_ssXcmd_BrainSet2(int argc, char **argv, int bank)
{
    return XCMD_DONE;
}

int test_ssXcmd_BrainSet(int argc, char **argv, int bank)
{
    return XCMD_DONE;
}

int test_ssXcmd_ChangeGroupID(int argc, char **argv, int bank)
{
    return XCMD_DONE;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018dfd70
void ssXcmd_LockToTargChara(int argc, char **argv, short bank)
{
    ALYSSAPAN_STUB("ssXcmd_LockToTargChara", 0x018dfd70);
}

// stub of CT3 0x018dd760
int ssXcmd_AXE_CheckHitAndCompDir(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_AXE_CheckHitAndCompDir", 0x018dd760);
    return {};
}

// stub of CT3 0x018ddcd0
int ssXcmd_AXE_SetPath(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_AXE_SetPath", 0x018ddcd0);
    return {};
}

// stub of CT3 0x018de100
int ssXcmd_AXE_SetStartPos(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_AXE_SetStartPos", 0x018de100);
    return {};
}

// stub of CT3 0x018e01d0
int ssXcmd_ArrowSetChild(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_ArrowSetChild", 0x018e01d0);
    return {};
}

// stub of CT3 0x018e0a90
int ssXcmd_ArrowSetHitPos(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_ArrowSetHitPos", 0x018e0a90);
    return {};
}

// stub of CT3 0x018e0420
int ssXcmd_ArrowSetStartPos(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_ArrowSetStartPos", 0x018e0420);
    return {};
}

// stub of CT3 0x018df6c0
int ssXcmd_CheckAtkHitALYSSA(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_CheckAtkHitALYSSA", 0x018df6c0);
    return {};
}

// stub of CT3 0x018df8a0
int ssXcmd_CheckAtkHitChara(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_CheckAtkHitChara", 0x018df8a0);
    return {};
}

// stub of CT3 0x018dfa10
int ssXcmd_CheckAtkHitMaterial(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_CheckAtkHitMaterial", 0x018dfa10);
    return {};
}

// stub of CT3 0x018de5e0
int ssXcmd_ForceIntoSearchMode(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_ForceIntoSearchMode", 0x018de5e0);
    return {};
}

// stub of CT3 0x018dfb80
int ssXcmd_GetAtkHitInfoID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_GetAtkHitInfoID", 0x018dfb80);
    return {};
}

// stub of CT3 0x018e08a0
int ssXcmd_GetAttackMotionID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_GetAttackMotionID", 0x018e08a0);
    return {};
}

// stub of CT3 0x018df610
int ssXcmd_GetCharFootEndMaterial(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_GetCharFootEndMaterial", 0x018df610);
    return {};
}

// stub of CT3 0x018e09c0
int ssXcmd_ObjectSetHitPos(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_ObjectSetHitPos", 0x018e09c0);
    return {};
}

// stub of CT3 0x018e0c10
int ssXcmd_ScnAtkRequest(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_ScnAtkRequest", 0x018e0c10);
    return {};
}

// stub of CT3 0x018e0670
int ssXcmd_SetArrowPath(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetArrowPath", 0x018e0670);
    return {};
}

// stub of CT3 0x018e0590
int ssXcmd_SetArrowPath2(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetArrowPath2", 0x018e0590);
    return {};
}

// stub of CT3 0x018e0170
int ssXcmd_SetArwRotMaxID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetArwRotMaxID", 0x018e0170);
    return {};
}

// stub of CT3 0x018e0b40
int ssXcmd_SetDurabilityToVar(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetDurabilityToVar", 0x018e0b40);
    return {};
}

// stub of CT3 0x018e0e30
int ssXcmd_test3(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_test3", 0x018e0e30);
    return {};
}

// stub of CT3 0x018def30
int test_ssXcmd_BrainSet4(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("test_ssXcmd_BrainSet4", 0x018def30);
    return {};
}

