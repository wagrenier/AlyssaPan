// game/PicPaz.c -- Clock Tower 3: the aa x commands of the trigger script (0x018da090..0x018dab60): the
// movie's fade words (aaXcmd_MovieFedeOutCancel, read by movie.c's AA_StartMovie / AA_movie), the
// memory-card screen's file list, cursor and file details (Character_Init.c's mc_Getinfo_* into script
// variables and events), the card work buffer, and a bank's end by file name (aa_search). ssTrigExecuteActCmd
// (ssGrpPortalCheck.c) calls them through ssXcmd_functions. Written 2026-11-11: all 9 functions.
//
// aa_to_big is a CodeWarrior while loop (b to the test): Ghidra splits its test off as FUN_018dab3c
// (0x018dab3c..0x018dab60). Written here as one function (hazards.md, AA_ToSmall's case).
//
// The words are upper-cased in place, as the ROM does: argv points into the script's own strings.

#include <stdlib.h>
#include <string.h>
#include "game/PicPaz.h"
#include "game/xcmd_tn.h"
#include "game/movie.h"
#include "game/Character_Init.h"
#include "game/ssGrpPortalCheck.h"
#include "game/fileio.h"
#include "game/ahalloc.h"
#include "game/unassigned_data.h"

// CT3 0x018da090: the file's empty command (its name is the symtab's).
int PicPaz(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018da0a0: the next movie's fades. No argument: cut the fade-out. argv[0] "CUT": cut the fade-out;
// argv[1] the fade-in ("WHITE" from white, "BLACK" from black, "-" none); argv[2] "WHITE" fades out to
// white; argv[3] "OUT2" hands the fade-out on to tiFade. Every word not given is cleared.
int aaXcmd_MovieFedeOutCancel(int argc, char **argv, int bank)
{
    char *s;

    (void)bank;
    AA_MvInWhite = AA_MV_BLACK;
    AA_MvOutWhite = AA_MV_BLACK;
    AA_MvFedeOut2Md = 0;
    AA_MvFedeOutCut = 0;
    if (argc == 0) {
        AA_MvFedeOutCut = 1;
        return XCMD_DONE;
    }
    s = argv[0];
    aa_to_big(s);
    if (strcmp(s, AA_WORD_CUT) == 0)
        AA_MvFedeOutCut = 1;
    if ((u_int)argc > 1) {
        s = argv[1];
        aa_to_big(s);
        if (strcmp(s, AA_WORD_WHITE) == 0)
            AA_MvInWhite = AA_MV_WHITE;
        else if (strcmp(s, AA_WORD_BLACK) == 0)
            AA_MvInWhite = AA_MV_FADE;
        else if (strcmp(s, AA_WORD_NONE) == 0)
            AA_MvInWhite = AA_MV_BLACK;
    }
    if ((u_int)argc > 2) {
        s = argv[2];
        aa_to_big(s);
        if (strcmp(s, AA_WORD_WHITE) == 0)
            AA_MvOutWhite = AA_MV_WHITE;
    }
    if ((u_int)argc > 3) {
        s = argv[3];
        aa_to_big(s);
        if (strcmp(s, AA_WORD_OUT2) == 0)
            AA_MvFedeOut2Md = AA_MV_FADE2_TI_FADE;
    }
    return XCMD_DONE;
}

// CT3 0x018da210: argv[0] == 1 takes the card work buffer (kept between flows) unless it is held; any
// other value frees it if it is held.
int aaXcmd_McWorkAlloc(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    if (atoi(argv[0]) == AA_MC_WORK_ALLOC) {
        if (mc_CheckMcWorkAllocc() == 0) {
            Set_AA_New_McAlloc_Mode();
            mc_alloc();
        }
    } else if (mc_CheckMcWorkAllocc() == 1) {
        mc_free();
    }
    return XCMD_DONE;
}

// CT3 0x018da280: move the file cursor (mcFile_ID) down (argv[3] == 0) or up. With event argv[2] off it
// jumps to the next file in that direction that holds a save, staying put if there is none; with it on it
// steps by one within 0..7. Variable argv[0] takes the cursor; event argv[1] says whether it moved.
int aaXcmd_McCursorUpDate(int argc, char **argv, int bank)
{
    int old, up, id;

    (void)argc;
    old = mcFile_ID;
    up = atoi(argv[3]);
    if (ssTrigCheckEvent(atoi(argv[2]) & 0xffff, 0, bank) == 0) {
        id = mcFile_ID;
        if (up == 0) {
            do {
                if (--id < 0)
                    goto moved;
            } while (mc_Getinfo_count(id) == 0);
        } else {
            do {
                if (++id > AA_MC_FILE_LAST)
                    goto moved;
            } while (mc_Getinfo_count(id) == 0);
        }
        mcFile_ID = id;
    } else if (up == 0) {
        if (mcFile_ID > 0)
            mcFile_ID--;
    } else if (mcFile_ID < AA_MC_FILE_LAST) {
        mcFile_ID++;
    }
moved:
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, mcFile_ID, 0, bank);
    if (old == mcFile_ID)
        ssTrigRemoveEvent(atoi(argv[1]) & 0xffff, 0, bank);
    else
        ssTrigRegisterEvent(atoi(argv[1]) & 0xffff, 0, bank);
    return XCMD_DONE;
}

// CT3 0x018da420: events argv[0..7] on for the files that hold a save, off for the rest; with a ninth
// argument, event argv[8] on when mc_Getinfo_mc_type answers 0.
int aaXcmd_MCFileList(int argc, char **argv, int bank)
{
    int i;

    for (i = 0; i < MC_INFO_COUNT; i++) {
        if (mc_Getinfo_count(i) == 0)
            ssTrigRemoveEvent(atoi(argv[i]) & 0xffff, 0, bank);
        else
            ssTrigRegisterEvent(atoi(argv[i]) & 0xffff, 0, bank);
    }
    if ((u_int)argc > MC_INFO_COUNT) {
        if (mc_Getinfo_mc_type() == 0)
            ssTrigRegisterEvent(atoi(argv[MC_INFO_COUNT]) & 0xffff, 0, bank);
        else
            ssTrigRemoveEvent(atoi(argv[MC_INFO_COUNT]) & 0xffff, 0, bank);
    }
    return XCMD_DONE;
}

// CT3 0x018da530: the details of the file in variable argv[0], into variables: argv[1] its save count
// (0: empty, then every detail is 0), argv[2] the branch - 1, argv[3] the message number (at least 1),
// argv[4..9] the play time's digits h h m m s s (99:59:59 from 100 hours on); with an eleventh argument,
// event argv[10] on when the save has cleared the game.
int aaXcmd_CheckMCFile(int argc, char **argv, int bank)
{
    int file, count, branch, msg, t, hours, rest, min, sec;
    int hTen, hOne, mTen, mOne, sTen, sOne;

    file = ssTrigGetVariable(atoi(argv[0]), 0, bank);
    count = mc_Getinfo_count(file);
    ssTrigSetVariable(atoi(argv[1]) & 0xffff, count, 0, bank);
    if (count == 0) {
        branch = msg = 0;
        hTen = hOne = mTen = mOne = sTen = sOne = 0;
    } else {
        branch = mc_Getinfo_BranchNo(file) - 1;
        msg = mc_Getinfo_msg_num(file);
        if (msg < 1)
            msg = 1;
        t = mc_Getinfo_time(file) / AA_MC_TIME_FPS;
        hours = t / 3600;
        if (hours < AA_MC_TIME_HOURS_MAX) {
            rest = t % 3600;
            min = (rest / 60) % 60;
            sec = (rest - min * 60) % 60;
            hTen = hours / 10 - (hours / 100) * 10;
            hOne = hours % 100 - hTen * 10;
            mTen = min / 10;
            mOne = min % 10;
            sTen = sec / 10;
            sOne = sec % 10;
        } else {
            hTen = hOne = mOne = sOne = 9;
            mTen = sTen = 5;
        }
    }
    ssTrigSetVariable(atoi(argv[2]) & 0xffff, branch, 0, bank);
    ssTrigSetVariable(atoi(argv[3]) & 0xffff, msg, 0, bank);
    ssTrigSetVariable(atoi(argv[4]) & 0xffff, hTen, 0, bank);
    ssTrigSetVariable(atoi(argv[5]) & 0xffff, hOne, 0, bank);
    ssTrigSetVariable(atoi(argv[6]) & 0xffff, mTen, 0, bank);
    ssTrigSetVariable(atoi(argv[7]) & 0xffff, mOne, 0, bank);
    ssTrigSetVariable(atoi(argv[8]) & 0xffff, sTen, 0, bank);
    ssTrigSetVariable(atoi(argv[9]) & 0xffff, sOne, 0, bank);
    if ((u_int)argc > 10) {
        if (mc_Getinfo_ClearFlag(file) == 0)
            ssTrigRemoveEvent(atoi(argv[10]) & 0xffff, 0, bank);
        else
            ssTrigRegisterEvent(atoi(argv[10]) & 0xffff, 0, bank);
    }
    return XCMD_DONE;
}

// CT3 0x018da900: end a bank's scene: argv[0] names its file (aa_search), or is "VALUE" / "*" with the
// bank's number in argv[1]. A group's later bank ends the whole group from its head.
int aaXcmd_SceneEndBank(int argc, char **argv, int bank)
{
    char *s;
    int idx, state;

    (void)bank;
    if (argc == 0)
        return XCMD_DONE;
    s = argv[0];
    aa_to_big(s);
    if ((u_int)argc > 1 && (strcmp(s, AA_WORD_VALUE) == 0 || strcmp(s, AA_WORD_ANY) == 0))
        idx = atoi(argv[1]);
    else
        idx = aa_search(s);
    if (idx >= BANK_MAX || idx < 0)
        return XCMD_DONE;
    state = BankInfo[idx].state;
    if (state >= 0 && state > BANK_STATE_LOADED)
        idx -= state - 1;               /* a group's later bank holds its place: back to the head */
    SceneEndBank(idx, BankInfo[idx].groupCount);
    return XCMD_DONE;
}

// CT3 0x018daa10: the loaded bank whose file is name, searched from the first bank past the global scene
// heap's used part (bank 6 + the 256 KB slots it reaches into); -1 when none.
int aa_search(const char *name)
{
    int used, i;
    BANK_INFO *bi;

    used = (int)((u_char *)SunAllocUsedBorderCheck(&gHeapGlobalScene) - (u_char *)gHeapGlobalScene.top);
    for (i = (used + BANK_SLOT_SIZE - 1) / BANK_SLOT_SIZE + BANK_GLOBAL_MAX; i < BANK_MAX; i++) {
        bi = &BankInfo[i];
        if (bi->state == BANK_STATE_LOADED) {
            if (bi->name[0] != '\0' && strcmp(bi->name, name) == 0)
                return i;
            i += bi->groupCount - 1;
        }
    }
    return -1;
}

// CT3 0x018dab10 (its loop test 0x018dab3c, Ghidra's FUN_018dab3c): a..z to A..Z in place.
void aa_to_big(char *s)
{
    for (; *s != '\0'; s++) {
        if (*s > '`' && *s < '{')
            *s -= ' ';
    }
}
