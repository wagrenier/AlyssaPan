// game/AA_viBuf.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/AA_viBuf.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018c0dc0
void AA_viBufAddDMA(ViBuf *f)
{
    ALYSSAPAN_STUB("AA_viBufAddDMA", 0x018c0dc0);
}

// stub of CT3 0x018c12c0
int AA_viBufCreate(ViBuf *f, u_char *data, void *tag, int n, TimeStamp *ts, int n_ts)
{
    ALYSSAPAN_STUB("AA_viBufCreate", 0x018c12c0);
    return {};
}

// stub of CT3 0x018c0990
int AA_viBufDelete(ViBuf *f)
{
    ALYSSAPAN_STUB("AA_viBufDelete", 0x018c0990);
    return {};
}

// stub of CT3 0x018c08f0
int AA_viBufFlush(ViBuf *f)
{
    ALYSSAPAN_STUB("AA_viBufFlush", 0x018c08f0);
    return {};
}

// stub of CT3 0x018c0440
int AA_viBufGetTs(ViBuf *f, TimeStamp *ts)
{
    ALYSSAPAN_STUB("AA_viBufGetTs", 0x018c0440);
    return {};
}

// stub of CT3 0x018c0620
int AA_viBufPutTs(ViBuf *f, TimeStamp *ts)
{
    ALYSSAPAN_STUB("AA_viBufPutTs", 0x018c0620);
    return {};
}

// stub of CT3 0x018c1160
int AA_viBufReset(ViBuf *f)
{
    ALYSSAPAN_STUB("AA_viBufReset", 0x018c1160);
    return {};
}

// stub of CT3 0x018c09e0
void AA_viBufRestartDMA(ViBuf *f)
{
    ALYSSAPAN_STUB("AA_viBufRestartDMA", 0x018c09e0);
}

// stub of CT3 0x018c0ce0
void AA_viBufStopDMA(ViBuf *f)
{
    ALYSSAPAN_STUB("AA_viBufStopDMA", 0x018c0ce0);
}

// stub of CT3 0x018c1060
void AA_viBufBeginPut(ViBuf *f, u_char **ptr0, int *len0, u_char **ptr1, int *len1)
{
    ALYSSAPAN_STUB("AA_viBufBeginPut", 0x018c1060);
}

// stub of CT3 0x018c0940
int AA_viBufCount(ViBuf *f)
{
    ALYSSAPAN_STUB("AA_viBufCount", 0x018c0940);
    return {};
}

// stub of CT3 0x018c1000
void AA_viBufEndPut(ViBuf *f, int size)
{
    ALYSSAPAN_STUB("AA_viBufEndPut", 0x018c1000);
}
