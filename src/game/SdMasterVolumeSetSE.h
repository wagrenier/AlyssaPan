// game/SdMasterVolumeSetSE.h -- CT3 game/SdMasterVolumeSetSE.c (the EE side of the sound driver: SE, SQ,
// ADS stream). The declarations other TUs call (game/sound.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "game/CpDivideEECom.h"
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

// The bank's register record (SD_SE_LD +0x100 in the ROM): SdLoadSEProc's last step hands it to the driver
// with SND_CMD_SE_REGISTER | jls. layout: ee -- the IOP side reads it (+0x94 unwritten).
typedef struct SD_SE_REGISTER {
    char path[0x80];            // +0x00 gDeviceName + dir + name, '\' separated, upper case
    u_int iopHd;                // +0x80 the .HD's IOP heap block
    u_int iopBd;                // +0x84 where the .BD went (gSEHeapBD, the staging block it went through)
    u_int spuAddr;              // +0x88 the .BD's SPU2 address
    int zero;                   // +0x8c 0
    u_int iopSdt;               // +0x90 the .SDT's IOP heap block
    int unk94;                  // +0x94
    int bdSize;                 // +0x98
    u_char unk9c[0x24];         // +0x9c
} SD_SE_REGISTER;
static_assert(sizeof(SD_SE_REGISTER) == 0xc0, "SD_SE_REGISTER");
static_assert(offsetof(SD_SE_REGISTER, iopHd) == 0x80, "SD_SE_REGISTER.iopHd");
static_assert(offsetof(SD_SE_REGISTER, bdSize) == 0x98, "SD_SE_REGISTER.bdSize");

// SD_SE_LD.step: SdLoadSEProc's steps, one a frame. A bank is three files, NAME.SDT (the SE table, 16-byte
// rows), NAME.HD and NAME.BD (the VAG header and body); a packed .SDT carries its .HD behind it
// (FL_PACK_INFO.partSize), so the HD steps are skipped. The .BD reaches SPU2 RAM in 0x32000-byte pieces
// through gSEHeapBD.
enum SD_SE_LD_STEP {
    SD_SE_LD_IDLE = 0,
    SD_SE_LD_OPEN_SDT = 1,      // SdReqLoadSE's request: stop the bank's SEs (stopPlaying), open .SDT
    SD_SE_LD_READ_SDT = 2,      // the size known: SdAlloc "SD1", read
    SD_SE_LD_SEND_SDT = 3,      // read: stamp the jls into every row, to the IOP heap
    SD_SE_LD_OPEN_HD = 4,
    SD_SE_LD_READ_HD = 5,       // SdAlloc "SD2"
    SD_SE_LD_SEND_HD = 6,
    SD_SE_LD_OPEN_BD = 7,
    SD_SE_LD_READ_BD = 8,       // SdAlloc "SD3"
    SD_SE_LD_BD_READ_WAIT = 9,
    SD_SE_LD_BD_TRANS = 10,     // the next piece to gSEHeapBD
    SD_SE_LD_BD_SPU = 11,       // the driver moves it to SPU2 RAM (SND_CMD_TRANS_SPU2), at most once a
                                // second (gExecDriver_2 counts 60 frames down between two)
    SD_SE_LD_BD_SPU_WAIT = 12,  // done: the next piece, or REGISTER
    SD_SE_LD_REGISTER = 13,     // the register record to the driver
    SD_SE_LD_DONE_14 = 14,      // 14, 15 and 0x14 finish the load (the ROM's switch lists all three)
    SD_SE_LD_DONE_15 = 15,
    SD_SE_LD_DONE = 0x14,
    SD_SE_LD_ERROR = 0x15,      // drop everything, free the slot
    SD_SE_LD_ERROR_16 = 0x16,
};
#define SD_SE_LD_FLAG_LOADED 0x1u   // SD_SE_LD.flags: set by the done steps
#define SD_SE_LD_STOP_PLAYING 1     // SD_SE_LD.stopPlaying: stop the slot's SEs before loading over it

// gSELd's rows: one SE bank load (SdReqLoadSE). layout: host -- `buf` holds SdAlloc's host pointer; the
// IOP-bound records (`spuCom`, `reg`) keep their EE layout. EE offsets in the comments (0x1c0 bytes).
typedef struct SD_SE_LD {
    int handle;                 // +0x00 the load's handle (SdReqLoadSE's answer); 0 = free slot
    char dir[0x38];             // +0x04 FlReqOpen's directory (upper case)
    char name[0x38];            // +0x3c the bank's file name ("E_RYUSAN"), upper case, no extension
    u_int spuAddr;              // +0x74 the .BD's SPU2 address (SdReqLoadSE's fourth argument)
    int jls;                    // +0x78 its slot
    u_int iopSdt;               // +0x7c the .SDT's IOP heap block (0 = none)
    u_int iopHd;                // +0x80 the .HD's (inside iopSdt for a packed bank)
    u_int iopBd;                // +0x84 gSEHeapBD once the .BD goes out
    int sdtSize;                // +0x88
    int hdSize;                 // +0x8c
    int bdSize;                 // +0x90
    int step;                   // +0x94 SD_SE_LD_STEP; 0 once loaded (SdLoadSEEnd)
    int fd;                     // +0x98 the open FlReqOpen handle, 0 = none
    u_int flags;                // +0x9c SD_SE_LD_FLAG_*
    void *buf;                  // +0xa0 the SdAlloc block the file is read into
    int stopPlaying;            // +0xa4 SdReqLoadSE's fifth argument
    u_char unka8[0x18];         // +0xa8
    SND_JSL_COM spuCom;         // +0xc0 SND_CMD_TRANS_SPU2: arg[0] staging block, [1] SPU2 address, [2] size
    u_char unke0[0x20];         // +0xe0
    SD_SE_REGISTER reg;         // +0x100
} SD_SE_LD;
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
    u_char unkb0[0x10];         // +0xb0
    SND_JSL_COM volCom;         // +0xc0 SdVolumeSetSQ's record: unk00 the player, arg[0] 0xff, arg[1] volume
    u_char unke0[0xe0];         // +0xe0
} SD_SQ_LD;
static_assert(sizeof(SD_SQ_LD) == 0x1c0, "SD_SQ_LD");
static_assert(offsetof(SD_SQ_LD, name) == 0x38, "SD_SQ_LD.name");
static_assert(offsetof(SD_SQ_LD, playVol) == 0x9c, "SD_SQ_LD.playVol");
static_assert(offsetof(SD_SQ_LD, vol) == 0xac, "SD_SQ_LD.vol");
static_assert(offsetof(SD_SQ_LD, volCom) == 0xc0, "SD_SQ_LD.volCom");
#define SD_SQ_LD_COUNT 2

// gPlaySESp1's rows: the special SEs (SdStopSESp1), kept over a movie or a demo.
typedef struct PLAY_SE_SP1 {    // 0x34 bytes
    int active;                 // +0x00 the row's slot (jls); != 0: the row is in use (SdLoadSEProc
                                //       clears it when its slot reloads)
    int unk04;                  // +0x04
    int seNo;                   // +0x08 SdLoadSEProc keeps the row over a reload when >= 1000
    u_char unk0c[0x14];         // +0x0c
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
extern int gLdi;                                  /* sbss 0x019437dc: the gSELd row SdLoadSEProc steps */
extern int gSELdHandleCounter;                    /* sbss 0x019437e0: SdReqLoadSE's last handle */
extern u_int gSEHeapBD;                           /* sbss 0x019437e4: the IOP staging block for .BD pieces */
extern int gExecDriver_2;                         /* sbss 0x019437e8: frames before the next SPU2 transfer */

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
void SdVolumeSetSQ(int player, int vol);                                                 // CT3 0x018cdff0
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
