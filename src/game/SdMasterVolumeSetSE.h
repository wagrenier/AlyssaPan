// game/SdMasterVolumeSetSE.h -- CT3 game/SdMasterVolumeSetSE.c (the EE side of the sound driver: SE, SQ,
// ADS stream). The declarations other TUs call (game/sound.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "game/CpDivideEECom.h"
#include "sdk/scetypes.h"

#include <stddef.h>

// ---- the driver's records (read by game/sound.c's slice 2, 2026-11-09; fields named from that use) -----
// layout: ee -- ints and chars only; the driver TU names the rest.

// SD_SE.callback / PLAY_SE_SP1.update: a positional SE's per-frame update, called with the SE's handle
// (SdPlaySEProc, SdPlaySESpProc; sound.c's SetPositionalParam_*).
typedef void (*SD_SE_UPDATE)(int handle);

// SD_SE.state: SdPlaySEProc moves a row through these.
enum SD_SE_STATE {
    SD_SE_STATE_REQUEST = 0,    // SdReqPlaySE's row: NextPlaySE picks the oldest, SdPlaySE keys it on
    SD_SE_STATE_KEYED = 1,      // the key-on went out: waits for its channel in gSdSEKeyOnCH
    SD_SE_STATE_PLAYING = 2,    // sounding: freed when its channel goes quiet; callback every frame
};

// gSdSE's rows: one playing SE. SdFindPlaySE(handle) answers its row; SdSetParamSE writes its fields.
// layout: host -- the ROM's +0x50 is an EE code word (the update callback); here it is a host function
// pointer (decided 2026-11-10, the driver TU: open item aa). EE 0x54 bytes; offsets up to +0x40 hold.
typedef struct SD_SE {
    int handle;                 // +0x00 0 = free; SdStopSE / SdSetParamSE take it
    int jls;                    // +0x04 the bank's slot (gFileSE: 6 = the E_ enemy bank)
    int seNo;                   // +0x08 the SE number in its bank (SdReqPlaySE's second argument)
    int reqArg2;                // +0x0c SdReqPlaySE's third argument: >= SD_SE_RESIDENT survives a bank
                                //       reload; SD_SE_REQ_SUPPLY_CH takes over the channel of another such SE
    int state;                  // +0x10 SD_SE_STATE
    int chBits;                 // +0x14 the voice channel it plays on, as a bit (SdSupplyCHSE)
    int ch;                     // +0x18 the same channel as a number (0..21), the play word's top byte
    int posMode;                // +0x1c SdSetParamSE 0x100000: SSGetPositional's last argument (Sp1)
    int objId;                  // +0x20 SdSetParamSE 0x80000: the sound object (SOUND_OBJ.id); != 0:
                                //       positional, its callback sets vol / pan every frame
    int bank;                   // +0x24 SdSetParamSE 0x40000: the sound object's bank
    u_int savedVol;             // +0x28 SSSaveVolumeSE: SD_SE_SAVED | the volume to restore
    u_int paramFlags;           // +0x2c SdSetParamSE ors in its mask's bits; SdPlaySE / SdPlaySEUpdate
                                //       send and clear the VOLUME / PAN / 30 bits (SD_SE_PARAM_SEND)
    int unk30;                  // +0x30 SdSetParamSE 0x20000000 (999 from SdReqPlaySE); the play word's
                                //       third byte -- pitch or priority, the IOP side decides
    int vol;                    // +0x34 SdSetParamSE SD_SE_PARAM_VOLUME (999 from SdReqPlaySE)
    int pan;                    // +0x38 SdSetParamSE SD_SE_PARAM_PAN (999 from SdReqPlaySE)
    int posVol;                 // +0x3c SdSetParamSE 0x10000000: the volume in tenths (0 = full, 1 = muted)
    char name[0x10];            // +0x40 SdSetParamSE 0x20000 (strncpy, 15): with SD_SE_FLAG_WAIT_BANK, the
                                //       bank the SE waits for
    SD_SE_UPDATE callback;      // +0x50 SdSetParamSE 0x10000: the per-frame positional update
} SD_SE;
static_assert(offsetof(SD_SE, chBits) == 0x14, "SD_SE.chBits");
static_assert(offsetof(SD_SE, objId) == 0x20, "SD_SE.objId");
static_assert(offsetof(SD_SE, savedVol) == 0x28, "SD_SE.savedVol");
static_assert(offsetof(SD_SE, vol) == 0x34, "SD_SE.vol");
static_assert(offsetof(SD_SE, posVol) == 0x3c, "SD_SE.posVol");
static_assert(offsetof(SD_SE, name) == 0x40, "SD_SE.name");

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
#define SD_SE_FLAG_WAIT_BANK 0x8000u    // paramFlags: play only once the slot holds the bank named in `name`
                                        // (until then the request waits; without it, no bank frees the row)
#define SD_SE_PARAM_SEND 0xe0000000u    // paramFlags: VOLUME | PAN | 30, the parts a play word carries
#define SD_SE_PARAM_UPDATE 0xe0010000u  // paramFlags: SdPlaySEProc sends an update for these
#define SD_SE_REQ_SUPPLY_CH 0x3e0       // reqArg2: take the channel of the SE already playing with it
#define SD_SE_CH_COUNT 0x16             // SPU2 voices the SEs share (bits 0..21)
#define SD_SE_CH_ALL 0xffffff           // SdReqPlaySE with no free row stops every channel
#define SD_SE_VOL_MAX 0x7f

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

// gSQLd's rows: the two SQ (sequenced music) players; row 0 is the one sound.c reads. SdPlaySQ fills
// row 0, or row 1 while row 0 still plays (row 0 is then told to stop: SdLoadSQProc's swap, unread).
typedef struct SD_SQ_LD {       // 0x1c0 bytes
    char dir[0x38];             // +0x00 SdPlaySQ's directory (upper case)
    char name[0x38];            // +0x38 the playing SQ ("HINMIN"); '\0' = none
    int arg5;                   // +0x70 SdPlaySQ's sixth argument (SdLoadSQProc reads it)
    int arg6;                   // +0x74 its seventh
    u_int arg2;                 // +0x78 its third
    u_int iopSq;                // +0x7c an IOP heap block (SdStopSQAllNow frees it second)
    u_int iopBd;                // +0x80 an IOP heap block (freed first)
    int unk84;                  // +0x84 SdPlaySQ clears +0x84..+0x90
    int unk88;                  // +0x88
    int unk8c;                  // +0x8c
    int unk90;                  // +0x90
    int step;                   // +0x94 SD_SQ_LD_STEP_*: SdLoadSQProc's step; cleared with the name
    u_char unk98[0x4];          // +0x98
    int playVol;                // +0x9c the volume it was started at
    u_int flags;                // +0xa0 SD_SQ_FLAG_*
    int arg4;                   // +0xa4 SdPlaySQ's fifth argument
    int unka8;                  // +0xa8 SdPlaySQ clears it
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
static_assert(offsetof(SD_SQ_LD, iopSq) == 0x7c, "SD_SQ_LD.iopSq");
static_assert(offsetof(SD_SQ_LD, step) == 0x94, "SD_SQ_LD.step");
static_assert(offsetof(SD_SQ_LD, arg4) == 0xa4, "SD_SQ_LD.arg4");
#define SD_SQ_LD_COUNT 2
#define SD_SQ_FLAG_STOP 0x1u            // SdStopSQ / SdStopSQAll / SdPlaySQ over a playing SQ
#define SD_SQ_FLAG_VOLUME 0x2u          // SdVolumeSQ: move to `vol`
#define SD_SQ_FLAG_FADE 0x4u            // SdVolumeSQ / SdStopSQAll(fade): gradually
#define SD_SQ_LD_STEP_REQUEST 1         // SdPlaySQ's request
#define SD_SQ_LD_STEP_LOADED 0x19       // from here the SQ is loaded (SdLoadSQEndWaitAll waits below it)
#define SD_SQ_LD_STEP_REPLAY_MAX 0x1c   // SdPlaySQ of the playing SQ below this step only resets its volume

// gPlaySESp1's rows: the special SEs (SdReqPlaySESp1): one SE from a random range replayed at an interval
// (SdPlaySESpProc), kept over a movie or a demo. layout: host -- +0x18 is an EE code word in the ROM (the
// update callback, SD_SE_UPDATE here; 2026-11-10). EE 0x34 bytes; offsets up to +0x14 hold.
typedef struct PLAY_SE_SP1 {
    int active;                 // +0x00 the row's slot (jls); != 0: the row is in use (SdLoadSEProc
                                //       clears it when its slot reloads)
    int seNoBase;               // +0x04 the first SE number of the range
    int seNo;                   // +0x08 SdReqPlaySE's third argument (SD_SE.reqArg2); SdLoadSEProc keeps
                                //       the row over a reload when >= SD_SE_RESIDENT
    int posMode;                // +0x0c -> SD_SE.posMode
    int objId;                  // +0x10 -> SD_SE.objId
    int bank;                   // +0x14 -> SD_SE.bank
    SD_SE_UPDATE update;        // +0x18 -> SD_SE.callback, also called once at each start
    int seNoRange;              // +0x1c SdRnd() % this is added to seNoBase
    int posVol;                 // +0x20 -> SD_SE.posVol; MidiControl_DemoStart sets PLAY_SE_SP1_POS_VOL_DEMO
    int interval;               // +0x24 ga_main_timer ticks between two starts
    int lastTime;               // +0x28 ga_main_timer at the last start
    int handle;                 // +0x2c the gSdSE handle playing it
    int stopped;                // +0x30 SSSaveVolumeSE sets 1 after stopping it (no start while set)
} PLAY_SE_SP1;
static_assert(offsetof(PLAY_SE_SP1, bank) == 0x14, "PLAY_SE_SP1.bank");
#define PLAY_SE_SP1_COUNT 5
#define PLAY_SE_SP1_POS_VOL_DEMO 2      // posVol 2: two tenths of the volume over a demo

extern SD_SE gSdSE[SD_SE_COUNT];                  /* bss 0x01c848c0 */
extern SD_SE_LD gSELd[SD_SE_LD_COUNT];            /* bss 0x01c85400 */
extern SD_SQ_LD gSQLd[SD_SQ_LD_COUNT];            /* bss 0x01943340 */
extern PLAY_SE_SP1 gPlaySESp1[PLAY_SE_SP1_COUNT]; /* bss 0x019436d0 */
// gPlayADS: the ADS stream (SdPlayStream). layout: ee -- ints and chars only.
typedef struct SD_PLAY_ADS {    // 0x90 bytes
    char name[0x38];            // +0x00 the stream's name ("22_BREAK"...)
    int vol;                    // +0x38 SdVolumeStream's volume, before the master scale
    int loop;                   // +0x3c SdPlayStream's fourth argument (gFileADS' loop)
    int arg2;                   // +0x40 SdPlayStream's third
    int stopArg;                // +0x44 SdStopStream's argument (SdPlayADSProc reads it)
    u_char unk48[0x48];         // +0x48
} SD_PLAY_ADS;
static_assert(sizeof(SD_PLAY_ADS) == 0x90, "SD_PLAY_ADS");
static_assert(offsetof(SD_PLAY_ADS, vol) == 0x38, "SD_PLAY_ADS.vol");
static_assert(offsetof(SD_PLAY_ADS, stopArg) == 0x44, "SD_PLAY_ADS.stopArg");

extern SD_PLAY_ADS gPlayADS;                      /* bss 0x019437f0 */
extern int gSdADSStop;                            /* sbss 0x01943880: 1 = SdPlayADSProc stops the stream */
extern int gSdADSPhase;                           /* sbss 0x01943884: SdPlayADSProc's phase; 1 = SdPlayStream's request */
extern int gSQi;                                  /* sbss 0x01942ee4: the gSQLd row SdLoadSQProc steps */
extern void *sqAllocPtr;                          /* sbss 0x01943330: the SQ's SdAlloc block */
extern int gSdEffectType[2];                      /* bss 0x01943898: per SPU2 core, the reverb type set */
extern u_int gSdEffectDepth[2];                   /* bss 0x01943890: per SPU2 core, depth << 7 | 0x7f */
extern int gUseSoundAlloc;                        /* sbss 0x019438a0: 1 while SdAlloc's one block is out */
extern int gLdi;                                  /* sbss 0x019437dc: the gSELd row SdLoadSEProc steps */
extern int gSELdHandleCounter;                    /* sbss 0x019437e0: SdReqLoadSE's last handle */
extern u_int gSEHeapBD;                           /* sbss 0x019437e4: the IOP staging block for .BD pieces */
extern int gExecDriver_2;                         /* sbss 0x019437e8: frames before the next SPU2 transfer */
extern int gSdSEHandleCounter;                    /* sbss 0x019437d4: SdReqPlaySE's last handle */
extern u_int gSdSEKeyOnCH;                        /* sbss 0x019437d8: the channels sounding (the driver's reply) */
extern int gSEi;                                  /* sdata 0x0193a4ec (-1): the gSdSE row SdPlaySE keyed last */

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
void SdReqPlaySESp1(int jls, int seNoBase, int seNo, int seNoRange, int interval, int objId, int bank,
                    SD_SE_UPDATE update, int posVol, int posMode);                       // CT3 0x018cc350
int IsPlaySEReady(const char *name);                                                     // CT3 0x018cd820
u_int SdSetParamSE(int handle, u_int mask, ...);                                          // CT3 0x018cd190
void SdPlayStream(const char *name, int vol, int a2, int loop);                          // CT3 0x018cde40
int SdReqPlaySE(int jls, int seNo, int reqArg2);                                               // CT3 0x018cd3c0
void SdPurgeSE(const char *name);                                                        // CT3 0x018cd930
void SdLoadSEEndWait(int handle);                                                        // CT3 0x018cda30
int SdLoadSEEnd(int handle);                                                             // CT3 0x018cda90
void SdLoadSEEndWaitAll(void);                                                           // CT3 0x018cdb10
void SdReqLoadSECancel(int ldi);                                                         // CT3 0x018cfd30
int SdSupplyCHSE(SD_SE *se);                                                             // CT3 0x018ccc40
int SdReqLoadSE(const char *dir, const char *name, int jls, u_int spuAddr, int stopPlaying);     // CT3 0x018cdba0
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
void SdChangeStereoMono(int stereo);                                                     // CT3 0x018d1810
void *SdAlloc(int size, int align, const char *name);                                    // CT3 0x018d17d0
void SdFree(void **mem);                                                                 // CT3 0x018d17a0
void SdLoadSQEndWaitAll(void);                                                           // CT3 0x018ce640
void IopLoadCapcomSound(const char *device);                                             // CT3 0x018d1860 (IopLoadModules' last call)
void SdSetParamSESDT(int jls, int seNo, int reqArg2, u_int mask, ...);                   // CT3 0x018cce40
// SdFindLoadSEInfo's first argument: what the second one is.
#define SD_FIND_LOAD_SE_HANDLE 1        // an SdReqLoadSE handle (0 finds a free row)
#define SD_FIND_LOAD_SE_NAME 2          // a bank name, any case
#define SD_FIND_LOAD_SE_JLS 3           // a slot
int SdFindLoadSEInfo(int by, ...);                                                       // CT3 0x018cd600
extern u_int gSQHeapBD;                           /* sbss 0x019436c0: the IOP staging block for the SQ's .BD */

// sbss 0x01943888: != 0 while an ADS stream is being opened; FlProc holds its reads back meanwhile.
extern int gADSOpenSync;
