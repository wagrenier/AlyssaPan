// game/SdMasterVolumeSetSE.c -- Clock Tower 3, the EE side of the sound driver (CT3 0x018cbf90..0x018d1cb0):
// SE banks and voices, SQ music, the ADS stream, reverb, and the FSS thread that hands every command to
// the IOP (game/CpDivideEECom.c -> SNDDRV.IRX). Slice 1 (2026-11-10): the command ring and its thread, the
// master / stream volumes, reverb, SE stop and lookup, the load-end wait, SdAlloc / SdFree, SdRnd. Slice 2
// (2026-11-10): SdProc, the SE bank loader (SdLoadSEProc, NextLoadSE), the SQ volume commands. Slice 4
// (2026-11-10): the SE stops, the special SEs' request, the load waits, the SQ requests, the stream
// request, sound on / off, stereo. Slice 5 (2026-11-10): the stream player (SdPlayADSProc), the driver's
// boot (IopLoadCapcomSound), SdSetParamSESDT, SdFindLoadSEInfo. Slice 6 (2026-11-10): the SQ loader and
// player (SdLoadSQProc). The TU is complete.
//
// Every command goes the same way: take fss_sid, fill a command record (a JSL_COM, gTransJSLCOM or
// gTransADS), queue (command word, record, reply pointer) on RingBuff with CpCall_IOPSndDriver_FIFO, and
// release fss_sid; FSS_Thread wakes, takes fss_sid, and sends the oldest entry through
// CpCall_IOPSndDriver. FSS_Thread_Init runs first thing in IopLoadCapcomSound (IopLoadModules, at boot).

#include "game/SdMasterVolumeSetSE.h"

#include "common/alyssapan_stub.h"
#include "game/ArtoolDebugOpen.h"
#include "game/CpDivideEECom.h"
#include "game/ahalloc.h"
#include "ga/gaInitInterrupt.h"
#include "game/unassigned_data.h"
#include "sdk/eekernel.h"
#include "sdk/libgraph.h"
#include "sdk/newlib.h"
#include "sdk/sdremote.h"
#include "sdk/sifdev.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ---- the TU's data (the symtab's names and addresses) -----------------------------------------------

SD_SE gSdSE[SD_SE_COUNT];                       /* bss  0x01c848c0 */
SD_SE_LD gSELd[SD_SE_LD_COUNT];                 /* bss  0x01c85400 */
SD_SQ_LD gSQLd[SD_SQ_LD_COUNT];                 /* bss  0x01943340 */
PLAY_SE_SP1 gPlaySESp1[PLAY_SE_SP1_COUNT];      /* bss  0x019436d0 */
SD_PLAY_ADS gPlayADS;                           /* bss  0x019437f0 */
int gSdADSStop;                                 /* sbss 0x01943880 */
int gSdADSPhase;                                /* sbss 0x01943884 */
int gSQi;                                       /* sbss 0x01942ee4 */
void *sqAllocPtr;                               /* sbss 0x01943330 */
int gADSOpenSync;                               /* sbss 0x01943888 */
u_int gSdEffectDepth[2];                        /* bss  0x01943890 */
int gSdEffectType[2];                           /* bss  0x01943898 */
int gUseSoundAlloc;                             /* sbss 0x019438a0 */
int gLdi;                                       /* sbss 0x019437dc: the gSELd row SdLoadSEProc steps */
int gSELdHandleCounter;                         /* sbss 0x019437e0 */
u_int gSEHeapBD;                                /* sbss 0x019437e4: the IOP block a .BD piece goes through */
u_int gSQHeapBD;                                /* sbss 0x019436c0: the IOP block for the SQ's .BD */
int gExecDriver_2;                              /* sbss 0x019437e8: frames before the next SPU2 transfer */
int gSdSEHandleCounter;                         /* sbss 0x019437d4 */
u_int gSdSEKeyOnCH;                             /* sbss 0x019437d8 */
int gSEi = -1;                                  /* sdata 0x0193a4ec */
static int sIdxUpdate;                          /* sbss 0x01942f70 (sIdxUpdate$366): SdPlaySEProc's update scan */
static u_int sBitRingCH = 1;                    /* sdata 0x0193a4f4 (sBitRingCH$971): SdSupplyCHSE's ring */
static u_int sBitLastSupplyCH;                  /* sbss 0x01942f20 (sBitLastSupplyCH$972) */
static const char s_loadMax[] = "### ERR: SdLoadSE, load max ###\n"; /* @1926 */

static int sTransedSize;                        /* sbss 0x01942fbc (sTransedSize$226): .BD bytes sent */
static int sTransSize;                          /* sbss 0x01942fb8 (sTransSize$227): this piece's bytes */
static char sFname[0x38];                       /* bss  0x01942f80 (sFname$228) */

static const char s_sdt[] = ".SDT";             /* @2676 */
static const char s_hd[] = ".HD";               /* @2328 */
static const char s_bd[] = ".BD";               /* @2331 */
static const char s_allocSdt[] = "SD1";         /* @2677: SdAlloc's names (unread) */
static const char s_allocHd[] = "SD2";          /* @2678 */
static const char s_allocBd[] = "SD3";          /* @2679 */
static const char s_errDma[] = "### ERR: SdLoadSEProc, CpSifDmaTransEEToIOP ###\n"; /* @2330 */
static const char s_errTrans[] = "### ERR: SdLoadSEProc, sceSdTransToIOP ###\n";    /* @2680 */

// The .SDT: 16-byte rows; the loader stamps the slot into byte 1 and clears byte 4's top bit.
#define SDT_ROW_SIZE 0x10
#define SDT_ROW_JLS 1
#define SDT_ROW_FLAGS 4
#define SDT_ROW_FLAG_80 0x80
#define SD_BD_PIECE 0x32000                     /* .BD bytes per SPU2 transfer */
#define SD_SPU_TRANS_WAIT 0x3c                  /* frames between two transfers (gExecDriver_2) */
#define SD_SPU_TRANS_ALIGN 0x40
#define SD_FILE_ALIGN 0x800                     /* SdAlloc's alignment for a file */
#define SD_SE_RESIDENT 1000                     /* an SE number from here on survives its bank's reload */
#define SD_SQ_VOL_COM_ARG0 0xff                 /* SdVolumeSetSQ's arg[0] */

static SND_JSL_COM seStopJSLCOM;                /* bss  0x01942f00 (seStopJSLCOM$1185) */
static SND_JSL_COM effectJSLCOM;                /* bss  0x01943000 (effectJSLCOM$121) */
static u_int sNext = 0x12345678;                        /* sdata 0x0193a4f0 (sNext$117): SdRnd's state */

// The command ring: 64 entries of (command, record, reply). layout: host (EE: 12 bytes, pointers as words).
typedef struct FSS_COMMAND {
    u_int command;          // EE +0 the driver's command word (SND_CMD_*)
    void *data;             // EE +4 its record
    int *reply;             // EE +8 != NULL: FSS_Thread stores the driver's reply word here
} FSS_COMMAND;
#define FSS_RING_COUNT 0x40
#define FSS_RING_MASK (FSS_RING_COUNT - 1)

static FSS_COMMAND RingBuff[FSS_RING_COUNT];    /* bss  0x01943030 (768 bytes) */
static u_int wp;                                /* sbss 0x01942ee8: the next entry to fill */
static u_int rp;                                /* sbss 0x01942eec: the next entry to send */
static int fss_sid;                             /* sbss 0x01943020: guards the records and the ring */
static int fss_tid;                             /* sbss 0x01943024 */

#define FSS_STACK_SIZE 0x1000
#define FSS_THREAD_PRIORITY 2
#define FSS_INIT_CALLER_PRIORITY 3
alignas(16) static u_char fss_stack[FSS_STACK_SIZE]; /* bss 0x01c838c0 (fss_stack$75) */

static const char s_loadSEEndErr[] = "### ERR: SdLoadSEEnd(%d) ###\n"; /* data (@1887) */

static void SdStopSECH(int chBits, int mode);
static int SdScaleVolume(int vol);
static void CpCall_IOPSndDriver_FIFO(u_int command, void *data, int *reply);
static void FSS_Thread(void *arg);
static void SdLoadSEProc(void);
static void SdLoadSQProc(void);
static void SdPlaySEProc(void);
static void SdPlaySESpProc(void);
static void SdPlayADSProc(void);
static int NextLoadSE(int from);
static int NextPlaySE(int from);
static void SdPlaySE(SD_SE *se);
static void SdPlaySEUpdate(SD_SE *se);
static int FindSEFile(const char *name);
static int FindSEFile_JLS_SPU2(int jls, u_int spuAddr);

// CT3 0x018cbf90
void SdMasterVolumeSetSE(int vol)
{
    WaitSema(fss_sid);
    gTransJSLCOM.arg[0] = vol;
    CpCall_IOPSndDriver_FIFO(SND_CMD_MASTER_VOL_SE, &gTransJSLCOM, NULL);
    SignalSema(fss_sid);
}

// CT3 0x018cc0f0
void SdStopSE(int handle)
{
    SD_SE *se = SdFindPlaySE(handle);
    if (se != NULL) {
        SdStopSECH(se->chBits, 1);
        se->handle = 0;
    }
}

// CT3 0x018cc130
static void SdStopSECH(int chBits, int mode)
{
    WaitSema(fss_sid);
    seStopJSLCOM.arg[0] = 0;
    seStopJSLCOM.arg[2] = (int)(mode == 0 ? SD_SECH_STOP_WORD_0 : SD_SECH_STOP_WORD_1);
    seStopJSLCOM.arg[1] = chBits;
    CpCall_IOPSndDriver_FIFO(SND_CMD_STOP_SE_CH, &seStopJSLCOM, NULL);
    SignalSema(fss_sid);
}

// CT3 0x018ccde0
SD_SE *SdFindPlaySE(int handle)
{
    if (handle != 0) {
        for (int i = 0; i < SD_SE_COUNT; i++) {
            if (handle == gSdSE[i].handle) {
                return &gSdSE[i];
            }
        }
    }
    return NULL;
}

// CT3 0x018cd550 -- the next load to step after row `from`: among the rows waiting to start
// (SD_SE_LD_OPEN_SDT), the oldest handle newer than `from`'s, else row 0.
static int NextLoadSE(int from)
{
    int cur = from;
    int next = 0;
    int best = 0x7fffffff;
    int at = from;
    for (int n = 0; n < SD_SE_LD_COUNT; n++) {
        if (++at > SD_SE_LD_COUNT - 1) {
            at = 0;
        }
        int handle = gSELd[at].handle;
        if (handle != 0 && gSELd[at].step == SD_SE_LD_OPEN_SDT && gSELd[cur].handle < handle && handle < best) {
            best = handle;
            next = at;
        }
    }
    return next;
}

// CT3 0x018cda30
void SdLoadSEEndWait(int handle)
{
    while (SdLoadSEEnd(handle) == 0) {
        FlProc();
        SdProc();
        sceGsSyncV(0);
    }
}

// CT3 0x018cda90 -- 1 once the bank load `handle` has finished; 0 while loading, or (with a message) when
// no slot holds that handle.
int SdLoadSEEnd(int handle)
{
    for (int i = 0; i < SD_SE_LD_COUNT; i++) {
        if (handle == gSELd[i].handle) {
            return gSELd[i].step == SD_SE_LD_IDLE;
        }
    }
    printf(s_loadSEEndErr, handle);
    return 0;
}

// CT3 0x018cddd0
void SdStopStream(int a0)
{
    gPlayADS.stopArg = a0;
    gSdADSStop = 1;
}

// CT3 0x018cdde0
void SdVolumeStream(int vol)
{
    gPlayADS.vol = vol;
    u_int scaled = (u_int)SdScaleVolume(vol);
    WaitSema(fss_sid);
    gTransADS.vol = scaled | scaled << 16;
    CpCall_IOPSndDriver_FIFO(SND_CMD_ADS_VOLUME, &gTransADS, NULL);
    SignalSema(fss_sid);
}

// CT3 0x018cdf50
int SdIsPlayingStream(void)
{
    EE_IOP_STAT *stat = CpGetEEIOPStatus();
    return stat->adsActive != 0 || stat->iopBusy != 0;
}

// CT3 0x018cdf90
void SdMasterVolSetADS(int vol)
{
    gMasterVolADS = vol;
    if (SdIsPlayingStream()) {
        SdVolumeStream(gPlayADS.vol);
    }
}

// CT3 0x018cdfc0
static int SdScaleVolume(int vol)
{
    return (vol * gMasterVolADS) / 0xff;
}

// CT3 0x018cdff0
void SdVolumeSetSQ(int player, int vol)
{
    WaitSema(fss_sid);
    SND_JSL_COM *com = &gSQLd[player].volCom;
    com->unk00 = player;
    com->arg[0] = SD_SQ_VOL_COM_ARG0;
    com->arg[1] = vol;
    CpCall_IOPSndDriver_FIFO(SND_CMD_VOLUME_SQ, com, NULL);
    SignalSema(fss_sid);
}

// CT3 0x018ce080
void SdMasterVolumeSetSQ(int vol)
{
    WaitSema(fss_sid);
    gTransJSLCOM.unk00 = 0;
    gTransJSLCOM.arg[0] = vol;
    CpCall_IOPSndDriver_FIFO(SND_CMD_MASTER_VOL_SQ, &gTransJSLCOM, NULL);
    SignalSema(fss_sid);
}

// The open request of `fd` (gReqRd's row), as SdLoadSEProc inlines the file layer's search. A handle no
// row holds makes the ROM read the row before gReqRd (index -1); the port answers NULL instead.
static const FL_REQ_RD *SdFindReq(int fd)
{
    if (fd != 0) {
        for (int i = 0; i < FL_REQ_MAX; i++) {
            if (fd == gReqRd[i].handle) {
                return &gReqRd[i];
            }
        }
    }
    return NULL;
}

// Steps 1, 4, 7: open NAME<ext>, and go on once the file layer took the request.
static void SdLoadSEOpen(SD_SE_LD *ld, const char *ext)
{
    char path[0x40];
    strcpy(path, ld->name);
    strcat(path, ext);
    ld->fd = FlReqOpen(ld->dir, path);
    if (ld->fd != 0) {
        ld->step++;
    }
}

// Steps 2, 5, 8: once the size is known, a buffer for the whole file and the read.
static void SdLoadSERead(SD_SE_LD *ld, int *size, const char *allocName)
{
    const FL_REQ_RD *req = SdFindReq(ld->fd);
    int fileSize = req != NULL ? req->size : -1;
    if (fileSize > 0) {
        *size = fileSize;
        ld->buf = SdAlloc(*size, SD_FILE_ALIGN, allocName);
        if (ld->buf != NULL) {
            FlReqRead(ld->fd, ld->buf, -1, -1);
            ld->step++;
        }
    }
}

// Step 3's stamp: the slot into every row of the table.
static void SdStampSDT(SD_SE_LD *ld, int bytes)
{
    u_char *row = (u_char *)ld->buf;
    for (int i = 0; i < bytes / SDT_ROW_SIZE; i++, row += SDT_ROW_SIZE) {
        row[SDT_ROW_JLS] = (u_char)ld->jls;
        row[SDT_ROW_FLAGS] &= (u_char)~SDT_ROW_FLAG_80;
    }
}

static u_int SdIopHeapAlloc(int size)
{
    return (u_int)(uintptr_t)sceSifAllocIopHeap((u_int)(size + 0xf) & ~0xfu);
}

static int SdSendToIop(const SD_SE_LD *ld, u_int iopAddr, int size)
{
    return CpSifDmaTransEEToIOP(ld->buf, iopAddr, (u_int)(size + 0xf) & ~0xfu, 0, 0);
}

// CT3 0x018cfe40 -- one step of the SE bank load gLdi (SD_SE_LD_STEP says what each does). The ROM
// re-reads gSELd[gLdi] after every call; nothing else moves gLdi, so `ld` stands for it.
static void SdLoadSEProc(void)
{
    SD_SE_LD *ld = &gSELd[gLdi];
    switch (ld->step) {
    case SD_SE_LD_IDLE:
        gLdi = NextLoadSE(gLdi);
        break;

    case SD_SE_LD_OPEN_SDT:
        if (ld->stopPlaying == SD_SE_LD_STOP_PLAYING) {
            for (int i = 0; i < SD_SE_COUNT; i++) {
                if (gSdSE[i].handle != 0 && gSdSE[i].jls == ld->jls && gSdSE[i].reqArg2 < SD_SE_RESIDENT) {
                    SdStopSE(gSdSE[i].handle);
                }
            }
            for (int i = 0; i < PLAY_SE_SP1_COUNT; i++) {
                PLAY_SE_SP1 *sp = &gPlaySESp1[i];
                if (sp->active == ld->jls && sp->seNo < SD_SE_RESIDENT) {
                    sp->active = 0;
                }
            }
        }
        SdLoadSEOpen(ld, s_sdt);
        break;

    case SD_SE_LD_READ_SDT:
        SdLoadSERead(ld, &ld->sdtSize, s_allocSdt);
        break;

    case SD_SE_LD_SEND_SDT: {
        if (FlIsExecute(ld->fd) != 0) {
            break;
        }
        const FL_REQ_RD *req = SdFindReq(ld->fd);
        int packIdx = req != NULL ? req->packIdx : -1;
        if (packIdx < 0) {
            // A loose file: the .SDT alone.
            SdStampSDT(ld, ld->sdtSize);
            ld->iopSdt = SdIopHeapAlloc(ld->sdtSize);
            if (ld->iopSdt == 0) {
                ld->step = SD_SE_LD_ERROR;
            } else if (SdSendToIop(ld, ld->iopSdt, ld->sdtSize) == 0) {
                ld->step++;
            } else {
                printf(s_errDma);
                ld->step = SD_SE_LD_ERROR;
            }
        } else {
            // A packed .SDT carries its .HD: both go up in one block, the HD steps are skipped.
            int part = gFlPackInfo[packIdx].partSize;
            SdStampSDT(ld, part);
            ld->iopSdt = SdIopHeapAlloc(ld->sdtSize);
            if (ld->iopSdt == 0) {
                ld->step = SD_SE_LD_ERROR;
            } else if (SdSendToIop(ld, ld->iopSdt, ld->sdtSize) == 0) {
                ld->hdSize = ld->sdtSize - part;
                ld->sdtSize = part;
                ld->iopHd = ld->iopSdt + (u_int)ld->sdtSize;
                ld->step = SD_SE_LD_OPEN_BD;
            } else {
                printf(s_errDma);
                ld->step = SD_SE_LD_ERROR;
            }
        }
        SdFree(&ld->buf);
        FlReqClose(ld->fd);
        ld->fd = 0;
        break;
    }

    case SD_SE_LD_OPEN_HD:
        SdLoadSEOpen(ld, s_hd);
        break;

    case SD_SE_LD_READ_HD:
        SdLoadSERead(ld, &ld->hdSize, s_allocHd);
        break;

    case SD_SE_LD_SEND_HD:
        if (FlIsExecute(ld->fd) != 0) {
            break;
        }
        FlReqClose(ld->fd);
        ld->fd = 0;
        ld->iopHd = SdIopHeapAlloc(ld->hdSize);
        if (ld->iopHd == 0) {
            ld->step = SD_SE_LD_ERROR; // the buffer stays: the error step frees it
            break;
        }
        if (SdSendToIop(ld, ld->iopHd, ld->hdSize) == 0) {
            ld->step++;
        } else {
            printf(s_errDma);
            ld->step = SD_SE_LD_ERROR;
        }
        SdFree(&ld->buf);
        break;

    case SD_SE_LD_OPEN_BD:
        SdLoadSEOpen(ld, s_bd);
        break;

    case SD_SE_LD_READ_BD:
        SdLoadSERead(ld, &ld->bdSize, s_allocBd);
        break;

    case SD_SE_LD_BD_READ_WAIT:
        if (FlIsExecute(ld->fd) != 0) {
            return;
        }
        FlReqClose(ld->fd);
        ld->fd = 0;
        sTransedSize = 0;
        ld->step++;
        // fall through: the first piece goes this frame
    case SD_SE_LD_BD_TRANS:
        ld->iopBd = gSEHeapBD;
        sTransSize = ld->bdSize - sTransedSize;
        if (sTransSize > SD_BD_PIECE) {
            sTransSize = SD_BD_PIECE;
        }
        if (sceSdTransToIOP((u_char *)ld->buf + sTransedSize, gSEHeapBD, (u_int)sTransSize, 1) == 0) {
            ld->step++;
        } else {
            printf(s_errTrans);
            ld->step = SD_SE_LD_ERROR;
        }
        break;

    case SD_SE_LD_BD_SPU:
        if (gExecDriver_2 < 1) {
            WaitSema(fss_sid);
            ld->spuCom.arg[0] = (int)gSEHeapBD;
            ld->spuCom.arg[1] = sTransedSize + (int)ld->spuAddr;
            ld->spuCom.arg[2] = (int)((u_int)(sTransSize + SD_SPU_TRANS_ALIGN - 1) & ~(u_int)(SD_SPU_TRANS_ALIGN - 1));
            CpCall_IOPSndDriver_2(SND_CMD_TRANS_SPU2, &ld->spuCom);
            SignalSema(fss_sid);
            gExecDriver_2 = SD_SPU_TRANS_WAIT;
            ld->step++;
        } else {
            gExecDriver_2--;
        }
        break;

    case SD_SE_LD_BD_SPU_WAIT:
        if (CpCheckEE_IOPComEnd(SND_CMD_TRANS_SPU2, 1) == 0) {
            gExecDriver_2 = 0;
            sTransedSize += sTransSize;
            if (sTransedSize < ld->bdSize) {
                ld->step = SD_SE_LD_BD_TRANS;
            } else {
                SdFree(&ld->buf);
                ld->step++;
            }
        }
        break;

    case SD_SE_LD_REGISTER: {
        char path[0x40];
        strcpy(path, ld->dir);
        for (int i = 0; path[i] != '\0'; i++) {
            if (path[i] == '/') {
                path[i] = '\\';
            }
        }
        strcat(path, ld->name);
        strupr(path);
        strcpy(sFname, gDeviceName);
        strcat(sFname, path);
        WaitSema(fss_sid);
        strcpy(ld->reg.path, sFname);
        ld->reg.iopHd = ld->iopHd;
        ld->reg.iopBd = ld->iopBd;
        ld->reg.spuAddr = ld->spuAddr;
        ld->reg.zero = 0;
        ld->reg.iopSdt = ld->iopSdt;
        ld->reg.bdSize = ld->bdSize;
        CpCall_IOPSndDriver_FIFO((u_int)ld->jls | SND_CMD_SE_REGISTER, &ld->reg, NULL);
        SignalSema(fss_sid);
        ld->step = SD_SE_LD_DONE;
    }
        // fall through
    case SD_SE_LD_DONE_14:
    case SD_SE_LD_DONE_15:
    case SD_SE_LD_DONE:
        ld->flags |= SD_SE_LD_FLAG_LOADED;
        ld->step = SD_SE_LD_IDLE;
        break;

    case SD_SE_LD_ERROR:
    case SD_SE_LD_ERROR_16:
        if (ld->fd != 0) {
            FlReqClose(ld->fd);
        }
        if (ld->iopHd != 0) {
            sceSifFreeIopHeap((void *)(uintptr_t)ld->iopHd);
            ld->iopHd = 0;
        }
        if (ld->iopSdt != 0) {
            sceSifFreeIopHeap((void *)(uintptr_t)ld->iopSdt);
            ld->iopSdt = 0;
        }
        SdFree(&ld->buf);
        ld->step = SD_SE_LD_IDLE;
        ld->handle = 0;
        break;
    }
}

// CT3 0x018d1390 -- the driver's frame: one step of each loader and player (FileSoundServer, and the
// wait loops).
void SdProc(void)
{
    SdLoadSEProc();
    SdLoadSQProc();
    SdPlaySEProc();
    SdPlaySESpProc();
    SdPlayADSProc();
}

// CT3 0x018d1600 -- sets core `core`'s reverb, only when type or depth changed.
void SdSetEffect(int core, int type, int depth)
{
    if (depth > SD_EFFECT_DEPTH_MAX) {
        depth = SD_EFFECT_DEPTH_MAX;
    }
    u_int depthWord = (u_int)depth << 7 | 0x7f;
    if (type != gSdEffectType[core] || depthWord != gSdEffectDepth[core]) {
        SdSetEffectDontSave(core, type, (int)depthWord);
        gSdEffectType[core] = type;
        gSdEffectDepth[core] = depthWord;
    }
}

// CT3 0x018d16a0 -- sends the reverb command without remembering it (SdSetEffect remembers).
void SdSetEffectDontSave(int core, int type, int depthWord)
{
    if (depthWord > SD_EFFECT_DEPTH_LIMIT) {
        depthWord = 0;
    }
    if (core == SD_EFFECT_CORE_1) {
        switch (type) {
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            type = SD_EFFECT_TYPE_CORE1_MAX;
            break;
        }
    }
    WaitSema(fss_sid);
    effectJSLCOM.arg[2] = depthWord;
    effectJSLCOM.arg[4] = type != gSdEffectType[core];
    effectJSLCOM.arg[0] = core;
    effectJSLCOM.arg[1] = type;
    effectJSLCOM.arg[3] = depthWord;
    CpCall_IOPSndDriver_FIFO(SND_CMD_EFFECT, &effectJSLCOM, NULL);
    SignalSema(fss_sid);
}

// CT3 0x018d1770 -- the driver's own linear congruential generator.
u_int SdRnd(void)
{
    sNext = sNext * 0x7d2b89dd + 0xbc614e;
    return sNext & 0x7fffffff;
}

// CT3 0x018d17a0
void SdFree(void **mem)
{
    if (mem != NULL && *mem != NULL) {
        gUseSoundAlloc = 0;
        _SunFree(&gHeapSound, mem);
    }
}

// CT3 0x018d17d0 -- the sound heap's single block: NULL while it is out. The ROM passes only size and
// align on; `name` is unread.
void *SdAlloc(int size, int align, const char *name)
{
    (void)name;
    if (gUseSoundAlloc == 0) {
        gUseSoundAlloc = 1;
        return _SunAlloc(&gHeapSound, size, align);
    }
    return NULL;
}

// CT3 0x018d1b00 -- waits until the semaphore is free and the ring is empty.
void FSS_Thread_Sync(void)
{
    SemaParam info;
    do {
        do {
            ReferSemaStatus(fss_sid, &info);
        } while (info.currentCount == 0);
    } while (rp != wp);
}

// CT3 0x018d1b40
void FSS_Thread_Init(void)
{
    ChangeThreadPriority(GetThreadId(), FSS_INIT_CALLER_PRIORITY);

    SemaParam sema = {};
    sema.initCount = 1;
    sema.maxCount = 1;
    sema.option = 0;
    fss_sid = CreateSema(&sema);

    ThreadParam thread = {};
    thread.entry = FSS_Thread;
    thread.stack = fss_stack;
    thread.stackSize = FSS_STACK_SIZE;
    thread.gpReg = NULL; // the ROM passes &_gp
    thread.initPriority = FSS_THREAD_PRIORITY;
    fss_tid = CreateThread(&thread);
    StartThread(fss_tid, NULL);
}

// CT3 0x018d1bd0
static void FSS_Thread(void *arg)
{
    (void)arg;
    for (;;) {
        do {
            SleepThread();
        } while (rp == wp);
        WaitSema(fss_sid);
        u_int i = rp;
        int *reply = CpCall_IOPSndDriver(RingBuff[i].command, RingBuff[i].data);
        if (RingBuff[i].reply != NULL) {
            *RingBuff[i].reply = *reply;
        }
        rp = (i + 1) & FSS_RING_MASK;
        SignalSema(fss_sid);
    }
}

// CT3 0x018d1c60 -- queues one command (the caller holds fss_sid) and wakes FSS_Thread.
static void CpCall_IOPSndDriver_FIFO(u_int command, void *data, int *reply)
{
    RingBuff[wp].command = command;
    RingBuff[wp].data = data;
    RingBuff[wp].reply = reply;
    wp = (wp + 1) & FSS_RING_MASK;
    WakeupThread(fss_tid);
}

// ---- slice 4 (2026-11-10): SE stops, the special SEs, load waits, SQ requests, the stream request, sound
// on / off, stereo ------------------------------------------------------------------------------------------

static const char s_adsDir[] = "DATA\\SOUND\\ADS\\";      /* @1943: a stream whose name starts with a digit */
static const char s_adsDir2[] = "DATA\\SOUND\\ADS2\\";    /* @1944: the others */
static const char s_sqAny[] = "ANY";                      /* @1984: SdVolumeSQ's "whatever row 0 plays" */
#define SD_SQ_ANY_LEN 3
#define SD_SQ_STOP_NOW_ARG1 0xb                           /* SND_CMD_STOP_SQ_NOW's arg[1] */
#define SD_SQ_STOP_NOW_FRAMES 10                          /* frames SdStopSQAllNow waits before freeing */
#define SD_SOUND_ENABLE_SPIN 999999                       /* SdSoundEnable's wait for EE_IOP_SND_READY */
#define SD_SOUND_ENABLE_FRAMES 7
#define SD_ADS_DISABLE_FRAMES 100                         /* SdSoundDisable's wait for the stream to end */
#define SD_ADS_PHASE_REQUEST 1

// CT3 0x018cbfe0 -- silences every channel and frees every SE row.
void SdStopSEAll(void)
{
    SdStopSECH(SD_SE_CH_ALL, 1);
    for (int i = 0; i < SD_SE_COUNT; i++) {
        if (gSdSE[i].handle != 0) {
            gSdSE[i].handle = 0;
        }
    }
}

// CT3 0x018cc040 -- stops the SEs seNo of slot jls (reqArg2 0: all of them, else only those requested with
// it), with one stop command for their channels.
void SdStopSESDT(int jls, int seNo, int reqArg2)
{
    u_int chBits = 0;
    for (int i = 0; i < SD_SE_COUNT; i++) {
        SD_SE *se = &gSdSE[i];
        if (se->handle != 0 && jls == se->jls && seNo == se->seNo) {
            if (reqArg2 == 0 || reqArg2 == se->reqArg2) {
                se->handle = 0;
                chBits |= (u_int)se->chBits;
            }
        }
    }
    if (chBits != 0) {
        SdStopSECH((int)chBits, 1);
    }
}

// CT3 0x018cc1b0
void SdStopSEAllSp1(void)
{
    for (int i = 0; i < PLAY_SE_SP1_COUNT; i++) {
        if (gPlaySESp1[i].active != 0) {
            SdStopSE(gPlaySESp1[i].handle);
            gPlaySESp1[i].active = 0;
        }
    }
}

// CT3 0x018cc220 -- frees the special SE rows playing `handle` (the ROM stops the SE once per row found).
void SdStopSESp1ByHandle(int handle)
{
    for (int i = 0; i < PLAY_SE_SP1_COUNT; i++) {
        if (handle == gPlaySESp1[i].handle) {
            SdStopSE(handle);
            gPlaySESp1[i].active = 0;
        }
    }
}

// CT3 0x018cc2a0 -- stops the special SEs (jls, seNoBase, seNo). The ROM sets no return value (v0 is
// whatever the loop left); SSStopSESp1 passes it on, so the port answers 0.
int SdStopSESp1(int jls, int seNoBase, int seNo)
{
    for (int i = 0; i < PLAY_SE_SP1_COUNT; i++) {
        PLAY_SE_SP1 *sp = &gPlaySESp1[i];
        if (jls == sp->active && seNoBase == sp->seNoBase && seNo == sp->seNo) {
            SdStopSE(sp->handle);
            sp->active = 0;
        }
    }
    return 0;
}

// CT3 0x018cc350 -- a special SE: plays seNoBase + SdRnd() % seNoRange now and again every `interval`
// ticks (SdPlaySESpProc). With an object it is positional: the parameters go to the SE and `update` runs
// once now. Nothing when all five rows are taken.
void SdReqPlaySESp1(int jls, int seNoBase, int seNo, int seNoRange, int interval, int objId, int bank,
                    SD_SE_UPDATE update, int posVol, int posMode)
{
    int i;
    for (i = 0; gPlaySESp1[i].active != 0; i++) {
        if (i + 1 > PLAY_SE_SP1_COUNT - 1) {
            return;
        }
    }
    PLAY_SE_SP1 *sp = &gPlaySESp1[i];
    sp->active = jls;
    sp->seNoBase = seNoBase;
    sp->seNo = seNo;
    sp->posMode = posMode;
    sp->objId = objId;
    sp->bank = bank;
    sp->update = objId == 0 ? NULL : update;
    sp->seNoRange = seNoRange;
    sp->posVol = posVol;
    sp->interval = interval;
    sp->lastTime = ga_main_timer;
    sp->stopped = 0;
    int pick = (int)SdRnd() % sp->seNoRange;
    sp->handle = SdReqPlaySE(jls, seNoBase + pick, seNo);
    if (objId == 0) {
        return;
    }
    SdSetParamSE(sp->handle, SD_SE_PARAM_POS_VOL | SD_SE_PARAM_POS_MODE | SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK |
                                 SD_SE_PARAM_CALLBACK,
                 posVol, posMode, objId, bank, update);
    update(sp->handle);
}

// CT3 0x018cd820 -- the gSELd row holding the loaded bank `name`, or -1 (not there, or still loading).
int IsPlaySEReady(const char *name)
{
    for (int i = 0; i < SD_SE_LD_COUNT; i++) {
        if (gSELd[i].handle != 0 && strcmp(name, gSELd[i].name) == 0 && gSELd[i].step == SD_SE_LD_IDLE) {
            return i;
        }
    }
    return -1;
}

// CT3 0x018cdb10 -- runs the file and sound servers, a frame at a time, until every bank load is through.
void SdLoadSEEndWaitAll(void)
{
    for (int i = 0; i < SD_SE_LD_COUNT; i++) {
        if (gSELd[i].handle != 0) {
            while (gSELd[i].step != SD_SE_LD_IDLE) {
                FlProc();
                SdProc();
                sceGsSyncV(0);
            }
        }
    }
}

// CT3 0x018cde40 -- streams the ADS `name` (SdPlayADSProc opens it): a digit-first name lives in ADS\, the
// others in ADS2\. The stream already playing goes on untouched; another one is stopped first.
void SdPlayStream(const char *name, int vol, int a2, int loop)
{
    char upper[0x40];

    strcpy(upper, name);
    strupr(upper);
    strcpy(gTransADS.path, gDeviceName);
    strcat(gTransADS.path, (upper[0] < '0' || '9' < upper[0]) ? s_adsDir2 : s_adsDir);
    strcat(gTransADS.path, upper);
    gPlayADS.loop = loop;
    gPlayADS.arg2 = a2;
    if (SdIsPlayingStream()) {
        if (strcmp(upper, gPlayADS.name) == 0) {
            return;
        }
        if (upper[0] != '\0') {
            CpCall_IOPSndDriver_FIFO(SND_CMD_ADS_STOP, NULL, NULL);
        }
    }
    strcpy(gPlayADS.name, upper);
    gSdADSPhase = SD_ADS_PHASE_REQUEST;
    gSdADSStop = 0;
    gPlayADS.vol = vol;
}

// The gSQLd row playing `upper`, or -1.
static int SdFindSQ(const char *upper)
{
    for (int i = 0; i < SD_SQ_LD_COUNT; i++) {
        if (strcmp(upper, gSQLd[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

// CT3 0x018ce0d0 -- moves the SQ `name` ("ANY": whatever row 0 plays) to `vol`, gradually with fade.
void SdVolumeSQ(const char *name, int vol, int fade)
{
    char upper[0x40];

    strcpy(upper, name);
    strupr(upper);
    if (memcmp(upper, s_sqAny, SD_SQ_ANY_LEN) == 0) {
        if (gSQLd[0].name[0] == '\0') {
            return;
        }
        strcpy(upper, gSQLd[0].name);
    }
    int i = SdFindSQ(upper);
    if (i < 0) {
        return;
    }
    gSQLd[i].vol = vol;
    gSQLd[i].flags |= SD_SQ_FLAG_VOLUME;
    if (fade != 0) {
        gSQLd[i].flags |= SD_SQ_FLAG_FADE;
    }
}

// CT3 0x018ce1e0 -- stops the SQ at once: waits for the loads, tells the driver, gives it ten frames, frees
// gSQi's IOP blocks and the SQ buffer, and clears both rows.
void SdStopSQAllNow(void)
{
    SdLoadSQEndWaitAll();
    if (gSQLd[0].name[0] != '\0') {
        WaitSema(fss_sid);
        SND_JSL_COM *com = &gSQLd[0].volCom;
        com->arg[1] = SD_SQ_STOP_NOW_ARG1;
        com->unk00 = 0;
        com->arg[0] = 0;
        CpCall_IOPSndDriver_FIFO(SND_CMD_STOP_SQ_NOW, com, NULL);
        SignalSema(fss_sid);
        for (int n = 0; n < SD_SQ_STOP_NOW_FRAMES; n++) {
            sceGsSyncV(0);
        }
        SD_SQ_LD *sq = &gSQLd[gSQi];
        if (sq->iopHd != 0) {
            sceSifFreeIopHeap((void *)(uintptr_t)sq->iopHd);
            sq->iopHd = 0;
        }
        if (sq->iopSq != 0) {
            sceSifFreeIopHeap((void *)(uintptr_t)sq->iopSq);
            sq->iopSq = 0;
        }
        SdFree(&sqAllocPtr);
    }
    memset(gSQLd, 0, sizeof gSQLd);
}

// CT3 0x018ce310 -- stops both players (fade: to volume 0 first); row 1 is dropped at once.
void SdStopSQAll(int fade)
{
    for (int i = 0; i < SD_SQ_LD_COUNT; i++) {
        SD_SQ_LD *sq = &gSQLd[i];
        if (sq->name[0] != '\0') {
            sq->flags |= SD_SQ_FLAG_STOP;
            if (fade != 0) {
                sq->flags |= SD_SQ_FLAG_VOLUME | SD_SQ_FLAG_FADE;
                sq->vol = 0;
            }
            if (i != 0) {
                sq->name[0] = '\0';
                sq->step = 0;
            }
        }
    }
}

// CT3 0x018ce380 -- stops the SQ `name`; in row 1 it is dropped at once.
void SdStopSQ(const char *name)
{
    char upper[0x40];

    strcpy(upper, name);
    strupr(upper);
    int i = SdFindSQ(upper);
    if (i < 0) {
        return;
    }
    gSQLd[i].flags |= SD_SQ_FLAG_STOP;
    if (i != 0) {
        gSQLd[i].name[0] = '\0';
        gSQLd[i].step = 0;
    }
}

// CT3 0x018ce440 -- plays the SQ dir\name at vol. The SQ row 0 already plays (still before
// SD_SQ_LD_STEP_REPLAY_MAX) keeps playing, at the new volume; otherwise the request goes into row 0, or into
// row 1 while row 0 is told to stop. Answers 1 when it went into row 1.
int SdPlaySQ(const char *dir, const char *name, u_int spuAddr, int vol, int effectDelay, int effectType,
             int effectDepth)
{
    char upper[0x40];
    SD_SQ_LD *sq = &gSQLd[0];

    strcpy(upper, name);
    strupr(upper);
    if (strcmp(upper, sq->name) == 0 && sq->step < SD_SQ_LD_STEP_REPLAY_MAX) {
        sq->flags &= ~SD_SQ_FLAG_STOP;
        if (sq->vol != vol) {
            sq->flags |= SD_SQ_FLAG_VOLUME;
            sq->vol = vol;
        }
        return 0;
    }
    int busy = sq->name[0] != '\0';
    if (busy) {
        sq->flags |= SD_SQ_FLAG_STOP;
        sq = &gSQLd[1];
    }
    strcpy(sq->dir, dir);
    strupr(sq->dir);
    strcpy(sq->name, upper);
    sq->effectType = effectType;
    sq->effectDepth = effectDepth;
    sq->spuAddr = spuAddr;
    sq->sqSize = 0;
    sq->bdSize = 0;
    sq->hdSize = 0;
    sq->iopSq = 0;
    sq->iopBd = 0;
    sq->iopHd = 0;
    sq->stopWait = 0;
    sq->flags = 0;
    sq->vol = vol;
    sq->playVol = vol;
    sq->step = SD_SQ_LD_STEP_REQUEST;
    sq->effectDelay = effectDelay;
    return busy;
}

// CT3 0x018ce640 -- runs the file and sound servers until neither SQ row is loading.
void SdLoadSQEndWaitAll(void)
{
    for (int i = 0; i < SD_SQ_LD_COUNT; i++) {
        while (gSQLd[i].step > 0 && gSQLd[i].step < SD_SQ_LD_STEP_LOADED) {
            FlProc();
            SdProc();
            sceGsSyncV(0);
        }
    }
}

// CT3 0x018d13d0 -- sound back on (after SdSoundDisable): resumes FSS_Thread, restarts the driver and
// waits (bounded) for the IOP to say ready, then the reverb and the SQ master volume again.
void SdSoundEnable(void)
{
    ResumeThread(fss_tid);
    CpCall_IOPSndDriver_FIFO(SND_CMD_SOUND_ON, NULL, NULL);
    WaitSema(fss_sid);
    CpGetEEIOPStatus()->sndFlags &= ~EE_IOP_SND_READY;
    // The ROM reads +0x40 back and stores it again (a volatile touch, no change).
    CpCall_IOPSndDriver_FIFO(SND_CMD_ADS_ENABLE, &gTransADS, NULL);
    for (int n = SD_SOUND_ENABLE_SPIN; n != 0; n--) {
        if ((CpGetEEIOPStatus()->sndFlags & EE_IOP_SND_READY) != 0) {
            break;
        }
    }
    SignalSema(fss_sid);
    SdSetEffectDontSave(0, gSdEffectType[0], (int)gSdEffectDepth[0]);
    SdSetEffectDontSave(SD_EFFECT_CORE_1, gSdEffectType[1], (int)gSdEffectDepth[1]);
    SdMasterVolumeSetSQ(gMasterVolSQ);
    for (int n = 0; n < SD_SOUND_ENABLE_FRAMES; n++) {
        sceGsSyncV(0);
    }
}

// CT3 0x018d14d0 -- sound off: ends the stream (up to 100 frames, then until the IOP says it stopped),
// stops the SQ, the special SEs and every channel, turns the driver and reverb off, and suspends
// FSS_Thread once its ring is empty.
void SdSoundDisable(void)
{
    if (gSdADSPhase != 0) {
        SdStopStream(0);
        for (int n = 0; n < SD_ADS_DISABLE_FRAMES; n++) {
            if (gSdADSPhase == 0) {
                break;
            }
            SdProc();
            sceGsSyncV(0);
        }
        while (SdIsPlayingStream()) {
        }
    }
    memset(&gPlayADS, 0, sizeof gPlayADS);
    gSdADSPhase = 0;
    gADSOpenSync = 0;
    gSdADSStop = 0;
    SdStopSQAllNow();
    SdStopSEAllSp1();
    memset(gSdSE, 0, sizeof gSdSE);
    SdStopSECH(SD_SE_CH_ALL, 0);
    CpCall_IOPSndDriver_FIFO(SND_CMD_SOUND_OFF, NULL, NULL);
    WaitSema(fss_sid);
    CpCall_IOPSndDriver_FIFO(SND_CMD_ADS_DISABLE, &gTransADS, NULL);
    SignalSema(fss_sid);
    SdSetEffectDontSave(SD_EFFECT_CORE_1, 0, 0);
    SdSetEffectDontSave(0, 0, 0);
    FSS_Thread_Sync();
    SuspendThread(fss_tid);
}

// CT3 0x018d1810 -- stereo (1) or mono (0) output.
void SdChangeStereoMono(int stereo)
{
    WaitSema(fss_sid);
    gTransJSLCOM.arg[0] = stereo != 0;
    CpCall_IOPSndDriver_FIFO(SND_CMD_STEREO, &gTransJSLCOM, NULL);
    SignalSema(fss_sid);
}

// ---- slice 5 (2026-11-10): the stream player, the driver's boot, SdSetParamSESDT, SdFindLoadSEInfo ---------

// SdPlayADSProc's phases (gSdADSPhase); 8, 9, 13 and 14 are unused (the jump table sends them to the end).
typedef enum SD_ADS_PHASE {
    SD_ADS_IDLE = 0,
    SD_ADS_WAIT_READS = 1,      // SdPlayStream's request: wait for the file layer's reads to drain
    SD_ADS_OPEN = 2,            // fill gTransADS and send SND_CMD_ADS_OPEN
    SD_ADS_WAIT_OPEN = 3,       // until the IOP sets EE_IOP_ADS_OPENED, then SND_CMD_ADS_START
    SD_ADS_OPENED = 4,          // the file layer may read again
    SD_ADS_PLAYING = 5,         // until it ends, or gSdADSStop (stopArg != 0: fade, else at once)
    SD_ADS_ENDED = 6,           // arg2 != 0: play it again after SD_ADS_REPLAY_WAIT frames
    SD_ADS_REPLAY = 7,
    SD_ADS_FADE = 10,           // down by SD_ADS_FADE_STEP a frame, then stop
    SD_ADS_STOP = 11,
    SD_ADS_WAIT_STOP = 12,
    SD_ADS_DONE = 15,
} SD_ADS_PHASE;
#define SD_ADS_REPLAY_WAIT 5
#define SD_ADS_FADE_STEP 4
#define SD_ADS_OPEN_4C 0x5100u
#define SD_ADS_OPEN_50 0x10100u
#define SD_ADS_VOICE_L 0x16u
#define SD_ADS_VOICE_R 0x17u

static int sLoopWait;                           /* sbss 0x01942fc0 (sLoopWait$184): SD_ADS_REPLAY's frames */

// CT3 0x018d10e0 -- SdProc's stream step: opens the ADS SdPlayStream asked for once the file layer is idle
// (gADSOpenSync holds the file layer's reads back meanwhile), then watches it: replay when it ends with
// arg2 set, fade out or stop at once on SdStopStream.
static void SdPlayADSProc(void)
{
    switch (gSdADSPhase) {
    case SD_ADS_WAIT_READS:
        if (GetNowReadIdx() == -1) {
            gADSOpenSync = 1;
            gSdADSPhase++;
        }
        break;
    case SD_ADS_OPEN: {
        u_int vol = (u_int)SdScaleVolume(gPlayADS.vol);
        WaitSema(fss_sid);
        gTransADS.mode = SND_ADS_MODE;
        gTransADS.open4c = SD_ADS_OPEN_4C;
        gTransADS.one5c = 1;
        gTransADS.open50 = SD_ADS_OPEN_50;
        gTransADS.voiceR = SD_ADS_VOICE_R;
        gTransADS.voiceL = SD_ADS_VOICE_L;
        gTransADS.loop = gPlayADS.loop != 0;
        gTransADS.vol = vol | vol << 16;
        gTransADS.unk04 = 0;
        CpGetEEIOPStatus()->sndFlags &= ~EE_IOP_ADS_OPENED;
        // The ROM reads +0x40 back and stores it again (a volatile touch, no change).
        int packIdx = FlFindPacked(gPlayADS.name);
        gTransADS.sect = packIdx < 0 ? -1 : gFlPackInfo[packIdx].sect;
        CpCall_IOPSndDriver_FIFO(SND_CMD_ADS_OPEN, &gTransADS, NULL);
        SignalSema(fss_sid);
        gSdADSPhase++;
        break;
    }
    case SD_ADS_WAIT_OPEN:
        if ((CpGetEEIOPStatus()->sndFlags & EE_IOP_ADS_OPENED) != 0) {
            CpCall_IOPSndDriver_FIFO(SND_CMD_ADS_START, NULL, NULL);
            gSdADSPhase++;
        }
        break;
    case SD_ADS_OPENED:
        gSdADSPhase++;
        gADSOpenSync = 0;
        break;
    case SD_ADS_PLAYING:
        if (gSdADSStop != 0) {
            gSdADSPhase = gPlayADS.stopArg != 0 ? SD_ADS_FADE : SD_ADS_STOP;
        } else if (!SdIsPlayingStream()) {
            gSdADSPhase++;
        }
        break;
    case SD_ADS_ENDED:
        if (gPlayADS.arg2 == 0) {
            gSdADSPhase = SD_ADS_DONE;
        } else {
            sLoopWait = SD_ADS_REPLAY_WAIT;
            gSdADSPhase = SD_ADS_REPLAY;
        }
        break;
    case SD_ADS_REPLAY:
        if (--sLoopWait < 1) {
            SdPlayStream(gPlayADS.name, gPlayADS.vol, gPlayADS.arg2, 0);
        }
        break;
    case SD_ADS_FADE:
        gPlayADS.vol -= SD_ADS_FADE_STEP;
        if (gPlayADS.vol < 1) {
            gSdADSPhase++;
        } else {
            SdVolumeStream(gPlayADS.vol);
        }
        break;
    case SD_ADS_STOP:
        CpCall_IOPSndDriver_FIFO(SND_CMD_ADS_STOP, NULL, NULL);
        gSdADSPhase++;
        break;
    case SD_ADS_WAIT_STOP:
        if (!SdIsPlayingStream()) {
            gSdADSPhase = SD_ADS_DONE;
        }
        break;
    case SD_ADS_DONE:
        gSdADSStop = 0;
        gSdADSPhase = SD_ADS_IDLE;
        break;
    default:
        break;
    }
}

static const char s_sndDrv[] = "\\IOP\\SNDDRV.IRX;1";  /* @2829 (18 bytes) */
#define SD_COM_THREAD_PRIORITY 10
#define SD_COM_THREAD_STACK 0x1000
#define SD_INIT_VOICE_MASK 0xffffff                     /* SND_CMD_INIT_07's arg[1] */
#define SD_SE_HEAP_BD_SIZE 0x32000                      /* gSEHeapBD: one .BD piece */
#define SD_SQ_HEAP_BD_SIZE 0x3a000                      /* gSQHeapBD */

// CT3 0x018d1860 -- boots the sound side (IopLoadModules' last step): the FSS thread, the EE <-> IOP
// channel and its receive thread, SNDDRV.IRX from `device` (NULL: the bare path) with gArg_CpDriverInit's
// arguments, the RPC client; then the stream buffers (waiting for EE_IOP_SND_READY), reverb off on both
// cores, stereo / mono, the voices, the master volumes and the IOP staging blocks for SE and SQ .BDs.
void IopLoadCapcomSound(const char *device)
{
    char args[0x100];
    char path[0x40];

    FSS_Thread_Init();
    CpInit_Gloval_EE();
    CpCreateEEComRecieveThread(SD_COM_THREAD_PRIORITY, SD_COM_THREAD_STACK);
    gArg_CpDriverInit.h1a = 0;
    gArg_CpDriverInit.w00 = 0x1fffff;
    gArg_CpDriverInit.w04 = 0x1ffff;
    gArg_CpDriverInit.h14 = 0x1b;
    gArg_CpDriverInit.w08 = 1;
    gArg_CpDriverInit.h16 = 0x1b;
    gArg_CpDriverInit.w0c = 0x1047;
    gArg_CpDriverInit.h10 = 9;
    gArg_CpDriverInit.h12 = 0xb;
    gArg_CpDriverInit.h18 = 0x17;
    if (device == NULL) {
        memcpy(path, s_sndDrv, sizeof s_sndDrv);
    } else {
        strcpy(path, device);
        strcat(path, s_sndDrv);
    }
    int argLen = (signed char)CpMakeSnddrvInitStr(args, &gArg_CpDriverInit);
    sceSifLoadModule(path, argLen, args);
    CpEEInit_RpcClient();
    CpExchangeTransBuffer();

    WaitSema(fss_sid);
    gTransADS.unk6c = 0;
    gTransADS.one5c = 1;
    gTransADS.buf54 = 0xb000;
    gTransADS.buf58 = 0x5800;
    gTransADS.buf70 = 0x50;
    gTransADS.buf74 = 5;
    gTransADS.buf78 = 0xb;
    gTransADS.buf7c = 0x2000;
    CpGetEEIOPStatus()->sndFlags &= ~EE_IOP_SND_READY;
    // The ROM reads +0x40 back and stores it again (a volatile touch, no change).
    CpCall_IOPSndDriver_FIFO(SND_CMD_ADS_ENABLE, &gTransADS, NULL);
    for (int n = SD_SOUND_ENABLE_SPIN; n != 0; n--) {
        if ((CpGetEEIOPStatus()->sndFlags & EE_IOP_SND_READY) != 0) {
            break;
        }
    }
    SignalSema(fss_sid);

    WaitSema(fss_sid);
    gTransJSLCOM.arg[0] = 0;
    CpCall_IOPSndDriver_FIFO(SND_CMD_INIT_08, &gTransJSLCOM, NULL);
    SignalSema(fss_sid);
    SdSetEffect(0, 0, 0);
    SdSetEffect(SD_EFFECT_CORE_1, 0, 0);
    SdChangeStereoMono(gStereo);
    WaitSema(fss_sid);
    gTransJSLCOM.arg[1] = SD_INIT_VOICE_MASK;
    gTransJSLCOM.arg[0] = 0;
    CpCall_IOPSndDriver_FIFO(SND_CMD_INIT_07, &gTransJSLCOM, NULL);
    SignalSema(fss_sid);
    SdMasterVolumeSetSE(gMasterVolSE);
    WaitSema(fss_sid);
    gTransJSLCOM.arg[0] = 0;
    CpCall_IOPSndDriver_FIFO(SND_CMD_INIT_25, &gTransJSLCOM, NULL);
    SignalSema(fss_sid);
    if (gSEHeapBD == 0) {
        gSEHeapBD = (u_int)(uintptr_t)sceSifAllocIopHeap(SD_SE_HEAP_BD_SIZE);
    }
    if (gSQHeapBD == 0) {
        gSQHeapBD = (u_int)(uintptr_t)sceSifAllocIopHeap(SD_SQ_HEAP_BD_SIZE);
    }
    SdMasterVolumeSetSQ(gMasterVolSQ);
}

// CT3 0x018cce40 -- SdSetParamSE for every SE of slot jls, number seNo (reqArg2 0: whatever it was requested
// with, else only that one): each bit of `mask` takes the next argument, in SD_SE_PARAM_* order; what is
// left of the mask is or-ed into each SE's paramFlags. Nothing is answered (the ROM's v0 is unset).
void SdSetParamSESDT(int jls, int seNo, int reqArg2, u_int mask, ...)
{
    int vol = 0, pan = 0, unk30 = 0, posVol = 0, posMode = 0, objId = 0, bank = 0;
    const char *name = NULL;
    SD_SE_UPDATE callback = NULL;
    va_list ap;

    if (mask == 0) {
        return;
    }
    va_start(ap, mask);
    if (mask & SD_SE_PARAM_VOLUME) {
        vol = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_PAN) {
        pan = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_30) {
        unk30 = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_POS_VOL) {
        posVol = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_POS_MODE) {
        posMode = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_OBJ_ID) {
        objId = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_BANK) {
        bank = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_NAME) {
        name = va_arg(ap, const char *);
    }
    if (mask & SD_SE_PARAM_CALLBACK) {
        callback = va_arg(ap, SD_SE_UPDATE);
    }
    va_end(ap);

    for (int i = 0; i < SD_SE_COUNT; i++) {
        SD_SE *se = &gSdSE[i];
        if (se->handle == 0 || jls != se->jls || seNo != se->seNo || (reqArg2 != 0 && reqArg2 != se->reqArg2)) {
            continue;
        }
        u_int left = mask;
        if (mask & SD_SE_PARAM_VOLUME) {
            se->vol = vol;
        }
        if (mask & SD_SE_PARAM_PAN) {
            se->pan = pan;
        }
        if (mask & SD_SE_PARAM_30) {
            se->unk30 = unk30;
        }
        if (left & SD_SE_PARAM_POS_VOL) {
            left &= ~SD_SE_PARAM_POS_VOL;
            se->posVol = posVol;
        }
        if (left & SD_SE_PARAM_POS_MODE) {
            left &= ~SD_SE_PARAM_POS_MODE;
            se->posMode = posMode;
        }
        if (left & SD_SE_PARAM_OBJ_ID) {
            left &= ~SD_SE_PARAM_OBJ_ID;
            se->objId = objId;
        }
        if (left & SD_SE_PARAM_BANK) {
            left &= ~SD_SE_PARAM_BANK;
            se->bank = bank;
        }
        if (left & SD_SE_PARAM_NAME) {
            strncpy(se->name, name, sizeof se->name - 1);
            left &= ~SD_SE_PARAM_NAME;
        }
        if (left & SD_SE_PARAM_CALLBACK) {
            se->callback = callback;
            left &= ~SD_SE_PARAM_CALLBACK;
        }
        se->paramFlags |= left;
    }
}

// CT3 0x018cd600 -- the gSELd row of a bank load, found by SD_FIND_LOAD_SE_* (the one variadic argument:
// a handle, a bank name, or a slot), or -1. By handle the ROM does not skip free rows: handle 0 finds the
// first free one.
int SdFindLoadSEInfo(int by, ...)
{
    va_list ap;
    va_start(ap, by);
    int found = -1;
    if (by == SD_FIND_LOAD_SE_JLS) {
        int jls = va_arg(ap, int);
        for (int i = 0; i < SD_SE_LD_COUNT; i++) {
            if (gSELd[i].handle != 0 && jls == gSELd[i].jls) {
                found = i;
                break;
            }
        }
    } else if (by == SD_FIND_LOAD_SE_NAME) {
        char upper[0x48];
        strcpy(upper, va_arg(ap, const char *));
        strupr(upper);
        for (int i = 0; i < SD_SE_LD_COUNT; i++) {
            if (gSELd[i].handle != 0 && strcmp(upper, gSELd[i].name) == 0) {
                found = i;
                break;
            }
        }
    } else if (by == SD_FIND_LOAD_SE_HANDLE) {
        int handle = va_arg(ap, int);
        for (int i = 0; i < SD_SE_LD_COUNT; i++) {
            if (handle == gSELd[i].handle) {
                found = i;
                break;
            }
        }
    }
    va_end(ap);
    return found;
}

// ---- slice 3 (2026-11-10): SE bank requests, SE play ------------------------------------------------------

// The play word (SND_CMD_PLAY_SE's arg[0]) and its values (arg[1]).
#define SD_PLAY_KEY_ON 0xa1000000u              /* SdPlaySE: start seNo */
#define SD_PLAY_UPDATE 0x21000000u              /* SdPlaySEUpdate: change a sounding SE */
#define SD_PLAY_HAS_VOL 0x100000u               /* arg[1] bits 8..15: volume - 0x7f (0x80: silent) */
#define SD_PLAY_HAS_30 0x400000u                /* arg[1] bits 16..23: SD_SE.unk30 */
#define SD_PLAY_HAS_PAN 0x10000u                /* arg[1] bits 0..7: the pan */
#define SD_PLAY_CH_SHIFT 24                     /* arg[1] bits 24..31: the channel */
#define SD_POS_VOL_FULL 0                       /* SD_SE.posVol: no scale */
#define SD_POS_VOL_MUTE 1                       /* SD_SE.posVol: silent */
#define SD_POS_VOL_TENTHS 10

// CT3 0x018cd8b0 -- the gSELd row holding the bank `name` (upper case), or -1.
static int FindSEFile(const char *name)
{
    for (int i = 0; i < SD_SE_LD_COUNT; i++) {
        if (gSELd[i].handle != 0 && strcmp(name, gSELd[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

// CT3 0x018cd7b0 -- the gSELd row on the SPU2 address `spuAddr` or the slot `jls`, or -1.
static int FindSEFile_JLS_SPU2(int jls, u_int spuAddr)
{
    for (int i = 0; i < SD_SE_LD_COUNT; i++) {
        if (gSELd[i].handle != 0) {
            if (spuAddr == gSELd[i].spuAddr) {
                return i;
            }
            if (jls == gSELd[i].jls) {
                return i;
            }
        }
    }
    return -1;
}

// CT3 0x018cdba0 -- asks for the SE bank dir\name on the slot `jls`, its .BD at `spuAddr`: answers the
// load's handle (that of the row already holding the bank, if one does), 0 when all rows are taken. A bank
// on the same slot or SPU2 address is cancelled first. SdLoadSEProc does the load, one step a frame.
int SdReqLoadSE(const char *dir, const char *name, int jls, u_int spuAddr, int stopPlaying)
{
    char upper[0x40];
    int i;

    strcpy(upper, name);
    strupr(upper);
    i = FindSEFile(upper);
    if (i >= 0) {
        return gSELd[i].handle;
    }
    i = FindSEFile_JLS_SPU2(jls, spuAddr);
    if (i >= 0) {
        SdReqLoadSECancel(i);   // the ROM passes the name too; the body reads only the row
    }
    for (i = 0; i < SD_SE_LD_COUNT; i++) {
        if (gSELd[i].handle == 0) {
            break;
        }
    }
    if (i >= SD_SE_LD_COUNT) {
        printf(s_loadMax);
        return 0;
    }
    SD_SE_LD *ld = &gSELd[i];
    ld->handle = ++gSELdHandleCounter;
    strcpy(ld->dir, dir);
    strupr(ld->dir);
    strcpy(ld->name, upper);
    ld->spuAddr = spuAddr;
    ld->jls = jls;
    ld->sdtSize = 0;
    ld->bdSize = 0;
    ld->hdSize = 0;
    ld->iopSdt = 0;
    ld->iopBd = 0;
    ld->iopHd = 0;
    ld->flags = 0;
    ld->fd = 0;
    ld->step = SD_SE_LD_OPEN_SDT;
    ld->buf = NULL;
    ld->stopPlaying = stopPlaying;
    return ld->handle;
}

// CT3 0x018cfd30 -- drops the gSELd row `ldi`: closes its file, frees its IOP blocks and buffer, and
// purges the bank from the driver when it had loaded (step 0 with a handle, or a done / error step).
void SdReqLoadSECancel(int ldi)
{
    SD_SE_LD *ld = &gSELd[ldi];

    if (ld->fd != 0) {
        FlReqClose(ld->fd);
    }
    if (ld->iopHd != 0) {
        sceSifFreeIopHeap((void *)(uintptr_t)ld->iopHd);
        ld->iopHd = 0;
    }
    if (ld->iopSdt != 0) {
        sceSifFreeIopHeap((void *)(uintptr_t)ld->iopSdt);
        ld->iopSdt = 0;
    }
    SdFree(&ld->buf);
    if ((ld->handle != 0 && ld->step == SD_SE_LD_IDLE) || ld->step >= SD_SE_LD_DONE) {
        SdPurgeSE(ld->name);
    }
    ld->step = SD_SE_LD_IDLE;
    ld->handle = 0;
}

// CT3 0x018cd930 -- drops the bank `name` from the driver and frees its row.
void SdPurgeSE(const char *name)
{
    char upper[0x40];
    int i;

    strcpy(upper, name);
    strupr(upper);
    i = FindSEFile(upper);
    if (i < 0) {
        return;
    }
    SD_SE_LD *ld = &gSELd[i];
    WaitSema(fss_sid);
    // The ROM's record is gSELd[gLdi].reg (the row SdLoadSEProc steps), not this row's (disassembly
    // 0x018cd984): kept as it is; the slot travels in the command word.
    CpCall_IOPSndDriver_FIFO((u_int)ld->jls | SND_CMD_SE_PURGE, &gSELd[gLdi].reg, NULL);
    SignalSema(fss_sid);
    if (ld->iopHd != 0) {
        sceSifFreeIopHeap((void *)(uintptr_t)ld->iopHd);
        ld->iopHd = 0;
    }
    if (ld->iopSdt != 0) {
        sceSifFreeIopHeap((void *)(uintptr_t)ld->iopSdt);
        ld->iopSdt = 0;
    }
    ld->handle = 0;
}

// CT3 0x018cd3c0 -- queues SE `seNo` of the slot `jls`: answers its handle; with no free row, stops every
// channel and answers 0. SdPlaySEProc keys it on. (posMode stays as the row had it, as in the ROM.)
int SdReqPlaySE(int jls, int seNo, int reqArg2)
{
    for (int i = 0; i < SD_SE_COUNT; i++) {
        SD_SE *se = &gSdSE[i];
        if (se->handle != 0) {
            continue;
        }
        se->handle = ++gSdSEHandleCounter;
        se->jls = jls;
        se->seNo = seNo;
        se->reqArg2 = reqArg2;
        se->paramFlags = 0;
        se->posVol = SD_POS_VOL_FULL;
        se->savedVol = 0;
        se->bank = 0;
        se->objId = 0;
        se->ch = 0;
        se->chBits = 0;
        se->state = SD_SE_STATE_REQUEST;
        se->pan = 999;
        se->unk30 = 999;
        se->vol = 999;
        se->name[0] = '\0';
        se->callback = NULL;
        return se->handle;
    }
    SdStopSECH(SD_SE_CH_ALL, 0);
    gSdSEKeyOnCH = 0;
    return 0;
}

// CT3 0x018cd190 -- sets the SE's parameters: each bit of `mask` takes the next argument, in the order of
// SD_SE_PARAM_*. What is left of the mask is or-ed into paramFlags and answered: VOLUME / PAN / 30 stay
// (the next play or update sends them), the others are taken out.
u_int SdSetParamSE(int handle, u_int mask, ...)
{
    u_int left = mask;
    SD_SE *se;
    va_list ap;

    if (handle == 0 || (se = SdFindPlaySE(handle)) == NULL) {
        return left;
    }
    va_start(ap, mask);
    if (mask & SD_SE_PARAM_VOLUME) {
        se->vol = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_PAN) {
        se->pan = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_30) {
        se->unk30 = va_arg(ap, int);
    }
    if (mask & SD_SE_PARAM_POS_VOL) {
        se->posVol = va_arg(ap, int);
        left &= ~SD_SE_PARAM_POS_VOL;
    }
    if (left & SD_SE_PARAM_POS_MODE) {
        se->posMode = va_arg(ap, int);
        left &= ~SD_SE_PARAM_POS_MODE;
    }
    if (left & SD_SE_PARAM_OBJ_ID) {
        se->objId = va_arg(ap, int);
        left &= ~SD_SE_PARAM_OBJ_ID;
    }
    if (left & SD_SE_PARAM_BANK) {
        se->bank = va_arg(ap, int);
        left &= ~SD_SE_PARAM_BANK;
    }
    if (left & SD_SE_PARAM_NAME) {
        strncpy(se->name, va_arg(ap, const char *), sizeof se->name - 1);
        left &= ~SD_SE_PARAM_NAME;
    }
    if (left & SD_SE_PARAM_CALLBACK) {
        se->callback = va_arg(ap, SD_SE_UPDATE);
        left &= ~SD_SE_PARAM_CALLBACK;
    }
    va_end(ap);
    se->paramFlags |= left;
    return left;
}

// CT3 0x018cc510 -- after row `from`, the requested row with the lowest handle (the oldest request), or -1.
static int NextPlaySE(int from)
{
    int last;
    int best = -1;
    int bestHandle = 0x7fffffff;

    if (from < 0) {
        from = 0;
        last = 0;
    } else {
        last = gSdSE[from].handle;
    }
    for (int n = 0; n < SD_SE_COUNT; n++) {
        if (++from >= SD_SE_COUNT) {
            from = 0;
        }
        int h = gSdSE[from].handle;
        if (h != 0 && gSdSE[from].state == SD_SE_STATE_REQUEST && h != last && h < bestHandle) {
            bestHandle = h;
            best = from;
        }
    }
    return best;
}

// A volume byte for the play word: 0 is silent (0x80), else the volume less 0x7f.
static u_int SdPlayVolByte(int vol)
{
    return (vol == 0 ? 0xffffff80u : (u_int)(vol - SD_SE_VOL_MAX)) & 0xff;
}

// `vol` scaled by the SE's posVol in tenths, clamped (inline in SdPlaySE / SdPlaySEUpdate).
static int SdPosVolume(int vol, int posVol)
{
    if (posVol == SD_POS_VOL_FULL) {
        return vol;
    }
    if (posVol == SD_POS_VOL_MUTE) {
        return 0;
    }
    vol = vol * posVol / SD_POS_VOL_TENTHS;
    return vol > SD_SE_VOL_MAX ? SD_SE_VOL_MAX : vol;
}

// Fills the slot's play command (SdPlaySE and SdPlaySEUpdate repeat this inline in the ROM): the word
// kind | seNo, the channel, and the parameters set since the last one (paramFlags VOLUME / PAN / 30,
// cleared as they go); with none of them set, a posVol other than full sends 0x7f scaled by it.
static void SdFillPlayCom(SD_SE *se, SND_JSL_COM *com, u_int kind)
{
    u_int word = (u_int)se->seNo | kind;
    u_int vals = (u_int)se->ch << SD_PLAY_CH_SHIFT;

    com->unk00 = se->jls;
    if ((se->paramFlags & SD_SE_PARAM_SEND) == 0) {
        if (se->posVol != SD_POS_VOL_FULL) {
            word |= SD_PLAY_HAS_VOL;
            vals |= SdPlayVolByte(SdPosVolume(SD_SE_VOL_MAX, se->posVol)) << 8;
        }
    } else {
        if (se->paramFlags & SD_SE_PARAM_30) {
            word |= SD_PLAY_HAS_30;
            vals |= ((u_int)se->unk30 & 0xff) << 16;
            se->paramFlags &= ~SD_SE_PARAM_30;
        }
        if (se->paramFlags & SD_SE_PARAM_VOLUME) {
            word |= SD_PLAY_HAS_VOL;
            vals |= SdPlayVolByte(SdPosVolume(se->vol, se->posVol)) << 8;
            se->paramFlags &= ~SD_SE_PARAM_VOLUME;
        }
        if (se->paramFlags & SD_SE_PARAM_PAN) {
            word |= SD_PLAY_HAS_PAN;
            vals |= (u_int)se->pan;
            se->paramFlags &= ~SD_SE_PARAM_PAN;
        }
    }
    com->arg[0] = (int)word;
    com->arg[1] = (int)vals;
}

// The loaded gSELd row on the SE's slot, or -1 (the search SdPlaySE / SdPlaySEUpdate open with).
static int SdFindLoadedSlot(const SD_SE *se)
{
    for (int i = 0; i < SD_SE_LD_COUNT; i++) {
        if (gSELd[i].handle != 0 && gSELd[i].step == SD_SE_LD_IDLE && se->jls == gSELd[i].jls) {
            return i;
        }
    }
    return -1;
}

// CT3 0x018cc8d0 -- keys the requested SE on: finds its slot's loaded bank (with SD_SE_FLAG_WAIT_BANK it
// must be the bank named in se->name, else the request waits), takes a channel (SdSupplyCHSE) and sends
// the play word. With no bank on the slot the request is dropped (unless it waits); with no channel, too.
static void SdPlaySE(SD_SE *se)
{
    int i = SdFindLoadedSlot(se);

    if (i < 0) {
        if ((se->paramFlags & SD_SE_FLAG_WAIT_BANK) == 0) {
            se->handle = 0;
        }
        return;
    }
    if ((se->paramFlags & SD_SE_FLAG_WAIT_BANK) != 0 && strcmp(gSELd[i].name, se->name) != 0) {
        return;
    }
    if (SdSupplyCHSE(se) == -1) {
        se->handle = 0;
        return;
    }
    WaitSema(fss_sid);
    SdFillPlayCom(se, &gSELd[i].spuCom, SD_PLAY_KEY_ON);
    CpCall_IOPSndDriver_FIFO(SND_CMD_PLAY_SE, &gSELd[i].spuCom, NULL);
    SignalSema(fss_sid);
    se->state = SD_SE_STATE_KEYED;
}

// CT3 0x018ccc40 -- gives the SE a channel: an SD_SE_REQ_SUPPLY_CH SE takes over the channel of another
// one playing (stopping it); else the next channel round the ring that neither sounds nor went out last.
// Answers the channel (se->ch, se->chBits), -1 when all 22 are busy.
int SdSupplyCHSE(SD_SE *se)
{
    if (se->reqArg2 == SD_SE_REQ_SUPPLY_CH) {
        for (int i = 0; i < SD_SE_COUNT; i++) {
            SD_SE *other = &gSdSE[i];
            if (other->handle != 0 && other->reqArg2 == SD_SE_REQ_SUPPLY_CH && other != se) {
                SdStopSECH(other->chBits, 1);
                sBitLastSupplyCH = (u_int)other->chBits;
                se->chBits = other->chBits;
                se->ch = other->ch;
                other->handle = 0;
                return se->ch;
            }
        }
    }
    for (int n = 0;; n++) {
        sBitRingCH <<= 1;
        if (sBitRingCH & (1u << SD_SE_CH_COUNT)) {
            sBitRingCH = 1;
        }
        if (((gSdSEKeyOnCH | sBitLastSupplyCH) & sBitRingCH) == 0) {
            break;
        }
        if (n + 1 >= SD_SE_CH_COUNT) {
            sBitLastSupplyCH = 0;
            se->chBits = 0;
            se->ch = -1;
            return -1;
        }
    }
    sBitLastSupplyCH = sBitRingCH;
    se->chBits = (int)sBitRingCH;
    for (int ch = 0; ch < SD_SE_CH_COUNT; ch++) {
        if ((1u << ch) & sBitRingCH) {
            se->ch = ch;
            break;
        }
    }
    return se->ch;
}

// CT3 0x018cc5e0 -- sends a sounding SE's changed parameters; drops the SE when its bank is gone.
static void SdPlaySEUpdate(SD_SE *se)
{
    int i = SdFindLoadedSlot(se);

    if (i < 0) {
        se->handle = 0;
        return;
    }
    WaitSema(fss_sid);
    SdFillPlayCom(se, &gSELd[i].spuCom, SD_PLAY_UPDATE);
    CpCall_IOPSndDriver_FIFO(SND_CMD_PLAY_SE, &gSELd[i].spuCom, NULL);
    SignalSema(fss_sid);
}

// CT3 0x018cfb70 -- SdProc's SE step: asks the driver which channels sound, moves each SE on (keyed ->
// playing once its channel sounds; playing -> free once it stops, its callback every frame meanwhile),
// then keys on the oldest request, or, with none, sends one sounding SE's changed parameters.
static void SdPlaySEProc(void)
{
    WaitSema(fss_sid);
    CpCall_IOPSndDriver_FIFO(SND_CMD_SE_KEY_ON_CH, NULL, (int *)&gSdSEKeyOnCH);
    SignalSema(fss_sid);
    for (int i = 0; i < SD_SE_COUNT; i++) {
        SD_SE *se = &gSdSE[i];
        if (se->handle == 0) {
            continue;
        }
        if (se->state == SD_SE_STATE_KEYED) {
            if (gSdSEKeyOnCH & (u_int)se->chBits) {
                se->state = SD_SE_STATE_PLAYING;
            }
        } else if (se->state == SD_SE_STATE_PLAYING) {
            if ((gSdSEKeyOnCH & (u_int)se->chBits) == 0) {
                se->handle = 0;
            } else if (se->callback != NULL) {
                se->callback(se->handle);
            }
        }
    }
    gSEi = NextPlaySE(gSEi);
    if (gSEi >= 0) {
        SdPlaySE(&gSdSE[gSEi]);
        return;
    }
    for (int n = 0; n < SD_SE_COUNT; n++) {
        if (++sIdxUpdate >= SD_SE_COUNT) {
            sIdxUpdate = 0;
        }
        SD_SE *se = &gSdSE[sIdxUpdate];
        if (se->handle != 0 && se->state == SD_SE_STATE_PLAYING && (se->paramFlags & SD_SE_PARAM_UPDATE) != 0) {
            SdPlaySEUpdate(se);
            return;
        }
    }
}

// CT3 0x018cfa50 -- SdProc's special-SE step: each row whose interval has passed starts a random SE of its
// range (unless stopped), positional through its update, and notes the time.
static void SdPlaySESpProc(void)
{
    for (int i = 0; i < PLAY_SE_SP1_COUNT; i++) {
        PLAY_SE_SP1 *sp = &gPlaySESp1[i];
        if (sp->active == 0 || ga_main_timer - sp->lastTime < sp->interval) {
            continue;
        }
        if (sp->stopped == 0) {
            int r = (int)SdRnd();
            sp->handle = SdReqPlaySE(sp->active, sp->seNoBase + r % sp->seNoRange, sp->seNo);
            SdSetParamSE(sp->handle,
                         SD_SE_PARAM_POS_VOL | SD_SE_PARAM_POS_MODE | SD_SE_PARAM_OBJ_ID | SD_SE_PARAM_BANK |
                             SD_SE_PARAM_CALLBACK,
                         sp->posVol, sp->posMode, sp->objId, sp->bank, sp->update);
            if (sp->update != NULL) {
                sp->update(sp->handle);
            }
        }
        sp->lastTime = ga_main_timer;
    }
}

// ---- slice 6 (2026-11-10): the SQ loader and player -----------------------------------------------------

static int sSQTransedSize;                      /* sbss 0x01942f6c (sTransedSize$420): .BD bytes sent */
static int sSQTransSize;                        /* sbss 0x01942f68 (sTransSize$421): this piece's bytes */
static char sSQFname[0x38];                     /* bss  0x01942f30 (sFname$422) */

static const char s_sq[] = ".SQ";               /* @2325 */
static const char s_allocSq[] = "SD4";          /* 0x019366e8: SdAlloc's names (unread) */
static const char s_allocSqHd[] = "SD5";        /* 0x01936730 */
static const char s_allocSqBd[] = "SD6";        /* 0x01936780 */
static const char s_errSqDma[] = "### ERR: SdLoadSQProc, CpSifDmaTransEEToIOP ###\n"; /* @2327 */
static const char s_errSqTrans[] = "### ERR: SdLoadSQProc, sceSdTransToIOP ###\n";    /* @2333 */

#define SD_SQ_BD_PIECE 0x3a000                  /* .BD bytes per SPU2 transfer (gSQHeapBD's size) */
#define SD_SQ_STOP_WAIT 2                       /* frames between the stop command and the free */
#define SD_SQ_START_ARG0 1                      /* SND_CMD_SQ_START's arg[0] (SND_CMD_SQ_18's is 0) */

// Steps 1, 4, 7: open NAME<ext>, and go on once the file layer took the request.
static void SdLoadSQOpen(SD_SQ_LD *sq, const char *ext)
{
    char path[0x40];
    strcpy(path, sq->name);
    strcat(path, ext);
    sq->fd = FlReqOpen(sq->dir, path);
    if (sq->fd != 0) {
        sq->step++;
    }
}

// Steps 2, 5, 8: once the size is known, sqAllocPtr for the whole file and the read.
static void SdLoadSQRead(SD_SQ_LD *sq, int *size, const char *allocName)
{
    const FL_REQ_RD *req = SdFindReq(sq->fd);
    int fileSize = req != NULL ? req->size : -1;
    if (fileSize > 0) {
        *size = fileSize;
        sqAllocPtr = SdAlloc(*size, SD_FILE_ALIGN, allocName);
        if (sqAllocPtr != NULL) {
            FlReqRead(sq->fd, sqAllocPtr, -1, -1);
            sq->step++;
        }
    }
}

// Steps 3 and 6: an IOP heap block for sqAllocPtr's `size` bytes, and the DMA; 0 on success. The block goes
// to *iop (0 when the heap is full: then the step is ERROR and nothing is sent).
static int SdLoadSQSend(SD_SQ_LD *sq, u_int *iop, int size, const char *err)
{
    *iop = SdIopHeapAlloc(size);
    if (*iop == 0) {
        sq->step = SD_SQ_LD_ERROR;
        return -1;
    }
    if (CpSifDmaTransEEToIOP(sqAllocPtr, *iop, (u_int)(size + 0xf) & ~0xfu, 0, 0) != 0) {
        printf(err);
        sq->step = SD_SQ_LD_ERROR;
        return -1;
    }
    return 0;
}

// Steps 0xe..0x11 and 0x14: one command with the player in volCom.
static void SdLoadSQCommand(SD_SQ_LD *sq, u_int command, int setArg0, int arg0)
{
    WaitSema(fss_sid);
    sq->volCom.unk00 = gSQi;
    if (setArg0) {
        sq->volCom.arg[0] = arg0;
    }
    CpCall_IOPSndDriver_FIFO(command, &sq->volCom, NULL);
    SignalSema(fss_sid);
}

// CT3 0x018ce6c0 -- one step of the SQ player gSQi (SD_SQ_LD_STEP says what each does): its three files to
// the IOP and SPU2 RAM, the driver's set-up and start, then volume changes and the stop; once stopped, row 1
// moves into row 0. The ROM re-reads gSQLd[gSQi] after every call; nothing moves gSQi, so `sq` stands for it.
static void SdLoadSQProc(void)
{
    SD_SQ_LD *sq = &gSQLd[gSQi];
    switch (sq->step) {
    case SD_SQ_LD_STEP_REQUEST:
        SdLoadSQOpen(sq, s_sq);
        break;

    case SD_SQ_LD_READ_SQ:
        SdLoadSQRead(sq, &sq->sqSize, s_allocSq);
        break;

    case SD_SQ_LD_SEND_SQ: {
        if (FlIsExecute(sq->fd) != 0) {
            break;
        }
        const FL_REQ_RD *req = SdFindReq(sq->fd);
        int packIdx = req != NULL ? req->packIdx : -1;
        if (SdLoadSQSend(sq, &sq->iopSq, sq->sqSize, s_errSqDma) == 0) {
            if (packIdx < 0) {
                sq->step++;
            } else {
                // A packed .SQ carries its .HD: both went up in one block, the HD steps are skipped.
                int part = gFlPackInfo[packIdx].partSize;
                sq->hdSize = sq->sqSize - part;
                sq->sqSize = part;
                sq->iopHd = sq->iopSq + (u_int)sq->sqSize;
                sq->step = SD_SQ_LD_OPEN_BD;
            }
        }
        // The ROM keeps fd after the close here and in steps 6 and 9 (SdLoadSEProc clears it).
        SdFree(&sqAllocPtr);
        FlReqClose(sq->fd);
        break;
    }

    case SD_SQ_LD_OPEN_HD:
        SdLoadSQOpen(sq, s_hd);
        break;

    case SD_SQ_LD_READ_HD:
        SdLoadSQRead(sq, &sq->hdSize, s_allocSqHd);
        break;

    case SD_SQ_LD_SEND_HD:
        if (FlIsExecute(sq->fd) != 0) {
            break;
        }
        FlReqClose(sq->fd);
        sq->iopHd = SdIopHeapAlloc(sq->hdSize);
        if (sq->iopHd == 0) {
            sq->step = SD_SQ_LD_ERROR; // the buffer stays: END frees it
            break;
        }
        if (CpSifDmaTransEEToIOP(sqAllocPtr, sq->iopHd, (u_int)(sq->hdSize + 0xf) & ~0xfu, 0, 0) == 0) {
            sq->step++;
        } else {
            printf(s_errDma); // the ROM prints SdLoadSEProc's message here (@2330)
            sq->step = SD_SQ_LD_ERROR;
        }
        SdFree(&sqAllocPtr);
        break;

    case SD_SQ_LD_OPEN_BD:
        SdLoadSQOpen(sq, s_bd);
        break;

    case SD_SQ_LD_READ_BD:
        SdLoadSQRead(sq, &sq->bdSize, s_allocSqBd);
        break;

    case SD_SQ_LD_BD_READ_WAIT:
        if (FlIsExecute(sq->fd) != 0) {
            return;
        }
        FlReqClose(sq->fd);
        sSQTransedSize = 0;
        sq->step++;
        // fall through: the first piece goes this frame
    case SD_SQ_LD_BD_TRANS:
        sq->iopBd = gSQHeapBD;
        sSQTransSize = sq->bdSize - sSQTransedSize;
        if (sSQTransSize > SD_SQ_BD_PIECE) {
            sSQTransSize = SD_SQ_BD_PIECE;
        }
        if (sceSdTransToIOP((u_char *)sqAllocPtr + sSQTransedSize, gSQHeapBD, (u_int)sSQTransSize, 1) == 0) {
            sq->step++;
        } else {
            printf(s_errSqTrans);
            sq->step = SD_SQ_LD_ERROR;
        }
        break;

    case SD_SQ_LD_BD_SPU:
        if (gExecDriver_2 < 1) {
            WaitSema(fss_sid);
            sq->volCom.arg[0] = (int)gSQHeapBD;
            sq->volCom.arg[1] = sSQTransedSize + (int)sq->spuAddr;
            sq->volCom.arg[2] =
                (int)((u_int)(sSQTransSize + SD_SPU_TRANS_ALIGN - 1) & ~(u_int)(SD_SPU_TRANS_ALIGN - 1));
            CpCall_IOPSndDriver_2(SND_CMD_TRANS_SPU2, &sq->volCom);
            SignalSema(fss_sid);
            gExecDriver_2 = SD_SPU_TRANS_WAIT;
            sq->step++;
        } else {
            gExecDriver_2--;
        }
        break;

    case SD_SQ_LD_BD_SPU_WAIT:
        if (CpCheckEE_IOPComEnd(SND_CMD_TRANS_SPU2, 1) == 0) {
            gExecDriver_2 = 0;
            sSQTransedSize += sSQTransSize;
            if (sSQTransedSize < sq->bdSize) {
                sq->step = SD_SQ_LD_BD_TRANS;
            } else {
                SdFree(&sqAllocPtr);
                sq->step++;
            }
        }
        break;

    case SD_SQ_LD_REGISTER: {
        char path[0x40];
        strcpy(path, sq->dir);
        for (int i = 0; path[i] != '\0'; i++) {
            if (path[i] == '/') {
                path[i] = '\\';
            }
        }
        strcat(path, sq->name);
        strupr(path);
        strcpy(sSQFname, gDeviceName);
        strcat(sSQFname, path);
        WaitSema(fss_sid);
        strcpy(sq->reg.path, sSQFname);
        sq->reg.iopHd = sq->iopHd;
        sq->reg.iopBd = sq->iopBd;
        sq->reg.spuAddr = sq->spuAddr;
        sq->reg.iopSq = sq->iopSq;
        sq->reg.zero90 = 0;
        sq->reg.bdSize = sq->bdSize;
        sq->reg.zero9c = 0;
        sq->reg.player = (u_char)gSQi;
        sq->reg.zeroA9 = 0;
        sq->reg.player2 = (u_char)gSQi;
        sq->reg.zeroAb = 0;
        CpCall_IOPSndDriver_FIFO((u_int)gSQi | SND_CMD_SE_REGISTER, &sq->reg, NULL);
        SignalSema(fss_sid);
        sq->step++;
        break;
    }

    case SD_SQ_LD_CMD_0C:
        SdLoadSQCommand(sq, SND_CMD_SQ_0C, 0, 0);
        sq->step++;
        break;

    case SD_SQ_LD_CMD_0D:
        SdLoadSQCommand(sq, SND_CMD_SQ_0D, 0, 0);
        sq->step++;
        break;

    case SD_SQ_LD_CMD_17:
        SdLoadSQCommand(sq, SND_CMD_SQ_17, 0, 0);
        sq->step++;
        break;

    case SD_SQ_LD_CMD_18:
        SdLoadSQCommand(sq, SND_CMD_SQ_18, 1, 0);
        sq->step++;
        break;

    case SD_SQ_LD_MASTER_VOL:
        SdMasterVolumeSetSQ(gMasterVolSQ);
        sq->step++;
        break;

    case SD_SQ_LD_EFFECT:
        if (sq->effectDelay-- < 1) {
            SdSetEffect(0, sq->effectType, sq->effectDepth);
            sq->step++;
        }
        break;

    case SD_SQ_LD_START:
        if ((sq->flags & SD_SQ_FLAG_STOP) == 0) {
            SdLoadSQCommand(sq, SND_CMD_SQ_START, 1, SD_SQ_START_ARG0);
            sq->step++;
        } else {
            sq->stopWait = 0;
            sq->step = SD_SQ_LD_END;
        }
        break;

    case SD_SQ_LD_VOLUME:
        SdVolumeSetSQ(gSQi, sq->playVol);
        sq->step++;
        break;

    case SD_SQ_LD_16:
    case SD_SQ_LD_17:
    case SD_SQ_LD_18:
        sq->step = SD_SQ_LD_STEP_LOADED;
        // fall through
    case SD_SQ_LD_STEP_LOADED:
        if ((sq->flags & SD_SQ_FLAG_VOLUME) != 0) {
            sq->step = SD_SQ_LD_FADE;
        } else if ((sq->flags & SD_SQ_FLAG_STOP) != 0) {
            sq->step = SD_SQ_LD_STEP_REPLAY_MAX;
        }
        break;

    case SD_SQ_LD_FADE:
        if ((sq->flags & SD_SQ_FLAG_FADE) == 0) {
            sq->playVol = sq->vol;
            sq->flags &= ~SD_SQ_FLAG_VOLUME;
            sq->step = SD_SQ_LD_STEP_LOADED;
        } else if (sq->vol < sq->playVol) {
            if (--sq->playVol <= sq->vol) {
                sq->playVol = sq->vol;
                sq->flags &= ~(SD_SQ_FLAG_VOLUME | SD_SQ_FLAG_FADE);
                sq->step = SD_SQ_LD_STEP_LOADED;
            }
        } else {
            if (sq->vol <= ++sq->playVol) {
                sq->playVol = sq->vol;
                sq->flags &= ~(SD_SQ_FLAG_VOLUME | SD_SQ_FLAG_FADE);
                sq->step = SD_SQ_LD_STEP_LOADED;
            }
        }
        SdVolumeSetSQ(gSQi, sq->playVol);
        break;

    case SD_SQ_LD_STOP:
    case SD_SQ_LD_STEP_REPLAY_MAX:
        WaitSema(fss_sid);
        sq->volCom.unk00 = gSQi;
        sq->volCom.arg[0] = 0;
        sq->volCom.arg[1] = SD_SQ_STOP_NOW_ARG1;
        CpCall_IOPSndDriver_FIFO(SND_CMD_STOP_SQ_NOW, &sq->volCom, NULL);
        SignalSema(fss_sid);
        sq->stopWait = SD_SQ_STOP_WAIT;
        sq->step++;
        break;

    case SD_SQ_LD_END:
        if (sq->stopWait-- > 0) {
            return;
        }
        if (sq->iopHd != 0) {
            sceSifFreeIopHeap((void *)(uintptr_t)sq->iopHd);
            sq->iopHd = 0;
        }
        if (sq->iopSq != 0) {
            sceSifFreeIopHeap((void *)(uintptr_t)sq->iopSq);
            sq->iopSq = 0;
        }
        SdFree(&sqAllocPtr);
        WaitSema(fss_sid);
        CpCall_IOPSndDriver_FIFO((u_int)gSQi | SND_CMD_SE_PURGE, &sq->reg, NULL);
        SignalSema(fss_sid);
        memset(&sq->reg, 0, SD_SQ_REGISTER_CLEAR);
        memset(&sq->volCom, 0, sizeof sq->volCom);
        sq->step++;
        // fall through
    case SD_SQ_LD_SWAP:
        memcpy(&gSQLd[0], &gSQLd[1], sizeof gSQLd[0]);
        memset(&gSQLd[1], 0, sizeof gSQLd[1]);
        break;

    case SD_SQ_LD_ERROR:
        sq->stopWait = 0;
        sq->step = SD_SQ_LD_END;
        break;
    }
}
