// game/hhXcmd_BGM_SE_VOLUME.h -- CT3 game/hhXcmd_BGM_SE_VOLUME.c (0x018dc1e0..0x018dc330): the option
// menu script's sound command. Written 2026-11-13.
#pragma once

#include "sdk/scetypes.h"

// The sound settings' factory values (the command's reset branch; main.c's system_init sets the same).
#define SOUND_BGM_VOLUME_DEFAULT 20     /* BGM_MASTER_VOLUME_BUFF, the top of 0..SOUND_MENU_VOLUME_STEPS */
#define SOUND_SE_VOLUME_DEFAULT 18      /* SE_MASTER_VOLUME_BUFF */

// ssXcmd_functions slot 191 (ssTrigExecuteActCmd's command 0x514, xcmd_tn.h's SS_XCMD shape).
int hhXcmd_BGM_SE_VOLUME(int argc, char **argv, int bank);                               // CT3 0x018dc1e0
