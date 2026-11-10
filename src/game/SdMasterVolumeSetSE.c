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
