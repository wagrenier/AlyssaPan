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

#ifdef __cplusplus
}
#endif
