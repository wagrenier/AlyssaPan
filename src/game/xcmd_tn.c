// game/xcmd_tn.c -- Clock Tower 3: the trigger script's x commands by tn / wide (0x018ee5f0..0x018f3cb0;
// the source string "xcmd_tn.c"). Each takes the command's argument strings (argv[0..argc-1]) and the
// bank whose script runs it; ssTrigExecuteActCmd (ssGrpPortalCheck.c) calls them through ssXcmd_functions.
// Slice 1 (2026-11-11): the ambient colour, the waits, the variables, the room-in icons and the small
// bank / flag commands. Slice 2 (2026-11-11): clips, panic, continue, time stop, the frees, stay room,
// the message bank, the menu map, the fog animations, wideXcmd_PlayAnimeToFream2. Slice 3 (2026-11-11):
// the camera-time control lock, screen adjust, omni lights, the screen filter, movie buffers, the
// puzzle's start, the current scene, the texture page and the clear's points. Left: key lock, time set,
// the puzzle's animation and the other anime / variable commands.

#include <stdio.h>
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
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/asMothPause.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/movie.h"
#include "ga/gaInit3dlib.h"

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

// ---- slice 2 (2026-11-11): clips, panic, continue, time stop, frees, stay room, the message bank, the
// menu map, the fog animations and the scene animation command ----

// CT3 0x019394d0 (RoomFileName_STR_Tbl, 53 words): the menu map's file for each area number
// (GetAreaNumber's answer; area 0 has none). PORT: read-only data, kept here beside its one reader.
static const char *const RoomFileName_STR_Tbl[XCMD_AREA_COUNT] = {
    NULL,   "M101", "M102", "M103", "M104", "M105", "M106", "M107", "M108", "M109", "M110",
    "M203", "M204", "M205", "M206", "M207", "M208", "M101", "M102", "M301", "M302", "M303",
    "M304", "M305", "M306", "M307", "M308", "M309", "M310", "M311", "M312", "M313", "M401",
    "M402", "M403", "M404", "M405", "M407", "M406", "M408", "M409", "M410", "M411", "M412",
    "M413", "M414", "M415", "M416", "M409", "M502", "M503", "M504", "M308",
};

// CT3 0x0194493c (load_id): the area whose map tnXcmd_Menu_MapLoad loaded last.
int load_id;

// layout: host (a port table, not ROM data). The script's track kinds by name (argv[4] of
// wideXcmd_PlayAnimeToFream2) and the part selection each
// takes (asSceneAnimationStart's pose / other / byPart, bits 0..2).
typedef struct XCMD_ANIM_TYPE {
    const char *name;
    int kind;                           /* TRACK_KIND_* */
    int select;                         /* XCMD_ANIM_SEL_* */
} XCMD_ANIM_TYPE;
#define XCMD_ANIM_SEL_POSE 0x1
#define XCMD_ANIM_SEL_OTHER 0x2
#define XCMD_ANIM_SEL_BY_PART 0x4
static const XCMD_ANIM_TYPE XcmdAnimTypes[] = {
    {"ANIM_TYPE_ROT", TRACK_KIND_ROT, XCMD_ANIM_SEL_POSE},
    {"ANIM_TYPE_POS", TRACK_KIND_POS, XCMD_ANIM_SEL_POSE},
    {"ANIM_TYPE_SCA", TRACK_KIND_VECTOR, XCMD_ANIM_SEL_POSE},
    {"ANIM_TYPE_NODE_ROT", TRACK_KIND_BASE_ROT, XCMD_ANIM_SEL_POSE},
    {"ANIM_TYPE_NODE_POS", TRACK_KIND_BASE_POS, XCMD_ANIM_SEL_POSE},
    {"ANIM_TYPE_VERT", TRACK_KIND_VERTEX, XCMD_ANIM_SEL_POSE},
    {"ANIM_TYPE_NORM", TRACK_KIND_NORMAL, XCMD_ANIM_SEL_POSE},
    {"ANIM_TYPE_REFRACT", TRACK_KIND_FLOAT1_1_14, XCMD_ANIM_SEL_OTHER},
    {"ANIM_TYPE_BUMPMAP", TRACK_KIND_NUMBER, XCMD_ANIM_SEL_OTHER},
    {"ANIM_TYPE_LIGHT", TRACK_KIND_VECTOR_XYZ, XCMD_ANIM_SEL_OTHER},
    {"ANIM_TYPE_FOCAL_LEN", TRACK_KIND_FLOAT0_7_9, XCMD_ANIM_SEL_OTHER},
    {"ANIM_TYPE_ALPHA", TRACK_KIND_SUB_NUMBER, XCMD_ANIM_SEL_OTHER},
    {"ANIM_TYPE_UV", TRACK_KIND_UV, XCMD_ANIM_SEL_BY_PART},
    {"ANIM_TYPE_PARAPARA", TRACK_KIND_COPY, XCMD_ANIM_SEL_BY_PART},
};

// A number argument as the script gives it: argv[i + 1] XCMD_VALUE_CONST takes argv[i] itself, 0 / 1
// read variable argv[i] (1: global). *ok is 0 for any other source (the ROM then uses a stale register).
static int XcmdValue(char **argv, int i, int bank, int *ok)
{
    int source = atoi(argv[i + 1]);

    *ok = 1;
    if (source == XCMD_VALUE_CONST)
        return atoi(argv[i]);
    if (source == 1 || source == 0)
        return ssTrigGetVariable(atoi(argv[i]) & 0xffff, source, bank);
    *ok = 0;
    return 0;
}

// CT3 0x018ef1c0: stops time (not while paused, a pause is booked or the menu is held by a trigger: the
// command waits). argv[0] 0 sets the default stop (pause, player, menu) for argv[1] frames; otherwise
// time_stop_flag as it stands. Each stopped system stays running when its panic "keep" event is set.
int tnXcmd_TimeStop(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    if (pause != 0 || pause_reservation != 0 || (TrgCharStopFlag & TRG_CHAR_STOP_MENU) != 0)
        return XCMD_WAIT;
    if (atoi(argv[0]) == 0) {
        time_stop_flag = TIME_STOP_PAUSE_OFF | TIME_STOP_PLAYER | TIME_STOP_MENU;
        time_stop_fream = atoi(argv[1]);
    }
    if ((time_stop_flag & TIME_STOP_PAUSE_OFF) != 0) {
        pause_reservation = 1;
        pause = 1;
    }
    if ((time_stop_flag & TIME_STOP_PLAYER) != 0 && ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_PLAYER, 1, -1) == 0)
        PlayerControlUpdate(1);
    if ((time_stop_flag & TIME_STOP_MONSTER) != 0) {
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MONSTERS, 1, -1) == 0)
            MonsterAIUpdate(1);
        if (ssTrigCheckEvent(TRIG_EVENT_PANIC_NO_RELOAD, 1, -1) == 0)
            ssTrigRegisterEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
        asMothPause(1);
    }
    if ((time_stop_flag & TIME_STOP_MENU) != 0 && ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MENU, 1, -1) == 0)
        ssMenuUpdate(1);
    return XCMD_DONE;
}

// CT3 0x018ef320: argv[0] "LOAD" / "load" asks the continue screen to load; anything else keeps the
// continue backup (ContinueBackupSet) with TRIG_EVENT_CONTINUE registered while it copies, as before
// afterwards.
int tnXcmd_Continue(int argc, char **argv, int bank)
{
    int had;

    (void)argc;
    (void)bank;
    if (strcmp("LOAD", argv[0]) == 0 || strcmp("load", argv[0]) == 0) {
        load_continue = 1;
    } else {
        had = ssTrigCheckEvent(TRIG_EVENT_CONTINUE, 1, 0);
        ssTrigRegisterEvent(TRIG_EVENT_CONTINUE, 1, 0);
        ContinueBackupSet();
        if (had == 0)
            ssTrigRemoveEvent(TRIG_EVENT_CONTINUE, 1, 0);
    }
    return XCMD_DONE;
}

// The end of a full panic: the mode bit and both panic events off.
static void XcmdPanicOff(void)
{
    PanicInfoData.mode &= ~PANIC_MODE_FULL;
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_FULL, 1, 0);
}

// CT3 0x018ef3d0: ends a full panic. argv[0] (0 with no arguments): XCMD_END_PANIC_* -- whether only a
// running one ends, and where the gauge goes (kept, 0, or Panicdef.beatFrom, where the heartbeat starts).
int tnXcmd_EndPanic(int argc, char **argv, int bank)
{
    int mode = argc == 0 ? XCMD_END_PANIC_KEEP : atoi(argv[0]);
    int full = (PanicInfoData.mode & PANIC_MODE_FULL) != 0;

    (void)bank;
    switch (mode) {
    case XCMD_END_PANIC_FORCE_BEAT:
        XcmdPanicOff();
        PanicInfoData.gauge = Panicdef.beatFrom;
        break;
    case XCMD_END_PANIC_FORCE_ZERO:
        PanicInfoData.gauge = 0.0f;
        XcmdPanicOff();
        break;
    case XCMD_END_PANIC_BEAT:
        if (full) {
            XcmdPanicOff();
            PanicInfoData.gauge = Panicdef.beatFrom;
        }
        break;
    case XCMD_END_PANIC_ZERO:
        if (full) {
            PanicInfoData.gauge = 0.0f;
            XcmdPanicOff();
        }
        break;
    case XCMD_END_PANIC_KEEP:
        if (full)
            XcmdPanicOff();
        break;
    default:
        break;
    }
    return XCMD_DONE;
}

// CT3 0x018efa90: registers event argv[2] (global argv[3]) while character argv[0]'s room is the room
// argv[1] (4 characters, upper-cased), removes it otherwise.
int tnXcmd_StayRoom(int argc, char **argv, int bank)
{
    char room[112];
    char want[112];

    (void)argc;
    strcpy(room, asConvertHash2String(Character[atoi(argv[0])].roomHash));
    strcpy(want, argv[1]);
    UpperCase(want);
    if (memcmp(room, want, XCMD_ROOM_NAME_LEN) == 0)
        ssTrigRegisterEvent(atoi(argv[2]), atoi(argv[3]), bank);
    else
        ssTrigRemoveEvent(atoi(argv[2]), atoi(argv[3]), bank);
    return XCMD_DONE;
}

// CT3 0x018efb80: the message bank (2) around a change of messages. argv[0] "set" loads back the file
// kept in Backup_Filename (when the bank is not drawn); anything else waits for the bank's load, keeps
// its name (4 characters) and ends it.
int wideXcmd_ReadChengeMsg(int argc, char **argv, int bank)
{
    BANK_INFO *msg = &BankInfo[XCMD_MSG_BANK];

    (void)argc;
    (void)bank;
    if (strcmp("set", argv[0]) == 0) {
        if (Backup_Filename[0] == '\0') {
            printf("Attention!! Not Free Msg processed!!\n");
        } else if (msg->drawOn == 0) {
            TrgGLBLSceneLoad(Backup_Filename, XCMD_MSG_BANK);
            Backup_Filename[0] = '\0';
        }
    } else {
        FSMltLoadEndWaitFile(XCMD_MSG_BANK, NULL);
        if (msg->state == 0) {
            printf("Attention!! Not Draw Msg processed!!\n");
        } else {
            strcpy(Backup_Filename, msg->name);
            Backup_Filename[XCMD_ROOM_NAME_LEN] = '\0';
            if (msg->drawOn != 0)
                TrgGLBLSceneDraw(0, Backup_Filename, XCMD_MSG_BANK);
            SceneEndBank(XCMD_MSG_BANK, 1);
        }
    }
    return XCMD_DONE;
}

// CT3 0x018f1570: loads the menu map of the current area into bank 4 (the old one's drawing off first)
// and cuts to its camera: argv[0] 0 "Camera02", else "Camera01".
int tnXcmd_Menu_MapLoad(int argc, char **argv, int bank)
{
    char name[sizeof(BankInfo[0].name)];
    char *file;

    (void)argc;
    (void)bank;
    load_id = GetAreaNumber(0);
    if (load_id < 0)
        return XCMD_DONE;
    if (BankInfo[XCMD_MAP_BANK].drawOn != 0) {
        strcpy(name, BankInfo[XCMD_MAP_BANK].name);
        name[XCMD_ROOM_NAME_LEN] = '\0';
        TrgGLBLSceneDraw(0, name, XCMD_MAP_BANK);
    }
    file = (char *)RoomFileName_STR_Tbl[load_id];
    if (file == NULL) // PORT: area 0 has no map (the ROM would pass a null name on)
        return XCMD_DONE;
    TrgGLBLSceneLoad(file, XCMD_MAP_BANK);
    TrgGLBLSceneDraw(1, file, XCMD_MAP_BANK);
    if (atoi(argv[0]) == 0)
        CamChange_Hash_S_Hash(XCMD_MAP_BANK, ahGetHash("Camera02"), 1, 0);
    else
        CamChange_Hash_S_Hash(XCMD_MAP_BANK, ahGetHash("Camera01"), 1, 0);
    return XCMD_DONE;
}

// CT3 0x018f1e20: frees bank argv[1], which should hold "<argv[0]>.BIN": its queued load cancelled, its
// load waited for, its drawing ended by its kind (FS_LOAD_*) and the bank group ended.
int tnXcmd_BankFree(int argc, char **argv, int bank)
{
    char file[16];
    int b, chara;
    BANK_INFO *info;

    (void)argc;
    (void)bank;
    b = atoi(argv[1]);
    info = &BankInfo[b];
    snprintf(file, sizeof(file), "%s.BIN", argv[0]);
    if (memcmp(info->name, file, XCMD_ROOM_NAME_LEN) != 0) {
        // The ROM formats "tnXcmd_BankFree (Bank:%d) / Free Scene Name / Stay Scene Name" for
        // WideErrorMsgDebug (xcmd_tn.c line 0x7c4), which the retail build leaves empty.
        WideErrorMsgDebug();
    }
    FSMltLoadCancelQue(file);
    FSMltLoadEndWaitFile(b, NULL);
    if (info->drawOn != 0) {
        switch (info->type) {
        case FS_LOAD_BEHAVIOR:
            // PORT: the ROM passes a register it never set as the character; the bank's owner is meant.
            chara = BankID_TO_CharID((short)b);
            if (chara >= 0)
                TrgCharacterBehaviorFree(chara, b);
            break;
        case FS_LOAD_SHAPE:
            chara = BankID_TO_CharID((short)b);
            TrgCharacterDraw(0, chara);
            TrgCharacterShapeFree(chara);
            break;
        case FS_LOAD_SCENE:
            TrgSceneDraw(0, 0, 0, argv[0], b);
            break;
        case FS_LOAD_GLBL:
            TrgGLBLSceneDraw(0, argv[0], b);
            break;
        default:
            break;
        }
    }
    SceneEndBank(b, info->groupCount);
    return XCMD_DONE;
}

// CT3 0x018f20b0: the character fog of the bank eases over argv[10] frames; each of argv[0], [2], [4],
// [6] non-zero animates charaFog[0..2] (to argv[1], [3], [5]) and charaFogArg (to argv[7..9]).
int tnXcmd_CharFogAnimation(int argc, char **argv, int bank)
{
    FOG_ANIME *fa = &FogAnimeBuff[bank];
    int i;

    (void)argc;
    fa->charaFlags = 0;
    for (i = 0; i < 3; i++) {
        if (atoi(argv[i * 2]) != 0) {
            fa->charaFog[i] = (float)atoi(argv[i * 2 + 1]);
            fa->charaFlags |= FOG_ANIME_FOG0 << i;
        }
    }
    if (atoi(argv[6]) != 0) {
        for (i = 0; i < 3; i++)
            fa->charaFogArg[i] = atoi(argv[7 + i]);
        fa->charaFlags |= FOG_ANIME_FOG_ARG;
    }
    fa->charaTime = (short)atoi(argv[10]);
    return XCMD_DONE;
}

// CT3 0x018f2240: the bank's own fog eases over argv[14] frames: as tnXcmd_CharFogAnimation for fog[]
// and fogArg[], and argv[10] non-zero animates the clear colour (to argv[11..13]).
int tnXcmd_SceneFogAnimation(int argc, char **argv, int bank)
{
    FOG_ANIME *fa = &FogAnimeBuff[bank];
    int i;

    (void)argc;
    fa->flags = 0;
    for (i = 0; i < 3; i++) {
        if (atoi(argv[i * 2]) != 0) {
            fa->fog[i] = (float)atoi(argv[i * 2 + 1]);
            fa->flags |= FOG_ANIME_FOG0 << i;
        }
    }
    if (atoi(argv[6]) != 0) {
        for (i = 0; i < 3; i++)
            fa->fogArg[i] = atoi(argv[7 + i]);
        fa->flags |= FOG_ANIME_FOG_ARG;
    }
    if (atoi(argv[10]) != 0) {
        for (i = 0; i < 3; i++)
            fa->clearColor[i] = atoi(argv[11 + i]);
        fa->flags |= FOG_ANIME_CLEAR_COLOR;
    }
    fa->time = (short)atoi(argv[14]);
    return XCMD_DONE;
}

// CT3 0x018f25c0: the panic gauge. The value is argv[0] by argv[1]'s source (XcmdValue). With four
// arguments and argv[3] "DIRECT" the gauge takes it; with four and anything else, nothing; otherwise
// PanicSet(value, source argv[2]) adds it as a rise.
int tnXcmd_SetPanic(int argc, char **argv, int bank)
{
    int ok;
    float value = (float)XcmdValue(argv, 0, bank, &ok);

    if (argc == 4) {
        if (strcmp("DIRECT", argv[3]) == 0)
            PanicInfoData.gauge = value;
    } else {
        PanicSet(value, atoi(argv[2]));
    }
    return XCMD_DONE;
}

// CT3 0x018f26c0: the 3D library's near Z (screen_0[GA_SCREEN_NEAR]).
int tnXcmd_SetNearClip(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    screen_0[GA_SCREEN_NEAR] = (float)atoi(argv[0]);
    return XCMD_DONE;
}

// CT3 0x018f26f0: the far Z (screen_0[GA_SCREEN_FAR]).
int tnXcmd_SetFarClip(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    screen_0[GA_SCREEN_FAR] = (float)atoi(argv[0]);
    return XCMD_DONE;
}

// CT3 0x018f2720: frees character argv[0]'s behaviour bank argv[1]. With three arguments and argv[2]
// "FREE" the bank group is ended as well -- only when the bank is one of the character's.
int tnXcmd_BehaviorFree(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);
    int b = atoi(argv[1]);
    int i;

    (void)bank;
    if (argc == 3 && strcmp(argv[2], "FREE") == 0) {
        for (i = 0; i < CHARA_BEHAVIOR_MAX; i++) {
            if (b == Character[chara].behaviorBank[i])
                break;
        }
        if (i == CHARA_BEHAVIOR_MAX)
            return XCMD_DONE;
    }
    TrgCharacterBehaviorFree(chara, b);
    if (argc == 3)
        SceneEndBank(b, BankInfo[b].groupCount);
    return XCMD_DONE;
}

// CT3 0x018f2830: frees character argv[0]'s shape (when it has one); with two arguments and argv[1]
// "FREE" its bank group is ended too.
int tnXcmd_ShapeFree(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);
    short b = Character[chara].bank;

    (void)bank;
    if (b < 0)
        return XCMD_DONE;
    TrgCharacterShapeFree(chara);
    if (argc == 2 && strcmp(argv[1], "FREE") == 0)
        SceneEndBank(b, BankInfo[b].groupCount);
    return XCMD_DONE;
}

// CT3 0x018f28f0: plays the bank's tracks of kind argv[4] (ANIM_TYPE_*) on part argv[0] to the frame
// argv[1] (by argv[2]'s source, XcmdValue) at speed argv[3] -- backwards when the frame lies before the
// current one; nothing when the track is there already.
int wideXcmd_PlayAnimeToFream2(int argc, char **argv, int bank)
{
    const XCMD_ANIM_TYPE *type = NULL;
    ALL_TABLE *tbl = All_table_TBL[bank];
    int part, now, to, ok;
    size_t i;

    (void)argc;
    for (i = 0; i < sizeof(XcmdAnimTypes) / sizeof(XcmdAnimTypes[0]); i++) {
        if (strcmp(argv[4], XcmdAnimTypes[i].name) == 0)
            type = &XcmdAnimTypes[i];
    }
    if (type == NULL) // PORT: the ROM goes on with stale registers for an unknown kind
        return XCMD_DONE;
    part = atoi(argv[0]) & 0xffff;
    now = asGetSceneAnimationCurrent(0, tbl, part, type->kind) >> 1;
    to = XcmdValue(argv, 1, bank, &ok) * XCMD_FRAME_TIME;
    if (!ok) // PORT: likewise for an unknown source
        return XCMD_DONE;
    if (to != now) {
        asSceneAnimationStart((float)atoi(argv[3]), 0, tbl, -1, to, part,
                              (type->select & XCMD_ANIM_SEL_POSE) != 0,
                              (type->select & XCMD_ANIM_SEL_OTHER) != 0,
                              (type->select & XCMD_ANIM_SEL_BY_PART) != 0,
                              0, 0, to < now, SCENE_ANIM_SCENE, 0);
    }
    return XCMD_DONE;
}

// ---- slice 3 (2026-11-11): control lock, screen adjust, omni lights, filter, movie buffers, puzzle ----

// CT3 0x018ee5f0: the clear's points (the "Rooders" score), digit by digit into local variables argv[0]
// (units) .. argv[9] (10^9, the quotient, not taken mod 10). Time: 10 points a second under
// XCMD_ROODERS_TIME_LIMIT seconds of play (progress_time / 30, unsigned), never below 0; plus
// XCMD_ROODERS_ITEM_POINTS for each of ArrowData[1], ArrowData[2] and the held counts (byte 1) of item rows
// 1..3 (names from this one use).
int tnXcmd_SetRoodersPts(int argc, char **argv, int bank)
{
    int score, rest, i;
    int div = XCMD_ROODERS_TOP_DIGIT;

    (void)argc;
    score = (XCMD_ROODERS_TIME_LIMIT - (int)((u_int)progress_time / XCMD_ROODERS_FPS)) * 10;
    if (score < 0)
        score = 0;
    score += (ArrowData[1] + ArrowData[2] + ItemData[3][ITEM_ROW_HELD] + ItemData[1][ITEM_ROW_HELD]
              + ItemData[2][ITEM_ROW_HELD]) * XCMD_ROODERS_ITEM_POINTS;
    // The ROM peels the digits from the top (score % 10^9, % 10^8, ...) and stores them from the units up.
    int digit[10];
    rest = score;
    digit[9] = score / div;
    for (i = 8; i >= 0; i--) {
        rest %= div;
        div /= 10;
        digit[i] = rest / div;
    }
    for (i = 0; i < 10; i++)
        ssTrigSetVariable(atoi(argv[i]) & 0xffff, digit[i], 0, bank);
    return XCMD_DONE;
}

// "ON" / "on": the commands' switch argument.
static int XcmdIsOn(const char *arg)
{
    return strcmp("ON", arg) == 0 || strcmp("on", arg) == 0;
}

// The lock of tnXcmd_SetAllControlDisableCamera: the menu, the player's keys and (outside panic event
// 0x257) the stalkers' AI stop (mode 1) or run again (mode 0), each unless its panic "keep" event is set;
// TRIG_EVENT_MC_RELOADED follows the lock unless TRIG_EVENT_PANIC_NO_RELOAD is set; asMothPause too.
static void XcmdControlLock(int mode)
{
    if (!ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MENU, 1, -1))
        ssMenuUpdate(mode);
    if (!ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_PLAYER, 1, -1))
        PlayerControlUpdate(mode);
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_257, 1, 0))
        return;
    if (!ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MONSTERS, 1, -1))
        MonsterAIUpdate(mode);
    if (!ssTrigCheckEvent(TRIG_EVENT_PANIC_NO_RELOAD, 1, -1)) {
        if (mode)
            ssTrigRegisterEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
        else
            ssTrigRemoveEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
    }
    asMothPause(mode);
}

// CT3 0x018ee9c0: argv[0] 0 locks the controls while the camera plays (XcmdControlLock(1)); anything
// else releases them. The ROM writes both branches out in full.
int tnXcmd_SetAllControlDisableCamera(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    XcmdControlLock(atoi(argv[0]) == 0 ? 1 : 0);
    return XCMD_DONE;
}

// CT3 0x018eedf0: the picture's position on the TV (the options' screen adjust). argv[0] "GET" / "get"
// reads ga_disp's DISPLAY into variables argv[1] (x) and argv[2] (y); anything else takes them from those
// variables, clamped to +-XCMD_SCREEN_ADJUST_MAX, puts them into DISPLAY.DX / DY and sends ga_disp, then
// writes the clamped values back. The variables are local. On the host DISPLAY places nothing (the
// renderer owns the picture's geometry); the values are kept for the options screen and the movies.
int wideXcmd_ScreenAdjust(int argc, char **argv, int bank)
{
    int x, y;

    (void)argc;
    if (strcmp("GET", argv[0]) == 0 || strcmp("get", argv[0]) == 0) {
        y = (int)ga_disp.display.DY - AA_DISPLAY_DY_BASE;
        x = ((int)ga_disp.display.DX - AA_DISPLAY_DX_BASE) / AA_DISPLAY_DX_STEP;
    } else {
        x = ssTrigGetVariable(atoi(argv[1]) & 0xffff, 0, bank);
        y = ssTrigGetVariable(atoi(argv[2]) & 0xffff, 0, bank);
        if (x > XCMD_SCREEN_ADJUST_MAX)
            x = XCMD_SCREEN_ADJUST_MAX;
        if (y > XCMD_SCREEN_ADJUST_MAX)
            y = XCMD_SCREEN_ADJUST_MAX;
        if (x < -XCMD_SCREEN_ADJUST_MAX)
            x = -XCMD_SCREEN_ADJUST_MAX;
        if (y < -XCMD_SCREEN_ADJUST_MAX)
            y = -XCMD_SCREEN_ADJUST_MAX;
        ga_disp.display.DX = x * AA_DISPLAY_DX_STEP + AA_DISPLAY_DX_BASE;
        ga_disp.display.DY = y + AA_DISPLAY_DY_BASE;
        sceGsPutDispEnv(&ga_disp);
    }
    ssTrigSetVariable(atoi(argv[1]) & 0xffff, x, 0, bank);
    ssTrigSetVariable(atoi(argv[2]) & 0xffff, y, 0, bank);
    return XCMD_DONE;
}

// CT3 0x018ef550: the point-light override every bank's light set takes (AllOmniEffect, read by
// Character_Init.c's light sets). argv[0] "ON": slot argv[1] (1..XCMD_OMNI_SLOTS) = this bank's part
// argv[2]. Otherwise argv[1] 0 clears every slot, another slot number clears that one.
int tnXcmd_AllOmniEffect(int argc, char **argv, int bank)
{
    int slot, i;

    (void)argc;
    if (XcmdIsOn(argv[0])) {
        slot = atoi(argv[1]) - 1;
        AllOmniEffect[slot * 2] = (u_short)bank;
        AllOmniEffect[slot * 2 + 1] = (u_short)atoi(argv[2]);
    } else if (atoi(argv[1]) == 0) {
        for (i = 0; i < XCMD_OMNI_SLOTS * 2; i++)
            AllOmniEffect[i] = XCMD_OMNI_NONE;
    } else {
        slot = atoi(argv[1]) - 1;
        AllOmniEffect[slot * 2] = XCMD_OMNI_NONE;
        AllOmniEffect[slot * 2 + 1] = XCMD_OMNI_NONE;
    }
    return XCMD_DONE;
}

// CT3 0x018ef670: as tnXcmd_AllOmniEffect for character argv[1]'s light set only (CharOmniEffect):
// "ON": slot argv[2] = this bank's part argv[3]; otherwise slot argv[2] 0 clears the four, another
// clears that one.
int tnXcmd_CharOmniEffect(int argc, char **argv, int bank)
{
    int chara, slot, i;

    (void)argc;
    chara = atoi(argv[1]);
    if (XcmdIsOn(argv[0])) {
        slot = atoi(argv[2]) - 1;
        CharOmniEffect[chara][slot * 2] = (u_short)bank;
        CharOmniEffect[chara][slot * 2 + 1] = (u_short)atoi(argv[3]);
    } else if (atoi(argv[2]) == 0) {
        for (i = 0; i < XCMD_OMNI_SLOTS; i++) {
            CharOmniEffect[chara][i * 2] = XCMD_OMNI_NONE;
            CharOmniEffect[chara][i * 2 + 1] = XCMD_OMNI_NONE;
        }
    } else {
        slot = atoi(argv[2]) - 1;
        CharOmniEffect[chara][slot * 2] = XCMD_OMNI_NONE;
        CharOmniEffect[chara][slot * 2 + 1] = XCMD_OMNI_NONE;
    }
    return XCMD_DONE;
}

// The falloff of tnXcmd_OmniRange: lightPos w = 1 / the value argv[4] by argv[5]'s source (XcmdValue);
// another source leaves it.
static void XcmdOmniFalloff(MODEL_PARTS *part, char **argv, int bank)
{
    int ok, value = XcmdValue(argv, 4, bank, &ok);

    if (ok)
        part->lightPos[3] = 1.0f / (float)value;
}

// CT3 0x018ef810: point light part argv[0] of the bank. With three arguments the range ease stops and
// only the falloff is set; otherwise the range eases (OmniRangeChange) to argv[1] by argv[2]'s source over
// argv[3] steps, then the falloff. PORT: with three arguments the ROM still reads argv[4] and argv[5]
// (stale slots of the dispatcher's array); the host has no such slots and leaves the falloff.
int tnXcmd_OmniRange(int argc, char **argv, int bank)
{
    MODEL_PARTS *part = XcmdPart(bank, atoi(argv[0]));
    int ok, value;

    if (argc == 3) {
        part->omniChanging = 0;
        part->omniSteps = 0;
        return XCMD_DONE;
    }
    part->omniChanging = 1;
    part->omniSteps = atoi(argv[3]);
    value = XcmdValue(argv, 1, bank, &ok);
    if (ok)
        part->omniRange = (float)value;
    XcmdOmniFalloff(part, argv, bank);
    return XCMD_DONE;
}

// CT3 0x018f1710: the screen filter (FilterChangeWork, drawn by QuatSlerp.c's ScreenFilter). argv[0]
// "ON": waits while a fade runs, then turns it on: argv[1] "type1" FILTER_MODE_ADD / "type2"
// FILTER_MODE_BLEND (else the mode is kept), argv[2] "R,G,B,A" the target; with five arguments it fades
// over argv[3] frames from argv[4] "R,G,B,A" (else at once), with six argv[5] "AUTO_OFF" turns it off at
// the end. Anything else: waits while a fade runs, then off.
int tnXcmd_Filter_Change(int argc, char **argv, int bank)
{
    FILTER_CHANGE_WORK *w = &FilterChangeWork;

    (void)bank;
    if (w->timer != 0)
        return XCMD_WAIT;
    if (!XcmdIsOn(argv[0])) {
        w->active = 0;
        w->timer = 0;
        return XCMD_DONE;
    }
    w->active = FILTER_ON;
    if (strcmp("type1", argv[1]) == 0)
        w->mode = FILTER_MODE_ADD;
    if (strcmp("type2", argv[1]) == 0)
        w->mode = FILTER_MODE_BLEND;
    sscanf(argv[2], "%d,%d,%d,%d", &w->target[1], &w->target[2], &w->target[3], &w->target[0]);
    if (argc < 5) {
        w->timer = 0;
    } else {
        w->timer = atoi(argv[3]);
        sscanf(argv[4], "%d,%d,%d,%d", &w->current[1], &w->current[2], &w->current[3], &w->current[0]);
    }
    if (argc >= 6 && strcmp("AUTO_OFF", argv[5]) == 0)
        w->active |= FILTER_OFF_AT_END;
    return XCMD_DONE;
}

// CT3 0x018f2430: the movie's banks: AA_MovieUseBank[i].bank = argv[i] for the six blocks. When any of
// them overlaps a drawn bank (BankUseCheck) the ROM reports "%s Bank:%d-%d" (the block's name, its first
// and last bank) through WideErrorMsgDebug (line 0x6ee, empty in retail) and unsets all six.
int tnXcmd_SetMovieWorkBuff(int argc, char **argv, int bank)
{
    int i, first, clash = 0;

    (void)argc;
    (void)bank;
    for (i = 0; i < AA_MOVIE_WORKS; i++) {
        first = atoi(argv[i]);
        if (BankUseCheck(first, AA_MovieUseBank[i].count)) {
            WideErrorMsgDebug();
            clash = 1;
        }
        AA_MovieUseBank[i].bank = first;
    }
    if (clash) {
        for (i = 0; i < AA_MOVIE_WORKS; i++)
            AA_MovieUseBank[i].bank = -1;
    }
    return XCMD_DONE;
}

// CT3 0x018f2fa0: puzzle 1P01's start: the scene animations argv[0] and argv[1] of the bank jump to the
// frame of the saved state, (row * XCMD_PUZZLE_ROW_FRAMES + col * XCMD_PUZZLE_COL_FRAMES / 100) script
// frames from global variables 0x12 and 0x21.
int tnXcmd_1P01_PazzleInit(int argc, char **argv, int bank)
{
    int col = ssTrigGetVariable(TRIG_VAR_PUZZLE_COL, 1, 0);
    int row = ssTrigGetVariable(TRIG_VAR_PUZZLE_ROW, 1, 0);
    int to = (row * XCMD_PUZZLE_ROW_FRAMES + col * XCMD_PUZZLE_COL_FRAMES / 100) * XCMD_FRAME_TIME;

    (void)argc;
    asSceneAnimationStart(1.0f, 0, All_table_TBL[bank], -1, to, atoi(argv[0]) & 0xffff, 1, 0, 0, 0, 1, 0,
                          SCENE_ANIM_SCENE, 0);
    asSceneAnimationStart(1.0f, 0, All_table_TBL[bank], -1, to, atoi(argv[1]) & 0xffff, 1, 0, 0, 0, 1, 0,
                          SCENE_ANIM_SCENE, 0);
    return XCMD_DONE;
}

// CT3 0x018f3ba0: the current scene, and the camera's, = argv[0].
int tnCurrentSceneSet(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    idCurrentScene = atoi(argv[0]);
    idCameraScene = idCurrentScene;
    return XCMD_DONE;
}

// CT3 0x018f3bd0: the bank's texture file goes to page argv[0] (argv[1]: back).
int tnXcmd_TexFileSetNext(int argc, char **argv, int bank)
{
    (void)argc;
    TexFileSetNext(bank, atoi(argv[0]), atoi(argv[1]));
    return XCMD_DONE;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018f2c50
int tnXcmd_1P01_PazzleAnim(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tnXcmd_1P01_PazzleAnim", 0x018f2c50);
    return {};
}

// stub of CT3 0x018f36b0
int tnXcmd_Character_Ambient_Change(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tnXcmd_Character_Ambient_Change", 0x018f36b0);
    return {};
}

// stub of CT3 0x018efc50
int tnXcmd_Key_Lock(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tnXcmd_Key_Lock", 0x018efc50);
    return {};
}

// stub of CT3 0x018f3c30
int tnXcmd_TexFileSetPage(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tnXcmd_TexFileSetPage", 0x018f3c30);
    return {};
}

// stub of CT3 0x018f1a10
int tnXcmd_TimeSet(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tnXcmd_TimeSet", 0x018f1a10);
    return {};
}

// stub of CT3 0x018f37e0
int tnXcmd_VistaVision(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tnXcmd_VistaVision", 0x018f37e0);
    return {};
}

// stub of CT3 0x018f3280
int wideXcmd_PlayAnimeRange(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_PlayAnimeRange", 0x018f3280);
    return {};
}

// stub of CT3 0x018f30e0
int wideXcmd_PlayAnimeToFream(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_PlayAnimeToFream", 0x018f30e0);
    return {};
}

// stub of CT3 0x018f34a0
int wideXcmd_VariableCal(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_VariableCal", 0x018f34a0);
    return {};
}
