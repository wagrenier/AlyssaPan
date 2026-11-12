// game/AA_SoundMonoSoart.c -- Clock Tower 3: the movie's audio decoder (SCE's ezmpegstr audiodec.c,
// AA_-prefixed). Written 2026-11-12, all 20 functions.
//
// The stream callback (copy2area.c) fills the EE ring through AA_audioDecBeginPut / EndPut: the 40-byte
// SShd / SSbd header first, then 16-bit PCM in 0x400-byte units (0x200 left, 0x200 right). The movie loop
// sends whole units to the IOP ring (AA_audioDecSendToIOP), which the SPU2's auto-DMA on core 0 plays
// once AA_audioDecStart starts it. CT3 adds two things to SCE's sample:
//   - time stamps: each put records its pts on the EE ring (addEETimeStamp), each send carries the
//     stamps over to the IOP ring (sendToIOP -> addIOPTimeStamp), and every send reads the SPU2's play
//     position back to tell the movie clock where the sound is (AA_syncClockInAudio). A byte is
//     0.46875 ticks of 90 kHz (48 kHz, 4 bytes a stereo sample);
//   - mono (AA_SOUND_MODE, the options' monaural): each unit's left half is copied over its right before
//     it leaves the EE.
//
// PORT: IOP addresses are host addresses below 4 GB (sdk/sif.cpp), kept in u_ints as the ROM keeps them;
// the SPU2 status needs no 24-bit mask on the host (AA_audioDecSendToIOP).

#include "game/AA_SoundMonoSoart.h"

#include "game/AA_cpuCycles.h"
#include "game/movie.h"
#include "game/unassigned_data.h"
#include "sdk/eekernel.h"
#include "sdk/libsd.h"
#include "sdk/sdremote.h"
#include "sdk/sif.h"
#include "sdk/sifman.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AU_AUTODMA_CORE 0               /* the auto-DMA channel: core 0 */
#define AU_BLOCK_MODE_STOP 2            /* sceSdBlockTrans: stop, answering where it was */
#define AU_BLOCK_MODE_WRITE_FROM_LOOP 0x13 /* play the ring in a loop from an address */
#define AU_VOICE_MODE_WRITE 0
#define AU_SPU_INPUT_CORE0 0x4000       /* the SPU2's sound-data input area of core 0 */
#define AU_VOLUME_MAX 0x7fff
#define AU_VOLUME_STEP 0x666            /* BGM_MASTER_VOLUME_BUFF (0..20) to a BVOL value */
#define AU_PTS_PER_BYTE 0.46875         /* 90000 / (48000 * 4) */

static u_char _0_buf[AU_ZERO_BUFF_SIZE]; /* bss 0x01a41a80 */

static u_long AA_bytesToPts(int bytes)
{
    return (u_long)(long long)((double)(float)bytes * AU_PTS_PER_BYTE);
}

static u_char *AA_iopPtr(u_int iop)
{
    return (u_char *)(uintptr_t)iop;
}

// CT3 0x018bb610: mono: copy one unit's left half (0x200 bytes) over its right half.
void AA_SoundMonoSoart(u_char *unit)
{
    memcpy(unit + AU_UNIT_SIZE / 2, unit, AU_UNIT_SIZE / 2);
}

// CT3 0x018bb680: mono for every whole unit of `size` bytes.
void AA_MonSoart(u_char *data, int size)
{
    int i;

    for (i = 0; i < size / AU_UNIT_SIZE; i++)
        AA_SoundMonoSoart(data + i * AU_UNIT_SIZE);
}

// CT3 0x018bb6e0: stamp `len` bytes from `pos` of the IOP ring.
static void addIOPTimeStamp(AudioDec *ad, int pos, int len, u_long pts)
{
    AudioTimeStamp *ts = &ad->iopTs[ad->iopTsPut];

    ts->pts = pts;
    ts->pos = pos;
    ts->len = len;
    ad->iopTsCount++;
    ad->iopTsPut = (ad->iopTsPut + 1) % ad->iopTsSize;
}

// CT3 0x018bb750: stamp `len` bytes from `pos` of the EE ring.
static void addEETimeStamp(AudioDec *ad, int pos, int len, u_long pts)
{
    AudioTimeStamp *ts = &ad->eeTs[ad->eeTsPut];

    ts->pts = pts;
    ts->pos = pos;
    ts->len = len;
    ad->eeTsCount++;
    ad->eeTsPut = (ad->eeTsPut + 1) % ad->eeTsSize;
}

// CT3 0x018bb7c0: after the movie: stop core 0's transfer.
void Ass_StopIopDataTrans(void)
{
    sceSdRemote(1, SDR_STOP_TRANS, AU_AUTODMA_CORE);
}

// CT3 0x018bb7d0: the auto-DMA's input volume, both sides.
static void changeInputVolume(u_int val)
{
    sceSdRemote(1, SDR_SET_PARAM, AU_AUTODMA_CORE | SD_P_BVOLL, val);
    sceSdRemote(1, SDR_SET_PARAM, AU_AUTODMA_CORE | SD_P_BVOLR, val);
}

// CT3 0x018bb820: DMA `size` bytes of the EE ring at `src` to the IOP address `dst`, then carry the EE
// ring's stamps of those bytes over to the IOP ring. Answers the size sent.
static int sendToIOP(AudioDec *ad, u_int dst, u_char *src, int size)
{
    sceSifDmaData d;
    u_int id;
    int off = (int)(src - ad->data);
    int end = off + size;
    int first;
    int last;
    int found;
    int i;
    int k;

    if (size < 1)
        return 0;
    if (AA_SOUND_MODE != 0)
        AA_MonSoart(src, size);
    d.data = src;
    d.addr = AA_iopPtr(dst);
    d.size = size;
    d.mode = 0;
    FlushCache(0);
    id = sceSifSetDma(&d, 1);
    while (sceSifDmaStat(id) >= 0)
        ;

    // The newest EE stamp holding the first byte.
    first = ad->eeTsPut;
    found = 0;
    for (k = 0; k < ad->eeTsCount && k < ad->eeTsSize; k++) {
        first = (ad->eeTsSize + (first - 1) % ad->eeTsSize) % ad->eeTsSize;
        if (ad->eeTs[first].pos <= off && off <= ad->eeTs[first].pos + ad->eeTs[first].len) {
            // At slot 0 the ROM also tries record eeTsCount - 1 and, if it holds the byte too, starts at
            // the ring's last slot. PORT: it reads that record only while it lies in the ring (the ROM
            // reads past the ring's end once eeTsCount outgrows it).
            if (first == 0 && ad->eeTsCount <= ad->eeTsSize) {
                const AudioTimeStamp *prev = &ad->eeTs[ad->eeTsCount - 1];

                if (prev->pos <= off && off <= prev->pos + prev->len)
                    first = ad->eeTsSize - 1;
            }
            found = 1;
            break;
        }
    }
    if (!found)
        return size;

    // The stamp holding the last byte.
    last = first;
    found = 0;
    for (k = 0; k < ad->eeTsSize; k++) {
        if (last == ad->eeTsSize)
            last = 0;
        if (end <= ad->eeTs[last].pos + ad->eeTs[last].len) {
            found = 1;
            break;
        }
        last++;
    }
    if (!found)
        return size;

    if (first == last) {
        const AudioTimeStamp *ts = &ad->eeTs[first];

        addIOPTimeStamp(ad, (int)(dst - ad->iopBuff), end - off, ts->pts + AA_bytesToPts(off - ts->pos));
        return size;
    }
    {
        const AudioTimeStamp *ts = &ad->eeTs[first];
        int skip = off - ts->pos;

        addIOPTimeStamp(ad, (int)(dst - ad->iopBuff), ts->len - skip, ts->pts + AA_bytesToPts(skip));
    }
    // The ROM steps the middle stamps modulo the IOP ring's size (both rings hold AA_AUD_TIMESTAMPS).
    for (i = (first + 1) % ad->eeTsSize; i != last; i = (ad->iopTsSize + (i + 1) % ad->iopTsSize) % ad->iopTsSize) {
        const AudioTimeStamp *ts = &ad->eeTs[i];

        addIOPTimeStamp(ad, (int)(dst - ad->iopBuff) + (ad->size + (ts->pos - off) % ad->size) % ad->size,
                        ts->len, ts->pts);
    }
    addIOPTimeStamp(ad, (int)(dst - ad->iopBuff) + (ad->size + (ad->eeTs[last].pos - off) % ad->size) % ad->size,
                    end - ad->eeTs[last].pos, ad->eeTs[last].pts);
    return size;
}

// CT3 0x018bbc50: send the EE ring's two areas (s0 at ps0, s1 at ps1) into the IOP ring's two (d0 at pd0,
// d1 at pd1), as much as fits. Answers the bytes sent.
static int sendToIOP2area(AudioDec *ad, u_int pd0, int d0, u_int pd1, int d1, u_char *ps0, int s0, u_char *ps1,
                          int s1)
{
    if (d0 + d1 < s0 + s1) {
        int diff = (s0 + s1) - (d0 + d1);

        if (diff < s1) {
            s1 -= diff;
        } else {
            s0 -= diff - s1;
            s1 = 0;
        }
    }
    if (s0 < d0) {
        if (s1 < d0 - s0) {
            sendToIOP(ad, pd0, ps0, s0);
            sendToIOP(ad, pd0 + s0, ps1, s1);
        } else {
            sendToIOP(ad, pd0, ps0, s0);
            sendToIOP(ad, pd0 + s0, ps1, d0 - s0);
            sendToIOP(ad, pd1, ps1 + d0 - s0, s1 - (d0 - s0));
        }
    } else {
        sendToIOP(ad, pd0, ps0, d0);
        sendToIOP(ad, pd1, ps0 + d0, s0 - d0);
        sendToIOP(ad, pd1 + s0 - d0, ps1, s1);
    }
    return s0 + s1;
}

// CT3 0x018bbdd0: the IOP ring's free room, from iopLastPos up to a unit short of the play position
// `pos`, in whole units: one or two areas.
static void iopGetArea(u_int *pd0, int *d0, u_int *pd1, int *d1, AudioDec *ad, int pos)
{
    int len = (pos + ad->iopBuffSize - ad->iopLastPos - AU_UNIT_SIZE) % ad->iopBuffSize;

    if (pos - ad->iopLastPos >= 0 && pos - ad->iopLastPos < AU_UNIT_SIZE) {
        *pd0 = ad->iopBuff;
        *d0 = 0;
        *pd1 = ad->iopBuff;
        *d1 = 0;
        return;
    }
    len = len / AU_UNIT_SIZE * AU_UNIT_SIZE;
    if (ad->iopBuffSize - ad->iopLastPos < len) {
        *pd0 = ad->iopBuff + ad->iopLastPos;
        *d0 = ad->iopBuffSize - ad->iopLastPos;
        *pd1 = ad->iopBuff;
        *d1 = len - (ad->iopBuffSize - ad->iopLastPos);
    } else {
        *pd0 = ad->iopBuff + ad->iopLastPos;
        *d0 = len;
        *pd1 = 0;
        *d1 = 0;
    }
}

// CT3 0x018bbeb0: the movie loop's audio step: tell the movie clock where the SPU2 plays (by the IOP
// ring's stamps), then send the EE ring's whole units into the IOP ring's free room. Answers the bytes sent.
int AA_audioDecSendToIOP(AudioDec *ad)
{
    u_int pd0 = 0;
    u_int pd1 = 0;
    int d0 = 0;
    int d1 = 0;
    int pos = 0;
    int sent = 0;
    int i;
    int k;
    u_char *ps0;
    u_char *ps1;
    int s0;
    int s1;
    int units;

    switch (ad->state) {
    case AU_STATE_PAUSE:
    case AU_STATE_INIT:
        return 0;
    case AU_STATE_PLAYING:
        // PORT: the ROM masks the status to the IOP's 24-bit address (libsd keeps flags above it); the
        // host's IOP addresses are wider and its status carries no flags.
        pos = (int)((u_int)sceSdRemote(1, SDR_BLOCK_TRANS_STATUS, AU_AUTODMA_CORE) - ad->iopBuff);
        iopGetArea(&pd0, &d0, &pd1, &d1, ad, pos);
        break;
    case AU_STATE_PRESET:
        pd0 = ad->iopBuff + ad->totalBytesSent % ad->iopBuffSize;
        d0 = ad->iopBuffSize - ad->totalBytesSent;
        pd1 = 0;
        d1 = 0;
        break;
    default:
        break;
    }

    // The newest IOP stamp holding the play position sets the clock.
    i = ad->iopTsPut;
    for (k = 0; k < ad->iopTsSize; k++) {
        i = (ad->iopTsSize + (i - 1) % ad->iopTsSize) % ad->iopTsSize;
        if (ad->iopTs[i].pos <= pos && pos <= ad->iopTs[i].pos + ad->iopTs[i].len) {
            AA_syncClockInAudio(ad->iopTs[i].pts + AA_bytesToPts(pos - ad->iopTs[i].pos));
            break;
        }
    }

    ps0 = ad->data + (ad->size + ad->put - ad->count) % ad->size;
    ps1 = ad->data;
    units = ad->count / AU_UNIT_SIZE * AU_UNIT_SIZE;
    s0 = (int)(ad->data + ad->size - ps0);
    if (units < s0)
        s0 = units;
    s1 = units - s0;
    if (d0 + d1 >= AU_UNIT_SIZE && s0 + s1 >= AU_UNIT_SIZE)
        sent = sendToIOP2area(ad, pd0, d0, pd1, d1, ps0, s0, ps1, s1);
    ad->count -= sent;
    ad->totalBytesSent += sent;
    ad->iopLastPos = (ad->iopLastPos + sent) % ad->iopBuffSize;
    return sent;
}

// CT3 0x018bc130: the IOP ring is full: the movie may start the sound.
int AA_audioDecIsPreset(AudioDec *ad)
{
    return ad->iopBuffSize <= ad->totalBytesSent;
}

// CT3 0x018bc150: `size` bytes were put where AA_audioDecBeginPut said; those past the header start at `pts`.
void AA_audioDecEndPut(AudioDec *ad, int size, u_long pts)
{
    if (ad->state == AU_STATE_INIT) {
        u_int hdr = AU_HEADER_SIZE - ad->hdrCount;

        if ((u_int)size <= hdr)
            hdr = (u_int)size;
        ad->hdrCount += (int)hdr;
        if ((u_int)ad->hdrCount >= AU_HEADER_SIZE)
            ad->state = AU_STATE_PRESET;
        size -= (int)hdr;
    }
    if (size > 0) {
        if (ad->put + size > ad->size) {
            int head = ad->size - ad->put;

            addEETimeStamp(ad, ad->put, head, pts);
            addEETimeStamp(ad, 0, size - head, pts + AA_bytesToPts(head));
        } else {
            addEETimeStamp(ad, ad->put, size, pts);
        }
    }
    ad->put = (ad->put + size) % ad->size;
    ad->count += size;
    ad->totalBytes += size;
}

// CT3 0x018bc2a0: where the stream callback may write: what is left of the header (then the ring), else
// the ring's free room in one or two areas.
void AA_audioDecBeginPut(AudioDec *ad, u_char **ptr0, int *len0, u_char **ptr1, int *len1)
{
    int room;

    if (ad->state == AU_STATE_INIT) {
        *ptr0 = (u_char *)&ad->sshd + ad->hdrCount;
        *len0 = AU_HEADER_SIZE - ad->hdrCount;
        *ptr1 = ad->data;
        *len1 = ad->size;
        return;
    }
    room = ad->size - ad->count;
    if (ad->size - ad->put < room) {
        *ptr0 = ad->data + ad->put;
        *len0 = ad->size - ad->put;
        *ptr1 = ad->data;
        *len1 = room - (ad->size - ad->put);
    } else {
        *ptr0 = ad->data + ad->put;
        *len0 = room;
        *ptr1 = NULL;
        *len1 = 0;
    }
}

// CT3 0x018bc360: stop the sound and empty both rings.
void AA_audioDecReset(AudioDec *ad)
{
    AA_audioDecPause(ad);
    ad->state = AU_STATE_INIT;
    ad->hdrCount = 0;
    ad->put = 0;
    ad->count = 0;
    ad->totalBytes = 0;
    ad->totalBytesSent = 0;
    ad->iopLastPos = 0;
    ad->iopPausePos = 0;
}

// CT3 0x018bc3b0: start the sound (the ROM's one instruction jumps to AA_audioDecResume).
void AA_audioDecStart(AudioDec *ad)
{
    AA_audioDecResume(ad);
}

// CT3 0x018bc3c0: the input volume from the BGM option, then the auto-DMA loops the IOP ring from where
// it paused.
void AA_audioDecResume(AudioDec *ad)
{
    u_int vol = (u_int)BGM_MASTER_VOLUME_BUFF * AU_VOLUME_STEP;

    if (vol > AU_VOLUME_MAX)
        vol = AU_VOLUME_MAX;
    changeInputVolume(vol);
    sceSdRemote(1, SDR_BLOCK_TRANS, AU_AUTODMA_CORE, AU_BLOCK_MODE_WRITE_FROM_LOOP, ad->iopBuff,
                ad->iopBuffSize / AU_UNIT_SIZE * AU_UNIT_SIZE, ad->iopBuff + ad->iopPausePos);
    ad->state = AU_STATE_PLAYING;
}

// CT3 0x018bc450: mute, stop the auto-DMA where it plays, and clear core 0's input area to silence.
void AA_audioDecPause(AudioDec *ad)
{
    ad->state = AU_STATE_PAUSE;
    changeInputVolume(0);
    // PORT: no 24-bit mask on the host (AA_audioDecSendToIOP).
    ad->iopPausePos =
        (int)((u_int)sceSdRemote(1, SDR_BLOCK_TRANS, AU_AUTODMA_CORE, AU_BLOCK_MODE_STOP, 0, 0) - ad->iopBuff);
    sceSdRemote(1, SDR_VOICE_TRANS, AU_AUTODMA_CORE, AU_VOICE_MODE_WRITE, ad->iopZero, AU_SPU_INPUT_CORE0,
                AU_ZERO_BUFF_SIZE);
}

// CT3 0x018bc4d0: the IOP blocks stay (audioDecInit took them once for the game).
int AA_audioDecDelete(AudioDec *ad)
{
    (void)ad;
    return 1;
}

// CT3 0x018bc4e0: a movie's decoder: the EE ring, the IOP ring's size, the two stamp rings (cleared to
// pts -1); the IOP's silence block is cleared.
int audioDecCreate(AudioDec *ad, void *buff, int buffSize, int iopBuffSize, void *eeTimeStamp,
                   int eeTimeStamps, void *iopTimeStamp, int iopTimeStamps)
{
    int i;

    ad->state = AU_STATE_INIT;
    ad->hdrCount = 0;
    ad->data = (u_char *)buff;
    ad->put = 0;
    ad->count = 0;
    ad->size = buffSize;
    ad->totalBytes = 0;
    ad->totalBytesSent = 0;
    ad->iopBuffSize = iopBuffSize;
    ad->iopLastPos = 0;
    ad->iopPausePos = 0;
    ad->eeTs = (AudioTimeStamp *)eeTimeStamp;
    ad->eeTsSize = eeTimeStamps;
    ad->eeTsCount = 0;
    ad->eeTsPut = 0;
    for (i = 0; i < ad->eeTsSize; i++) {
        ad->eeTs[i].pts = (u_long)-1;
        ad->eeTs[i].pos = 0;
        ad->eeTs[i].len = 0;
    }
    ad->iopTs = (AudioTimeStamp *)iopTimeStamp;
    ad->iopTsSize = iopTimeStamps;
    ad->iopTsCount = 0;
    ad->iopTsPut = 0;
    // The ROM clears the IOP ring for the EE ring's count (both AA_AUD_TIMESTAMPS).
    for (i = 0; i < ad->eeTsSize; i++) {
        ad->iopTs[i].pts = (u_long)-1;
        ad->iopTs[i].pos = 0;
        ad->iopTs[i].len = 0;
    }
    memset(_0_buf, 0, sizeof(_0_buf));
    sendToIOP(ad, ad->iopZero, _0_buf, AU_ZERO_BUFF_SIZE);
    return 1;
}

// CT3 0x018bc620: once for the game (AA_MovieIOPInit): the IOP ring and the silence block.
int audioDecInit(AudioDec *ad, int iopBuffSize)
{
    int ret;

    ad->iopBuff = (u_int)(uintptr_t)sceSifAllocSysMemory(0, (u_int)iopBuffSize, NULL);
    ret = (int)ad->iopBuff;
    if (ad->iopBuff == 0)
        ret = printf("Cannot allocate IOP memory\n");
    ad->iopZero = (u_int)(uintptr_t)sceSifAllocSysMemory(0, AU_ZERO_BUFF_SIZE, NULL);
    ret = (int)ad->iopZero;
    if (ad->iopZero == 0)
        ret = printf("Cannot allocate IOP memory\n");
    return ret;
}
