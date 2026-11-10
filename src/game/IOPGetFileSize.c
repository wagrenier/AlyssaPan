// game/IOPGetFileSize.c -- CT3 0x0189f460..0x0189f620: the EE side of the IOP's file server (FSYS_IOP.IRX).
// Reconstructed 2026-11-10 from the symtab's 4 functions (the key is the object's first function).
//
// IOPBindRPC binds the three FSYS_IOP services (gSCD[0..2], numbers 'SUN@' / 'SUNA' / 'SUNB', each with a
// debug name the ROM passes but never reads); IOPGetFileSize posts a size request on the second one and
// IOPComSync says whether a call is still in flight (FlProc polls it, ArtoolDebugOpen.c).
//
// PORT: no host server answers FSYS_ID_GET_FILE_SIZE yet (sdk/sif.cpp answers zeros for an unregistered
// service), so a loose file's size reads 0: open-items.md (bf).

#include "game/IOPGetFileSize.h"

#include "sdk/eekernel.h"
#include "sdk/sifrpc.h"

#include <stdio.h>
#include <string.h>

// ---- data ---------------------------------------------------------------------------------------------
static char sFileNameToIOP[FSYS_NAME_SIZE];      /* sbss 0x0193e360: the name IOPGetFileSize sends */
sceSifClientData gSCD[FSYS_CLIENT_COUNT];        /* sbss 0x0193e3a0 */

static const char s_getFileSizeError[] = "error IOPGetFileSize(), sceSifCallRpc() %d\n"; /* @103 0x01934e00 */
static const char s_nameComTest[] = "IOPComTest()";                                     /* @109 0x01934e30 */
static const char s_nameGetFileSize[] = "IOPGetFileSize()";                             /* @110 0x01934e40 */
static const char s_nameSetSDT[] = "IOPSetSDT()";                                       /* @111 0x01934e58 */
static const char s_bindError[] = "bind error sceSifBindRpc()=%d\n";                     /* @127 0x01934e70 */

// The spin bind_rpc waits between tries (0x10000 + 1 decrements).
#define BIND_RPC_SPIN 0x10000

// ---- code ---------------------------------------------------------------------------------------------

// CT3 0x0189f460: ask the file server for `name`'s size and first sector (result[0], [1]; 0x10 bytes
// answered). `mode` is sceSifCallRpc's (FlProc passes SIF_RPC_M_NOWAIT and polls IOPComSync). 0 when the
// call was posted, sceSifCallRpc's error otherwise.
int IOPGetFileSize(int mode, const char *name, int *result)
{
    int ret;

    strcpy(sFileNameToIOP, name);
    ret = sceSifCallRpc(&gSCD[FSYS_CLIENT_FILE], FSYS_ID_GET_FILE_SIZE, (unsigned int)mode, sFileNameToIOP,
                        FSYS_NAME_SIZE, result, 0x10, NULL, NULL);
    if (ret < 0) {
        printf(s_getFileSizeError, ret);
        return ret;
    }
    return 0;
}

// CT3 0x0189f4f0
void IOPBindRPC(void)
{
    bind_rpc(s_nameComTest, &gSCD[FSYS_CLIENT_COM_TEST], FSYS_ID_COM_TEST);
    bind_rpc(s_nameGetFileSize, &gSCD[FSYS_CLIENT_FILE], FSYS_ID_GET_FILE_SIZE);
    bind_rpc(s_nameSetSDT, &gSCD[FSYS_CLIENT_SET_SDT], FSYS_ID_SET_SDT);
}

// CT3 0x0189f550: bind `client` to service `id`, retrying until the IOP has registered it (serve set).
// `name` is not read. Returns the last bind's result, or printf's when that bind failed (the ROM's v0).
int bind_rpc(const char *name, sceSifClientData *client, unsigned int id)
{
    int ret;

    (void)name;
    for (;;) {
        ret = sceSifBindRpc(client, id, 0);
        if (ret < 0) {
            ret = printf(s_bindError, ret);
        }
        if (client->serve != NULL) {
            return ret;
        }
        for (volatile int spin = BIND_RPC_SPIN; spin-- != 0;) {
        }
    }
}

// CT3 0x0189f5e0: 1 while the call on `client` is in flight; 0 (after flushing the data cache, so the
// answer is visible) when it has landed.
int IOPComSync(sceSifClientData *client)
{
    if (sceSifCheckStatRpc(&client->rpcd) != 0) {
        return 1;
    }
    FlushCache(0);
    return 0;
}
