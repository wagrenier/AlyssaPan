// game/Character_Init.h -- CT3 game/Character_Init.c: the character table's reset, the IOP's boot, and the
// memory card (McInit, mc_*, MemorycardAuto*, MemorycardUpdate). Slice 1 reconstructed 2026-11-09; the
// prototypes of what is still a stub come from the call sites.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

#include <stddef.h>

// ---- the memory card work buffer (mc_alloc) ----
// One SdAlloc block (gHeapSound, the sound driver's single slot) of MC_BUFF_ALLOC bytes, MC_BUFF_CLEAR of
// them zeroed, cut into: the master record (0x20 bytes), the header (0x80), then MC_DATA_COUNT save images
// of MC_DATA_SIZE bytes each (mc_databuff[]). The images are the card's bytes: EE layout, no pointers.
#define MC_BUFF_ALLOC 0x8391f
#define MC_BUFF_CLEAR 0x83120
#define MC_BUFF_ALIGN 0x10
#define MC_MASTER_OFFSET 0x00
#define MC_HEADER_OFFSET 0x20
#define MC_DATA_OFFSET 0xa0
#define MC_DATA_SIZE 0x10610
#define MC_DATA_COUNT 8

// The master record (mc_master, the buffer's first 0x20 bytes): XOR-of-halfwords checksums of the header
// and of each save image (mc_make_checksum).
typedef struct MC_MASTER {
    u_short headerSum;                  /* 0x00: over the header's first MC_HEADER_SUM_HALFS halfwords */
    u_short dataSum[MC_DATA_COUNT];     /* 0x02: over each whole save image */
    u_char pad12[0x20 - 0x12];
} MC_MASTER;
static_assert(sizeof(MC_MASTER) == MC_HEADER_OFFSET, "MC_MASTER");
#define MC_HEADER_SUM_HALFS 0x3c        /* 0x78 of the header's 0x80 bytes */
#define MC_DATA_SUM_HALFS (MC_DATA_SIZE / 2)

// mc_broke_data[]: set by mc_make_checksum when a checksum failed and the part was reset.
#define MC_BROKE_SYSTEM MC_DATA_COUNT   /* [0..7] the save images, [8] the header (mc_save_system_init) */
#define MC_BROKE_COUNT (MC_DATA_COUNT + 1)

// mc_load_new: the save file, read in MC_LOAD_CHUNKS chunks of MC_LOAD_CHUNK bytes and a last one of
// MC_LOAD_TAIL -- MC_BUFF_CLEAR bytes in all, the whole work buffer.
#define MC_LOAD_CHUNK 0x19000
#define MC_LOAD_CHUNKS 5
#define MC_LOAD_TAIL 0x6120
static_assert(MC_LOAD_CHUNK * MC_LOAD_CHUNKS + MC_LOAD_TAIL == MC_BUFF_CLEAR, "mc_load_new's chunks");
#define MC_OPEN_RDONLY 1                /* sceMcOpen mode (SCE_RDONLY) */

// The steps of the card state machines (mc_load_new's mc_load_new_num), and their answers.
#define MC_STEP_ERROR (-1)              /* close the file and fail */
#define MC_RESULT_OK 0
#define MC_RESULT_FAIL 1
#define MC_RESULT_BUSY 2                /* the library refused the call: wait a vblank, try the step again */

// One record of the MC_SAVE_RECORD_MAX at the start of each save image (mc_save_data_init: id = -1).
typedef struct MC_SAVE_RECORD {
    u_char pad00[0x10];
    int id;                             /* 0x10: -1 = empty */
    u_char pad14[0x1c];
} MC_SAVE_RECORD;
static_assert(sizeof(MC_SAVE_RECORD) == 0x30, "MC_SAVE_RECORD");
static_assert(offsetof(MC_SAVE_RECORD, id) == 0x10, "MC_SAVE_RECORD.id");
#define MC_SAVE_RECORD_MAX 35           /* 0x23 */
#define MC_SAVE_RECORD_EMPTY (-1)

// mc_check's retry limits: sceMcGetInfo refused more than this many times, sceMcSync failed more than
// this many times; the same card type MC_CHECK_STABLE_READS reads in a row settles it.
#define MC_CHECK_INFO_RETRIES 20
#define MC_CHECK_SYNC_RETRIES 3
#define MC_CHECK_STABLE_READS 4

// mc_check_always: polls the card once every MC_CHECK_ALWAYS_PERIOD + 1 calls (frames).
#define MC_CHECK_ALWAYS_PERIOD 10

// mcErr_ID values written here (MemorycardUpdate names the rest when it is written).
#define MC_ERR_NONE 0
#define MC_ERR_NO_CARD 2

#define MC_SLOT_COUNT 2                 /* SlotErr[] */

#define CHAR_INIT_MAX_HP_COUNT 41       /* Char_Init_MAX_HP: 164 bytes; Character_All_Init reads 35 */
extern int Char_Init_MAX_HP[CHAR_INIT_MAX_HP_COUNT]; /* data 0x01939000 */

void Character_All_Init(void);                                                           // CT3 0x018b4070
void IOPInit(char **argv);                                                               // CT3 0x018b4430
void SceneLightSet(int scene);                                                           // CT3 0x018b4480
void CharLightSet(int chara);                                                            // CT3 0x018b49f0
void MemorycardAutoLoadInsertedCheck3(void);                                             // CT3 0x018b5d00
void MemorycardUpdate(void);                                                             // CT3 0x018b6080
void mc_check_always(int clearError);                                                    // CT3 0x018b78d0
int mc_load_new(void);                                                                   // CT3 0x018b8b90
void mc_free(void);                                                                      // CT3 0x018b89b0
int mc_alloc(void);                                                                      // CT3 0x018b8a10
void mc_make_checksum(int save);                                                         // CT3 0x018ba6d0
void mc_save_data_init(u_char *data);                                                    // CT3 0x018bacb0
void mc_save_system_init(void);                                                          // CT3 0x018badd0
int mc_check3(void);                                                                     // CT3 0x018bb2d0
int mc_check(void);                                                                      // CT3 0x018bb460
int McInit(void);                                                                        // CT3 0x018bb5c0
