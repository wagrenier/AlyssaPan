// game/sound.h -- CT3 game/sound.c: the game's sound layer over the Sd driver (SE banks, SQ music, the
// stage BGM state). Slice 1 written 2026-11-09; the prototypes of functions still stubbed come from their
// call sites.
#pragma once

#include "sdk/scetypes.h"

// gFileSE's rows: an SE bank file is picked by its name's first two letters ("C_0000" -> "C_").
typedef struct SE_FILE {        // 0xc bytes on the EE; layout: host (the prefix is a host pointer)
    const char *prefix;         // +0x0 "C_", "E_", "Z_", "R_", "T_", "B_"; nullptr ends the table
    int jls;                    // +0x4 the Sd driver's SE slot (4..8): SdReqLoadSE / SdReqPlaySE / SdStopSESDT
    u_int spuAddr;              // +0x8 where the bank goes in SPU2 RAM (R_ and B_ share 0x1562b0)
} SE_FILE;

// gFileSQ's rows: the SQ (sequenced music) files by name. The three words after the name go to SdPlaySQ
// as they are; their meaning is the Sd driver TU's to name.
typedef struct SQ_FILE {        // 0x14 bytes on the EE; layout: host (the name is a host pointer)
    const char *name;           // +0x0 "PIANO", "TUISEKI", ...; nullptr ends the table
    u_int sdArg2;               // +0x4 SdPlaySQ's third argument (0x20000 in every row)
    int sdArg5;                 // +0x8 SdPlaySQ's sixth argument (4, or 5 for PIANO / MISSHORT / MISLONG)
    int sdArg6;                 // +0xc SdPlaySQ's seventh argument (0x2d..0x7f)
    int unused10;               // +0x10 0x64 (0x28 for FUAN): SSPlaySQ2 does not read it
} SQ_FILE;

#define SE_FILE_COUNT 6         // GetJLSbySEFileName's scan bound (the table's live rows)
#define SE_JLS_DEFAULT 4        // GetJLSbySEFileName's answer when no prefix matches
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
#define TRIG_VAR_SOUND_58 0x58         // cleared by SSInitClearValiable
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
extern int gPlayingChaseBGM;                   /* sbss 0x01942dd4 */
extern int gPlayingStageBGM;                   /* sbss 0x01942dd8: enum STAGE_BGM */
extern u_char gSavePlaySESp1[0x104];           /* sbss 0x01942de0: gPlaySESp1 kept over a movie */

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
