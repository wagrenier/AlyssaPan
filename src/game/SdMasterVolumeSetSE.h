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
    u_char unk10[0xc];          // +0x10
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
    int used;                   // +0x00 != 0: the slot holds a bank
    u_char unk04[0x38];         // +0x04
    char name[0x3c];            // +0x3c the bank's file name ("E_RYUSAN"), as loaded
    int jls;                    // +0x78 its slot
    u_char unk7c[0x144];        // +0x7c
} SD_SE_LD;
static_assert(sizeof(SD_SE_LD) == 0x1c0, "SD_SE_LD");
static_assert(offsetof(SD_SE_LD, name) == 0x3c, "SD_SE_LD.name");
static_assert(offsetof(SD_SE_LD, jls) == 0x78, "SD_SE_LD.jls");
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
extern char gPlayADS[0x90];                       /* bss 0x019437f0: the playing ADS stream, its name first */

void SdMasterVolumeSetSE(int vol);                                                       // CT3 0x018cbf90
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
void SdLoadSEEndWaitAll(void);                                                           // CT3 0x018cdb10
int SdReqLoadSE(const char *dir, const char *name, int jls, u_int spuAddr, int a4);     // CT3 0x018cdba0
void SdStopStream(int a0);                                                               // CT3 0x018cddd0
void SdVolumeStream(int vol);                                                            // CT3 0x018cdde0
void SdMasterVolSetADS(int vol);                                                         // CT3 0x018cdf90
void SdMasterVolumeSetSQ(int vol);                                                       // CT3 0x018ce080
void SdVolumeSQ(const char *name, int vol, int a2);                                      // CT3 0x018ce0d0
void SdStopSQAllNow(void);                                                               // CT3 0x018ce1e0
void SdStopSQAll(int fade);                                                              // CT3 0x018ce310
void SdStopSQ(const char *name);                                                         // CT3 0x018ce380
int SdPlaySQ(const char *dir, const char *name, u_int a2, int vol, int a4, int a5, int a6); // CT3 0x018ce440
void SdSoundEnable(void);                                                                // CT3 0x018d13d0
void SdSoundDisable(void);                                                               // CT3 0x018d14d0
void SdSetEffect(int a0, int a1, int a2);                                                // CT3 0x018d1600
void SdChangeStereoMono(void);                                                           // CT3 0x018d1810
void *SdAlloc(int size, int align, const char *name);                                    // CT3 0x018d17d0
void SdFree(void **mem);                                                                 // CT3 0x018d17a0
void SdLoadSQEndWaitAll(void);                                                           // CT3 0x018ce640
void IopLoadCapcomSound(const char *device);                                              // CT3 IopLoadModules' last call (game/ArtoolDebugOpen.c)

// sbss 0x01943888: != 0 while an ADS stream is being opened; FlProc holds its reads back meanwhile.
extern int gADSOpenSync;
