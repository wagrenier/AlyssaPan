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
    u_char unk24[0x2c];
    int iopBusy;            // +0x50 != 0: the IOP is using the drive (FlProc waits)
    u_char unk54[0x2c];
} EE_IOP_STAT;
static_assert(sizeof(EE_IOP_STAT) == 0x80, "EE_IOP_STAT is 0x80 bytes");

static_assert(offsetof(EE_IOP_STAT, adsActive) == 0x20, "EE_IOP_STAT.adsActive");
static_assert(offsetof(EE_IOP_STAT, iopBusy) == 0x50, "EE_IOP_STAT.iopBusy");

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

// A JSL command record (gTransJSLCOM, and the Sd TU's seStopJSLCOM / effectJSLCOM): 8 words, the
// arguments from +8. layout: ee.
typedef struct SND_JSL_COM {
    int unk00;
    int unk04;
    int arg[6];             // +0x08..+0x1c
} SND_JSL_COM;
static_assert(sizeof(SND_JSL_COM) == 0x20, "SND_JSL_COM");

// gTransADS (bss 0x01943b40, 0x88 bytes): the ADS stream's command record. layout: ee.
typedef struct SND_TRANS_ADS {
    u_char unk00[0x68];
    u_int vol;              // +0x68 the scaled volume, both channels (v | v << 16)
    u_char unk6c[0x1c];
} SND_TRANS_ADS;
static_assert(sizeof(SND_TRANS_ADS) == 0x88, "SND_TRANS_ADS");
static_assert(offsetof(SND_TRANS_ADS, vol) == 0x68, "SND_TRANS_ADS.vol");

extern SND_JSL_COM gTransJSLCOM;        // bss 0x01943c40 (defined in lib/unassigned_data.cpp until this TU)
extern SND_TRANS_ADS gTransADS;         // bss 0x01943b40 (likewise)

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
