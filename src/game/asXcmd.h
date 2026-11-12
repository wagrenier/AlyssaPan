// game/asXcmd.h -- CT3 game/asXcmd.c (0x018dab60..0x018dc1e0): the trigger script's x commands by "as"
// (panic, moths, arrows, hit points, the camera's quake / reset / look-around, attaching parts, aiming,
// seek bars, motions) and the attack-target list. Written 2026-11-11: all 20 functions.
#pragma once

#include "sdk/scetypes.h"

// ---- AttackTargetStack (sbss 0x0193fa30, 64 bytes) ------------------------------------------------------
// The targets a script locked on: pairs of (the bank whose script added it, the number the script gave).
// asXcmd_AddForLockToTargChara adds and clears them; the reader is not written yet (open-items.md 1).
#define ATTACK_TARGET_MAX 16
typedef struct ATTACK_TARGET {
    u_short bank;                       /* 0x0: the adding script's bank; 0 = a free entry */
    u_short target;                     /* 0x2: the script's number (a Character[] slot, by the name) */
} ATTACK_TARGET;
static_assert(sizeof(ATTACK_TARGET) == 4, "ATTACK_TARGET");
extern ATTACK_TARGET AttackTargetStack[ATTACK_TARGET_MAX];  /* sbss 0x0193fa30 */

// asXcmd_AddForLockToTargChara's argc modes beside "add argc targets".
#define ATTACK_TARGET_CLEAR_ALL 0               /* argc 0: every entry free (main: scene start / end) */
#define ATTACK_TARGET_DROP_BANK 100000000       /* argc >= this: drop bank's entries and those of banks no
                                                   longer loaded (main: a scene bank's end) */

// asXcmd_PanicSet's source names (@349..@357), in PanicSet's source order (0..8).
#define PANIC_SOURCE_NAMES 9

// asXcmd_ALYSSA_ComeHere: Character[0] turns to a part of the script's bank and plays a motion.
#define COME_HERE_AIM_SPEED 0x6d                /* CHARA_AIM_SPEED (asPIA0_KEY_TBL_PULLED's too) */
#define CHARA_COME_HERE_MOTION 0x518            /* int: the motion it plays (CHARA_BHV_W518), -1 = none */

// asXcmd_I_have_x_Arrows: an arrow part waits far below the world until it is shot.
#define ARROW_PART_FLAGS43_ARMED 3              /* MODEL_PARTS.flags43: PARTS_FLAG43_WARPED | _TRANS */
#define ARROW_PART_PARKED_Y (-100000000.0f)     /* MODEL_PARTS.drawOnPos[1] (0xccbebc20) */
#define ARROW_SE_BANK "T_0010"                  /* @133 (sdata 0x0193a908): the bow's SEs */

// asXcmd_AttachObject: the attach record over a part's MODEL_PARTS 0x70..0x75 (the words a point light's
// part uses as omniChanging / omniSteps: a part is one kind or the other).
#define PARTS_KIND_ATTACHED 0x1e                /* MODEL_PARTS.kind of an attached part */
#define PARTS_ATTACH_BANK 0x70                  /* short: the bank it follows, -1 none */
#define PARTS_ATTACH_PART 0x72                  /* short: that bank's part */
#define PARTS_ATTACH_MODE 0x74                  /* short: the command's mode (ATTACH_MODE_0 / _1) */
enum ATTACH_MODE {                              /* the command's 2nd argument */
    ATTACH_MODE_0 = 0,                          /* attach (PARTS_ATTACH_MODE 0) */
    ATTACH_MODE_1 = 1,                          /* attach (PARTS_ATTACH_MODE 1) */
    ATTACH_MODE_HIDE = 2,                       /* no attachment; PARTS_FLAG_HIDDEN set */
};
#define ATTACH_TO_BANK 0                        /* 3rd argument: the 4th is a bank; else a Character[] slot */

// The 16-bit angle of `deg` degrees (the commands' angle arguments: 0x10000 a turn).
#define XCMD_DEG_TO_ANGLE(deg) ((int)(((float)(deg) * 65536.0f) / 360.0f) & 0xffff)
#define AUTO_AIM_SPEED_DIV 300                  /* asXcmd_CharacterAutoAiming: CHARA_AIM_SPEED = angle / 300 */
#define AUTO_AIM_OWN_BANK (-1)                  /* its 2nd argument: the script's own bank */

// asXcmd_CameraGuriGuriSetting's first argument (0 / 1: CameGri_Mode, the stick pair camera_view reads).
#define CAMEGRI_CMD_OFF (-1)                    /* CameGri_Mode CAMEGRI_MODE_OFF */
#define CAMEGRI_CMD_RECENTRE (-2)               /* the eased deflection back to 0 */
#define CAMEGRI_MODE_OFF 0xff

#define SEEKBAR_FRAME_SCALE 10                  /* asXcmd_SeekBarSetting: the script's frames * 10 */
#define CAM_QUAKE_ARGC_NO_PHASE 6               /* asXcmd_CameraQuake: six arguments, phase 0 */

// Called from main.c with no argv (argc ATTACK_TARGET_CLEAR_ALL or ATTACK_TARGET_DROP_BANK).
int asXcmd_AddForLockToTargChara(int argc, char **argv, int bank);                       // CT3 0x018db340

// The x commands (ssXcmd_functions slots, ssTrigExecuteActCmd's command 0x514): argv[0..argc-1] are the
// command's argument strings, bank the bank whose script runs it; each answers XCMD_DONE.
int asXcmd_ShadowOrderBW(int argc, char **argv, int bank);                               // CT3 0x018dc1b0
int asXcmd_PlayMotionByNowMotion(int argc, char **argv, int bank);                       // CT3 0x018dc050
int asXcmd_CameraGuriGuriSetting(int argc, char **argv, int bank);                       // CT3 0x018dbf40
int asXcmd_SeekBarSetting(int argc, char **argv, int bank);                              // CT3 0x018dbd80
int asXcmd_CharacterAutoAiming(int argc, char **argv, int bank);                         // CT3 0x018dbc20
int asXcmd_I_have_x_Arrows(int argc, char **argv, int bank);                             // CT3 0x018dba30
int asXcmd_ShootArrow(int argc, char **argv, int bank);                                  // CT3 0x018dba20
int asXcmd_AttachObject(int argc, char **argv, int bank);                                // CT3 0x018db880
int asXcmd_CameraQuake(int argc, char **argv, int bank);                                 // CT3 0x018db7b0
int asXcmd_SlowMotion(int argc, char **argv, int bank);                                  // CT3 0x018db770
int asXcmd_SetHitPoint(int argc, char **argv, int bank);                                 // CT3 0x018db580
int asXcmd_SettingCameraReset(int argc, char **argv, int bank);                          // CT3 0x018db510
int asXcmd_I_have_x_Mothes(int argc, char **argv, int bank);                             // CT3 0x018db160
int asXcmd_SetMoth(int argc, char **argv, int bank);                                     // CT3 0x018db000
int asXcmd_ForceOperateAnimation(int argc, char **argv, int bank);                       // CT3 0x018daf80
int asXcmd_ForceOperateMirroring(int argc, char **argv, int bank);                       // CT3 0x018daf30
int asXcmd_ALYSSA_ComeHere(int argc, char **argv, int bank);                             // CT3 0x018dae40
int asXcmd_DistanceBetweenChar(int argc, char **argv, int bank);                         // CT3 0x018dad50
int asXcmd_PanicSet(int argc, char **argv, int bank);                                    // CT3 0x018dab60
