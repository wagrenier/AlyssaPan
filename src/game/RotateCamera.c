// game/RotateCamera.c -- Clock Tower 3: two trigger-script x commands that turn a scene bank's parts
// (0x018d9a40..0x018da090; symtab local chunk 127, the statics LDeg1$64, LDeg2$65, CameLang$66):
// RotateCamera keeps a part turned towards Alyssa while an event is set; LockerCamera is a look-around
// (one stick pans and tilts, the other zooms, four held actions pan and tilt without the stick) -- the
// name says a locker, the hiding place. Each takes the command's argument strings (argv[0..argc-1]) and
// the bank whose script runs it; ssTrigExecuteActCmd (ssGrpPortalCheck.c) calls them through
// ssXcmd_functions. Both written 2026-11-13.

#include <math.h>
#include <stdlib.h>
#include "game/RotateCamera.h"
#include "game/xcmd_tn.h"
#include "game/unassigned_data.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ssGrpPortalCheck.h"

// ---- statics (symtab LOCAL) ----------------------------------------------------------------------------
static float LDeg1;             /* sbss 0x019447b8 (LDeg1$64): the pan, degrees, the first part about Y */
static float LDeg2;             /* sbss 0x019447bc (LDeg2$65): the tilt, degrees, the second part about X */
static float CameLang = 1.0f;   /* sdata 0x0193a900 (CameLang$66): the zoom, the second part's initial
                                   translation scaled by it */

// LockerCamera's numbers (named from this use; which way each action turns on screen is not yet seen).
#define LOCKER_STICK_LOOK 0             /* asGetAnalogStickInformation stick 0: the pair that looks */
#define LOCKER_STICK_ZOOM STICK_FIRST   /* stick 1: the pair that zooms */
#define LOCKER_STICK_DEAD 0x28          /* the dead zone both sticks are read with */
#define LOCKER_STEP 2.0f                /* degrees per frame, stick or actions */
#define LOCKER_ZOOM_STEP 0.1f
#define LOCKER_ZOOM_MIN 0.5f            /* the zoom steps down while above this */
#define LOCKER_ZOOM_MAX 1.5f            /* and up while below this */
#define LOCKER_ACT_TILT_DEC 0x0c        /* KEY_MAP actions, read held (TRIG_INPUT_CNT) */
#define LOCKER_ACT_PAN_DEC 0x0d
#define LOCKER_ACT_TILT_INC 0x0e
#define LOCKER_ACT_PAN_INC 0x0f
#define LOCKER_ACT_LEAVE_0 5            /* argv[2] 0: the action that would end the view */
#define LOCKER_ACT_LEAVE_1 6            /* argv[2] 1 (any other value: action 0) */
#define LOCKER_LEAVE_MODE 1             /* the mode the leave check passes: no TRIG_INPUT_* (see below) */

// CT3 0x018d9a40: turn a part about Y towards Alyssa while the bank's temp event argv[1] is set. argv[0]
// the part, argv[2] degrees added to the bearing. Answers XCMD_WAIT while it turns (every frame the event
// stays set) and XCMD_DONE once the event is clear.
int RotateCamera(int argc, char **argv, int bank)
{
    (void)argc;
    alignas(16) float alyssa[4];
    alignas(16) float object[4];

    if (!ssTrigCheckEvent(atoi(argv[1]), TRIG_SET_TEMP, bank))
        return XCMD_DONE;

    asGetAlyssaPosition(alyssa);
    asGetObjectPositionAtWorld(object, All_table_TBL[bank], atoi(argv[0]));
    u_short bearing = (u_short)asRadianByPositions_Rorate(alyssa, object, 0, ROTATE_AXIS_Y);
    int part = atoi(argv[0]);
    int offset = atoi(argv[2]);
    asObjectRotate((360.0f * (float)bearing) / 65536.0f + (float)offset, All_table_TBL[bank], part,
                   ROTATE_SET, ROTATE_AXIS_Y);
    return XCMD_WAIT;
}

// CT3 0x018d9b50: the look-around. argv[0] the part turned about Y (the pan), argv[1] the part tilted about
// X and moved for the zoom, argv[2] which action would end the view (0 / 1), argv[3] the pan's rest angle,
// argv[4] / argv[5] the pan's lower / upper limits (argv[4] negated), argv[6] / argv[7] the tilt's,
// argv[8] the bank's temp event that ends it. When the event is set the parts go back to rest, the
// statics reset and the command is done; otherwise one frame of looking: XCMD_WAIT.
int LockerCamera(int argc, char **argv, int bank)
{
    (void)argc;
    float degrees = 0.0f;
    float magnitude = 0.0f;
    alignas(16) float trans[4];

    if (ssTrigCheckEvent(atoi(argv[8]), TRIG_SET_TEMP, bank)) {
        int part = atoi(argv[0]);
        asObjectRotate(-(float)atoi(argv[3]), All_table_TBL[bank], part, ROTATE_SET, ROTATE_AXIS_Y);
        asObjectRotate(0.0f, All_table_TBL[bank], atoi(argv[1]), ROTATE_SET, ROTATE_AXIS_X);
        CameLang = 1.0f;
        LDeg1 = 0.0f;
        LDeg2 = 0.0f;
        return XCMD_DONE;
    }

    int leave;
    switch (atoi(argv[2])) {
    case 1:
        leave = LOCKER_ACT_LEAVE_1;
        break;
    case 0:
        leave = LOCKER_ACT_LEAVE_0;
        break;
    default:
        leave = 0;
        break;
    }
    // The ROM passes mode 1, which ssTrigCheckInput answers 0 for (only 0 / 4 / 8 read a pad word): this
    // way out never fires, the event above ends the view. Kept as the ROM does.
    if (ssTrigCheckInput(leave, LOCKER_LEAVE_MODE)) {
        LDeg1 = 0.0f;
        CameLang = 1.0f;
        LDeg2 = 0.0f;
        return XCMD_DONE;
    }

    // Zoom: the second part's initial translation scaled (xyz) by CameLang; the zoom stick within 90
    // degrees of 0 steps it down, the other half up.
    asGetObjectTransInitialValue(trans, All_table_TBL[bank], atoi(argv[1]));
    asGetAnalogStickInformation(LOCKER_STICK_ZOOM, LOCKER_STICK_DEAD, &degrees, &magnitude);
    if (magnitude > 0.0f) {
        if (degrees > 270.0f || degrees < 90.0f) {
            if (CameLang > LOCKER_ZOOM_MIN)
                CameLang -= LOCKER_ZOOM_STEP;
        } else if (CameLang < LOCKER_ZOOM_MAX) {
            CameLang += LOCKER_ZOOM_STEP;
        }
    }
    trans[0] *= CameLang;       // VU0 vmulx.xyz: w kept
    trans[1] *= CameLang;
    trans[2] *= CameLang;
    asSetObjectTrans(All_table_TBL[bank], atoi(argv[1]), trans);

    float panMax = (float)atoi(argv[5]);
    float panMin = -(float)atoi(argv[4]);
    float tiltMax = (float)atoi(argv[7]);
    float tiltMin = -(float)atoi(argv[6]);

    asGetAnalogStickInformation(LOCKER_STICK_LOOK, LOCKER_STICK_DEAD, &degrees, &magnitude);
    if (magnitude > 0.0f) {
        LDeg2 -= cosf((degrees * 6.2831855f) / 360.0f) * LOCKER_STEP;
        LDeg1 -= sinf((degrees * 6.2831855f) / 360.0f) * LOCKER_STEP;
        if (LDeg2 < tiltMin)
            LDeg2 = tiltMin;
        if (tiltMax < LDeg2)
            LDeg2 = tiltMax;
        if (LDeg1 < panMin)
            LDeg1 = panMin;
        if (panMax < LDeg1)
            LDeg1 = panMax;
    } else {
        if (ssTrigCheckInput(LOCKER_ACT_TILT_DEC, TRIG_INPUT_CNT) && tiltMin < LDeg2)
            LDeg2 -= LOCKER_STEP;
        if (ssTrigCheckInput(LOCKER_ACT_TILT_INC, TRIG_INPUT_CNT) && LDeg2 < tiltMax)
            LDeg2 += LOCKER_STEP;
        if (ssTrigCheckInput(LOCKER_ACT_PAN_DEC, TRIG_INPUT_CNT) && panMin < LDeg1)
            LDeg1 -= LOCKER_STEP;
        if (ssTrigCheckInput(LOCKER_ACT_PAN_INC, TRIG_INPUT_CNT) && LDeg1 < panMax)
            LDeg1 += LOCKER_STEP;
    }

    int part = atoi(argv[0]);
    asObjectRotate(LDeg1 - (float)atoi(argv[3]), All_table_TBL[bank], part, ROTATE_SET, ROTATE_AXIS_Y);
    asObjectRotate(LDeg2, All_table_TBL[bank], atoi(argv[1]), ROTATE_SET, ROTATE_AXIS_X);
    return XCMD_WAIT;
}
