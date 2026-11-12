// game/asMothPause.h -- CT3 game/asMothPause.c: pausing the moths a script armed (MothInfo,
// asMothRouteSearch.h) while the game stops (time stop, panic, the card load, the control-disable commands).
#pragma once

#include "sdk/scetypes.h"

// asMothPause's mode, kept in MothInfo.paused.
#define MOTH_PAUSE_OFF 0                /* resume: release the moths if Alyssa moved meanwhile */
#define MOTH_PAUSE_ON 1                 /* pause: remember where Alyssa stands */

// MOTH.state / w01 / w02 as asMothPause reads and sets them (named from this one use: asMothOperate's TU
// names the states).
#define MOTH_STATE_5 5                  /* the state a resume releases (on Alyssa, by its effect) */
#define MOTH_RELEASE_W01 2              /* MOTH.w01 after the release */
#define MOTH_RELEASE_W02 3              /* MOTH.w02 after the release */

// Beyond this distance from the paused position a resume releases the moths.
#define MOTH_PAUSE_MOVED 1.0f           /* 0x3f800000 */

// Does nothing unless moths are armed and their bank is drawn. MOTH_PAUSE_ON remembers Alyssa's position;
// MOTH_PAUSE_OFF, when she has since moved more than MOTH_PAUSE_MOVED, releases every moth in
// MOTH_STATE_5 and stops the moths' pad rumble.
void asMothPause(int mode);                                                              // CT3 0x01883ca0
