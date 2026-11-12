// game/movie.c -- CT3 0x018bda90..0x018bf9d0: the movie player (SCE's ezmpegstr main.c, AA_-prefixed and
// grown into the game). All 20 of the symtab's functions reconstructed (slices 1 and 2, 2026-11-10; 0x018bf05c
// is AA_ToSmall's loop test, which Ghidra splits off as a thunk target).
//
// A scene script calls AA_OpenMovie(movie, caption, skip) (the names, and the caption file read from
// data/caption/ to EE 0x200000), then AA_PlayMovieReady; main's frame loop calls AA_MovieProc, which runs
// PlayMovie (BRANCH_CLEAR.c) -> AA_StartMovie. That waits for the file system, picks 20 of banks 6..85 for
// the decoder's six memory blocks (AA_MovieUseBank; AA_AutoBank_Seelect drops scene banks to make room),
// resets the GS for a 512 x 448 movie frame and plays data/movie1..3/<name> through AA_movie.
//
// Slice 2 (2026-11-10): AA_movie, initAll, readMpeg, termAll -- the decode loop, kept in the ROM's shape:
// the host EE kernel runs the decoder thread (AA_videoDecMain) and the vblank handler, sdk/libmpeg
// (FFmpeg behind sceMpegDemuxPssRing / sceMpegGetPicture) decodes, and the picture reaches the screen
// through AA_GetDataWord.c's display code (its TU decides the host draw: no GIF transfer-end interrupt
// fires on the host, AddDmacHandler registers nothing).
//
// PORT: the bank memory words (*AA_MovieUseBank[].work, AA_mpegWork, ...) stay EE addresses (BankInfo's
// `image`; open-items.md item 1 (h)); the caption file is read to a host block, AA_MojiImage.

#include "game/movie.h"

#include "game/AA_GetDataWord.h"
#include "game/AA_cpuCycles.h"
#include "game/ArtoolDebugOpen.h"
#include "game/Character_Init.h"
#include "game/Clipping_Portal.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ahalloc.h"
#include "game/copy2area.h"
#include "game/ct3_types.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "ga/pad_button_read.h"
#include "alyssapan/core/movie_skip.h"
#include "sdk/eekernel.h"
#include "sdk/libdma.h"
#include "sdk/eeregs.h"
#include "sdk/libgraph.h"
#include "sdk/libmpeg.h"
#include "sdk/sdremote.h"

#include <stdio.h>
#include <string.h>

#define AA_DBUFF_CLEAR 1                /* sceGsSetDefDBuff's clear argument: clear each frame */

// initAll's decoder set-up (the ROM's literals).
#define AA_D_CTRL_DMAE_RELE 3           /* D_CTRL |= DMAE | RELE */
#define AA_D_STAT_CIS2 4                /* D_STAT: clear channel 2's (GIF) interrupt status */
#define AA_MPEG_WORK_SIZE 0x121768      /* AA_videoDecCreate's work: in the 8 "MPEG decode Work" banks */
#define AA_VIBUF_TAGS 0x100             /* the video stream's DMA tags (AA_viBufTag, 0x1010 bytes) */
#define AA_TIMESTAMPS 0x200             /* the video's time stamps (AA_timeStamp) */
#define AA_AUD_TIMESTAMPS 0x40          /* the audio's EE and IOP time stamps */
#define AA_VIDEO_STACK_SIZE 0x20000     /* the decoder thread's stack, as CreateThread is told */
#define AA_MOJI_FB_OFFSET 0x1e4000      /* AA_MoiveMojiFB: the captions' buffer inside the decode work */

// readMpeg's stream loop.
#define AA_READ_UNIT 0x10000            /* bytes read per turn, when the ring has that much free */
#define AA_DEMUX_TAIL 4                 /* the loop ends with this many bytes (or fewer) left to demux */
#define AA_SKIP_MIN_FRAMES 0x10         /* decoded frames before the pad may skip */
#define AA_AUDIO_START_DELAY 0xb9533b   /* EE cycles (~41 ms) between the first picture and the audio */
#define KEY_MAP_MOVIE_SKIP 11           /* KEY_MAP[11] (+0x16): main.c's KEY_MAP_SKIP */
#define KEY_MAP_MOVIE_STOP 20           /* KEY_MAP[20] (+0x28): the second button that stops a skippable movie */

// AA_movie's ending.
#define AA_FADE_FRAMES 0x3c             /* the fade-out's frames; AA_FadeInCount's too */
#define AA_DISP_TAG_SIZE 0x400          /* AA_DISP_TAG: 0x100 words of 0xffffffff */
#define AA_DISP_TAG_ALIGN 0x80
#define AA_TI_FADE_FROM_BLACK 4         /* tiFade_Start's type after a fade to black (FADE_TYPE_MC_LOAD) */
#define AA_TI_FADE_FROM_WHITE 5         /*   and after a fade to white */
#define AA_FILE_OPEN_ERROR_LINE 0x4b5   /* __LINE__ in the ROM's movie.c (1205) */

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
static int AA_MovieAbortFlag;                               /* sbss 0x0193f53c: readMpeg skipped the movie */
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
static const char s_useMovieMemoryBank[] = "Use Movie Memory Bank \n";          /* @1079 0x01935410 */
static const char s_bankRange[] = "\t%02d-%02d(%d) ";                           /* @1080 0x01935430 */
static const char s_bankState[] = "%d ";                                        /* @1081 0x01935440 */
static const char s_newLine[] = "\n";                                           /* @1082 0x01935448 */
static const char s_fileOpenError[] = "File Open Error!! \n\n  %s \n\n      %s line:%d"; /* @1083 0x01935450 */
static const char s_movieC[] = "movie.c";                                         /* @1084 0x01935480 */

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

// CT3 0x018be530: undo initAll (and stop AA_defaultTh, which CT3 never creates: the kernel refuses id 0).
static void termAll(void)
{
    AA_readBufDelete(&AA_readBuf);
    AA_voBufDelete(&voBuf);
    TerminateThread(AA_videoDecTh);
    DeleteThread(AA_videoDecTh);
    TerminateThread(AA_defaultTh);
    DeleteThread(AA_defaultTh);
    DisableDmac(DMAC_GIF);
    RemoveDmacHandler(DMAC_GIF, AA_videoDec.hid_endimage);
    DisableIntc(INTC_VBLANK_S);
    RemoveIntcHandler(INTC_VBLANK_S, AA_videoDec.hid_vblank);
    AA_videoDecDelete(&AA_videoDec);
    if (AA_isWithAudio != 0)
        AA_audioDecDelete(&AA_audioDec);
    AA_strFileClose(&AA_infile);
}

// CT3 0x018be5d0: set the decoder up for `name`: the read ring, libmpeg, the video and audio decoders (the
// video stream to AA_videoCallback, the PCM stream to AA_pcmCallback, both through the read ring), the
// picture queue, the decoder thread; open the file (retrying every vsync until it opens) and hook the
// vblank and the GIF transfer end.
static void initAll(const char *name)
{
    ThreadParam th = {};

    REG_DMAC_CTRL = REG_DMAC_CTRL | AA_D_CTRL_DMAE_RELE;
    *D_STAT = AA_D_STAT_CIS2;
    AA_MovieAbortFlag = 0;
    AA_readBufCreate(&AA_readBuf);
    sceMpegInit();
    AA_videoDecCreate(&AA_videoDec, AA_mpegWork, AA_MPEG_WORK_SIZE, AA_viBufData, AA_viBufTag, AA_VIBUF_TAGS,
                      AA_timeStamp, AA_TIMESTAMPS);
    if (AA_isWithAudio != 0)
        audioDecCreate(&AA_audioDec, AA_audioBuff, AA_AUDIOBUFF_SIZE, AA_IOP_AUDIO_BUFF, AA_audEETimeStamp,
                       AA_AUD_TIMESTAMPS, AA_audIOPTimeStamp, AA_AUD_TIMESTAMPS);
    AA_videoDecSetStream(&AA_videoDec, sceMpegStrM2V, 0, AA_videoCallback, &AA_readBuf);
    if (AA_isWithAudio != 0)
        AA_videoDecSetStream(&AA_videoDec, sceMpegStrPCM, 0, AA_pcmCallback, &AA_readBuf);
    AA_voBufCreate(&voBuf, AA_voBufData, AA_voBufTag, AA_VOBUF_COUNT);  // PORT: the ROM passes the uncached alias
                                                                       // 0x2193f700 (UncAddr)
    th.entry = AA_videoDecMain;
    th.stack = AA_videoDecStack;
    th.stackSize = AA_VIDEO_STACK_SIZE;  // the ROM's 0x20000, though AA_MemoryMapSet allocates 0x10800
    th.gpReg = NULL;                     // the ROM passes &_gp
    th.initPriority = AA_THREAD_PRIORITY;
    th.option = 0;
    AA_videoDecTh = CreateThread(&th);
    StartThread(AA_videoDecTh, &AA_videoDec);
    AA_CdDvd_trial_Ct = 0xff;
    while (AA_strFileOpen(&AA_infile, name) == 0)
        sceGsSyncV(0);
    AA_videoDec.hid_vblank = AddIntcHandler(INTC_VBLANK_S, AA_vblankHandler, 0);
    EnableIntc(INTC_VBLANK_S);
    AA_videoDec.hid_endimage = AddDmacHandler(DMAC_GIF, AA_handler_endimage, 0);
    EnableDmac(DMAC_GIF);
}

// CT3 0x018be7b0: the audio is ready to start (always, without audio). readMpeg's.
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

// CT3 0x018be7f0: stream the file through the decoder (ezmpegstr's readMpeg, with CT3's pad skip). Each
// turn: read the pad; past 15 decoded frames the skip buttons may abort the movie; read 64 KB into the ring
// when that much is free; let the decoder thread run; demux what the ring holds; feed the IOP. The picture
// starts once both picture buffers are full, the audio is preset and AA_WaitCount ran out: the display
// first, the audio ~41 ms later. Ends at the file's last 4 bytes or when the decoder ends, then flushes.
static int readMpeg(VideoDec *vd, ReadBuf *rb, StrFile *file)
{
    int started = 0;
    int demuxRest = file->size;
    int readRest = demuxRest;
    u_char *put, *get;
    u_int trg, t0;
    int n;

    while (demuxRest > AA_DEMUX_TAIL && AA_videoDecGetState(vd) != VD_STATE_END) {
        if (controller_mode == CONTROLLER_MODE_100)
            gaPadRead(pad, 1);
        else
            gaPadRead(pad, 0);
        pad_cnt_button[0] = pad[0].cnt;
        pad_trg_button[0] = pad[0].trg;
        pad_key_button[0] = pad[0].key;
        pad_cnt_button[1] = pad[1].cnt;
        pad_trg_button[1] = pad[1].trg;
        pad_key_button[1] = pad[1].key;
        trg = pad_trg_button[0];
        if (vd->mpeg.frameCount >= AA_SKIP_MIN_FRAMES) {
            if (ssTrigCheckEvent(TRIG_EVENT_MOVIE_END_62, 1, 0) == 0) {
                if (AA_MovieStopOkFlag == AA_MOVIE_SKIP_OK && AA_MovieAbortFlag == 0
                    && ((trg & KEY_MAP[KEY_MAP_MOVIE_STOP]) != 0 || (trg & KEY_MAP[KEY_MAP_MOVIE_SKIP]) != 0)) {
                    AA_MovieAbortFlag = 1;
                    AA_videoDecAbort(&AA_videoDec);
                    ssTrigRegisterEvent(TRIG_EVENT_MOVIE_SKIPPED, 1, 0);
                }
                if (DEMO_SKIP_SET != 0 && AA_MovieAbortFlag == 0 && (trg & KEY_MAP[KEY_MAP_MOVIE_SKIP]) != 0) {
                    AA_MovieAbortFlag = 1;
                    AA_videoDecAbort(&AA_videoDec);
                    ssTrigRegisterEvent(TRIG_EVENT_MOVIE_SKIPPED, 1, 0);
                }
            } else if (DEMO_SKIP_FLAG == 0 && AA_MovieAbortFlag == 0 && (trg & KEY_MAP[KEY_MAP_MOVIE_SKIP]) != 0) {
                AA_MovieAbortFlag = 1;
                AA_videoDecAbort(&AA_videoDec);
                ssTrigRegisterEvent(TRIG_EVENT_MOVIE_SKIPPED, 1, 0);
            }
        }

        n = AA_readBufBeginPut(rb, &put);
        if (readRest > 0 && n >= AA_READ_UNIT) {
            n = AA_strFileRead(file, put, AA_READ_UNIT);
            AA_readBufEndPut(rb, n);
            readRest -= n;
        }
        AA_switchThread();
        n = AA_readBufBeginGet(rb, &get);
        if (n > 0) {
            n = sceMpegDemuxPssRing(&vd->mpeg, get, n, rb->data, rb->size);
            AA_readBufEndGet(rb, n);
            demuxRest -= n;
        }
        AA_proceedAudio();

        if (!started && AA_voBufIsFull(&voBuf) != 0 && isAudioOK() != 0 && AA_WaitCount < 1) {
            AA_startDisplay(1);
            if (AA_isWithAudio != 0) {
                t0 = AA_cpuCycles();
                while (AA_cpuCycles() - t0 < AA_AUDIO_START_DELAY)
                    ;
                AA_audioDecStart(&AA_audioDec);
            }
            started = 1;
        }
    }

    while (AA_videoDecFlush(vd) == 0)
        AA_switchThread();
    while (AA_videoDecIsFlushed(vd) == 0 && AA_videoDecGetState(vd) != VD_STATE_END)
        AA_switchThread();
    if (AA_isWithAudio != 0)
        AA_audioDecReset(&AA_audioDec);
    return 1;
}

// PORT (no ROM counterpart): the movie skip switch (alyssapan/core/movie_skip.h, [game] skip_intro, on in
// smoke runs). AA_movie's ending for a movie skipped before its first frame, with nothing decoded: the
// skip event registered where readMpeg's pad check would allow a skip (a movie the pad cannot skip ends
// as played through), the GS cleared to the fade's colour, the fade handed on to tiFade as the ROM's
// fade-out does, the memory map freed and the banks reloaded. No decoder thread, no 60-frame fade.
static int AA_movieSkipped(const char *name)
{
    int skippable;

    printf("[movie] skip_intro: %s skipped\n", name);
    if (ssTrigCheckEvent(TRIG_EVENT_MOVIE_END_62, 1, 0) == 0)
        skippable = AA_MovieStopOkFlag == AA_MOVIE_SKIP_OK || DEMO_SKIP_SET != 0;
    else
        skippable = DEMO_SKIP_FLAG == 0;
    if (skippable)
        ssTrigRegisterEvent(TRIG_EVENT_MOVIE_SKIPPED, 1, 0);

    if (AA_MvFedeOutCut == 0 && AA_MvFedeOut2Md == AA_MV_FADE2_TI_FADE)
        tiFade_Start(0.0f, AA_MvOutWhite == AA_MV_BLACK ? AA_TI_FADE_FROM_BLACK : AA_TI_FADE_FROM_WHITE);
    if (AA_MvOutWhite == AA_MV_WHITE)
        AA_clearGsMem(0xff, 0xff, 0xff, AA_RESET_WIDTH, AA_RESET_HEIGHT);
    else
        AA_clearGsMem(0, 0, 0, AA_RESET_WIDTH, AA_RESET_HEIGHT);
    AA_MvFedeOutCut = 0;

    AA_MemoryMapFree();
    PlayMovieReturnLoad();
    return controller_val;
}

// CT3 0x018beba0: play `name`. Waits for the file system, takes the decoder's memory (0 when it cannot),
// logs the movie banks, plays the stream; then puts up a tag block of 0xff words (AA_ReTagSet), fades the
// last picture out over 60 frames (unless AA_MvFedeOutCut) to black or white (AA_MvOutWhite), clears
// the GS to that colour and takes everything down. A file that never opened skips the playback and
// resets the system (the error message is built but never shown). Answers controller_val.
static int AA_movie(const char *name)
{
    char message[0x100];
    int row, i, step, level;

    AA_FadeOutEnable = 0;
    AA_DISP_TAG = NULL;
    while (IsFSLoading() != 0 || IsMltLoading() != 0)
        AA_WaitFrame();
    AA_SOUND_MODE = SOUND_STEREO_MONAURAL_BUFF;
    if (AA_MemoryMapSet() != 0)
        return 0;

    if (AlyssaPan_SkipMovies() != 0)
        return AA_movieSkipped(name);

    printf(s_useMovieMemoryBank);
    for (row = 0; row < AA_MOVIE_WORKS; row++) {
        printf(s_bankRange, AA_MovieUseBank[row].bank, AA_MovieUseBank[row].bank + AA_MovieUseBank[row].count - 1,
               AA_MovieUseBank[row].count);
        for (i = 0; i < AA_MovieUseBank[row].count; i++)
            printf(s_bankState, BankInfo[AA_MovieUseBank[row].bank + i].state);
        printf(s_newLine);
    }
    printf(s_newLine);

    initAll(name);
    if (AA_MvInWhite != AA_MV_BLACK)
        AA_FadeInCount = AA_FADE_FRAMES;
    else
        AA_FadeInCount = 0;
    if (AA_mpegWork != 0)
        AA_MoiveMojiFB = AA_mpegWork + AA_MOJI_FB_OFFSET;
    else
        AA_MoiveMojiFB = 0;
    AA_test_RamInit();
    AA_CleanMojiBuffer();
    if (AA_infile.openError == 0)
        readMpeg(&AA_videoDec, &AA_readBuf, &AA_infile);
    AA_endDisplay();

    AA_DISP_TAG = (u_int *)_SunAlloc(&gHeapSound, AA_DISP_TAG_SIZE, AA_DISP_TAG_ALIGN);
    if (AA_DISP_TAG != NULL && AA_infile.openError == 0) {
        for (i = 0; i < AA_DISP_TAG_SIZE / (int)sizeof(u_int); i++)
            AA_DISP_TAG[i] = 0xffffffff;
        FlushCache(0);
        AA_ReTagSet(AA_DISP_TAG);
        FlushCache(0);
        sceGsSyncV(0);
        AA_isCountVblank = 1;
        if (AA_MvFedeOutCut == 0) {
            AA_FadeOutEnable = 1;
            if (AA_MvOutWhite == AA_MV_BLACK) {
                // The fade packet's alpha from 0x80 down to 1 (RGB 0).
                for (i = 0, step = 0; i < AA_FADE_FRAMES; i++, step += 0x80) {
                    level = 0x80 - step / AA_FADE_FRAMES;
                    if (level < 1)
                        level = 1;
                    *AA_FadePackAdr = (u_int)level << 24;
                    FlushCache(0);
                    sceGsSyncV(0);
                }
                if (AA_MvFedeOut2Md == AA_MV_FADE2_TI_FADE)
                    tiFade_Start(0.0f, AA_TI_FADE_FROM_BLACK);
            } else {
                // Grey from 1 up to 0xfb, alpha 0x80.
                for (i = 0, step = 0; i < AA_FADE_FRAMES; i++, step += 0x100) {
                    level = step / AA_FADE_FRAMES;
                    if (level < 1)
                        level = 1;
                    *AA_FadePackAdr = (u_int)level;
                    *AA_FadePackAdr |= (u_int)level << 8;
                    *AA_FadePackAdr |= (u_int)level << 16;
                    *AA_FadePackAdr |= 0x80000000u;
                    FlushCache(0);
                    sceGsSyncV(0);
                }
                if (AA_MvFedeOut2Md == AA_MV_FADE2_TI_FADE)
                    tiFade_Start(0.0f, AA_TI_FADE_FROM_WHITE);
            }
        }
        if (AA_MvOutWhite == AA_MV_WHITE)
            AA_clearGsMem(0xff, 0xff, 0xff, AA_RESET_WIDTH, AA_RESET_HEIGHT);
        else
            AA_clearGsMem(0, 0, 0, AA_RESET_WIDTH, AA_RESET_HEIGHT);
        AA_isCountVblank = 0;
        AA_FadeOutEnable = 0;
        AA_MvFedeOutCut = 0;
    }

    termAll();
    if (AA_DISP_TAG != NULL) {
        _SunFree(&gHeapSound, (void **)&AA_DISP_TAG);
        AA_DISP_TAG = NULL;
    }
    if (AA_infile.openError != 0) {
        SystemReReset();
        sprintf(message, s_fileOpenError, name, s_movieC, AA_FILE_OPEN_ERROR_LINE);
    }
    AA_MemoryMapFree();
    PlayMovieReturnLoad();
    return controller_val;
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
