// game/copy2area.h -- CT3 game/copy2area.c: the declarations other TUs call (game/movie.c first,
// 2026-11-10). Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "sdk/libmpeg.h"

// Copies a PES payload that may wrap the demux ring (s0 / s1) into a destination that may wrap its own
// (d0 / d1); answers the bytes copied (ezmpegstr's copy2area; AA_videoCallback and AA_pcmCallback).
int copy2area(u_char *pd0, int d0, u_char *pd1, int d1,
              u_char *ps0, int s0, u_char *ps1, int s1);                                // CT3 0x018bf9d0

// The movie's audio stream callback (initAll registers it for sceMpegStrPCM, channel 0).
int AA_pcmCallback(sceMpeg *mp, sceMpegCbData *data, void *readBuf);                    // CT3 0x018bfb10
