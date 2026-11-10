// game/fileio.h -- CT3 game/fileio.c (CT3 0x018c2170..0x018c7fb0): the bank loader. Files are asked for
// by name into a queue of 20 requests (FQue); FSMltLoadProc, once a frame through FileSoundServer, takes
// the head, reads the file through the file layer (ArtoolDebugOpen.c's Fl*) and places it in a bank of
// BankInfo[86] (ct3_types.h BANK_INFO): banks 0..5 are the global banks, allocated from the GLBL heap;
// 6..85 are fixed 256 KB slots, a file larger than one taking a group of consecutive slots.
// Slice 1 (2026-11-10): the queue and the bank table (init, find, status, wait, cancel, draw on / off,
// scene end, the file server). Prototypes of functions not yet written come from their call sites.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// ---- the queue ----------------------------------------------------------------------------------------
#define FQUE_MAX 20                     /* FQue[]: 1280 bytes in the symtab, the searches stop at 0x14 */

// FQue's rows: one asked-for file. Row 0 is the one FSMltLoadProc works on; FSMltLoad fills the first
// empty row (a global bank's request goes before the first non-global one: FSMltInsertQuePriority).
// layout: ee (the rows move as whole 0x40-byte records).
typedef struct FQUE {
    char dir[0x10];                     /* 0x00: FSFileType's directory (FSMltLoadProc fills it) */
    char name[0x20];                    /* 0x10: the file ("A000.ANM"); "" = an empty row */
    int kind;                           /* 0x30: FS_LOAD_* (main2.h) = the bank's BANK_INFO.type */
    int bank;                           /* 0x34: the first bank asked for */
    int count;                          /* 0x38: banks asked for (1 for a global bank) */
    int handle;                         /* 0x3c: FSMltLoad's answer (gHandleCounter, never 0) */
} FQUE;
static_assert(sizeof(FQUE) == 0x40, "FQUE");
static_assert(offsetof(FQUE, name) == 0x10, "FQUE.name");
static_assert(offsetof(FQUE, kind) == 0x30, "FQUE.kind");
static_assert(offsetof(FQUE, bank) == 0x34, "FQUE.bank");
static_assert(offsetof(FQUE, handle) == 0x3c, "FQUE.handle");

// FSMltLoadProc's steps (Phase), the ROM's numbers.
enum FSMLT_PHASE {
    FSMLT_IDLE = 0,                     /* row 0 asked for: already in its bank -> DONE, else START */
    FSMLT_START = 1,                    /* free and reserve the banks, the path */
    FSMLT_OPEN = 2,                     /* FlReqOpen, retried every frame */
    FSMLT_READ_HEAD = 3,                /* sector 0 into Buf1stSector */
    FSMLT_HEAD_WAIT = 4,                /* bank / count from the file, the image, the sector count */
    FSMLT_READ_REST = 5,                /* a plain file's other sectors straight into the image */
    FSMLT_REST_WAIT = 6,
    FSMLT_READ_PACKED = 7,              /* a packed file: up to 60 sectors into ExtBuf */
    FSMLT_PACKED_WAIT = 8,              /* Extract them; more -> READ_PACKED */
    FSMLT_CHECK = 10,                   /* byte sum, relocation, set-up; then the rows move up */
    FSMLT_CHECKED = 11,
    FSMLT_DONE = 12,                    /* the rows move up */
    FSMLT_ERROR = 0x14,                 /* a read failed: close, free the banks, START again */
};

// ---- the whole-file queue (FSLoad) --------------------------------------------------------------------
#define FSLOAD_MAX 6                    /* sPreLoad[]: 336 bytes = 6 rows of 0x38 */

// sPreLoad's rows: a whole file into the caller's buffer. Row 0 is the one FSLoadProc works on.
// layout: host -- buff is a host pointer (EE +0x34 was an EE address); only fileio.c reads the rows.
typedef struct FSLOAD_REQ {
    int handle;                         /* EE 0x00: FSLoad's answer; 0 = an empty row */
    char dir[0x10];                     /* EE 0x04: FlReqOpen's directory */
    char name[0x20];                    /* EE 0x14: the file */
    u_char *buff;                       /* EE 0x34: where the file goes */
} FSLOAD_REQ;

// FSLoadProc's steps (sPreLoadPhase).
enum FSLOAD_PHASE {
    FSLOAD_IDLE = 0,                    /* row 0 asked for: OPEN */
    FSLOAD_OPEN = 1,                    /* size it (FlGetLen) and open it */
    FSLOAD_OPEN_WAIT = 2,
    FSLOAD_READ = 3,                    /* all but the last sector into buff, then the last one */
    FSLOAD_READ_LAST = 4,               /* the last sector into BufLastSector, its bytes copied to buff */
    FSLOAD_READ_WAIT = 5,
    FSLOAD_CLOSE = 10,
    FSLOAD_NEXT = 11,                   /* the rows move up */
    FSLOAD_ERROR = 0x14,                /* close what is open, then NEXT: the file is dropped */
};

// ---- the banks ----------------------------------------------------------------------------------------
// BANK_INFO.state: a group's head holds BANK_STATE_LOADED, its other banks their place in the group
// (2..groupCount: SceneEndBank walks back to the head with it).
#define BANK_STATE_FREE 0
#define BANK_STATE_LOADED 1
#define BANK_STATE_LOADING (-1)         /* FSMltLoadStatus answers FS_STATUS_LOADING; a cancel of row 0 frees it */

#define BANK_GLOBAL_MAX 6               /* banks 0..5: a global bank (FS_LOAD_GLBL) is only one of these */
#define BANK_SLOT_SIZE 0x40000          /* banks 6..85: 256 KB each, from gGlobalBankArea + 0x10000 */
#define BANK_SLOT_OFFSET 0x10000
#define BANK_SLOT_COUNT 80              /* BANK_MAX - BANK_GLOBAL_MAX */
#define GLOBAL_BANK_AREA_EE 0x00400000u /* gGlobalBankArea's value in the ROM (sdata 0x01939c54) */

#define BANK_FLAG_SCENE_INIT 0x1        /* SceneEndBank: the bank's scene was set up: scene_end it */

// FSMltLoadStatus's answers.
#define FS_STATUS_NONE 0                /* neither in a bank nor asked for (or in the bank still loading) */
#define FS_STATUS_LOADED 1
#define FS_STATUS_LOADING 2             /* the bank is BANK_STATE_LOADING */
#define FS_STATUS_QUEUED 3              /* waiting in FQue */

// FSMltLoadEnd's answers besides the bank.
#define FS_END_PENDING (-1)             /* still in the queue */
#define FS_END_UNKNOWN (-2)             /* no bank holds the handle */

// ---- globals ------------------------------------------------------------------------------------------
extern FQUE FQue[FQUE_MAX];                      /* bss  0x01c833c0 */
extern int Phase;                                /* sbss 0x01942b4c: FSMltLoadProc's step */
extern int sErrCount;                            /* sbss 0x0193f774 */
// PORT: the slots' host memory (BankImage[6..85] point into it; the ROM's slots are EE RAM).
extern u_char *gBankSlotArea;

// ---- functions ----------------------------------------------------------------------------------------
void CharacterSort(void);                                                                // CT3 0x018c2170
void objSortWithBlurIdx(int scene, int count, void *table);                              // CT3 0x018c23b0
void objSort(int scene, int count, void *table);                                         // CT3 0x018c2880
void Round2ChangeChar(int round);                                                        // CT3 0x018c2df0
void DebugBankDisp(void);                                                                // CT3 0x018c3790
void FSLoadProc(void);                                                                   // CT3 0x018c37a0
void FSLoadCancelQue(int handle);                                                        // CT3 0x018c3a80
int FSLoadEndWait(int handle);                                                           // CT3 0x018c3b10
int FSLoadEnd(int handle);                                                               // CT3 0x018c3b90
int FSLoad(char *dir, char *name, void *buff);                                           // CT3 0x018c3be0
int IsFSLoading(void);                                                                   // CT3 0x018c3ce0
int FSMltLoadEndWaitFile(int bank, char *name);                                          // CT3 0x018c3cf0
int FSMltLoadEnd(int handle);                                                            // CT3 0x018c3f30
int FSMltLoadStatus(int bank, char *name);                                               // CT3 0x018c3fd0
void FSMltSetDrawBank(int bank, char *name, int mode);                                   // CT3 0x018c40d0
int FSMltFindBankHandle(int handle);                                                     // CT3 0x018c4450
int FSMltFindBank(char *name);                                                           // CT3 0x018c44c0
void FSMltLoadCancelIdxQue(int idx);                                                     // CT3 0x018c4560
void FSMltLoadCancelQue(char *name);                                                     // CT3 0x018c4810
int IsQueWaiting(int bank, char *name);                                                  // CT3 0x018c48a0
int FsIsQueExisting(void);                                                               // CT3 0x018c4950
void SceneEndBank(int first, int count);                                                 // CT3 0x018c5780
int FSMltLoad(char *name, int kind, int bank, int count);                                // CT3 0x018c5cd0
int FSMltInsertQuePriority(FQUE *req);                                                   // CT3 0x018c5f10
void FSMltLoadCancelAll(void);                                                           // CT3 0x018c6070
void FSMltLoadEndWaitAll(void);                                                          // CT3 0x018c60a0
void FSMltInit(void);                                                                    // CT3 0x018c6100
void FSMltLoadProc(void);                                                                // CT3 0x018c6290
int Extract(const u_char *hdr, const u_char *src, const u_char *end, u_char **out);      // CT3 0x018c74b0
char *FSFileType(int kind, char *dir);                                                   // CT3 0x018c7cb0
int IsMltLoading(void);                                                                  // CT3 0x018c7d70
void FileSoundServer_DontMltLoad(void);                                                  // CT3 0x018c7d80
void FileSoundServer(void);                                                              // CT3 0x018c7dc0
void FS_work_init(void);                                                                 // CT3 0x018c7e00
void FSInit(void);                                                                       // CT3 0x018c7eb0
char *fs_error(const char *file, int line, const char *title, const char *fmt, ...);     // CT3 0x018c7ec0
