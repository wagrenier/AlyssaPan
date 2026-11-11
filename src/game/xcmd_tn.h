// game/xcmd_tn.h -- CT3 game/xcmd_tn.c (0x018ee5f0..0x018f3cb0): the trigger script's "x commands" by tn /
// wide (tnXcmd_*, wideXcmd_*). Nothing calls them by name: ssTrigExecuteActCmd's command 0x514 reads an
// index byte and argc strings from ActData and calls ssXcmd_functions[index](argc, argv, bank)
// (data 0x0193a500, 256 words). A command answers non-zero when it is done; 0 runs it again next frame
// (the dispatcher steps its cursor back over the command). The dispatcher tests the low byte.
#pragma once

#include "game/ct3_types.h"

// An x command (the table's slot type). Commands that ignore their arguments still take them here; the
// ROM's own prototypes vary (CodeWarrior passes the three in a0..a2 either way).
typedef int (*SS_XCMD)(int argc, char **argv, int bank);

#define XCMD_DONE 1                     /* the command finished: the script goes on */
#define XCMD_WAIT 0                     /* run the command again next frame */

#define XCMD_MAP_BANK 4                 /* the menu map's bank (tnXcmd_Menu_MapLoad / _MapOff) */
#define XCMD_LOAD_WAIT_FIRST_BANK 6     /* tnXcmd_LoadWait looks at banks 6..BANK_MAX-1 */
#define XCMD_ROOM_NAME_LEN 4            /* scene / room names compare 4 characters ("A101") */

// MemorycardAutoLoad's answer as the script hands it: 2 / 3 are a fresh press of 0 / 1 (the variable
// is set back to 0 / 1, the press itself passed on).
#define XCMD_AUTOLOAD_PRESS 2

int tnXcmd_AutoLoad(int argc, char **argv, int bank);                                   // CT3 0x018eebb0
int tnXcmd_AutoLoadInit(int argc, char **argv, int bank);                               // CT3 0x018eecd0
int wideXcmd_TitleStart(int argc, char **argv, int bank);                               // CT3 0x018eeb40
int wideXcmd_CinemaTheaterSet(int argc, char **argv, int bank);                         // CT3 0x018eeb70
int wideXcmd_MovieWait(int argc, char **argv, int bank);                                // CT3 0x018eecf0
int wideXcmd_SkipWait(int argc, char **argv, int bank);                                 // CT3 0x018eed40
int wideXcmd_FrameWait(int argc, char **argv, int bank);                                // CT3 0x018eed50
int wideXcmd_GlobalFree(int argc, char **argv, int bank);                               // CT3 0x018eeff0
int wideXcmd_BranchClear(int argc, char **argv, int bank);                              // CT3 0x018ef090
int wideXcmd_GameOver(int argc, char **argv, int bank);                                 // CT3 0x018ef0b0
int wideXcmd_DemoSkipCheck(int argc, char **argv, int bank);                            // CT3 0x018ef0e0
int tnXcmd_TexFileInitPageStop(int argc, char **argv, int bank);                        // CT3 0x018ef1b0
int tnXcmd_Room_IN_ALYSSA(int argc, char **argv, int bank);                             // CT3 0x018f1340
int tnXcmd_Room_IN_Icon(int argc, char **argv, int bank);                               // CT3 0x018f13f0
int tnXcmd_Room_IN_Check(int argc, char **argv, int bank);                              // CT3 0x018f1480
int tnXcmd_Menu_MapOff(int argc, char **argv, int bank);                                // CT3 0x018f1510
int wideXcmd_Variable_Copy(int argc, char **argv, int bank);                            // CT3 0x018f1680
int tnXcmd_Char_DirectLight_Change(int argc, char **argv, int bank);                    // CT3 0x018f1880
int tnXcmd_Scene_Ambient_Change(int argc, char **argv, int bank);                       // CT3 0x018f1940
int tnXcmd_ScneName(int argc, char **argv, int bank);                                   // CT3 0x018f1da0
int tnXcmd_LoadWait(int argc, char **argv, int bank);                                   // CT3 0x018f1ff0
int BankUseCheck(int first, int count);                                                 // CT3 0x018f2560
