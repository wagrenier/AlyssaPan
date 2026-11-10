// game/SdMasterVolumeSetSE.c -- Clock Tower 3, the EE side of the sound driver (CT3 0x018cbf90..0x018d1cb0):
// SE banks and voices, SQ music, the ADS stream, reverb, and the FSS thread that hands every command to
// the IOP (game/CpDivideEECom.c -> SNDDRV.IRX). Slice 1 (2026-11-10): the command ring and its thread, the
// master / stream volumes, reverb, SE stop and lookup, the load-end wait, SdAlloc / SdFree, SdRnd.
//
// Every command goes the same way: take fss_sid, fill a command record (a JSL_COM, gTransJSLCOM or
// gTransADS), queue (command word, record, reply pointer) on RingBuff with CpCall_IOPSndDriver_FIFO, and
// release fss_sid; FSS_Thread wakes, takes fss_sid, and sends the oldest entry through
// CpCall_IOPSndDriver. Until FSS_Thread_Init has run (IopLoadCapcomSound, still a stub) fss_sid / fss_tid
// are 0 and the host kernel answers their calls with an error, so commands only fill the ring.

#include "game/SdMasterVolumeSetSE.h"

#include "common/alyssapan_stub.h"
#include "game/ArtoolDebugOpen.h"
#include "game/CpDivideEECom.h"
#include "game/ahalloc.h"
#include "game/unassigned_data.h"
#include "sdk/eekernel.h"
#include "sdk/libgraph.h"

#include <stdio.h>

// ---- the TU's data (the symtab's names and addresses) -----------------------------------------------

SD_SE gSdSE[SD_SE_COUNT];                       /* bss  0x01c848c0 */
SD_SE_LD gSELd[SD_SE_LD_COUNT];                 /* bss  0x01c85400 */
SD_SQ_LD gSQLd[SD_SQ_LD_COUNT];                 /* bss  0x01943340 */
PLAY_SE_SP1 gPlaySESp1[PLAY_SE_SP1_COUNT];      /* bss  0x019436d0 */
SD_PLAY_ADS gPlayADS;                           /* bss  0x019437f0 */
int gSdADSStop;                                 /* sbss 0x01943880 */
int gADSOpenSync;                               /* sbss 0x01943888 */
u_int gSdEffectDepth[2];                        /* bss  0x01943890 */
int gSdEffectType[2];                           /* bss  0x01943898 */
int gUseSoundAlloc;                             /* sbss 0x019438a0 */

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
            return gSELd[i].loading == 0;
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018d1810
void SdChangeStereoMono(void)
{
    ALYSSAPAN_STUB("SdChangeStereoMono", 0x018d1810);
}

// stub of CT3 0x018cdb10
void SdLoadSEEndWaitAll(void)
{
    ALYSSAPAN_STUB("SdLoadSEEndWaitAll", 0x018cdb10);
}

// stub of CT3 0x018ce080
void SdMasterVolumeSetSQ(int vol)
{
    ALYSSAPAN_STUB("SdMasterVolumeSetSQ", 0x018ce080);
}

// stub of CT3 0x018ce440
int SdPlaySQ(const char *dir, const char *name, u_int a2, int vol, int a4, int a5, int a6)
{
    ALYSSAPAN_STUB("SdPlaySQ", 0x018ce440);
    return {};
}

// stub of CT3 0x018cd930
void SdPurgeSE(const char *name)
{
    ALYSSAPAN_STUB("SdPurgeSE", 0x018cd930);
}

// stub of CT3 0x018cdba0
int SdReqLoadSE(const char *dir, const char *name, int jls, u_int spuAddr, int a4)
{
    ALYSSAPAN_STUB("SdReqLoadSE", 0x018cdba0);
    return {};
}

// stub of CT3 0x018cd3c0
int SdReqPlaySE(int jls, int a1, int a2)
{
    ALYSSAPAN_STUB("SdReqPlaySE", 0x018cd3c0);
    return {};
}

// stub of CT3 0x018d14d0
void SdSoundDisable(void)
{
    ALYSSAPAN_STUB("SdSoundDisable", 0x018d14d0);
}

// stub of CT3 0x018d13d0
void SdSoundEnable(void)
{
    ALYSSAPAN_STUB("SdSoundEnable", 0x018d13d0);
}

// stub of CT3 0x018cbfe0
void SdStopSEAll(void)
{
    ALYSSAPAN_STUB("SdStopSEAll", 0x018cbfe0);
}

// stub of CT3 0x018cc1b0
void SdStopSEAllSp1(void)
{
    ALYSSAPAN_STUB("SdStopSEAllSp1", 0x018cc1b0);
}

// stub of CT3 0x018cc040
void SdStopSESDT(int jls, int a1, int a2)
{
    ALYSSAPAN_STUB("SdStopSESDT", 0x018cc040);
}

// stub of CT3 0x018ce1e0
void SdStopSQAllNow(void)
{
    ALYSSAPAN_STUB("SdStopSQAllNow", 0x018ce1e0);
}

// stub of CT3 0x018ce380
void SdStopSQ(const char *name)
{
    ALYSSAPAN_STUB("SdStopSQ", 0x018ce380);
}

// stub of CT3 0x018ce0d0
void SdVolumeSQ(const char *name, int vol, int a2)
{
    ALYSSAPAN_STUB("SdVolumeSQ", 0x018ce0d0);
}

// stub of CT3 0x018cde40
void SdPlayStream(const char *name, int vol, int a2, int loop)
{
    ALYSSAPAN_STUB("SdPlayStream", 0x018cde40);
}

// stub of CT3 0x018cd190
int SdSetParamSE(int handle, u_int mask, ...)
{
    ALYSSAPAN_STUB("SdSetParamSE", 0x018cd190);
    return {};
}

// stub of CT3 0x018cc2a0
int SdStopSESp1(int jls, int a1, int a2)
{
    ALYSSAPAN_STUB("SdStopSESp1", 0x018cc2a0);
    return {};
}

// stub of CT3 0x018cc220
void SdStopSESp1ByHandle(int handle)
{
    ALYSSAPAN_STUB("SdStopSESp1ByHandle", 0x018cc220);
}

// stub of CT3 0x018ce310
void SdStopSQAll(int fade)
{
    ALYSSAPAN_STUB("SdStopSQAll", 0x018ce310);
}

// stub of CT3 0x018cc350
int SdReqPlaySESp1(int jls, int a1, int a2, int a3, int a4, int a5, int a6, void (*update)(int handle), int a7,
                   int a8)
{
    ALYSSAPAN_STUB("SdReqPlaySESp1", 0x018cc350);
    return {};
}

// stub of CT3 0x018ce640
void SdLoadSQEndWaitAll(void)
{
    ALYSSAPAN_STUB("SdLoadSQEndWaitAll", 0x018ce640);
}

// stub of CT3 0x018d1860
void IopLoadCapcomSound(const char *device)
{
    ALYSSAPAN_STUB("IopLoadCapcomSound", 0x018d1860);
}

// stub of CT3 0x018d1390
void SdProc(void)
{
    ALYSSAPAN_STUB("SdProc", 0x018d1390);
}
