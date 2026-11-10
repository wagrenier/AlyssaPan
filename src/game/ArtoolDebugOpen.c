// game/ArtoolDebugOpen.c -- CT3 0x0189e180..0x0189f460: the file layer (the key is the object's first
// function; the code is the Fl* file queue). Reconstructed 2026-11-10 from the symtab's 15 functions.
//
// A file is opened with FlReqOpen (a handle), read with FlReqRead (whole sectors into a buffer) and closed
// with FlReqClose; FlProc, called once a frame by the loaders, moves one request at a time through its
// states (FL_STATE_*) and issues the sceCdRead calls. Files inside DATA\CT3PACK.DAT are found in its
// directory (gFlPackInfo, read by FlInit, sectors made absolute with the pack's own sector); loose files
// are sized and located by the IOP's file server (IOPGetFileSize). FlLoad is the blocking form.
//
// PORT: sceCdRead's destination was the buffer's uncached alias (| 0x20000000); the host passes the buffer.

#include "game/ArtoolDebugOpen.h"

#include "game/AlignedNum.h"
#include "game/CpDivideEECom.h"
#include "game/IOPGetFileSize.h"
#include "game/SdMasterVolumeSetSE.h"
#include "sdk/eekernel.h"
#include "sdk/fileio.h"
#include "sdk/libcdvd.h"
#include "sdk/libgraph.h"
#include "sdk/newlib.h"
#include "sdk/sifdev.h"
#include "sdk/sifrpc.h"

#include <stdio.h>
#include <string.h>

// ---- data ---------------------------------------------------------------------------------------------
int gHandleCounter;                              /* sbss 0x0193e310 */
static int IR;                                   /* sbss 0x0193e314: the request FlProc serves */
char gDeviceName[0x20];                          /* sbss 0x0193e320 */
int gNumFlPackFiles;                             /* sbss 0x0193e340 */
int gValIOPGetFileLen[4];                        /* sbss 0x0193e350 */
FL_REQ_RD gReqRd[FL_REQ_MAX];                    /* bss  0x0194e3e0 */
FL_PACK_INFO gFlPackInfo[FL_PACK_MAX];           /* bss  0x0194ecc0 */
alignas(64) u_int gBuf1Sect[FL_SECTOR_SIZE / 4]; /* bss  0x01957640 */

static const char s_noDir[] = "";                                      /* @449 0x01934b08 */
static const char s_version[] = ";1";                                  /* @450 0x01934b10 */
static const char s_root[] = "\\";                                     /* @451 0x01934b18 */
static const char s_packFile[] = "DATA\\CT3PACK.DAT;1";                /* @588 0x01934b90 */
static const char s_packDiscPath[] = "\\DATA\\CT3PACK.DAT;1";          /* @591 0x01934c10 */
static const char s_device[] = "cdrom0:\\";                            /* @594 0x01934c60 */
static const char s_iopImage[] = "cdrom0:\\\\IOP\\IOPRP255.IMG;1";     /* @609 0x01934ca0 */
static const char *const s_iopModules[] = {                            /* @610..@619 0x01934cc0.. */
    "\\IOP\\SIO2MAN.IRX;1", "\\IOP\\PADMAN.IRX;1",  "\\IOP\\MCMAN.IRX;1",   "\\IOP\\MCSERV.IRX;1",
    "\\IOP\\LIBSD.IRX;1",   "\\IOP\\SDRDRV.IRX;1",  "\\IOP\\MODHSYN.IRX;1", "\\IOP\\MODMIDI.IRX;1",
    "\\IOP\\MODMSIN.IRX;1", "\\IOP\\FSYS_IOP.IRX;1",
};

// The IOP's file-server channel FlProc waits on (gSCD + 0x28).
#define FL_IOP_CHANNEL (&gSCD[1])
// IOPGetFileSize's first argument as FlProc passes it.
#define FL_IOP_GET_SIZE 1

// The row of an open handle, or -1. The four handle functions inline the same search.
static int FlFindHandle(int handle)
{
    if (handle != 0) {
        for (int i = 0; i < FL_REQ_MAX; i++) {
            if (handle == gReqRd[i].handle) {
                return i;
            }
        }
    }
    return -1;
}

// CT3 0x0189e180: empty (jr ra) in the release build.
void ArtoolDebugOpen(void)
{
}

// CT3 0x0189e190: one step of the request FlGetProcessIndex picks; FL_STATE_* say what each does.
void FlProc(void)
{
    static sceCdRMode cdMode;        /* data 0x019398b0: all zero */
    static int sReadSect;            /* sbss 0x0193e31c */
    static int sDivReadWait;         /* sbss 0x0193e318 */
    FL_REQ_RD *req;

    IR = FlGetProcessIndex();
    req = &gReqRd[IR];
    switch (req->state) {
    case FL_STATE_OPEN:
        if (req->size > 0) {
            req->flags &= ~FL_REQ_OPENING;
            req->state = FL_STATE_IDLE;
        } else {
            req->state = FL_STATE_SIZE_ASK;
        }
        break;

    case FL_STATE_SIZE_ASK:
        if (IOPComSync(FL_IOP_CHANNEL) == 0) {
            IOPGetFileSize(FL_IOP_GET_SIZE, gReqRd[IR].name, gValIOPGetFileLen);
            gReqRd[IR].state++;
        }
        break;

    case FL_STATE_SIZE_WAIT:
        if (IOPComSync(FL_IOP_CHANNEL) != 0) {
            return;
        }
        req = &gReqRd[IR];
        req->size = gValIOPGetFileLen[0];
        req->baseSect = gValIOPGetFileLen[1];
        req->flags &= ~FL_REQ_OPENING;
        req->state++;
        // fall through: the request is idle now
    case 4:
    case 5:
    case 6:
    case 7:
    case FL_STATE_IDLE_LAST:
        req = &gReqRd[IR];
        if ((req->flags & FL_REQ_READING) == 0) {
            req->state++;
            break;
        }
        if (req->name[0] == '\0') {
            req->state = FL_STATE_START; // FlReqRead planned a packed file's sectors
            goto start;
        }
        req->state = FL_STATE_PLAN;
        // fall through
    case FL_STATE_PLAN:
        req = &gReqRd[IR];
        if (req->startSect == -1) {
            req->startSect = req->baseSect + req->pos;
        } else {
            req->startSect += req->baseSect;
        }
        if (req->numSect == -1) {
            req->numSect = (req->size + FL_SECTOR_SIZE - 1) / FL_SECTOR_SIZE - req->pos;
        }
        req->state++;
        // fall through
    case FL_STATE_START:
    start:
        req = &gReqRd[IR];
        req->curSect = req->startSect;
        req->remainSect = req->numSect;
        req->curBuf = req->buff;
        req->state++;
        // fall through
    case FL_STATE_READ:
        if (gADSOpenSync == 0 && sceCdSync(0) == 0) {
            sReadSect = gReqRd[IR].remainSect;
            if (CpGetEEIOPStatus()->iopBusy == 0 &&
                sceCdRead((u_int)gReqRd[IR].curSect, (u_int)sReadSect, gReqRd[IR].curBuf, &cdMode) != 0) {
                gReqRd[IR].state++;
            }
        }
        break;

    case FL_STATE_ROTATE:
        req->state = FL_STATE_IDLE;
        IR++;
        if (IR >= FL_REQ_MAX) {
            IR = 0;
        }
        break;

    case FL_STATE_READ_WAIT:
        if (sceCdSync(1) == 0) {
            SyncDCache(gReqRd[IR].curBuf, gReqRd[IR].curBuf + sReadSect * FL_SECTOR_SIZE);
            req = &gReqRd[IR];
            req->curSect += sReadSect;
            req->remainSect -= sReadSect;
            req->curBuf += sReadSect * FL_SECTOR_SIZE;
            if (req->remainSect < 1) {
                req->state = FL_STATE_READ_DONE;
            } else {
                req->state++;
            }
        }
        break;

    case FL_STATE_PAUSE:
        sDivReadWait = FL_DIV_READ_WAIT;
        req->state++;
        break;

    case FL_STATE_PAUSE_WAIT:
        sDivReadWait--;
        if (sDivReadWait < 1) {
            req->state = FL_STATE_READ;
        }
        break;

    case FL_STATE_READ_DONE:
        req->pos += req->numSect;
        req->flags &= ~FL_REQ_READING;
        req->state = FL_STATE_IDLE;
        break;

    case FL_STATE_CANCEL:
        req->flags &= ~FL_REQ_BUSY;
        req->state = FL_STATE_IDLE;
        break;

    default:
        break;
    }
}

// CT3 0x0189e780: ask for numSect sectors from startSect (relative to the file) into buff; -1 / -1 reads
// from the file's position to its end. A packed file's sectors are planned here, a loose file's by
// FlProc once its base sector is known. Answers 0, or -1 for an unknown handle.
int FlReqRead(int handle, void *buff, int startSect, int numSect)
{
    const int idx = FlFindHandle(handle);
    FL_REQ_RD *req;

    if (idx == -1) {
        printf("### ERR: FlReqRead, unknown HANDLE %d ###\n", handle); /* @403 */
        return -1;
    }
    req = &gReqRd[idx];
    req->flags |= FL_REQ_READING;
    req->buff = (u_char *)buff;
    if (req->packIdx == -1) {
        req->startSect = startSect;
        if (startSect >= 0) {
            req->pos = startSect;
        }
        req->numSect = numSect;
    } else {
        const FL_PACK_INFO *pack = &gFlPackInfo[req->packIdx];

        if (startSect >= 0) {
            req->pos = startSect;
        }
        req->baseSect = pack->sect;
        if (startSect == -1) {
            req->startSect = req->baseSect + req->pos;
        } else {
            req->startSect = startSect + req->baseSect;
        }
        if (numSect == -1) {
            if (startSect == -1) {
                startSect = req->pos;
            }
            req->numSect = pack->numSect - startSect;
        } else {
            req->numSect = numSect;
        }
    }
    return 0;
}

// CT3 0x0189e970: free a handle's row, breaking the drive's read when it is the one in flight.
void FlReqClose(int handle)
{
    const int idx = FlFindHandle(handle);

    if (idx == -1) {
        printf("### ERR: FlReqClose, unknown HANDLE %d ###\n", handle); /* @425 */
        return;
    }
    if (idx == IR && FlIsExecute(handle) != 0) {
        sceCdBreak();
    }
    memset(&gReqRd[idx], 0, sizeof(gReqRd[idx]));
}

// CT3 0x0189ea40: open dir + name (the name alone is looked up in the pack first) and answer its handle,
// or 0 when all FL_REQ_MAX rows are taken.
int FlReqOpen(const char *dir, const char *name)
{
    char upper[0x40];
    char path[0x40];
    const int idx = FlSetProcessIndex();
    int packIdx;
    FL_REQ_RD *req;

    if (idx == -1) {
        printf("### ERR: FlReqOpen, QUE is MAX %s ###\n", name); /* @448 */
        return 0;
    }
    strcpy(upper, name);
    strupr(upper);
    packIdx = FlFindPacked(upper);
    req = &gReqRd[idx];
    if (packIdx == -1) {
        if (dir == NULL) {
            dir = s_noDir;
        }
        req->size = -1;
        req->packIdx = -1;
        strcpy(path, dir);
        strupr(path);
        strcat(path, upper);
        for (int i = 0; path[i] != '\0'; i++) {
            if (path[i] == '/') {
                path[i] = '\\';
            }
        }
        strcat(path, s_version);
        strcpy(req->name, s_root);
        strcat(req->name, path);
    } else {
        req->size = gFlPackInfo[packIdx].size;
        req->packIdx = packIdx;
        req->name[0] = '\0';
    }
    req->flags = FL_REQ_OPENING;
    gHandleCounter++;
    req->handle = gHandleCounter;
    req->state = FL_STATE_OPEN;
    req->pos = 0;
    return req->handle;
}

// CT3 0x0189ec50: the row being read now, or -1.
int GetNowReadIdx(void)
{
    if (gReqRd[IR].handle == 0 || (gReqRd[IR].flags & FL_REQ_BUSY) == 0) {
        return -1;
    }
    return IR;
}

// CT3 0x0189ecb0: the first free row from IR on (wrapping), or -1.
int FlSetProcessIndex(void)
{
    int idx = IR;

    for (int tries = 0;; tries++, idx++) {
        if (idx >= FL_REQ_MAX) {
            idx = 0;
        }
        if (gReqRd[idx].handle == 0) {
            return idx;
        }
        if (tries + 1 >= FL_REQ_MAX) {
            return -1;
        }
    }
}

// CT3 0x0189ed10: the first open row with a state from IR on (wrapping), or 0.
int FlGetProcessIndex(void)
{
    int idx = IR;

    for (int tries = 0;; tries++, idx++) {
        if (idx >= FL_REQ_MAX) {
            idx = 0;
        }
        if (gReqRd[idx].handle != 0 && gReqRd[idx].state != FL_STATE_FREE) {
            return idx;
        }
        if (tries + 1 >= FL_REQ_MAX) {
            return 0;
        }
    }
}

// CT3 0x0189ed80: read the whole file into buff, a vsync per FlProc step; answers its size, or -1.
int FlLoad(const char *dir, const char *name, void *buff)
{
    const int handle = FlReqOpen(dir, name);
    int size;

    if (handle == 0) {
        return -1;
    }
    if (FlReqRead(handle, buff, -1, -1) < 0) {
        FlReqClose(handle);
        return -1;
    }
    do {
        FlProc();
        sceGsSyncV(0);
    } while (FlIsExecute(handle) != 0);
    // The ROM searches the rows again; the handle is open, so the search finds it.
    size = gReqRd[FlFindHandle(handle)].size;
    FlReqClose(handle);
    return size;
}

// CT3 0x0189ee70: 1 while the handle is opening or reading, 0 when idle, -1 for an unknown handle.
int FlIsExecute(int handle)
{
    const int idx = FlFindHandle(handle);

    if (idx == -1) {
        printf("### ERR: FlIsExecute, unknown HANDLE %d ###\n", handle); /* @520 */
        return -1;
    }
    return (gReqRd[idx].flags & FL_REQ_BUSY) != 0;
}

// CT3 0x0189ef10: a file's size in bytes, or -1. mode != 0 looks in the pack first (by name alone);
// otherwise, or when it is not there, the file is opened on the device: dir + name, gDeviceName + "\"
// in front unless the path names a device, upper case after the device, '/' -> '\', ";1" behind.
int FlGetLen(const char *dir, const char *name, int mode)
{
    char path[0x40];  // sp+0x30
    char given[0x40]; // sp+0x70
    int len;
    int at;
    int fd;
    int size;

    if (mode != 0) {
        int packIdx;

        strcpy(path, name);
        strupr(path);
        packIdx = FlFindPacked(path);
        if (packIdx >= 0) {
            return gFlPackInfo[packIdx].size;
        }
    }
    if (dir == NULL) {
        strcpy(given, name);
    } else {
        strcpy(given, dir);
        strcat(given, name);
    }
    len = (int)strlen(given);
    for (at = 0; at < len; at++) {
        if (given[at] == ':') {
            break;
        }
    }
    if (at == len) {
        strcpy(path, gDeviceName);
        strcat(path, s_root);
        at = (int)strlen(path);
        strcat(path, given);
    } else {
        at++;
        strcpy(path, given);
    }
    strupr(path + at);
    // The ROM tests the character after the one it steps over: path[at] itself is never converted.
    while (path[at++] != '\0') {
        if (path[at] == '/') {
            path[at] = '\\';
        }
    }
    strcat(path, s_version);
    fd = sceOpen(path, SCE_RDONLY);
    if (fd < 0) {
        printf("### ERR: FlGetLen, sceOpen(%s), file not found ###\n", path); /* @551 */
        return -1;
    }
    size = sceLseek(fd, 0, SCE_SEEK_END);
    sceClose(fd);
    return size;
}

// CT3 0x0189f0d0: the pack row named `name` (upper case), or -1. The first character is compared before
// strcmp, as the ROM does.
int FlFindPacked(const char *name)
{
    for (int i = 0; i < gNumFlPackFiles; i++) {
        if (name[0] == gFlPackInfo[i].name[0] && strcmp(gFlPackInfo[i].name + 1, name + 1) == 0) {
            return i;
        }
    }
    return -1;
}

// CT3 0x0189f160: read CT3PACK.DAT's directory (sector 0's header, then its sectors 1..) and make its
// sectors absolute with the pack's own sector on the disc, retrying until the disc search answers. A
// directory larger than gFlPackInfo stops the game, as the ROM does.
void FlInit(void)
{
    char path[0x40];
    sceCdlFILE file;
    int fd;
    int bytes;
    int found;

    for (;;) {
        strcpy(path, gDeviceName);
        strcat(path, s_packFile);
        fd = sceOpen(path, SCE_RDONLY);
        if (fd < 0) {
            printf("### ERR: FlInit sceOpen(%s) ###\n", path); /* @589 */
            gNumFlPackFiles = 0;
            return;
        }
        sceRead(fd, gBuf1Sect, FL_SECTOR_SIZE);
        bytes = ((int)gBuf1Sect[FL_PACK_HDR_SECTS] - 1) * FL_SECTOR_SIZE;
        if (bytes > (int)sizeof(gFlPackInfo)) {
            printf("### ERR: TOO SMALL gFlPackInfo ###\n"); /* @590 */
            sceClose(fd);
            for (;;) {
            }
        }
        sceRead(fd, gFlPackInfo, bytes);
        sceClose(fd);
        gNumFlPackFiles = (int)gBuf1Sect[FL_PACK_HDR_COUNT];
        found = sceCdSearchFile(&file, s_packDiscPath);
        if (found != 0) {
            for (int i = 0; i < (int)gBuf1Sect[FL_PACK_HDR_COUNT]; i++) {
                gFlPackInfo[i].sect += (int)file.lsn;
            }
            return;
        }
        printf("### ERR: FlInit sceCdSearchFile(%s)=%d ###\n", s_packDiscPath, found); /* @592 */
    }
}

// CT3 0x0189f2d0: gDeviceName = "cdrom0:\" (the ROM copies the 9 bytes as a doubleword and a byte).
char *SetDeviceName(void)
{
    memcpy(gDeviceName, s_device, sizeof(s_device));
    return gDeviceName;
}

// CT3 0x0189f2f0: reboot the IOP with the disc's IOPRP255.IMG, bring the drive and the file system back,
// then load the game's ten modules from \IOP\ and the sound driver's banks. The ROM reads no argument.
void IopLoadModules(void)
{
    SetDeviceName();
    sceSifInitRpc(0);
    sceCdInit(SCECdINIT);
    sceCdMmode(SCECdDVD);
    sceCdDiskReady(SCECdBlock);
    printf("REPLACE DVD BOOT IMEGAE FILE: %s\n", s_iopImage); /* @608 (sic) */
    while (sceSifRebootIop(s_iopImage) == 0) {
    }
    while (sceSifSyncIop() == 0) {
    }
    sceSifInitRpc(0);
    sceCdInit(SCECdINIT);
    sceCdMmode(SCECdDVD);
    sceCdDiskReady(SCECdBlock);
    sceFsReset();
    sceSifInitRpc(0);
    sceSifInitIopHeap();
    for (size_t i = 0; i < sizeof(s_iopModules) / sizeof(s_iopModules[0]); i++) {
        IopLoadModule(gDeviceName, s_iopModules[i]);
    }
    IOPBindRPC();
    IopLoadCapcomSound(gDeviceName);
}
