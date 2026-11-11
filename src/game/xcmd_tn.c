// game/xcmd_tn.c -- Clock Tower 3: the trigger script's x commands by tn / wide (0x018ee5f0..0x018f3cb0;
// the source string "xcmd_tn.c"). Each takes the command's argument strings (argv[0..argc-1]) and the
// bank whose script runs it; ssTrigExecuteActCmd (ssGrpPortalCheck.c) calls them through ssXcmd_functions.
// Slice 1 (2026-11-11): the ambient colour, the waits, the variables, the room-in icons and the small
// bank / flag commands. The rest (rooders, camera, screen adjust, time stop, panic, omni lights, stay
// room, message, map load, filter, time set, bank free, fog animations, movie buffers) is a later slice.

#include <stdlib.h>
#include <string.h>
#include "common/alyssapan_stub.h"
#include "game/xcmd_tn.h"
#include "game/Character_Init.h"
#include "game/QuatSlerp.h"
#include "game/ssGrpPortalCheck.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/unassigned_data.h"

// The bank's part i (the ROM: header + i * 0x380, the flags word at +0x50).
static MODEL_PARTS *XcmdPart(int bank, int i)
{
    return (MODEL_PARTS *)(pModelPartsHeader[bank] + i * PARTS_REC_SIZE);
}

// CT3 0x018eeb40: the title's choice from the script variable (global 0x4c).
int wideXcmd_TitleStart(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    TITLE_SELECT_NUM = (u_char)ssTrigGetVariable(TRIG_VAR_TITLE_SELECT, 1, 0);
    return XCMD_DONE;
}

// CT3 0x018eeb70: the cinema theater (an extra) is unlocked: OMAKE_1, and its global event.
int wideXcmd_CinemaTheaterSet(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    OMAKE_FLAG |= OMAKE_1;
    ssTrigRegisterEvent(TRIG_EVENT_OMAKE_1, 1, 0);
    return XCMD_DONE;
}

// CT3 0x018eebb0: the auto-load at boot, once a frame until it ends. argv: [0, 1] the variable (number,
// global) that gets the message (mcErr_ID - 1) when the flow waits, [2, 3] a variable read for nothing
// (the ROM passes it to MemorycardAutoLoad, which takes one argument), [4, 5] the player's answer.
int tnXcmd_AutoLoad(int argc, char **argv, int bank)
{
    int answer, done;

    (void)argc;
    (void)ssTrigGetVariable(atoi(argv[2]), atoi(argv[3]), bank);
    answer = ssTrigGetVariable(atoi(argv[4]), atoi(argv[5]), bank);
    if (answer == XCMD_AUTOLOAD_PRESS || answer == XCMD_AUTOLOAD_PRESS + 1)
        ssTrigSetVariable(atoi(argv[4]) & 0xffff, answer - XCMD_AUTOLOAD_PRESS, atoi(argv[5]), bank);
    done = MemorycardAutoLoad(answer);
    if (done == 0)
        ssTrigSetVariable(atoi(argv[0]) & 0xffff, mcErr_ID - 1, atoi(argv[1]), bank);
    return done != 0;
}

// CT3 0x018eecd0
int tnXcmd_AutoLoadInit(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    MemorycardAutoLoadInit();
    return XCMD_DONE;
}

// CT3 0x018eecf0: waits for the playing movie to end (and for SkipMain's frames to run out).
int wideXcmd_MovieWait(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    if (MoviePlayCheck == 0)
        return XCMD_DONE;
    if (MovieEndCheck == 0 || SkipMain != 0)
        return XCMD_WAIT;
    MoviePlayCheck = 0;
    MovieEndCheck = 0;
    return XCMD_DONE;
}

// CT3 0x018eed40: waits for SkipMain's frames to run out.
int wideXcmd_SkipWait(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return SkipMain == 0;
}

// CT3 0x018eed50: counts the variable argv[0] (global argv[1]) down by one; done once it goes below 0.
int wideXcmd_FrameWait(int argc, char **argv, int bank)
{
    int left;

    (void)argc;
    left = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank) - 1;
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, left, atoi(argv[1]), bank);
    return left < 0;
}

// CT3 0x018eeff0: ends bank argv[0] (a global bank: its scene drawing off first when it is drawn).
int wideXcmd_GlobalFree(int argc, char **argv, int bank)
{
    char name[sizeof(BankInfo[0].name)];
    int b;

    (void)argc;
    (void)bank;
    b = atoi(argv[0]);
    if (BankInfo[b].state != 0) {
        strncpy(name, BankInfo[b].name, sizeof(name));
        name[XCMD_ROOM_NAME_LEN] = '\0';
        if (BankInfo[b].drawOn != 0)
            TrgGLBLSceneDraw(0, name, b);
        SceneEndBank(b, 1);
    }
    return XCMD_DONE;
}

// CT3 0x018ef090
int wideXcmd_BranchClear(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    BRANCH_CLEAR();
    return XCMD_DONE;
}

// CT3 0x018ef0b0
int wideXcmd_GameOver(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    GAME_OVER_END(atoi(argv[0]));
    return XCMD_DONE;
}

// CT3 0x018ef0e0: a skippable demo. The first time (its skip event not yet registered) DEMO_SKIP_SET is
// 0 and the event is registered; later DEMO_SKIP_SET is 1. With a second argument, the bank's event
// argv[1] follows DEMO_SKIP_SET (registered when the demo may be skipped, removed when not).
int wideXcmd_DemoSkipCheck(int argc, char **argv, int bank)
{
    int event = DemoSkipGetEvent(atoi(argv[0]));

    if (DemoSkipCheckEvent(event) == 0) {
        DEMO_SKIP_SET = 0;
        DemoSkipRegisterEvent(event);
        if (argc == 2)
            ssTrigRemoveEvent(atoi(argv[1]), 0, bank);
    } else {
        DEMO_SKIP_SET = 1;
        if (argc == 2)
            ssTrigRegisterEvent(atoi(argv[1]), 0, bank);
    }
    return XCMD_DONE;
}

// CT3 0x018ef1b0: stops the texture file system's init pages (the 1 it stores is its answer in v0).
int tnXcmd_TexFileInitPageStop(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    TexFileSysInitStop = 1;
    return XCMD_DONE;
}

// CT3 0x018f1340: shows the room-in icon part argv[0] of this bank when the current scene is argv[1]
// (Alyssa's room).
int tnXcmd_Room_IN_ALYSSA(int argc, char **argv, int bank)
{
    char name[16];

    (void)argc;
    strcpy(name, argv[1]);
    UpperCase(name);
    if (memcmp(BankInfo[idCurrentScene].name, name, XCMD_ROOM_NAME_LEN) == 0)
        XcmdPart(bank, atoi(argv[0]))->flags &= ~PARTS_FLAG_FOG;
    return XCMD_DONE;
}

// CT3 0x018f13f0: shows the room-in icon part argv[0] once the global event argv[1] is registered.
// PARTS_FLAG_FOG (bit 0) is the "not drawn" bit here.
int tnXcmd_Room_IN_Icon(int argc, char **argv, int bank)
{
    (void)argc;
    if (ssTrigCheckEvent(atoi(argv[1]), 1, 0) != 0)
        XcmdPart(bank, atoi(argv[0]))->flags &= ~PARTS_FLAG_FOG;
    return XCMD_DONE;
}

// CT3 0x018f1480: hides the room-in icon part argv[0] once the global event argv[1] is registered.
int tnXcmd_Room_IN_Check(int argc, char **argv, int bank)
{
    (void)argc;
    if (ssTrigCheckEvent(atoi(argv[1]), 1, 0) != 0)
        XcmdPart(bank, atoi(argv[0]))->flags |= PARTS_FLAG_FOG;
    return XCMD_DONE;
}

// CT3 0x018f1510: ends the menu map's bank (its drawing off first).
int tnXcmd_Menu_MapOff(int argc, char **argv, int bank)
{
    char name[sizeof(BankInfo[0].name)];

    (void)argc;
    (void)argv;
    (void)bank;
    if (BankInfo[XCMD_MAP_BANK].drawOn != 0) {
        strncpy(name, BankInfo[XCMD_MAP_BANK].name, sizeof(name));
        name[XCMD_ROOM_NAME_LEN] = '\0';
        TrgGLBLSceneDraw(0, name, XCMD_MAP_BANK);
    }
    SceneEndBank(XCMD_MAP_BANK, 1);
    return XCMD_DONE;
}

// CT3 0x018f1680: variable argv[0] (global argv[1]) = variable argv[2] (global argv[3]).
int wideXcmd_Variable_Copy(int argc, char **argv, int bank)
{
    int value;

    (void)argc;
    value = ssTrigGetVariable(atoi(argv[2]), atoi(argv[3]), bank);
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, value, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018f1880: forces a directional light set on character argv[0]: fade argv[1], the light parts
// argv[2..5] (CharDirectLightSet).
int tnXcmd_Char_DirectLight_Change(int argc, char **argv, int bank)
{
    u_short *set;
    int i;

    (void)argc;
    (void)bank;
    set = CharDirectLightSet[atoi(argv[0])];
    set[CHAR_DLS_ON] = 1;
    set[CHAR_DLS_FADE] = (u_short)atoi(argv[1]);
    for (i = 0; i < 4; i++)
        set[CHAR_DLS_DIR + i] = (u_short)atoi(argv[2 + i]);
    return XCMD_DONE;
}

// CT3 0x018f1940: the bank's ambient colour goes to (argv[1], argv[2], argv[3]) over argv[0] frames
// (AmbientAlphaUpdate moves every part's ALL_TABLE.partsPos toward it; the VU1 light programs add it
// to each vertex colour). Until a scene's script runs it, the scene's ambient is black.
int tnXcmd_Scene_Ambient_Change(int argc, char **argv, int bank)
{
    SCENE_ALPHA_ANIM *anim = &scene_alpha_anim[bank];

    (void)argc;
    anim->on = 1;
    anim->steps = atoi(argv[0]);
    anim->target[0] = (float)atoi(argv[1]);
    anim->target[1] = (float)atoi(argv[2]);
    anim->target[2] = (float)atoi(argv[3]);
    return XCMD_DONE;
}

// CT3 0x018f1da0: variable argv[0] (global argv[1]) = the current area's number - 1 (when there is one).
int tnXcmd_ScneName(int argc, char **argv, int bank)
{
    int area;

    (void)argc;
    area = GetAreaNumber(0);
    if (area >= 0)
        ssTrigSetVariable(atoi(argv[0]) & 0xffff, area - 1, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018f1ff0: waits until a scene bank (6..85, one look per group) has the name argv[0] (as many
// characters as it has, upper-cased).
int tnXcmd_LoadWait(int argc, char **argv, int bank)
{
    char name[16];
    size_t len;
    int b;

    (void)argc;
    (void)bank;
    len = strlen(argv[0]);
    strcpy(name, argv[0]);
    UpperCase(name);
    for (b = XCMD_LOAD_WAIT_FIRST_BANK; b < BANK_MAX; b++) {
        if (BankInfo[b].state > 0) {
            if (memcmp(BankInfo[b].name, name, len) == 0)
                return XCMD_DONE;
            b += BankInfo[b].groupCount - 1;
        }
    }
    return XCMD_WAIT;
}

// CT3 0x018f2560: whether any of banks first..first+count-1 is drawn (tnXcmd_SetMovieWorkBuff refuses
// those for the movie's work buffers).
int BankUseCheck(int first, int count)
{
    int b;

    for (b = first; b < first + count; b++) {
        if (BankInfo[b].drawOn != 0)
            return 1;
    }
    return 0;
}
