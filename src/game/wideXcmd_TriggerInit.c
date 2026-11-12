// game/wideXcmd_TriggerInit.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// ssXcmd_SetAllControlDisable is reconstructed (2026-11-11); slice 2 (2026-11-12): the wide triggers,
// the axe's state, the stalkers' AI commands and the test commands; slice 3 (2026-11-12): the hit-record
// queries and the arrow commands; slice 4 (2026-11-12): the axe's start, the arrows' homing, Alyssa's hits and
// the scene attacks; slices 5-6 (2026-11-12): the bow lock-on, the axe path and hits, the brain set-up (all written).

#include "common/alyssapan_stub.h"
#include "game/wideXcmd_TriggerInit.h"
#include "game/Character_Init.h"
#include "game/BRANCH_CLEAR.h"
#include "game/asMothPause.h"
#include "game/asXcmd.h"
#include "game/asUseArrow.h"
#include "game/LinkMatrixTree.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/xcmd_tn.h"
#include "game/tiAvoidObject.h"
#include "game/tiXcmd_TransItem.h"
#include "game/ssBrain_ChgHideAtk1.h"
#include "game/tiBrain_ChgTrace_BOSS_BALOWS.h"
#include "game/tiBrain_ChgTrace_Ghost.h"
#include "game/QuatSlerp.h"
#include "game/ahalloc.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaSinCos.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

#define TRIG_EVENT_CHASE_FLAG 0xcb           /* global: a stalker with a brain chases (SetChaseFlagAndTimer) */

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
        u_char *brain = CharaBrainPtr(axe);
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
    return CharaBrainPtr(&Character[chara]);
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
        ssTrigRemoveEvent(TRIG_EVENT_CHASE_FLAG, XCMD_SCOPE_GLOBAL, -1);
        return XCMD_DONE;
    }
    ssTrigRegisterEvent(TRIG_EVENT_CHASE_FLAG, XCMD_SCOPE_GLOBAL, -1);
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
        ssTrigRemoveEvent(TRIG_EVENT_CHASE_FLAG, XCMD_SCOPE_GLOBAL, -1);
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

// The emptied test commands: each only answers done.
// CT3 0x018df5d0
int test_ssXcmd_BrainSet3(int argc, char **argv, int bank)
{
    return XCMD_DONE;
}

// CT3 0x018df5e0
int test_ssXcmd_BrainSet2(int argc, char **argv, int bank)
{
    return XCMD_DONE;
}

// CT3 0x018df5f0
int test_ssXcmd_BrainSet(int argc, char **argv, int bank)
{
    return XCMD_DONE;
}

// CT3 0x018df600
int test_ssXcmd_ChangeGroupID(int argc, char **argv, int bank)
{
    return XCMD_DONE;
}

// ---- hits, arrows and objects (2026-11-12, slice 3) ----

#define HIT_DATA_MATERIAL 0x19b         /* HIT_DATA: u_char, the surface's material (the floor a foot ends on) */
#define HIT_DATA_DURABILITY 0x19e       /* HIT_DATA: u_char, a damage shape's durability (SetDurabilityToVar) */
#define HIT_VAR_NONE -1                 /* the answer when nothing matches */
#define HIT_DURABILITY_NONE 0xff        /* SetDurabilityToVar: no shape with that part */
#define ARROW_PATH2_BACK -60.0f         /* SetArrowPath2: the step along the part's X axis (-1 * 60) */

// The short at off of the bank object at EE word ee (a HIT_DATA's part, an object's HIT_OBJ_ID).
static short HitObjShort(int bank, u_int ee, int off)
{
    return *(const short *)(BANK_PTR(const u_char *, bank, ee) + off);
}

// The bank's hit table (ALL_TABLE.hits).
static HIT_TABLE *BankHits(int bank)
{
    return BANK_PTR(HIT_TABLE *, bank, All_table_TBL[bank]->hits);
}

// The floor object Character[chara] stands on (CHARACTER.w300[0], CHARA_TARGET); NULL for none.
// PORT: the EE word has no host form yet (hhXcmd_DressUp.c's FootGround, open-items.md 1 (v)).
static const u_char *FootEndObject(int chara)
{
    (void)chara;
    return NULL;
}

// CT3 0x018df610: "GetCharFootEndMaterial <chara> <var>": the bank variable var = the material of the
// floor the character stands on (HIT_DATA_MATERIAL), -1 with none.
int ssXcmd_GetCharFootEndMaterial(int argc, char **argv, int bank)
{
    const u_char *floor = FootEndObject(atoi(argv[0]));
    if (floor == NULL)
        ssTrigSetVariable(atoi(argv[1]) & 0xffff, HIT_VAR_NONE, XCMD_SCOPE_BANK, bank);
    else
        ssTrigSetVariable(atoi(argv[1]) & 0xffff, floor[HIT_DATA_MATERIAL], XCMD_SCOPE_BANK, bank);
    return XCMD_DONE;
}

// CT3 0x018dfb80: "GetAtkHitInfoID <id> <var>": the bank variable var = the index of this frame's hit
// record whose object has that id, -1 for none. A scene bank (BANK_TYPE_SHADOW) looks in its object hits
// (HIT_OBJ_ID) and then its damage-shape hits (the HIT_DATA's part), each counted from 0; a character bank
// (BANK_TYPE_2) in its attack hits (HIT_OBJ_ID).
int ssXcmd_GetAtkHitInfoID(int argc, char **argv, int bank)
{
    int id = atoi(argv[0]) & 0xffff;
    int type = BankInfo[bank].type;

    if (type == BANK_TYPE_SHADOW) {
        HIT_TABLE *hits = BankHits(bank);
        const HIT_OBJ_REC *objs = BANK_PTR(const HIT_OBJ_REC *, bank, hits->objHits);
        for (int i = 0; i < hits->objHitCount; i++) {
            if (id == HitObjShort(bank, objs[i].object, HIT_OBJ_ID)) {
                ssTrigSetVariable(atoi(argv[1]) & 0xffff, i, XCMD_SCOPE_BANK, bank);
                return XCMD_DONE;
            }
        }
        const HIT_ATK_REC *scn = BANK_PTR(const HIT_ATK_REC *, bank, hits->scnHits);
        for (int i = 0; i < hits->w2b8; i++) {
            if (id == HitObjShort(bank, scn[i].object, offsetof(HIT_DATA, part))) {
                ssTrigSetVariable(atoi(argv[1]) & 0xffff, i, XCMD_SCOPE_BANK, bank);
                return XCMD_DONE;
            }
        }
    } else if (type == BANK_TYPE_2) {
        HIT_TABLE *hits = BankHits(bank);
        const HIT_OBJ_REC *atk = BANK_PTR(const HIT_OBJ_REC *, bank, hits->atkHits);
        for (int i = 0; i < hits->w1c; i++) {
            if (id == HitObjShort(bank, atk[i].object, HIT_OBJ_ID)) {
                ssTrigSetVariable(atoi(argv[1]) & 0xffff, i, XCMD_SCOPE_BANK, bank);
                return XCMD_DONE;
            }
        }
    }
    ssTrigSetVariable(atoi(argv[1]) & 0xffff, HIT_VAR_NONE, XCMD_SCOPE_BANK, bank);
    return XCMD_DONE;
}

// The hit record the bank variable var indexes: a scene bank's (BANK_TYPE_SHADOW) object hits, a
// character bank's (BANK_TYPE_2) attack hits; NULL for another bank type (the commands then do nothing).
static const HIT_OBJ_REC *HitRecByVar(int var, int bank)
{
    int i = ssTrigGetVariable(var, XCMD_SCOPE_BANK, bank);
    int type = BankInfo[bank].type;
    HIT_TABLE *hits = BankHits(bank);

    if (type == BANK_TYPE_SHADOW)
        return &BANK_PTR(const HIT_OBJ_REC *, bank, hits->objHits)[i];
    if (type == BANK_TYPE_2)
        return &BANK_PTR(const HIT_OBJ_REC *, bank, hits->atkHits)[i];
    return NULL;
}

// CT3 0x018df8a0: "CheckAtkHitChara <var> <charaVar> <idVar>": for the hit record var indexes (scene or
// character bank), the bank variable charaVar = the character it hit, and idVar = its attacker's id
// (HIT_ATTACKER_ID) when it has an attacker.
int ssXcmd_CheckAtkHitChara(int argc, char **argv, int bank)
{
    const HIT_OBJ_REC *rec = HitRecByVar(atoi(argv[0]), bank);
    if (rec != NULL) {
        ssTrigSetVariable(atoi(argv[1]) & 0xffff, rec->chara, XCMD_SCOPE_BANK, bank);
        if (rec->id != 0)
            ssTrigSetVariable(atoi(argv[2]) & 0xffff, HitObjShort(bank, (u_int)rec->id, HIT_ATTACKER_ID),
                              XCMD_SCOPE_BANK, bank);
    }
    return XCMD_DONE;
}

// CT3 0x018dfa10: "CheckAtkHitMaterial <var> <material> <event>": the bank event set when the hit record
// var indexes hit that material (bit material & 31 of its materials), removed when not.
int ssXcmd_CheckAtkHitMaterial(int argc, char **argv, int bank)
{
    const HIT_OBJ_REC *rec = HitRecByVar(atoi(argv[0]), bank);
    u_int material = (u_int)atoi(argv[1]);
    if (rec != NULL) {
        if ((rec->materials & (1u << (material & 0x1f))) == 0)
            ssTrigRemoveEvent(atoi(argv[2]) & 0xffff, XCMD_SCOPE_BANK, bank);
        else
            ssTrigRegisterEvent(atoi(argv[2]) & 0xffff, XCMD_SCOPE_BANK, bank);
    }
    return XCMD_DONE;
}

#define ARROW_READY_VALUE 1             /* ArrowSetStartPos: the global ready variable's threshold */

// CT3 0x018e0420: "ArrowSetStartPos <charaVar> <part> <readyVar>": while the global variable readyVar is
// not below 1 (ssTrigCheckVariable TRIG_CMP_LT fails), the character the global variable charaVar names
// (the arrow) is placed on that part of the script's bank -- its position the part's, its yaw from the
// part's X axis (ssRadianByO, axis 1: 0x10000 minus the angle) -- and the command waits; once readyVar
// is below 1 it is done. The ROM also builds the Y rotation of the angle and turns the part's X axis
// by it into a stack temporary nothing reads (0x018e0528..0x018e055c): left out.
int ssXcmd_ArrowSetStartPos(int argc, char **argv, int bank)
{
    if (ssTrigCheckVariable(atoi(argv[2]), ARROW_READY_VALUE, XCMD_SCOPE_GLOBAL, TRIG_CMP_LT, bank) != 0)
        return XCMD_DONE;

    CHARACTER *chr = &Character[ssTrigGetVariable(atoi(argv[0]), XCMD_SCOPE_GLOBAL, bank)];
    const MODEL_PARTS *part = &((const MODEL_PARTS *)pModelPartsHeader[bank])[atoi(argv[1])];
    int angle = (int)((ssRadianByO(part->matrix[0], 1) * 65536.0) / 6.283185307179586);

    chr->rot = AXE_ANGLE_FULL - (angle & 0xffff);
    for (int k = 0; k < 4; k++)
        ((float *)chr->pos)[k] = part->pos[k];
    return XCMD_WAIT;
}

// CT3 0x018e0170: "SetArwRotMaxID <value> <var>": the global variable var = value.
int ssXcmd_SetArwRotMaxID(int argc, char **argv, int bank)
{
    u_int var = (u_int)atoi(argv[1]);
    ssTrigSetVariable(var & 0xffff, atoi(argv[0]), XCMD_SCOPE_GLOBAL, bank);
    return XCMD_DONE;
}

// CT3 0x018e0590: "SetArrowPath2 <chara> <part>": the character (the arrow) moved 60 back along the X
// axis (row 0 of the world matrix) of that part of its bank; all four lanes, w too (vmulbc / vadd).
// The ROM skips a NULL &Character[n], which cannot happen.
int ssXcmd_SetArrowPath2(int argc, char **argv, int bank)
{
    CHARACTER *chr = &Character[atoi(argv[0])];
    float *pos = (float *)chr->pos;
    const MODEL_PARTS *parts = (const MODEL_PARTS *)pModelPartsHeader[chr->bank];
    const float *axis = parts[atoi(argv[1])].matrix[0];

    for (int k = 0; k < 4; k++)
        pos[k] += axis[k] * ARROW_PATH2_BACK;
    return XCMD_DONE;
}

// CT3 0x018e08a0: "GetAttackMotionID <part> <var>": the bank variable var = the motion of the attack
// that hit the script's bank's damage shape of that part this frame (a character's attack only), else -1.
int ssXcmd_GetAttackMotionID(int argc, char **argv, int bank)
{
    int part = atoi(argv[0]) & 0xffff;
    HIT_TABLE *hits = BankHits(bank);
    const HIT_ATK_REC *scn = BANK_PTR(const HIT_ATK_REC *, bank, hits->scnHits);

    for (int i = 0; i < hits->w2b8; i++) {
        if (part == HitObjShort(bank, scn[i].object, offsetof(HIT_DATA, part))) {
            if (scn[i].byObject == 0)
                ssTrigSetVariable(atoi(argv[1]) & 0xffff, scn[i].motion, XCMD_SCOPE_BANK, bank);
            else
                ssTrigSetVariable(atoi(argv[1]) & 0xffff, HIT_VAR_NONE, XCMD_SCOPE_BANK, bank);
            return XCMD_DONE;
        }
    }
    ssTrigSetVariable(atoi(argv[1]) & 0xffff, HIT_VAR_NONE, XCMD_SCOPE_BANK, bank);
    return XCMD_DONE;
}

// CT3 0x018e09c0: "ObjectSetHitPos <part> <object>": the scene object warped (asWarpSceneObject) to where
// the script's bank's damage shape of that part was hit this frame; nothing when it was not.
int ssXcmd_ObjectSetHitPos(int argc, char **argv, int bank)
{
    int part = atoi(argv[0]) & 0xffff;
    HIT_TABLE *hits = BankHits(bank);
    HIT_ATK_REC *scn = BANK_PTR(HIT_ATK_REC *, bank, hits->scnHits);

    for (int i = 0; i < hits->w2b8; i++) {
        if (part == HitObjShort(bank, scn[i].object, offsetof(HIT_DATA, part))) {
            asWarpSceneObject(All_table_TBL[bank], atoi(argv[1]), scn[i].pos);
            return XCMD_DONE;
        }
    }
    return XCMD_DONE;
}

// CT3 0x018e0a90: "ArrowSetHitPos <chara> <var>": the character (the arrow) placed at the attack hit
// the bank variable var indexes (the script's bank's HIT_TABLE.atkHits; GetAtkHitInfoID's answer).
int ssXcmd_ArrowSetHitPos(int argc, char **argv, int bank)
{
    HIT_TABLE *hits = BankHits(bank);
    float *pos = (float *)Character[atoi(argv[0])].pos;
    int i = ssTrigGetVariable(atoi(argv[1]), XCMD_SCOPE_BANK, bank);
    const HIT_OBJ_REC *atk = BANK_PTR(const HIT_OBJ_REC *, bank, hits->atkHits);

    for (int k = 0; k < 4; k++)
        pos[k] = atk[i].pos[k];
    return XCMD_DONE;
}

// CT3 0x018e0b40: "SetDurabilityToVar <part> <var>": the bank variable var = the durability of the
// script's bank's damage shape of that part (HIT_DATA_DURABILITY), 0xff for none.
int ssXcmd_SetDurabilityToVar(int argc, char **argv, int bank)
{
    int part = atoi(argv[0]);
    int value = HIT_DURABILITY_NONE;
    HIT_TABLE *hits = BankHits(bank);
    const u_int *dmg = BANK_PTR(const u_int *, bank, hits->dmgData);

    for (int i = 0; i < hits->dmgCount; i++) {
        const u_char *shape = BANK_PTR(const u_char *, bank, dmg[i]);
        if (part == ((const HIT_DATA *)shape)->part) {
            value = shape[HIT_DATA_DURABILITY];
            break;
        }
    }
    ssTrigSetVariable(atoi(argv[1]) & 0xffff, value, XCMD_SCOPE_BANK, bank);
    return XCMD_DONE;
}

// ---- slice 4 (2026-11-12): the axe's start, the arrows' homing, Alyssa's hits, scene attacks ----

#define AXE_PART_NAME "wo_R_axe"        /* AXE_SetStartPos: the axe stalker's part that also turns the axe */
#define AXE_START_ROLL 0x4000           /* a quarter turn about Z (gaSetRotMatrixZ) for that part */
#define AXE_BOSS_ACTION_START 1         /* TRIG_VAR_BOSS_ACTION in a boss battle once the axe flies */
#define ARROW_CHARA_0 0x0b              /* ArrowSetChild: the two arrow characters, each with its own state */
#define ARROW_CHARA_1 0x0c
#define ARROW_TURN_MAX 400              /* SetArrowPath: the most the arrow turns per run (ahAcos units) */
#define ARROW_STEP 60.0f                /* SetArrowPath: the move along the turned direction */
#define CHECK_ALYSSA_ARGC_KEKKAI 2      /* "<var> <ticks>": the kekkai's ticks are tested too */

// The brain's bytes ForceIntoSearchMode touches (from this use and tiBrain_Init2's).
#define BRAIN_KIND 0x230                /* u_char: the ssAI_BrainInit slot */
#define BRAIN_MOTION 0x23e              /* short: the motion the brain starts (tiBrain_Init2 requests it) */
#define BRAIN_SEARCH_MOTION 0x240       /* short: the motion ForceIntoSearchMode puts back */

// The ssAI_BrainInit table (0x01938580: ssAI_BrainInit[brain kind](chr); slots 6.. are 0).
// PORT: a copy of the ROM's six slots until the AI TU (tiAvoidObject.c) owns and exports it.
#define BRAIN_KIND_MAX 6
typedef int (*BRAIN_INIT)(CHARACTER *chr);
static BRAIN_INIT const ssAI_BrainInit[BRAIN_KIND_MAX] = {
    ssBrain_Init1, tiBrain_Init1, tiBrain_Init2, tiBrain_Init_BOSS1, tiBrain_Init_BOSS_SCISSORS_W, tiBrain_Init3,
};

// vmul / vaddy / vaddz, vrsqrt, vmulq.xyz: xyz by 1 / |xyz|, w kept (the VU's RSQRT of 0: the largest float).
static void XyzNormalize(float *dst, const float *src)
{
    float len2 = src[0] * src[0] + src[1] * src[1] + src[2] * src[2];
    float q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;
    dst[0] = src[0] * q;
    dst[1] = src[1] * q;
    dst[2] = src[2] * q;
    dst[3] = src[3];
}

// VU0 OPMULA / OPMSUB .xyz: out = u x v, w u's.
static void XyzCross(float *out, const float *u, const float *v)
{
    float x = u[1] * v[2] - u[2] * v[1];
    float y = u[2] * v[0] - u[0] * v[2];
    float z = u[0] * v[1] - u[1] * v[0];
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = u[3];
}

// The 4x4 placement matrix of a part: placeRot's three rows and drawOnPos.
static float (*PartPlacement(MODEL_PARTS *part))[4]
{
    return (float (*)[4])part->placeRot;
}

// CT3 0x018de100: "AXE_SetStartPos <part> <axe>": the axe (part `axe` of the script's bank) armed
// (flags43 3) and placed on the axe stalker's part named argv[0]; when that part is AXE_PART_NAME the
// axe also takes the stalker's yaw and a quarter roll. In a boss battle the global TRIG_VAR_BOSS_ACTION
// becomes 1. The throw starts (AxeInfo.throwing).
int ssXcmd_AXE_SetStartPos(int argc, char **argv, int bank)
{
    CHARACTER *axe = &Character[HIT_CHARA_ONO];
    u_char *rec = asFindModelParts(pModelPartsHeader[axe->bank], ahGetHash(argv[0]));
    int rot = axe->rot;
    MODEL_PARTS *part = &((MODEL_PARTS *)pModelPartsHeader[bank])[atoi(argv[1])];

    part->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
    if (strcmp(AXE_PART_NAME, argv[0]) == 0) {
        alignas(16) float ry[4][4], rz[4][4];
        gaSetRotMatrixY(ry, rot);
        gaSetRotMatrixZ(rz, AXE_START_ROLL);
        gaMulMatrix(PartPlacement(part), ry, rz);
    }
    // PORT: the ROM reads the position at rec + 0xf0 even when no part has the name (EE low memory);
    // the host leaves drawOnPos as it was then.
    if (rec != NULL) {
        const MODEL_PARTS *from = (const MODEL_PARTS *)(rec - PARTS_HDR_RECORDS);
        for (int k = 0; k < 4; k++)
            part->drawOnPos[k] = from->pos[k];
    }
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, XCMD_SCOPE_GLOBAL, -1) != 0)
        ssTrigSetVariable(TRIG_VAR_BOSS_ACTION, AXE_BOSS_ACTION_START, XCMD_SCOPE_GLOBAL, bank);
    AxeInfo.throwing = 1;
    return XCMD_DONE;
}

// CT3 0x018de5e0: "ForceIntoSearchMode <chara>": a stalker with a brain back to AI_MODE_SEARCH, its
// search motion restored, and its brain kind's initialiser run again (its answer is not used).
int ssXcmd_ForceIntoSearchMode(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);
    u_char *brain = CharaBrain(chara);

    if (brain != NULL) {
        brain[BRAIN_AI_MODE] = AI_MODE_SEARCH;
        *(short *)(brain + BRAIN_MOTION) = *(short *)(brain + BRAIN_SEARCH_MOTION);
        // PORT: the ROM indexes the table with no bound (slots past the six are 0 there).
        u_char kind = brain[BRAIN_KIND];
        if (kind < BRAIN_KIND_MAX)
            ssAI_BrainInit[kind](&Character[chara]);
    }
    return XCMD_DONE;
}

// CT3 0x018df6c0: "CheckAtkHitALYSSA <var> [ticks]": the bank variable var = the attack (HIT_REC.attack)
// of the first hit on Alyssa this frame by a HIT_KIND_PANIC attacker of the script's bank (a character
// whose bank it is, or a part of it), -1 for none; with ticks, -1 as well while asKatchuKekkaiCheck(ticks).
int ssXcmd_CheckAtkHitALYSSA(int argc, char **argv, int bank)
{
    int alyssa = Character[0].bank;
    HIT_TABLE *hits = BankHits(alyssa);

    if (hits->count != 0) {
        const HIT_REC *recs = BANK_PTR(const HIT_REC *, alyssa, hits->recs);
        for (int i = 0; i < hits->count; i++) {
            const HIT_REC *rec = &recs[i];
            if (BANK_PTR(const u_char *, alyssa, rec->attacker)[HIT_ATTACKER_KIND] != HIT_KIND_PANIC)
                continue;
            int mine = rec->isObject == 0 ? (u_short)Character[rec->who].bank == (u_short)bank
                                          : rec->bank == (u_short)bank;
            if (!mine)
                continue;
            if (argc == CHECK_ALYSSA_ARGC_KEKKAI && asKatchuKekkaiCheck(atoi(argv[1])) != 0) {
                ssTrigSetVariable(atoi(argv[0]) & 0xffff, HIT_VAR_NONE, XCMD_SCOPE_BANK, bank);
                return XCMD_DONE;
            }
            ssTrigSetVariable(atoi(argv[0]) & 0xffff, rec->attack, XCMD_SCOPE_BANK, bank);
            return XCMD_DONE;
        }
    }
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, HIT_VAR_NONE, XCMD_SCOPE_BANK, bank);
    return XCMD_DONE;
}

// ArrowSetChild's state per arrow (statics of this object).
static int flag0_190;                   /* CT3 0x019447e8: ARROW_CHARA_0 has a last direction */
static int flag1_191;                   /* CT3 0x019447ec: ARROW_CHARA_1 */
alignas(16) static float lastTargDir0_192[4]; /* CT3 0x01944850 */
alignas(16) static float lastTargDir1_193[4]; /* CT3 0x01944840 */

// CT3 0x018e01d0: "ArrowSetChild <charaVar> <partVar> <arrow> <event> <arrowPart>": the arrow character
// (ARROW_CHARA_0 / _1) follows the part (bank variables partVar of the character charaVar names): its
// position the part's, and its part arrowPart turned by the turn of the part's X axis since the last run
// (asMakeRotMatrixByQuartanion about now x last). The first run only notes the direction. The command
// waits while the bank event is set; when it clears the arrow forgets its direction and it is done.
int ssXcmd_ArrowSetChild(int argc, char **argv, int bank)
{
    int arrow = atoi(argv[2]);
    CHARACTER *chr = &Character[arrow];
    int target = ssTrigGetVariable(atoi(argv[0]), XCMD_SCOPE_BANK, bank);
    int index = ssTrigGetVariable(atoi(argv[1]), XCMD_SCOPE_BANK, bank);
    MODEL_PARTS *part = &((MODEL_PARTS *)pModelPartsHeader[Character[target].bank])[index];
    int *flag;
    float *last;

    // PORT: another arrow id leaves the ROM with its caller's s0 / s1 (no state): the host uses arrow 0's.
    if (arrow == ARROW_CHARA_1) {
        flag = &flag1_191;
        last = lastTargDir1_193;
    } else {
        flag = &flag0_190;
        last = lastTargDir0_192;
    }

    if (*flag == 0) {
        *flag = 1;
        XyzNormalize(last, part->matrix[0]);
    } else {
        alignas(16) float dir[4], axis[4], m[4][4];
        MODEL_PARTS *mine = &((MODEL_PARTS *)pModelPartsHeader[chr->bank])[atoi(argv[4])];
        XyzNormalize(dir, part->matrix[0]);
        XyzCross(axis, dir, last);
        asMakeRotMatrixByQuartanion(m, axis, ahAcos(gaInnerProductXYZ(last, dir)));
        gaMulMatrix(PartPlacement(mine), m, PartPlacement(mine));
        for (int k = 0; k < 4; k++)
            last[k] = dir[k];
    }
    for (int k = 0; k < 4; k++)
        ((float *)chr->pos)[k] = part->pos[k];
    if (ssTrigCheckEvent(atoi(argv[3]), XCMD_SCOPE_BANK, bank) == 0) {
        *flag = 0;
        return XCMD_DONE;
    }
    return XCMD_WAIT;
}

// The Y flip SetArrowPath composes into the turn (_128, 0x0193a920).
alignas(16) static float _128[4][4] = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, -1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f },
};

// CT3 0x018e0670: "SetArrowPath <arrow> <targetVar> <part>": the arrow homes on the centre of the
// character the global variable targetVar names: its part's backward X axis is turned towards the
// target by at most ARROW_TURN_MAX (the part's placement turned by the same turn and the Y flip), and
// the arrow moves ARROW_STEP along the turned direction. The ROM skips a NULL &Character[n] (impossible).
int ssXcmd_SetArrowPath(int argc, char **argv, int bank)
{
    CHARACTER *chr = &Character[atoi(argv[0])];
    CHARACTER *tgt = &Character[ssTrigGetVariable(atoi(argv[1]), XCMD_SCOPE_GLOBAL, bank)];
    alignas(16) float pos[4], centre[4], back[4], to[4], axis[4], turned[4], m[4][4];
    MODEL_PARTS *part;
    int angle;

    for (int k = 0; k < 4; k++) {
        pos[k] = ((float *)chr->pos)[k];
        centre[k] = ((const SS_OBB *)tgt->obb)->center[k];
    }
    part = &((MODEL_PARTS *)pModelPartsHeader[chr->bank])[atoi(argv[2])];
    for (int k = 0; k < 3; k++)                     // vmulx.xyz by -1: w stays the row's
        back[k] = part->matrix[0][k] * -1.0f;
    back[3] = part->matrix[0][3];
    for (int k = 0; k < 3; k++)                     // vsub.xyz: w stays the centre's
        to[k] = centre[k] - pos[k];
    to[3] = centre[3];
    XyzNormalize(to, to);
    XyzCross(axis, to, back);
    angle = ahAcos(gaInnerProductXYZ(to, back));
    if (angle > ARROW_TURN_MAX)
        angle = ARROW_TURN_MAX;
    asMakeRotMatrixByQuartanion(m, axis, angle);
    for (int k = 0; k < 4; k++)                     // back as a row vector through m, all four lanes
        turned[k] = back[0] * m[0][k] + back[1] * m[1][k] + back[2] * m[2][k] + back[3] * m[3][k];
    gaMulMatrix(m, _128, m);
    gaMulMatrix(PartPlacement(part), m, PartPlacement(part));
    for (int k = 0; k < 3; k++)                     // vmulx.xyz / vadd.xyz: w stays turned's
        ((float *)chr->pos)[k] = turned[k] * ARROW_STEP + pos[k];
    ((float *)chr->pos)[3] = turned[3];
    return XCMD_DONE;
}

// CT3 0x018e0c10: "ScnAtkRequest <event> <object> <chara> <power> <b0d> <var> <motion>": while the
// bank event is set, a scene attack (SS_HIT_ATK_SCENE) by the script's bank's attack object queued each
// run, aimed at chara (its lightBank the bank struck). The object's index is the bank variable var;
// while var is -1 the bank's attack objects are searched for the id (HIT_OBJ_ID) and var remembers it
// (no attack when none has it). Waits while the event is set; done once it clears.
int ssXcmd_ScnAtkRequest(int argc, char **argv, int bank)
{
    if (ssTrigCheckEvent(atoi(argv[0]), XCMD_SCOPE_BANK, bank) == 0)
        return XCMD_DONE;

    u_int var = (u_int)atoi(argv[5]);
    int index = ssTrigGetVariable(var, XCMD_SCOPE_BANK, bank);
    if (index == -1) {
        int id = atoi(argv[1]);
        HIT_TABLE *hits = BankHits(bank);
        const u_int *objs = BANK_PTR(const u_int *, bank, hits->attackObjs);
        for (int i = 0; i < hits->attackObjCount; i++) {
            if (id == HitObjShort(bank, objs[i], HIT_OBJ_ID)) {
                ssHitAddAttackRequest(i, bank & 0xff, SS_HIT_ATK_SCENE, atoi(argv[3]), atoi(argv[2]),
                                      atoi(argv[4]), atoi(argv[6]), Character[atoi(argv[2])].lightBank);
                ssTrigSetVariable(var & 0xffff, i, XCMD_SCOPE_BANK, bank);
                return XCMD_WAIT;
            }
        }
    } else {
        ssHitAddAttackRequest(index, bank & 0xff, SS_HIT_ATK_SCENE, atoi(argv[3]), atoi(argv[2]),
                              atoi(argv[4]), atoi(argv[6]), Character[atoi(argv[2])].lightBank);
    }
    return XCMD_WAIT;
}

// CT3 0x018e0e30: "test3 <event>": waits while the bank event is set.
int ssXcmd_test3(int argc, char **argv, int bank)
{
    return ssTrigCheckEvent(atoi(argv[0]), XCMD_SCOPE_BANK, bank) == 0 ? XCMD_DONE : XCMD_WAIT;
}

// The bow's lock-on (ssXcmd_LockToTargChara).
#define LOCK_NONE 1e+08f                /* the nearest square distance before any candidate */
#define LOCK_RANGE_SQ 250000.0f         /* outside the boss battle: lock within 500 units */
#define LOCK_BOSS_SLOT 6                /* the boss battle never locks on stalker slot 6 */
#define LOCK_TARGET_PART (-1)           /* the variable's value for a scripted part (or nothing) */

// CT3 0x018dfd70: "LockToTargChara <chara> <var>": turns the character to its nearest target and
// stores the target's Character[] slot in the global variable <var> (-1 for a scripted part, or when
// nothing is in range). The candidates are the live stalkers (ssAtkCharaList2) and the parts the
// scripts put on AttackTargetStack. In the boss battle (TRIG_EVENT_BOSS_BATTLE) any distance counts,
// slot 6 is left out and stalkers marked CHARACTER_FLAG_NO_HIT count too; otherwise the target must be
// within 500 units. A character that is not active (CHARACTER_FLAG_SHADOW clear) or has no hits locks
// on nothing. The ROM asks for the event again at each test; so does this.
int ssXcmd_LockToTargChara(int argc, char **argv, int bank)
{
    CHARACTER *chr = &Character[atoi(argv[0])];
    short target = LOCK_TARGET_PART;
    float best = LOCK_NONE;
    float to[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float d[4];
    float origin[4];
    float dist;
    int i, k;

    if ((chr->flags & CHARACTER_FLAG_SHADOW) == 0 || (chr->flags & CHARACTER_FLAG_NO_HIT) != 0) {
        ssTrigSetVariable(atoi(argv[1]) & 0xffff, -1, XCMD_SCOPE_GLOBAL, bank);
        return XCMD_DONE;
    }
    memcpy(origin, chr->pos, sizeof(origin));

    for (i = 0; i < ATK_CHARA_LIST2_COUNT; i++) {
        int slot = ssAtkCharaList2[i];
        CHARACTER *mon = &Character[slot];
        if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, XCMD_SCOPE_GLOBAL, 0) != 0 && slot == LOCK_BOSS_SLOT) {
            continue;
        }
        if ((mon->flags & CHARACTER_FLAG_NO_HIT) != 0
            && ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, XCMD_SCOPE_GLOBAL, 0) == 0) {
            continue;
        }
        if ((mon->flags & CHARACTER_FLAG_SHADOW) == 0) {
            continue;
        }
        for (k = 0; k < 4; k++) {
            d[k] = ((const float *)mon->pos)[k] - origin[k];
        }
        dist = gaInnerProductXYZ(d, d);
        if (dist < best) {
            target = (short)slot;
            memcpy(to, d, sizeof(to));
            best = dist;
        }
    }

    for (i = 0; i < ATTACK_TARGET_MAX; i++) {
        const ATTACK_TARGET *t = &AttackTargetStack[i];
        MODEL_PARTS *part;
        if (t->bank == 0 || BankInfo[t->bank].drawOn == 0) {
            continue;
        }
        part = (MODEL_PARTS *)(pModelPartsHeader[t->bank] + t->target * PARTS_REC_SIZE);
        if ((part->flags & PARTS_FLAG_HIDDEN) != 0) {
            continue;
        }
        for (k = 0; k < 4; k++) {
            d[k] = part->pos[k] - origin[k];
        }
        dist = gaInnerProductXYZ(d, d);
        if (dist < best) {
            target = LOCK_TARGET_PART;
            memcpy(to, d, sizeof(to));
            best = dist;
        }
    }

    if ((ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, XCMD_SCOPE_GLOBAL, 0) != 0 && best != LOCK_NONE)
        || (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, XCMD_SCOPE_GLOBAL, 0) == 0 && best <= LOCK_RANGE_SQ)) {
        int angle = (int)((ssRadianByO(to, 1) * 65536.0) / 6.283185307179586);
        chr->rot = 0x10000 - (angle & 0xffff);
        ssTrigSetVariable(atoi(argv[1]) & 0xffff, target, XCMD_SCOPE_GLOBAL, bank);
    } else {
        ssTrigSetVariable(atoi(argv[1]) & 0xffff, -1, XCMD_SCOPE_GLOBAL, bank);
    }
    return XCMD_DONE;
}

// AxeInfo.dir turned about Y by angle (gaSetRotMatrixY's rows times dir: the ROM's vmula chain, xyzw).
static void AxeTurnDir(int angle)
{
    float m[4][4];
    float v[4];
    gaSetRotMatrixY(m, angle);
    for (int k = 0; k < 4; k++)
        v[k] = m[0][k] * AxeInfo.dir[0] + m[1][k] * AxeInfo.dir[1] + m[2][k] * AxeInfo.dir[2] +
               m[3][k] * AxeInfo.dir[3];
    for (int k = 0; k < 4; k++)
        AxeInfo.dir[k] = v[k];
}

// A yaw in turn units (65536 a turn) as the ROM computes it: dptoli of radians * 65536 / 2 pi.
static int AxeYaw(const float *v)
{
    return (int)((ssRadianByO(v, 1) * 65536.0) / 6.283185307179586);
}

// CT3 0x018ddcd0: "AXE_SetPath <part> <fall> <turn> <home>": one frame of the thrown axe. turn != 0
// turns the direction by turn * dt / 2 a frame; home != 0 steers it toward Alyssa by at most
// home * dt / 2 (the short way round); the part (its drawOnPos) moves by dir * speed * dt, and with
// fall != 0 also drops drop * (2 * (ticks / 5) + 1). dt is the global ticks / 5 (0 when negative);
// the clock (AxeInfo.ticks) runs on the scene ticks.
int ssXcmd_AXE_SetPath(int argc, char **argv, int bank)
{
    MODEL_PARTS *part = (MODEL_PARTS *)(pModelPartsHeader[bank] + atoi(argv[0]) * PARTS_REC_SIZE);
    float step[4];
    float dt;
    int arg;

    arg = atoi(argv[2]);
    if (arg != 0) {
        dt = gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / 5.0f;
        AxeTurnDir(AxeWrapAngle((int)(((float)arg * dt) / 2.0f)));
    }

    arg = atoi(argv[3]);
    if (arg != 0) {
        // The unit vector from the part to Alyssa (xyz; w keeps her position's).
        float to[4];
        float len2, q;
        memcpy(to, Character[0].pos, sizeof(to));
        for (int k = 0; k < 3; k++)
            to[k] -= part->drawOnPos[k];
        len2 = to[0] * to[0] + to[1] * to[1] + to[2] * to[2];
        q = 1.0f / sqrtf(len2);
        for (int k = 0; k < 3; k++)
            to[k] *= q;
        int from = AxeYaw(AxeInfo.dir);
        int want = AxeYaw(to);
        dt = gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / 5.0f;
        int diff = (AXE_ANGLE_FULL - (want & 0xffff)) - (AXE_ANGLE_FULL - (from & 0xffff));
        int limit = (int)(((float)arg * dt) / 2.0f);
        int mag = diff < 0 ? -diff : diff;
        int turn;
        if (mag < limit || AXE_ANGLE_FULL - limit < mag) {
            // Within one frame's turn: go straight there.
            turn = diff < 1 ? diff + AXE_ANGLE_FULL : diff;
        } else if (diff < 1) {
            turn = diff < -0x8000 ? limit : AXE_ANGLE_FULL - limit;
        } else {
            turn = diff > 0x8000 ? AXE_ANGLE_FULL - limit : limit;
        }
        AxeTurnDir(turn);
    }

    dt = gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / 5.0f;
    float scale = AxeInfo.speed * dt;
    for (int k = 0; k < 4; k++)
        step[k] = AxeInfo.dir[k] * scale;
    if (atoi(argv[1]) != 0)
        step[1] -= AxeInfo.drop * (float)((AxeInfo.ticks / AXE_DROP_PERIOD) * 2 + 1);
    for (int k = 0; k < 3; k++)
        part->drawOnPos[k] += step[k];
    part->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
    AxeInfo.ticks += gNextTicksInterval;
    return XCMD_DONE;
}

// AXE_CheckHitAndCompDir's answers in its variable (named from this one writer; the axe script reads them).
#define AXE_RESULT_NONE (-1)            /* nothing hit this frame: fly on */
#define AXE_RESULT_STOP 0               /* stuck in a surface (or no slot left to hide a part): speed 0 */
#define AXE_RESULT_BOUNCE 1             /* glanced off a surface: reflected, the part hit hidden */
#define AXE_RESULT_ARROW 2              /* struck by a strong arrow: it flies on along the arrow */
#define AXE_RESULT_LANDED 3             /* it reached its target (speed 0) */
#define AXE_RESULT_LANDED_CALM 4        /* it reached Alyssa outside a full panic (the speed kept) */
#define AXE_ARROW_LEVEL_CUTS 1          /* an arrow above this level knocks the axe away */
#define AXE_RESULT_VAR_UNSET (-1)       /* BOUNCE needs the variable still at -1 (one bounce a frame) */

// The surface materials (HIT_DATA_MATERIAL) the axe sticks in or glances off.
static int AxeSticksIn(int material)
{
    return material == 0 || material == 3 || material == 6 || material == 7 || material == 9;
}
static int AxeGlancesOff(int material)
{
    return material == 1 || material == 2 || material == 5 || material == 8;
}

// The parts the axe hid shown again (op 1, at once), every slot free.
static void AxeShowHidden(void)
{
    for (int i = 0; i < AXE_HIT_MAX; i++) {
        if (AxeInfo.hits[i].bank != 0) {
            ssTrigSetEnable((u_short)AxeInfo.hits[i].part, 1, 0, AxeInfo.hits[i].bank);
            AxeInfo.hits[i].bank = 0;
        }
    }
}

// CT3 0x018dd760: "AXE_CheckHitAndCompDir <axeId> <part> <var> <bounce>": the thrown axe's hits this
// frame, answered in the bank variable var (AXE_RESULT_*), first match wins:
//  1. a strong arrow (level > 1) struck the scene shape of part <part>: the axe takes the arrow's
//     direction and speed and flies on (ARROW);
//  2. outside the boss battle, the axe stalker's blow landed (CHARA_HIT_LANDED) while its brain's +0x181
//     was set: that byte cleared, then LANDED in a full panic (speed 0), else LANDED_CALM; in the boss
//     battle a landed blow is LANDED;
//  3. the axe stalker was hit by an object of this bank: LANDED;
//  4. the bank's object hit with HIT_OBJ_ID axeId (none: NONE): a surface hit (kind 1) sticks (STOP) or
//     glances off (BOUNCE: dir reflected about the surface by 1 + bounce, speed scaled, the surface's part
//     hidden in a free AxeInfo.hits slot and the previous slot's part shown again; no slot: STOP).
int ssXcmd_AXE_CheckHitAndCompDir(int argc, char **argv, int bank)
{
    int axeId = atoi(argv[0]);
    int target = atoi(argv[1]);
    HIT_TABLE *hits = BankHits(bank);
    CHARACTER *axe = &Character[HIT_CHARA_ONO];

    const HIT_ATK_REC *scn = BANK_PTR(const HIT_ATK_REC *, bank, hits->scnHits);
    for (int i = 0; i < hits->w2b8; i++) {
        const HIT_ATK_REC *rec = &scn[i];
        if (rec->byObject != 1 || HitObjShort(bank, rec->object, offsetof(HIT_DATA, part)) != (target & 0xffff))
            continue;
        HIT_TABLE *by = BankHits(rec->attackBank);
        const u_int *objs = BANK_PTR(const u_int *, rec->attackBank, by->attackObjs);
        const ARROW *arrow = asArrowInformation(HitObjShort(rec->attackBank, objs[rec->attackObj], HIT_OBJ_ID));
        // PORT: the ROM reads the answer unchecked; the host's is NULL without an arrow list.
        if (arrow != NULL && arrow->level > AXE_ARROW_LEVEL_CUTS) {
            memcpy(AxeInfo.dir, arrow->dir, sizeof(AxeInfo.dir));
            AxeInfo.ticks = 0;
            AxeInfo.speed = (float)arrow->shootArg1;
            AxeInfo.drop = 0.0f;
            AxeShowHidden();
            ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_ARROW, XCMD_SCOPE_BANK, bank);
            return XCMD_DONE;
        }
        break;
    }

    int landed = *(int *)((u_char *)axe + CHARA_HIT_LANDED) == 1;
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, XCMD_SCOPE_GLOBAL, -1) == 0) {
        // PORT: brain is an EE word nothing on the host sets yet (open-items.md 1 am).
        u_char *brain = CharaBrain(HIT_CHARA_ONO);
        if (brain != NULL && (s_char)brain[BRAIN_AXE_181] != -1 && landed) {
            brain[BRAIN_AXE_181] = 0xff;
            if (ssTrigCheckEvent(TRIG_EVENT_PANIC_FULL, XCMD_SCOPE_GLOBAL, -1) == 0) {
                ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_LANDED_CALM, XCMD_SCOPE_BANK, bank);
                return XCMD_DONE;
            }
            AxeInfo.speed = 0.0f;
            ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_LANDED, XCMD_SCOPE_BANK, bank);
            return XCMD_DONE;
        }
    } else if (landed) {
        AxeInfo.speed = 0.0f;
        ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_LANDED, XCMD_SCOPE_BANK, bank);
        return XCMD_DONE;
    }

    HIT_TABLE *axeHits = BankHits(axe->bank);
    if (axeHits->count != 0) {
        const HIT_REC *recs = BANK_PTR(const HIT_REC *, axe->bank, axeHits->recs);
        for (int i = 0; i < axeHits->count; i++) {
            if (recs[i].isObject == 1 && recs[i].bank == (u_char)bank) {
                ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_LANDED, XCMD_SCOPE_BANK, bank);
                return XCMD_DONE;
            }
        }
    }

    const HIT_OBJ_REC *objs = BANK_PTR(const HIT_OBJ_REC *, bank, hits->objHits);
    const HIT_OBJ_REC *rec = NULL;
    for (int i = 0; i < hits->objHitCount; i++) {
        if ((axeId & 0xffff) == HitObjShort(bank, objs[i].object, HIT_OBJ_ID)) {
            rec = &objs[i];
            break;
        }
    }
    if (rec == NULL) {
        ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_NONE, XCMD_SCOPE_BANK, bank);
        return XCMD_DONE;
    }
    if (rec->kind != 1)
        return XCMD_DONE;
    // A surface hit: id is the HIT_DATA's EE word in the bank chara names (the ROM hides the part there).
    int surfBank = rec->chara;
    HIT_DATA *surf = BANK_PTR(HIT_DATA *, surfBank, (u_int)rec->id);
    int material = ((const u_char *)surf)[HIT_DATA_MATERIAL];
    if (AxeSticksIn(material)) {
        AxeInfo.speed = 0.0f;
        ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_STOP, XCMD_SCOPE_BANK, bank);
    } else if (AxeGlancesOff(material)
               && ssTrigCheckVariable(atoi(argv[2]), AXE_RESULT_VAR_UNSET, XCMD_SCOPE_BANK, 1, bank) != 0) {
        AxeInfo.speed *= ssGetReflectVector3(atoff(argv[3]) + 1.0f, AxeInfo.dir, surf->worldN, AxeInfo.dir);
        int i;
        for (i = 0; i < AXE_HIT_MAX; i++) {
            if (AxeInfo.hits[i].bank == 0) {
                AxeInfo.hits[i].bank = surfBank;
                AxeInfo.hits[i].part = surf->part;
                ssTrigSetEnable((u_short)surf->part, 0, 0, surfBank);
                if (i > 0)
                    ssTrigSetEnable((u_short)AxeInfo.hits[i - 1].part, 1, 0, AxeInfo.hits[i - 1].bank);
                break;
            }
        }
        ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_BOUNCE, XCMD_SCOPE_BANK, bank);
        if (i == AXE_HIT_MAX) {
            AxeInfo.speed = 0.0f;
            ssTrigSetVariable(atoi(argv[2]) & 0xffff, AXE_RESULT_STOP, XCMD_SCOPE_BANK, bank);
        }
    }
    return XCMD_DONE;
}

// ---- slice 6 (2026-11-12): the brain's set-up ----

// PORT: CHARACTER.brain's records (ct3_types.h: the handle's table). The AI TU (tiAvoidObject.c) takes it
// over with ssAI_BrainFree; test_ssXcmd_BrainSet4 is its first writer.
u_char *CharaBrainHost[CHARACTER_MAX];

// A stalker's brain sets, in its shape bank: ALL_TABLE.ctrl's second word points at them (a camera bank
// keeps its CAM_TRG records there). layout: EE (words relocated to the bank).
typedef struct BRAIN_SET {
    u_int w00;                          /* 0x00: not read here */
    u_int param;                        /* 0x04: the brain's parameter block (0x124 bytes of words) */
    int stateCount;                     /* 0x08: entries in states (the brain keeps 8 at most) */
    u_int states;                       /* 0x0c: BRAIN_STATE_REF[stateCount] */
} BRAIN_SET;
typedef struct BRAIN_STATE_REF {
    u_int w00;                          /* 0x00: not read here */
    u_int state;                        /* 0x04: a state's 16 words */
} BRAIN_STATE_REF;
static_assert(sizeof(BRAIN_SET) == 0x10 && offsetof(BRAIN_SET, states) == 0x0c, "BRAIN_SET");
static_assert(sizeof(BRAIN_STATE_REF) == 8 && offsetof(BRAIN_STATE_REF, state) == 4, "BRAIN_STATE_REF");

#define BRAIN_SIZE 0x280                /* _SunAlloc(gHeapJunc, 0x280, 0x10) */
#define BRAIN_ALIGN 0x10
#define BRAIN_DRAWN 0x002               /* u_char: the stalker is shown (CHARACTER_FLAG_SHADOW at set-up) */
#define BRAIN_ROOM 0x02c                /* u_int: the low word of its room hash (0 with a room argument) */
#define BRAIN_ROOM_ARG 0x030            /* u_char: BrainSet4's sixth argument */
#define BRAIN_STATES 0x080              /* 0x20 bytes each, BRAIN_STATE_COUNT of them */
#define BRAIN_STATE_SIZE 0x20
#define BRAIN_STATE_COUNT 0x180         /* u_char */
#define BRAIN_ID 0x24e                  /* short: ssAI_BrainEnd's second argument (main2.c's name) */
#define BRAIN_SET_FROM_VAR 200          /* the set number: then global variable 200 holds it */
#define BRAIN_ROOM_NONE_HASH 0x83060c1  /* ahGetHash("AAAA"): no room -- the stalker hidden, no light */
#define BRAIN_LIGHT_NONE (-2)           /* CHARACTER.lightBank with no room */
#define BRAIN_INIT_FAILED (-1)          /* an ssAI_BrainInit answer: the brain ended at once */
#define BRAIN_ARGC_ROOM 6               /* "<chara> <set> <mode> <motion> <room> <roomArg>" */
#define BRAIN_U8(b, o) (*(u_char *)((b) + (o)))
#define BRAIN_S16(b, o) (*(short *)((b) + (o)))
#define BRAIN_U32(b, o) (*(u_int *)((b) + (o)))

// The per-slot events BrainSet4 registers (global): the three boss slots get their own; slots 1..6 and
// 0x20 the chase BGM's (sound.h TRIG_EVENT_CHASE); the D4 chase event is removed in any case.
#define BRAIN_CHARA_1D 0x1d
#define BRAIN_CHARA_1E 0x1e
#define BRAIN_CHARA_1F 0x1f
#define BRAIN_CHARA_20 0x20
#define TRIG_EVENT_BRAIN_1D 0xcd
#define TRIG_EVENT_BRAIN_1E 0xce
#define TRIG_EVENT_BRAIN_1F 0xcf

// CT3 0x018def30: "test_BrainSet4 <chara> <set> <mode> <motion> [<room> <roomArg>]": a fresh brain for
// the character (the old one freed): its AI mode, the start motion, its state and parameters copied from
// brain set <set> of its shape bank (BRAIN_SET_FROM_VAR: global variable 200's), frame counts made ticks
// (x TICKS_PER_VSYNC). With a room, the character belongs to it: "AAAA" hides it (no light), else its
// light is the room's bank and an undrawn bank leaves it hidden. A shown stalker is drawn, the set's
// appearance time started, the slot's event registered, the shadow copied from Alyssa's and the brain
// kind's initialiser run (BRAIN_INIT_FAILED: the brain ends). Done in one run.
int test_ssXcmd_BrainSet4(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);
    CHARACTER *chr = &Character[chara];
    u_char *brain;
    const BRAIN_SET *set;
    const int *p;
    int setNo, appear, i;

    if (chr->brain != 0)
        ssAI_BrainFree(chr);
    brain = (u_char *)_SunAlloc(&gHeapJunc, BRAIN_SIZE, BRAIN_ALIGN);
    CharaBrainSet(chr, chara, brain);
    // PORT: the ROM writes into whatever _SunAlloc answered; the host stops on a full heap.
    if (brain == NULL)
        return XCMD_DONE;

    const u_int *ctrl = BANK_PTR(const u_int *, chr->bank, All_table_TBL[chr->bank]->ctrl);
    setNo = atoi(argv[1]);
    if (setNo == BRAIN_SET_FROM_VAR)
        setNo = ssTrigGetVariable(BRAIN_SET_FROM_VAR, XCMD_SCOPE_GLOBAL, bank);
    set = &BANK_PTR(const BRAIN_SET *, chr->bank, ctrl[1])[setNo];
    p = BANK_PTR(const int *, chr->bank, set->param);

    brain[BRAIN_AI_MODE] = (u_char)atoi(argv[2]);
    brain[0x001] = 0;
    brain[BRAIN_DRAWN] = (chr->flags & CHARACTER_FLAG_SHADOW) != 0;
    brain[0x003] = 0;
    BRAIN_S16(brain, 0x004) = -1;
    BRAIN_S16(brain, 0x020) = -1;
    BRAIN_S16(brain, 0x028) = chr->h100;
    brain[0x02a] = (u_char)chr->lightBank;
    BRAIN_U32(brain, BRAIN_ROOM) = (u_int)chr->roomHash;
    BRAIN_S16(brain, 0x032) = -1;
    brain[0x03c] = 0xff;
    BRAIN_S16(brain, 0x03e) = -1;
    BRAIN_S16(brain, 0x040) = -1;
    BRAIN_S16(brain, 0x070) = -1;
    BRAIN_S16(brain, 0x07c) = -1;

    // The parameter block's words (indices into p) into the brain, as the ROM orders them.
    brain[0x183] = (u_char)p[0];
    brain[0x184] = 0;
    for (i = 0; i < 6; i++)
        BRAIN_U32(brain, 0x188 + i * 4) = (u_int)p[1 + i];          /* 0x04..0x18 */
    BRAIN_U32(brain, 0x1a0) = (u_int)p[0x114 / 4];
    for (i = 0; i < 8; i++)
        BRAIN_U32(brain, 0x1a4 + i * 4) = (u_int)p[7 + i];          /* 0x1c..0x38 */
    BRAIN_U32(brain, 0x1c4) = (u_int)p[0x98 / 4];
    BRAIN_U32(brain, 0x1c8) = (u_int)p[0x3c / 4];
    brain[0x1cc] = (u_char)p[0x40 / 4];
    brain[0x1cd] = (u_char)p[0x120 / 4];
    BRAIN_U32(brain, 0x1d0) = 0;
    BRAIN_U32(brain, 0x1dc) = (u_int)p[0x44 / 4];
    BRAIN_U32(brain, 0x1e0) = (u_int)p[0x100 / 4];
    BRAIN_U32(brain, 0x1ec) = (u_int)p[0x48 / 4];
    BRAIN_U32(brain, 0x1f8) = (u_int)p[0x4c / 4];
    BRAIN_U32(brain, 0x208) = (u_int)(p[0x50 / 4] * TICKS_PER_VSYNC);
    BRAIN_U32(brain, 0x20c) = 0;
    BRAIN_U32(brain, 0x214) = (u_int)(p[0x54 / 4] * TICKS_PER_VSYNC);
    BRAIN_U32(brain, BRAIN_RETURN_TIMER) = (u_int)p[0x58 / 4];
    BRAIN_U32(brain, 0x21c) = (u_int)p[0xfc / 4];
    BRAIN_U32(brain, 0x220) = 0;
    BRAIN_U32(brain, 0x224) = (u_int)(p[0x118 / 4] * TICKS_PER_VSYNC);
    BRAIN_U32(brain, 0x228) = (u_int)(p[0x11c / 4] * TICKS_PER_VSYNC);
    brain[0x22c] = (u_char)p[0x104 / 4];
    brain[0x22d] = (u_char)p[0x108 / 4];
    brain[0x22e] = (u_char)p[0x10c / 4];
    brain[0x22f] = (u_char)p[0xf8 / 4];
    for (i = 0; i < 14; i++)
        brain[BRAIN_KIND + i] = (u_char)p[0x5c / 4 + i];            /* 0x5c..0x90; the first is the kind */
    BRAIN_S16(brain, BRAIN_MOTION) = (short)atoi(argv[3]);
    for (i = 0; i < 23; i++)
        BRAIN_S16(brain, BRAIN_SEARCH_MOTION + i * 2) = (short)p[0x9c / 4 + i];  /* 0x9c..0xf4 */
    BRAIN_U32(brain, 0x270) = (u_int)p[0x94 / 4];
    BRAIN_U32(brain, 0x274) = 0xffffffffu;
    appear = p[0x110 / 4];
    BRAIN_U32(brain, 0x278) = 0;

    // The set's states.
    const BRAIN_STATE_REF *refs = BANK_PTR(const BRAIN_STATE_REF *, chr->bank, set->states);
    brain[BRAIN_STATE_COUNT] = (u_char)set->stateCount;
    for (i = 0; i < set->stateCount; i++) {
        const int *s = BANK_PTR(const int *, chr->bank, refs[i].state);
        u_char *st = brain + BRAIN_STATES + i * BRAIN_STATE_SIZE;
        BRAIN_U32(st, 0x00) = 0;
        BRAIN_U32(st, 0x04) = (u_int)(s[13] * TICKS_PER_VSYNC);
        BRAIN_U32(st, 0x08) = (u_int)s[0];
        BRAIN_U32(st, 0x0c) = (u_int)s[1];
        BRAIN_S16(st, 0x10) = (short)s[2];
        BRAIN_S16(st, 0x12) = (short)s[3];
        BRAIN_S16(st, 0x14) = (short)s[4];
        st[0x16] = (u_char)s[5];
        st[0x17] = (u_char)s[6];
        st[0x18] = (u_char)s[7];
        st[0x19] = (u_char)s[14];
        st[0x1a] = (u_char)s[15];
        for (int k = 0; k < 5; k++)
            st[0x1b + k] = (u_char)s[8 + k];
    }

    if (argc == BRAIN_ARGC_ROOM) {
        BRAIN_U32(brain, BRAIN_ROOM) = 0;
        chr->roomHash = ahGetHash(argv[4]);
        brain[BRAIN_ROOM_ARG] = (u_char)atoi(argv[5]);
        if (chr->roomHash == BRAIN_ROOM_NONE_HASH) {
            chr->lightBank = BRAIN_LIGHT_NONE;
            ssAI_MonsterDraw(chr, 0);
            brain[BRAIN_DRAWN] = 0;
        } else {
            chr->lightBank = (short)ssAI_GetScnBankID((u_int)chr->roomHash);
            // PORT: the ROM reads BankInfo[-1] when no bank has the room; the host treats it as undrawn.
            if (chr->lightBank < 0 || BankInfo[chr->lightBank].drawOn == 0) {
                chr->flags &= ~CHARACTER_FLAG_SHADOW;
                brain[BRAIN_DRAWN] = 0;
            }
        }
    }
    if (brain[BRAIN_DRAWN] != 0)
        ssAI_MonsterDraw(chr, 1);
    if (appear != 0)
        ssAI_SetAppearanceTimer(appear);

    switch (chara) {
    case BRAIN_CHARA_1F:
        ssTrigRegisterEvent(TRIG_EVENT_BRAIN_1F, XCMD_SCOPE_GLOBAL, -1);
        break;
    case BRAIN_CHARA_1E:
        ssTrigRegisterEvent(TRIG_EVENT_BRAIN_1E, XCMD_SCOPE_GLOBAL, -1);
        break;
    case BRAIN_CHARA_1D:
        ssTrigRegisterEvent(TRIG_EVENT_BRAIN_1D, XCMD_SCOPE_GLOBAL, -1);
        break;
    case 1: case 2: case 3: case 4: case 5: case 6: case BRAIN_CHARA_20:
        ssTrigRegisterEvent(TRIG_EVENT_CHASE, XCMD_SCOPE_GLOBAL, -1);
        break;
    default:
        break;
    }
    ssTrigRemoveEvent(TRIG_EVENT_CHASE_D4, XCMD_SCOPE_GLOBAL, -1);
    asCopyShadowInformation(chr->id, 0);
    // PORT: the ROM indexes the table with no bound (slots past the six are 0 there).
    u_char kind = brain[BRAIN_KIND];
    if (kind < BRAIN_KIND_MAX && ssAI_BrainInit[kind](chr) == BRAIN_INIT_FAILED)
        ssAI_BrainEnd(chr, BRAIN_S16(brain, BRAIN_ID));
    return XCMD_DONE;
}

