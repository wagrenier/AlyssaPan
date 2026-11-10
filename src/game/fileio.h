// game/fileio.h -- CT3 game/fileio.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void CharacterSort(void);                                                                // CT3 0x018c2170
void SceneEndBank(int first, int count);                                                 // CT3 0x018c5780
void FSMltSetDrawBank(int bank, char *name, int mode);                                   // CT3 0x018c40d0 (mc_load_set: mode 0)
void FSMltLoadEndWaitAll(void);                                                          // CT3 0x018c60a0
void FileSoundServer(void);                                                              // CT3 0x018c7dc0
void FS_work_init(void);                                                                 // CT3 0x018c7e00
void objSortWithBlurIdx(int scene, int count, void *table);                              // CT3 0x018c23b0
void objSort(int scene, int count, void *table);                                         // CT3 0x018c2880
char *fs_error(const char *file, int line, const char *title, const char *fmt, ...);     // CT3 0x018c7ec0
void FSInit(void);                                                                       // CT3 0x018c7eb0
void Round2ChangeChar(int round);                                                        // CT3 0x018c2df0
// From game/main2.c's calls (2026-11-10): FSMltLoad answers a handle FSMltLoadEnd polls (< 0: not yet);
// FSMltLoadStatus is non-zero while the bank's file is still loading; FSMltLoadEndWaitFile waits for it
// and answers the bank. kind: main2.h FS_LOAD_*.
int FSMltLoad(char *name, int kind, int bank, int arg);                                  // CT3 0x018c5cd0
int FSMltLoadEnd(int handle);                                                            // CT3 0x018c3f30
int FSMltLoadStatus(int bank, char *name);                                               // CT3 0x018c3fd0
int FSMltLoadEndWaitFile(int bank, char *name);                                          // CT3 0x018c3cf0
