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

// mcErr_ID: the card menus' message (MemorycardUpdate, 2026-11-10). Named where every write agrees;
// MC_ERR_MSG_<n> where the ROM sets the same number on unlike paths (the menu TU that prints them names
// them).
#define MC_ERR_NONE 0
#define MC_ERR_CHECKING 1               /* the buffer was just taken / the card is being looked at */
#define MC_ERR_NO_CARD 2
#define MC_ERR_UNFORMATTED 3            /* the format check said unformatted twice */
#define MC_ERR_MSG_04 4                 /* format failed; auto-load: a card without the save */
#define MC_ERR_NEW_DATA 5               /* no save on the card: the images were reset for a new one */
#define MC_ERR_MSG_06 6                 /* load: the file chosen holds nothing, or the load failed */
#define MC_ERR_FORMAT_ASK 7
#define MC_ERR_MSG_08 8                 /* no room on the card, or the format failed */
#define MC_ERR_SAVE_FAILED 9
#define MC_ERR_OVERWRITE_ASK 10         /* the image chosen already holds a save */
#define MC_ERR_SAVING 0xb
#define MC_ERR_SAVED 0xc
#define MC_ERR_MSG_0E 0xe
#define MC_ERR_LOADED 0xf
#define MC_ERR_MSG_11 0x11              /* the file is not the game's (header id != MC_HEADER_ID), or no room */

// mcAccess_ID: which card flow MemorycardUpdate runs (the names are read from the flows' shape).
#define MC_ACCESS_NONE (-1)
#define MC_ACCESS_AUTO_LOAD 0           /* load, fade out, mc_load_set, TRIG_EVENT_MC_LOADED */
#define MC_ACCESS_AUTO_SAVE 1           /* load the header, then save new / over (format offered) */
#define MC_ACCESS_LOAD 3                /* the load menu: format checks, file choice, fade, load */
#define MC_ACCESS_SAVE 4                /* the save menu: format checks, file choice, save */

// The card SEs (bank "C_0000"): MC_SE_ACCESS sounds while the card works, MC_SE_DONE when it is done.
#define MC_SE_ACCESS 0x47
#define MC_SE_DONE 0x48

// Trigger events MemorycardUpdate sets or reads (named from this use only).
#define TRIG_EVENT_LOAD_SLOT2 0x0f      /* set while the save lives in slot 2 (LOAD_SLOT_NUM == 1) */
#define TRIG_EVENT_MC_LOADED 0x99       /* a save was loaded: the game picks up from it */
#define TRIG_EVENT_SAVE_ALT 0xa9        /* set: the save menu passes 1 to mc_save_new / mc_save_normal */

// The header (mc_header): word 0 is the game's id, word 1 the uid (high 28 bits) and last file (low 4).
#define MC_HEADER_ID 3
#define MC_HEADER_FILE_MASK 0xfu
#define MC_HEADER_UID_MASK 0xfffffff0u

// The header (mc_header, 0x80 bytes after the master record): the system data every save carries -- the
// extras unlocked, the events seen, the screen position, the key map and the sound / vibration options.
// mc_save_set fills it, system_data_load_set reads it back (slice 3, 2026-11-10). Card bytes: EE layout.
#define MC_SKIP_EVENT_COUNT 20          /* SkipEventFlag's bytes */
#define MC_KEY_MAP_COUNT 35             /* KEY_MAP's first 35 actions (0x23) */
typedef struct MC_HEADER {
    int id;                             /* 0x00: MC_HEADER_ID */
    u_int uidFile;                      /* 0x04: AA_UidCount << 8 | the last file (MC_HEADER_FILE_MASK) */
    u_int omake;                        /* 0x08: OMAKE_FLAG's OMAKE_1 / OMAKE_2 */
    u_char skipEvent[MC_SKIP_EVENT_COUNT]; /* 0x0c: SkipEventFlag, OR-ed back in on a load */
    u_long display;                     /* 0x20: ga_disp.display (the screen position) */
    u_short keyMap[MC_KEY_MAP_COUNT];   /* 0x28: KEY_MAP[0..34] */
    u_char stereo;                      /* 0x6e: SOUND_STEREO_MONAURAL_BUFF */
    u_char bgmVolume;                   /* 0x6f: BGM_MASTER_VOLUME_BUFF */
    u_char seVolume;                    /* 0x70: SE_MASTER_VOLUME_BUFF */
    u_char vibration;                   /* 0x71: fVibration */
    u_char pad72[0x80 - 0x72];
} MC_HEADER;
static_assert(sizeof(MC_HEADER) == 0x80, "MC_HEADER");
static_assert(offsetof(MC_HEADER, skipEvent) == 0x0c, "MC_HEADER.skipEvent");
static_assert(offsetof(MC_HEADER, display) == 0x20, "MC_HEADER.display");
static_assert(offsetof(MC_HEADER, keyMap) == 0x28, "MC_HEADER.keyMap");
static_assert(offsetof(MC_HEADER, stereo) == 0x6e, "MC_HEADER.stereo");
static_assert(offsetof(MC_HEADER, vibration) == 0x71, "MC_HEADER.vibration");
#define MC_HEADER_UID_SHIFT 8           /* mc_save_set: uid = AA_UidCount << 8 (the ROM's shift) */

// KEY_MAP[19..20] take KEY_MAP[54..55] after a load (system_data_load_set; read from the disassembly,
// the meaning of the pairs is the key-config TU's).
#define KEY_MAP_LOAD_DST 0x13
#define KEY_MAP_LOAD_SRC 0x36

// system_data_load_set's modes.
#define SYSTEM_LOAD_FILE_ID 0           /* only mcFile_ID from the header */
#define SYSTEM_LOAD_ALL 1               /* the options, key map, screen position, then the unlocks */
#define SYSTEM_LOAD_UNLOCKS 2           /* only OMAKE_1 and the skip-event flags (OR-ed in) */

// Trigger events and variables the system data drives (main.c's system_init sets the same numbers).
#define TRIG_EVENT_OMAKE_1 0xa7         /* OMAKE_FLAG's OMAKE_1 (set by the game-end save) */
#define TRIG_EVENT_MONAURAL 0xa8        /* registered while SOUND_STEREO_MONAURAL_BUFF != 0 */
#define TRIG_EVENT_OMAKE_2 0xc5         /* OMAKE_FLAG != 0 after a full load */
#define TRIG_VAR_BGM_VOLUME 0x4e
#define TRIG_VAR_SE_VOLUME 0x4f

// A save image's word at +0xc710: non-zero once the image holds a save (the save flow asks before
// writing over it). Named from MemorycardUpdate's use only.
#define MC_DATA_USED 0xc710
#define MC_DATA_C714 0xc714             /* a word the game-end save clears (unread so far) */
#define MC_DATA_EVENTS 0xc72c           /* the image's event bits (McRegisterEvent: bit e & 7 of byte e >> 3) */
#define MC_EVENT_GAME_END 0x9d          /* McRegisterEvent'd by the game-end save */

// mc_save_set(alt != 0): the game-end save's image, read from the disc into gHeapSound.
#define MC_GAME_END_SIZE 0x10e0f
#define MC_GAME_END_ALIGN 0x10
#define MC_OPEN_WRITE 2                 /* sceMcOpen mode (SCE_WRONLY) */
#define MC_OPEN_CREATE 0x203            /* sceMcOpen mode (SCE_CREAT | SCE_RDWR) */

// mc_save_new: the icon files copied from data/mc_icon/ (view, copy, delete), each read into gHeapJunc.
#define MC_ICON_FILES 3
#define MC_ICON_PAD 0x7ff               /* added to the file's length for the allocation */
#define MC_ICON_ALIGN 0x40
#define MC_ICON_TRANS_RATE 0x60         /* icon.sys TransRate */

// mc_save_new's steps (mc_save_new_num; the ROM's numbers, 3, 4 and 8 unused).
#define MC_NEW_CHDIR 0
#define MC_NEW_MKDIR 1
#define MC_NEW_ICONS 2
#define MC_NEW_LOAD_ICON 5
#define MC_NEW_OPEN_ICON 6
#define MC_NEW_WRITE_ICON 7
#define MC_NEW_CLOSE_ICON 9
#define MC_NEW_OPEN_SAVE 10
#define MC_NEW_WRITE_SAVE 11
#define MC_NEW_WRITE_TAIL 12
#define MC_NEW_CLOSE_SAVE 13
#define MC_NEW_OPEN_ICONSYS 0x14
#define MC_NEW_BUILD_ICONSYS 0x15
#define MC_NEW_WRITE_ICONSYS 0x16
#define MC_NEW_CLOSE_ICONSYS 0x17

extern int mc_bgcolor[4][4];            /* data 0x019399f0: icon.sys BgColor */
extern float mc_lightdir[3][4];         /* data 0x01939a30 */
extern float mc_lightcol[3][4];         /* data 0x01939a60 */
extern float mc_ambient[4];             /* data 0x01939a90 */

extern u_char continue_backup[MC_DATA_SIZE]; /* bss 0x01a31450: the image ContinueBackupSet keeps */

// mc_check4's answer: sceMcGetInfo's refusal - 200, or sceMcSync's result. MemorycardUpdate treats
// -1..-80 as the card gone (sceMcRes*), anything else as go on.
#define MC_CHECK4_CARD_ERROR(r) ((r) < 0 && (r) > -0x51)

// mc_formatcheck2's answers.
#define MC_FORMAT_NO 0
#define MC_FORMAT_YES 1
#define MC_FORMAT_BUSY 2                /* sceMcGetInfo refused: ask again next frame */

// The vblanks MemorycardUpdate waits between card checks (AA_count).
#define MC_RETRY_WAIT 5
#define MC_FORMAT_WAIT 3

// fream_counter's preset after a load: mc_check_always polls at once.
#define MC_FRAME_COUNTER_NOW 1000

#define MC_FILE_NONE (-1)               /* mc_cursor[]: no file chosen in that slot */
#define MC_INFO_COUNT 8                 /* mc_Getinfo_count's files */

#define MC_SLOT_COUNT 2                 /* SlotErr[] */
#define MC_SLOT_ERR_NO_CARD 1           /* mc_check3 found no card */
#define MC_SLOT_ERR_UNFORMATTED 4       /* mc_formatcheck said unformatted */

#define CHAR_INIT_MAX_HP_COUNT 41       /* Char_Init_MAX_HP: 164 bytes; Character_All_Init reads 35 */
extern int Char_Init_MAX_HP[CHAR_INIT_MAX_HP_COUNT]; /* data 0x01939000 */

void Character_All_Init(void);                                                           // CT3 0x018b4070
void IOPInit(char **argv);                                                               // CT3 0x018b4430
void SceneLightSet(int scene);                                                           // CT3 0x018b4480
void CharLightSet(int chara);                                                            // CT3 0x018b49f0
int MemorycardAutoLoadFormatCheck(void);                                                 // CT3 0x018b5cc0
void MemorycardAutoLoadInsertedCheck3(void);                                             // CT3 0x018b5d00
void MemorycardAutoLoadInsertedCheck2(void);                                             // CT3 0x018b5d40
void ContinueBackupSet(void);                                                            // CT3 0x018b5e70
void mc_save_set(int alt);                                                               // CT3 0x018b9a80
void McRegisterEvent(u_int event);                                                       // CT3 0x018b9e90
void data_save_set(u_char *data);                                                        // CT3 0x018b9ee0
int mc_check2(void);                                                                     // CT3 0x018bb350
int mc_Getinfo_count(int file);                                                          // CT3 0x018b6020
void MemorycardUpdate(void);                                                             // CT3 0x018b6080
void mc_check_always2(int clearError);                                                   // CT3 0x018b7870
void mc_check_always(int clearError);                                                    // CT3 0x018b78d0
void mc_load_set(void);                                                                  // CT3 0x018b7930
void system_data_load_set(int mode);                                                     // CT3 0x018b8680
int mc_save_new(int alt);                                                                // CT3 0x018b8eb0
int mc_save_normal(int alt);                                                             // CT3 0x018b9790
int mc_free_area(void);                                                                  // CT3 0x018baac0
void mc_save_init(void);                                                                 // CT3 0x018bac60
int mc_formatcheck2(void);                                                               // CT3 0x018bb070
int mc_formatcheck(void);                                                                // CT3 0x018bb150
int mc_check4(void);                                                                     // CT3 0x018bb280
int mc_load_new(void);                                                                   // CT3 0x018b8b90
void mc_free(void);                                                                      // CT3 0x018b89b0
int mc_alloc(void);                                                                      // CT3 0x018b8a10
void mc_make_checksum(int save);                                                         // CT3 0x018ba6d0
void mc_save_data_init(u_char *data);                                                    // CT3 0x018bacb0
void mc_save_system_init(void);                                                          // CT3 0x018badd0
int mc_check3(void);                                                                     // CT3 0x018bb2d0
int mc_check(void);                                                                      // CT3 0x018bb460
int McInit(void);                                                                        // CT3 0x018bb5c0
