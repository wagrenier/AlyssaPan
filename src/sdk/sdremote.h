// sdk/sdremote.h -- SCE's sdremote client, EE side (the ROM links it: sceSdRemoteInit, sceSdTransToIOP,
// sceSdCallBack, sceSdRemote; data/layout.json). Only what the game calls so far.
#pragma once

#include "scetypes.h"

#ifdef __cplusplus
extern "C" {
#endif

// CT3 0x018d2b18: SIF DMA of `size` bytes from `ee` to the IOP address `iopAddr`; mode 1 waits for it,
// any other non-zero mode only starts it. 0 = started, -1 = mode 0 or the DMA queue refused it. The host
// copies at once (sdk/sif.cpp).
int sceSdTransToIOP(const void *ee, u_int iopAddr, u_int size, int mode);

// CT3 0x018d2a68: bind the IOP's sdrdrv RPC service (AA_MovieIOPInit). The host has no sdrdrv to bind:
// it answers 0 at once (sdk/sif.cpp).
int sceSdRemoteInit(void);

// sdrdrv's function numbers (sceSdRemote's `fno`), RosePan's libsdr.h: the libsd call of the same name.
#define SDR_SET_PARAM 0x8010            /* sceSdSetParam(entry, value) */
#define SDR_VOICE_TRANS 0x80d0          /* sceSdVoiceTrans(chan, mode, iop, spu, size) */
#define SDR_BLOCK_TRANS 0x80e0          /* sceSdBlockTrans(chan, mode, iop, size[, start]) */
#define SDR_BLOCK_TRANS_STATUS 0x8100   /* sceSdBlockTransStatus(chan, flag) */
#define SDR_STOP_TRANS 0x8180           /* sceSdStopTrans(chan), by its place: Ass_StopIopDataTrans's */

// CT3 0x018d2bd8: run libsd function `fno` on the IOP with the arguments that follow (ints; IOP addresses
// travel as ints); `wait` 1 waits for the answer. The host calls sdk/iop.cpp's libsd at once (sdk/sif.cpp).
int sceSdRemote(int wait, int fno, ...);

#ifdef __cplusplus
}
#endif
