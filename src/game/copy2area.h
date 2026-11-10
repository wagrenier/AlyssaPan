// game/copy2area.h -- CT3 game/copy2area.c: the declarations other TUs call (game/movie.c first,
// 2026-11-10). Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "sdk/libmpeg.h"

// The movie's audio stream callback (initAll registers it for sceMpegStrPCM, channel 0).
int AA_pcmCallback(sceMpeg *mp, sceMpegCbData *data, void *readBuf);                    // CT3 0x018bfb10
