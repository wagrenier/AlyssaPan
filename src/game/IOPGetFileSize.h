// game/IOPGetFileSize.h -- CT3 game/IOPGetFileSize.c (0x0189f460..): the EE side of the IOP's file server
// (FSYS_IOP.IRX). The declarations other TUs call (game/ArtoolDebugOpen.c first, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "sdk/sifrpc.h"

// sbss 0x0193e3a0 (120 bytes): three RPC clients; FlProc waits on [1] (gSCD + 0x28). layout: host.
extern sceSifClientData gSCD[3];

int IOPGetFileSize(int mode, const char *name, int *result);   // CT3 0x0189f460: result[0] size, [1] first sector
void IOPBindRPC(void);                                         // CT3 0x0189f4f0
int IOPComSync(sceSifClientData *client);                      // CT3 0x0189f5e0: != 0 while a call is in flight
