// game/fileio.c -- Clock Tower 3: the bank loader (CT3 0x018c2170..0x018c7fb0). A file is asked for by
// name (FSMltLoad) into FQue; FSMltLoadProc, once a frame from FileSoundServer, loads the queue's head into
// its bank of BankInfo; the game waits for it (FSMltLoadEndWaitFile / FSMltLoadEnd), turns its drawing on
// and off (FSMltSetDrawBank) and frees it at a scene's end (SceneEndBank). FSLoad* is a second, simpler
// queue for whole files into the caller's buffer. The source string "fileio.c" is fs_error's file argument.
// Slice 1 reconstructed 2026-11-10 (27 functions): the queue (FSMltLoad, its priority insert, the cancels,
// the status and wait functions), the bank table (FSMltInit, the finds, draw on / off, SceneEndBank), the
// file server, FSFileType and fs_error. Left: the loaders (FSMltLoadProc, Extract, FSLoad*, ReserveBank,
// SceneEndBankMovie), the object sorts and the second-round character swap (Round2Change*).

#include "game/fileio.h"

#include "common/alyssapan_stub.h"
#include "game/ArtoolDebugOpen.h"
#include "game/Clipping_Portal.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/ahalloc.h"
#include "game/ct3_types.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/sound.h"
#include "game/unassigned_data.h"
#include "ga/gaInitInterrupt.h"
#include "sdk/libgraph.h"
#include "sdk/newlib.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// ---- data ---------------------------------------------------------------------------------------------

static const char s_srcFile[] = "fileio.c";     /* data 0x01936098: fs_error's file argument */

// fs_error's line arguments (decimal, as the ROM passes them).
#define FS_LINE_TYPE_ERROR 502                  /* FSFileType */
#define FS_LINE_WAIT_NAME 2021                  /* FSMltLoadEndWaitFile, bank < 0 */
#define FS_LINE_WAIT_BANK 2054                  /* FSMltLoadEndWaitFile, bank >= 0 */
#define FS_LINE_DRAWON 1884                     /* FSMltSetDrawBank */
#define FS_LINE_GLOBAL_LIMIT 1284               /* FSMltLoad */
#define FS_LINE_QUE_MAX 1387                    /* FSMltLoad */
#define FS_LINE_FSLOAD_QUE_MAX 2368             /* FSLoad */
#define FS_LINE_FSLOAD_SIZE 2495                /* FSLoadProc */
#define FS_LINE_FSLOAD_OPEN 2509                /* FSLoadProc */
#define FS_LINE_FSLOAD_READ 2537                /* FSLoadProc */

FQUE FQue[FQUE_MAX];                            /* bss  0x01c833c0 */
int Phase;                                      /* sbss 0x01942b4c: FSMltLoadProc's step */
static int FH;                                  /* sbss 0x01942b50: FSMltLoadProc's open file (an Fl handle) */
static int sPreRsects;                          /* sbss 0x01942b5c sPreRsects$914: FSLoadProc's sectors read */
static int sPreLoadNowSects;                    /* sbss 0x01942b60: sectors already in buff */
static int sPreLoadSects;                       /* sbss 0x01942b64: the file's sectors */
static int sPreLoadSize;                        /* sbss 0x01942b68: the file's bytes */
static int sPreLoadFileHandle;                  /* sbss 0x01942b6c: the Fl handle */
static int sPreLoadPhase;                       /* sbss 0x01942b70: FSLoadProc's step (FSLOAD_*) */
static int sPreLoadHandleCounter;               /* sbss 0x01942b74 */
static FSLOAD_REQ sPreLoad[FSLOAD_MAX];         /* sbss 0x01942b80 */
alignas(64) static u_char BufLastSector[0x800]; /* bss  0x01c643c0: the last sector, read whole */
static char OpenFile[56];                       /* sbss 0x01942d10: the name FSMltLoadProc has open */
int sErrCount;                                  /* sbss 0x0193f774 */

// PORT: the slots' memory. The ROM's banks 6..85 are EE RAM from gGlobalBankArea + 0x10000 (EE 0x00410000)
// up to the ELF (0x01810000); the host gives them one buffer and records each slot's host address and EE
// address in BankImage[] / BankImageEE[] (ct3_types.h BankEePtr, open-items.md 1 h).
alignas(64) static u_char gBankSlotArea_host[BANK_SLOT_COUNT * BANK_SLOT_SIZE];
u_char *gBankSlotArea = gBankSlotArea_host;

// ---- the file server ----------------------------------------------------------------------------------

// CT3 0x018c7dc0 -- once a frame (main's loop, the movie player, every wait): the file layer, the two
// loaders, the sound driver and the game's sound layer.
void FileSoundServer(void)
{
    FlProc();
    FSMltLoadProc();
    FSLoadProc();
    SdProc();
    SSProc();
}

// CT3 0x018c7d80 -- FileSoundServer without the bank loader (FSLoadEndWait's loop).
void FileSoundServer_DontMltLoad(void)
{
    FlProc();
    FSLoadProc();
    SdProc();
    SSProc();
}

// CT3 0x018c3790 -- empty in the retail ROM (a debug dump of BankInfo, compiled out).
void DebugBankDisp(void)
{
}

// CT3 0x018c7ec0 -- prints "<title> / <message>" and answers "<title>\n\n\n<message>\n\n\n<file>  line:<n>"
// for an error screen. The ROM answers its own stack buffer (dead once it returns; no caller in the ROM
// uses the answer as far as read); PORT: a static buffer, and the formats are bounded.
char *fs_error(const char *file, int line, const char *title, const char *fmt, ...)
{
    static char out[560];
    char msg[512];
    va_list ap;

    // callcheck: sprintf, strcat, strcpy, vsprintf (PORT: bounded vsnprintf / snprintf / strncat)
    DebugBankDisp();
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    printf("%s / %s\n", title, msg);                    /* data 0x019363d8 @2542 */
    snprintf(out, sizeof(out), "%s\n\n\n%s", title, msg); /* @2543 "\n\n\n" */
    snprintf(msg, sizeof(msg), "\n\n\n%s  line:%d", file, line); /* @2544 */
    return strncat(out, msg, sizeof(out) - strlen(out) - 1);
}

// ---- the whole-file queue -----------------------------------------------------------------------------

// CT3 0x018c3be0 -- asks for dir/name whole into buff. Answers the handle FSLoadEnd takes, -1 when full.
int FSLoad(char *dir, char *name, void *buff)
{
    int i;

    for (i = 0; i < FSLOAD_MAX; i++) {
        if (sPreLoad[i].handle == 0) {
            sPreLoad[i].handle = ++sPreLoadHandleCounter;
            sPreLoad[i].buff = (u_char *)buff;
            strcpy(sPreLoad[i].dir, dir);
            strcpy(sPreLoad[i].name, name);
            return sPreLoad[i].handle;
        }
    }
    fs_error(s_srcFile, FS_LINE_FSLOAD_QUE_MAX, "FSLoad QUE MAX error", "%s", name); /* data 0x01936110 */
    return -1;
}

// CT3 0x018c3b90 -- 1 once the request is done (or dropped), 0 while queued, -1 for a bad handle.
int FSLoadEnd(int handle)
{
    int i;

    if (handle < 0)
        return -1;
    for (i = 0; i < FSLOAD_MAX; i++) {
        if (handle == sPreLoad[i].handle)
            return 0;
    }
    return 1;
}

// CT3 0x018c3b10 -- waits for an FSLoad request without loading banks meanwhile (FileSoundServer_
// DontMltLoad); the wait does not count as game time. 0, or -1 for a bad handle.
int FSLoadEndWait(int handle)
{
    int timer = ga_main_timer;
    int ret;

    if (handle < 0) {
        ret = -1;
    } else {
        while (FSLoadEnd(handle) == 0) {
            FileSoundServer_DontMltLoad();
            sceGsSyncV(0);
        }
        ret = 0;
    }
    ga_main_timer = timer;
    return ret;
}

// CT3 0x018c3a80 -- drops a waiting request (rows 1..: never the file in flight). The ROM moves
// (idx + 1) rows up from idx + 1 instead of the 5 - idx behind it: for idx 1 it moves rows that are not
// there (empty), for idx >= 3 it copies past sPreLoad's end into the sbss behind it. PORT: only the rows
// behind move (hazards.md "AlyssaPan", 2026-11-10).
void FSLoadCancelQue(int handle)
{
    int i;

    for (i = 1; i < FSLOAD_MAX; i++) {
        if (handle == sPreLoad[i].handle) {
            // callcheck: memcpy (the rows overlap: memmove on the host)
            memmove(&sPreLoad[i], &sPreLoad[i + 1], (FSLOAD_MAX - 1 - i) * sizeof(FSLOAD_REQ));
            sPreLoad[FSLOAD_MAX - 1].handle = 0;
            return;
        }
    }
}

// Asks for the rest of the file: all but its last sector straight into buff (READ_WAIT), the last one
// into BufLastSector (READ_LAST). Answers FlReqRead's answer (non-zero: refused).
static int fs_preload_read(void)
{
    int ret;

    sPreRsects = sPreLoadSects - sPreLoadNowSects;
    if (sPreRsects == 1) {
        ret = FlReqRead(sPreLoadFileHandle, BufLastSector, -1, 1);
        sPreLoadPhase = FSLOAD_READ_LAST;
    } else {
        sPreRsects--;
        ret = FlReqRead(sPreLoadFileHandle, sPreLoad[0].buff + sPreLoadNowSects * FL_SECTOR_SIZE, -1, sPreRsects);
        sPreLoadPhase = FSLOAD_READ_WAIT;
    }
    return ret;
}

// Drops row 0 and moves the rest up (NEXT).
static void fs_preload_next(void)
{
    memmove(&sPreLoad[0], &sPreLoad[1], (FSLOAD_MAX - 1) * sizeof(FSLOAD_REQ)); /* ROM: memcpy, 0x118 */
    sPreLoad[FSLOAD_MAX - 1].handle = 0;
    sPreLoadPhase = FSLOAD_IDLE;
}

// CT3 0x018c37a0 -- once a frame: works on row 0 (FSLOAD_PHASE). The last sector is read apart so a
// file whose size is not whole sectors does not overrun buff. A step that just asked for a read goes
// straight on to READ_LAST's wait, as the ROM falls through to it.
void FSLoadProc(void)
{
    // callcheck: FlReqRead, memcpy (fs_preload_read asks for the reads; fs_preload_next moves the rows up with memmove)
    switch (sPreLoadPhase) {
    case FSLOAD_ERROR:
        if (sPreLoadFileHandle != 0) {
            FlReqClose(sPreLoadFileHandle);
            sPreLoadFileHandle = 0;
        }
        sPreLoadPhase = FSLOAD_NEXT;
        return;
    case FSLOAD_NEXT:
        fs_preload_next();
        return;
    case FSLOAD_CLOSE:
        break;
    case FSLOAD_READ_WAIT:
        if (FlIsExecute(sPreLoadFileHandle) != 0)
            return;
        sPreLoadPhase = FSLOAD_READ;
        sPreLoadNowSects = sPreRsects;
        goto read;
    case FSLOAD_READ_LAST:
        goto read_last;
    case FSLOAD_READ:
        goto read;
    case FSLOAD_OPEN_WAIT:
        if (FlIsExecute(sPreLoadFileHandle) != 0)
            return;
        sPreLoadPhase++;
        goto read;
    case FSLOAD_IDLE:
        if (sPreLoad[0].handle == 0)
            return;
        sPreLoadPhase++;
        /* fall through */
    case FSLOAD_OPEN:
        sPreLoadSize = FlGetLen(sPreLoad[0].dir, sPreLoad[0].name, 1);
        if (sPreLoadSize < 0) {
            fs_error(s_srcFile, FS_LINE_FSLOAD_SIZE, "file error!!", "%s", sPreLoad[0].name); /* data 0x019360c8 */
            sPreLoadPhase = FSLOAD_ERROR;
            return;
        }
        sPreLoadSects = sPreLoadSize / FL_SECTOR_SIZE + (sPreLoadSize % FL_SECTOR_SIZE != 0);
        sPreLoadNowSects = 0;
        sPreLoadFileHandle = FlReqOpen(sPreLoad[0].dir, sPreLoad[0].name);
        if (sPreLoadFileHandle == 0) {
            fs_error(s_srcFile, FS_LINE_FSLOAD_OPEN, "open error!!", "[%s] [%s]",  /* data 0x019360e0, 0x019360f0 */
                     sPreLoad[0].dir, sPreLoad[0].name);
            sPreLoadPhase = FSLOAD_ERROR;
            return;
        }
        sPreLoadPhase = FSLOAD_OPEN_WAIT;
        return;
    default:
        return;
    }

    // FSLOAD_CLOSE
    FlReqClose(sPreLoadFileHandle);
    sPreLoadFileHandle = 0;
    sPreLoadPhase = FSLOAD_NEXT;
    fs_preload_next();
    return;

read:
    if (fs_preload_read() != 0) {
        fs_error(s_srcFile, FS_LINE_FSLOAD_READ, "read error!!", "%s", sPreLoad[0].name); /* data 0x01936100 */
        sPreLoadPhase = FSLOAD_ERROR;
        return;
    }
read_last:
    if (FlIsExecute(sPreLoadFileHandle) != 0)
        return;
    memcpy(sPreLoad[0].buff + sPreLoadNowSects * FL_SECTOR_SIZE, BufLastSector,
           sPreLoadSize - sPreLoadNowSects * FL_SECTOR_SIZE);
    sPreLoadPhase = FSLOAD_CLOSE;
    FlReqClose(sPreLoadFileHandle);
    sPreLoadFileHandle = 0;
    sPreLoadPhase = FSLOAD_NEXT;
    fs_preload_next();
}

// ---- the bank table -----------------------------------------------------------------------------------

// CT3 0x018c6100 -- empties BankInfo and the queue; banks 0..5 have no memory until loaded (the GLBL
// heap), banks 6..85 get their fixed slots. Closes FSMltLoadProc's open file.
void FSMltInit(void)
{
    int i;

    memset(BankInfo, 0, sizeof(BankInfo));
    for (i = 0; i < BANK_GLOBAL_MAX; i++) {
        BankInfo[i].state = BANK_STATE_FREE;
        BankInfo[i].image = 0;
        BankImage[i] = NULL;                            /* PORT */
        BankImageEE[i] = 0;
    }
    for (i = BANK_GLOBAL_MAX; i < BANK_MAX; i++) {
        int slot = (i - BANK_GLOBAL_MAX) * BANK_SLOT_SIZE;

        BankInfo[i].state = BANK_STATE_FREE;
        BankInfo[i].image = GLOBAL_BANK_AREA_EE + BANK_SLOT_OFFSET + slot;
        BankImage[i] = gBankSlotArea + slot;            /* PORT */
        BankImageEE[i] = BankInfo[i].image;
    }
    memset(FQue, 0, sizeof(FQue));
    Phase = 0;
    if (FH != 0)
        FlReqClose(FH);
    FH = 0;
}

// CT3 0x018c7eb0 -- IOPInit's call: a jump to FSMltInit.
void FSInit(void)
{
    FSMltInit();
}

// CT3 0x018c7e00 -- a new game or a load: drops the queue (waiting for the file in flight), turns every
// bank's drawing off and empties the table.
void FS_work_init(void)
{
    int i;

    FSMltLoadCancelAll();
    sErrCount = 0;
    for (i = 0; i < BANK_MAX; i++)
        BankInfo[i].drawOn = 0;
    FSMltInit();
}

// CT3 0x018c44c0 -- the loaded bank (a group's head) holding the file, or -1.
int FSMltFindBank(char *name)
{
    int i;

    for (i = 0; i < BANK_MAX; i++) {
        if (BankInfo[i].state == BANK_STATE_LOADED) {
            if (strcmp(BankInfo[i].name, name) == 0)
                return i;
            i += BankInfo[i].groupCount - 1;
        }
    }
    return -1;
}

// CT3 0x018c4450 -- the loaded bank FSMltLoad's handle filled, or -1.
int FSMltFindBankHandle(int handle)
{
    int i;

    for (i = 0; i < BANK_MAX; i++) {
        if (BankInfo[i].state == BANK_STATE_LOADED) {
            if (BankInfo[i].handle == handle)
                return i;
            i += BankInfo[i].groupCount - 1;
        }
    }
    return -1;
}

// Sets one group's drawOn (FSMltSetDrawBank's two paths).
static void fs_set_group_draw(int bank, int mode)
{
    int n = BankInfo[bank].groupCount;
    int i;

    for (i = 0; i < n; i++)
        BankInfo[bank + i].drawOn = mode;
}

// CT3 0x018c40d0 -- turns a loaded file's drawing on or off (DRAW_BANK_*), its whole group. The bank is
// a hint: when it does not hold the file, the file is looked for in every bank; not found is an error
// only when turning on.
void FSMltSetDrawBank(int bank, char *name, int mode)
{
    if (bank >= 0 && BankInfo[bank].state == BANK_STATE_LOADED && strcmp(name, BankInfo[bank].name) == 0) {
        fs_set_group_draw(bank, mode);
        return;
    }
    bank = FSMltFindBank(name);
    if (bank < 0) {
        if (mode != 0)
            fs_error(s_srcFile, FS_LINE_DRAWON, "SET BANK DRAWON ERROR!!",      /* data 0x019361c0 */
                     "DRAWONしようとしたけど\n%sがBANKにそんざいしない", name);   /* data 0x019361e0 */
        return;
    }
    fs_set_group_draw(bank, mode);
}

// Empties one group's rows (SceneEndBank).
static void fs_clear_group(int bank, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        BANK_INFO *b = &BankInfo[bank + i];

        b->drawOn = 0;
        b->groupCount = 0;
        b->state = BANK_STATE_FREE;
        b->flags = 0;
        b->name[0] = '\0';
    }
}

// CT3 0x018c5780 -- frees the loaded banks in [first, first + count): a bank inside a group widens the
// range back to its head. A set-up scene (BANK_FLAG_SCENE_INIT) is ended first; a global bank's memory
// goes back to the GLBL heap; the slots stay where they are.
void SceneEndBank(int first, int count)
{
    int i;

    if (first < 0 || count <= 0)
        return;
    if (BankInfo[first].state > BANK_STATE_LOADED) {
        count += BankInfo[first].state - 1;
        first = first - BankInfo[first].state + 1;
    }
    if (first + count > BANK_MAX)
        count = BANK_MAX - first;
    for (i = first; i < first + count; i++) {
        BANK_INFO *b = &BankInfo[i];
        int n;

        if (b->state != BANK_STATE_LOADED)
            continue;
        n = b->groupCount;
        if (b->type == BANK_TYPE_SHADOW || b->type == BANK_TYPE_2) {    /* FS_LOAD_SCENE / FS_LOAD_SHAPE */
            if (b->flags & BANK_FLAG_SCENE_INIT)
                scene_end(i);
            fs_clear_group(i, n);
        } else if (b->type == BANK_TYPE_0) {                          /* FS_LOAD_GLBL */
            if (b->flags & BANK_FLAG_SCENE_INIT)
                scene_end(i);
            if (b->image != 0) {
                // The ROM frees the EE address in BankInfo +0x1c; the host frees its image.
                _SunFree(&gHeapGlobalScene, (void **)&BankImage[i]);
                b->image = 0;
                BankImageEE[i] = 0;
            }
            b->drawOn = 0;
            b->groupCount = 0;
            b->state = BANK_STATE_FREE;
            b->flags = 0;
            b->name[0] = '\0';
        } else if (b->type == BANK_TYPE_NO_ROOM_SOUND) {              /* FS_LOAD_BEHAVIOR: no scene_end */
            fs_clear_group(i, n);
        }
        i += n - 1;
    }
}

// ---- the queue ----------------------------------------------------------------------------------------

// CT3 0x018c4950 -- any row asked for.
int FsIsQueExisting(void)
{
    int i;

    for (i = 0; i < FQUE_MAX; i++) {
        if (FQue[i].name[0] != '\0')
            return 1;
    }
    return 0;
}

// CT3 0x018c7d70 -- a jump to FsIsQueExisting.
int IsMltLoading(void)
{
    return FsIsQueExisting();
}

// CT3 0x018c3ce0 -- the whole-file queue (FSLoadProc) is busy.
int IsFSLoading(void)
{
    return sPreLoadPhase != 0;
}

// CT3 0x018c48a0 -- the file is queued: 1 for the bank, 2 for another bank, 0 not queued (the search
// stops at the first empty row).
int IsQueWaiting(int bank, char *name)
{
    int i;

    for (i = 0; i < FQUE_MAX; i++) {
        if (FQue[i].name[0] == '\0')
            return 0;
        if (strcmp(FQue[i].name, name) == 0)
            return bank == FQue[i].bank ? 1 : 2;
    }
    return 0;
}

// CT3 0x018c3fd0 -- FS_STATUS_*. With a bank, the bank must hold the file (a group head) or the file be
// queued for that bank; without one (bank < 0), anywhere. A bank of the file's name that is neither
// loaded nor loading answers FS_STATUS_NONE without looking at the queue.
int FSMltLoadStatus(int bank, char *name)
{
    if (bank < 0) {
        if (FSMltFindBank(name) >= 0)
            return FS_STATUS_LOADED;
        if (IsQueWaiting(-1, name) != 0)        /* the ROM passes FSMltFindBank's -1 */
            return FS_STATUS_QUEUED;
        return FS_STATUS_NONE;
    }
    if (BankInfo[bank].state != BANK_STATE_FREE && strcmp(name, BankInfo[bank].name) == 0) {
        if (BankInfo[bank].state == BANK_STATE_LOADED)
            return FS_STATUS_LOADED;
        if (BankInfo[bank].state == BANK_STATE_LOADING)
            return FS_STATUS_LOADING;
        return FS_STATUS_NONE;
    }
    if (IsQueWaiting(bank, name) == 1)
        return FS_STATUS_QUEUED;
    return FS_STATUS_NONE;
}

// CT3 0x018c3f30 -- the bank FSMltLoad's handle loaded into, FS_END_PENDING while it is queued, or
// FS_END_UNKNOWN. Handle 0 skips the queue.
int FSMltLoadEnd(int handle)
{
    int bank;
    int i;

    if (handle != 0) {
        for (i = 0; i < FQUE_MAX; i++) {
            if (FQue[i].name[0] != '\0' && FQue[i].handle == handle)
                return FS_END_PENDING;
        }
    }
    bank = FSMltFindBankHandle(handle);
    return bank == -1 ? FS_END_UNKNOWN : bank;
}

// One wait step: serve the files, wait a field, keep the vibration's timer going.
static void fs_wait_frame(void)
{
    FileSoundServer();
    sceGsSyncV(0);
    if (fVibTimer)
        tiVibrationTimer();
}

// CT3 0x018c3cf0 -- waits for a queued file and answers its bank. Without a bank (< 0) the file may land
// anywhere; with one, the bank must end up holding it (or, name NULL, any file the bank's queue row
// brings). The wait does not count as game time: ga_main_timer is put back (not on the error paths).
// Neither loaded nor queued is an error (0); bank >= 0 with no name and no row answers -1.
int FSMltLoadEndWaitFile(int bank, char *name)
{
    // callcheck: FileSoundServer, sceGsSyncV, tiVibrationTimer (the two wait loops: fs_wait_frame)
    int timer = ga_main_timer;
    int found;
    int i;

    if (bank < 0) {
        while ((found = FSMltFindBank(name)) < 0) {
            for (i = 0; i < FQUE_MAX; i++) {
                if (FQue[i].name[0] != '\0' && strcmp(FQue[i].name, name) == 0)
                    break;
            }
            if (i >= FQUE_MAX) {
                fs_error(s_srcFile, FS_LINE_WAIT_NAME, "load end wait error!!",   /* data 0x01936130 */
                         "%s が LOAD されてないのに\nLOAD まちしようとした", name); /* data 0x01936150 */
                return 0;
            }
            fs_wait_frame();
        }
        ga_main_timer = timer;
        return found;
    }

    for (;;) {
        if (BankInfo[bank].state == BANK_STATE_LOADED) {
            if (name == NULL || strcmp(name, BankInfo[bank].name) == 0) {
                ga_main_timer = timer;
                return bank;
            }
        }
        for (i = 0; i < FQUE_MAX; i++) {
            if (name == NULL) {
                // The ROM does not skip empty rows here (an emptied row's bank is 0).
                if (bank == FQue[i].bank)
                    break;
            } else if (FQue[i].name[0] != '\0' && strcmp(FQue[i].name, name) == 0) {
                break;
            }
        }
        if (i >= FQUE_MAX)
            break;
        fs_wait_frame();
    }
    if (name != NULL) {
        fs_error(s_srcFile, FS_LINE_WAIT_BANK, "load end wait error!!",          /* data 0x01936130 */
                 "%s が BANK %d に LOAD されてないのに\nLOAD まちしようとした", name, bank); /* data 0x01936180 */
        return 0;
    }
    ga_main_timer = timer;
    return -1;
}

// CT3 0x018c60a0 -- serves the files until both queues are empty.
void FSMltLoadEndWaitAll(void)
{
    while (IsFSLoading() || IsMltLoading()) {
        FlProc();
        FSMltLoadProc();
        FSLoadProc();
        sceGsSyncV(0);
    }
}

// CT3 0x018c4560 -- drops queue row idx. Row 0 is the file in flight: its open file is closed, OpenFile
// forgotten, a bank still loading for it emptied and FSMltLoadProc restarted. The rows below move up.
void FSMltLoadCancelIdxQue(int idx)
{
    if (idx == 0) {
        int bank;

        if (FH != 0)
            FlReqClose(FH);
        FH = 0;
        OpenFile[0] = '\0';
        bank = FQue[0].bank;
        if (bank >= 0 && BankInfo[bank].state < 0) {
            int n = BankInfo[bank].groupCount;
            int i;

            for (i = 0; i < n; i++) {
                BANK_INFO *b = &BankInfo[bank + i];

                b->handle = 0;
                b->drawOn = 0;
                b->groupCount = 0;
                b->state = BANK_STATE_FREE;
                b->name[0] = '\0';
                b->flags = 0;
            }
        }
        Phase = 0;
    }
    // callcheck: memcpy (the rows overlap: memmove on the host)
    if (idx != FQUE_MAX - 1)
        memmove(&FQue[idx], &FQue[idx + 1], (FQUE_MAX - 1 - idx) * sizeof(FQUE)); /* ROM: memcpy */
    FQue[FQUE_MAX - 1].name[0] = '\0';
}

// CT3 0x018c4810 -- drops a waiting request by file name (rows 1..: never the file in flight).
void FSMltLoadCancelQue(char *name)
{
    char upper[64];
    int i;

    strcpy(upper, name);
    strupr(upper);
    for (i = 1; i < FQUE_MAX; i++) {
        if (FQue[i].name[0] != '\0' && strcmp(FQue[i].name, upper) == 0) {
            FSMltLoadCancelIdxQue(i);
            return;
        }
    }
}

// CT3 0x018c6070 -- empties rows 1..19 and waits for the file in flight.
void FSMltLoadCancelAll(void)
{
    memset(&FQue[1], 0, (FQUE_MAX - 1) * sizeof(FQUE));
    FSMltLoadEndWaitAll();
}

// CT3 0x018c5f10 -- puts a global bank's request after the queue's other global requests (row 0 stays
// the file in flight); the rows behind move down, the last one falling off. Answers its row.
int FSMltInsertQuePriority(FQUE *req)
{
    int i;
    int j;

    if (FQue[0].name[0] == '\0') {
        FQue[0] = *req;
        return 0;
    }
    for (i = 1; i < FQUE_MAX; i++) {
        if (FQue[i].name[0] == '\0' || FQue[i].kind != FS_LOAD_GLBL)
            break;
    }
    if (FQue[i].name[0] != '\0') {
        for (j = FQUE_MAX - 1; j > i; j--)
            FQue[j] = FQue[j - 1];
    }
    FQue[i] = *req;
    return i;
}

// CT3 0x018c5cd0 -- asks for a file into banks [bank, bank + count) as kind (FS_LOAD_*; a global file
// takes one bank of 0..5). Requests for overlapping banks are dropped first (row 0 only when it is
// another file). Answers the handle FSMltLoadEnd takes, or -1 when the queue is full.
int FSMltLoad(char *name, int kind, int bank, int count)
{
    FQUE req;
    int handle;
    int i;

    if (kind == FS_LOAD_GLBL) {
        count = 1;
        if (bank >= BANK_GLOBAL_MAX) {
            fs_error(s_srcFile, FS_LINE_GLOBAL_LIMIT, "GLOBAL BANK limit error!!", /* data 0x01936260 */
                     "%s BANK%d", name, bank);                                    /* data 0x01936280 */
            bank = 0;
            count = 1;
        }
    }
    if (bank >= 0 && count > 0) {
        for (i = 0; i < FQUE_MAX && FQue[i].name[0] != '\0'; i++) {
            int other = FQue[i].bank;
            int overlap;

            if (bank < other)
                overlap = other - bank <= count - 1;
            else
                overlap = bank - other <= FQue[i].count - 1;
            if (overlap && (i != 0 || strcmp(name, FQue[0].name) != 0)) {
                FSMltLoadCancelIdxQue(i);
                i--;
            }
        }
    }
    memset(req.work, 0, sizeof(req.work));      /* PORT: the ROM leaves its stack here */
    strcpy(req.name, name);
    req.bank = bank;
    req.count = count;
    req.kind = kind;
    // The ROM answers the incremented counter even when it wraps to 0 (and only then resets it to 1).
    handle = ++gHandleCounter;
    if (gHandleCounter == 0)
        gHandleCounter = 1;
    req.handle = handle;
    for (i = 0; i < FQUE_MAX; i++) {
        if (FQue[i].name[0] == '\0') {
            if (kind == FS_LOAD_GLBL)
                FSMltInsertQuePriority(&req);
            else
                FQue[i] = req;
            return handle;
        }
    }
    fs_error(s_srcFile, FS_LINE_QUE_MAX, "error", "file QUE MAX !!");    /* data 0x01936290, 0x019362a0 */
    return -1;
}

// CT3 0x018c7cb0 -- the directory a bank kind's files are in.
char *FSFileType(int kind, char *dir)
{
    if (kind == FS_LOAD_SHAPE || kind == FS_LOAD_BEHAVIOR)
        return strcpy(dir, "DATA/PERSONA/");                     /* data 0x019363b0 @2450 */
    if (kind == FS_LOAD_SCENE)
        return strcpy(dir, "DATA/SCENE/");                       /* data 0x019363a0 @2449 */
    if (kind == FS_LOAD_GLBL)
        return strcpy(dir, "DATA/GLOBAL/");                      /* data 0x01936390 @2448 */
    fs_error(s_srcFile, FS_LINE_TYPE_ERROR, "type error", "type %d", kind); /* data 0x019363c0, 0x019363d0 */
    return strcpy(dir, "DATA/SCENE/");
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018c2170
void CharacterSort(void)
{
    ALYSSAPAN_STUB("CharacterSort", 0x018c2170);
}

// stub of CT3 0x018c2880
void objSort(int scene, int count, void *table)
{
    ALYSSAPAN_STUB("objSort", 0x018c2880);
}

// stub of CT3 0x018c23b0
void objSortWithBlurIdx(int scene, int count, void *table)
{
    ALYSSAPAN_STUB("objSortWithBlurIdx", 0x018c23b0);
}

// stub of CT3 0x018c2df0
void Round2ChangeChar(int round)
{
    ALYSSAPAN_STUB("Round2ChangeChar", 0x018c2df0);
}

// stub of CT3 0x018c6290
void FSMltLoadProc(void)
{
    ALYSSAPAN_STUB("FSMltLoadProc", 0x018c6290);
}
