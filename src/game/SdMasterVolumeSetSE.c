// game/SdMasterVolumeSetSE.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/SdMasterVolumeSetSE.h"

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018d1810
void SdChangeStereoMono(void)
{
    ALYSSAPAN_STUB("SdChangeStereoMono", 0x018d1810);
}

// stub of CT3 0x018cda30
void SdLoadSEEndWait(int handle)
{
    ALYSSAPAN_STUB("SdLoadSEEndWait", 0x018cda30);
}

// stub of CT3 0x018cdb10
void SdLoadSEEndWaitAll(void)
{
    ALYSSAPAN_STUB("SdLoadSEEndWaitAll", 0x018cdb10);
}

// stub of CT3 0x018cdf90
void SdMasterVolSetADS(int vol)
{
    ALYSSAPAN_STUB("SdMasterVolSetADS", 0x018cdf90);
}

// stub of CT3 0x018cbf90
void SdMasterVolumeSetSE(int vol)
{
    ALYSSAPAN_STUB("SdMasterVolumeSetSE", 0x018cbf90);
}

// stub of CT3 0x018ce080
void SdMasterVolumeSetSQ(int vol)
{
    ALYSSAPAN_STUB("SdMasterVolumeSetSQ", 0x018ce080);
}

// stub of CT3 0x018ce440
int SdPlaySQ(const char *dir, const char *name, u_int a2, int vol, int a4, int a5, int a6)
{
    ALYSSAPAN_STUB("SdPlaySQ", 0x018ce440);
    return {};
}

// stub of CT3 0x018cd930
void SdPurgeSE(const char *name)
{
    ALYSSAPAN_STUB("SdPurgeSE", 0x018cd930);
}

// stub of CT3 0x018cdba0
int SdReqLoadSE(const char *dir, const char *name, int jls, u_int spuAddr, int a4)
{
    ALYSSAPAN_STUB("SdReqLoadSE", 0x018cdba0);
    return {};
}

// stub of CT3 0x018cd3c0
int SdReqPlaySE(int jls, int a1, int a2)
{
    ALYSSAPAN_STUB("SdReqPlaySE", 0x018cd3c0);
    return {};
}

// stub of CT3 0x018d1600
void SdSetEffect(int a0, int a1, int a2)
{
    ALYSSAPAN_STUB("SdSetEffect", 0x018d1600);
}

// stub of CT3 0x018d14d0
void SdSoundDisable(void)
{
    ALYSSAPAN_STUB("SdSoundDisable", 0x018d14d0);
}

// stub of CT3 0x018d13d0
void SdSoundEnable(void)
{
    ALYSSAPAN_STUB("SdSoundEnable", 0x018d13d0);
}

// stub of CT3 0x018cbfe0
void SdStopSEAll(void)
{
    ALYSSAPAN_STUB("SdStopSEAll", 0x018cbfe0);
}

// stub of CT3 0x018cc1b0
void SdStopSEAllSp1(void)
{
    ALYSSAPAN_STUB("SdStopSEAllSp1", 0x018cc1b0);
}

// stub of CT3 0x018cc040
void SdStopSESDT(int jls, int a1, int a2)
{
    ALYSSAPAN_STUB("SdStopSESDT", 0x018cc040);
}

// stub of CT3 0x018ce1e0
void SdStopSQAllNow(void)
{
    ALYSSAPAN_STUB("SdStopSQAllNow", 0x018ce1e0);
}

// stub of CT3 0x018cddd0
void SdStopStream(int a0)
{
    ALYSSAPAN_STUB("SdStopStream", 0x018cddd0);
}

// stub of CT3 0x018cdde0
void SdVolumeStream(int vol)
{
    ALYSSAPAN_STUB("SdVolumeStream", 0x018cdde0);
}

// stub of CT3 0x018ce380
void SdStopSQ(const char *name)
{
    ALYSSAPAN_STUB("SdStopSQ", 0x018ce380);
}

// stub of CT3 0x018ce0d0
void SdVolumeSQ(const char *name, int vol, int a2)
{
    ALYSSAPAN_STUB("SdVolumeSQ", 0x018ce0d0);
}

// stub of CT3 0x018cde40
void SdPlayStream(const char *name, int vol, int a2, int loop)
{
    ALYSSAPAN_STUB("SdPlayStream", 0x018cde40);
}

// stub of CT3 0x018cd190
int SdSetParamSE(int handle, u_int mask, ...)
{
    ALYSSAPAN_STUB("SdSetParamSE", 0x018cd190);
    return {};
}

// stub of CT3 0x018cc0f0
void SdStopSE(int handle)
{
    ALYSSAPAN_STUB("SdStopSE", 0x018cc0f0);
}

// stub of CT3 0x018cc2a0
int SdStopSESp1(int jls, int a1, int a2)
{
    ALYSSAPAN_STUB("SdStopSESp1", 0x018cc2a0);
    return {};
}

// stub of CT3 0x018ccde0
SD_SE * SdFindPlaySE(int handle)
{
    ALYSSAPAN_STUB("SdFindPlaySE", 0x018ccde0);
    return {};
}

// stub of CT3 0x018cc220
void SdStopSESp1ByHandle(int handle)
{
    ALYSSAPAN_STUB("SdStopSESp1ByHandle", 0x018cc220);
}

// stub of CT3 0x018ce310
void SdStopSQAll(int fade)
{
    ALYSSAPAN_STUB("SdStopSQAll", 0x018ce310);
}

// stub of CT3 0x018cc350
int SdReqPlaySESp1(int jls, int a1, int a2, int a3, int a4, int a5, int a6, void (*update)(int handle), int a7,
                   int a8)
{
    ALYSSAPAN_STUB("SdReqPlaySESp1", 0x018cc350);
    return {};
}

// stub of CT3 0x018d17d0
void * SdAlloc(int size, int align, const char *name)
{
    ALYSSAPAN_STUB("SdAlloc", 0x018d17d0);
    return {};
}

// stub of CT3 0x018d17a0
void SdFree(void **mem)
{
    ALYSSAPAN_STUB("SdFree", 0x018d17a0);
}

// stub of CT3 0x018ce640
void SdLoadSQEndWaitAll(void)
{
    ALYSSAPAN_STUB("SdLoadSQEndWaitAll", 0x018ce640);
}
