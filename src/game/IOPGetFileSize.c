// game/IOPGetFileSize.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/IOPGetFileSize.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x0189f4f0
void IOPBindRPC(void)
{
    ALYSSAPAN_STUB("IOPBindRPC", 0x0189f4f0);
}

// stub of CT3 0x0189f5e0
int IOPComSync(sceSifClientData *client)
{
    ALYSSAPAN_STUB("IOPComSync", 0x0189f5e0);
    return {};
}

// stub of CT3 0x0189f460
int IOPGetFileSize(int mode, const char *name, int *result)
{
    ALYSSAPAN_STUB("IOPGetFileSize", 0x0189f460);
    return {};
}
