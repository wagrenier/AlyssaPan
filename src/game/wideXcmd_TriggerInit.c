// game/wideXcmd_TriggerInit.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// ssXcmd_SetAllControlDisable is reconstructed (2026-11-11); the rest are stubs until reconstructed.

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

#include <stdlib.h>

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

// stub of CT3 0x018de260
int ssXcmd_AXE_InitParam(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_AXE_InitParam", 0x018de260);
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

// stub of CT3 0x018dd6b0
int ssXcmd_AXE_ThrowEnd(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_AXE_ThrowEnd", 0x018dd6b0);
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

// stub of CT3 0x018ded70
int ssXcmd_GetChaseAIState(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_GetChaseAIState", 0x018ded70);
    return {};
}

// stub of CT3 0x018de4e0
int ssXcmd_GetMonAImode(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_GetMonAImode", 0x018de4e0);
    return {};
}

// stub of CT3 0x018de7e0
int ssXcmd_IsMonAI_SearchMode(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_IsMonAI_SearchMode", 0x018de7e0);
    return {};
}

// stub of CT3 0x018de580
int ssXcmd_MonsterSetDraw(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_MonsterSetDraw", 0x018de580);
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

// stub of CT3 0x018de4a0
int ssXcmd_SetAdvantageousTime(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetAdvantageousTime", 0x018de4a0);
    return {};
}

// stub of CT3 0x018dec30
int ssXcmd_SetAppearanceTimer(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetAppearanceTimer", 0x018dec30);
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

// stub of CT3 0x018dec60
int ssXcmd_SetChaseFlagAndTimer(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetChaseFlagAndTimer", 0x018dec60);
    return {};
}

// stub of CT3 0x018e0b40
int ssXcmd_SetDurabilityToVar(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetDurabilityToVar", 0x018e0b40);
    return {};
}

// stub of CT3 0x018de410
int ssXcmd_SetHideAttackPole(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetHideAttackPole", 0x018de410);
    return {};
}

// stub of CT3 0x018de670
int ssXcmd_SetMonReturnTimer(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetMonReturnTimer", 0x018de670);
    return {};
}

// stub of CT3 0x018de9a0
int ssXcmd_SetNoReactionTimer(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_SetNoReactionTimer", 0x018de9a0);
    return {};
}

// stub of CT3 0x018de890
int ssXcmd_StayGroup(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_StayGroup", 0x018de890);
    return {};
}

// stub of CT3 0x018e0e30
int ssXcmd_test3(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("ssXcmd_test3", 0x018e0e30);
    return {};
}

// stub of CT3 0x018deed0
int test_ssXcmd_BrainEnd(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("test_ssXcmd_BrainEnd", 0x018deed0);
    return {};
}

// stub of CT3 0x018df5f0
int test_ssXcmd_BrainSet(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("test_ssXcmd_BrainSet", 0x018df5f0);
    return {};
}

// stub of CT3 0x018df5e0
int test_ssXcmd_BrainSet2(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("test_ssXcmd_BrainSet2", 0x018df5e0);
    return {};
}

// stub of CT3 0x018df5d0
int test_ssXcmd_BrainSet3(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("test_ssXcmd_BrainSet3", 0x018df5d0);
    return {};
}

// stub of CT3 0x018def30
int test_ssXcmd_BrainSet4(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("test_ssXcmd_BrainSet4", 0x018def30);
    return {};
}

// stub of CT3 0x018df600
int test_ssXcmd_ChangeGroupID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("test_ssXcmd_ChangeGroupID", 0x018df600);
    return {};
}

// stub of CT3 0x018dee50
int test_ssXcmd_SetGroupID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("test_ssXcmd_SetGroupID", 0x018dee50);
    return {};
}

// stub of CT3 0x018dd590
int wideXcmd_GlobalSetEnable(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_GlobalSetEnable", 0x018dd590);
    return {};
}

// stub of CT3 0x018dd550
int wideXcmd_TriggerInit(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_TriggerInit", 0x018dd550);
    return {};
}
