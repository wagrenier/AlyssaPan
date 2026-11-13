// game/hhXcmd_BGM_SE_VOLUME.c -- Clock Tower 3: the option menu script's sound command (0x018dc1e0..
// 0x018dc330, one function, its object's own symtab chunk 131). The script keeps the menu's sound
// settings in trigger state -- event 0xa8 (monaural) and variables 0x4e / 0x4f (BGM / SE volume, 0..20)
// -- and this command copies them to the sound buffers and applies one SSSetSoundMode, or resets all three
// to the factory values. Written 2026-11-13.

#include <stdlib.h>
#include <string.h>
#include "game/hhXcmd_BGM_SE_VOLUME.h"
#include "game/xcmd_tn.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/Character_Init.h"
#include "game/unassigned_data.h"

// CT3 0x018dc1e0: argv[0] "SET" or "set": read the menu's settings from the trigger state (each volume
// cut to 20 as a byte, so a negative variable reads 20 too) and apply SSSetSoundMode(atoi(argv[1])) -- one
// SOUND_MODE, or SOUND_MODE_ALL. Any other argv[0]: stereo, BGM 20, SE 18, mode SOUND_MODE_ALL. Both ways
// the trigger state is then written back from the buffers.
int hhXcmd_BGM_SE_VOLUME(int argc, char **argv, int bank)
{
    int mode;

    (void)argc;
    (void)bank;
    if (strcmp("SET", argv[0]) == 0 || strcmp("set", argv[0]) == 0) {
        SOUND_STEREO_MONAURAL_BUFF = (u_char)ssTrigCheckEvent(TRIG_EVENT_MONAURAL, 1, 0);
        BGM_MASTER_VOLUME_BUFF = (u_char)ssTrigGetVariable(TRIG_VAR_BGM_VOLUME, 1, 0);
        SE_MASTER_VOLUME_BUFF = (u_char)ssTrigGetVariable(TRIG_VAR_SE_VOLUME, 1, 0);
        if (BGM_MASTER_VOLUME_BUFF > SOUND_MENU_VOLUME_STEPS) {
            BGM_MASTER_VOLUME_BUFF = SOUND_MENU_VOLUME_STEPS;
        }
        if (SE_MASTER_VOLUME_BUFF > SOUND_MENU_VOLUME_STEPS) {
            SE_MASTER_VOLUME_BUFF = SOUND_MENU_VOLUME_STEPS;
        }
        mode = atoi(argv[1]);
    } else {
        SOUND_STEREO_MONAURAL_BUFF = 0;
        BGM_MASTER_VOLUME_BUFF = SOUND_BGM_VOLUME_DEFAULT;
        mode = SOUND_MODE_ALL;
        SE_MASTER_VOLUME_BUFF = SOUND_SE_VOLUME_DEFAULT;
    }
    if (SOUND_STEREO_MONAURAL_BUFF == 0) {
        ssTrigRemoveEvent(TRIG_EVENT_MONAURAL, 1, 0);
    } else {
        ssTrigRegisterEvent(TRIG_EVENT_MONAURAL, 1, 0);
    }
    ssTrigSetVariable(TRIG_VAR_BGM_VOLUME, BGM_MASTER_VOLUME_BUFF, 1, 0);
    ssTrigSetVariable(TRIG_VAR_SE_VOLUME, SE_MASTER_VOLUME_BUFF, 1, 0);
    SSSetSoundMode(mode);
    return XCMD_DONE;
}
