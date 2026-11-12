// game/movie.h -- CT3 game/movie.c: the movie player's front end (SCE's ezmpegstr main.c grown into the
// game: AA_OpenMovie names the movie and its caption file, PlayMovie -> AA_StartMovie frees 20 banks for
// the decoder and plays it through AA_movie). Slice 1 2026-11-10: the front end, the bank choice and the
// memory map; slice 2 (same day): AA_movie, initAll, readMpeg and termAll, the playback.
#pragma once

#include "sdk/scetypes.h"
#include "sdk/sce_gs.h"
#include "game/AA_SoundMonoSoart.h"
#include "game/AA_mpegTS.h"
#include "game/AA_videoCallback.h"

// ---- names ----------------------------------------------------------------------------------------------
#define TRIG_EVENT_MOVIE_SKIPPED 0xa0   /* readMpeg registers it when the pad skips; AA_StartMovie clears it */

// AA_MvInWhite / AA_MvOutWhite: the colour the movie comes from / goes to.
#define AA_MV_BLACK 0                   /* in: no fade-in (FadeInCount 0); out: fade to black */
#define AA_MV_WHITE 1                   /* clear to white; out: fade to white */
#define AA_MV_FADE 2                    /* in (AA_OpenMovie's default): fade in from black over 60 frames */

#define AA_MV_FADE2_TI_FADE 1           /* AA_MvFedeOut2Md: hand the fade-out on to tiFade_Start */
#define AA_MOVIE_SKIP_OK 1              /* AA_OpenMovie's skip argument: the pad may end the movie */

#define AA_MOVIE_WORKS 6                /* AA_MovieUseBank's rows */
#define AA_BANK_FIRST 6                 /* banks a movie may take: 6..85 (BANK_SCENE_MAX..BANK_MAX) */
#define AA_BANK_FREE 0                  /* AA_AutoBank_Seelect's map: nothing there */
#define AA_BANK_SCENE 1                 /*   a loaded scene bank (type 1, not drawn): can be dropped */
#define AA_BANK_TAKEN 2                 /*   in use (global heap, drawn, loading, other type) or chosen */
#define AA_BANK_MAP_SIZE 0x66           /* its _SunAlloc size (0x56 entries cleared) */
#define AA_BANK_CANDIDATES 0x60         /* the candidate list: 0x480 bytes of 12-byte rows */

// AA_MemoryMapSet's gHeapSound blocks (all 0x40-aligned).
#define AA_READBUF_SIZE 0x60000
#define AA_VIBUFTAG_SIZE 0x1010
#define AA_TIMESTAMP_SIZE 0x3000
#define AA_AUD_TIMESTAMP_SIZE 0x400
#define AA_AUDIOBUFF_SIZE 0xc000
#define AA_THREAD_STACK_SIZE 0x10800
#define AA_HEAP_ALIGN 0x40
#define AA_GS_TAG_STRIDE 0x20000        /* a VoTag's second tag block follows its first */
#define AA_MOVIE_ROW_GS_TAG0 4          /* AA_MovieUseBank's row of AA_voBufTag[0]'s tag blocks (then [1]) */
#define AA_IOP_AUDIO_BUFF 0x6000        /* audioDecInit's IOP buffer */
#define AA_CAPTION_ADDR 0x200000        /* AA_MojiData: the EE address the caption file is read to */
#define AA_RESET_WIDTH 0x200            /* the movie's 512 x 448 frame (sceGsSetDefDBuff, AA_clearGsMem) */
#define AA_RESET_HEIGHT 0x1c0
#define AA_CLUT_CBP 0x3e00              /* AA_GS_SetClut's buffer */
#define AA_START_WAIT 2                 /* AA_WaitCount: vsyncs before the picture may start */
#define AA_THREAD_PRIORITY 3            /* AA_switchThread rotates this ready queue */
#define AA_DISPLAY_DX_BASE 0x27c        /* AA_StartMovie snaps DISPLAY.DX to 0x27c + 5n */
#define AA_DISPLAY_DX_STEP 5
#define AA_DISPLAY_DY_BASE 0x32         /* AA_YOffset = (DY - 0x32) & 1 */

// One movie memory block and the banks it takes (AA_MovieUseBank, sdata 0x01939bf0). AA_StartMovie
// writes BankInfo[bank].image into *work: the decoder works in the banks' memory. layout: host.
typedef struct AA_MOVIE_BANK {
    int bank;                           /* EE 0x00: first bank, -1 = not chosen */
    int count;                          /* EE 0x04: banks (8, 4, 4, 2, 1, 1) */
    u_int *work;                        /* EE 0x08: the global that receives the EE address */
    const char *name;                   /* EE 0x0c: "MPEG decode Work", ... (never read) */
} AA_MOVIE_BANK;

// ---- data ------------------------------------------------------------------------------------------------
extern int AA_isWithAudio;                                  /* sdata 0x01939be0: 1 */
extern AA_MOVIE_BANK AA_MovieUseBank[AA_MOVIE_WORKS];       /* sdata 0x01939bf0 */
extern int AA_SOUND_MODE;                                   /* sbss 0x0193f0d8 */
extern int AA_MvInWhite;                                    /* sbss 0x0193f0dc: AA_MV_* */
extern int AA_MvOutWhite;                                   /* sbss 0x0193f0e0: AA_MV_* */
extern int AA_MvFedeOutCut;                                 /* sbss 0x0193f0e4: != 0: no fade-out */
extern sceGsDBuff db;                                       /* sbss 0x0193f0f0 */
extern int AA_YOffset;                                      /* sbss 0x0193f320: DY's parity */
extern int AA_FadeInCount;                                  /* sbss 0x0193f324 */
extern int AA_MvFedeOut2Md;                                 /* sbss 0x0193f328: AA_MV_FADE2_* */
extern int AA_WaitCount;                                    /* sbss 0x0193f32c */
extern u_int AA_MojiData;                                   /* sbss 0x0193f330: EE address of the captions, 0 = none */
extern int AA_Moive_Skip;                                   /* sbss 0x0193f334 */
extern int controller_val;                                  /* sbss 0x0193f338: AA_movie's answer */
extern int AA_frd;                                          /* sbss 0x0193f33c */
extern VoBuf voBuf;                                         /* sbss 0x0193f340 */
extern AudioDec AA_audioDec;                                /* sbss 0x0193f390 */
extern VideoDec AA_videoDec;                                /* sbss 0x0193f410 */
extern StrFile AA_infile;                                   /* sbss 0x0193f4f0 */
extern int AA_defaultTh;                                    /* sbss 0x0193f528 */
extern int AA_videoDecTh;                                   /* sbss 0x0193f52c */
extern int AA_CdDvd_trial_Ct;                               /* sbss 0x0193f530 */
extern int AA_FadeOutEnable;                                /* sbss 0x0193f534 */
extern u_int *AA_FadePackAdr;                               /* sbss 0x0193f538 */
extern u_int *AA_DISP_TAG;                                  /* sbss 0x0193f544 */
extern int AA_MovieRedyFlag;                                /* sbss 0x0193f548 */
extern u_int AA_viBufData;                                  /* sbss 0x0193f6c0: EE address (a movie bank) */
extern u_int AA_MoiveMojiFB;                                /* sbss 0x0193f6c4 */
extern u_int AA_mpegWork;                                   /* sbss 0x0193f6c8: EE address (8 movie banks) */
extern VoTag AA_voBufTag[AA_VOBUF_COUNT];                   /* sbss 0x0193f6d0 */
extern u_int AA_voBufData[AA_VOBUF_COUNT];                  /* sbss 0x0193f700: EE addresses (4 banks each) */

// ---- functions -------------------------------------------------------------------------------------------
void AA_AutoBank_Seelect(void);                                                          // CT3 0x018bda90
int AA_proceedAudio(void);                                                               // CT3 0x018be510
int ErrMessage(const char *message);                                                     // CT3 0x018be520
void AA_switchThread(void);                                                              // CT3 0x018be7e0
void AA_ToSmall(char *s);                                                                // CT3 0x018bf030
void AA_StartMovie(void);                                                                // CT3 0x018bf080
int AA_PlayMovieReady(void);                                                             // CT3 0x018bf4f0
int AA_OpenMovie(const char *movie, const char *caption, u_int skip);                    // CT3 0x018bf510
void AA_MovieProc(void);                                                                 // CT3 0x018bf6c0
void AA_MemoryMapFree(void);                                                             // CT3 0x018bf6f0
int AA_MemoryMapSet(void);                                                               // CT3 0x018bf7d0
void AA_MovieIOPInit(void);                                                              // CT3 0x018bf980
