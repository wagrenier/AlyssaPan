// game/sound.c -- Clock Tower 3: the game's sound layer over the Sd driver (SE banks by file name, SQ
// music by name, the stage BGM state across movies, the option menu's volumes). CT3 0x018c7fb0..0x018cbf90.
// Slice 1 reconstructed 2026-11-09 (38 functions): the boot / end path, the SE and SQ entry points, the
// stages' SQ rules (Branch1..5 SSPlaySQ / SSStopSQ), the movie and demo mute, the angle helpers.
// Slice 2 (2026-11-09, 30 functions): the BGM state machines (SSProc, SSBGMProc, Branch1..5BGMProc), the
// sound objects and trigger actions, the positional updates, the volume save / restore around demos and
// pauses, SSPlayADS, SSPlaySESp1 / SSStopSESp1, SceneSELoad / BranchSELoad. Still a stub:
// SSLoadSEfromLoadPoint (it reads fileio.c's load-point record, +0xd4).
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

// CT3 0x0193a020 gFileADS (.data, 0x26c bytes): the ADS streams SSPlayADS knows, with SdPlayStream's
// fourth argument (1 for the numbered BGM streams, 0 for the one-shot ones: a loop flag, by its rows).
const ADS_FILE gFileADS[31] = {
    {"01.ADS", 1},      {"02.ADS", 1},      {"03.ADS", 1},      {"10.ADS", 1},
    {"11.ADS", 1},      {"12.ADS", 1},      {"16.ADS", 0},      {"17.ADS", 0},
    {"18.ADS", 0},      {"19.ADS", 0},      {"22.ADS", 1},      {"23.ADS", 1},
    {"24.ADS", 0},      {"29.ADS", 1},      {"30.ADS", 1},      {"GEKITAI.ADS", 0},
    {"T_BREAK.ADS", 0}, {"B_BREAK.ADS", 0}, {"DICK1.ADS", 0},   {"DICK2.ADS", 0},
    {"DICK3.ADS", 0},   {"DICK4.ADS", 0},   {"DICK5.ADS", 0},   {"OTHER01.ADS", 0},
    {"OTHER02.ADS", 0}, {"OTHER03.ADS", 0}, {"OTHER04.ADS", 0}, {"OTHER05.ADS", 0},
    {"OTHER06.ADS", 0}, {"OTHER07.ADS", 0}, {"", 0},
};

// CT3 0x0193a2a0 sLoopSE (.data, 0x23c bytes): per SE bank, the SE numbers that loop -- the ones
// SSReturnVolumeSE replays instead of restoring a volume. Each list ends with LOOP_SE_END.
static const LOOP_SE sLoopSE[13] = {
    {"C_0000", {0x3d, 0x4b, LOOP_SE_END}},
    {"E_BAL", {0x1b, LOOP_SE_END}},
    {"E_HAMMER", {0x21, LOOP_SE_END}},
    {"E_RYUSAN", {0x00, 0x01, LOOP_SE_END}},
    {"R_2A02", {0x03, LOOP_SE_END}},
    {"R_3C05", {0x00, LOOP_SE_END}},
    {"R_4B01", {0x02, LOOP_SE_END}},
    {"R_5A04", {0x02, 0x04, 0x06, 0x09, 0x0a, LOOP_SE_END}},
    {"T_0010", {0x05, 0x0f, LOOP_SE_END}},
    {"T_00A0", {0x00, LOOP_SE_END}},
    {"T_00B0", {0x02, LOOP_SE_END}},
    {"Z_YUMIGT", {0x00, LOOP_SE_END}},
    {nullptr, {0, 0, 0, 0, 0, 0, 0, 0, LOOP_SE_END, 0}},
};
#define LOOP_SE_BANKS 12   // SSReturnVolumeSE_CheckLoopSE's scan bound (the terminator is not read)

// sound.c's .sbss variables from 0x01942d48 (the symtab's names and sizes).
int gDemoStartFlg;                  /* sbss 0x01942d48 */
int gBgmControlSaveVol;             /* sbss 0x01942d4c */
char gBgmControlSaveFile[0x38];     /* sbss 0x01942d50 */
int gBgmControl;                    /* sbss 0x01942d88 */
int gPlayMidiDemoVol;               /* sbss 0x01942d8c */
char gPlayMidiDemo[0x38];           /* sbss 0x01942d90 */
int gBossAlysDead;                  /* sbss 0x01942dc8 */
int gPlayTbreak;                    /* sbss 0x01942dcc */
int gChaseStopDelay;                /* sbss 0x01942dd0 */
int gPlayingChaseBGM;               /* sbss 0x01942dd4 */
int gPlayingStageBGM;               /* sbss 0x01942dd8 */
PLAY_SE_SP1 gSavePlaySESp1[PLAY_SE_SP1_COUNT]; /* sbss 0x01942de0 */

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
static const char s_soundObjNotFound[] = "sound object not found [maxid=%d]\n"; // 0x01936520 (@1127)

// ---- sound objects (a bank's SE sources; the trigger actions) ------------------------------------------

// CT3 0x018c7fb0: bank's sound object id (ALL_TABLE.sound), or nullptr (id 0, no table, not listed).
SOUND_OBJ *SSFindSoundObj(int bank, int id)
{
    SOUND_OBJ_TABLE *table;
    SOUND_OBJ *obj;

    if (id == 0) {
        return nullptr;
    }
    table = All_table_TBL[bank]->sound;
    if (table == nullptr) {
        return nullptr;
    }
    for (obj = (SOUND_OBJ *)BankEePtr(bank, table->objs);; obj++) {
        if (obj->id == id) {
            return obj;
        }
        if (obj->id == 0) {
            return nullptr;
        }
    }
}

// CT3 0x018c8020: a room is entered: start its auto-start sound objects (C_0000 SEs); a positional one
// gets its source and SetPositionalParam_Alys as the driver's per-frame update. Banks of type
// BANK_TYPE_NO_ROOM_SOUND start nothing.
void SSAutoStartSoundRoomIn(int bank)
{
    SOUND_OBJ_TABLE *table;
    SOUND_OBJ *objs;
    int i;

    if (BankInfo[bank].type == BANK_TYPE_NO_ROOM_SOUND) {
        return;
    }
    table = All_table_TBL[bank]->sound;
    if (table == nullptr) {
        return;
    }
    objs = (SOUND_OBJ *)BankEePtr(bank, table->objs);
    for (i = 0; i < SOUND_OBJ_MAX; i++) {
        SOUND_OBJ *obj = &objs[i];
        int handle;

        if (objs == nullptr || obj->id == 0) {
            return;
        }
        if (obj->autoStart == 0) {
            continue;
        }
        handle = SSPlaySE(SE_BANK_COMMON, obj->seNo, 1);
        if (obj->positional != 0) {
            SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK | SD_SE_PARAM_CALLBACK,
                         0, obj->id, bank, SetPositionalParam_Alys);
            SetPositionalParam_Alys(handle);
        }
    }
}

// CT3 0x018c81c0: empty in the ROM.
void SSActVolume(int bank, int id)
{
    (void)bank;
    (void)id;
}

// CT3 0x018c81d0: trigger action: stop sound object id's SE.
int SSActStopSound(int bank, int id)
{
    SOUND_OBJ *obj = SSFindSoundObj(bank, id);

    if (obj == nullptr) {
        return printf(s_soundObjNotFound, id);
    }
    SSStopSE(SE_BANK_COMMON, obj->seNo, 1);
    return 0;
}

// CT3 0x018c8230: trigger action: play sound object id -- its seNo picks one of
// five fixed SEs (C_0000 1 / 2 / 3 / 5, T_0000 0); any other plays nothing.
int SSActPlaySound(int bank, int id)
{
    SOUND_OBJ *obj = SSFindSoundObj(bank, id);

    if (obj == nullptr) {
        printf(s_soundObjNotFound, id);
        return 0;
    }
    switch (obj->seNo) {
    case 8: return SSPlaySE(SE_BANK_T0000, 0, 1);
    case 6: return SSPlaySE(SE_BANK_COMMON, 2, 1);
    case 5: return SSPlaySE(SE_BANK_COMMON, 3, 1);
    case 9: return SSPlaySE(SE_BANK_COMMON, 5, 1);
    case 3: return SSPlaySE(SE_BANK_COMMON, 1, 1);
    default: return 0;
    }
}

// CT3 0x018c8330: trigger action: load SE bank name (a tail jump).
int SSActLoadBankSE(const char *name)
{
    return SSLoadSE(name, 0);
}

// ---- positional SEs (the driver calls these every frame through SD_SE.callback) ------------------------

// The three updates share one shape: the SE's source (bank, objId) -> volume and pan, or a stop when the
// source is gone. Every SE but a SD_SE_SAVED_RESTART-marked one also gets its unk30 back: the ROM leaves
// it in the fifth argument register (t0) from the compare, and the mask's 0x20000000 takes it.

// CT3 0x018c8340: a special SE heard from the player, its fall-off from posMode; stops it as Sp1.
void SetPositionalParam_Alys_Sp1(int handle)
{
    SD_SE *se;
    int vol;
    int pan;

    if (handle == 0 || (se = SdFindPlaySE(handle)) == nullptr) {
        return;
    }
    if (SSGetPositional(se->bank, se->objId, &vol, &pan, 1, se->posMode) == -1) {
        SdStopSESp1ByHandle(handle);
    } else if (se->unk30 == SD_SE_SAVED_RESTART) {
        SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_PAN, vol, pan);
    } else {
        SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_PAN | SD_SE_PARAM_30, vol, pan, se->unk30);
    }
}

// CT3 0x018c8400: an SE heard from the player (fall-off 3).
void SetPositionalParam_Alys(int handle)
{
    SD_SE *se;
    int vol;
    int pan;

    if (handle == 0 || (se = SdFindPlaySE(handle)) == nullptr) {
        return;
    }
    if (SSGetPositional(se->bank, se->objId, &vol, &pan, 1, POSITIONAL_FALLOFF_DEFAULT) == -1) {
        SdStopSE(handle);
    } else if (se->unk30 == SD_SE_SAVED_RESTART) {
        SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_PAN, vol, pan);
    } else {
        SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_PAN | SD_SE_PARAM_30, vol, pan, se->unk30);
    }
}

// CT3 0x018c84c0: an SE heard from the camera (fall-off 3).
void SetPositionalParam_Cam(int handle)
{
    SD_SE *se;
    int vol;
    int pan;

    if (handle == 0 || (se = SdFindPlaySE(handle)) == nullptr) {
        return;
    }
    if (SSGetPositional(se->bank, se->objId, &vol, &pan, 0, POSITIONAL_FALLOFF_DEFAULT) == -1) {
        SdStopSE(handle);
    } else if (se->unk30 == SD_SE_SAVED_RESTART) {
        SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_PAN, vol, pan);
    } else {
        SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_PAN | SD_SE_PARAM_30, vol, pan, se->unk30);
    }
}

// CT3 0x018c8580: the volume and pan of sound source objId of bank, heard from the player (fromPlayer)
// or the camera, facing the camera's view. The source is model part objId (a type-2 bank with objId < 1:
// the part PARTS_HASH_SOUND_ORIGIN names); -1 when that part is missing (vol / pan 0) or the bank is not
// a loaded single bank.
int SSGetPositional(int bank, int objId, int *vol, int *pan, int fromPlayer, int falloff)
{
    float src[4];
    float camPos[4];
    float camTrg[4];
    float facing;
    float dist;
    int part = objId;
    MODEL_PARTS *parts;

    if (BankInfo[bank].type == BANK_TYPE_2 && objId < 1) {
        part = asFindModelPartsID(pModelPartsHeader[bank], PARTS_HASH_SOUND_ORIGIN);
        if (part == -1) {
            *pan = 0;
            *vol = 0;
            return -1;
        }
    }
    if (BankInfo[bank].count != 1 || BankInfo[bank].loaded == 0) {
        return -1;
    }
    parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    memcpy(src, parts[part].pos, sizeof src);
    CameraMatrix2CameraPos(camPos);
    CameraMatrix2CameraTrg3m(camTrg);
    facing = SSGetAngle(camTrg[0] - camPos[0], camTrg[2] - camPos[2]);
    if (fromPlayer == 0) {
        facing = SSGetPositional2(facing, camPos, src, &dist);
    } else {
        const float *player = (const float *)&Character[Player_BankID].posBefore;

        facing = SSGetPositional3(facing, player, src);
        dist = SSGetPositional3Dist(player, src);
    }
    SSGetPositionalTo0_64_127(dist, facing, vol, pan, falloff);
    return 0;
}

// CT3 0x018c8810: distance and angle -> the driver's volume (0x7f minus falloff per 55 units) and pan
// (0..0x7f: the angle in 256ths of a turn, folded so front and back meet).
void SSGetPositionalTo0_64_127(float dist, float angle, int *vol, int *pan, int falloff)
{
    int atten = (int)(dist / POSITIONAL_UNIT) * falloff;
    int turn;
    int p;

    if (atten > SQ_VOLUME_MAX) {
        atten = SQ_VOLUME_MAX;
    }
    *vol = SQ_VOLUME_MAX - atten;
    turn = ((int)angle * 0x100) / 360;
    p = turn + 0x40;
    if (p > 0xff) {
        p = turn - 0xc0;
    }
    if (p < 0x80) {
        p = 0x7f - p;
    } else {
        p -= 0x80;
    }
    if (p > 0x7f) {
        p = 0x7f;
    }
    *pan = p;
}

// ---- the movie mute (AA_StartMovie)--------------------------------------------------------------------

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

// CT3 0x018c8d80: stage 5's BGM rule (OMOIDE). Per frame: in 5A02 event 0x3e1 stops OMOIDE; in 5A01
// event 0x3ac starts it. On a scene change: 5A99 stops it, 5A01 sets the reverb and starts it on event
// 0x3ac; from boot 5A02 starts it, from any other scene every other room does.
void Branch5BGMProc(int sceneChange)
{
    if (sceneChange == 0) {
        if (gNowScene == SCENE_CODE('5', 'A', '0', '2')) {
            if (gPlayingStageBGM == STAGE_BGM_OMOIDE
                && (char)ssTrigCheckEvent(TRIG_EVENT_STAGE5_OMOIDE_STOP, 1, 0) == 1) {
                SSStopSQ("OMOIDE", 1);
                gPlayingStageBGM = STAGE_BGM_NONE;
            }
        } else if (gNowScene == SCENE_CODE('5', 'A', '0', '1') && gPlayingStageBGM != STAGE_BGM_OMOIDE
                   && (char)ssTrigCheckEvent(TRIG_EVENT_STAGE5_OMOIDE, 1, 0) == 1) {
            SSPlaySQ("OMOIDE", SQ_VOLUME_MAX);
        }
    } else if (gBfoScene == SCENE_CODE_BOOT_BFO) {
        if (gNowScene == SCENE_CODE('5', 'A', '9', '9')) {
            SSStopSQ("OMOIDE", 1);
            gPlayingStageBGM = STAGE_BGM_NONE;
        } else if (gNowScene == SCENE_CODE('5', 'A', '0', '2')) {
            SSPlaySQ("OMOIDE", SQ_VOLUME_MAX);
        } else if (gNowScene == SCENE_CODE('5', 'A', '0', '1')) {
            SdSetEffect(1, 3, 0x78);
            if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE5_OMOIDE, 1, 0) == 1) {
                SSPlaySQ("OMOIDE", SQ_VOLUME_MAX);
            }
        }
    } else if (gNowScene == SCENE_CODE('5', 'A', '9', '9')) {
        SSStopSQ("OMOIDE", 1);
        gPlayingStageBGM = STAGE_BGM_NONE;
    } else if (gNowScene == SCENE_CODE('5', 'A', '0', '1')) {
        SdSetEffect(1, 3, 0x78);
        if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE5_OMOIDE, 1, 0) == 1) {
            SSPlaySQ("OMOIDE", SQ_VOLUME_MAX);
        }
    } else {
        SSPlaySQ("OMOIDE", SQ_VOLUME_MAX);
    }
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

// CT3 0x018c90b0: stage 4's BGM rule (TSUNORU in 4A, FUAN in 4B, KOJO from 5A01, OMOIDE in 0A16; the
// reverb per room). Per frame: in 4A04 event 0x5c6 stops TSUNORU. A scene change from boot only sets 4C01's
// dry reverb or 4A01's TSUNORU; 0A30 and 4N03 change nothing.
void Branch4BGMProc(int sceneChange)
{
    u_int now = gNowScene;

    if (sceneChange == 0) {
        if (now == SCENE_CODE('4', 'A', '0', '4')
            && (char)ssTrigCheckEvent(TRIG_EVENT_STAGE4_TSUNORU_OFF, 1, 0) == 1
            && gPlayingStageBGM != STAGE_BGM_NONE) {
            SSStopSQ("TSUNORU", 1);
            gPlayingStageBGM = STAGE_BGM_NONE;
        }
        return;
    }
    if (gBfoScene == SCENE_CODE_BOOT_BFO) {
        if (now == SCENE_CODE('4', 'C', '0', '1')) {
            SdSetEffect(1, 0, 0);
        } else if (now == SCENE_CODE('4', 'A', '0', '1')) {
            SdSetEffect(1, 3, 0x78);
            if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE4_TSUNORU_OFF, 1, 0) != 1) {
                SSPlaySQ("TSUNORU", SQ_VOLUME_MAX);
            }
        }
        return;
    }
    if (now == SCENE_CODE('0', 'A', '3', '0')) {
        return;
    }
    if (now == SCENE_CODE('0', 'A', '1', '6')) {
        SSPlaySQ("OMOIDE", SQ_VOLUME_MAX);
    } else if (now == SCENE_CODE('4', 'B', '0', '3')) {
        SdSetEffect(1, 0, 0);
    } else if (now == SCENE_CODE('4', 'B', '1', '0')) {
        SdSetEffect(1, 3, 100);
        SSPlaySQ("FUAN", SQ_VOLUME_MAX);
    } else if (now == SCENE_CODE('4', 'B', '0', '9') || now == SCENE_CODE('4', 'B', '0', '1')) {
        SSStopSQ("FUAN", 1);
        gPlayingStageBGM = STAGE_BGM_NONE;
    } else if (now == SCENE_CODE('4', 'N', '0', '3')) {
        return;
    } else if (now == SCENE_CODE('4', 'B', '0', '6')) {
        SSStopSQ("KOJO", 1);
        gPlayingStageBGM = STAGE_BGM_NONE;
    } else if (now == SCENE_CODE('4', 'B', '0', '5') || now == SCENE_CODE('4', 'B', '0', '4')) {
        SdSetEffect(1, 0, 0);
    } else if (now == SCENE_CODE('5', 'A', '0', '1')) {
        SdSetEffect(1, 3, 0x78);
        SSPlaySQ("KOJO", SQ_VOLUME_MAX);
    } else if (now == SCENE_CODE('4', 'C', '0', '1')) {
        SdSetEffect(1, 0, 0);
    } else if (now == SCENE_CODE('4', 'A', '0', '1') || now == SCENE_CODE('4', 'A', '0', '2')) {
        if (now == SCENE_CODE('4', 'A', '0', '2')) {
            SdSetEffect(1, 3, 0x78);
        }
        if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE4_TSUNORU_OFF, 1, 0) != 1) {
            SSPlaySQ("TSUNORU", SQ_VOLUME_MAX);
        }
    }
}

// CT3 0x018c9430: stage 3's BGM rule (HAKABA in 3D / 3E, CHITEI in 3A / 3F; the reverb per room).
// Per frame: 3D03 starts HAKABA on event 0x3e4 unless 0x3af or 0x3d6 is set; 3A01 starts CHITEI on 0x3d1.
void Branch3BGMProc(int sceneChange)
{
    u_int now = gNowScene;

    if (sceneChange == 0) {
        if (now == SCENE_CODE('3', 'D', '0', '3')) {
            if (ssTrigCheckEvent(TRIG_EVENT_STAGE3_HAKABA_OFF, 1, 0) == 0
                && ssTrigCheckEvent(TRIG_EVENT_STAGE3_HAKABA_OFF2, 1, 0) == 0
                && gPlayingStageBGM != STAGE_BGM_HAKABA
                && (char)ssTrigCheckEvent(TRIG_EVENT_STAGE3_HAKABA, 1, 0) == 1) {
                SSPlaySQ("HAKABA", SQ_VOLUME_MAX);
            }
        } else if (now == SCENE_CODE('3', 'A', '0', '1') && gPlayingStageBGM != STAGE_BGM_CHITEI
                   && (char)ssTrigCheckEvent(TRIG_EVENT_STAGE3_CHITEI, 1, 0) == 1) {
            SSPlaySQ("CHITEI", SQ_VOLUME_MAX);
        }
    } else if (gBfoScene == SCENE_CODE_BOOT_BFO) {
        if (now == SCENE_CODE('3', 'E', '0', '1')) {
            SdSetEffect(1, 3, 0x78);
            SSPlaySQ("HAKABA", SQ_VOLUME_MAX);
        } else if (now == SCENE_CODE('3', 'D', '0', '3')) {
            if (ssTrigCheckEvent(TRIG_EVENT_STAGE3_HAKABA_OFF, 1, 0) == 0
                && ssTrigCheckEvent(TRIG_EVENT_STAGE3_HAKABA_OFF2, 1, 0) == 0) {
                SSPlaySQ("HAKABA", SQ_VOLUME_MAX);
            }
        } else if (now == SCENE_CODE('3', 'C', '0', '5')) {
            SdSetEffect(1, 0, 0);
        } else if (now == SCENE_CODE('3', 'A', '0', '1')) {
            SdSetEffect(1, 3, 0x78);
        }
    } else if (now == SCENE_CODE('3', 'D', '0', '1')) {
        SSStopSQ("HAKABA", 1);
        gPlayingStageBGM = STAGE_BGM_NONE;
    } else if (now == SCENE_CODE('B', 'T', '3', '1')) {
        SSStopSQ("CHITEI", 1);
        gPlayingStageBGM = STAGE_BGM_NONE;
    } else if (now == SCENE_CODE('3', 'D', '0', '5')) {
        SdSetEffect(1, 3, 0x78);
    } else if (now == SCENE_CODE('3', 'F', '0', '1')) {
        SdSetEffect(1, 0, 0);
        SSStopSQ("HAKABA", 1);
        gPlayingStageBGM = STAGE_BGM_NONE;
        SSPlaySQ("CHITEI", SQ_VOLUME_MAX);
    } else if (now == SCENE_CODE('3', 'E', '0', '1')) {
        SdSetEffect(1, 3, 0x78);
        SSPlaySQ("HAKABA", SQ_VOLUME_MAX);
    } else if (now == SCENE_CODE('3', 'D', '0', '3')) {
        SdSetEffect(1, 0, 0);
        if (ssTrigCheckEvent(TRIG_EVENT_STAGE3_HAKABA_OFF, 1, 0) == 0) {
            if (ssTrigCheckEvent(TRIG_EVENT_STAGE3_HAKABA_OFF2, 1, 0) == 0) {
                if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE3_HAKABA, 1, 0) == 1) {
                    SSPlaySQ("HAKABA", SQ_VOLUME_MAX);
                }
            } else {
                SSStopSQ("CHITEI", 1);
                gPlayingStageBGM = STAGE_BGM_NONE;
            }
        }
    } else if (now == SCENE_CODE('3', 'C', '0', '6')) {
        SdSetEffect(1, 0, 0);
        SSStopSQ("CHITEI", 1);
        gPlayingStageBGM = STAGE_BGM_NONE;
    } else if (now == SCENE_CODE('3', 'C', '0', '5')) {
        SdSetEffect(1, 0, 0);
    } else if (now == SCENE_CODE('3', 'A', '0', '2')) {
        SdSetEffect(1, 3, 0x78);
        SSPlaySQ("CHITEI", SQ_VOLUME_MAX);
    } else if (now == SCENE_CODE('3', 'A', '0', '1')) {
        SdSetEffect(1, 3, 0x78);
    }
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

// CT3 0x018c98f0: stage 2's BGM rule (HINMIN in 2A / 2B, KYOHU in 0A17 / 0B01; the reverb per room).
// Scene changes only. 2D01 also forgets the special SEs (both tables cleared).
void Branch2BGMProc(int sceneChange)
{
    u_int now = gNowScene;
    u_int bfo = gBfoScene;

    if (sceneChange == 0) {
        return;
    }
    if (bfo == SCENE_CODE_BOOT_BFO) {
        if (now == SCENE_CODE('2', 'D', '0', '2')) {
            SdSetEffect(1, 3, 0x7f);
        } else if (now == SCENE_CODE('2', 'A', '0', '2')) {
            // nothing
        } else if (now == SCENE_CODE('2', 'A', '0', '1')) {
            if (ssTrigCheckEvent(TRIG_EVENT_STAGE2_HINMIN_OFF, 1, 0) == 0) {
                SSPlaySQ("HINMIN", SQ_VOLUME_MAX);
            }
        } else if (now == SCENE_CODE('0', 'A', '1', '7')) {
            if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE2_KYOHU, 1, 0) == 1) {
                SSPlaySQ("KYOHU", SQ_VOLUME_MAX);
            } else {
                SdStopSQAll(1);
                gPlayingStageBGM = STAGE_BGM_NONE;
            }
        } else if (now == SCENE_CODE('0', 'B', '0', '1')) {
            SSPlaySQ("KYOHU", SQ_VOLUME_MAX);
        }
    } else if (now == SCENE_CODE('2', 'A', '0', '2')) {
        if (bfo == SCENE_CODE('2', 'B', '0', '5')) {
            SSStopSQ("HINMIN", 1);
            gPlayingStageBGM = STAGE_BGM_NONE;
        }
    } else if (now == SCENE_CODE('2', 'B', '0', '5')) {
        if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE2_HINMIN_2B05_OFF, 1, 0) != 1) {
            SSPlaySQ("HINMIN", SQ_VOLUME_MAX);
        }
    } else if (now == SCENE_CODE('0', 'A', '1', '7')) {
        if (bfo == SCENE_CODE('2', 'A', '0', '1')) {
            if (gPlayingStageBGM == STAGE_BGM_HINMIN) {
                SSStopSQ("HINMIN", 1);
                gPlayingStageBGM = STAGE_BGM_NONE;
            }
            if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE2_KYOHU, 1, 0) == 1) {
                SSPlaySQ("KYOHU", SQ_VOLUME_MAX);
            }
        }
    } else if (now == SCENE_CODE('2', 'A', '0', '1')) {
        if (bfo == SCENE_CODE('0', 'A', '1', '7')
            && (char)ssTrigCheckEvent(TRIG_EVENT_STAGE2_KYOHU, 1, 0) != 1) {
            SSPlaySQ("HINMIN", SQ_VOLUME_MAX);
        } else if (gPlayingStageBGM == STAGE_BGM_KYOHU) {
            SSStopSQ("KYOHU", 1);
            gPlayingStageBGM = STAGE_BGM_NONE;
        }
    } else if (now == SCENE_CODE('2', 'D', '0', '1')) {
        SdSetEffect(1, 0, 0);
        memset(gPlaySESp1, 0, sizeof gSavePlaySESp1);
        memset(gSavePlaySESp1, 0, sizeof gSavePlaySESp1);
    } else if (now == SCENE_CODE('2', 'D', '0', '3')) {
        SdSetEffect(1, 3, 0x7f);
    } else if (now == SCENE_CODE('0', 'A', '0', '1')) {
        SdSetEffect(1, 0, 0);
    }
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

// CT3 0x018c9d40: stage 1's BGM rule (PIANO). Per frame: in 1A04, once event 0x3b1 is set, the piano
// starts silent and fades to 90. On a scene change from boot: 1B01 starts it at 100, 1A11 (and 1A02
// after event 0x3b8) at 70 unless event 0x415 is set; from any other scene 1B04 / 1B01 only fade it.
void Branch1BGMProc(int sceneChange)
{
    if (sceneChange == 0) {
        if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE1_SQ_ALLOW, 1, 0) == 1
            && gNowScene == SCENE_CODE('1', 'A', '0', '4') && gPlayingStageBGM != STAGE_BGM_STAGE1) {
            SSPlaySQ("PIANO", 0);
            SdVolumeSQ("PIANO", 0x5a, 1);
        }
    } else if (gBfoScene == SCENE_CODE_BOOT_BFO) {
        if (gNowScene == SCENE_CODE('1', 'B', '0', '1')) {
            SSPlaySQ("PIANO", 100);
        } else if (gNowScene == SCENE_CODE('1', 'A', '1', '1')) {
            if ((char)ssTrigCheckEvent(TRIG_EVENT_STAGE1_PIANO_OFF, 1, 0) != 1) {
                SSPlaySQ("PIANO", 0x46);
            }
        } else if (gNowScene == SCENE_CODE('1', 'A', '0', '2')
                   && (char)ssTrigCheckEvent(TRIG_EVENT_STAGE1_PIANO_1A02, 1, 0) == 1
                   && (char)ssTrigCheckEvent(TRIG_EVENT_STAGE1_PIANO_OFF, 1, 0) != 1) {
            SSPlaySQ("PIANO", 0x46);
        }
    } else if (gNowScene == SCENE_CODE('1', 'B', '0', '4')) {
        SdVolumeSQ("PIANO", SQ_VOLUME_MAX, 1);
    } else if (gNowScene == SCENE_CODE('1', 'B', '0', '1')) {
        SdVolumeSQ("PIANO", 0x6e, 1);
    }
}

// CT3 0x018c9f10: the frame's BGM rules (stage = trigger variable 0x1d, 1..5):
//  1. the SE mute event stops every SE; in a boss battle (event 0x9a) the boss loops and battle ADS stop
//     when Alyssa falls (0x49), and the T_BREAK stream's countdown (0x294 or 900 frames, by variables
//     0x53 / 0x54) ends in the stage's battle ADS (22.ADS, 23.ADS on stage 5) unless 0x96 or 0x49 is set;
//  2. the room: when the current bank is a loaded single bank past the boot ones, its file name's first
//     four letters are the scene code; the stage's BranchNBGMProc runs per frame (same room) or once
//     (sceneChange 1), and gBfoScene follows;
//  3. outside a card load: the chase BGM (TUISEKI on stages 1-2, TUISEKI2 on 3-4) plays while event
//     0xc9 or 0xd4 holds and stops CHASE_STOP_DELAY frames after, the stage BGM coming back; in a boss
//     battle 0x51 / 0x49 stop the boss loops (0x49 once plays Alyssa's fall).
void SSBGMProc(void)
{
    int stage = ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0);

    if ((char)ssTrigCheckEvent(TRIG_EVENT_SE_MUTE, 1, 0) == 1) {
        SdStopSEAll();
    }

    // 1. the boss battle's loops and the T_BREAK countdown
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) != 0) {
        if ((char)ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) == 1) {
            SSStopSE("T_0010", 0xf, 0);
            SSStopSE(SE_BANK_COMMON, 0x4b, 0);
            // the ROM passes gPlayADS as SSStopADS's first argument, which it does not read
            if (memcmp(gPlayADS, "22", 2) == 0) {
                SSStopADS(0, 1);
            } else if (memcmp(gPlayADS, "29", 2) == 0) {
                SSStopADS(0, 1);
            } else if (memcmp(gPlayADS, "30", 2) == 0) {
                SSStopADS(0, 1);
            }
        }
        if (gPlayTbreak == 0) {
            if (memcmp(gPlayADS + 1, "_BREAK", 6) == 0) {
                if (ssTrigGetVariable(TRIG_VAR_TBREAK_SHORT, 1, 0) == 1) {
                    gPlayTbreak = TBREAK_FRAMES_SHORT;
                    SSStopSE("T_0010", 0xf, 0);
                    SSStopSE("E_BAL", 0x1b, 0);
                    SSStopSE(SE_BANK_COMMON, 0x4b, 0);
                } else if (ssTrigGetVariable(TRIG_VAR_TBREAK_LONG, 1, 0) == 1) {
                    gPlayTbreak = TBREAK_FRAMES_LONG;
                    SSStopSE("T_0010", 0xf, 0);
                    SSStopSE("E_BAL", 0x1b, 0);
                    SSStopSE(SE_BANK_COMMON, 0x4b, 0);
                }
            }
        } else if (--gPlayTbreak < 1) {
            if (memcmp(gPlayADS + 1, "_BREAK", 6) == 0) {
                SdStopStream(0);
            }
            if ((char)ssTrigCheckEvent(TRIG_EVENT_96, 1, 0) != 1
                && (char)ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) != 1) {
                if (pBossInfo[BOSS_INFO_BAL_LOOP] != 0) {
                    SSPlaySE("E_BAL", 0x1b, 1);
                }
                if (stage == 5) {
                    SSPlayADS("23.ADS", SD_VOLUME_MAX, 1);
                } else if (stage >= 1 && stage <= 4) {
                    SSPlayADS("22.ADS", SD_VOLUME_MAX, 1);
                }
            }
            gPlayTbreak = 0;
        }
    }

    // 2. the room's rule
    if (idCurrentScene > BANK_BOOT_LAST && BankInfo[idCurrentScene].count == 1
        && BankInfo[idCurrentScene].loaded == 1) {
        int sceneChange;

        memcpy(&gNowScene, BankInfo[idCurrentScene].name, sizeof gNowScene);
        sceneChange = gNowScene != gBfoScene;
        switch (stage) {
        case 5: Branch5BGMProc(sceneChange); break;
        case 4: Branch4BGMProc(sceneChange); break;
        case 3: Branch3BGMProc(sceneChange); break;
        case 2: Branch2BGMProc(sceneChange); break;
        case 1: Branch1BGMProc(sceneChange); break;
        default: break;
        }
        gBfoScene = gNowScene;
    }

    // 3. the chase BGM
    if (MC_NOW_LOADING != 0) {
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) != 0) {
        if (ssTrigCheckEvent(TRIG_EVENT_BOSS_SE_STOP, 1, 0) != 0) {
            SSStopSE("T_0010", 0xf, 1);
            SSStopSE(SE_BANK_COMMON, 0x4b, 1);
            gPlayTbreak = 0;
        }
        if (ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) != 0) {
            if (gBossAlysDead == 0) {
                SSPlaySE(SE_BANK_COMMON, 0x3b, 1);
                gBossAlysDead = 1;
            }
            SSStopSE("T_0010", 0xf, 1);
            SSStopSE(SE_BANK_COMMON, 0x4b, 1);
            gPlayTbreak = 0;
        }
        return;
    }
    if ((char)ssTrigCheckEvent(TRIG_EVENT_96, 1, 0) == 1
        || (char)ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) == 1) {
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_CHASE, 1, 0) != 0 || ssTrigCheckEvent(TRIG_EVENT_CHASE_D4, 1, 0) != 0) {
        if (ssTrigGetVariable(TRIG_VAR_SOUND_4D, 1, 0) == 1) {
            gPlayingChaseBGM = 0;
        } else if (gPlayingChaseBGM == 0) {
            if (stage == 4 || stage == 3) {
                SSPlaySQ2("TUISEKI2", SQ_VOLUME_MAX);
                gPlayingChaseBGM = stage;
            } else if (stage == 2 || stage == 1) {
                SSPlaySQ2("TUISEKI", SQ_VOLUME_MAX);
                gPlayingChaseBGM = stage;
            }
            gChaseStopDelay = CHASE_STOP_DELAY;
        }
        return;
    }
    if (--gChaseStopDelay >= 1 || gPlayingChaseBGM == 0) {
        return;
    }
    if (stage == 5) {
        if (ssTrigGetVariable(TRIG_VAR_SOUND_4D, 1, 0) != 1 && gPlayingStageBGM == STAGE_BGM_OMOIDE) {
            SSPlaySQ("OMOIDE", SQ_VOLUME_MAX);
        }
    } else if (stage == 4) {
        SSStopSQ("TUISEKI2", 1);
        gPlayingChaseBGM = 0;
        if (ssTrigGetVariable(TRIG_VAR_SOUND_4D, 1, 0) != 1) {
            if (gPlayingStageBGM == STAGE_BGM_OMOIDE) {
                SSPlaySQ("OMOIDE", SQ_VOLUME_MAX);
            } else if (gPlayingStageBGM == STAGE_BGM_FUAN) {
                SSPlaySQ("FUAN", SQ_VOLUME_MAX);
            } else if (gPlayingStageBGM == STAGE_BGM_KOJO) {
                SSPlaySQ("KOJO", SQ_VOLUME_MAX);
            } else if (gPlayingStageBGM == STAGE_BGM_TSUNORU) {
                SSPlaySQ("TSUNORU", SQ_VOLUME_MAX);
            }
        }
    } else if (stage == 3) {
        SSStopSQ("TUISEKI2", 1);
        gPlayingChaseBGM = 0;
        if (ssTrigGetVariable(TRIG_VAR_SOUND_4D, 1, 0) != 1) {
            if (gPlayingStageBGM == STAGE_BGM_HAKABA) {
                SSPlaySQ("HAKABA", SQ_VOLUME_MAX);
            } else if (gPlayingStageBGM == STAGE_BGM_CHITEI) {
                SSPlaySQ("CHITEI", SQ_VOLUME_MAX);
            }
        }
    } else if (stage == 2) {
        SSStopSQ("TUISEKI", 1);
        gPlayingChaseBGM = 0;
        ssTrigGetVariable(TRIG_VAR_SOUND_4D, 1, 0);   // read, unused (the ROM's)
    } else if (stage == 1) {
        int vol;

        SSStopSQ("TUISEKI", 1);
        gPlayingChaseBGM = 0;
        // the piano comes back at the volume trigger variable 0x26 holds (the ROM passes it straight on)
        if (ssTrigGetVariable(TRIG_VAR_SOUND_4D, 1, 0) != 1
            && (vol = ssTrigGetVariable(TRIG_VAR_PIANO_VOLUME, 1, 0)) > 0) {
            SSPlaySQ("PIANO", vol);
        }
    }
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

// CT3 0x018ca940: a demo starts (VistaVisionStart): keep the special SEs, mark the playing ones, and from
// stage 2 on turn the playing SQ down for the demo -- or, when it is the chase BGM, stop it outright and
// set trigger variable 0x4d.
void MidiControl_DemoStart(void)
{
    int i;

    gDemoStartFlg = 1;
    memcpy(gSavePlaySESp1, gPlaySESp1, sizeof gSavePlaySESp1);
    for (i = 0; i < PLAY_SE_SP1_COUNT; i++) {
        if (gPlaySESp1[i].active != 0) {
            gPlaySESp1[i].state = PLAY_SE_SP1_STATE_DEMO;
        }
    }
    if (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0) > 1 && gSQLd[0].name[0] != '\0') {
        if (gPlayingChaseBGM == 0) {
            strcpy(gPlayMidiDemo, gSQLd[0].name);
            gPlayMidiDemoVol = gSQLd[0].playVol;
            SSVolumeSQ(gSQLd[0].name, DEMO_SQ_VOLUME, 1);
        } else {
            gPlayMidiDemo[0] = '\0';
            gPlayingChaseBGM = 0;
            ssTrigSetVariable(TRIG_VAR_SOUND_4D, 1, 1, 0);
            SSVolumeSQ(gSQLd[0].name, 0, 1);
            SdStopSQ(gSQLd[0].name);
        }
    }
}

// CT3 0x018caa40: the SQ back to the volume SSSaveVolumeMidi kept (straight to the driver: no stage rule).
void SSReturnVolumeMidi(void)
{
    if (gBgmControlSaveFile[0] != '\0' && gBgmControlSaveVol != 0) {
        SdVolumeSQ(gSQLd[0].name, gBgmControlSaveVol, 1);
    }
}

// CT3 0x018caa80: keep the playing SQ's name and target volume, then set it to vol (fading when fade).
void SSSaveVolumeMidi(int vol, int fade)
{
    if (gSQLd[0].name[0] == '\0') {
        gBgmControlSaveFile[0] = '\0';
        gBgmControlSaveVol = 0;
        return;
    }
    strcpy(gBgmControlSaveFile, gSQLd[0].name);
    gBgmControlSaveVol = gSQLd[0].vol;
    SdVolumeSQ(gSQLd[0].name, vol, fade != 0 ? 1 : 0);
}

// CT3 0x018cab10: undo SSSaveVolumeSE: the special SEs as they were, and every SE with a saved volume
// gets it back -- a positional SE (objId) through posVol, a loop SE (SD_SE_SAVED_RESTART) replayed
// from the start when its bank lists it in sLoopSE, then stopped as the old handle.
void SSReturnVolumeSE(void)
{
    int i;

    memcpy(gPlaySESp1, gSavePlaySESp1, sizeof gSavePlaySESp1);
    for (i = 0; i < SD_SE_COUNT; i++) {
        SD_SE *se = &gSdSE[i];
        u_int saved;

        if (se->handle == 0) {
            continue;
        }
        saved = se->savedVol;
        if ((saved & SD_SE_SAVED) == 0) {
            continue;
        }
        if (se->objId != 0) {
            se->posVol = (int)(saved & ~SD_SE_SAVED);
        } else if ((saved & ~SD_SE_SAVED) == SD_SE_SAVED_RESTART) {
            if (SSReturnVolumeSE_CheckLoopSE(se->jls, se->seNo) != 0) {
                SdReqPlaySE(se->jls, se->seNo, se->reqArg2);
            }
            SdStopSE(se->handle);
        } else {
            SdSetParamSE(se->handle, SD_SE_PARAM_VOLUME, (int)(saved & 0x7f));
        }
        se->savedVol = 0;
    }
}

// CT3 0x018cac30: 1 when SE seNo of the bank loaded in slot jls loops (sLoopSE). The ROM scans gSELd and
// sLoopSE with one counter: when the first loaded bank in slot jls is not in sLoopSE, the counter leaves
// the bank scan at 12 and ends the slot scan too, so the answer is 0 without looking at later slots
// (kept: hazards.md).
int SSReturnVolumeSE_CheckLoopSE(int jls, int seNo)
{
    char upper[32];
    int i;
    int k;

    for (i = 0; i < SD_SE_LD_COUNT; i++) {
        if (gSELd[i].used == 0 || gSELd[i].jls != jls) {
            continue;
        }
        strcpy(upper, gSELd[i].name);
        strupr(upper);
        for (i = 0; i < LOOP_SE_BANKS; i++) {
            if (strcmp(upper, sLoopSE[i].bank) == 0) {
                for (k = 0; k < 10; k++) {
                    if (sLoopSE[i].seNo[k] == LOOP_SE_END) {
                        return 0;
                    }
                    if (sLoopSE[i].seNo[k] == seNo) {
                        return 1;
                    }
                }
                return 0;
            }
        }
    }
    return 0;
}

// CT3 0x018cad60: silence every SE and remember its volume (SSReturnVolumeSE undoes it): the special SEs
// are kept and stopped; a plain SE goes to volume 0; a positional one (objId: its callback sets the
// volume every frame) keeps posVol and gets 1.
void SSSaveVolumeSE(void)
{
    int i;

    memcpy(gSavePlaySESp1, gPlaySESp1, sizeof gSavePlaySESp1);
    for (i = 0; i < PLAY_SE_SP1_COUNT; i++) {
        if (gPlaySESp1[i].active != 0) {
            SdStopSE(gPlaySESp1[i].handle);
            gPlaySESp1[i].stopped = 1;
        }
    }
    for (i = 0; i < SD_SE_COUNT; i++) {
        SD_SE *se = &gSdSE[i];

        if (se->handle == 0) {
            continue;
        }
        if (se->objId == 0) {
            se->savedVol = (u_int)se->vol | SD_SE_SAVED;
            SdSetParamSE(se->handle, SD_SE_PARAM_VOLUME, 0);
        } else {
            se->savedVol = (u_int)se->posVol | SD_SE_SAVED;
            se->posVol = 1;
        }
    }
}

// CT3 0x018cae50: the enemy bank's SEs over a pause. on: the loaded enemy bank (the last of E_RYU* /
// E_ONO* in gSELd) decides which plain SEs stop; a positional one saves posVol and gets 1. off: a
// positional SE gets its saved posVol back; every enemy SE forgets the save.
void SSEnemyPauseOnOff(int on)
{
    int bank = ENEMY_PAUSE_NONE;
    int i;

    for (i = 0; i < SD_SE_LD_COUNT; i++) {
        if (gSELd[i].used == 0) {
            continue;
        }
        if (memcmp(gSELd[i].name, SE_BANK_E_RYU, 5) == 0) {
            bank = ENEMY_PAUSE_RYU;
        } else if (memcmp(gSELd[i].name, SE_BANK_E_ONO, 5) == 0) {
            bank = ENEMY_PAUSE_ONO;
        }
    }
    for (i = 0; i < SD_SE_COUNT; i++) {
        SD_SE *se = &gSdSE[i];

        if (se->handle == 0 || se->jls != SE_JLS_ENEMY) {
            continue;
        }
        if (on == 0) {
            if ((se->savedVol & SD_SE_SAVED) != 0) {
                if (se->objId != 0) {
                    se->posVol = (int)(se->savedVol & ~SD_SE_SAVED);
                }
                se->savedVol = 0;
            }
        } else if (se->objId != 0) {
            se->savedVol = (u_int)se->posVol | SD_SE_SAVED;
            se->posVol = 1;
        } else if (bank == ENEMY_PAUSE_RYU) {
            if (se->seNo == 1 || se->seNo == 0) {
                SdStopSE(se->handle);
            }
        } else if (bank == ENEMY_PAUSE_ONO && se->seNo == 3) {
            SdStopSE(se->handle);
        }
    }
}

// CT3 0x018cb530: stop a special SE of name's bank; during a demo the saved table is the live one, so
// the stop goes through it.
int SSStopSESp1(const char *name, int a1, int a2)
{
    int ret;

    if (gDemoStartFlg != 0) {
        memcpy(gPlaySESp1, gSavePlaySESp1, sizeof gSavePlaySESp1);
    }
    ret = SdStopSESp1(GetJLSbySEFileName(name), a1, a2);
    if (gDemoStartFlg != 0) {
        memcpy(gSavePlaySESp1, gPlaySESp1, sizeof gSavePlaySESp1);
    }
    return ret;
}

// CT3 0x018cb8f0: stream ADS file name (gFileADS, case-insensitive) at vol; "02." streams play 60 lower.
// Nothing while a demo is skipped; an unknown name plays nothing. The ROM copies name into 16 bytes.
void SSPlayADS(const char *name, int vol, int a2)
{
    char upper[16];
    int i;

    if (DemoSkip != 0) {
        return;
    }
    strcpy(upper, name);
    strupr(upper);
    for (i = 0; strcmp(upper, gFileADS[i].name) != 0; i++) {
        if (gFileADS[i].name[0] == '\0') {
            return;
        }
    }
    if (memcmp(name, ADS_QUIET_PREFIX, 3) == 0) {
        vol -= ADS_QUIET_CUT;
        if (vol < 0) {
            vol = 0;
        }
    }
    SdPlayStream(name, vol, a2, gFileADS[i].loop);
}

// CT3 0x018cb060: the frame's sound work: event 0x96 stops the special SEs; trigger variable 0x58 turns
// the music down (1: to DEMO_SQ_VOLUME, 2: to silence, fading) and every SE off, 0 brings both back;
// then SSBGMProc.
void SSProc(void)
{
    int quiet;

    if (ssTrigCheckEvent(TRIG_EVENT_96, 1, 0) != 0) {
        SdStopSEAllSp1();
    }
    quiet = ssTrigGetVariable(TRIG_VAR_SOUND_QUIET, 1, 0);
    if (gBgmControl == 1) {
        if (quiet == 0) {
            SSReturnVolumeMidi();
            SSReturnVolumeSE();
            gBgmControl = 0;
        } else if (gSQLd[0].name[0] != '\0') {
            if (gSQLd[0].vol != (quiet == SOUND_QUIET_SILENT ? 0 : DEMO_SQ_VOLUME)) {
                if (quiet == SOUND_QUIET_SILENT) {
                    SSSaveVolumeMidi(0, 1);
                } else {
                    SSSaveVolumeMidi(DEMO_SQ_VOLUME, 0);
                }
            }
        }
    } else if (quiet > 0) {
        // the ROM writes SSSaveVolumeMidi(0) here; its second argument register still holds the 1 it
        // compared gBgmControl with
        if (quiet == SOUND_QUIET_SILENT) {
            SSSaveVolumeMidi(0, 1);
        } else {
            SSSaveVolumeMidi(DEMO_SQ_VOLUME, 0);
        }
        SSSaveVolumeSE();
        gBgmControl = 1;
    }
    SSBGMProc();
}

// CT3 0x018cb5b0: play a special SE of name's bank, panned from the player every frame
// (SetPositionalParam_Alys_Sp1); during a demo the saved table is the live one. a1..a8 go to the driver
// as they are (its TU names them).
void SSPlaySESp1(const char *name, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    if (DemoSkip != 0) {
        return;
    }
    if (gDemoStartFlg != 0) {
        memcpy(gPlaySESp1, gSavePlaySESp1, sizeof gSavePlaySESp1);
    }
    SdReqPlaySESp1(GetJLSbySEFileName(name), a1, a2, a3, a4, a5, a6, SetPositionalParam_Alys_Sp1, a7, a8);
    if (gDemoStartFlg != 0) {
        memcpy(gSavePlaySESp1, gPlaySESp1, sizeof gSavePlaySESp1);
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

// CT3 0x018cba00: a room's own SE banks, loaded one at a time when the current bank is past the boot ones:
// 2A01 R_2A01 + T_0020, 2A02 R_2A02 + T_0020, 5A02 R_5A04.
void SceneSELoad(void)
{
    const char *room;

    if (idCurrentScene <= BANK_BOOT_LAST || BankInfo[idCurrentScene].count == 0) {
        return;
    }
    room = BankInfo[idCurrentScene].name;
    if (memcmp(room, "2A01", 4) == 0) {
        SSLoadSEEndWait(SSLoadSE("R_2A01", 0));
        SSLoadSEEndWait(SSLoadSE("T_0020", 0));
    } else if (memcmp(room, "2A02", 4) == 0) {
        SSLoadSEEndWait(SSLoadSE("R_2A02", 0));
        SSLoadSEEndWait(SSLoadSE("T_0020", 0));
    } else if (memcmp(room, "5A02", 4) == 0) {
        SSLoadSEEndWait(SSLoadSE("R_5A04", 0));
    }
}

// CT3 0x018cbb70: a stage begins: request the common banks and the stage's enemy bank (1 E_HAMMER,
// 2 E_RYUSAN, 3 E_ONO, 4 E_SWO, 5 E_BAL; no wait), and forget the battle and demo state.
void BranchSELoad(void)
{
    int stage = ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0);

    SSLoadSE(SE_BANK_COMMON, 0);
    SSLoadSE("Z_REI", 0);
    if (stage == 5) {
        SSLoadSE("E_BAL", 0);
    } else if (stage == 4) {
        SSLoadSE("E_SWO", 0);
    } else if (stage == 3) {
        SSLoadSE("E_ONO", 0);
    } else if (stage == 2) {
        SSLoadSE("E_RYUSAN", 0);
    } else if (stage == 1) {
        SSLoadSE("E_HAMMER", 0);
    }
    ssTrigSetVariable(TRIG_VAR_SOUND_4D, 0, 1, 0);
    gPlayTbreak = 0;
    gBossAlysDead = 0;
    gPlayMidiDemo[0] = '\0';
    gPlayMidiDemoVol = 0;
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
    ssTrigSetVariable(TRIG_VAR_SOUND_QUIET, 0, 1, 0);
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
