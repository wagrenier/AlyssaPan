// game/sound.h -- CT3 game/sound.c: the game's sound layer over the Sd driver (SE banks, SQ music, the
// stage BGM state). Slice 1 written 2026-11-09; the prototypes of functions still stubbed come from their
// call sites.
#pragma once

#include "game/SdMasterVolumeSetSE.h"
#include "game/ct3_types.h"
#include "sdk/scetypes.h"

// gFileSE's rows: an SE bank file is picked by its name's first two letters ("C_0000" -> "C_").
typedef struct SE_FILE {        // 0xc bytes on the EE; layout: host (the prefix is a host pointer)
    const char *prefix;         // +0x0 "C_", "E_", "Z_", "R_", "T_", "B_"; nullptr ends the table
    int jls;                    // +0x4 the Sd driver's SE slot (4..8): SdReqLoadSE / SdReqPlaySE / SdStopSESDT
    u_int spuAddr;              // +0x8 where the bank goes in SPU2 RAM (R_ and B_ share 0x1562b0)
} SE_FILE;

// gFileSQ's rows: the SQ (sequenced music) files by name. The three words after the name go to SdPlaySQ
// as they are (SdLoadSQProc reads them, 2026-11-10).
typedef struct SQ_FILE {        // 0x14 bytes on the EE; layout: host (the name is a host pointer)
    const char *name;           // +0x0 "PIANO", "TUISEKI", ...; nullptr ends the table
    u_int spuAddr;              // +0x4 the .BD's SPU2 address (0x20000 in every row)
    int effectType;             // +0x8 the reverb type SdSetEffect gets (4, or 5 for PIANO / MISSHORT / MISLONG)
    int effectDepth;            // +0xc its depth (0x2d..0x7f)
    int unused10;               // +0x10 0x64 (0x28 for FUAN): SSPlaySQ2 does not read it
} SQ_FILE;

// gFileADS's rows: the ADS (streamed audio) files SSPlayADS plays.
typedef struct ADS_FILE {       // 0x14 bytes; layout: ee
    char name[0x10];            // +0x0 "01.ADS", ...; "" ends the table
    int loop;                   // +0x10 SdPlayStream's fourth argument (game-inferred: a loop flag)
} ADS_FILE;
static_assert(sizeof(ADS_FILE) == 0x14, "ADS_FILE");

// sLoopSE's rows (sound.c static): a bank's looping SE numbers.
#define LOOP_SE_END 0xff
typedef struct LOOP_SE {        // 0x2c bytes on the EE; layout: host (the name is a host pointer)
    const char *bank;           // +0x0 the bank file ("C_0000"); nullptr ends the table
    int seNo[10];               // +0x4 ends with LOOP_SE_END
} LOOP_SE;

#define ADS_QUIET_PREFIX "02."  // SSPlayADS: streams named 02.* play this much below the asked volume
#define ADS_QUIET_CUT 0x3c

// SSEnemyPauseOnOff: the enemy banks (E_ in slot 6) whose SEs a pause stops, and which SE numbers.
#define SE_JLS_ENEMY 6
#define SE_BANK_E_RYU "E_RYU"   // 0x01936598 (@1827): stops SE 0 and 1
#define SE_BANK_E_ONO "E_ONO"   // 0x019365a0 (@1828): stops SE 3
enum ENEMY_PAUSE_BANK { ENEMY_PAUSE_NONE = 0, ENEMY_PAUSE_RYU = 2, ENEMY_PAUSE_ONO = 3 };
// The trigger actions' and room-in SE banks.
#define SE_BANK_COMMON "C_0000"         // 0x019364a8: the resident common SEs (sound objects play these)
#define SE_BANK_T0000 "T_0000"          // 0x01936548: SSActPlaySound's seNo 8
#define BANK_TYPE_2 2                   // SSGetPositional: objId < 1 means the PARTS_HASH_SOUND_ORIGIN part
#define BANK_TYPE_NO_ROOM_SOUND 3       // SSAutoStartSoundRoomIn starts nothing in banks of this type
#define PARTS_HASH_SOUND_ORIGIN 0x3161c34c2ull // asFindModelPartsID special-cases it (header +0x10)
#define POSITIONAL_FALLOFF_DEFAULT 3    // SetPositionalParam_Alys / _Cam: volume lost per POSITIONAL_UNIT
#define POSITIONAL_UNIT 55.0f
// A scene code: the 4-char room name ("5A02") as the little-endian word gNowScene / gBfoScene hold.
#define SCENE_CODE(a, b, c, d) ((u_int)(a) | (u_int)(b) << 8 | (u_int)(c) << 16 | (u_int)(d) << 24)
// The BGM rules' trigger events (ssGrpPortalCheck.c owns them; named from BranchNBGMProc's use only).
#define TRIG_EVENT_STAGE5_OMOIDE 0x3ac         // 5A01: OMOIDE plays
#define TRIG_EVENT_STAGE5_OMOIDE_STOP 0x3e1    // 5A02: OMOIDE stops
#define TRIG_EVENT_STAGE1_PIANO_1A02 0x3b8     // 1A02 plays the piano on entry
#define TRIG_EVENT_STAGE1_PIANO_OFF 0x415      // 1A11 / 1A02: no piano
#define TRIG_EVENT_STAGE4_TSUNORU_OFF 0x5c6    // 4A01 / 4A02: no TSUNORU; 4A04: it stops
#define TRIG_EVENT_STAGE2_HINMIN_OFF 0x48f     // 2A01 from boot: no HINMIN
#define TRIG_EVENT_STAGE2_HINMIN_2B05_OFF 0x3c9 // 2B05: no HINMIN
#define TRIG_EVENT_STAGE2_KYOHU 0x593          // 0A17: KYOHU plays (from boot: else every SQ stops)
#define TRIG_EVENT_STAGE3_HAKABA 0x3e4         // 3D03: HAKABA plays
#define TRIG_EVENT_STAGE3_HAKABA_OFF 0x3af     // 3D03: no HAKABA
#define TRIG_EVENT_STAGE3_HAKABA_OFF2 0x3d6    // 3D03: no HAKABA (entering: CHITEI stops)
#define TRIG_EVENT_STAGE3_CHITEI 0x3d1         // 3A01: CHITEI plays
// SSBGMProc's events and variables (named from that use only; ssGrpPortalCheck.c owns them).
#define TRIG_EVENT_BOSS_BATTLE 0x9a            // the boss loops (T_0010 15, C_0000 0x4b) and T_BREAK run
#define TRIG_EVENT_BOSS_ALYS_DEAD 0x49         // Alyssa falls in the battle: loops stop, C_0000 0x3b once
#define TRIG_EVENT_BOSS_SE_STOP 0x51           // the boss loops stop
#define TRIG_EVENT_96 0x96                     // holds the chase BGM and the battle ADS back
#define TRIG_EVENT_CHASE 0xc9                  // a pursuer is on: the chase BGM plays
#define TRIG_EVENT_CHASE_D4 0xd4               // likewise
#define TRIG_VAR_TBREAK_SHORT 0x53             // == 1: T_BREAK counts TBREAK_FRAMES_SHORT
#define TRIG_VAR_TBREAK_LONG 0x54              // == 1: T_BREAK counts TBREAK_FRAMES_LONG
#define TRIG_VAR_PIANO_VOLUME 0x26             // stage 1: the piano's volume after a chase (0: none)
#define TBREAK_FRAMES_SHORT 0x294
#define TBREAK_FRAMES_LONG 900
#define CHASE_STOP_DELAY 0xf                   // frames the chase BGM outlasts its events
#define BOSS_INFO_BAL_LOOP 0x93                // pBossInfo byte: != 0 -> E_BAL 0x1b resumes after T_BREAK
#define BANK_BOOT_LAST 5                       // SSBGMProc reads scene codes only from banks past this
void SSBGMProc(void);                                                                    // CT3 0x018c9f10
void SSProc(void);                                                                       // CT3 0x018cb060
void SSPlaySESp1(const char *name, int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8); // CT3 0x018cb5b0
void SceneSELoad(void);                                                                  // CT3 0x018cba00
void BranchSELoad(void);                                                                 // CT3 0x018cbb70
void Branch3BGMProc(int sceneChange);                                                    // CT3 0x018c9430
void Branch4BGMProc(int sceneChange);                                                    // CT3 0x018c90b0
void Branch2BGMProc(int sceneChange);                                                    // CT3 0x018c98f0
void Branch5BGMProc(int sceneChange);                                                    // CT3 0x018c8d80
void Branch1BGMProc(int sceneChange);                                                    // CT3 0x018c9d40
#define DEMO_SQ_VOLUME 0x28     // MidiControl_DemoStart turns the playing SQ down to this

#define SE_FILE_COUNT 6        // GetJLSbySEFileName's scan bound (the table's live rows)
#define SE_JLS_DEFAULT 4        // GetJLSbySEFileName's answer when no prefix matches
extern const ADS_FILE gFileADS[31];            /* data 0x0193a020 */
SOUND_OBJ *SSFindSoundObj(int bank, int id);                                             // CT3 0x018c7fb0
void SSAutoStartSoundRoomIn(int bank);                                                   // CT3 0x018c8020
void SSLoadSEfromLoadPoint(u_char *image);                                               // CT3 0x018c8140
void SSActVolume(int bank, int id);                                                      // CT3 0x018c81c0
int SSActStopSound(int bank, int id);                                                    // CT3 0x018c81d0
int SSActPlaySound(int bank, int id);                                                    // CT3 0x018c8230
int SSActLoadBankSE(const char *name);                                                   // CT3 0x018c8330
void SetPositionalParam_Alys_Sp1(int handle);                                            // CT3 0x018c8340
void SetPositionalParam_Alys(int handle);                                                // CT3 0x018c8400
void SetPositionalParam_Cam(int handle);                                                 // CT3 0x018c84c0
int SSGetPositional(int bank, int objId, int *vol, int *pan, int fromPlayer, int falloff); // CT3 0x018c8580
void SSGetPositionalTo0_64_127(float dist, float angle, int *vol, int *pan, int falloff); // CT3 0x018c8810
void MidiControl_DemoStart(void);                                                        // CT3 0x018ca940
void SSReturnVolumeMidi(void);                                                           // CT3 0x018caa40
void SSSaveVolumeMidi(int vol, int fade);                                                // CT3 0x018caa80
void SSReturnVolumeSE(void);                                                             // CT3 0x018cab10
int SSReturnVolumeSE_CheckLoopSE(int jls, int seNo);                                     // CT3 0x018cac30
void SSSaveVolumeSE(void);                                                               // CT3 0x018cad60
void SSEnemyPauseOnOff(int on);                                                          // CT3 0x018cae50
int SSStopSESp1(const char *name, int a1, int a2);                                       // CT3 0x018cb530
void SSPlayADS(const char *name, int vol, int a2);                                       // CT3 0x018cb8f0
#define SQ_VOLUME_MAX 0x7f

// gPlayingStageBGM: which stage BGM SSSoundEnable restarts after a movie (game-inferred from that use).
enum STAGE_BGM {
    STAGE_BGM_NONE = 0,
    STAGE_BGM_STAGE1 = 1,       // Branch1SSPlaySQ lets the SQ play: stage 1 records 1 for any SQ
    STAGE_BGM_HINMIN = 2,
    STAGE_BGM_KYOHU = 3,
    STAGE_BGM_CHITEI = 4,
    STAGE_BGM_HAKABA = 5,
    STAGE_BGM_TSUNORU = 6,
    STAGE_BGM_KOJO = 7,
    STAGE_BGM_FUAN = 8,
    STAGE_BGM_OMOIDE = 9,
    STAGE_BGM_OTHER = 99,       // BranchNSSPlaySQ: not this stage's BGM: play it, record nothing
};

// SSSetSoundMode's modes (the option menu's sound settings).
enum SOUND_MODE {
    SOUND_MODE_ALL = 0,         // 1, 2 and 3, a vsync apart
    SOUND_MODE_STEREO = 1,      // SOUND_STEREO_MONAURAL_BUFF -> gStereo
    SOUND_MODE_BGM_VOLUME = 2,  // BGM_MASTER_VOLUME_BUFF (0..20) -> SQ and ADS master volume
    SOUND_MODE_SE_VOLUME = 3,   // SE_MASTER_VOLUME_BUFF (0..20) -> SE master volume
};
#define SOUND_MENU_VOLUME_STEPS 0x14   // the menu's volume buffers run 0..20, the driver's 0..255
#define SD_VOLUME_MAX 0xff

// ssTrig variables and events sound.c touches (ssGrpPortalCheck.c owns them; names game-inferred).
#define TRIG_VAR_STAGE 0x1d            // SSSoundEnable: 2..5 pick the stage BGM set
#define TRIG_VAR_SOUND_4D 0x4d         // cleared by SSInitClearValiable
#define TRIG_VAR_SOUND_QUIET 0x58      // SSProc: 1 music down to DEMO_SQ_VOLUME and SEs off, 2 music silent, 0 back
#define SOUND_QUIET_SILENT 2
#define TRIG_EVENT_SE_MUTE 0xb7        // SSPlaySE plays nothing while this event is set
// Branch1SSPlaySQ's events (stage 1's piano / miss SQs): 0x3b1 set lets them play; else 0x3ba set and a
// current bank other than "1B04" refuses, and 0x3bf or 0x3bd set refuses.
#define TRIG_EVENT_STAGE1_SQ_ALLOW 0x3b1
#define TRIG_EVENT_STAGE1_SQ_1B04 0x3ba
#define TRIG_EVENT_STAGE1_SQ_BLOCK_3BD 0x3bd
#define TRIG_EVENT_STAGE1_SQ_BLOCK_3BF 0x3bf

extern const SE_FILE gFileSE[7];               /* data 0x01939ea0 */
extern const SQ_FILE gFileSQ[14];              /* data 0x01939f00 */
extern int gDemoStartFlg;                      /* sbss 0x01942d48 */
extern int gBgmControlSaveVol;                 /* sbss 0x01942d4c */
extern char gBgmControlSaveFile[0x38];         /* sbss 0x01942d50 */
extern int gBgmControl;                        /* sbss 0x01942d88 */
extern int gPlayMidiDemoVol;                   /* sbss 0x01942d8c */
extern char gPlayMidiDemo[0x38];               /* sbss 0x01942d90 */
extern int gBossAlysDead;                      /* sbss 0x01942dc8 */
extern int gPlayTbreak;                        /* sbss 0x01942dcc */
extern int gChaseStopDelay;                   /* sbss 0x01942dd0: frames the chase BGM outlasts its events */
extern int gPlayingChaseBGM;                   /* sbss 0x01942dd4 */
extern int gPlayingStageBGM;                   /* sbss 0x01942dd8: enum STAGE_BGM */
extern PLAY_SE_SP1 gSavePlaySESp1[PLAY_SE_SP1_COUNT]; /* sbss 0x01942de0: gPlaySESp1 kept over a movie */

float SSGetPositional3Dist(const float *from, const float *to);                           // CT3 0x018c88d0
float SSGetPositional3(float facing, const float *from, const float *to);                // CT3 0x018c8920
float SSGetPositional2(float facing, const float *from, const float *to, float *dist);   // CT3 0x018c8980
float SSGetAngle(float x, float z);                                                      // CT3 0x018c8a40
void MidiControl_DemoEnd(void);                                                          // CT3 0x018ca8c0
int Branch5SSStopSQ(const char *name);                                                   // CT3 0x018c8cf0
int Branch5SSPlaySQ(const char *name);                                                   // CT3 0x018c8d30
int Branch4SSStopSQ(const char *name);                                                   // CT3 0x018c8f90
int Branch4SSPlaySQ(const char *name);                                                   // CT3 0x018c8fd0
int Branch3SSStopSQ(const char *name);                                                   // CT3 0x018c93a0
int Branch3SSPlaySQ(const char *name);                                                   // CT3 0x018c93b0
int Branch2SSStopSQ(const char *name);                                                   // CT3 0x018c9830
int Branch2SSPlaySQ(const char *name);                                                   // CT3 0x018c9870
int Branch1SSStopSQ(const char *name);                                                   // CT3 0x018c9bf0
int Branch1SSPlaySQ(const char *name);                                                   // CT3 0x018c9c00
void SSVolumeSQ(const char *name, int vol, int a2);                                      // CT3 0x018cb170
void SSStopSQ(const char *name, int muteFirst);                                          // CT3 0x018cb1b0
void SSPlaySQ(const char *name, int vol);                                                // CT3 0x018cb360
void SSSoundEnable(void);                                                                // CT3 0x018c8af0
void SSSoundDisable(void);                                                               // CT3 0x018c8cc0
int SSPlaySQ2(const char *name, int vol);                                                // CT3 0x018cb2b0
int SSVolumeAdjustSQ(const char *name, int vol);                                         // CT3 0x018cb460
int SSFindSQSPU2(const char *name);                                                      // CT3 0x018cb470
void SSStopSE(const char *name, int a1, int a2);                                         // CT3 0x018cb4f0
int SSPlaySE(const char *name, int a1, int a2);                                          // CT3 0x018cb6a0
int GetJLSbySEFileName(const char *name);                                                // CT3 0x018cb730
void SSPurgeSE(const char *name);                                                        // CT3 0x018cb7c0
void SSLoadSEEndWait(int handle);                                                        // CT3 0x018cb7d0
int SSLoadSE(const char *name, int a1);                                                  // CT3 0x018cb7e0
void SSVolumeADS(int a0, int vol);                                                       // CT3 0x018cb8d0
void SSStopADS(int a0, int a1);                                                          // CT3 0x018cb8e0
void Sound_data_load_set(void);                                                          // CT3 0x018cbc90
void Sound_system_end(void);                                                             // CT3 0x018cbca0
void Sound_system_init(void);                                                            // CT3 0x018cbd00
void Sound_FirstInitialization(void);                                                    // CT3 0x018cbda0
void SSInit(void);                                                                       // CT3 0x018cbdb0
void SSInitClearValiable(void);                                                          // CT3 0x018cbdc0
void SSSetSoundMode(int mode);                                                           // CT3 0x018cbe30
