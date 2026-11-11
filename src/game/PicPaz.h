// game/PicPaz.h -- CT3 game/PicPaz.c (0x018da090..0x018dab60): the aa x commands of the trigger script
// (the movie's fades, the memory-card file list and cursor, a bank's end by name) and their two string
// helpers. Written 2026-11-11: all 9 functions.
#pragma once

#include "sdk/scetypes.h"

// ---- names ----------------------------------------------------------------------------------------------
// aaXcmd_MovieFedeOutCancel's words (upper-cased first by aa_to_big; data 0x019369c8..0x019369e8).
#define AA_WORD_CUT "CUT"               /* argv[0]: no fade-out (AA_MvFedeOutCut) */
#define AA_WORD_WHITE "WHITE"           /* argv[1]: in from white (AA_MV_WHITE); argv[2]: out to white */
#define AA_WORD_BLACK "BLACK"           /* argv[1]: fade in from black (AA_MV_FADE) */
#define AA_WORD_NONE "-"                /* argv[1]: no fade-in (AA_MV_BLACK) */
#define AA_WORD_OUT2 "OUT2"             /* argv[3]: hand the fade-out on to tiFade (AA_MV_FADE2_TI_FADE) */
// aaXcmd_SceneEndBank's words (data 0x019369f0 / 0x019369f8): argv[1] is then the bank's number.
#define AA_WORD_VALUE "VALUE"
#define AA_WORD_ANY "*"

#define AA_MC_FILE_LAST 7               /* aaXcmd_McCursorUpDate: files 0..7 (MC_INFO_COUNT - 1) */
#define AA_MC_WORK_ALLOC 1              /* aaXcmd_McWorkAlloc's argv[0]: take the work buffer; else free it */

// aaXcmd_CheckMCFile: a save's play time, frames at 30 a second, shown as hh:mm:ss (99:59:59 at most).
#define AA_MC_TIME_FPS 30
#define AA_MC_TIME_HOURS_MAX 100

// ---- functions --------------------------------------------------------------------------------------------
// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): argv[0..argc-1] are the
// command's argument strings, bank the bank whose script runs it; each answers XCMD_DONE.
int PicPaz(int argc, char **argv, int bank);                                             // CT3 0x018da090
int aaXcmd_MovieFedeOutCancel(int argc, char **argv, int bank);                          // CT3 0x018da0a0
int aaXcmd_McWorkAlloc(int argc, char **argv, int bank);                                 // CT3 0x018da210
int aaXcmd_McCursorUpDate(int argc, char **argv, int bank);                              // CT3 0x018da280
int aaXcmd_MCFileList(int argc, char **argv, int bank);                                  // CT3 0x018da420
int aaXcmd_CheckMCFile(int argc, char **argv, int bank);                                 // CT3 0x018da530
int aaXcmd_SceneEndBank(int argc, char **argv, int bank);                                // CT3 0x018da900
int aa_search(const char *name);                                                         // CT3 0x018daa10
void aa_to_big(char *s);                                                                 // CT3 0x018dab10
