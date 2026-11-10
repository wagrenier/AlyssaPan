// game/sound.c -- Clock Tower 3: the game's sound layer over the Sd driver (SE banks by file name, SQ
// music by name, the stage BGM state across movies, the option menu's volumes). CT3 0x018c7fb0..0x018cbf90.
// Slice 1 reconstructed 2026-11-09 (38 functions): the boot / end path, the SE and SQ entry points, the
// stages' SQ rules (Branch1..5 SSPlaySQ / SSStopSQ), the movie and demo mute, the angle helpers. Still
// stubs: the BGM state machines (Branch1..5BGMProc), SSGetPositional / SetPositionalParam_* (they read
// the Sd driver's play records), the trigger actions (SSAct*, SSFindSoundObj, SSAutoStartSoundRoomIn),
// SSLoadSEfromLoadPoint, SSSave/ReturnVolumeMidi (gSQLd) and the 12 functions past the slice.
//
// The source string "sound.c" is fs_error's file argument in SSLoadSE (line 674). Every Sd* call goes to
// game/SdMasterVolumeSetSE.c, the EE side of the IOP sound driver.

#include "game/sound.h"

#include "common/alyssapan_stub.h"
#include "game/QuatSlerp.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/fileio.h"
#include "game/hhXcmd_DressUp.h"
#include "game/main.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "sdk/libgraph.h"
#include "sdk/newlib.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

// ---- data ---------------------------------------------------------------------------------------------

// CT3 0x01939ea0 gFileSE (.data, 0x54 bytes): the SE bank files by name prefix.
const SE_FILE gFileSE[7] = {
    {"C_", 4, 0x0e8000},
    {"E_", 6, 0x182630},
    {"Z_", 7, 0x1b96f0},
    {"R_", 5, 0x1562b0},
    {"T_", 8, 0x1d40c0},
    {"B_", 5, 0x1562b0},
    {nullptr, 0, 0},
};

// CT3 0x01939f00 gFileSQ (.data, 0x118 bytes): the SQ music files.
const SQ_FILE gFileSQ[14] = {
    {"PIANO", 0x20000, 5, 0x7f, 0x64},
    {"TUISEKI", 0x20000, 4, 0x3c, 0x64},
    {"TUISEKI2", 0x20000, 4, 0x37, 0x64},
    {"CHITEI", 0x20000, 4, 0x46, 0x64},
    {"FUAN", 0x20000, 4, 0x46, 0x28},
    {"HAKABA", 0x20000, 4, 0x2d, 0x64},
    {"HINMIN", 0x20000, 4, 0x46, 0x64},
    {"KOJO", 0x20000, 4, 0x32, 0x64},
    {"KYOHU", 0x20000, 4, 0x32, 0x64},
    {"OMOIDE", 0x20000, 4, 0x3c, 0x64},
    {"TSUNORU", 0x20000, 4, 0x3c, 0x64},
    {"MISSHORT", 0x20000, 5, 0x7f, 0x64},
    {"MISLONG", 0x20000, 5, 0x7f, 0x64},
    {nullptr, 0, 0, 0, 0},
};

// sound.c's .sbss variables from 0x01942d48 (the symtab's names and sizes; 0x01942dd0 unread).
int gDemoStartFlg;                  /* sbss 0x01942d48 */
int gBgmControlSaveVol;             /* sbss 0x01942d4c */
char gBgmControlSaveFile[0x38];     /* sbss 0x01942d50 */
int gBgmControl;                    /* sbss 0x01942d88 */
int gPlayMidiDemoVol;               /* sbss 0x01942d8c */
char gPlayMidiDemo[0x38];           /* sbss 0x01942d90 */
int gBossAlysDead;                  /* sbss 0x01942dc8 */
int gPlayTbreak;                    /* sbss 0x01942dcc */
int gPlayingChaseBGM;               /* sbss 0x01942dd4 */
int gPlayingStageBGM;               /* sbss 0x01942dd8 */
u_char gSavePlaySESp1[0x104];       /* sbss 0x01942de0 */

// .rodata strings (the ROM's literals, by address).
static const char s_soundDir[] = "DATA\\SOUND\\";          // 0x019365e0
static const char s_sqDir[] = "DATA\\SOUND\\SQ\\";         // 0x019365d0
static const char s_srcFile[] = "sound.c";                 // 0x019365f0
static const char s_seLoad[] = "SE LOAD";                  // 0x019365f8
static const char s_fileNotFound[] = "FILE NOT FOUND %s\n"; // 0x01936600
static const char s_sqNotFound[] = "SSPlaySQ2(%s) Not Found!!\n"; // 0x019365b0 (_1872)

#define SSLOADSE_ERROR_LINE 0x2a2   // sound.c line 674, fs_error's line argument
#define SCENE_CODE_BOOT_BFO 0x78787842 // "Bxxx"
#define SCENE_CODE_BOOT_NOW 0x7878784e // "Nxxx"

// ---- the movie mute (AA_StartMovie) --------------------------------------------------------------------

// CT3 0x018c8af0: sound back on after a movie: the special SEs as they were, and the stage BGM of the
// stage the trigger table names, when one was playing. Stage 5 plays OMOIDE only; stage 4 one of four.
void SSSoundEnable(void)
{
    SdSoundEnable();
    memcpy(gPlaySESp1, gSavePlaySESp1, sizeof gSavePlaySESp1);
    if (gPlayingStageBGM == STAGE_BGM_NONE) {
        return;
    }
    switch (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0)) {
    case 5:
        if (gPlayingStageBGM == STAGE_BGM_OMOIDE) {
            SSPlaySQ2("OMOIDE", SQ_VOLUME_MAX);
        }
        break;
    case 4:
        if (gPlayingStageBGM == STAGE_BGM_OMOIDE) {
            SSPlaySQ2("OMOIDE", SQ_VOLUME_MAX);
        } else if (gPlayingStageBGM == STAGE_BGM_FUAN) {
            SSPlaySQ2("FUAN", SQ_VOLUME_MAX);
        } else if (gPlayingStageBGM == STAGE_BGM_KOJO) {
            SSPlaySQ2("KOJO", SQ_VOLUME_MAX);
        } else if (gPlayingStageBGM == STAGE_BGM_TSUNORU) {
            SSPlaySQ2("TSUNORU", SQ_VOLUME_MAX);
        }
        break;
    case 3:
        if (gPlayingStageBGM == STAGE_BGM_HAKABA) {
            SSPlaySQ2("HAKABA", SQ_VOLUME_MAX);
        } else if (gPlayingStageBGM == STAGE_BGM_CHITEI) {
            SSPlaySQ2("CHITEI", SQ_VOLUME_MAX);
        }
        break;
    case 2:
        if (gPlayingStageBGM == STAGE_BGM_KYOHU) {
            SSPlaySQ2("KYOHU", SQ_VOLUME_MAX);
        } else if (gPlayingStageBGM == STAGE_BGM_HINMIN) {
            SSPlaySQ2("HINMIN", SQ_VOLUME_MAX);
        }
        break;
    default:
        break;
    }
}

// CT3 0x018c8cc0: sound off for a movie: keep the special SEs' state, forget the chase BGM.
void SSSoundDisable(void)
{
    memcpy(gSavePlaySESp1, gPlaySESp1, sizeof gSavePlaySESp1);
    gPlayingChaseBGM = 0;
    SdSoundDisable();
}

// ---- positional SE math (the ear at `from`, the sound at `to`; angles in degrees around Y, 0..360) -----

#define DEG_FULL_TURN 360.0f
#define DEG_HALF_TURN 180.0f
#define PI_F 3.1415927f     // the ROM's literal (0x40490fdb)

// CT3 0x018c88d0: the distance from `from` to `to` (fabsf of a square root, as the ROM's tail call does).
float SSGetPositional3Dist(const float *from, const float *to)
{
    float dx = to[0] - from[0];
    float dy = to[1] - from[1];
    float dz = to[2] - from[2];

    return fabsf(sqrtf(dx * dx + dy * dy + dz * dz));
}

// CT3 0x018c8920: the direction from `from` to `to` in the XZ plane, relative to `facing`, in 0..360.
float SSGetPositional3(float facing, const float *from, const float *to)
{
    float angle = SSGetAngle(to[0] - from[0], to[2] - from[2]);

    if (!(facing <= angle)) {
        angle += DEG_FULL_TURN;
    }
    return angle - facing;
}

// CT3 0x018c8980: SSGetPositional3 that also stores the distance in *dist.
float SSGetPositional2(float facing, const float *from, const float *to, float *dist)
{
    float dx = to[0] - from[0];
    float dy = to[1] - from[1];
    float dz = to[2] - from[2];
    float angle;

    *dist = fabsf(sqrtf(dx * dx + dy * dy + dz * dz));
    angle = SSGetAngle(dx, dz);
    if (!(facing <= angle)) {
        angle += DEG_FULL_TURN;
    }
    return angle - facing;
}

// CT3 0x018c8a40: the angle of (x, z) from +X, 0..360 (z < 0 is the lower half); 0 length reads as acos(0).
float SSGetAngle(float x, float z)
{
    float len = sqrtf(x * x + z * z);
    float angle;

    x = (len == 0.0f) ? 0.0f : x / len;
    angle = (DEG_HALF_TURN * acosf(x)) / PI_F;
    if (z < 0.0f) {
        angle = DEG_FULL_TURN - angle;
    }
    return angle;
}

// ---- SQ music: the stage rules (TRIG_VAR_STAGE picks Branch1..5) ---------------------------------------
// BranchNSSPlaySQ answers 0 (do not play: a chase BGM is on, or stage 1 refuses), the STAGE_BGM_* to record
// (compared on the name's first two letters), or STAGE_BGM_OTHER (play, record nothing). BranchNSSStopSQ
// answers 1 (stop it) and forgets the stage BGM when the name is the one it watches (compared on that
// name's own length, as the ROM's memcmp counts).

// CT3 0x018c8cf0
int Branch5SSStopSQ(const char *name)
{
    if (memcmp(name, "OMOIDE", 6) == 0) {
        gPlayingStageBGM = STAGE_BGM_NONE;
    }
    return 1;
}

// CT3 0x018c8d30
int Branch5SSPlaySQ(const char *name)
{
    if (memcmp(name, "OMOIDE", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_OMOIDE : 0;
    }
    return STAGE_BGM_OTHER;
}

// CT3 0x018c8f90
int Branch4SSStopSQ(const char *name)
{
    if (memcmp(name, "FUAN", 4) == 0) {
        gPlayingStageBGM = STAGE_BGM_NONE;
    }
    return 1;
}

// CT3 0x018c8fd0
int Branch4SSPlaySQ(const char *name)
{
    if (memcmp(name, "TSUNORU", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_TSUNORU : 0;
    }
    if (memcmp(name, "KOJO", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_KOJO : 0;
    }
    if (memcmp(name, "FUAN", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_FUAN : 0;
    }
    if (memcmp(name, "OMOIDE", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_OMOIDE : 0;
    }
    return STAGE_BGM_OTHER;
}

// CT3 0x018c93a0
int Branch3SSStopSQ(const char *name)
{
    (void)name;
    return 1;
}

// CT3 0x018c93b0
int Branch3SSPlaySQ(const char *name)
{
    if (memcmp(name, "CHITEI", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_CHITEI : 0;
    }
    if (memcmp(name, "HAKABA", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_HAKABA : 0;
    }
    return STAGE_BGM_OTHER;
}

// CT3 0x018c9830
int Branch2SSStopSQ(const char *name)
{
    if (memcmp(name, "KYOHU", 5) == 0) {
        gPlayingStageBGM = STAGE_BGM_NONE;
    }
    return 1;
}

// CT3 0x018c9870
int Branch2SSPlaySQ(const char *name)
{
    if (memcmp(name, "HINMIN", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_HINMIN : 0;
    }
    if (memcmp(name, "KYOHU", 2) == 0) {
        return gPlayingChaseBGM == 0 ? STAGE_BGM_KYOHU : 0;
    }
    return STAGE_BGM_OTHER;
}

// CT3 0x018c9bf0
int Branch1SSStopSQ(const char *name)
{
    (void)name;
    return 1;
}

// CT3 0x018c9c00: stage 1 plays every SQ (and records STAGE_BGM_STAGE1), except the piano and miss SQs,
// which wait on the stage's events and the current bank.
int Branch1SSPlaySQ(const char *name)
{
    if (memcmp(name, "PIANO", 2) == 0 || memcmp(name, "MISS", 4) == 0 || memcmp(name, "MISL", 4) == 0) {
        if (gPlayingChaseBGM != 0) {
            return 0;
        }
        if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE1_SQ_ALLOW, 1, 0) != 1) {
            if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE1_SQ_1B04, 1, 0) == 1
                && memcmp(BankInfo[idCurrentScene].name, "1B04", 4) != 0) {
                return 0;
            }
            if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE1_SQ_BLOCK_3BF, 1, 0) == 1
                || (char)ssTrigCheckEvent(TRIG_EVENT_STAGE1_SQ_BLOCK_3BD, 1, 0) == 1) {
                return 0;
            }
        }
    }
    return STAGE_BGM_STAGE1;
}

// CT3 0x018ca8c0: a demo ends (VistaVisionEnd): the special SEs as they were, and from stage 2 on the
// demo's SQ back to its volume, or trigger variable 0x4d cleared when no SQ was turned down.
void MidiControl_DemoEnd(void)
{
    gDemoStartFlg = 0;
    memcpy(gPlaySESp1, gSavePlaySESp1, sizeof gSavePlaySESp1);
    if (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0) > 1) {
        if (gPlayMidiDemo[0] == '\0') {
            ssTrigSetVariable(TRIG_VAR_SOUND_4D, 0, 1, 0);
        } else {
            SSVolumeSQ(gPlayMidiDemo, gPlayMidiDemoVol, 1);
            gPlayMidiDemo[0] = '\0';
        }
    }
}

// CT3 0x018cb170
void SSVolumeSQ(const char *name, int vol, int a2)
{
    SdVolumeSQ(name, SSVolumeAdjustSQ(name, vol), a2);
}

// CT3 0x018cb1b0: stop SQ name if the stage's rule agrees, silenced first when muteFirst.
// The ROM leaves its answer register (s0) unset when the stage is not 1..5, so the stop then depends on
// the caller's s0; the port takes 0 there (no stop), hazards.md.
void SSStopSQ(const char *name, int muteFirst)
{
    int stop = 0;

    switch (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0)) {
    case 5: stop = Branch5SSStopSQ(name); break;
    case 4: stop = Branch4SSStopSQ(name); break;
    case 3: stop = Branch3SSStopSQ(name); break;
    case 2: stop = Branch2SSStopSQ(name); break;
    case 1: stop = Branch1SSStopSQ(name); break;
    default: break;
    }
    if (stop != 0) {
        if (muteFirst != 0) {
            SSVolumeSQ(name, 0, 1);
        }
        SdStopSQ(name);
    }
}

// CT3 0x018cb360: play SQ name at vol if the stage's rule agrees, and record the stage BGM it is.
void SSPlaySQ(const char *name, int vol)
{
    int bgm;

    switch (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0)) {
    case 5: bgm = Branch5SSPlaySQ(name); break;
    case 4: bgm = Branch4SSPlaySQ(name); break;
    case 3: bgm = Branch3SSPlaySQ(name); break;
    case 2: bgm = Branch2SSPlaySQ(name); break;
    case 1: bgm = Branch1SSPlaySQ(name); break;
    default: bgm = 0; break;
    }
    if (bgm != 0) {
        SSPlaySQ2(name, vol);
        if (bgm != STAGE_BGM_OTHER) {
            gPlayingStageBGM = bgm;
        }
    }
}

// ---- SQ music ------------------------------------------------------------------------------------------

// CT3 0x018cb2b0: play the SQ file by name (DATA\SOUND\SQ\) at vol; an unknown name only prints.
int SSPlaySQ2(const char *name, int vol)
{
    int index = SSFindSQSPU2(name);

    if (index < 0) {
        return printf(s_sqNotFound, name);
    }
    vol = SSVolumeAdjustSQ(name, vol);
    return SdPlaySQ(s_sqDir, name, gFileSQ[index].sdArg2, vol, 0, gFileSQ[index].sdArg5,
                    gFileSQ[index].sdArg6);
}

// CT3 0x018cb460: a per-file volume hook; the ROM's answers vol unchanged.
int SSVolumeAdjustSQ(const char *name, int vol)
{
    (void)name;
    return vol;
}

// CT3 0x018cb470: gFileSQ's row for name (case-insensitive), or -1.
int SSFindSQSPU2(const char *name)
{
    char upper[64];
    int i;

    strcpy(upper, name);
    strupr(upper);
    for (i = 0; gFileSQ[i].name != nullptr; i++) {
        if (strcmp(gFileSQ[i].name, upper) == 0) {
            return i;
        }
    }
    return -1;
}

// ---- SE banks ------------------------------------------------------------------------------------------

// CT3 0x018cb4f0: stop the SE of name's bank.
void SSStopSE(const char *name, int a1, int a2)
{
    SdStopSESDT(GetJLSbySEFileName(name), a1, a2);
}

// CT3 0x018cb6a0: play an SE of name's bank, unless a demo is being skipped or the mute event is set.
int SSPlaySE(const char *name, int a1, int a2)
{
    if (DemoSkip != 0) {
        return 0;
    }
    if ((char)ssTrigCheckEvent(TRIG_EVENT_SE_MUTE, 1, 0) == 1) {
        return 0;
    }
    return SdReqPlaySE(GetJLSbySEFileName(name), a1, a2);
}

// CT3 0x018cb730: the Sd slot of name's bank (its first two letters, case as given), SE_JLS_DEFAULT when
// none of the six banks matches.
int GetJLSbySEFileName(const char *name)
{
    char copy[64];
    int i;

    strcpy(copy, name);
    for (i = 0; i < SE_FILE_COUNT; i++) {
        if (strncmp(gFileSE[i].prefix, copy, 2) == 0) {
            return gFileSE[i].jls;
        }
    }
    return SE_JLS_DEFAULT;
}

// CT3 0x018cb7c0: a tail jump to the driver.
void SSPurgeSE(const char *name)
{
    SdPurgeSE(name);
}

// CT3 0x018cb7d0: a tail jump to the driver.
void SSLoadSEEndWait(int handle)
{
    SdLoadSEEndWait(handle);
}

// CT3 0x018cb7e0: request the load of SE bank name (DATA\SOUND\) into its prefix's slot; an unknown prefix
// is fs_error's "FILE NOT FOUND" and answers 0.
int SSLoadSE(const char *name, int a1)
{
    char upper[64];
    int i;

    strcpy(upper, name);
    strupr(upper);
    for (i = 0;; i++) {
        if (gFileSE[i].prefix == nullptr) {
            fs_error(s_srcFile, SSLOADSE_ERROR_LINE, s_seLoad, s_fileNotFound, upper);
            return 0;
        }
        if (strncmp(gFileSE[i].prefix, upper, 2) == 0) {
            break;
        }
    }
    return SdReqLoadSE(s_soundDir, upper, gFileSE[i].jls, gFileSE[i].spuAddr, a1);
}

// CT3 0x018cb8d0: a tail jump: the ADS stream's volume (a0 unused).
void SSVolumeADS(int a0, int vol)
{
    (void)a0;
    SdVolumeStream(vol);
}

// CT3 0x018cb8e0: a tail jump: stop the ADS stream (a0 unused).
void SSStopADS(int a0, int a1)
{
    (void)a0;
    SdStopStream(a1);
}

// ---- boot, scene and end -------------------------------------------------------------------------------

// CT3 0x018cbc90: a tail jump to SSInitClearValiable (data_load_set).
void Sound_data_load_set(void)
{
    SSInitClearValiable();
}

// CT3 0x018cbca0: stop everything (system_end).
void Sound_system_end(void)
{
    EventSoundEnd();
    SdStopSEAllSp1();
    SdStopSEAll();
    SdLoadSEEndWaitAll();
    SdStopSQAllNow();
    SdStopStream(0);
    SdSetEffect(1, 0, 0);
    SSInitClearValiable();
}

// CT3 0x018cbd00: clear the state, set the reverb, and load the four resident SE banks one at a time
// (system_init).
void Sound_system_init(void)
{
    SSInitClearValiable();
    gBfoScene = SCENE_CODE_BOOT_BFO;
    gNowScene = SCENE_CODE_BOOT_NOW;
    SdSetEffect(0, 3, 0x3fff);
    SSLoadSEEndWait(SSLoadSE("C_0000", 0));
    SSLoadSEEndWait(SSLoadSE("T_0000", 0));
    SSLoadSEEndWait(SSLoadSE("E_HAMMER", 0));
    SSLoadSEEndWait(SSLoadSE("Z_REI", 0));
}

// CT3 0x018cbda0: empty in the ROM.
void Sound_FirstInitialization(void)
{
}

// CT3 0x018cbdb0: empty in the ROM (IOPInit).
void SSInit(void)
{
}

// CT3 0x018cbdc0: forget the BGM state (the symtab spells it "Valiable"). gPlayingChaseBGM is cleared
// twice, as the ROM does.
void SSInitClearValiable(void)
{
    ssTrigSetVariable(TRIG_VAR_SOUND_4D, 0, 1, 0);
    gPlayingChaseBGM = 0;
    ssTrigSetVariable(TRIG_VAR_SOUND_58, 0, 1, 0);
    gBgmControl = 0;
    gBgmControlSaveVol = 0;
    gBgmControlSaveFile[0] = '\0';
    gPlayMidiDemo[0] = '\0';
    gPlayMidiDemoVol = 0;
    hhXcmd_InitPlayStopMIDI();
    gPlayingStageBGM = STAGE_BGM_NONE;
    gPlayingChaseBGM = 0;
    gPlayTbreak = 0;
    gBossAlysDead = 0;
    gDemoStartFlg = 0;
}

// CT3 0x018cbe30: push the option menu's sound settings to the driver (mode: enum SOUND_MODE).
void SSSetSoundMode(int mode)
{
    if (mode == SOUND_MODE_SE_VOLUME) {
        gMasterVolSE = (SE_MASTER_VOLUME_BUFF * SD_VOLUME_MAX) / SOUND_MENU_VOLUME_STEPS;
        SdMasterVolumeSetSE(gMasterVolSE);
    } else if (mode == SOUND_MODE_BGM_VOLUME) {
        gMasterVolSQ = (BGM_MASTER_VOLUME_BUFF * SD_VOLUME_MAX) / SOUND_MENU_VOLUME_STEPS;
        SdMasterVolumeSetSQ(gMasterVolSQ);
        sceGsSyncV(0);
        gMasterVolADS = (BGM_MASTER_VOLUME_BUFF * SD_VOLUME_MAX) / SOUND_MENU_VOLUME_STEPS;
        SdMasterVolSetADS(gMasterVolADS);
    } else if (mode == SOUND_MODE_STEREO) {
        gStereo = (SOUND_STEREO_MONAURAL_BUFF == 0);
        SdChangeStereoMono();
    } else if (mode == SOUND_MODE_ALL) {
        SSSetSoundMode(SOUND_MODE_STEREO);
        sceGsSyncV(0);
        SSSetSoundMode(SOUND_MODE_BGM_VOLUME);
        sceGsSyncV(0);
        SSSetSoundMode(SOUND_MODE_SE_VOLUME);
        sceGsSyncV(0);
    }
}

// ---- stubs (tools/reverse/stubgen.py) ----
