// game/copy2area.c -- CT3 0x018bf9d0..0x018bfbf0: the copy from the read ring into a decoder's input
// ring, and the movie's PCM stream callback -- SCE's ezmpegstr sample (copy2area, pcmCallback), CT3's
// names. AA_videoCallback (game/AA_videoCallback.c) is the video stream's twin.

#include "game/copy2area.h"

#include "game/AA_SoundMonoSoart.h"
#include "game/AA_videoCallback.h"
#include "game/movie.h"

#include <string.h>

// The PCM packet's own header in front of its samples: the demuxer hands it over with the payload and
// the callback skips it (ezmpegstr's "+ 4").
#define PCM_PACKET_HEADER 4

// CT3 0x018bf9d0: copies a payload that may wrap its source ring (ps0 / s0, then ps1 / s1) into a
// destination that may wrap its own (pd0 / d0, then pd1 / d1). Copies nothing when the payload does not
// fit; answers the bytes copied.
int copy2area(u_char *pd0, int d0, u_char *pd1, int d1,
              u_char *ps0, int s0, u_char *ps1, int s1)
{
    if (d0 + d1 < s0 + s1)
        return 0;

    if (s0 < d0) {
        if (s1 < d0 - s0) {
            // both source runs fit in the first area
            memcpy(pd0, ps0, s0);
            memcpy(pd0 + s0, ps1, s1);
        } else {
            // the second source run spills into the second area
            memcpy(pd0, ps0, s0);
            memcpy(pd0 + s0, ps1, d0 - s0);
            memcpy(pd1, ps1 + (d0 - s0), s1 - (d0 - s0));
        }
    } else {
        // the first source run already fills the first area
        memcpy(pd0, ps0, d0);
        memcpy(pd1, ps0 + d0, s0 - d0);
        memcpy(pd1 + (s0 - d0), ps1, s1);
    }
    return s0 + s1;
}

// CT3 0x018bfb10: the demuxer's PCM callback: copies one packet's samples (past its header; they may wrap
// the read ring's end) into the audio decoder's ring and records their time stamp. Answers non-zero when
// bytes were copied.
int AA_pcmCallback(sceMpeg *mp, sceMpegCbData *data, void *readBuf)
{
    ReadBuf *rb = (ReadBuf *)readBuf;
    sceMpegCbDataStr *cb = (sceMpegCbDataStr *)data;
    u_char *ringData = rb->data;
    u_char *ringEnd = ringData + rb->size;
    u_char *src = cb->data + PCM_PACKET_HEADER;
    int len = (int)cb->len - PCM_PACKET_HEADER;
    int len0;
    u_char *ptr0;
    u_char *ptr1;
    int room0;
    int room1;
    int copied;

    (void)mp;
    if (ringEnd <= src)            // the header ended the ring: the samples start at its beginning
        src -= rb->size;
    len0 = (int)(ringEnd - src);   // the bytes before the ring wraps
    if (len < len0)
        len0 = len;
    AA_audioDecBeginPut(&AA_audioDec, &ptr0, &room0, &ptr1, &room1);
    copied = copy2area(ptr0, room0, ptr1, room1, src, len0, ringData, len - len0);
    AA_audioDecEndPut(&AA_audioDec, copied, (u_long)cb->pts);
    return copied > 0;
}
