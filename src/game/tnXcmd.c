// game/tnXcmd.c -- Clock Tower 3: the trigger script's tn x commands after xcmd_tn.c (0x018f3cb0..0x018f3f60;
// symtab local chunk 140, the statics tnXcmd_LeverClattering*): the lever-mashing struggle, the player /
// monster control locks, scripted damage and a random event. Each takes the command's argument strings
// (argv[0..argc-1]) and the bank whose script runs it; ssTrigExecuteActCmd (ssGrpPortalCheck.c) calls them
// through ssXcmd_functions. All 8 written 2026-11-13.

#include <stdlib.h>
#include "game/tnXcmd.h"
#include "game/xcmd_tn.h"
#include "game/main.h"
#include "game/unassigned_data.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ssGrpPortalCheck.h"

// ---- statics (symtab LOCAL, .sbss) -------------------------------------------------------------------
static int tnXcmd_LeverClatteringMeterDown;     /* sbss 0x01944940: frames since the last press */
static int tnXcmd_LeverClatteringMeter;         /* sbss 0x01944944: presses, less the decay */
static int tnXcmd_LeverClatteringTimer;         /* sbss 0x01944948: frames left */

// CT3 0x018f3dd0: start a lever struggle. argv[0] the frames it lasts. Alyssa goes into panic mode
// (main.c reads PlayerPanicMode); the meter starts empty. The ROM leaves MeterDown as it was.
int tnXcmd_LeverClatteringInit(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    int frames = (u_short)atoi(argv[0]);
    tnXcmd_LeverClatteringMeter = 0;
    tnXcmd_LeverClatteringTimer = frames;
    PlayerPanicMode = 1;
    return XCMD_DONE;
}

// CT3 0x018f3cb0: one frame of the lever struggle. argv[0] the event, argv[1] the frames without a press
// before the meter drops one, argv[2] the meter to beat. Each press of LEVER_CLATTER_BUTTON fills the meter
// by one; when the timer runs out the event is removed (lost), when the meter passes argv[2] first it is
// registered (won); either way panic mode and the controller mode end and the command is done.
int tnXcmd_LeverClattering(int argc, char **argv, int bank)
{
    (void)argc;
    if (pad_trg_button[0] & LEVER_CLATTER_BUTTON) {
        tnXcmd_LeverClatteringMeterDown = 0;
        tnXcmd_LeverClatteringMeter++;
    } else {
        tnXcmd_LeverClatteringMeterDown++;
        int idle = tnXcmd_LeverClatteringMeterDown;
        if ((int)(u_short)atoi(argv[1]) < idle) {
            tnXcmd_LeverClatteringMeter--;
            tnXcmd_LeverClatteringMeterDown = 0;
            if (tnXcmd_LeverClatteringMeter < 0)
                tnXcmd_LeverClatteringMeter = 0;
        }
    }

    tnXcmd_LeverClatteringTimer--;
    if (tnXcmd_LeverClatteringTimer < 0) {
        ssTrigRemoveEvent((u_short)atoi(argv[0]), 0, bank);
        PlayerPanicMode = 0;
        controller_mode = CONTROLLER_MODE_NORMAL;
        return XCMD_DONE;
    }

    int meter = tnXcmd_LeverClatteringMeter;
    if ((int)(u_short)atoi(argv[2]) < meter) {
        ssTrigRegisterEvent((u_short)atoi(argv[0]), 0, bank);
        PlayerPanicMode = 0;
        controller_mode = CONTROLLER_MODE_NORMAL;
        return XCMD_DONE;
    }
    return XCMD_WAIT;
}

// CT3 0x018f3e00: the monsters stop (TRG_CHAR_STOP_CHAR).
int tnXcmd_MonsterControllStop(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    TrgCharStopFlag |= TRG_CHAR_STOP_CHAR;
    return XCMD_DONE;
}

// CT3 0x018f3e20: the monsters move again.
int tnXcmd_MonsterControllStart(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    TrgCharStopFlag &= ~TRG_CHAR_STOP_CHAR;
    return XCMD_DONE;
}

// CT3 0x018f3e40: the player's keys stop (TRG_CHAR_STOP_KEYS).
int tnXcmd_PlayerControllStop(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    TrgCharStopFlag |= TRG_CHAR_STOP_KEYS;
    return XCMD_DONE;
}

// CT3 0x018f3e60: the player's keys work again.
int tnXcmd_PlayerControllStart(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    TrgCharStopFlag &= ~TRG_CHAR_STOP_KEYS;
    return XCMD_DONE;
}

// CT3 0x018f3e80: damage Alyssa by argv[0] (PlayerDamage's answer is dropped).
int tnXcmd_PlayerDamage(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    PlayerDamage((u_short)atoi(argv[0]));
    return XCMD_DONE;
}

// CT3 0x018f3eb0: argv[0] percent of the time (tnRand(100) <= argv[0]) register the event argv[1] (global
// flag argv[2]), otherwise remove it. Unlike the lever, the event and flag are not cut to 16 bits.
int tnXcmd_Probability(int argc, char **argv, int bank)
{
    (void)argc;
    int percent = (u_short)atoi(argv[0]);
    if (percent < tnRand(PROBABILITY_RANGE)) {
        int event = atoi(argv[1]);
        ssTrigRemoveEvent(event, atoi(argv[2]), bank);
    } else {
        int event = atoi(argv[1]);
        ssTrigRegisterEvent(event, atoi(argv[2]), bank);
    }
    return XCMD_DONE;
}
