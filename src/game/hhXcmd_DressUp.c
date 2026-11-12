// game/hhXcmd_DressUp.c -- Clock Tower 3: the trigger script's sound x commands by hh (0x018dc330..
// 0x018dd550): the costume swap, the SQ (MIDI) music, the special and ordinary SEs, the ADS stream, the
// reverb, and FootSEConversion, the footsteps by floor material. Each command takes the command's
// argument strings (argv[0..argc-1]) and the bank whose script runs it; ssTrigExecuteActCmd
// (ssGrpPortalCheck.c) calls them through ssXcmd_functions. Written 2026-11-11: all 22 functions.
//
// PORT: the floor a character stands on is CHARACTER.w300[0] (CHARA_TARGET, an EE word no written TU
// sets yet: open-items.md 1 (v)); FootGround answers none, so a footstep answers -1 as the ROM does for a
// character with no floor, until the word's writer decides its host form.
//
// callcheck lists calls as missing from StopMIDI / PlayMIDI, PlaySESp1 / PlaySE, the three ...ChangeSE
// and FootSEConversion: the static helpers (IsAnyMidi, WaitPlaySEReady, ChangeSEParam, PlayFootSE) make
// them. Explained, not a gap.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "game/hhXcmd_DressUp.h"
#include "game/xcmd_tn.h"
#include "game/sound.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/ssGrpPortalCheck.h"
#include "game/fileio.h"
#include "game/ArtoolDebugOpen.h"
#include "game/BRANCH_CLEAR.h"
#include "game/main.h"
#include "game/unassigned_data.h"
#include "sdk/libgraph.h"
#include "sdk/newlib.h"

// Statics (the symtab's locals, .bss).
static int sStopVol;                            // CT3 0x019447cc: gSQLd[0].vol when StopMIDI ran
static char sStopMidi[0x10];                    // CT3 0x019447d0: the SQ StopMIDI stopped ("" = none)
static int sMonsterFootLeftRight;               // CT3 0x019447e0 (FootSEConversion's $109)
static int sAlysFootLeftRight;                  // CT3 0x019447e4 (FootSEConversion's $108)

// CT3 0x018dc330: the costume: Round2ChangeChar with the script variable argv[0] (global) -- the
// variable holds a ROUND2_* value (fileio.c: 2..5 the costumes; open-items.md 1 (eo)).
int hhXcmd_DressUp(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    Round2ChangeChar(ssTrigGetVariable(atoi(argv[0]), 1, 0));
    return XCMD_DONE;
}

// CT3 0x018dc370: a free slot of the table.
int hhXcmd___________Free5(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018dc380: a free slot of the table.
int hhXcmd___________Free4(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018dc390: a free slot of the table.
int hhXcmd___________Free3(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018dc3a0: SQ argv[0]'s volume argv[1], over argv[2] (only with exactly three arguments; else 0).
int hhXcmd_VolumeMIDI(int argc, char **argv, int bank)
{
    int fade;

    (void)bank;
    fade = argc == 3 ? atoi(argv[2]) : 0;
    SSVolumeSQ(argv[0], atoi(argv[1]), fade);
    return XCMD_DONE;
}

// CT3 0x018dc410: forget the stopped SQ (SSInitClearValiable).
void hhXcmd_InitPlayStopMIDI(void)
{
    sStopMidi[0] = '\0';
    sStopVol = 0;
}

// "ANY" / "any" as a command's SQ name (@354 / @355): every SQ.
static int IsAnyMidi(const char *name)
{
    return (name[0] == 'A' && memcmp(name, "ANY", 3) == 0)
        || (name[0] == 'a' && memcmp(name, "any", 3) == 0);
}

// CT3 0x018dc420: stop SQ argv[0] ("ANY": all of them, unless the chase BGM plays) over argv[1] frames
// (only with exactly two arguments; else 0); the playing SQ and its volume are kept for PlayMIDI "ANY".
int hhXcmd_StopMIDI(int argc, char **argv, int bank)
{
    int fade;

    (void)bank;
    fade = argc == 2 ? atoi(argv[1]) : 0;
    // The ROM strcpys the 0x38-byte name into 16 bytes: SQ names are short ("HINMIN").
    strncpy(sStopMidi, gSQLd[0].name, sizeof sStopMidi - 1);
    sStopMidi[sizeof sStopMidi - 1] = '\0';
    sStopVol = gSQLd[0].vol;
    if (IsAnyMidi(argv[0])) {
        if (gPlayingChaseBGM == 0) {
            SdStopSQAll(fade);
        }
    } else {
        SSStopSQ(argv[0], fade);
    }
    return XCMD_DONE;
}

#define MIDI_VOL_MAX 0x7f

// CT3 0x018dc500: play SQ argv[0] at volume argv[1] (0..0x7f, default 0x7f); "ANY" plays again what
// StopMIDI stopped, at its old volume.
int hhXcmd_PlayMIDI(int argc, char **argv, int bank)
{
    int vol;

    (void)bank;
    if ((u_int)argc < 2) {
        vol = MIDI_VOL_MAX;
    } else {
        vol = atoi(argv[1]);
        if (vol < 0) {
            vol = 0;
        } else if (vol > MIDI_VOL_MAX) {
            vol = MIDI_VOL_MAX;
        }
    }
    if (sStopMidi[0] != '\0' && IsAnyMidi(argv[0])) {
        SSPlaySQ(sStopMidi, sStopVol);
    } else {
        SSPlaySQ(argv[0], vol);
    }
    return XCMD_DONE;
}

// CT3 0x018dc5e0: stop special SE argv[1] of bank argv[0] (argv[2] passed on).
int hhXcmd_StopSESp1(int argc, char **argv, int bank)
{
    char name[32];
    int no;

    (void)argc;
    (void)bank;
    strcpy(name, argv[0]);
    strupr(name);
    no = atoi(argv[1]);
    SSStopSESp1(name, no, atoi(argv[2]));
    return XCMD_DONE;
}

#define SE_REQ_WAIT_LOAD 10000          // reqArg2 >= this: wait until the SE's bank is in before playing
#define SE_REQ_RESIDENT 1000            // reqArg2 >= this: a resident SE (hhXcmd_PlaySE names it)

// A positional volume argument in tenths: the ROM reads chars 0 and 2 ("1.0" = 10, "0.5" = 5).
static int TenthsArg(const char *s)
{
    return (s[0] - '0') * 10 + (s[2] - '0');
}

// Wait for name's bank, stepping the file and sound servers each vsync (the ROM's loop).
static void WaitPlaySEReady(const char *name)
{
    while (IsPlaySEReady(name) < 0) {
        FlProc();
        SdProc();
        sceGsSyncV(0);
    }
}

// CT3 0x018dc640: play special SE argv[1] of bank argv[0]: argv[2] reqArg2 (>= SE_REQ_WAIT_LOAD waits for
// the bank), argv[3] / argv[4] / argv[5] passed on, argv[6] the positional volume in tenths, argv[7]
// passed on (default 10, from the disassembly: the decompiler dropped this stack argument); skipped while
// a demo is skipped or event 0x96 holds.
int hhXcmd_PlaySESp1(int argc, char **argv, int bank)
{
    char name[32];
    int a5 = 0;
    int posVol = 0;
    int a8 = 10;
    int reqArg2;
    int no;
    int a3;
    int a4;

    if (DemoSkip != 0 || ssTrigCheckEvent(TRIG_EVENT_96, 1, 0) != 0) {
        return XCMD_DONE;
    }
    strcpy(name, argv[0]);
    strupr(name);
    reqArg2 = atoi(argv[2]);
    if (reqArg2 >= SE_REQ_WAIT_LOAD) {
        WaitPlaySEReady(name);
    }
    if ((u_int)argc > 5) {
        a5 = atoi(argv[5]);
        if ((u_int)argc > 6) {
            posVol = TenthsArg(argv[6]);
            if ((u_int)argc > 7) {
                a8 = atoi(argv[7]);
            }
        }
    }
    no = atoi(argv[1]);
    a3 = atoi(argv[3]);
    a4 = atoi(argv[4]);
    SSPlaySESp1(name, no, reqArg2, a3, a4, a5, bank, posVol, a8);
    return XCMD_DONE;
}

// CT3 0x018dc7e0: stop the ADS stream (SSStopADS's a1 = 1). PORT: the ROM passes argv[0] as a0, which
// SSStopADS drops; the host passes 0.
int hhXcmd_StopADS(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    SSStopADS(0, 1);
    return XCMD_DONE;
}

// CT3 0x018dc810: the ADS stream's volume argv[1]. PORT: the ROM also passes argv[0] (a0) and argv[2]
// (a2, with three or more arguments, else 0); SSVolumeADS tail-jumps to SdVolumeStream(vol), which reads
// neither -- the host passes only the volume.
int hhXcmd_VolumeADS(int argc, char **argv, int bank)
{
    int vol;

    (void)bank;
    vol = atoi(argv[1]);
    if ((u_int)argc >= 3) {
        (void)atoi(argv[2]);
    }
    SSVolumeADS(0, vol);
    return XCMD_DONE;
}

#define ADS_VOL_DEFAULT 0xff

// CT3 0x018dc890: play ADS stream argv[0] at volume argv[1] (default 0xff), argv[2] passed on (default 1).
int hhXcmd_PlayADS(int argc, char **argv, int bank)
{
    char name[32];
    int vol = ADS_VOL_DEFAULT;
    int a2 = 1;

    (void)bank;
    strcpy(name, argv[0]);
    strupr(name);
    if ((u_int)argc > 1) {
        vol = atoi(argv[1]);
    }
    if ((u_int)argc > 2) {
        a2 = atoi(argv[2]);
    }
    SSPlayADS(name, vol, a2);
    return XCMD_DONE;
}

#define EFFECT_DEPTH_MAX 0x7f

// CT3 0x018dc930: the reverb: type argv[0], depth argv[1] (default 0x7f, at most 0x7f), core argv[2]
// (default 1).
int hhXcmd_EffectSE(int argc, char **argv, int bank)
{
    int depth = EFFECT_DEPTH_MAX;
    int core = 1;

    (void)bank;
    if (argc == 3) {
        depth = atoi(argv[1]);
        core = atoi(argv[2]);
    } else if (argc == 2) {
        depth = atoi(argv[1]);
    }
    if (depth > EFFECT_DEPTH_MAX) {
        depth = EFFECT_DEPTH_MAX;
    }
    SdSetEffect(core, atoi(argv[0]), depth);
    return XCMD_DONE;
}

// CT3 0x018dc9e0: stop SE argv[1] of bank argv[0] (argv[2] passed on).
int hhXcmd_StopSE(int argc, char **argv, int bank)
{
    char name[32];
    int no;

    (void)argc;
    (void)bank;
    strcpy(name, argv[0]);
    strupr(name);
    no = atoi(argv[1]);
    SSStopSE(name, no, atoi(argv[2]));
    return XCMD_DONE;
}

// The three ...ChangeSE commands: set one parameter (mask) of SE argv[1] / reqArg2 argv[2] in loaded bank
// argv[0] to argv[3]; nothing when the bank is not loaded.
static void ChangeSEParam(char **argv, u_int mask)
{
    char name[32];
    int row;
    int no;
    int reqArg2;

    strcpy(name, argv[0]);
    strupr(name);
    row = SdFindLoadSEInfo(SD_FIND_LOAD_SE_NAME, name);
    if (row >= 0) {
        no = atoi(argv[1]);
        reqArg2 = atoi(argv[2]);
        SdSetParamSESDT(gSELd[row].jls, no, reqArg2, mask, atoi(argv[3]));
    }
}

// CT3 0x018dca40: an SE's pan (ChangeSEParam).
int hhXcmd_PanChangeSE(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    ChangeSEParam(argv, SD_SE_PARAM_PAN);
    return XCMD_DONE;
}

// CT3 0x018dcaf0: an SE's pitch (ChangeSEParam with SD_SE_PARAM_30: unk30 is the pitch).
int hhXcmd_PitchChangeSE(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    ChangeSEParam(argv, SD_SE_PARAM_30);
    return XCMD_DONE;
}

// CT3 0x018dcba0: an SE's volume (ChangeSEParam).
int hhXcmd_VolumeChangeSE(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    ChangeSEParam(argv, SD_SE_PARAM_VOLUME);
    return XCMD_DONE;
}

// FootSEConversion: the floor object's material byte, and the materials the tables cover.
#define FOOT_GROUND_MATERIAL 0x19b      /* CHARA_TARGET object: u_char, the floor's material 0..11 */
#define FOOT_MATERIAL_COUNT 12
#define FOOT_MATERIAL_ALYSSA_BANK 9     /* materials 9..11: a stalker's step plays Alyssa's bank's SE */
// The footstep banks: Alyssa's, and the stalkers' with their Character[] slot (the HIT_CHARA_* slots).
#define FOOT_BANK_ALYSSA "C_0000"
#define FOOT_CHARA_HA 2                 /* "E_HA" (the hammer) */
#define FOOT_CHARA_RY 3                 /* "E_RY" */
#define FOOT_CHARA_ON 4                 /* "E_ON" (the axe) */
#define FOOT_CHARA_SM 5                 /* "E_SM" */
#define FOOT_CHARA_SW 6                 /* "E_SW" */
#define FOOT_CHARA_BA 0x20              /* "E_BA": one step SE pair on every material */
#define FOOT_SE_BA_LEFT 8
#define FOOT_SE_BA_RIGHT 9
#define FOOT_SE_TRIG_EVENT 0xd2         /* Alyssa stepped: the scripts' footstep event */
#define FOOT_JITTER_ALYSSA 3            /* the pitch jitter: (rnd & 1) - (rnd & 3) */
#define FOOT_JITTER_STALKER 2           /* (rnd & 1) - (rnd & 2) */

// Each material's SE for the step whose left/right flag is [set, clear].
static const u_char sAlysFootSE[FOOT_MATERIAL_COUNT][2] = {
    {0x14, 0x30}, {0x13, 0x2f}, {0x19, 0x35}, {0x15, 0x31}, {0x17, 0x33}, {0x1a, 0x36},
    {0x16, 0x32}, {0x16, 0x32}, {0x13, 0x2f}, {0x18, 0x34}, {0x39, 0x3a}, {0x41, 0x42},
};
static const u_char sMonsterFootSE[FOOT_MATERIAL_COUNT][2] = {
    {0x0d, 0x0e}, {0x0b, 0x0c}, {0x15, 0x16}, {0x0f, 0x10}, {0x13, 0x14}, {0x17, 0x18},
    {0x11, 0x12}, {0x11, 0x12}, {0x0b, 0x0c}, {0x18, 0x34}, {0x39, 0x3a}, {0x41, 0x42},
};

// The floor object Character[chara] stands on (CHARACTER.w300[0]); NULL for none. PORT: see the file
// comment -- the EE word has no host form yet.
static const u_char *FootGround(int chara)
{
    (void)chara;
    return NULL;
}

// The stalker whose bank is name (its first four letters: @590..@595), -1 for none.
static int FootChara(const char *name)
{
    static const struct {
        const char *bank;
        int chara;
    } kStalkers[] = {
        {"E_HA", FOOT_CHARA_HA}, {"E_RY", FOOT_CHARA_RY}, {"E_ON", FOOT_CHARA_ON},
        {"E_SM", FOOT_CHARA_SM}, {"E_SW", FOOT_CHARA_SW}, {"E_BA", FOOT_CHARA_BA},
    };
    for (size_t i = 0; i < sizeof kStalkers / sizeof kStalkers[0]; i++) {
        if (memcmp(name, kStalkers[i].bank, 4) == 0) {
            return kStalkers[i].chara;
        }
    }
    return -1;
}

// Play footstep se of bank name with the pitch jitter, objId / bank / posVol as hhXcmd_PlaySE parsed
// them (posVol 0: none), panned by pan every frame.
static void PlayFootSE(const char *name, int se, int reqArg2, int jitter, int objId, int bank, int posVol,
                       SD_SE_UPDATE pan)
{
    int handle = SSPlaySE(name, se, reqArg2);

    if (posVol == 0) {
        SdSetParamSE(handle, SD_SE_PARAM_30 | SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK | SD_SE_PARAM_CALLBACK,
                     jitter, objId, bank, pan);
    } else {
        SdSetParamSE(handle,
                     SD_SE_PARAM_30 | SD_SE_PARAM_POS_VOL | SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK
                         | SD_SE_PARAM_CALLBACK,
                     jitter, posVol, objId, bank, pan);
    }
    pan(handle);
}

// CT3 0x018dcc50: a footstep (hhXcmd_PlaySE's FOOT_SE_NO): the SE for the floor under the stepping
// character, alternating left and right, with a little pitch jitter. Alyssa's (bank "C_0000") pans from
// the camera and raises FOOT_SE_TRIG_EVENT; a stalker's pans from Alyssa. Answers the SE number; -1 with
// no floor, an unknown bank, or Alyssa on an unknown material; 0 for a stalker on an unknown material.
int FootSEConversion(const char *name, int reqArg2, int objId, int bank, int posVol)
{
    const u_char *ground;
    int chara;
    int se;
    int jitter;
    u_int mat;

    if (memcmp(name, FOOT_BANK_ALYSSA, 6) == 0) {
        ground = FootGround(Player_BankID);
        if (ground == NULL) {
            return -1;
        }
        mat = ground[FOOT_GROUND_MATERIAL];
        if (mat >= FOOT_MATERIAL_COUNT) {
            return -1;
        }
        se = sAlysFootSE[mat][sAlysFootLeftRight == 0];
        sAlysFootLeftRight ^= 1;
        jitter = (int)(rnd() & 1);
        jitter -= (int)(rnd() & FOOT_JITTER_ALYSSA);
        PlayFootSE(name, se, reqArg2, jitter, objId, bank, posVol, SetPositionalParam_Cam);
        ssTrigRegisterEvent(FOOT_SE_TRIG_EVENT, 1, -1);
        return se;
    }

    chara = FootChara(name);
    if (chara == -1) {
        return -1;
    }
    // The ROM's asFindCharacterStruct answers its argument: the slot is the stalker's id.
    ground = FootGround((int)(intptr_t)asFindCharacterStruct((void *)(intptr_t)chara));
    if (ground == NULL) {
        return -1;
    }
    mat = ground[FOOT_GROUND_MATERIAL];
    if (mat >= FOOT_MATERIAL_COUNT) {
        return 0;
    }
    if (chara == FOOT_CHARA_BA) {
        se = sMonsterFootLeftRight != 0 ? FOOT_SE_BA_LEFT : FOOT_SE_BA_RIGHT;
    } else {
        se = sMonsterFootSE[mat][sMonsterFootLeftRight == 0];
        if (mat >= FOOT_MATERIAL_ALYSSA_BANK) {
            name = FOOT_BANK_ALYSSA;
        }
    }
    sMonsterFootLeftRight ^= 1;
    jitter = (int)(rnd() & 1);
    jitter -= (int)(rnd() & FOOT_JITTER_STALKER);
    PlayFootSE(name, se, reqArg2, jitter, objId, bank, posVol, SetPositionalParam_Alys);
    return se;
}

#define FOOT_SE_NO 0x1482a              /* 84010: hhXcmd_PlaySE's "a footstep" SE number */
#define SE_PARAM_8000 0x8000u           /* set beside SD_SE_PARAM_NAME for a resident SE; SdSetParamSE reads
                                           no argument for it */
#define SE_POS_VOL_FULL 10              /* posVol "1.0": sent without SD_SE_PARAM_POS_VOL */
#define TRIG_BANK_GLOBAL_LAST 5         /* banks 0..5 (the global banks) play through event 0x96 */

// CT3 0x018dd200: play SE argv[1] of bank argv[0]: argv[2] reqArg2 (default 1; >= SE_REQ_WAIT_LOAD waits
// for the bank, >= SE_REQ_RESIDENT names it), argv[3] the sound object, argv[4] the positional volume in
// tenths, argv[5] 'C' pans from the camera (else from Alyssa); FOOT_SE_NO plays a footstep
// (FootSEConversion). Skipped while a demo is skipped, or while event 0x96 holds for a scene bank's script.
int hhXcmd_PlaySE(int argc, char **argv, int bank)
{
    char name[32];
    int objId = 0;
    int posVol = 0;
    SD_SE_UPDATE pan = SetPositionalParam_Alys;
    int no;
    int reqArg2;
    int handle;

    if (DemoSkip != 0) {
        return XCMD_DONE;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_96, 1, 0) != 0 && bank > TRIG_BANK_GLOBAL_LAST) {
        return XCMD_DONE;
    }
    strcpy(name, argv[0]);
    strupr(name);
    no = atoi(argv[1]);
    reqArg2 = 1;
    if ((u_int)argc > 2) {
        reqArg2 = atoi(argv[2]);
        if (reqArg2 >= SE_REQ_WAIT_LOAD && no != FOOT_SE_NO) {
            WaitPlaySEReady(name);
        }
        if ((u_int)argc > 3) {
            objId = atoi(argv[3]);
            if ((u_int)argc > 4) {
                posVol = TenthsArg(argv[4]);
                if ((u_int)argc > 5 && (argv[5][0] == 'C' || argv[5][0] == 'c')) {
                    pan = SetPositionalParam_Cam;
                }
            }
        }
    }
    if (no == FOOT_SE_NO) {
        FootSEConversion(name, reqArg2, objId, bank, posVol);
        return XCMD_DONE;
    }
    handle = SSPlaySE(name, no, reqArg2);
    if (reqArg2 >= SE_REQ_RESIDENT) {
        SdSetParamSE(handle, SD_SE_PARAM_NAME | SE_PARAM_8000, name);
    }
    if ((u_int)argc > 3) {
        if ((u_int)argc < 5 || posVol == SE_POS_VOL_FULL) {
            SdSetParamSE(handle, SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK | SD_SE_PARAM_CALLBACK, objId, bank, pan);
        } else {
            SdSetParamSE(handle, SD_SE_PARAM_POS_VOL | SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK | SD_SE_PARAM_CALLBACK,
                         posVol, objId, bank, pan);
        }
        pan(handle);
    }
    return XCMD_DONE;
}

// CT3 0x018dd490: purge SE bank argv[0].
int hhXcmd_PurgeSE(int argc, char **argv, int bank)
{
    char name[32];

    (void)argc;
    (void)bank;
    strcpy(name, argv[0]);
    strupr(name);
    SSPurgeSE(name);
    return XCMD_DONE;
}

// CT3 0x018dd4d0: load SE bank argv[0] (argv[1] passed on, default 0). The command answers at once; the
// load runs in SdLoadSEProc.
int hhXcmd_LoadSE(int argc, char **argv, int bank)
{
    char name[32];
    int a1 = 0;

    (void)bank;
    strcpy(name, argv[0]);
    strupr(name);
    if ((u_int)argc > 1) {
        a1 = atoi(argv[1]);
    }
    SSLoadSE(name, a1);
    return XCMD_DONE;
}
