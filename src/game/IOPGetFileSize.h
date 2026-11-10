// game/IOPGetFileSize.h -- CT3 game/IOPGetFileSize.c (0x0189f460..0x0189f620): the EE side of the IOP's
// file server (FSYS_IOP.IRX). Written 2026-11-10 with the TU.
#pragma once

#include "sdk/scetypes.h"
#include "sdk/sifrpc.h"

// The three FSYS_IOP services IOPBindRPC binds ('SUN@', 'SUNA', 'SUNB'), the debug name beside each.
// Only GET_FILE_SIZE is called; the other two are bound and called from nowhere in the ROM.
#define FSYS_ID_COM_TEST      0x53554e40   // "IOPComTest()"
#define FSYS_ID_GET_FILE_SIZE 0x53554e41   // "IOPGetFileSize()": the call's fno is the service number too
#define FSYS_ID_SET_SDT       0x53554e42   // "IOPSetSDT()"

enum FsysClient {
    FSYS_CLIENT_COM_TEST = 0,
    FSYS_CLIENT_FILE = 1,          // FlProc waits on it (gSCD + 0x28)
    FSYS_CLIENT_SET_SDT = 2,
    FSYS_CLIENT_COUNT = 3,
};

// IOPGetFileSize's send buffer: the name, 0x40 bytes.
#define FSYS_NAME_SIZE 0x40

// sbss 0x0193e3a0 (120 bytes on the EE: 3 x 0x28). layout: host (sceSifClientData holds pointers).
extern sceSifClientData gSCD[FSYS_CLIENT_COUNT];

int IOPGetFileSize(int mode, const char *name, int *result);              // CT3 0x0189f460: result[0] size, [1] first sector
void IOPBindRPC(void);                                                    // CT3 0x0189f4f0
int bind_rpc(const char *name, sceSifClientData *client, unsigned int id); // CT3 0x0189f550
int IOPComSync(sceSifClientData *client);                                 // CT3 0x0189f5e0: 1 while a call is in flight
