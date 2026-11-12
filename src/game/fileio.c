// game/fileio.c -- Clock Tower 3: the bank loader (CT3 0x018c2170..0x018c7fb0). A file is asked for by
// name (FSMltLoad) into FQue; FSMltLoadProc, once a frame from FileSoundServer, loads the queue's head into
// its bank of BankInfo; the game waits for it (FSMltLoadEndWaitFile / FSMltLoadEnd), turns its drawing on
// and off (FSMltSetDrawBank) and frees it at a scene's end (SceneEndBank). FSLoad* is a second, simpler
// queue for whole files into the caller's buffer. The source string "fileio.c" is fs_error's file argument.
// Slice 1 reconstructed 2026-11-10 (27 functions): the queue (FSMltLoad, its priority insert, the cancels,
// the status and wait functions), the bank table (FSMltInit, the finds, draw on / off, SceneEndBank), the
// file server, FSFileType and fs_error. Slice 2 (2026-11-10): the bank loader itself (FSMltLoadProc,
// ReserveBank, Extract: the 'CT3X' unpacker). Slice 3 (2026-11-10): the sorts (CharacterSort, objSort,
// objSortWithBlurIdx, SortAlphaObjects), SceneEndBankMovie, Round2ChangeSectInfo. Slice 4 (2026-11-10):
// the second-round file swap (Round2ChangeChar): the TU's code is complete.

#include "game/fileio.h"

#include "common/alyssapan_stub.h"
#include "game/ArtoolDebugOpen.h"
#include "game/BRANCH_CLEAR.h"
#include "game/Clipping_Portal.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/AlignedNum.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
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
#define FS_LINE_RESERVE_GROUP 1588              /* ReserveBank: the bank is inside a drawn group */
#define FS_LINE_RESERVE_BANK 1604               /* ReserveBank: a drawn bank in the range */
#define FS_LINE_DECODE 563                      /* Extract */
#define FS_LINE_MLT_OPEN 793                    /* FSMltLoadProc */
#define FS_LINE_MLT_HEAD 806                    /* FSMltLoadProc: the first sector */
#define FS_LINE_MLT_REST 945                    /* FSMltLoadProc: a plain file's other sectors */
#define FS_LINE_MLT_PACKED 968                  /* FSMltLoadProc: a packed file's next sectors */
#define FS_LINE_ROUND2_DST 2729                 /* Round2ChangeSectInfo: dst not in the pack */
#define FS_LINE_ROUND2_SRC 2734                 /* Round2ChangeSectInfo: src not in the pack */

// ---- a bank file's first sector ------------------------------------------------------------------------
// FSMltLoadProc reads sector 0 into Buf1stSector and looks at it as one of two headers. layout: ee.
#define FILE_MAGIC_CT3X 0x43543358u             /* 'CT3X' (bytes "X3TC"): a packed file, FILE_HEAD_CT3X */
#define FILE_MAGIC_BHVR 0x52564842u             /* "BHVR": a behaviour file; its checksum starts at 0x40 */
#define FILE_SUM_MARK 0x77777777u               /* the file carries a byte sum of its body */

// A plain file: its first 0x80 bytes (the image keeps them; the ALL_TABLE follows at +0x80).
typedef struct FILE_HEAD {
    u_int size;                         /* 0x00: the file's bytes */
    u_int magic;                        /* 0x04: FILE_MAGIC_BHVR or anything else */
    u_char pad08[0x16];
    short bank;                         /* 0x1e: the bank it belongs in (a request without one takes it) */
    short count;                        /* 0x20: its banks */
    u_char pad22[2];
    u_int bhvrSumMark;                  /* 0x24: BHVR: FILE_SUM_MARK = sum checked */
    u_int bhvrSum;                      /* 0x28: BHVR: the bytes from 0x40 on, summed */
    u_char pad2c[0x18];
    u_int sumMark;                      /* 0x44: others: FILE_SUM_MARK = sum checked */
    u_int sum;                          /* 0x48: others: the bytes from 0x80 on, summed */
} FILE_HEAD;
static_assert(offsetof(FILE_HEAD, bank) == 0x1e, "FILE_HEAD.bank");
static_assert(offsetof(FILE_HEAD, bhvrSum) == 0x28, "FILE_HEAD.bhvrSum");
static_assert(offsetof(FILE_HEAD, sum) == 0x48, "FILE_HEAD.sum");

// A packed file ('CT3X', pCT3H = Buf1stSector): the whole first sector is the decoder's header, the
// packed bytes start at sector 1 (Extract).
#define CT3X_WORDS 246                          /* codes 10..255 */
#define CT3X_CODE_WORD0 10
typedef struct FILE_HEAD_CT3X {
    u_int size;                         /* 0x000: the packed file's bytes (sector 0 included) */
    u_int magic;                        /* 0x004: FILE_MAGIC_CT3X */
    u_int unpacked;                     /* 0x008: the image's bytes */
    u_char escape;                      /* 0x00c: the byte that starts a code */
    u_char pad0d[0xb];
    u_int words[CT3X_WORDS];            /* 0x018: code n >= 10 writes words[n - 10] */
    u_int byteWords[256];               /* 0x3f0: code 9, byte b writes byteWords[b] */
    short bank;                         /* 0x7f0 */
    short count;                        /* 0x7f2 */
    u_char pad7f4[0xc];
} FILE_HEAD_CT3X;
static_assert(sizeof(FILE_HEAD_CT3X) == FL_SECTOR_SIZE, "FILE_HEAD_CT3X");
static_assert(offsetof(FILE_HEAD_CT3X, words) == 0x18, "FILE_HEAD_CT3X.words");
static_assert(offsetof(FILE_HEAD_CT3X, byteWords) == 0x3f0, "FILE_HEAD_CT3X.byteWords");
static_assert(offsetof(FILE_HEAD_CT3X, bank) == 0x7f0, "FILE_HEAD_CT3X.bank");

// Extract's codes (the byte after the escape).
enum CT3X_CODE {
    CT3X_ESCAPE = 0,                    /* the escape byte itself */
    CT3X_FILL8 = 1,                     /* n, byte: n bytes */
    CT3X_FILL16 = 2,                    /* n, short: n shorts */
    CT3X_FILL32 = 3,                    /* n, word: n words */
    CT3X_DELTA = 4,                     /* d: the previous word + (signed) d */
    CT3X_BYTE_WORD = 9,                 /* b: byteWords[b] */
};
#define CT3X_TMP_MAX 12                         /* ExtBufTmp: the longest code (7) fits */

#define FS_PACKED_READ_SECTS 60                 /* ExtBuf: a packed file is read 60 sectors at a time */
#define FS_IMAGE_ALIGN 0x40                     /* a global bank's image in the GLBL heap */

FQUE FQue[FQUE_MAX];                            /* bss  0x01c833c0 */
int Phase;                                      /* sbss 0x01942b4c: FSMltLoadProc's step */
CAM2OBJ *pCam2objSort;                          /* sdata 0x0193f73c */
CAM2OBJ *pCam2objBuff;                          /* sdata 0x0193f738 */
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
static int LoadNowSects;                        /* sbss 0x01942b54: FSMltLoadProc's sectors read */
static int LoadTotalSects;                      /* sbss 0x01942b58: the file's sectors */
static int sRsect;                              /* sbss 0x01942cd0 sRsect$252: the packed read's sectors */
static u_char ExtBufTmp[CT3X_TMP_MAX];          /* sbss 0x01942d00: a code split across two reads */
static int ExtBufTmpIdx;                        /* sbss: its bytes */
static u_char *ExtWrPtr;                        /* sbss 0x01942d0c: where Extract writes next (host pointer) */
// ExtBuf: the packed file's reads; Buf1stSector right behind it in the ROM (bss 0x01c82bc0), and so
// what Extract peeks at past a full ExtBuf. PORT: the two stay one array.
alignas(64) static u_char ExtBufArea[FS_PACKED_READ_SECTS * FL_SECTOR_SIZE + FL_SECTOR_SIZE]; /* bss 0x01c64bc0 */
static u_char *const ExtBuf = ExtBufArea;
static u_char *const Buf1stSector = ExtBufArea + FS_PACKED_READ_SECTS * FL_SECTOR_SIZE;
#define pCT3H ((const FILE_HEAD_CT3X *)Buf1stSector)    /* sdata 0x01939e90: = &Buf1stSector */
#define pFileHead ((const FILE_HEAD *)Buf1stSector)
int sErrCount;                                  /* sbss 0x0193f774 */

// PORT: the slots' memory. The ROM's banks 6..85 are EE RAM from gGlobalBankArea + 0x10000 (EE 0x00410000)
// up to the ELF (0x01810000); the host gives them one buffer and records each slot's host address and EE
// address in BankImage[] / BankImageEE[] (ct3_types.h BankEePtr, open-items.md 1 h).
alignas(64) static u_char gBankSlotArea_host[BANK_SLOT_COUNT * BANK_SLOT_SIZE];
u_char *gBankSlotArea = gBankSlotArea_host;

// ---- the sorts ----------------------------------------------------------------------------------------
// The draw lists' and the characters' far-to-near order for translucent drawing: each sort fills
// pCam2objSort with one CAM2OBJ per item (its place, its id, its distance from the camera), merge-sorts
// the rows (SortAlphaObjects) and writes the items back in that order.

// The parts header record asFindModelParts answers (header + 0x40 + i * 0x380; see PARTS_REC).
#define REC_SORT_POINT 0x20             /* float[4]: the point CharacterSort measures (MODEL_PARTS +0x60) */
#define REC_MATRIX_PREV 0x1c0           /* float[4][4]: last frame's world matrix (MODEL_PARTS +0x200) */

// CharacterSort's rows: the ROM allocates 0x1a4 bytes (35 rows) and writes a 36th row's id and dist
// when every character counts. PORT: room for the 36th.
#define CHARA_SORT_ROWS (CHARACTER_MAX + 1)

// out = m applied to v (vmulax / vmadday / vmaddaz / vmaddw): m[0] v.x + m[1] v.y + m[2] v.z + m[3] v.w.
static void SortPointApply(float out[4], const float m[4][4], const float v[4])
{
    int i;
    for (i = 0; i < 4; i++)
        out[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
}

// CT3 0x018c2170
// Character[]'s drawn characters (both CHARACTER_FLAG_REFRACTION and _SHADOW set, a bank) into
// CharacterSortBuff, far to near: each measured from the camera at its bank's origin part
// (PARTS_HASH_SOUND_ORIGIN), placed by that part's last-frame matrix. -1 ends the list.
void CharacterSort(void)
{
    alignas(16) float cam[4];
    alignas(16) float at[4];
    short c;
    int n = 0;

    pCam2objSort = (CAM2OBJ *)_SunAlloc(&gHeapJunc, CHARA_SORT_ROWS * (int)sizeof(CAM2OBJ), 0x10);
    pCam2objBuff = (CAM2OBJ *)_SunAlloc(&gHeapJunc, CHARA_SORT_ROWS * (int)sizeof(CAM2OBJ), 0x10);
    CameraMatrix2CameraPos(cam);
    for (c = 0; c < CHARACTER_MAX; c++) {
        CHARACTER *ch = &Character[c];
        u_char *rec;

        if (!(ch->flags & CHARACTER_FLAG_REFRACTION) || !(ch->flags & CHARACTER_FLAG_SHADOW) || ch->bank < 0)
            continue;
        if (All_table_TBL[ch->bank] == NULL)    // PORT: the ROM reads a bank's table unchecked
            continue;
        rec = asFindModelParts(BANK_PTR(u_char *, ch->bank, All_table_TBL[ch->bank]->partsHeader),
                               PARTS_HASH_SOUND_ORIGIN);
        if (rec == NULL)                        // PORT: the ROM reads the answer unchecked
            continue;
        SortPointApply(at, (const float(*)[4])(rec + REC_MATRIX_PREV), (const float *)(rec + REC_SORT_POINT));
        pCam2objSort[n].index = n;
        pCam2objSort[n].id = c;
        pCam2objSort[n].dist = asVectorDistance(cam, at);
        n++;
    }
    pCam2objSort[n].id = 0;
    pCam2objSort[n].dist = 0.0f;
    if (n > 1)
        SortAlphaObjects(n, pCam2objSort);
    for (c = 0; c < n; c++)
        CharacterSortBuff[c] = (short)pCam2objSort[c].id;
    CharacterSortBuff[n] = -1;
    _SunFree(&gHeapJunc, (void **)&pCam2objBuff);
    _SunFree(&gHeapJunc, (void **)&pCam2objSort);
}

// CT3 0x018c23b0
// objSort for a bank with part motion-blur objects (ALLpartBlurObj[1][scene]): the rows are measured at
// the parts' pos only, and each blur object whose listIndex is a sorted entry's place in the objTrans
// list gets that entry's row (sortedIndex) and joins aftSortALLpartBlurObj in sorted order. The slots
// are cleared as they match (so an entry matches once) and restored at the end.
void objSortWithBlurIdx(int scene, int count, void *table)
{
    u_int *out = (u_int *)table;
    u_int *list = BANK_PTR(u_int *, scene, All_table_TBL[scene]->objTrans);
    PART_BLUR_OBJ **slots = ALLpartBlurObj[1][scene];
    PART_BLUR_OBJ *keep[PART_BLUR_MAX];
    PART_BLUR_OBJ *cur = NULL;
    u_int n, i, nBlur;
    int k, idx, matched = 0, exhausted = 0, done = 0;

    if (list[0] == 0) {
        out[0] = 0;
        return;
    }
    pCam2objSort = (CAM2OBJ *)_SunAlloc(&gHeapJunc, count * (int)sizeof(CAM2OBJ), 0x10);
    pCam2objBuff = (CAM2OBJ *)_SunAlloc(&gHeapJunc, count * (int)sizeof(CAM2OBJ), 0x10);
    n = (u_int)count - 1;
    for (i = 0; i < n; i++) {
        DRAW_OBJ_ENTRY *ent = BANK_PTR(DRAW_OBJ_ENTRY *, scene, list[i]);

        pCam2objSort[i].index = (int)i;
        pCam2objSort[i].id = ent->parts;
        pCam2objSort[i].dist = asVectorDistance(CameraPos, ((MODEL_PARTS *)pModelPartsHeader[scene])[ent->parts].pos);
    }
    pCam2objSort[count - 1].id = 0;
    pCam2objSort[count - 1].dist = 0.0f;
    SortAlphaObjects((int)n, pCam2objSort);

    nBlur = NumPartBlurObj[1][scene];
    for (i = 0; i < nBlur; i++)
        keep[i] = slots[i];
    for (i = 0; i < n; i++) {
        idx = pCam2objSort[i].index;
        k = 0;
        if (!exhausted) {                       // the first filled slot (the ROM reads slot 0 first)
            done = 0;
            while ((cur = slots[k]) == NULL) {
                if (++k >= (int)nBlur) {
                    exhausted = done = 1;
                    break;
                }
            }
        }
        out[i] = list[idx];
        while (!done && cur->listIndex < idx) { // on to the next filled slot
            do {
                if (++k >= (int)nBlur) {
                    done = 1;
                    break;
                }
                cur = slots[k];
            } while (cur == NULL);
        }
        if (!done && cur->listIndex == idx) {
            cur->sortedIndex = (int)i;
            slots[k] = NULL;
            aftSortALLpartBlurObj[matched++] = cur;
        }
    }
    aftSortALLpartBlurObj[nBlur] = NULL;        // ROM: at the slot count, not the matched count
    for (i = 0; i < nBlur; i++)
        slots[i] = keep[i];
    out[count - 1] = 0;
    _SunFree(&gHeapJunc, (void **)&pCam2objBuff);
    _SunFree(&gHeapJunc, (void **)&pCam2objSort);
}

// CT3 0x018c2880
// A bank's translucent list (ALL_TABLE.objTrans, count - 1 entries and its 0) into table, far to near:
// each entry measured from CameraPos at its part's pos, or, for a PARTS_FLAG_SORT_CENTER part, at its
// draw object's DRAW_OBJ_CENTER placed by the part's matrix. table gets the entries' EE words, then 0.
void objSort(int scene, int count, void *table)
{
    u_int *out = (u_int *)table;
    u_int *list = BANK_PTR(u_int *, scene, All_table_TBL[scene]->objTrans);
    alignas(16) float at[4];
    u_int i;

    if (list[0] == 0) {
        out[0] = 0;
        return;
    }
    pCam2objSort = (CAM2OBJ *)_SunAlloc(&gHeapJunc, count * (int)sizeof(CAM2OBJ), 0x10);
    pCam2objBuff = (CAM2OBJ *)_SunAlloc(&gHeapJunc, count * (int)sizeof(CAM2OBJ), 0x10);
    for (i = 0; i < (u_int)count - 1; i++) {
        DRAW_OBJ_ENTRY *ent = BANK_PTR(DRAW_OBJ_ENTRY *, scene, list[i]);
        MODEL_PARTS *parts = &((MODEL_PARTS *)pModelPartsHeader[scene])[ent->parts];

        pCam2objSort[i].index = (int)i;
        pCam2objSort[i].id = ent->parts;
        if (parts->flags & PARTS_FLAG_SORT_CENTER) {
            SortPointApply(at, parts->matrix, (const float *)(BANK_PTR(u_char *, scene, ent->obj) + DRAW_OBJ_CENTER));
            pCam2objSort[i].dist = asVectorDistance(CameraPos, at);
        } else {
            pCam2objSort[i].dist = asVectorDistance(CameraPos, parts->pos);
        }
    }
    pCam2objSort[count - 1].id = 0;
    pCam2objSort[count - 1].dist = 0.0f;
    SortAlphaObjects(count - 1, pCam2objSort);
    for (i = 0; i < (u_int)count - 1; i++)
        out[i] = list[pCam2objSort[i].index];
    out[count - 1] = 0;
    _SunFree(&gHeapJunc, (void **)&pCam2objBuff);
    _SunFree(&gHeapJunc, (void **)&pCam2objSort);
}

// CT3 0x018c2ad0
// Merge sort of n rows by dist, far to near: the halves sorted in place, the first copied to
// pCam2objBuff, then merged back. Not stable: on equal distances the second half's row goes first.
void SortAlphaObjects(int n, CAM2OBJ *rows)
{
    int half, i, j, k;

    if (n < 2)
        return;
    half = n / 2;
    SortAlphaObjects(half, rows);
    SortAlphaObjects(n - half, rows + half);
    for (i = 0; i < half; i++)
        pCam2objBuff[i] = rows[i];
    i = 0;
    j = half;
    k = 0;
    while (i < half && j < n) {
        if (rows[j].dist < pCam2objBuff[i].dist)
            rows[k++] = pCam2objBuff[i++];
        else
            rows[k++] = rows[j++];
    }
    while (i < half)
        rows[k++] = pCam2objBuff[i++];
}

// ---- the second round -----------------------------------------------------------------------------------
// Round2ChangeChar's argument: data_load_set passes ROUND2_NEW_GAME when the game was never cleared, else
// ssTrigGetVariable(TRIG_VAR_ROUND) clamped to >= 1; hhXcmd_DressUp passes its own. Rounds 2..5 dress
// Alyssa in one of four costumes (CAL1..CAL4); every round from 1 on also swaps the shared second-round
// files (s_r2CommonFiles). The meaning of 1..5 is read from the file names, not from the game's text.
#define ROUND2_NEW_GAME 0                       /* the pack directory as on the disc (FlInit reloads it) */
#define ROUND2_CLEARED 1                        /* reloaded, then only the shared second-round swaps */
#define ROUND2_COSTUME_1 2                      /* Alyssa's files from CAL1 / 2xxx */
#define ROUND2_COSTUME_4 5                      /* the last: CAL4 / 5xxx */
#define ROUND2_COSTUME_FILES 19                 /* Alyssa's model and animations, swapped per costume */
#define ROUND2_COMMON_FILES 25                  /* the shared swaps */

// The ROM calls Round2ChangeSectInfo once per pair with these literals (data 0x01935768..0x01936068, 16
// bytes each, in this order); the tables keep the order and the pairs.
static const char *const s_r2AlyssaFiles[ROUND2_COSTUME_FILES] = {
    "ALYS.BIN", "ALYS.ANM", "YAJI.ANM", "A000.ANM", "K004.ANM", "A001.ANM", "A002.ANM",
    "PALY.ANM", "A003.ANM", "A004.ANM", "A017.ANM", "A007.ANM", "A021.ANM", "A053.ANM",
    "ALE2.ANM", "ALE3.ANM", "S34D.ANM", "ALE4.ANM", "ALE5.ANM",
};

static const char *const s_r2CostumeFiles[ROUND2_COSTUME_4 - ROUND2_COSTUME_1 + 1][ROUND2_COSTUME_FILES] = {
    { "CAL1.BIN", "CAL1.ANM", "YAJ1.ANM", "2000.ANM", "6004.ANM", "2001.ANM", "2002.ANM",
      "2ALY.ANM", "2003.ANM", "2004.ANM", "2017.ANM", "2007.ANM", "2021.ANM", "2053.ANM",
      "2LE2.ANM", "2LE3.ANM", "234D.ANM", "2LE4.ANM", "2LE5.ANM" },
    { "CAL2.BIN", "CAL2.ANM", "YAJ2.ANM", "3000.ANM", "7004.ANM", "3001.ANM", "3002.ANM",
      "3ALY.ANM", "3003.ANM", "3004.ANM", "3017.ANM", "3007.ANM", "3021.ANM", "3053.ANM",
      "3LE2.ANM", "3LE3.ANM", "334D.ANM", "3LE4.ANM", "3LE5.ANM" },
    { "CAL3.BIN", "CAL3.ANM", "YAJ3.ANM", "4000.ANM", "8004.ANM", "4001.ANM", "4002.ANM",
      "4ALY.ANM", "4003.ANM", "4004.ANM", "4017.ANM", "4007.ANM", "4021.ANM", "4053.ANM",
      "4LE2.ANM", "4LE3.ANM", "434D.ANM", "4LE4.ANM", "4LE5.ANM" },
    { "CAL4.BIN", "CAL4.ANM", "YAJ4.ANM", "5000.ANM", "9004.ANM", "5001.ANM", "5002.ANM",
      "5ALY.ANM", "5003.ANM", "5004.ANM", "5017.ANM", "5007.ANM", "5021.ANM", "5053.ANM",
      "5LE2.ANM", "5LE3.ANM", "534D.ANM", "5LE4.ANM", "5LE5.ANM" },
};

static const char *const s_r2CommonFiles[ROUND2_COMMON_FILES][2] = {  /* { dst, src } */
    { "HAMM.BIN", "HAM1.BIN" }, { "HAMM.ANM", "HAM1.ANM" }, { "HALB.ANM", "HALF.ANM" },
    { "CSAA.BIN", "CSA1.BIN" }, { "CSAA.ANM", "CSA1.ANM" }, { "CSLB.ANM", "CSLF.ANM" },
    { "E_93.BIN", "E_83.BIN" },
    { "CAXA.BIN", "CAX1.BIN" }, { "CAXA.ANM", "CAX1.ANM" }, { "CALB.ANM", "CALF.ANM" },
    { "E_14.BIN", "E_24.BIN" },
    { "CSNA.BIN", "CSN1.BIN" }, { "CSNA.ANM", "CSN1.ANM" }, { "CNLB.ANM", "CNLF.ANM" },
    { "CSWA.BIN", "CSW1.BIN" }, { "CSWA.ANM", "CSW1.ANM" }, { "CWLB.ANM", "CWLF.ANM" },
    { "CVRA.BIN", "CVR1.BIN" }, { "CVRA.ANM", "CVR1.ANM" },
    { "HM02.ANM", "2HM2.ANM" }, { "HM04.ANM", "2HM4.ANM" }, { "CSA2.ANM", "2SA2.ANM" },
    { "CAX2.ANM", "2AX2.ANM" }, { "CSN2.ANM", "2SN2.ANM" }, { "CSW2.ANM", "2SW2.ANM" },
};

// CT3 0x018c2df0 -- points pack files at their second-round versions (Round2ChangeSectInfo: the names
// stay, the sectors change). Nothing happens without ALYS.BIN in the pack. A new game reloads the
// directory and stops; a cleared game reloads it before the shared swaps; a costume round swaps Alyssa's
// files on the directory as it stands (the ROM's cases 2..5 have no reload), then the shared ones. Any
// other value (the switch's default) does only the shared swaps.
void Round2ChangeChar(int round)
{
    int i;

    if (FlFindPacked("ALYS.BIN") == -1) {              /* data 0x01935768 */
        return;
    }
    if (round == ROUND2_NEW_GAME) {
        FlInit();
        return;
    }
    if (round == ROUND2_CLEARED) {
        FlInit();
    } else if ((u_int)round <= ROUND2_COSTUME_4 && round >= ROUND2_COSTUME_1) {  /* ROM: sltiu 6, table */
        for (i = 0; i < ROUND2_COSTUME_FILES; i++) {
            Round2ChangeSectInfo((char *)s_r2AlyssaFiles[i],
                                 (char *)s_r2CostumeFiles[round - ROUND2_COSTUME_1][i]);
        }
    }
    for (i = 0; i < ROUND2_COMMON_FILES; i++) {
        Round2ChangeSectInfo((char *)s_r2CommonFiles[i][0], (char *)s_r2CommonFiles[i][1]);
    }
}


// CT3 0x018c3670 -- the pack file dst is read from src's sectors from now on: gFlPackInfo[dst] takes
// src's size, sector and sector count (the names stay). Both names are upper-cased first; a missing one
// is reported and nothing changes.
void Round2ChangeSectInfo(char *dst, char *src)
{
    char dstName[32];
    char srcName[32];
    int d, s;

    strcpy(dstName, dst);
    strcpy(srcName, src);
    strupr(dstName);
    strupr(srcName);
    d = FlFindPacked(dstName);
    if (d < 0) {
        fs_error(s_srcFile, FS_LINE_ROUND2_DST, "2しゅうめCHAR", "%sがない", dstName); /* data 0x019360a8, 0x019360b8 */
        return;
    }
    s = FlFindPacked(srcName);
    if (s < 0) {
        fs_error(s_srcFile, FS_LINE_ROUND2_SRC, "2しゅうめCHAR", "%sがない", srcName);
        return;
    }
    gFlPackInfo[d].size = gFlPackInfo[s].size;
    gFlPackInfo[d].sect = gFlPackInfo[s].sect;
    gFlPackInfo[d].numSect = gFlPackInfo[s].numSect;
}

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
                     "DRAWONã—ã‚ˆã†ã¨ã—ãŸã‘ã©\n%sãŒBANKã«ãã‚“ã–ã„ã—ãªã„", name);   /* data 0x019361e0 */
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

// CT3 0x018c5400 -- SceneEndBank for a movie (AA_DeleteSceneBank): the same range and the same
// scene_end, but nothing is cleared for good. A scene or shape group's banks are parked -- the group's
// i-th bank to BANK_STATE_MOVIE_PARKED + i, its name, group and draw state kept -- and a global bank's
// memory goes back to the GLBL heap with the bank parked the same way. Behaviour banks stay loaded.
void SceneEndBankMovie(int first, int count)
{
    int i, j;

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
            for (j = 0; j < n; j++) {
                BankInfo[i + j].state = BANK_STATE_MOVIE_PARKED + j;
                BankInfo[i + j].flags &= ~BANK_FLAG_SCENE_INIT;
            }
        } else if (b->type == BANK_TYPE_0) {                          /* FS_LOAD_GLBL */
            if (b->flags & BANK_FLAG_SCENE_INIT)
                scene_end(i);
            if (b->image != 0) {
                // The ROM frees the EE address in BankInfo +0x1c; the host frees its image.
                _SunFree(&gHeapGlobalScene, (void **)&BankImage[i]);
                b->image = 0;
                BankImageEE[i] = 0;
            }
            b->state = BANK_STATE_MOVIE_PARKED;
            b->flags &= ~BANK_FLAG_SCENE_INIT;
        }
        i += n - 1;
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
                         "%s ãŒ LOAD ã•ã‚Œã¦ãªã„ã®ã«\nLOAD ã¾ã¡ã—ã‚ˆã†ã¨ã—ãŸ", name); /* data 0x01936150 */
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
                 "%s ãŒ BANK %d ã« LOAD ã•ã‚Œã¦ãªã„ã®ã«\nLOAD ã¾ã¡ã—ã‚ˆã†ã¨ã—ãŸ", name, bank); /* data 0x01936180 */
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
    memset(req.dir, 0, sizeof(req.dir));        /* PORT: the ROM leaves its stack here (FSMltLoadProc fills it) */
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

// ---- the loader ---------------------------------------------------------------------------------------

// Empties one group's rows, its handle too (ReserveBank, FSMltLoadProc's error step).
static void fs_free_group(int bank, int n)
{
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

// ReserveBank's complaint about a drawn bank it is about to take: ends its scene and empties its group.
static void fs_reserve_drawn(int bank, int line, const char *name, int first, int count)
{
    char title[64];

    // callcheck: sprintf (PORT: snprintf)
    snprintf(title, sizeof(title), "%s BANK%d-%d DRAW ON", BankInfo[bank].name, bank,  /* data _1725 */
             bank + BankInfo[bank].groupCount - 1);
    fs_error(s_srcFile, line, title, "ãªã®ã« %s BANK%d-%d\nã‚’LOADã—ã‚ˆã†ã¨ã—ãŸ",      /* data 0x01936230 */
             name, first, first + count - 1);
    if (BankInfo[bank].flags & BANK_FLAG_SCENE_INIT)
        scene_end(bank);
    fs_clear_group(bank, BankInfo[bank].groupCount);
}

// CT3 0x018c49a0 -- takes banks [first, first + count) for name: every group overlapping them is emptied
// (a drawn one is an error, its scene ended), then the banks are marked loading: state -(place), the
// group count left from each (count - place + 1), the name, kind and handle. The emptied groups' rows
// are re-read after the drawn-group clear, which zeroed their count: that second clear then does nothing.
static void ReserveBank(const char *name, int first, int count, int kind, int handle)
{
    int i;

    // callcheck: fs_error, scene_end, sprintf, strcpy (the drawn-bank complaint: fs_reserve_drawn)
    if (first < 0 || count <= 0)
        return;
    if (first != 0 && BankInfo[first].state > BANK_STATE_LOADED) {
        int head = first - (BankInfo[first].state - 1);

        if (head >= 0) {
            if (BankInfo[head].drawOn != 0)
                fs_reserve_drawn(head, FS_LINE_RESERVE_GROUP, name, first, count);
            fs_free_group(head, BankInfo[head].groupCount);
        }
    }
    for (i = first; i < first + count; i++) {
        if (BankInfo[i].state == BANK_STATE_FREE)
            continue;
        if (BankInfo[i].drawOn != 0)
            fs_reserve_drawn(i, FS_LINE_RESERVE_BANK, name, first, count);
        fs_free_group(i, BankInfo[i].groupCount);
        i += BankInfo[i].groupCount - 1;        /* 0 once emptied: the bank is looked at again, free */
    }
    for (i = 0; i < count; i++) {
        BANK_INFO *b = &BankInfo[first + i];

        b->state = -(i + 1);
        b->groupCount = count - i;
        strcpy(b->name, name);
        b->type = kind;
        b->drawOn = 0;
        b->handle = handle;
    }
}

// Stores the bytes of an unfinished code for the next call (Extract): answers how many.
static int ct3x_keep(const u_char *src, const u_char *end, u_char **out, u_char *wr)
{
    ExtBufTmpIdx = 0;
    for (; src < end; src++)
        ExtBufTmp[ExtBufTmpIdx++] = *src;
    *out = wr;
    return ExtBufTmpIdx;
}

// CT3 0x018c74b0 -- unpacks [src, end) of a 'CT3X' file to *out (advanced): bytes are copied, the header's
// escape byte starts a code (CT3X_CODE). A code cut by the read's end is kept in ExtBufTmp (the answer is
// its length) and finished first by the next call, through a recursive call on ExtBufTmp. The ROM reads the
// code byte before checking it is inside [src, end): past a full ExtBuf that is Buf1stSector's first byte.
int Extract(const u_char *hdr, const u_char *src, const u_char *end, u_char **out)
{
    const FILE_HEAD_CT3X *h = (const FILE_HEAD_CT3X *)hdr;
    u_char *wr = *out;

    if (ExtBufTmpIdx != 0) {
        int len = 2;

        // callcheck: memcpy
        memcpy(ExtBufTmp + ExtBufTmpIdx, src, CT3X_TMP_MAX - ExtBufTmpIdx);
        switch (ExtBufTmp[1]) {
        case CT3X_ESCAPE:
            break;
        case CT3X_FILL8:
            len = 4;
            break;
        case CT3X_FILL16:
            len = 5;
            break;
        case CT3X_FILL32:
            len = 7;
            break;
        case CT3X_DELTA:
        case CT3X_BYTE_WORD:
            len = 3;
            break;
        case 5: case 6: case 7: case 8:
            fs_error(s_srcFile, FS_LINE_DECODE, "decode error!!", "");          /* data 0x01936318 */
            break;
        default:
            len = 2;
            break;
        }
        src += len - ExtBufTmpIdx;
        ExtBufTmpIdx = 0;
        Extract(hdr, ExtBufTmp, ExtBufTmp + len, &wr);
    }
    while (src < end) {
        const u_char *p = src;
        u_int n, k;

        if (*p != h->escape) {
            *wr++ = *p;
            src = p + 1;
            continue;
        }
        switch (p[1]) {
        case CT3X_ESCAPE:
            if (end <= p + 1)
                return ct3x_keep(p, end, out, wr);
            *wr++ = h->escape;
            src = p + 2;
            break;
        case CT3X_FILL8:
            if (end <= p + 3)
                return ct3x_keep(p, end, out, wr);
            n = p[2];
            for (k = 0; k < n; k++)
                *wr++ = p[3];
            src = p + 4;
            break;
        case CT3X_FILL16:
            if (end <= p + 4)
                return ct3x_keep(p, end, out, wr);
            n = p[2];
            for (k = 0; k < n; k++, wr += 2)
                memcpy(wr, p + 3, 2);
            src = p + 5;
            break;
        case CT3X_FILL32:
            if (end <= p + 6)
                return ct3x_keep(p, end, out, wr);
            n = p[2];
            for (k = 0; k < n; k++, wr += 4)
                memcpy(wr, p + 3, 4);
            src = p + 7;
            break;
        case CT3X_DELTA: {
            int prev;

            if (end <= p + 2)
                return ct3x_keep(p, end, out, wr);
            memcpy(&prev, wr - 4, 4);
            prev += (s_char)p[2];
            memcpy(wr, &prev, 4);
            wr += 4;
            src = p + 3;
            break;
        }
        case 5: case 6: case 7: case 8:
            src = p + 2;                        /* the ROM skips them silently here */
            break;
        case CT3X_BYTE_WORD:
            if (end <= p + 2)
                return ct3x_keep(p, end, out, wr);
            memcpy(wr, &h->byteWords[p[2]], 4);
            wr += 4;
            src = p + 3;
            break;
        default:
            if (end <= p + 1)
                return ct3x_keep(p, end, out, wr);
            memcpy(wr, &h->words[p[1] - CT3X_CODE_WORD0], 4);
            wr += 4;
            src = p + 2;
            break;
        }
    }
    *out = wr;
    ExtBufTmpIdx = 0;
    return 0;
}

// The byte sum of [p, p + n) (FSMltLoadProc's check).
static int fs_byte_sum(const u_char *p, u_int n)
{
    int sum = 0;
    u_int i;

    for (i = 0; i < n; i++)
        sum += p[i];
    return sum;
}

// PORT: a global bank's image from the GLBL heap, with the EE address it would have had.
static void fs_alloc_global_image(int bank, int size)
{
    BankImage[bank] = (u_char *)_SunAlloc(&gHeapGlobalScene, AlignedNum(size, FL_SECTOR_SIZE), FS_IMAGE_ALIGN);
    BankInfo[bank].image = BankImage[bank]
        ? GLOBAL_BANK_AREA_EE + (u_int)(BankImage[bank] - gGlobalBankArea) : 0;
    BankImageEE[bank] = BankInfo[bank].image;
}

// FSMltLoadProc's FSMLT_FIND step: is FQue[0] already in its bank (loaded or loading)? Then the bank
// takes the new handle.
static int fs_mlt_already_there(void)
{
    int i, k;

    for (i = 0; i < BANK_MAX; i++) {
        if (BankInfo[i].state != BANK_STATE_LOADED && BankInfo[i].state != BANK_STATE_LOADING)
            continue;
        if (strcmp(BankInfo[i].name, FQue[0].name) == 0 && FQue[0].bank != -1 && i == FQue[0].bank) {
            for (k = 0; k < FQue[0].count; k++)
                BankInfo[i + k].handle = FQue[0].handle;
            return 1;
        }
        i += BankInfo[i].groupCount - 1;
    }
    return 0;
}

// FSMltLoadProc's FSMLT_CHECK step: the loaded file's set-up -- the byte sum (a mismatch reads it again
// from sector 0), relocation, the group's places, and for a scene / shape / global bank its ALL_TABLE,
// scene_init and matrix tree (and for a scene or global bank its sound effects). Answers 0 to read again.
static int fs_mlt_setup(void)
{
    const int bank = FQue[0].bank;
    BANK_INFO *b;
    int i;

    if (pFileHead->magic == FILE_MAGIC_BHVR) {
        if (pFileHead->bhvrSumMark == FILE_SUM_MARK
            && fs_byte_sum(BankImage[bank] + 0x40, pFileHead->size - 0x40) != (int)pFileHead->bhvrSum) {
            Phase = FSMLT_READ_HEAD;            /* the ROM keeps the image (a global one leaks) */
            return 0;
        }
    } else if (pFileHead->sumMark == FILE_SUM_MARK
               && fs_byte_sum(BankImage[bank] + 0x80, pFileHead->size - 0x80) != (int)pFileHead->sum) {
        if (FQue[0].kind == FS_LOAD_GLBL && BankInfo[bank].image != 0) {
            _SunFree(&gHeapGlobalScene, (void **)&BankImage[bank]);
            BankInfo[bank].image = 0;
            BankImageEE[bank] = 0;
        }
        Phase = FSMLT_READ_HEAD;
        return 0;
    }
    FlReqClose(FH);
    FH = 0;
    OpenFile[0] = '\0';
    ahRelocateFileObject(BankImage[bank], FQue[0].kind == FS_LOAD_SHAPE || FQue[0].kind == FS_LOAD_BEHAVIOR);
    for (i = 0; i < FQue[0].count; i++)
        BankInfo[bank + i].state = i + 1;
    b = &BankInfo[bank];
    if (b->type != FS_LOAD_BEHAVIOR) {
        All_table_TBL[bank] = (ALL_TABLE *)(BankImage[bank] + BANK_ALL_TABLE_OFFSET);
        scene_init(bank);
        for (i = 0; i < FQue[0].count; i++)
            BankInfo[bank + i].flags |= BANK_FLAG_SCENE_INIT;
        if (b->type == FS_LOAD_SCENE)
            sFirstCheck = 0;
        BuildMatrixTree(bank, 1, 1);
        if (b->type == FS_LOAD_GLBL || b->type == FS_LOAD_SCENE)
            SSLoadSEfromLoadPoint(BankImage[bank]);
    }
    return 1;
}

// CT3 0x018c6290 -- once a frame: one step of loading FQue[0] into its banks (FSMLT_PHASE, in Phase).
// The file is opened from FSFileType's directory, its first sector read and looked at: a 'CT3X' file is
// unpacked through ExtBuf 60 sectors at a time, a plain one read straight into the image. A request
// without a bank or count takes the file's. Loaded, the bank is set up (fs_mlt_setup) and the queue moves
// up. An open error is retried every frame; a read error starts the file over (FSMLT_ERROR).
void FSMltLoadProc(void)
{
    // callcheck: FlIsExecute, FlReqClose, FlReqOpen, FlReqRead, Extract, SceneEndBank, ReserveBank, strcmp, strcpy, strcat, AlignedNum, _SunAlloc, _SunFree, memcpy, ahRelocateFileObject, scene_init, BuildMatrixTree, SSLoadSEfromLoadPoint, printf, fs_error
    FQUE *q = &FQue[0];
    u_int size;

    switch (Phase) {
    case FSMLT_ERROR:
        printf("LOAD RETRY %s\n", q->name);                                 /* data 0x01936308 */
        if (FH != 0)
            FlReqClose(FH);
        FH = 0;
        fs_free_group(q->bank, q->count);       /* bank >= 0: only the steps after HEAD_WAIT fail here */
        Phase = FSMLT_START;
        return;
    case FSMLT_IDLE:
        if (q->name[0] == '\0')
            return;
        if (fs_mlt_already_there()) {
            Phase = FSMLT_DONE;
            return;
        }
        Phase = FSMLT_START;
        /* fall through */
    case FSMLT_START:
        SceneEndBank(q->bank, q->count);
        ReserveBank(q->name, q->bank, q->count, q->kind, q->handle);
        FSFileType(q->kind, q->dir);
        strcpy(OpenFile, q->dir);
        strcat(OpenFile, q->name);
        Phase = FSMLT_OPEN;
        return;
    case FSMLT_OPEN:
        FH = FlReqOpen(q->dir, q->name);
        if (FH == 0) {
            fs_error(s_srcFile, FS_LINE_MLT_OPEN, "file open error !!", "err%d %s", 0, OpenFile); /* data 0x019362b0 */
            return;
        }
        Phase = FSMLT_READ_HEAD;
        return;
    case FSMLT_READ_HEAD:
        if (FlReqRead(FH, Buf1stSector, 0, 1) != 0) {
            fs_error(s_srcFile, FS_LINE_MLT_HEAD, "file read error !!", "%s", OpenFile); /* data 0x019362e0 */
            return;
        }
        Phase = FSMLT_HEAD_WAIT;
        return;
    case FSMLT_HEAD_WAIT: {
        const int packed = pFileHead->magic == FILE_MAGIC_CT3X;
        const int fileBank = packed ? pCT3H->bank : pFileHead->bank;
        const int fileCount = packed ? pCT3H->count : pFileHead->count;

        if (FlIsExecute(FH) != 0)
            return;
        if (q->bank < 0 || q->count < 1) {
            if (q->bank < 0)
                q->bank = fileBank;
            if (q->count != fileCount)
                q->count = fileCount;
            SceneEndBank(q->bank, q->count);
            ReserveBank(q->name, q->bank, q->count, q->kind, q->handle);
        }
        if (q->kind == FS_LOAD_GLBL)
            fs_alloc_global_image(q->bank, packed ? (int)pCT3H->unpacked : (int)pFileHead->size);
        if (packed) {
            ExtWrPtr = BankImage[q->bank];
            ExtBufTmpIdx = 0;
        } else {
            memcpy(BankImage[q->bank], Buf1stSector, FL_SECTOR_SIZE);
        }
        size = pFileHead->size;
        LoadNowSects = 1;
        LoadTotalSects = (int)(((size & (FL_SECTOR_SIZE - 1)) ? size + FL_SECTOR_SIZE : size) / FL_SECTOR_SIZE);
        Phase = size <= FL_SECTOR_SIZE ? FSMLT_CHECK : FSMLT_READ_REST;
        return;
    }
    case FSMLT_READ_REST:
        if (pFileHead->magic != FILE_MAGIC_CT3X) {
            if (FlReqRead(FH, BankImage[q->bank] + FL_SECTOR_SIZE, -1, LoadTotalSects - LoadNowSects) != 0) {
                fs_error(s_srcFile, FS_LINE_MLT_REST, "READ ERROR !!", "%s", OpenFile); /* data 0x019362f8 */
                Phase = FSMLT_ERROR;
                return;
            }
            Phase = FSMLT_REST_WAIT;
            return;
        }
        Phase = FSMLT_READ_PACKED;
        /* fall through */
    case FSMLT_READ_PACKED:
        sRsect = LoadTotalSects - LoadNowSects;
        if (sRsect > FS_PACKED_READ_SECTS)
            sRsect = FS_PACKED_READ_SECTS;
        if (FlReqRead(FH, ExtBuf, -1, sRsect) != 0) {
            fs_error(s_srcFile, FS_LINE_MLT_PACKED, "READ ERROR !!", "%s", OpenFile);
            Phase = FSMLT_ERROR;
            return;
        }
        Phase = FSMLT_PACKED_WAIT;
        return;
    case FSMLT_REST_WAIT:
        if (FlIsExecute(FH) == 0)
            Phase = FSMLT_CHECK;
        return;
    case FSMLT_PACKED_WAIT:
        if (FlIsExecute(FH) != 0)
            return;
        LoadNowSects += sRsect;
        if (LoadNowSects < LoadTotalSects) {
            Extract(Buf1stSector, ExtBuf, ExtBuf + FS_PACKED_READ_SECTS * FL_SECTOR_SIZE, &ExtWrPtr);
            Phase = FSMLT_READ_PACKED;
            return;
        }
        // The last read ends at the file's last byte. ROM: a size that is a whole number of sectors
        // drops the last sector here (hazards.md "AlyssaPan", 2026-11-10).
        Extract(Buf1stSector, ExtBuf,
                ExtBuf + (pFileHead->size & (FL_SECTOR_SIZE - 1)) + (sRsect - 1) * FL_SECTOR_SIZE, &ExtWrPtr);
        {
            short bank = (short)q->bank;

            memcpy(BankImage[q->bank] + offsetof(FILE_HEAD, bank), &bank, 2);
            memcpy(BankImage[q->bank] + offsetof(FILE_HEAD, count), &pCT3H->count, 2);
        }
        Phase = FSMLT_CHECK;
        return;
    case FSMLT_CHECK:
        if (!fs_mlt_setup())
            return;
        break;
    case FSMLT_CHECKED:
    case FSMLT_DONE:
        break;
    default:
        return;
    }
    // FSMLT_CHECK -> CHECKED -> DONE in one call in the ROM: the rows move up.
    memmove(&FQue[0], &FQue[1], (FQUE_MAX - 1) * sizeof(FQUE));             /* ROM: memcpy */
    FQue[FQUE_MAX - 1].name[0] = '\0';
    Phase = FSMLT_IDLE;
}

