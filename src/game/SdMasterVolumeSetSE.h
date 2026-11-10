// game/SdMasterVolumeSetSE.h -- CT3 game/SdMasterVolumeSetSE.c (the EE side of the sound driver: SE, SQ,
// ADS stream). The declarations other TUs call (game/sound.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// ---- the driver's records (read by game/sound.c's slice 2, 2026-11-09; fields named from that use) -----
// layout: ee -- ints and chars only; the driver TU names the rest.

// gSdSE's rows: one playing SE. SdFindPlaySE(handle) answers its row; SdSetParamSE writes its fields.
typedef struct SD_SE {          // 0x54 bytes
    int handle;                 // +0x00 0 = free; SdStopSE / SdSetParamSE take it
    int jls;                    // +0x04 the bank's slot (gFileSE: 6 = the E_ enemy bank)
    int seNo;                   // +0x08 the SE number in its bank (SdReqPlaySE's second argument)
    int reqArg2;                // +0x0c SdReqPlaySE's third argument
    int unk10;                  // +0x10
    int chBits;                 // +0x14 the voice channels it plays on (SdStopSE hands them to SdStopSECH)
    int unk18;                  // +0x18
    int posMode;                // +0x1c SdSetParamSE 0x100000: SSGetPositional's last argument (Sp1)
    int objId;                  // +0x20 SdSetParamSE 0x80000: the sound object (SOUND_OBJ.id); != 0:
                                //       positional, its callback sets vol / pan every frame
    int bank;                   // +0x24 SdSetParamSE 0x40000: the sound object's bank
    u_int savedVol;             // +0x28 SSSaveVolumeSE: SD_SE_SAVED | the volume to restore
    u_int paramFlags;           // +0x2c SdSetParamSE ors in its mask's unhandled bits
    int unk30;                  // +0x30 SdSetParamSE 0x20000000; 999 (SD_SE_SAVED_RESTART) -> no update
    int vol;                    // +0x34 SdSetParamSE SD_SE_PARAM_VOLUME
    int pan;                    // +0x38 SdSetParamSE SD_SE_PARAM_PAN
    int posVol;                 // +0x3c SdSetParamSE 0x10000000: a positional SE's volume (1 = muted)
    char name[0x10];            // +0x40 SdSetParamSE 0x20000 (strncpy, 15)
    u_int callback;             // +0x50 SdSetParamSE 0x10000: the per-frame positional update, an EE
                                //       code address (the driver TU decides the host form)
} SD_SE;
static_assert(sizeof(SD_SE) == 0x54, "SD_SE");
static_assert(offsetof(SD_SE, chBits) == 0x14, "SD_SE.chBits");
static_assert(offsetof(SD_SE, objId) == 0x20, "SD_SE.objId");
static_assert(offsetof(SD_SE, callback) == 0x50, "SD_SE.callback");
static_assert(offsetof(SD_SE, savedVol) == 0x28, "SD_SE.savedVol");
static_assert(offsetof(SD_SE, vol) == 0x34, "SD_SE.vol");
static_assert(offsetof(SD_SE, posVol) == 0x3c, "SD_SE.posVol");

#define SD_SE_COUNT 0x22
#define SD_SE_SAVED 0x80000000u         // savedVol: a volume was saved
#define SD_SE_SAVED_RESTART 999         // savedVol's value: replay the SE on restore (a loop SE)
// SdSetParamSE's mask bits: each set bit takes the next argument, in this order.
#define SD_SE_PARAM_VOLUME 0x80000000u  // -> vol
#define SD_SE_PARAM_PAN 0x40000000u     // -> pan
#define SD_SE_PARAM_30 0x20000000u      // -> unk30
#define SD_SE_PARAM_POS_VOL 0x10000000u // -> posVol
#define SD_SE_PARAM_POS_MODE 0x100000u  // -> posMode
#define SD_SE_PARAM_OBJ_ID 0x80000u     // -> objId
#define SD_SE_PARAM_BANK 0x40000u       // -> bank
#define SD_SE_PARAM_NAME 0x20000u       // -> name
#define SD_SE_PARAM_CALLBACK 0x10000u   // -> callback

// gSELd's rows: one SE bank load (SdReqLoadSE).
typedef struct SD_SE_LD {       // 0x1c0 bytes
    int handle;                 // +0x00 the load's handle (SdReqLoadSE's answer); 0 = free slot
    u_char unk04[0x38];         // +0x04
    char name[0x3c];            // +0x3c the bank's file name ("E_RYUSAN"), as loaded
    int jls;                    // +0x78 its slot
    u_char unk7c[0x18];         // +0x7c
    int loading;                // +0x94 != 0 while the bank is still being loaded (SdLoadSEEnd)
    u_char unk98[0x128];        // +0x98
} SD_SE_LD;
static_assert(sizeof(SD_SE_LD) == 0x1c0, "SD_SE_LD");
static_assert(offsetof(SD_SE_LD, name) == 0x3c, "SD_SE_LD.name");
static_assert(offsetof(SD_SE_LD, jls) == 0x78, "SD_SE_LD.jls");
static_assert(offsetof(SD_SE_LD, loading) == 0x94, "SD_SE_LD.loading");
#define SD_SE_LD_COUNT 10

// gSQLd's rows: the two SQ (sequenced music) players; row 0 is the one sound.c reads.
typedef struct SD_SQ_LD {       // 0x1c0 bytes
    u_char unk00[0x38];         // +0x00
    char name[0x38];            // +0x38 the playing SQ ("HINMIN"); '\0' = none
    u_char unk70[0x24];         // +0x70
    int unk94;                  // +0x94 cleared with the name by SdStopSQAll
    u_char unk98[0x4];          // +0x98
    int playVol;                // +0x9c the volume it was started at
    u_int flags;                // +0xa0 SdVolumeSQ: | 2 (volume), | 4 (fade); SdStopSQAll: | 1 (stop)
    u_char unka4[0x8];          // +0xa4
    int vol;                    // +0xac SdVolumeSQ's target volume
    u_char unkb0[0x110];        // +0xb0
} SD_SQ_LD;
static_assert(sizeof(SD_SQ_LD) == 0x1c0, "SD_SQ_LD");
static_assert(offsetof(SD_SQ_LD, name) == 0x38, "SD_SQ_LD.name");
static_assert(offsetof(SD_SQ_LD, playVol) == 0x9c, "SD_SQ_LD.playVol");
static_assert(offsetof(SD_SQ_LD, vol) == 0xac, "SD_SQ_LD.vol");
#define SD_SQ_LD_COUNT 2

// gPlaySESp1's rows: the special SEs (SdStopSESp1), kept over a movie or a demo.
typedef struct PLAY_SE_SP1 {    // 0x34 bytes
    int active;                 // +0x00 != 0: the row is in use
    u_char unk04[0x1c];         // +0x04
    int state;                  // +0x20 MidiControl_DemoStart sets PLAY_SE_SP1_STATE_DEMO
    u_char unk24[0x8];          // +0x24
    int handle;                 // +0x2c the gSdSE handle playing it
    int stopped;                // +0x30 SSSaveVolumeSE sets 1 after stopping it
} PLAY_SE_SP1;
static_assert(sizeof(PLAY_SE_SP1) == 0x34, "PLAY_SE_SP1");
static_assert(offsetof(PLAY_SE_SP1, state) == 0x20, "PLAY_SE_SP1.state");
static_assert(offsetof(PLAY_SE_SP1, handle) == 0x2c, "PLAY_SE_SP1.handle");
#define PLAY_SE_SP1_COUNT 5
#define PLAY_SE_SP1_STATE_DEMO 2

extern SD_SE gSdSE[SD_SE_COUNT];                  /* bss 0x01c848c0 */
extern SD_SE_LD gSELd[SD_SE_LD_COUNT];            /* bss 0x01c85400 */
extern SD_SQ_LD gSQLd[SD_SQ_LD_COUNT];            /* bss 0x01943340 */
extern PLAY_SE_SP1 gPlaySESp1[PLAY_SE_SP1_COUNT]; /* bss 0x019436d0 */
// gPlayADS: the ADS stream (SdPlayStream). layout: ee -- ints and chars only.
typedef struct SD_PLAY_ADS {    // 0x90 bytes
    char name[0x38];            // +0x00 the stream's name ("22_BREAK"...)
    int vol;                    // +0x38 SdVolumeStream's volume, before the master scale
    u_char unk3c[0x8];          // +0x3c
    int stopArg;                // +0x44 SdStopStream's argument (SdPlayADSProc reads it)
    u_char unk48[0x48];         // +0x48
} SD_PLAY_ADS;
static_assert(sizeof(SD_PLAY_ADS) == 0x90, "SD_PLAY_ADS");
static_assert(offsetof(SD_PLAY_ADS, vol) == 0x38, "SD_PLAY_ADS.vol");
static_assert(offsetof(SD_PLAY_ADS, stopArg) == 0x44, "SD_PLAY_ADS.stopArg");

extern SD_PLAY_ADS gPlayADS;                      /* bss 0x019437f0 */
extern int gSdADSStop;                            /* sbss 0x01943880: 1 = SdPlayADSProc stops the stream */
extern int gSdEffectType[2];                      /* bss 0x01943898: per SPU2 core, the reverb type set */
extern u_int gSdEffectDepth[2];                   /* bss 0x01943890: per SPU2 core, depth << 7 | 0x7f */
extern int gUseSoundAlloc;                        /* sbss 0x019438a0: 1 while SdAlloc's one block is out */

#define SD_EFFECT_DEPTH_MAX 0x7f              /* SdSetEffect clamps its depth to this */
#define SD_EFFECT_DEPTH_LIMIT 0x3fff          /* SdSetEffectDontSave: a larger depth word becomes 0 */
#define SD_EFFECT_CORE_1 1                    /* core 1 takes no reverb type above 3 */
#define SD_EFFECT_TYPE_CORE1_MAX 3

// SdStopSECH's mode -> the stop command's +0x10 word (meaning on the IOP side unread).
#define SD_SECH_STOP_WORD_0 0x80000000u       /* mode 0 */
#define SD_SECH_STOP_WORD_1 0x4000000du       /* mode 1: SdStopSE */

void SdMasterVolumeSetSE(int vol);                                                       // CT3 0x018cbf90
void SdProc(void);                                                                       // CT3 0x018d1390 (FileSoundServer, once a frame)
void SdStopSEAll(void);                                                                  // CT3 0x018cbfe0
void SdStopSESDT(int jls, int a1, int a2);                                               // CT3 0x018cc040
void SdStopSE(int handle);                                                               // CT3 0x018cc0f0
void SdStopSEAllSp1(void);                                                               // CT3 0x018cc1b0
int SdStopSESp1(int jls, int a1, int a2);                                                // CT3 0x018cc2a0
SD_SE *SdFindPlaySE(int handle);                                                         // CT3 0x018ccde0
void SdStopSESp1ByHandle(int handle);                                                    // CT3 0x018cc220
int SdReqPlaySESp1(int jls, int a1, int a2, int a3, int a4, int a5, int a6, void (*update)(int handle), int a7,
                   int a8);                                                              // CT3 0x018cc350
int SdSetParamSE(int handle, u_int mask, ...);                                           // CT3 0x018cd190
void SdPlayStream(const char *name, int vol, int a2, int loop);                          // CT3 0x018cde40
int SdReqPlaySE(int jls, int a1, int a2);                                                // CT3 0x018cd3c0
void SdPurgeSE(const char *name);                                                        // CT3 0x018cd930
void SdLoadSEEndWait(int handle);                                                        // CT3 0x018cda30
int SdLoadSEEnd(int handle);                                                             // CT3 0x018cda90
void SdLoadSEEndWaitAll(void);                                                           // CT3 0x018cdb10
int SdReqLoadSE(const char *dir, const char *name, int jls, u_int spuAddr, int a4);     // CT3 0x018cdba0
void SdStopStream(int a0);                                                               // CT3 0x018cddd0
void SdVolumeStream(int vol);                                                            // CT3 0x018cdde0
void SdMasterVolSetADS(int vol);                                                         // CT3 0x018cdf90
int SdIsPlayingStream(void);                                                             // CT3 0x018cdf50
void SdMasterVolumeSetSQ(int vol);                                                       // CT3 0x018ce080
void SdVolumeSQ(const char *name, int vol, int a2);                                      // CT3 0x018ce0d0
void SdStopSQAllNow(void);                                                               // CT3 0x018ce1e0
void SdStopSQAll(int fade);                                                              // CT3 0x018ce310
void SdStopSQ(const char *name);                                                         // CT3 0x018ce380
int SdPlaySQ(const char *dir, const char *name, u_int a2, int vol, int a4, int a5, int a6); // CT3 0x018ce440
void SdSoundEnable(void);                                                                // CT3 0x018d13d0
void SdSoundDisable(void);                                                               // CT3 0x018d14d0
void SdSetEffect(int core, int type, int depth);                                         // CT3 0x018d1600
void SdSetEffectDontSave(int core, int type, int depthWord);                             // CT3 0x018d16a0
u_int SdRnd(void);                                                                       // CT3 0x018d1770
void FSS_Thread_Sync(void);                                                              // CT3 0x018d1b00
void FSS_Thread_Init(void);                                                              // CT3 0x018d1b40
void SdChangeStereoMono(void);                                                           // CT3 0x018d1810
void *SdAlloc(int size, int align, const char *name);                                    // CT3 0x018d17d0
void SdFree(void **mem);                                                                 // CT3 0x018d17a0
void SdLoadSQEndWaitAll(void);                                                           // CT3 0x018ce640
void IopLoadCapcomSound(const char *device);                                              // CT3 IopLoadModules' last call (game/ArtoolDebugOpen.c)

// sbss 0x01943888: != 0 while an ADS stream is being opened; FlProc holds its reads back meanwhile.
extern int gADSOpenSync;
