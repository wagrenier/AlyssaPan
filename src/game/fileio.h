// game/fileio.h -- CT3 game/fileio.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void CharacterSort(void);                                                                // CT3 0x018c2170
void SceneEndBank(int first, int count);                                                 // CT3 0x018c5780
void FSMltLoadEndWaitAll(void);                                                          // CT3 0x018c60a0
void FileSoundServer(void);                                                              // CT3 0x018c7dc0
void FS_work_init(void);                                                                 // CT3 0x018c7e00
void objSortWithBlurIdx(int scene, int count, void *table);                              // CT3 0x018c23b0
void objSort(int scene, int count, void *table);                                         // CT3 0x018c2880
char *fs_error(const char *file, int line, const char *title, const char *fmt, ...);     // CT3 0x018c7ec0
void FSInit(void);                                                                       // CT3 0x018c7eb0
