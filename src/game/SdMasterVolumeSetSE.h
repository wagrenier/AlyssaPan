// game/SdMasterVolumeSetSE.h -- CT3 game/SdMasterVolumeSetSE.c (the EE side of the sound driver: SE, SQ,
// ADS stream). The declarations other TUs call (game/sound.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void SdMasterVolumeSetSE(int vol);                                                       // CT3 0x018cbf90
void SdStopSEAll(void);                                                                  // CT3 0x018cbfe0
void SdStopSESDT(int jls, int a1, int a2);                                               // CT3 0x018cc040
void SdStopSEAllSp1(void);                                                               // CT3 0x018cc1b0
int SdReqPlaySE(int jls, int a1, int a2);                                                // CT3 0x018cd3c0
void SdPurgeSE(const char *name);                                                        // CT3 0x018cd930
void SdLoadSEEndWait(int handle);                                                        // CT3 0x018cda30
void SdLoadSEEndWaitAll(void);                                                           // CT3 0x018cdb10
int SdReqLoadSE(const char *dir, const char *name, int jls, u_int spuAddr, int a4);     // CT3 0x018cdba0
void SdStopStream(int a0);                                                               // CT3 0x018cddd0
void SdVolumeStream(int vol);                                                            // CT3 0x018cdde0
void SdMasterVolSetADS(int vol);                                                         // CT3 0x018cdf90
void SdMasterVolumeSetSQ(int vol);                                                       // CT3 0x018ce080
void SdVolumeSQ(const char *name, int vol, int a2);                                      // CT3 0x018ce0d0
void SdStopSQAllNow(void);                                                               // CT3 0x018ce1e0
void SdStopSQ(const char *name);                                                         // CT3 0x018ce380
int SdPlaySQ(const char *dir, const char *name, u_int a2, int vol, int a4, int a5, int a6); // CT3 0x018ce440
void SdSoundEnable(void);                                                                // CT3 0x018d13d0
void SdSoundDisable(void);                                                               // CT3 0x018d14d0
void SdSetEffect(int a0, int a1, int a2);                                                // CT3 0x018d1600
void SdChangeStereoMono(void);                                                           // CT3 0x018d1810
