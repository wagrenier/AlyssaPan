// game/CpDivideEECom.h -- CT3 game/CpDivideEECom.c (0x018d1cb0..): the EE <-> IOP status exchange. The
// declarations other TUs call (game/ArtoolDebugOpen.c first, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// gEEStat (bss 0x01943a40, 128 bytes): the status the IOP writes back. Only +0x50 is read so far (FlProc:
// no disc read while it is non-zero). layout: ee.
typedef struct EE_IOP_STAT {
    u_char unk00[0x20];
    int adsActive;          // +0x20 != 0: an ADS stream is playing (SdIsPlayingStream, with +0x50)
    u_char unk24[0x1c];
    u_int sndFlags;         // +0x40 bit 0 (EE_IOP_SND_READY): SdSoundEnable clears it, then waits for the IOP to set it
    u_char unk44[0xc];
    int iopBusy;           // +0x50 != 0: the IOP is using the drive (FlProc waits)
    u_char unk54[0x2c];
} EE_IOP_STAT;
static_assert(sizeof(EE_IOP_STAT) == 0x80, "EE_IOP_STAT is 0x80 bytes");

static_assert(offsetof(EE_IOP_STAT, adsActive) == 0x20, "EE_IOP_STAT.adsActive");
static_assert(offsetof(EE_IOP_STAT, iopBusy) == 0x50, "EE_IOP_STAT.iopBusy");
static_assert(offsetof(EE_IOP_STAT, sndFlags) == 0x40, "EE_IOP_STAT.sndFlags");
#define EE_IOP_SND_READY 0x1u
#define EE_IOP_ADS_OPENED 0x2u  /* sndFlags bit 1: SdPlayADSProc clears it, opens the stream, waits for it */

// The sound driver's command words (CpCall_IOPSndDriver's first argument), named from the EE side's use
// (game/SdMasterVolumeSetSE.c, 2026-11-10); the IOP side (SNDDRV.IRX) is unread.
#define SND_CMD_EFFECT 0x150000u          /* effectJSLCOM: core, type, depth, depth, type changed */
#define SND_CMD_MASTER_VOL_SE 0x240000u   /* gTransJSLCOM +8: the SE master volume */
#define SND_CMD_STOP_SE_CH 0x290000u      /* seStopJSLCOM: +0xc channel bits, +0x10 the stop word */
#define SND_CMD_ADS_VOLUME 0x310000u      /* gTransADS +0x68: L | R << 16 */
#define SND_CMD_SE_REGISTER 0xa0000u      /* | jls: an SE bank loaded (SD_SE_REGISTER) */
#define SND_CMD_TRANS_SPU2 0x120000u      /* CpCall_IOPSndDriver_2: IOP block arg[0] -> SPU2 arg[1], arg[2] bytes */
#define SND_CMD_MASTER_VOL_SQ 0x1b0000u   /* gTransJSLCOM: unk00 0, arg[0] the SQ master volume */
#define SND_CMD_VOLUME_SQ 0x1c0000u       /* SD_SQ_LD.volCom: one player's volume */
#define SND_CMD_SE_KEY_ON_CH 0x60000u     /* no record: the reply word is the channels sounding (gSdSEKeyOnCH) */
#define SND_CMD_SE_PURGE 0xb0000u         /* | jls: drop the slot's bank (SdPurgeSE) */
#define SND_CMD_PLAY_SE 0x260000u         /* SD_SE_LD.spuCom: unk00 jls, arg[0] the play word, arg[1] its values */
#define SND_CMD_STEREO 0x160000u          /* gTransJSLCOM arg[0]: 1 stereo, 0 mono */
#define SND_CMD_STOP_SQ_NOW 0x1a0000u     /* gSQLd[0].volCom: unk00 0, arg[0] 0, arg[1] 0xb */
#define SND_CMD_ADS_STOP 0x2d0000u        /* no record: SdPlayStream over another stream */
#define SND_CMD_SOUND_ON 0x20000u         /* no record: SdSoundEnable, first */
#define SND_CMD_SOUND_OFF 0x10000u        /* no record: SdSoundDisable */
#define SND_CMD_ADS_ENABLE 0x2a0000u      /* gTransADS: SdSoundEnable, then wait for EE_IOP_SND_READY */
#define SND_CMD_ADS_DISABLE 0x320000u     /* gTransADS: SdSoundDisable */
#define SND_CMD_ADS_OPEN 0x2b0000u        /* gTransADS: SdPlayADSProc opens the stream (path, loop, voices) */
#define SND_CMD_ADS_START 0x2c0000u       /* no record: SdPlayADSProc, once the IOP sets EE_IOP_ADS_OPENED */
#define SND_CMD_INIT_08 0x80000u          /* gTransJSLCOM arg[0] 0: IopLoadCapcomSound, after the stream set-up */
#define SND_CMD_INIT_07 0x70000u          /* gTransJSLCOM arg[0] 0, arg[1] 0xffffff (a 24-voice mask?) */
#define SND_CMD_INIT_25 0x250000u         /* gTransJSLCOM arg[0] 0: IopLoadCapcomSound's last */

// A JSL command record (gTransJSLCOM, and the Sd TU's seStopJSLCOM / effectJSLCOM): 8 words, the
// arguments from +8. layout: ee.
typedef struct SND_JSL_COM {
    int unk00;
    int unk04;
    int arg[6];             // +0x08..+0x1c
} SND_JSL_COM;
static_assert(sizeof(SND_JSL_COM) == 0x20, "SND_JSL_COM");

// gTransADS (bss 0x01943b40, 0x88 bytes): the ADS stream's command record (SND_CMD_ADS_ENABLE /
// SND_CMD_ADS_OPEN / SND_CMD_ADS_DISABLE). layout: ee. The IOP side reads the words and is unread, so they
// are named by who writes them: IopLoadCapcomSound the stream buffer set-up, SdPlayADSProc the open.
typedef struct SND_TRANS_ADS {
    u_int mode;             // +0x00 SdPlayADSProc: SND_ADS_MODE
    u_int unk04;            // +0x04 SdPlayADSProc: 0
    char path[0x40];        // +0x08 SdPlayStream: gDeviceName + DATA\SOUND\ADS(2)\ + the name
    u_int loop;             // +0x48 SdPlayADSProc: gPlayADS.loop != 0
    u_int open4c;           // +0x4c SdPlayADSProc: 0x5100
    u_int open50;           // +0x50 SdPlayADSProc: 0x10100
    u_int buf54;            // +0x54 IopLoadCapcomSound: 0xb000
    u_int buf58;            // +0x58 IopLoadCapcomSound: 0x5800
    u_int one5c;            // +0x5c both: 1
    u_int voiceL;           // +0x60 SdPlayADSProc: 0x16 (voices 22 / 23, above the SEs' 22 channels)
    u_int voiceR;           // +0x64 SdPlayADSProc: 0x17
    u_int vol;              // +0x68 the scaled volume, both channels (v | v << 16)
    u_int unk6c;            // +0x6c IopLoadCapcomSound: 0
    u_int buf70;            // +0x70 IopLoadCapcomSound: 0x50
    u_int buf74;            // +0x74 IopLoadCapcomSound: 5
    u_int buf78;            // +0x78 IopLoadCapcomSound: 0xb
    u_int buf7c;            // +0x7c IopLoadCapcomSound: 0x2000
    int sect;               // +0x80 SdPlayADSProc: a packed stream's first sector (gFlPackInfo), -1 loose
    u_int unk84;
} SND_TRANS_ADS;
static_assert(sizeof(SND_TRANS_ADS) == 0x88, "SND_TRANS_ADS");
static_assert(offsetof(SND_TRANS_ADS, loop) == 0x48, "SND_TRANS_ADS.loop");
static_assert(offsetof(SND_TRANS_ADS, vol) == 0x68, "SND_TRANS_ADS.vol");
static_assert(offsetof(SND_TRANS_ADS, sect) == 0x80, "SND_TRANS_ADS.sect");
#define SND_ADS_MODE 0x8000u

// gArg_CpDriverInit (bss 0x01943d40, 32 bytes): SNDDRV.IRX's start arguments, which CpMakeSnddrvInitStr
// turns into the module's argument string (IopLoadCapcomSound fills it). layout: ee. Named by value only:
// the IOP side reads them.
typedef struct SND_DRV_INIT {
    u_int w00;              // +0x00 0x1fffff
    u_int w04;              // +0x04 0x1ffff
    u_int w08;              // +0x08 1
    u_int w0c;              // +0x0c 0x1047
    u_short h10;            // +0x10 9
    u_short h12;            // +0x12 0xb
    u_short h14;            // +0x14 0x1b
    u_short h16;            // +0x16 0x1b
    u_short h18;            // +0x18 0x17
    u_short h1a;            // +0x1a 0
    u_char pad1c[4];
} SND_DRV_INIT;
static_assert(sizeof(SND_DRV_INIT) == 0x20, "SND_DRV_INIT");
static_assert(offsetof(SND_DRV_INIT, h1a) == 0x1a, "SND_DRV_INIT.h1a");

extern SND_JSL_COM gTransJSLCOM;        // bss 0x01943c40 (defined in lib/unassigned_data.cpp until this TU)
extern SND_TRANS_ADS gTransADS;         // bss 0x01943b40 (likewise)
extern SND_DRV_INIT gArg_CpDriverInit;  // bss 0x01943d40 (likewise)

// Prototypes from IopLoadCapcomSound's calls (2026-11-10); this TU writes them.
void CpInit_Gloval_EE(void);                                     // CT3 0x018d2770
void CpCreateEEComRecieveThread(int priority, int stackSize);    // CT3 0x018d2620
// Writes the driver's argument string for `init` into `buf`, answers its length (the ROM keeps the low
// byte, sign-extended).
int CpMakeSnddrvInitStr(char *buf, const SND_DRV_INIT *init);    // CT3 0x018d2850
void CpEEInit_RpcClient(void);                                   // CT3 0x018d2700
void CpExchangeTransBuffer(void);                                // CT3 0x018d26b0

// Sends one command to the IOP sound driver and answers its reply word's address (FSS_Thread copies the
// word to the ring entry's reply pointer). Prototype from FSS_Thread's call (2026-11-10).
int *CpCall_IOPSndDriver(u_int command, void *data);  // CT3 0x018d2250

EE_IOP_STAT *CpGetEEIOPStatus(void);    // CT3 0x018d1e70: &gEEStat (its uncached alias)

// Prototypes from SdLoadSEProc's calls and the ROM bodies (2026-11-10); this TU writes them.
// SIF DMA of `size` bytes (< 0xffff1) to the IOP address `iopAddr`, waited for; 0 = done, < 0 = refused.
// `intr` 1: from an interrupt handler (the i- calls), 0: from a thread.
int CpSifDmaTransEEToIOP(const void *src, u_int iopAddr, u_int size, int a3, int intr); // CT3 0x018d1fa0
// The driver's second channel (SND_CMD_TRANS_SPU2 / 0x110000 only): the RPC with an end callback that
// clears CpGetCPCMDCALLBACK's bit; answers the reply buffer.
int *CpCall_IOPSndDriver_2(u_int command, void *data); // CT3 0x018d2120
// 0 once the second-channel command `command` has ended; `poll` 1 asks once, 0 waits.
int CpCheckEE_IOPComEnd(u_int command, int poll);      // CT3 0x018d1e80
