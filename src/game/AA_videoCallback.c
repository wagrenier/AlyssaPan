// game/AA_videoCallback.c -- CT3 0x018bfbf0..0x018c0310: the movie's video stream callback, its read ring
// and its stream file -- SCE's ezmpegstr sample (videoCallback, readbuf.c, strfile.c) with CT3's
// additions: a bare file name streams a CT3PACK.DAT row, and a failed disc search is retried
// AA_CdDvd_trial_Ct times before AA_strFileOpen gives up for good (StrFile.openError).
//
// PORT: the drive's stream (sceCdStInit / StStart / StRead / StStop) is sdk/libcdvd.cpp's, reading the
// disc image at the stream's cursor inside each call; the IOP block AA_strFileInit allocates is passed on
// as the ROM does and unused there.

#include "game/AA_videoCallback.h"

#include "game/AA_mpegTS.h"
#include "game/ArtoolDebugOpen.h"
#include "game/copy2area.h"
#include "game/movie.h"
#include "sdk/fileio.h"
#include "sdk/libcdvd.h"
#include "sdk/libgraph.h"
#include "sdk/sif.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define STRFILE_DEVICE_CD "cdrom0"      // the device whose files stream from the drive (@203)
#define STRFILE_PATH_MAX  0x100         // AA_strFileOpen's path buffer
#define STRFILE_DEV_MAX   0x4c          // ... and its device buffer

// CT3 0x018bfbf0: the demuxer's video callback: copies one PES payload out of the read ring (it may wrap
// the ring's end) into the video decoder's input ring (it may wrap too) and records its time stamps.
// Answers non-zero when bytes were copied.
int AA_videoCallback(sceMpeg *mp, sceMpegCbData *data, void *readBuf)
{
    ReadBuf *rb = (ReadBuf *)readBuf;
    sceMpegCbDataStr *cb = (sceMpegCbDataStr *)data;
    u_char *ringData = rb->data;
    u_char *src = cb->data;
    u_int len = cb->len;
    u_int len0;
    u_char *ptr0;
    u_char *ptr1;
    int room0;
    int room1;
    int copied;

    (void)mp;
    len0 = (u_int)(ringData + rb->size - src);     // the bytes before the ring wraps
    if (len < len0)
        len0 = len;
    AA_videoDecBeginPut(&AA_videoDec, &ptr0, &room0, &ptr1, &room1);
    // PORT: the ROM passes the destinations' uncached aliases (& 0x0fffffff | 0x20000000).
    copied = copy2area(ptr0, room0, ptr1, room1, src, (int)len0, ringData, (int)(len - len0));
    if (copied > 0 && AA_videoDecPutTs(&AA_videoDec, cb->pts, cb->dts, ptr0, copied) == 0)
        ErrMessage("pts buffer overflow\n");
    AA_videoDecEndPut(&AA_videoDec, copied);
    return copied > 0;
}

// CT3 0x018bfd00: the demuxer took `size` bytes (at most what the ring holds); answers how many.
int AA_readBufEndGet(ReadBuf *b, int size)
{
    if (b->count <= size)
        size = b->count;
    b->count -= size;
    return size;
}

// CT3 0x018bfd20: where the oldest unread byte is; answers the bytes there are to read (their run may
// wrap the ring's end: sceMpegDemuxPssRing takes the ring).
int AA_readBufBeginGet(ReadBuf *b, u_char **ptr)
{
    if (b->count != 0)
        *ptr = b->data + (b->size + (b->put - b->count)) % b->size;
    return b->count;
}

// CT3 0x018bfd60: `size` bytes were written at the put position (at most the free room); answers how many.
int AA_readBufEndPut(ReadBuf *b, int size)
{
    int room = b->size - b->count;

    if (room <= size)
        size = room;
    b->put = (b->put + size) % b->size;
    b->count += size;
    return size;
}

// CT3 0x018bfdb0: where the next bytes go; answers the free room (all of it: unlike ezmpegstr, CT3 does
// not stop at the ring's end -- readMpeg reads AA_READ_UNIT at a time, a divisor of the ring's size).
int AA_readBufBeginPut(ReadBuf *b, u_char **ptr)
{
    int room = b->size - b->count;

    if (room != 0)
        *ptr = b->data + b->put;
    return room;
}

// CT3 0x018bfde0: nothing to free (AA_MemoryMapFree frees the data).
void AA_readBufDelete(ReadBuf *b)
{
    (void)b;
}

// CT3 0x018bfdf0: empties the ring (its data was allocated by AA_MemoryMapSet).
void AA_readBufCreate(ReadBuf *b)
{
    b->count = 0;
    b->put = 0;
    b->size = AA_READBUF_SIZE;     // movie.h: the bytes AA_MemoryMapSet allocates
}

// CT3 0x018bfe10: reads `size` bytes (whole sectors from the drive); answers the bytes read, or the
// drive's negative error.
int AA_strFileRead(StrFile *file, void *buff, int size)
{
    u_int err;
    int n;

    if (file->isOnCD == STRFILE_HOST)
        return sceRead(file->fd, buff, size);
    n = sceCdStRead((u_int)size >> STRFILE_SECTOR_SHIFT, (u_int *)buff, STMBLK, &err);
    if (n < 0) {
        printf("%d=sceCdStRead()\n", n);
        return n;
    }
    return n << STRFILE_SECTOR_SHIFT;
}

// CT3 0x018bfe80: closes the file (stops the drive's stream, retrying each frame); answers 1, or 0 when
// the open had given up.
int AA_strFileClose(StrFile *file)
{
    int ret;

    if (file->openError == 1)
        return 0;
    if (file->isOnCD == STRFILE_HOST) {
        sceClose(file->fd);
    } else {
        while ((ret = sceCdStStop()) == 0) {
            printf("ERR:%d=sceCdStStop()\n", 0);
            sceGsSyncV(0);
        }
        printf("%d=sceCdStStop()\n", ret);
    }
    return 1;
}

// CT3 0x018bff20: opens a stream file. "cdrom0:\PATH\NAME.PSS" (';1' added when missing) streams from the
// drive, another "<device>:<path>" opens through the file system, a bare name streams its CT3PACK.DAT row.
// Answers 1 when open -- or when it gave up for good (openError set: the file is not there) -- and 0 for
// the caller (initAll) to try again next frame.
int AA_strFileOpen(StrFile *file, const char *filename)
{
    char path[STRFILE_PATH_MAX];
    char dev[STRFILE_DEV_MAX];
    char name[STRFILE_PATH_MAX];
    const char *colon;
    sceCdRMode mode;
    int ret;
    int i;
    int n;

    file->openError = 0;
    path[0] = '\0';         // PORT: the ROM prints the buffer unset for a bare name
    colon = strchr(filename, ':');  // the ROM calls newlib's index()
    if (colon == NULL) {
        file->isOnCD = STRFILE_PACKED;
    } else {
        n = (int)(colon - filename);
        strncpy(dev, filename, n);
        dev[n] = '\0';
        if (strcmp(dev, STRFILE_DEVICE_CD) == 0) {
            // PORT: the ROM rewrites the caller's string in place ('/' -> '\\', upper case); the copy
            // gives the same path without writing to it.
            snprintf(name, sizeof(name), "%s", colon + 1);
            n = (int)strlen(name);
            file->isOnCD = STRFILE_CD;
            for (i = 0; i < n; i++) {
                if (name[i] == '/')
                    name[i] = '\\';
                name[i] = (char)toupper(name[i]);
            }
            sprintf(path, "%s%s", name, strchr(filename, ';') != NULL ? "" : ";1");
        } else {
            file->isOnCD = STRFILE_HOST;
            sprintf(path, "%s:%s", dev, colon + 1);
        }
    }
    printf("file: %s\n", path);

    if (file->isOnCD == STRFILE_HOST) {
        file->fd = sceOpen(path, SCE_RDONLY);
        if (file->fd < 0) {
            printf("Cannot open '%s'(sceOpen)\n", path);
            file->openError = 1;
            return 1;
        }
        file->size = sceLseek(file->fd, 0, SCE_SEEK_END);
        if (file->size < 0) {
            printf("sceLseek() fails (%s): %d\n", path, file->size);
            sceClose(file->fd);
            return 0;
        }
        if (sceLseek(file->fd, 0, SCE_SEEK_SET) < 0) {
            printf("sceLseek() fails (%s)\n", path);
            sceClose(file->fd);
            return 0;
        }
        return 1;
    }

    // PORT: the IOP address is the host block's low bits (the host's sceCdStInit does not use it).
    ret = sceCdStInit(STRFILE_ST_BUF_SECTS, STRFILE_ST_BANKS, (u_int)(((uintptr_t)file->iopBuf + 0xf) & ~0xfu));
    printf("%d=sceCdStInit()\n", ret);
    if (ret == 0)
        return 0;
    printf("%d=sceCdStStop()\n", sceCdStStop());

    if (file->isOnCD == STRFILE_PACKED) {
        int row = FlFindPacked(filename);

        if (row < 0) {
            printf("Pak Cannot open '%s'(sceCdSearchFile)\n", filename);
            file->openError = 1;
            return 1;
        }
        file->fp.lsn = (u_int)gFlPackInfo[row].sect;
        file->fp.size = (u_int)gFlPackInfo[row].size;
        file->fp.flag = 0;
        file->size = gFlPackInfo[row].size;
    } else {
        // PORT: the host's sceCdlFILE is longer than CT3's; its lsn and size are what the stream needs.
        sceCdlFILE found;

        memset(&found, 0, sizeof(found));

        if (sceCdSearchFile(&found, path) == 0) {
            int err = sceCdGetError();

            printf("Nomal Cannot open '%s'(sceCdSearchFile) %d GetErr=%d\n", path, 0, err);
            if (err != SCECdErNO)
                return 0;
            if (AA_CdDvd_trial_Ct < 0) {
                file->openError = 1;
                return 1;
            }
            AA_CdDvd_trial_Ct--;
            return 0;
        }
        file->fp.lsn = found.lsn;
        file->fp.size = found.size;
        memcpy(file->fp.name, found.name, sizeof(file->fp.name));
        memcpy(file->fp.date, found.date, sizeof(file->fp.date));
        file->fp.flag = found.flag;
        file->size = (int)file->fp.size;
    }

    mode.trycount = 0;
    mode.spindlctrl = SCECdSpinMax;
    mode.datapattern = SCECdSecS2048;
    mode.pad = 0;
    while ((ret = sceCdStStart(file->fp.lsn, &mode)) == 0) {
        printf("ERR:%d=sceCdStStart()\n", 0);
        sceGsSyncV(0);
    }
    printf("%d=sceCdStStart()\n", ret);
    return 1;
}

// CT3 0x018c02d0: the IOP block the drive streams into (allocated once per AA_MemoryMapSet's call).
void AA_strFileInit(StrFile *file)
{
    file->iopBuf = sceSifAllocSysMemory(0, STRFILE_IOP_BUF_SIZE, NULL);
}
