// game/movie.c -- CT3 0x018bda90..0x018bf9d0: the movie player (SCE's ezmpegstr main.c, AA_-prefixed and
// grown into the game). Slice 1 reconstructed 2026-11-10 (16 of the symtab's 20 functions; 0x018bf05c is
// AA_ToSmall's loop test, which Ghidra splits off as a thunk target).
//
// A scene script calls AA_OpenMovie(movie, caption, skip) (the names, and the caption file read from
// data/caption/ to EE 0x200000), then AA_PlayMovieReady; main's frame loop calls AA_MovieProc, which runs
// PlayMovie (QuatSlerp.c) -> AA_StartMovie. That waits for the file system, picks 20 of banks 6..85 for
// the decoder's six memory blocks (AA_MovieUseBank; AA_AutoBank_Seelect drops scene banks to make room),
// resets the GS for a 512 x 448 movie frame and plays data/movie1..3/<name> through AA_movie.
//
// Slice 2 (to come): AA_movie, initAll, readMpeg, termAll -- the decode loop (threads, IPU, the DMA end
// handler). On the host that loop drives sdk/libmpeg (FFmpeg) and draws through the movie seam.
//
// PORT: the bank memory words (*AA_MovieUseBank[].work, AA_mpegWork, ...) stay EE addresses (BankInfo's
// `image`; open-items.md item 1 (h)); the caption file is read to a host block, AA_MojiImage.

#include "game/movie.h"

#include "common/alyssapan_stub.h"
#include "game/AA_GetDataWord.h"
#include "game/ArtoolDebugOpen.h"
#include "game/Character_Init.h"
#include "game/Clipping_Portal.h"
#include "game/QuatSlerp.h"
#include "game/ahalloc.h"
#include "game/ct3_types.h"
#include "game/fileio.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "sdk/eekernel.h"
#include "sdk/libdma.h"
#include "sdk/libgraph.h"
#include "sdk/sdremote.h"

#include <stdio.h>
#include <string.h>

#define AA_DBUFF_CLEAR 1                /* sceGsSetDefDBuff's clear argument: clear each frame */

// ---- data ---------------------------------------------------------------------------------------------
int AA_isWithAudio = 1;                                     /* sdata 0x01939be0 */

static const char s_mpegDecodeWork[] = "MPEG decode Work";          /* @60 0x01935360 */
static const char s_mpegPictureBuffer0[] = "MPEG Picture Buffer 0"; /* @61 0x01935380 */
static const char s_mpegPictureBuffer1[] = "MPEG Picture Buffer 1"; /* @62 0x019353a0 */
static const char s_mpegVideoStream[] = "MPEG Video stream";        /* @63 0x019353c0 */
static const char s_mpegGsTag0[] = "MPEG GS tag 0";                 /* @64 0x019353d8 */
static const char s_mpegGsTag1[] = "MPEG Gs tag 1";                 /* @65 0x019353e8 */

AA_MOVIE_BANK AA_MovieUseBank[AA_MOVIE_WORKS] = {           /* sdata 0x01939bf0 */
    { -1, 8, &AA_mpegWork, s_mpegDecodeWork },
    { -1, 4, &AA_voBufData[0], s_mpegPictureBuffer0 },
    { -1, 4, &AA_voBufData[1], s_mpegPictureBuffer1 },
    { -1, 2, &AA_viBufData, s_mpegVideoStream },
    { -1, 1, &AA_voBufTag[0].tag[0], s_mpegGsTag0 },
    { -1, 1, &AA_voBufTag[1].tag[0], s_mpegGsTag1 },
};

static int AA_FileHandole;                                  /* sbss 0x0193f0d4: the caption file */
int AA_SOUND_MODE;                                          /* sbss 0x0193f0d8 */
int AA_MvInWhite;                                           /* sbss 0x0193f0dc */
int AA_MvOutWhite;                                          /* sbss 0x0193f0e0 */
int AA_MvFedeOutCut;                                        /* sbss 0x0193f0e4 */
sceGsDBuff db;                                              /* sbss 0x0193f0f0 */
int AA_YOffset;                                             /* sbss 0x0193f320 */
int AA_FadeInCount;                                         /* sbss 0x0193f324 */
int AA_MvFedeOut2Md;                                        /* sbss 0x0193f328 */
int AA_WaitCount;                                           /* sbss 0x0193f32c */
u_int AA_MojiData;                                          /* sbss 0x0193f330 */
int AA_Moive_Skip;                                          /* sbss 0x0193f334 */
int controller_val;                                         /* sbss 0x0193f338 */
int AA_frd;                                                 /* sbss 0x0193f33c */
VoBuf voBuf;                                                /* sbss 0x0193f340 */
AudioDec AA_audioDec;                                       /* sbss 0x0193f390 */
VideoDec AA_videoDec;                                       /* sbss 0x0193f410 */
StrFile AA_infile;                                          /* sbss 0x0193f4f0 */
int AA_defaultTh;                                           /* sbss 0x0193f528 */
int AA_videoDecTh;                                          /* sbss 0x0193f52c */
int AA_CdDvd_trial_Ct;                                      /* sbss 0x0193f530 */
int AA_FadeOutEnable;                                       /* sbss 0x0193f534 */
u_int *AA_FadePackAdr;                                      /* sbss 0x0193f538 */
static int AA_MovieAbortFlag;                               /* sbss 0x0193f53c (readMpeg: slice 2) */
static u_int AA_MovieStopOkFlag;                            /* sbss 0x0193f540: AA_MOVIE_SKIP_OK */
u_int *AA_DISP_TAG;                                         /* sbss 0x0193f544 */
int AA_MovieRedyFlag;                                       /* sbss 0x0193f548 */
static char AA_MovFiNameWk[0x100];                          /* sbss 0x0193f550: the path AA_movie opens */
static char AA_MoMiFileName[0x20];                          /* sbss 0x0193f650: the caption file */
static char AA_MoviFileName[0x20];                          /* sbss 0x0193f670: the movie file */
static void *AA_defStack;                                   /* sbss 0x0193f690 */
static void *AA_videoDecStack;                              /* sbss 0x0193f694 */
static void *AA_audIOPTimeStamp;                            /* sbss 0x0193f698 */
static void *AA_audEETimeStamp;                             /* sbss 0x0193f69c */
static void *AA_timeStamp;                                  /* sbss 0x0193f6a0 */
static void *AA_audioBuff;                                  /* sbss 0x0193f6a4 */
static ReadBuf AA_readBuf;                                  /* sbss 0x0193f6b0 */
u_int AA_viBufData;                                         /* sbss 0x0193f6c0 */
u_int AA_MoiveMojiFB;                                       /* sbss 0x0193f6c4 */
u_int AA_mpegWork;                                          /* sbss 0x0193f6c8 */
static void *AA_viBufTag;                                   /* sbss 0x0193f6cc */
VoTag AA_voBufTag[AA_VOBUF_COUNT];                          /* sbss 0x0193f6d0 */
u_int AA_voBufData[AA_VOBUF_COUNT];                         /* sbss 0x0193f700 */

// PORT: the caption file's home. The ROM reads it to EE 0x200000 (AA_CAPTION_ADDR, below the ELF) with no
// size limit; the host gives it 1 MB (open-items.md (fu)).
#define AA_CAPTION_MAX 0x100000
u_char AA_MojiImage[AA_CAPTION_MAX];

static const char s_errMessage[] = "[ Error ] %s\n";                           /* @890 0x019353f8 */
static const char s_movie1[] = "%sdata/movie1/%s";                             /* @1166 0x01935490 */
static const char s_movie2[] = "%sdata/movie2/%s";                             /* @1167 0x019354b0 */
static const char s_movie3[] = "%sdata/movie3/%s";                             /* @1168 0x019354d0 */
static const char s_openMpegFile[] = "Open Mpeg File \"%s\" \"%s\",%d\n";       /* @1199 0x019354f0 */
static const char s_captionDir[] = "data/caption/";                            /* @1200 0x01935510 */

// The candidate list AA_AutoBank_Seelect sorts: loaded scene groups it may drop (12 bytes a row).
typedef struct AA_BANK_CANDIDATE {
    int state;                          /* 0x00: 0, AA_BANK_TAKEN once a row took it, -1 ends the list */
    int bank;                           /* 0x04: the group's head */
    int count;                          /* 0x08: its groupCount */
} AA_BANK_CANDIDATE;
static_assert(sizeof(AA_BANK_CANDIDATE) == 0xc, "AA_BANK_CANDIDATE");

static int AA_DeleteSceneBank(int bank);
static int AA_movie(const char *name);

// ---- code ---------------------------------------------------------------------------------------------

// PORT: the ROM writes this frame wait out at each of its waits (AA_StartMovie, AA_movie).
static void AA_WaitFrame(void)
{
    FlProc();
    FileSoundServer();
    sceGsSyncV(0);
    if (AA_WaitCount > 0)
        AA_WaitCount--;
}

// Banks the global scene heap reaches into, counted from bank 6 (rounded up to whole banks). The ROM
// computes it twice in AA_AutoBank_Seelect.
static int AA_GlobalHeapBanks(void)
{
    int used = (int)((u_char *)SunAllocUsedBorderCheck(&gHeapGlobalScene) - (u_char *)gHeapGlobalScene.top);
    return (used + BANK_SLOT_SIZE - 1) / BANK_SLOT_SIZE;
}

// CT3 0x018bda90: choose the banks of every AA_MovieUseBank row still at -1. A map of banks 6..85 marks
// what the global heap covers, what is drawn, loading or not a scene bank (taken), and loaded scene banks.
// Per row: (1) the first free run long enough; (2) else the smallest loaded scene group at least as long
// (sorted by size), dropped; (3) else the first run with nothing taken, every group in it dropped.
void AA_AutoBank_Seelect(void)
{
    u_char *map;
    AA_BANK_CANDIDATE *cand;
    int i, j, k, n, row, count, searching, swapped, ok;

    map = (u_char *)_SunAlloc(&gHeapSound, AA_BANK_MAP_SIZE, AA_HEAP_ALIGN);
    for (i = 0; i < BANK_MAX; i++)
        map[i] = AA_BANK_FREE;
    n = AA_GlobalHeapBanks();
    for (i = AA_BANK_FIRST; i < n + AA_BANK_FIRST; i++)
        map[i] = AA_BANK_TAKEN;
    for (i = AA_BANK_FIRST; i < BANK_MAX; i++) {
        if (BankInfo[i].state != BANK_STATE_FREE) {
            map[i] = AA_BANK_SCENE;
            if (BankInfo[i].drawOn != 0)
                map[i] = AA_BANK_TAKEN;
            if (BankInfo[i].state < 0)
                map[i] = AA_BANK_TAKEN;
            if (BankInfo[i].type != BANK_TYPE_SHADOW)
                map[i] = AA_BANK_TAKEN;
        }
    }

    cand = (AA_BANK_CANDIDATE *)_SunAlloc(&gHeapSound, AA_BANK_CANDIDATES * (int)sizeof(AA_BANK_CANDIDATE),
                                          AA_HEAP_ALIGN);
    n = 0;
    for (i = AA_GlobalHeapBanks() + AA_BANK_FIRST; i < BANK_MAX; i++) {
        if (BankInfo[i].state > 0 && BankInfo[i].drawOn == 0) {
            cand[n].state = 0;
            cand[n].bank = i;
            cand[n].count = BankInfo[i].groupCount;
            n++;
            i += BankInfo[i].groupCount - 1;
        }
    }
    cand[n].state = -1;
    cand[n].bank = -1;
    cand[n].count = -1;
    // Bubble sort by size (bank and count move; state stays). As in the ROM, n is decremented before each
    // pass and sorting stops at n <= 1, so a list of two is never sorted.
    swapped = 1;
    while (--n > 1 && swapped) {
        swapped = 0;
        for (k = 0; k < n; k++) {
            if (cand[k + 1].count < cand[k].count) {
                int t = cand[k].bank;
                swapped = 1;
                cand[k].bank = cand[k + 1].bank;
                cand[k + 1].bank = t;
                t = cand[k].count;
                cand[k].count = cand[k + 1].count;
                cand[k + 1].count = t;
            }
        }
    }

    for (row = 0; row < AA_MOVIE_WORKS; row++) {
        AA_MOVIE_BANK *use = &AA_MovieUseBank[row];

        if (use->bank != -1)
            continue;
        // (1) a free run
        searching = 1;
        for (i = AA_BANK_FIRST; i < BANK_MAX && searching; i++) {
            ok = 1;
            for (j = i; j < i + use->count; j++) {
                if (map[j] != AA_BANK_FREE)
                    ok = 0;
            }
            if (ok && i + use->count < BANK_MAX) {
                use->bank = i;
                searching = 0;
                for (j = i; j < i + use->count; j++)
                    map[j] = AA_BANK_TAKEN;
            }
        }
        // (2) the smallest scene group that holds it
        if (searching) {
            int looking = 1;
            for (k = 0; cand[k].bank != -1 && looking; k++) {
                int b = cand[k].bank;
                if (use->count <= cand[k].count && b + use->count < BANK_MAX && cand[k].state != AA_BANK_TAKEN
                    && map[b] != AA_BANK_TAKEN && BankInfo[b].type == BANK_TYPE_SHADOW) {
                    int rest, next;
                    use->bank = b;
                    cand[k].state = AA_BANK_TAKEN;
                    count = use->count;
                    for (j = b; j < b + count; j++)
                        map[j] = AA_BANK_TAKEN;
                    rest = cand[k].count;
                    next = cand[k].bank + count;
                    AA_DeleteSceneBank(cand[k].bank);
                    for (rest -= count; rest > 0; rest--)
                        map[next++] = AA_BANK_FREE;     // the group's tail is free again
                    looking = 0;
                }
            }
        }
        // (3) the first run with nothing taken: drop what is there
        if (use->bank == -1) {
            count = use->count;
            i = AA_BANK_FIRST;
            do {
                ok = 1;
                for (j = i; j < i + count && j < BANK_MAX; j++) {
                    if (map[j] == AA_BANK_TAKEN)
                        ok = 0;
                }
                if (j + count > BANK_MAX)               // the ROM tests where the scan ended, plus count
                    ok = 0;
            } while (!ok && ++i < BANK_MAX);
            if (ok && i + count < BANK_MAX) {
                for (j = i; j < i + use->count; j++) {
                    if (BankInfo[j].state != BANK_STATE_FREE) {
                        int group = BankInfo[j].groupCount;
                        AA_DeleteSceneBank(j);
                        for (k = j; k < j + group && k < BANK_MAX; k++)
                            map[k] = AA_BANK_FREE;
                        j += group - 1;
                    }
                }
                for (j = i; j < i + use->count; j++)
                    map[j] = AA_BANK_TAKEN;
                use->bank = i;
            }
        }
    }
    _SunFree(&gHeapSound, (void **)&cand);
    _SunFree(&gHeapSound, (void **)&map);
}

// CT3 0x018be3e0: drop a loaded group (parked for the movie, fileio.c's SceneEndBankMovie). Answers its
// size, 0 when `bank` is not a loaded group's head.
static int AA_DeleteSceneBank(int bank)
{
    int count = 0;

    if (BankInfo[bank].state == BANK_STATE_LOADED) {
        count = BankInfo[bank].groupCount;
        SceneEndBankMovie(bank, count);
    }
    return count;
}

// CT3 0x018be440: are the chosen banks usable as they are? 0 yes; 1 a row has no bank; 3 a row's first
// bank holds something that is not a loaded group's head; 2 one of a row's banks is drawn.
static int AA_MovieWorkCheck(void)
{
    int row, j, bank;

    for (row = 0; row < AA_MOVIE_WORKS; row++) {
        bank = AA_MovieUseBank[row].bank;
        if (bank < 0)
            return 1;
        if (BankInfo[bank].state != BANK_STATE_FREE) {
            if (BankInfo[bank].state != BANK_STATE_LOADED)
                return 3;
            for (j = 0; j < AA_MovieUseBank[row].count; j++) {
                if (BankInfo[bank + j].drawOn != 0)
                    return 2;
            }
        }
    }
    return 0;
}

// CT3 0x018be290: empty the chosen banks and hand each row its memory. 1 a row has no bank, 2 one is
// drawn, 0 done.
static int AA_MovieWorkClean(void)
{
    int row, j, bank, loaded, drawn;

    row = 0;
    do {
        bank = AA_MovieUseBank[row].bank;
        if (bank < 0)
            return 1;
        loaded = 0;
        drawn = 0;
        for (j = 0; j < AA_MovieUseBank[row].count; j++) {
            if (BankInfo[bank + j].state != BANK_STATE_FREE)
                loaded = 1;
            if (BankInfo[bank + j].drawOn != 0)
                drawn = 1;
        }
        if (drawn)
            return 2;
        if (loaded) {
            // The ROM adds each dropped group's size, less one, to the ROW index, not to j: a dropped group
            // of n banks skips the next n - 1 rows (and their memory). Kept as the ROM has it.
            for (j = 0; j < AA_MovieUseBank[row].count; j++)
                row += AA_DeleteSceneBank(bank + j) - 1;
        }
        *AA_MovieUseBank[row].work = BankInfo[bank].image;
        row++;
    } while (row < AA_MOVIE_WORKS);
    return 0;
}

// CT3 0x018be510: feed the IOP's audio ring (the ROM tail-calls it).
int AA_proceedAudio(void)
{
    return AA_audioDecSendToIOP(&AA_audioDec);
}

// CT3 0x018be520
int ErrMessage(const char *message)
{
    return printf(s_errMessage, message);
}

// CT3 0x018be7b0: the audio is ready to start (always, without audio). readMpeg's (slice 2).
static int isAudioOK(void)
{
    if (AA_isWithAudio != 0)
        return AA_audioDecIsPreset(&AA_audioDec);
    return 1;
}

// CT3 0x018be7e0: let the decoder thread run (the ROM tail-calls RotateThreadReadyQueue).
void AA_switchThread(void)
{
    RotateThreadReadyQueue(AA_THREAD_PRIORITY);
}

// CT3 0x018beba0: play `name` (slice 2). Answers controller_val.
static int AA_movie(const char *name)
{
    (void)name;
    (void)isAudioOK;
    (void)AA_MovieAbortFlag;
    ALYSSAPAN_STUB("AA_movie", 0x018beba0);
    return 0;
}

// CT3 0x018bf030: lower-case `s` in place (A..Z only).
void AA_ToSmall(char *s)
{
    for (; *s != '\0'; s++) {
        if (*s > '@' && *s < '[')
            *s += ' ';
    }
}

// CT3 0x018bf080: play the movie AA_OpenMovie named (PlayMovie's call).
void AA_StartMovie(void)
{
    int row, ok;
    const char *fmt;

    ssTrigRemoveEvent(TRIG_EVENT_MOVIE_SKIPPED, 1, 0);
    AA_WaitCount = AA_START_WAIT;
    AA_GS_SetClut(AA_CLUT_CBP);
    if (AA_MojiData != 0 && AA_FileHandole != 0) {
        while (FlIsExecute(AA_FileHandole) != 0)
            AA_WaitFrame();
        FlReqClose(AA_FileHandole);
        AA_FileHandole = 0;
    }
    while (IsFSLoading() != 0 || IsMltLoading() != 0)
        AA_WaitFrame();
    SSSoundDisable();

    if (AA_MovieUseBank[0].bank == -1) {
        AA_AutoBank_Seelect();
        ok = 1;
        for (row = 0; row < AA_MOVIE_WORKS; row++) {
            if (AA_MovieUseBank[row].bank == -1)
                ok = 0;
            else
                *AA_MovieUseBank[row].work = BankInfo[AA_MovieUseBank[row].bank].image;
        }
        if (!ok) {
            AA_MovieUseBank[0].bank = -1;
            return;
        }
    } else {
        if (AA_MovieWorkCheck() != 0) {
            AA_MovieUseBank[0].bank = -1;
            return;
        }
        if (AA_MovieWorkClean() != 0) {
            AA_MovieUseBank[0].bank = -1;
            return;
        }
    }

    sceGsSyncV(0);
    if (AA_WaitCount > 0)
        AA_WaitCount--;
    sceGsResetPath();
    sceDmaReset(1);
    sceGsResetGraph(1, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FRAME);
    sceGsSetDefDBuff(&db, SCE_GS_PSMCT32, AA_RESET_WIDTH, AA_RESET_HEIGHT, SCE_GS_ZNOUSE, 0, AA_DBUFF_CLEAR);
    // The game's DISPLAY with DX snapped to 0x27c + 5n; both buffers show it.
    db.disp[0].display = ga_disp.display;
    db.disp[0].display.DX = (((int)ga_disp.display.DX - AA_DISPLAY_DX_BASE) / AA_DISPLAY_DX_STEP)
                            * AA_DISPLAY_DX_STEP + AA_DISPLAY_DX_BASE;
    AA_YOffset = ((int)ga_disp.display.DY - AA_DISPLAY_DY_BASE) & 1;
    db.disp[1].display = db.disp[0].display;
    if (AA_MvInWhite == AA_MV_WHITE)
        AA_clearGsMem(0xff, 0xff, 0xff, AA_RESET_WIDTH, AA_RESET_HEIGHT);
    else
        AA_clearGsMem(0, 0, 0, AA_RESET_WIDTH, AA_RESET_HEIGHT);

    // A packed name plays from the pack; otherwise data/movie1 (s...), movie2 (m...) or movie3, lower case.
    if (AA_MoviFileName[0] == 'm' || AA_MoviFileName[0] == 'M')
        fmt = s_movie2;
    else if (AA_MoviFileName[0] == 's' || AA_MoviFileName[0] == 'S')
        fmt = s_movie1;
    else
        fmt = s_movie3;
    strcpy(AA_MovFiNameWk, AA_MoviFileName);
    if (FlFindPacked(AA_MovFiNameWk) < 0) {
        AA_ToSmall(AA_MoviFileName);
        sprintf(AA_MovFiNameWk, fmt, gDeviceName, AA_MoviFileName);
    }

    AA_movie(AA_MovFiNameWk);
    Ass_StopIopDataTrans();
    SSSoundEnable();
    for (row = AA_MOVIE_WORKS - 1; row >= 0; row--)
        AA_MovieUseBank[row].bank = -1;
}

// CT3 0x018bf4f0: start the movie on the next frame (AA_MovieProc).
int AA_PlayMovieReady(void)
{
    MoviePlayCheck = 1;
    AA_MovieRedyFlag = 1;
    return 0;
}

// CT3 0x018bf510: name the next movie and its caption file (none when `caption` is empty), and start
// reading the captions (not with TRIG_EVENT_OMAKE_2 registered). `skip` (low 16 bits): AA_MOVIE_SKIP_OK
// lets the pad end it. 0 when the caption file does not open, 1 otherwise.
int AA_OpenMovie(const char *movie, const char *caption, u_int skip)
{
    int ret;

    tiVibrationOff(0);
    skip &= 0xffff;
    AA_MvInWhite = AA_MV_FADE;
    AA_MvOutWhite = AA_MV_BLACK;
    AA_MvFedeOut2Md = 0;
    AA_MvFedeOutCut = 0;
    AA_MovieStopOkFlag = skip;
    FSMltLoadEndWaitAll();
    printf(s_openMpegFile, movie, caption, skip);
    memset(AA_MoviFileName, 0, sizeof(AA_MoviFileName));
    memset(AA_MoMiFileName, 0, sizeof(AA_MoMiFileName));
    strncpy(AA_MoviFileName, movie, sizeof(AA_MoviFileName) - 1);
    strncpy(AA_MoMiFileName, caption, sizeof(AA_MoMiFileName) - 1);
    AA_Moive_Skip = (int)skip;
    if (AA_MoMiFileName[0] == '\0') {
        AA_MojiData = 0;
    } else {
        AA_MojiData = 0;
        if (AA_FileHandole != 0) {
            FlReqClose(AA_FileHandole);
            AA_FileHandole = 0;
        }
        AA_FileHandole = FlReqOpen(s_captionDir, AA_MoMiFileName);
        if (AA_FileHandole == 0) {
            AA_MojiData = 0;
            return 0;
        }
        ret = -1;
        AA_MojiData = AA_CAPTION_ADDR;
        if (ssTrigCheckEvent(TRIG_EVENT_OMAKE_2, 1, -1) == 0)
            ret = FlReqRead(AA_FileHandole, AA_MojiImage, -1, -1);  // PORT: the ROM reads to EE 0x200000
        if (ret < 0) {
            AA_MojiData = 0;
            FlReqClose(AA_FileHandole);
            AA_FileHandole = 0;
        }
    }
    AA_MvFedeOutCut = 0;
    return 1;
}

// CT3 0x018bf6c0: main's frame hook: play a movie AA_PlayMovieReady asked for.
void AA_MovieProc(void)
{
    if (AA_MovieRedyFlag != 0) {
        AA_MovieRedyFlag = 0;
        PlayMovie();
    }
}

// CT3 0x018bf6f0: give the decoder's gHeapSound blocks back.
void AA_MemoryMapFree(void)
{
    voBuf.w34[0] = 0;
    if (AA_defStack != NULL) {
        _SunFree(&gHeapSound, &AA_defStack);
        AA_defStack = NULL;
    }
    if (AA_videoDecStack != NULL) {
        _SunFree(&gHeapSound, &AA_videoDecStack);
        AA_videoDecStack = NULL;
    }
    if (AA_audioBuff != NULL) {
        _SunFree(&gHeapSound, &AA_audioBuff);
        AA_audioBuff = NULL;
    }
    if (AA_audIOPTimeStamp != NULL) {
        _SunFree(&gHeapSound, &AA_audIOPTimeStamp);
        AA_audIOPTimeStamp = NULL;
    }
    if (AA_audEETimeStamp != NULL) {
        _SunFree(&gHeapSound, &AA_audEETimeStamp);
        AA_audEETimeStamp = NULL;
    }
    if (AA_timeStamp != NULL) {
        _SunFree(&gHeapSound, &AA_timeStamp);
        AA_timeStamp = NULL;
    }
    if (AA_viBufTag != NULL) {
        _SunFree(&gHeapSound, &AA_viBufTag);
        AA_viBufTag = NULL;
    }
    if (AA_readBuf.data != NULL) {
        _SunFree(&gHeapSound, (void **)&AA_readBuf.data);
        AA_readBuf.data = NULL;
    }
}

// CT3 0x018bf7d0: take the decoder's gHeapSound blocks (the read ring stays between movies). 1 when one
// failed (everything is given back), 0 when all are there.
int AA_MemoryMapSet(void)
{
    int failed = 0;

    voBuf.w34[0] = 0;
    voBuf.w34[1] = 0;
    voBuf.w34[2] = 0;
    AA_voBufTag[0].tag[1] = AA_voBufTag[0].tag[0] + AA_GS_TAG_STRIDE;
    voBuf.w34[3] = 0;
    voBuf.w34[4] = 0;
    AA_voBufTag[1].tag[1] = AA_voBufTag[1].tag[0] + AA_GS_TAG_STRIDE;
    if (AA_readBuf.data == NULL
        && (AA_readBuf.data = (u_char *)_SunAlloc(&gHeapSound, AA_READBUF_SIZE, AA_HEAP_ALIGN)) == NULL)
        failed = 1;
    if ((AA_viBufTag = _SunAlloc(&gHeapSound, AA_VIBUFTAG_SIZE, AA_HEAP_ALIGN)) == NULL)
        failed = 1;
    if ((AA_timeStamp = _SunAlloc(&gHeapSound, AA_TIMESTAMP_SIZE, AA_HEAP_ALIGN)) == NULL)
        failed = 1;
    if ((AA_audEETimeStamp = _SunAlloc(&gHeapSound, AA_AUD_TIMESTAMP_SIZE, AA_HEAP_ALIGN)) == NULL)
        failed = 1;
    if ((AA_audIOPTimeStamp = _SunAlloc(&gHeapSound, AA_AUD_TIMESTAMP_SIZE, AA_HEAP_ALIGN)) == NULL)
        failed = 1;
    if ((AA_audioBuff = _SunAlloc(&gHeapSound, AA_AUDIOBUFF_SIZE, AA_HEAP_ALIGN)) == NULL)
        failed = 1;
    if ((AA_videoDecStack = _SunAlloc(&gHeapSound, AA_THREAD_STACK_SIZE, AA_HEAP_ALIGN)) == NULL)
        failed = 1;
    if ((AA_defStack = _SunAlloc(&gHeapSound, AA_THREAD_STACK_SIZE, AA_HEAP_ALIGN)) == NULL)
        failed = 1;
    if (!failed) {
        voBuf.w34[0] = 0;
        voBuf.w34[1] = 0;
        voBuf.w34[2] = 0;
        voBuf.w34[3] = 0;
        voBuf.w34[4] = 0;
    } else {
        AA_MemoryMapFree();
    }
    return failed;
}

// CT3 0x018bf980: the movie player's start-up (Character_Init's IOP set-up).
void AA_MovieIOPInit(void)
{
    AA_isWithAudio = 1;
    sceSdRemoteInit();
    AA_defStack = NULL;
    AA_videoDecStack = NULL;
    AA_readBuf.data = NULL;
    AA_MojiData = 0;
    AA_strFileInit(&AA_infile);
    audioDecInit(&AA_audioDec, AA_IOP_AUDIO_BUFF);
}
