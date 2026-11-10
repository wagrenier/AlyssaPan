// game/Character_Init.c -- Clock Tower 3: the character table's reset, the IOP's boot, and the memory
// card (CT3 0x018b3a60..0x018bb610, the symtab's local chunk 105). Slice 1 reconstructed 2026-11-09:
// Character_All_Init, IOPInit, McInit, the work buffer (mc_alloc / mc_free / mc_save_data_init) and the
// card-present checks (mc_check, mc_check_always, MemorycardAutoLoadInsertedCheck3).
//
// The file name is the tool's (the first keyed function's); the memory card code that fills most of the
// object (MemorycardUpdate, mc_load_new, mc_save_*, the icon "data/mc_icon/", "/BASLUS-20633") is still to
// come, so a later slice may rename it.

#include "common/alyssapan_stub.h"
#include "game/Character_Init.h"

#include "ga/gaPadInit.h"
#include "ga/gaInitInterrupt.h"
#include "game/ArtoolDebugOpen.h"
#include "game/Clipping_Portal.h"
#include "game/ssGrpPortalCheck.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ahalloc.h"
#include "game/fileio.h"
#include "game/movie.h"
#include "game/sound.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "sdk/libgraph.h"
#include "sdk/libmc.h"

#include <stdio.h>
#include <string.h>

// ---- data ----

// Char_Init_MAX_HP (.data 0x01939000, 164 bytes): each Character[] slot's starting hit points. The ROM's
// values; the 41 words outlast the 35 slots (the rest are zero).
int Char_Init_MAX_HP[CHAR_INIT_MAX_HP_COUNT] = {
    800, 200, 500, 400, 500, 600, 700, 800, 300, 300, 300,
};

// The statics, as the symtab places them (.sbss 0x0193ef78..0x0193f0bc).
static int alloc_flag;                  /* 0x0193ef80: mc_alloc's buffer is held */
static int fream_counter;               /* 0x0193ef84: mc_check_always's frame count (sic) */
static int SlotErr[MC_SLOT_COUNT];      /* 0x0193efb0: per slot, set when mc_check3 finds no card */
static u_char *mc_databuff[MC_DATA_COUNT]; /* 0x0193f080: the save images in mc_data_buff */
static u_char *mc_header;               /* 0x0193f0a0 */
static u_char *mc_master;               /* 0x0193f0b0 */
static u_char *mc_data_buff;            /* 0x0193f0b4: the SdAlloc block */
static int mc_save_normal_num;          /* 0x0193ef90: mc_save_normal's step (mc_load_new bumps it too) */
static int mc_load_new_fd;              /* 0x0193efa4 */
static int mc_load_new_num;             /* 0x0193efa8: mc_load_new's step */
static int mc_load_new_end_size;        /* 0x0193efc4 */
static int mc_load_new_loop_count;      /* 0x0193efc8 */
static int mc_load_new_loop_count_max;  /* 0x0193efcc */
static int mc_broke_data[MC_BROKE_COUNT]; /* 0x0193f050 */
static int AA_New_McAlloc_Mode;         /* 0x0193ef74: != 0 keeps the buffer between flows (nothing sets it yet) */
static int mc_menuid;                   /* 0x0193ef7c: a flow's sub-step */
static int AA_Phase_Step;               /* 0x0193ef88: the menus' format-check / save sub-step */
static int AA_UnFormatCk;               /* AA_UnFormatCk$1304, 0x0193efc0: the first format check said unformatted */
static int mc_type;                     /* 0x0193f0b8: 0 = the card holds the save, 1 = a new one is made */

// .data (0x01939aa0..0x01939b00), with the ROM's initial values.
static int AA_count = 1;                /* 0x01939aa0: vblanks left before the next card check */
static int mc_cursor[MC_SLOT_COUNT] = {MC_FILE_NONE, MC_FILE_NONE}; /* 0x01939af0: the file last used per slot */
static u_int mc_uidwork[MC_SLOT_COUNT]; /* 0x01939af8: that file's header uid per slot */

// @2405 (.rodata 0x01935150): the card SEs' bank. @2406 (0x01935160): MemorycardUpdate's debug line.
static const char s_seBank[] = "C_0000";
static const char s_check4Format[] = "%d=mc_check4()\n";

// mc_fname (.data 0x01939ab0, 64 bytes): the save's directory and file name.
static char mc_fname[0x40] = "/BASLUS-20633";

// @2706 (.rodata 0x01935270): SdAlloc's name for the block. @2707 (0x01935280): its failure message.
static const char s_mcAllocName[] = "MC";
static const char s_mcAllocError[] = "mc_alloc: SdAlloc error !!\n";
// @2749 (.rodata 0x019352a0): mc_load_new's path format.
static const char s_pathFormat[] = "%s%s";

// The VU's vf0, (0, 0, 0, 1.0f): what sqc2 $vf0 stores.
static const qword kVf0 = {0, 0, 0, 0x3f800000};

// ---- the character table ----

// CT3 0x018b4070: every Character[] slot back to free -- ids and indices -1, flags and counters zero,
// positions at the origin, hit points from Char_Init_MAX_HP -- and its junction block (if any) returned
// to gHeapJunc.
void Character_All_Init(void)
{
    int i;
    int j;
    int k;

    for (i = 0; i < CHARACTER_MAX; i++) {
        CHARACTER *chr = &Character[i];

        chr->id = -1;
        chr->bank = -1;
        for (j = 0; j < 10; j++) {
            chr->slot004[j] = -1;
            for (k = 0; k < 10; k++) {
                chr->slot022[j][k] = 0;
            }
        }
        for (j = 0; j < 10; j++) {
            chr->slot018[j] = 0;
        }
        chr->flags = 0;
        chr->w090 = 0;
        chr->w098 = 0;
        chr->lightBank = -1;
        memcpy(chr->posBefore, kVf0, sizeof(qword));
        memcpy(chr->pos, kVf0, sizeof(qword));
        chr->rotBefore = 0;
        chr->rot = 0;
        chr->w300[0] = 0;
        chr->w300[1] = 0;
        chr->w300[2] = 0;
        chr->w300[3] = 0;
        chr->b2f1 = 1;
        chr->b2f2 = 0;
        asInitPlayerBehaviorControler(chr);
        asShadowDrawing_Init(chr->bank);
        CharacterSortBuff[i] = -1;
        chr->hp = Char_Init_MAX_HP[i];
        chr->hpMax = Char_Init_MAX_HP[i];
        chr->w0fc = 0;
        chr->h100 = -1;
        chr->b102 = -1;
        chr->translucent = 0;
        chr->translucentAlpha = 0.0f;   /* the ROM stores the word 0 */
        chr->b534 = 0;
        for (j = 0; j < 6; j++) {
            chr->h536[j] = 0;
        }
        if (chr->junc != NULL) {
            _SunFree(&gHeapJunc, &chr->junc->buf);
            _SunFree(&gHeapJunc, (void **)&chr->junc);
        }
    }
}

// ---- the IOP ----

// CT3 0x018b4430: reboot the IOP with the game's modules (argv[0] goes to IopLoadModules), then bring up
// the pads, the memory card, the file and sound servers and the movie player's IOP side.
void IOPInit(char **argv)
{
    IopLoadModules(argv[0]);
    gaPadInit();
    McInit();
    FlInit();
    FSInit();
    SSInit();
    AA_MovieIOPInit();
}

// ---- the memory card ----

// CT3 0x018bb5c0: sceMcInit once; answers its result (0 when done before).
int McInit(void)
{
    static int initflag;                /* initflag$64, .sbss 0x0193ef78 */
    int ret = 0;

    if (initflag == 0) {
        ret = sceMcInit();
        if (ret == 0) {
            initflag = 1;
        }
    }
    return ret;
}

// CT3 0x018b89b0: give the work buffer back to the sound heap and forget its pieces.
void mc_free(void)
{
    int i;

    if (alloc_flag != 0) {
        SdFree((void **)&mc_data_buff);
        mc_data_buff = NULL;
        mc_master = NULL;
        mc_header = NULL;
        for (i = 0; i < MC_DATA_COUNT; i++) {
            mc_databuff[i] = NULL;
        }
        alloc_flag = 0;
    }
}

// CT3 0x018b8a10: once every file, SE and SQ load is done, borrow the sound heap's slot for the work
// buffer, zero it and cut it up. PORT: the ROM answers the block's address on success (non-zero) and
// printf's count on failure; the host answers 1 on success.
int mc_alloc(void)
{
    int ret;
    int i;

    FSMltLoadEndWaitAll();
    SdLoadSEEndWaitAll();
    SdLoadSQEndWaitAll();
    mc_data_buff = (u_char *)SdAlloc(MC_BUFF_ALLOC, MC_BUFF_ALIGN, s_mcAllocName);
    if (mc_data_buff == NULL) {
        ret = printf(s_mcAllocError);
    } else {
        for (i = 0; i < MC_BUFF_CLEAR; i++) {
            mc_data_buff[i] = 0;
        }
        alloc_flag = 1;
        mc_master = mc_data_buff + MC_MASTER_OFFSET;
        mc_header = mc_data_buff + MC_HEADER_OFFSET;
        for (i = 0; i < MC_DATA_COUNT; i++) {
            mc_databuff[i] = mc_data_buff + MC_DATA_OFFSET + i * MC_DATA_SIZE;
        }
        ret = 1;
    }
    return ret;
}

// CT3 0x018bacb0: zero one save image (data), then mark every record of every image in mc_databuff empty.
// The ROM clears data but marks the eight images, not data alone: kept.
void mc_save_data_init(u_char *data)
{
    int i;
    int n;

    for (i = 0; i < MC_DATA_SIZE; i++) {
        data[i] = 0;
    }
    for (i = 0; i < MC_DATA_COUNT; i++) {
        MC_SAVE_RECORD *rec = (MC_SAVE_RECORD *)mc_databuff[i];

        for (n = 0; n < MC_SAVE_RECORD_MAX; n++) {
            rec[n].id = MC_SAVE_RECORD_EMPTY;
        }
    }
}

// CT3 0x018bb460: ask the card in slot mcSlot_ID its type until four answers in a row agree (or the
// library gives up), one vblank between asks. Answers 0 for a PS2 card, 1 for anything else (none, PS1,
// PDA, or a card that never answered).
int mc_check(void)
{
    int type = 0;
    int result;
    int tries;
    int fails = 0;
    int same = 0;
    int seen[MC_CHECK_STABLE_READS];

    for (;;) {
        tries = 0;
        while (sceMcGetInfo(mcSlot_ID, 0, &type, NULL, NULL) != 0) {
            if (tries > MC_CHECK_INFO_RETRIES) {
                type = 0;
                goto settled;
            }
            tries++;
            fails = 0;
            same = 0;
            sceGsSyncV(0);
        }
        sceMcSync(0, NULL, &result);
        if (result < 0) {
            type = 0;
            if (fails > MC_CHECK_SYNC_RETRIES) {
                goto settled;
            }
            fails++;
            same = 0;
        } else {
            /* The ROM's order: a mismatch restarts the count at 1 without moving the new answer to
               seen[0], so the next answer is compared with the old first one. Kept. */
            seen[same] = type;
            if (same > 0) {
                if (seen[same - 1] == seen[same]) {
                    if (same == MC_CHECK_STABLE_READS - 1) {
                        goto settled;
                    }
                } else {
                    fails = 0;
                    same = 0;
                }
            }
            same++;
        }
        sceGsSyncV(0);
    }

settled:
    /* The ROM tests PDA, then PS1, then PS2: only a PS2 card answers 0. */
    return type == sceMcTypePS2 ? 0 : 1;
}

// CT3 0x018b78d0: every eleventh call, check the card: none -> MC_ERR_NO_CARD and back to the card menu's
// first state (mcMain_ID 0); present -> the error is cleared when clearError says so.
void mc_check_always(int clearError)
{
    fream_counter++;
    if (fream_counter > MC_CHECK_ALWAYS_PERIOD) {
        fream_counter = 0;
        if (mc_check() == 0) {
            if (clearError != 0) {
                mcErr_ID = MC_ERR_NONE;
            }
        } else {
            mcErr_ID = MC_ERR_NO_CARD;
            mcMain_ID = 0;
        }
    }
}

// CT3 0x018b5d00: the auto-load's third insertion check: mark the slot when mc_check3 finds no card.
void MemorycardAutoLoadInsertedCheck3(void)
{
    if (mc_check3() != 0) {
        SlotErr[mcSlot_ID] = MC_SLOT_ERR_NO_CARD;
    }
}

// CT3 0x018b5cc0: the auto-load's format check: mark the slot when the card is unformatted. Answers
// mc_formatcheck's answer XOR 1 (0 for a formatted card, as the ROM computes it).
int MemorycardAutoLoadFormatCheck(void)
{
    int formatted = mc_formatcheck();

    if (formatted == 0) {
        SlotErr[mcSlot_ID] = MC_SLOT_ERR_UNFORMATTED;
    }
    return formatted ^ 1;
}

// CT3 0x018bac60: reset the header (mc_save_system_init) and every save image.
void mc_save_init(void)
{
    int i;

    mc_save_system_init();
    for (i = 0; i < MC_DATA_COUNT; i++) {
        mc_save_data_init(mc_databuff[i]);
    }
}

// CT3 0x018bb150: ask the card's format flag until four answers in a row agree (mc_check's loop, on
// sceMcGetInfo's format word). Answers the flag, or 0 when the library gives up.
int mc_formatcheck(void)
{
    int format = 0;
    int result;
    int tries;
    int fails = 0;
    int same = 0;
    int seen[MC_CHECK_STABLE_READS];

    for (;;) {
        tries = 0;
        while (sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, &format) != 0) {
            if (tries > MC_CHECK_INFO_RETRIES) {
                return 0;
            }
            tries++;
            fails = 0;
            same = 0;
            sceGsSyncV(0);
        }
        sceMcSync(0, NULL, &result);
        if (result < 0) {
            format = 0;
            if (fails > MC_CHECK_SYNC_RETRIES) {
                return 0;
            }
            fails++;
            same = 0;
        } else {
            /* mc_check's order (a mismatch restarts the count at 1), kept. */
            seen[same] = format;
            if (same > 0) {
                if (seen[same - 1] == seen[same]) {
                    if (same == MC_CHECK_STABLE_READS - 1) {
                        return format;
                    }
                } else {
                    fails = 0;
                    same = 0;
                }
            }
            same++;
        }
        sceGsSyncV(0);
    }
}

// CT3 0x018bb070: one quick format check, asked twice: MC_FORMAT_YES / MC_FORMAT_NO, MC_FORMAT_BUSY when
// the library refused. The ROM answers YES after a first "formatted" whatever the second ask says.
int mc_formatcheck2(void)
{
    int format;
    int result;

    if (sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, &format) != 0) {
        return MC_FORMAT_BUSY;
    }
    sceMcSync(0, NULL, &result);
    if (format == 0) {
        if (sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, &format) != 0) {
            return MC_FORMAT_BUSY;
        }
        sceMcSync(0, NULL, &result);
        return format != 0 ? MC_FORMAT_BUSY : MC_FORMAT_NO;
    }
    if (sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, &format) != 0) {
        return MC_FORMAT_BUSY;
    }
    sceMcSync(0, NULL, &result);
    return MC_FORMAT_YES;
}

// CT3 0x018bb280: the card's state in one call: sceMcGetInfo's refusal - 200, or sceMcSync's result.
int mc_check4(void)
{
    int result;
    int ret = sceMcGetInfo(mcSlot_ID, 0, NULL, NULL, NULL);

    if (ret < 0) {
        result = ret - 200;
    } else {
        sceMcSync(0, NULL, &result);
    }
    return result;
}

// The XOR of count little-endian halfwords from p (mc_make_checksum's sum, written out once per use in
// the ROM). The buffers keep the EE's layout and byte order, which the host shares.
static u_short mc_xor_halfs(const u_char *p, int count)
{
    const u_short *h = (const u_short *)p;
    u_short sum = 0;
    int i;

    for (i = 0; i < count; i++) {
        sum ^= h[i];
    }
    return sum;
}

// CT3 0x018ba6d0: the master record's checksums. save != 0: (before writing) store the header's sum, and
// every image's except the one being saved (mcFile_ID) is checked as on a load. save == 0: (after
// reading) check them all; a part whose sum is wrong is reset (the header by mc_save_system_init, an
// image by mc_save_data_init), its sum stored again and mc_broke_data says which.
void mc_make_checksum(int save)
{
    MC_MASTER *master;
    u_char *base;
    u_short sum;
    int i;

    for (i = 0; i < MC_BROKE_COUNT; i++) {
        mc_broke_data[i] = 0;
    }
    master = (MC_MASTER *)mc_master;
    base = mc_master;
    sum = mc_xor_halfs(base + MC_HEADER_OFFSET, MC_HEADER_SUM_HALFS);
    if (save == 0) {
        if (master->headerSum != sum) {
            mc_save_system_init();
            master->headerSum = mc_xor_halfs(base + MC_HEADER_OFFSET, MC_HEADER_SUM_HALFS);
            mc_broke_data[MC_BROKE_SYSTEM] = 1;
        }
    } else {
        master->headerSum = sum;
    }
    for (i = 0; i < MC_DATA_COUNT; i++) {
        u_char *data = base + MC_DATA_OFFSET + i * MC_DATA_SIZE;

        sum = mc_xor_halfs(data, MC_DATA_SUM_HALFS);
        if (save == 0 || mcFile_ID != i) {
            if (master->dataSum[i] != sum) {
                mc_save_data_init(mc_databuff[i]);
                master->dataSum[i] = mc_xor_halfs(data, MC_DATA_SUM_HALFS);
                mc_broke_data[i] = 1;
            }
        } else {
            master->dataSum[i] = sum;
        }
    }
}

// CT3 0x018b8b90: read the save file (mc_fname twice: "/BASLUS-20633/BASLUS-20633") from the card in
// slot mcSlot_ID into the whole work buffer, then check it (mc_make_checksum(0)). Runs its steps until
// done, one vblank after each refused call; answers MC_RESULT_OK or MC_RESULT_FAIL. The steps fall
// through as the ROM's do: a read refused mid-file resumes at the chunk it stopped on.
int mc_load_new(void)
{
    char path[0x40c];
    int result;
    int ret = MC_RESULT_BUSY;

    mc_load_new_fd = 0;
    mc_load_new_num = 0;
    for (;;) {
        switch (mc_load_new_num) {
        case 0:
            sprintf(path, s_pathFormat, mc_fname, mc_fname);
            ret = MC_RESULT_BUSY;
            if (sceMcOpen(mcSlot_ID, 0, path, MC_OPEN_RDONLY) != 0) {
                break;
            }
            sceMcSync(0, NULL, &mc_load_new_fd);
            if (mc_load_new_fd < 0) {
                ret = MC_RESULT_FAIL;
                mc_load_new_num = MC_STEP_ERROR;
                break;
            }
            mc_load_new_num++;
            /* fall through */
        case 1:
            ret = MC_RESULT_BUSY;
            if (sceMcSeek(mc_load_new_fd, 0, 0) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            if (result < 0) {
                mc_load_new_num = MC_STEP_ERROR;
                break;
            }
            mc_load_new_loop_count = 0;
            mc_load_new_loop_count_max = MC_LOAD_CHUNKS;
            mc_load_new_end_size = MC_LOAD_TAIL;
            mc_load_new_num++;
            /* fall through */
        case 2:
            ret = MC_RESULT_OK;
            for (; mc_load_new_loop_count != mc_load_new_loop_count_max; mc_load_new_loop_count++) {
                if (sceMcRead(mc_load_new_fd, mc_data_buff + mc_load_new_loop_count * MC_LOAD_CHUNK,
                              MC_LOAD_CHUNK) != 0) {
                    ret = MC_RESULT_BUSY;
                    break;
                }
                sceMcSync(0, NULL, &result);
                if (result < 0) {
                    ret = MC_RESULT_BUSY;
                    mc_load_new_num = MC_STEP_ERROR;
                    break;
                }
            }
            if (ret != MC_RESULT_OK) {
                break;
            }
            /* The ROM bumps mc_save_normal_num here and after the tail read, not mc_load_new_num (the
               step stays 2): a slip copied from mc_save_normal, kept. */
            mc_save_normal_num++;
            /* fall through */
        case 3:
            ret = MC_RESULT_BUSY;
            if (sceMcRead(mc_load_new_fd, mc_data_buff + mc_load_new_loop_count * MC_LOAD_CHUNK,
                          mc_load_new_end_size) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            mc_save_normal_num++;
            if (result < 0) {
                mc_load_new_num = MC_STEP_ERROR;
                break;
            }
            /* fall through */
        case 4:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_load_new_fd) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;
            if (result >= 0) {
                mc_load_new_num = 0;
                ret = MC_RESULT_OK;
                mc_make_checksum(0);
            }
            break;
        case MC_STEP_ERROR:
            ret = MC_RESULT_BUSY;
            if (sceMcClose(mc_load_new_fd) != 0) {
                break;
            }
            sceMcSync(0, NULL, &result);
            ret = MC_RESULT_FAIL;       /* the ROM tests the result and fails either way */
            break;
        }
        if (ret == MC_RESULT_FAIL || ret == MC_RESULT_OK) {
            mc_load_new_num = 0;
            return ret;
        }
        sceGsSyncV(0);
    }
}

// ---- MemorycardUpdate: the card flows, one step per frame ----
//
// SE arguments: the ROM leaves a2 unset on some SSStopSE / SSPlaySE(MC_SE_ACCESS) calls; where the
// register can be traced it holds 1 (the value just compared or stored), so every call passes 1.

// The save menu's answer to mc_save_new / mc_save_normal (inlined four times in the ROM).
static void mc_save_menu_result(int ret)
{
    if (ret == MC_RESULT_FAIL) {
        mcErr_ID = MC_ERR_SAVE_FAILED;
        mcMain_ID = 5;
        SSStopSE(s_seBank, MC_SE_ACCESS, 1);
    } else if (ret == MC_RESULT_OK) {
        mcErr_ID = MC_ERR_NONE;
        mcMain_ID = 5;
        SSStopSE(s_seBank, MC_SE_ACCESS, 1);
        SSPlaySE(s_seBank, MC_SE_DONE, 1);
    }
}

// The slot the save now lives in: LOAD_SLOT_NUM and TRIG_EVENT_LOAD_SLOT2 (inlined twice in the ROM).
static void mc_set_load_slot(void)
{
    if (mcSlot_ID == 0) {
        ssTrigRemoveEvent(TRIG_EVENT_LOAD_SLOT2, 1, 0);
    } else {
        ssTrigRegisterEvent(TRIG_EVENT_LOAD_SLOT2, 1, 0);
    }
    LOAD_SLOT_NUM = mcSlot_ID != 0;
}

// The file to point at after a load: the one last used in this slot when the card's uid still matches,
// else the header's last file.
static int mc_pick_file(void)
{
    const u_int *header = (const u_int *)mc_header;

    if (mc_cursor[mcSlot_ID] == MC_FILE_NONE ||
        (header[1] & MC_HEADER_UID_MASK) != mc_uidwork[mcSlot_ID]) {
        return (int)(header[1] & MC_HEADER_FILE_MASK);
    }
    return mc_cursor[mcSlot_ID];
}

// MC_ACCESS_SAVE (mcAccess_ID 4).
static void mc_update_save_menu(void)
{
    int ret;
    int state;
    int result;

    switch (mcMain_ID) {
    case 0:
        mc_menuid = 0;
        AA_Phase_Step = 0;
        if (AA_New_McAlloc_Mode == 0 && alloc_flag != 0) {
            mc_free();
        }
        break;
    case 1:
        if (alloc_flag == 0) {
            if (AA_New_McAlloc_Mode == 0) {
                mc_alloc();
            }
            mcErr_ID = MC_ERR_CHECKING;
        } else if (mc_check() == 0) {
            mcErr_ID = MC_ERR_NONE;
            mcMain_ID = 5;
        } else {
            mcErr_ID = MC_ERR_NO_CARD;
            mcMain_ID = 2;
            AA_count = MC_RETRY_WAIT;
            mc_cursor[mcSlot_ID] = MC_FILE_NONE;
        }
        break;
    case 2:
        if (AA_count < 0) {
            AA_count = MC_RETRY_WAIT;
            if (mc_check() == 0) {
                mcErr_ID = MC_ERR_NONE;
                mc_menuid = 1;
                mcMain_ID = 3;
            }
        } else {
            AA_count--;
        }
        break;
    case 4:
        switch (AA_Phase_Step) {
        case 0:
            AA_Phase_Step = 0;
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 3;
            }
            break;
        case 1:
            ret = mc_formatcheck2();
            state = mc_check4();
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_FORMAT_YES) {
                AA_Phase_Step = 2;
                AA_count = MC_FORMAT_WAIT;
                AA_UnFormatCk = 0;
            } else if (ret == MC_FORMAT_NO) {
                if (mc_check() == 0) {
                    AA_UnFormatCk = 1;
                    AA_Phase_Step = 2;
                    AA_count = MC_FORMAT_WAIT;
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
            break;
        case 2:
            if (AA_count < 0) {
                AA_Phase_Step = 3;
            } else {
                AA_count--;
            }
            break;
        case 3:
            ret = mc_formatcheck2();
            state = mc_check4();
            printf(s_check4Format, state);
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_FORMAT_YES) {
                AA_Phase_Step = AA_UnFormatCk == 0 ? 4 : 1;
            } else if (ret == MC_FORMAT_NO) {
                if (mc_check() == 0) {
                    if (AA_UnFormatCk == 1) {
                        mcErr_ID = MC_ERR_UNFORMATTED;
                        mcMain_ID = 5;
                    } else {
                        AA_Phase_Step = 1;
                    }
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
            break;
        case 4:
            mc_free();
            mc_alloc();
            ret = mc_load_new();
            state = mc_check4();
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    system_data_load_set(0);
                    mcFile_ID = mc_pick_file();
                    if (mcFile_ID < 0) {
                        mcFile_ID = 0;
                    }
                    mcErr_ID = MC_ERR_NONE;
                    fream_counter = MC_FRAME_COUNTER_NOW;
                    mc_type = 0;
                    mcMain_ID = 5;
                } else {
                    mc_save_init();
                    mcFile_ID = 0;
                    mcErr_ID = MC_ERR_NONE;
                    mc_type = 0;
                    mcMain_ID = 5;
                }
            } else if (ret == MC_RESULT_FAIL) {
                if (mc_check() == 0) {
                    mc_save_init();
                    mcFile_ID = 0;
                    AA_Phase_Step = 5;
                    fream_counter = MC_FRAME_COUNTER_NOW;
                    mc_type = 1;
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 5;
                }
            }
            break;
        case 5:
            ret = mc_free_area();
            state = mc_check4();
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == 1) {
                mcErr_ID = MC_ERR_MSG_08;
                mcMain_ID = 5;
            } else if (ret == 0) {
                mc_type = 1;
                mcFile_ID = 0;
                fream_counter = MC_FRAME_COUNTER_NOW;
                mcErr_ID = MC_ERR_NONE;
                mcMain_ID = 5;
            }
            break;
        }
        break;
    case 5:
    case 9:
        mc_check_always2(0);
        AA_Phase_Step = 0;
        break;
    case 7:
        if (AA_Phase_Step == 1) {
            if (sceMcFormat(mcSlot_ID, 0) >= 0 && sceMcSync(0, NULL, &result) != 0) {
                if (result == 0) {
                    mc_cursor[mcSlot_ID] = 0;
                    mc_uidwork[mcSlot_ID] = 0;
                    mc_save_init();
                    mcFile_ID = 0;
                    mc_type = 1;
                    mcErr_ID = MC_ERR_NONE;
                    mcMain_ID = 5;
                    AA_Phase_Step = 0;
                } else {
                    AA_Phase_Step = 0;
                    mcErr_ID = MC_ERR_MSG_04;
                    mcMain_ID = 5;
                }
            }
        } else if (AA_Phase_Step == 0) {
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
            } else {
                AA_Phase_Step = 0;
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            }
        }
        break;
    case 8:
        if (AA_Phase_Step == 1) {
            int alt;

            mc_set_load_slot();
            mc_cursor[mcSlot_ID] = mcFile_ID;
            mc_uidwork[mcSlot_ID] = 0;
            alt = ssTrigCheckEvent(TRIG_EVENT_SAVE_ALT, 1, 0) != 0;
            if (mc_type == 1) {
                mc_free();
                mc_alloc();
                system_data_load_set(2);
                ret = mc_save_new(alt);
            } else {
                system_data_load_set(2);
                ret = mc_save_normal(alt);
            }
            if (ret != MC_RESULT_BUSY) {
                mc_save_menu_result(ret);
            }
        } else if (AA_Phase_Step == 0) {
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
                SSPlaySE(s_seBank, MC_SE_ACCESS, 1);
            } else {
                AA_Phase_Step = 0;
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            }
        }
        break;
    }
}

// MC_ACCESS_LOAD (mcAccess_ID 3).
static void mc_update_load_menu(void)
{
    int ret;
    int state;
    int i;
    int files;

    switch (mcMain_ID) {
    case 0:
        mc_menuid = 0;
        if (AA_New_McAlloc_Mode == 0 && alloc_flag != 0) {
            mc_free();
        }
        break;
    case 1:
        if (alloc_flag == 0) {
            if (AA_New_McAlloc_Mode == 0) {
                mc_alloc();
            }
            mcErr_ID = MC_ERR_CHECKING;
        } else {
            ret = mc_check();
            if (ret == 0) {
                mcErr_ID = MC_ERR_NONE;
                mcMain_ID = 5;
            } else if (ret == 1) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 2;
                AA_count = MC_RETRY_WAIT;
                mc_cursor[mcSlot_ID] = MC_FILE_NONE;
                mc_uidwork[mcSlot_ID] = 0;
            }
        }
        break;
    case 2:
        if (AA_count < 0) {
            AA_count = MC_RETRY_WAIT;
            if (mc_check() == 0) {
                mcErr_ID = MC_ERR_NONE;
                mc_menuid = 1;
                AA_Phase_Step = 0;
                mcMain_ID = 3;
            }
        } else {
            AA_count--;
        }
        break;
    case 4:
        if (AA_Phase_Step == 4) {
            mc_free();
            mc_alloc();
            ret = mc_load_new();
            state = mc_check4();
            printf(s_check4Format, state);
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    const u_int *header = (const u_int *)mc_header;

                    system_data_load_set(0);
                    files = 0;
                    for (i = 0; i < MC_INFO_COUNT; i++) {
                        if (mc_Getinfo_count(i) != 0) {
                            files++;
                        }
                    }
                    if (files == 0) {
                        mc_save_init();
                        mcFile_ID = 0;
                        mcErr_ID = MC_ERR_NEW_DATA;
                        mc_type = 0;
                        mcMain_ID = 5;
                    } else {
                        if (mc_cursor[mcSlot_ID] == MC_FILE_NONE ||
                            (header[1] & MC_HEADER_UID_MASK) != mc_uidwork[mcSlot_ID]) {
                            mcFile_ID = (int)(header[1] & MC_HEADER_FILE_MASK);
                            mc_uidwork[mcSlot_ID] = 0;
                        } else if (mc_Getinfo_count(mc_cursor[mcSlot_ID]) == 0) {
                            mcFile_ID = (int)(header[1] & MC_HEADER_FILE_MASK);
                        } else {
                            mcFile_ID = mc_cursor[mcSlot_ID];
                        }
                        if (mcFile_ID < 0) {
                            mcFile_ID = 0;
                        }
                        mcErr_ID = MC_ERR_NONE;
                        fream_counter = MC_FRAME_COUNTER_NOW;
                        mc_type = 0;
                        mcMain_ID = 5;
                    }
                } else {
                    mc_save_init();
                    mcFile_ID = 0;
                    mcErr_ID = MC_ERR_NEW_DATA;
                    mc_type = 0;
                    mcMain_ID = 5;
                }
            } else if (ret == MC_RESULT_FAIL) {
                if (mc_check() == 0) {
                    mc_save_init();
                    mcFile_ID = 0;
                    mc_type = 1;
                    mcErr_ID = MC_ERR_NEW_DATA;
                    mcMain_ID = 5;
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
        } else if (AA_Phase_Step == 3) {
            ret = mc_formatcheck2();
            state = mc_check4();
            /* No else here in the ROM (the save menu has one): a card error still lets the format
               answer move the step. Kept. */
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            }
            if (ret == MC_FORMAT_YES) {
                AA_Phase_Step = AA_UnFormatCk == 0 ? 4 : 1;
            } else if (ret == MC_FORMAT_NO) {
                if (mc_check() == 0) {
                    if (AA_UnFormatCk == 1) {
                        mcErr_ID = MC_ERR_UNFORMATTED;
                        mcMain_ID = 5;
                    } else {
                        AA_Phase_Step = 1;
                    }
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
        } else if (AA_Phase_Step == 2) {
            if (AA_count < 0) {
                AA_Phase_Step = 3;
            } else {
                AA_count--;
            }
        } else if (AA_Phase_Step == 1) {
            ret = mc_formatcheck2();
            state = mc_check4();
            if (MC_CHECK4_CARD_ERROR(state)) {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 5;
            } else if (ret == MC_FORMAT_YES) {
                AA_Phase_Step = 2;
                AA_count = MC_FORMAT_WAIT;
                AA_UnFormatCk = 0;
            } else if (ret == MC_FORMAT_NO) {
                if (mc_check() == 0) {
                    AA_UnFormatCk = 1;
                    AA_Phase_Step = 2;
                    AA_count = MC_FORMAT_WAIT;
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                }
            }
        } else if (AA_Phase_Step == 0) {
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 3;
            }
        }
        break;
    case 5:
    case 7:
        mc_check_always2(0);
        AA_Phase_Step = 0;
        break;
    case 6:
        if (AA_Phase_Step == 1) {
            ret = mc_load_new();
            if (ret == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    if (mc_Getinfo_count(mcFile_ID) == 0) {
                        mcErr_ID = MC_ERR_MSG_06;
                        mcMain_ID = 7;
                        SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                    } else {
                        mcErr_ID = MC_ERR_NONE;
                        mcMain_ID = 7;
                        mc_type = 0;
                        SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                        SSPlaySE(s_seBank, MC_SE_DONE, 1);
                    }
                } else {
                    mcErr_ID = MC_ERR_MSG_06;
                    mcMain_ID = 7;
                    mc_type = 0;
                    SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                }
            } else if (ret == MC_RESULT_FAIL) {
                if (mc_check() == 0) {
                    mcErr_ID = MC_ERR_MSG_06;
                    mcMain_ID = 7;
                    mc_type = 1;
                    SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                } else {
                    mcErr_ID = MC_ERR_NO_CARD;
                    mcMain_ID = 3;
                    SSStopSE(s_seBank, MC_SE_ACCESS, 1);
                }
            }
        } else if (AA_Phase_Step == 0) {
            if (mc_check() == 0) {
                AA_Phase_Step = 1;
                SSPlaySE(s_seBank, MC_SE_ACCESS, 1);
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 3;
            }
        }
        break;
    case 8:
        tiFade_Start(10.0f);
        mcMain_ID = 9;
        break;
    case 9:
        if (FadeEndTicks <= FadeStartTicks) {
            AA_Phase_Step = 0;
            mcMain_ID = 10;
        }
        break;
    case 10:
        if (AA_Phase_Step == 2) {
            mc_cursor[mcSlot_ID] = mcFile_ID;
            mc_uidwork[mcSlot_ID] = ((const u_int *)mc_header)[1] & MC_HEADER_UID_MASK;
            mc_free();
            mcMain_ID = 0;
            mc_set_load_slot();
            SceneSELoad();
            ssTrigRegisterEvent(TRIG_EVENT_MC_LOADED, 1, 0);
        } else if (AA_Phase_Step == 1) {
            system_data_load_set(1);
            mc_load_set();
            AA_Phase_Step = 2;
        } else if (AA_Phase_Step == 0) {
            AA_Phase_Step = 1;
        }
        break;
    }
}

// MC_ACCESS_AUTO_SAVE (mcAccess_ID 1).
static void mc_update_auto_save(void)
{
    int ret;
    int result;

    switch (mcMain_ID) {
    case 0:
        mc_menuid = 0;
        mc_free();
        break;
    case 1:
        if (mc_menuid == 2) {
            if (mc_load_new() == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    system_data_load_set(0);
                    mcErr_ID = MC_ERR_NONE;
                    mcMain_ID = 2;
                    mc_type = 0;
                    fream_counter = MC_FRAME_COUNTER_NOW;
                } else {
                    mcErr_ID = MC_ERR_MSG_11;
                    mcMain_ID = 0;
                }
            } else if (mc_check() == 0) {
                mc_save_init();
                mcErr_ID = MC_ERR_CHECKING;
                mcMain_ID = 2;
                fream_counter = MC_FRAME_COUNTER_NOW;
                mc_type = 1;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 0;
            }
        } else if (mc_menuid == 1) {
            if (mc_check() == 0) {
                mcErr_ID = MC_ERR_CHECKING;
                mc_menuid = 2;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 0;
            }
        } else if (mc_menuid == 0) {
            mc_alloc();
            mcErr_ID = MC_ERR_NONE;
            mc_menuid = 1;
        }
        break;
    case 2:
        mc_check_always(1);
        break;
    case 3:
        if (mc_type == 0) {
            mc_menuid = 4;
            if (*(const int *)(mc_databuff[mcFile_ID] + MC_DATA_USED) == 0) {
                mcMain_ID = 6;
            } else {
                mcErr_ID = MC_ERR_OVERWRITE_ASK;
                mcMain_ID = 5;
                fream_counter = MC_FRAME_COUNTER_NOW;
            }
        } else if (mc_formatcheck() == 0) {
            mc_menuid = 0;
            mcErr_ID = MC_ERR_MSG_06;
            mcMain_ID = 5;
            fream_counter = MC_FRAME_COUNTER_NOW;
        } else {
            ret = mc_free_area();
            if (ret == 2) {
                mcErr_ID = MC_ERR_MSG_0E;
                mcMain_ID = 0;
            } else if (ret == 1) {
                mcErr_ID = MC_ERR_MSG_11;
                mcMain_ID = 0;
            } else if (ret == 0) {
                mc_menuid = 2;
                fream_counter = MC_FRAME_COUNTER_NOW;
                mcErr_ID = MC_ERR_NEW_DATA;
                mcMain_ID = 5;
            }
        }
        break;
    case 5:
        mc_check_always(0);
        break;
    case 6:
        switch (mc_menuid) {
        case 0:
            mc_menuid = 1;
            mcErr_ID = MC_ERR_FORMAT_ASK;
            break;
        case 1:
            if (sceMcFormat(mcSlot_ID, 0) < 0) {
                mcErr_ID = MC_ERR_MSG_08;
                mcMain_ID = 0;
            } else if (sceMcSync(0, NULL, &result) != 0) {
                if (result == 0) {
                    mc_menuid = 2;
                } else {
                    mcErr_ID = MC_ERR_MSG_08;
                    mcMain_ID = 0;
                }
            }
            break;
        case 2:
        case 4:
            SSPlaySE(s_seBank, MC_SE_ACCESS, 1);
            mcErr_ID = MC_ERR_SAVING;
            mc_menuid++;
            break;
        case 3:
            system_data_load_set(2);
            ret = mc_save_new(0);
            if (ret == MC_RESULT_FAIL) {
                mc_menuid = 0;
                mcErr_ID = MC_ERR_MSG_0E;
                mcMain_ID = 0;
            } else if (ret == MC_RESULT_OK) {
                mcErr_ID = MC_ERR_SAVED;
                mcMain_ID = -1;
                mc_free();
            }
            SSStopSE(s_seBank, MC_SE_ACCESS, 1);
            break;
        case 5:
            system_data_load_set(2);
            if (mc_save_normal(0) == MC_RESULT_OK) {
                mcErr_ID = MC_ERR_SAVED;
                mcMain_ID = -1;
                mc_free();
            } else {
                mcErr_ID = MC_ERR_MSG_0E;
                mcMain_ID = 0;
            }
            SSStopSE(s_seBank, MC_SE_ACCESS, 1);
            break;
        }
        break;
    }
}

// MC_ACCESS_AUTO_LOAD (mcAccess_ID 0).
static void mc_update_auto_load(void)
{
    switch (mcMain_ID) {
    case 0:
        mc_menuid = 0;
        mc_free();
        break;
    case 1:
        if (mc_menuid == 2) {
            if (mc_load_new() == MC_RESULT_OK) {
                if (*(const int *)mc_header == MC_HEADER_ID) {
                    system_data_load_set(0);
                    mcErr_ID = MC_ERR_NONE;
                    mcMain_ID = 2;
                    fream_counter = MC_FRAME_COUNTER_NOW;
                } else {
                    mcErr_ID = MC_ERR_MSG_11;
                    mcMain_ID = 0;
                }
            } else if (mc_check() == 0) {
                mcErr_ID = MC_ERR_MSG_04;
                mcMain_ID = 0;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 0;
            }
        } else if (mc_menuid == 1) {
            if (mc_check() == 0) {
                mcErr_ID = MC_ERR_CHECKING;
                mc_menuid = 2;
            } else {
                mcErr_ID = MC_ERR_NO_CARD;
                mcMain_ID = 0;
            }
        } else if (mc_menuid == 0) {
            mc_alloc();
            mcErr_ID = MC_ERR_CHECKING;
            mc_menuid = 1;
        }
        break;
    case 2:
        mc_check_always(1);
        mc_menuid = 0;
        break;
    case 3:
        tiFade_Start(10.0f);
        mcMain_ID = 4;
        break;
    case 4:
        if (FadeEndTicks <= FadeStartTicks) {
            mcMain_ID = 5;
        }
        break;
    case 5:
        if (mc_menuid == 2) {
            mcErr_ID = MC_ERR_LOADED;
            mcMain_ID = -1;
            mc_free();
            ssTrigRegisterEvent(TRIG_EVENT_MC_LOADED, 1, 0);
        } else if (mc_menuid == 1) {
            mc_load_set();
            mc_menuid = 2;
        } else if (mc_menuid == 0) {
            mc_menuid = 1;
            mcErr_ID = MC_ERR_MSG_0E;
        }
        break;
    }
}

// CT3 0x018b6080: run one frame of the card flow mcAccess_ID names (main calls it every frame). The card
// calls wait vblanks inside, so the frame timer is put back as it was; the ROM returns early (timer not
// restored) for an mcAccess_ID it does not know. The ROM inlines the four flows; they are split here.
void MemorycardUpdate(void)
{
    // callcheck: SSPlaySE, SSStopSE, SceneSELoad, mc_Getinfo_count, mc_alloc, mc_check, mc_check4, mc_check_always, mc_check_always2, mc_formatcheck, mc_formatcheck2, mc_free, mc_free_area, mc_load_new, mc_load_set, mc_save_init, mc_save_new, mc_save_normal, printf, sceMcFormat, sceMcSync, ssTrigCheckEvent, ssTrigRegisterEvent, ssTrigRemoveEvent, system_data_load_set, tiFade_Start (the ROM inlines the four flows: the mc_update_* functions above make these calls)
    int timer = ga_main_timer;

    switch (mcAccess_ID) {
    case MC_ACCESS_SAVE:
        mc_update_save_menu();
        break;
    case MC_ACCESS_LOAD:
        mc_update_load_menu();
        break;
    case MC_ACCESS_AUTO_SAVE:
        mc_update_auto_save();
        break;
    case MC_ACCESS_AUTO_LOAD:
        mc_update_auto_load();
        break;
    case MC_ACCESS_NONE:
        mc_menuid = 0;
        break;
    default:
        return;
    }
    ga_main_timer = timer;
}

// ---- stubs (tools/reverse/stubgen.py) ----


// stub of CT3 0x018b4480
void SceneLightSet(int scene)
{
    ALYSSAPAN_STUB("SceneLightSet", 0x018b4480);
}

// stub of CT3 0x018b49f0
void CharLightSet(int chara)
{
    ALYSSAPAN_STUB("CharLightSet", 0x018b49f0);
}

// stub of CT3 0x018bb2d0
int mc_check3(void)
{
    ALYSSAPAN_STUB("mc_check3", 0x018bb2d0);
    return {};
}

// stub of CT3 0x018badd0
void mc_save_system_init(void)
{
    ALYSSAPAN_STUB("mc_save_system_init", 0x018badd0);
}

// stub of CT3 0x018b6020
int mc_Getinfo_count(int file)
{
    ALYSSAPAN_STUB("mc_Getinfo_count", 0x018b6020);
    return {};
}

// stub of CT3 0x018b7870
void mc_check_always2(int clearError)
{
    ALYSSAPAN_STUB("mc_check_always2", 0x018b7870);
}

// stub of CT3 0x018baac0
int mc_free_area(void)
{
    ALYSSAPAN_STUB("mc_free_area", 0x018baac0);
    return {};
}

// stub of CT3 0x018b7930
void mc_load_set(void)
{
    ALYSSAPAN_STUB("mc_load_set", 0x018b7930);
}

// stub of CT3 0x018b8eb0
int mc_save_new(int alt)
{
    ALYSSAPAN_STUB("mc_save_new", 0x018b8eb0);
    return {};
}

// stub of CT3 0x018b9790
int mc_save_normal(int alt)
{
    ALYSSAPAN_STUB("mc_save_normal", 0x018b9790);
    return {};
}

// stub of CT3 0x018b8680
void system_data_load_set(int mode)
{
    ALYSSAPAN_STUB("system_data_load_set", 0x018b8680);
}
