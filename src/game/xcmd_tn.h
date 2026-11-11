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

// Slice 2 (2026-11-11).
#define XCMD_MSG_BANK 2                 /* the message bank (wideXcmd_ReadChengeMsg) */
#define XCMD_AREA_COUNT 53              /* RoomFileName_STR_Tbl's words (GetAreaNumber's areas, 0 none) */
#define XCMD_VALUE_CONST 2              /* a number argument's source: the text itself (0 / 1: a variable,
                                           local / global) */
#define XCMD_FRAME_TIME 10              /* a script frame in asGetSceneAnimationCurrent's time / 100 units */
#define TRIG_EVENT_CONTINUE 0x9d        /* registered while tnXcmd_Continue keeps the continue backup */
// tnXcmd_EndPanic's argv[0]: only a running full panic ends (KEEP, ZERO, BEAT) or any (FORCE_*); the
// gauge kept, set to 0, or set to Panicdef.beatFrom.
#define XCMD_END_PANIC_KEEP 0
#define XCMD_END_PANIC_ZERO 1
#define XCMD_END_PANIC_BEAT 2
#define XCMD_END_PANIC_FORCE_ZERO 3
#define XCMD_END_PANIC_FORCE_BEAT 4
// Slice 3 (2026-11-11): the omni overrides take light slots 1..XCMD_OMNI_SLOTS (the ROM indexes
// [slot - 1]); a cleared pair is 0xffff (no bank). The screen adjust's offsets are clamped to
// +-XCMD_SCREEN_ADJUST_MAX; DX moves AA_DISPLAY_DX_STEP units per step from AA_DISPLAY_DX_BASE, DY 1
// from AA_DISPLAY_DY_BASE (movie.h).
#define XCMD_OMNI_SLOTS 4
#define XCMD_OMNI_NONE 0xffff
#define XCMD_SCREEN_ADJUST_MAX 0x10
#define XCMD_PUZZLE_ROW_FRAMES 24       /* tnXcmd_1P01_PazzleInit: frames per step of variable 0x12 */
#define XCMD_PUZZLE_COL_FRAMES 40       /* ... and per step of variable 0x21, / 100 */
#define TRIG_VAR_PUZZLE_ROW 0x12        /* named from tnXcmd_1P01_PazzleInit only */
#define TRIG_VAR_PUZZLE_COL 0x21
#define XCMD_ROODERS_TIME_LIMIT 0x2a30  /* tnXcmd_SetRoodersPts: 10800 s (3 h) of play earn time points */
#define XCMD_ROODERS_FPS 30             /* progress_time counts frames */
#define XCMD_ROODERS_ITEM_POINTS 10000
#define XCMD_ROODERS_TOP_DIGIT 1000000000
#define ITEM_ROW_HELD 1                 /* ItemData row byte +1: held count */
// Slice 4 (2026-11-11). The play-time clock (progress_time counts frames at 30 a second).
#define XCMD_FRAMES_PER_MINUTE 1800
#define XCMD_FRAMES_PER_HOUR 108000
#define XCMD_TIME_MAX 10799970          /* tnXcmd_TimeSet: 99:59:59; at or past it the digits read 9 / 5 */
// tnXcmd_1P01_PazzleAnim: the clock puzzle's hands (TRIG_VAR_PUZZLE_ROW the hour 0..11, _COL the
// minute 0..59). A turn back moves 40 minutes (16 frames), forward 25 (10 frames); the scene's track
// is XCMD_PUZZLE_TRACK_FRAMES (12 hours) long; the command then waits out the turn's frames.
#define XCMD_PUZZLE_HOURS 12
#define XCMD_PUZZLE_MINUTES 60
#define XCMD_PUZZLE_TRACK_FRAMES 288
#define XCMD_PUZZLE_BACK_MINUTES 40
#define XCMD_PUZZLE_BACK_FRAMES 16
#define XCMD_PUZZLE_BACK_WAIT 0x40
#define XCMD_PUZZLE_FWD_MINUTES 25
#define XCMD_PUZZLE_FWD_FRAMES 10
#define XCMD_PUZZLE_FWD_WAIT 0x28
#define XCMD_PUZZLE_SPEED 0.25f
// The scene animation commands' selection argument (wideXcmd_PlayAnimeToFream / _Range): 0 the pose
// tracks, 1 the other tracks, 2 by part (asSceneAnimationStart's pose / other / byPart).
#define XCMD_ANIM_SELECT_POSE 0
#define XCMD_ANIM_SELECT_OTHER 1
#define XCMD_ANIM_SELECT_BY_PART 2
extern int load_id;                    /* sbss 0x0194493c: the area tnXcmd_Menu_MapLoad loaded */

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

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): declared with the
// table's SS_XCMD shape (xcmd_tn.h) until this TU's session reads each one (2026-11-11).
int tnXcmd_TexFileSetPage(int argc, char **argv, int bank);                              // CT3 0x018f3c30
int tnXcmd_TexFileSetNext(int argc, char **argv, int bank);                              // CT3 0x018f3bd0
int tnCurrentSceneSet(int argc, char **argv, int bank);                                  // CT3 0x018f3ba0
int tnXcmd_VistaVision(int argc, char **argv, int bank);                                 // CT3 0x018f37e0
int tnXcmd_SetAllControlDisableCamera(int argc, char **argv, int bank);                  // CT3 0x018ee9c0
int tnXcmd_Character_Ambient_Change(int argc, char **argv, int bank);                    // CT3 0x018f36b0
int wideXcmd_VariableCal(int argc, char **argv, int bank);                               // CT3 0x018f34a0
int wideXcmd_PlayAnimeRange(int argc, char **argv, int bank);                            // CT3 0x018f3280
int wideXcmd_PlayAnimeToFream(int argc, char **argv, int bank);                          // CT3 0x018f30e0
int tnXcmd_1P01_PazzleInit(int argc, char **argv, int bank);                             // CT3 0x018f2fa0
int tnXcmd_1P01_PazzleAnim(int argc, char **argv, int bank);                             // CT3 0x018f2c50
int wideXcmd_PlayAnimeToFream2(int argc, char **argv, int bank);                         // CT3 0x018f28f0
int tnXcmd_ShapeFree(int argc, char **argv, int bank);                                   // CT3 0x018f2830
int tnXcmd_BehaviorFree(int argc, char **argv, int bank);                                // CT3 0x018f2720
int tnXcmd_SetFarClip(int argc, char **argv, int bank);                                  // CT3 0x018f26f0
int tnXcmd_SetNearClip(int argc, char **argv, int bank);                                 // CT3 0x018f26c0
int tnXcmd_SetPanic(int argc, char **argv, int bank);                                    // CT3 0x018f25c0
int tnXcmd_SetMovieWorkBuff(int argc, char **argv, int bank);                            // CT3 0x018f2430
int tnXcmd_SceneFogAnimation(int argc, char **argv, int bank);                           // CT3 0x018f2240
int tnXcmd_CharFogAnimation(int argc, char **argv, int bank);                            // CT3 0x018f20b0
int tnXcmd_BankFree(int argc, char **argv, int bank);                                    // CT3 0x018f1e20
int tnXcmd_TimeSet(int argc, char **argv, int bank);                                     // CT3 0x018f1a10
int tnXcmd_Filter_Change(int argc, char **argv, int bank);                               // CT3 0x018f1710
int tnXcmd_Menu_MapLoad(int argc, char **argv, int bank);                                // CT3 0x018f1570
int tnXcmd_Key_Lock(int argc, char **argv, int bank);                                    // CT3 0x018efc50
int tnXcmd_SetRoodersPts(int argc, char **argv, int bank);                               // CT3 0x018ee5f0
int wideXcmd_ReadChengeMsg(int argc, char **argv, int bank);                             // CT3 0x018efb80
int tnXcmd_StayRoom(int argc, char **argv, int bank);                                    // CT3 0x018efa90
int tnXcmd_OmniRange(int argc, char **argv, int bank);                                   // CT3 0x018ef810
int tnXcmd_CharOmniEffect(int argc, char **argv, int bank);                              // CT3 0x018ef670
int tnXcmd_AllOmniEffect(int argc, char **argv, int bank);                               // CT3 0x018ef550
int tnXcmd_EndPanic(int argc, char **argv, int bank);                                    // CT3 0x018ef3d0
int tnXcmd_Continue(int argc, char **argv, int bank);                                    // CT3 0x018ef320
int tnXcmd_TimeStop(int argc, char **argv, int bank);                                    // CT3 0x018ef1c0
int wideXcmd_ScreenAdjust(int argc, char **argv, int bank);                              // CT3 0x018eedf0
