// game/AA_SoundMonoSoart.h -- CT3 game/AA_SoundMonoSoart.c: the movie's audio decoder (SCE's ezmpegstr
// audiodec.c, AA_-prefixed), the declarations other TUs call (game/movie.c first, 2026-11-10).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

// The movie's audio decoder state (AA_audioDec, sbss 0x0193f390: 0x7c bytes in the symtab). Its fields are
// this TU's to name: movie.c only passes its address. layout: ee (opaque until then).
typedef struct AudioDec {
    u_char raw[0x7c];
} AudioDec;
static_assert(sizeof(AudioDec) == 0x7c, "AudioDec");

void Ass_StopIopDataTrans(void);                                                         // CT3 0x018bb7c0
int AA_audioDecSendToIOP(AudioDec *ad);                                                  // CT3 0x018bbeb0
int AA_audioDecIsPreset(AudioDec *ad);                                                   // CT3 0x018bc130
int AA_audioDecStart(AudioDec *ad);                                                      // CT3 0x018bc3b0
int AA_audioDecReset(AudioDec *ad);                                                      // CT3 0x018bc360
int AA_audioDecDelete(AudioDec *ad);                                                     // CT3 0x018bc4d0
// CT3's eight-argument create (ezmpegstr's plus the EE / IOP time-stamp rings): initAll's call.
int audioDecCreate(AudioDec *ad, void *buff, int buffSize, int iopBuffSize, void *eeTimeStamp,
                   int eeTimeStamps, void *iopTimeStamp, int iopTimeStamps);             // CT3 0x018bc4e0
int audioDecInit(AudioDec *ad, int iopBuffSize);                                         // CT3 0x018bc620
