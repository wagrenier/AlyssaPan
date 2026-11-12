// game/AA_mpegTS.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/AA_mpegTS.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018c1760
void AA_ReTagSet(u_int *tag)
{
    ALYSSAPAN_STUB("AA_ReTagSet", 0x018c1760);
}

// stub of CT3 0x018c1ce0
void AA_videoDecAbort(VideoDec *vd)
{
    ALYSSAPAN_STUB("AA_videoDecAbort", 0x018c1ce0);
}

// stub of CT3 0x018c1da0
int AA_videoDecCreate(VideoDec *vd, u_int mpegWork, int workSize, u_int viBufData, void *viBufTag,
                      int tagCount, void *timeStamp, int timeStamps)
{
    ALYSSAPAN_STUB("AA_videoDecCreate", 0x018c1da0);
    return {};
}

// stub of CT3 0x018c1cf0
int AA_videoDecDelete(VideoDec *vd)
{
    ALYSSAPAN_STUB("AA_videoDecDelete", 0x018c1cf0);
    return {};
}

// stub of CT3 0x018c1ba0
int AA_videoDecFlush(VideoDec *vd)
{
    ALYSSAPAN_STUB("AA_videoDecFlush", 0x018c1ba0);
    return {};
}

// stub of CT3 0x018c1cd0
int AA_videoDecGetState(VideoDec *vd)
{
    ALYSSAPAN_STUB("AA_videoDecGetState", 0x018c1cd0);
    return {};
}

// stub of CT3 0x018c1b60
int AA_videoDecIsFlushed(VideoDec *vd)
{
    ALYSSAPAN_STUB("AA_videoDecIsFlushed", 0x018c1b60);
    return {};
}

// stub of CT3 0x018c1b00
void AA_videoDecMain(void *videoDec)
{
    ALYSSAPAN_STUB("AA_videoDecMain", 0x018c1b00);
}

// stub of CT3 0x018c1d80
void AA_videoDecSetStream(VideoDec *vd, int strType, int ch, sceMpegCallback cb, void *data)
{
    ALYSSAPAN_STUB("AA_videoDecSetStream", 0x018c1d80);
}

// stub of CT3 0x018c2090
void AA_voBufCreate(VoBuf *f, u_int *data, VoTag *tag, int n)
{
    ALYSSAPAN_STUB("AA_voBufCreate", 0x018c2090);
}

// stub of CT3 0x018c2080
void AA_voBufDelete(VoBuf *f)
{
    ALYSSAPAN_STUB("AA_voBufDelete", 0x018c2080);
}

// stub of CT3 0x018c2030
int AA_voBufIsFull(VoBuf *f)
{
    ALYSSAPAN_STUB("AA_voBufIsFull", 0x018c2030);
    return {};
}

// stub of CT3 0x018c1ea0
void AA_voBufDecCount(VoBuf *f)
{
    ALYSSAPAN_STUB("AA_voBufDecCount", 0x018c1ea0);
}
