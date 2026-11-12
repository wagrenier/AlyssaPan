// game/asMothPause.c -- CT3 object 85 of the symtab (0x01883ca0..0x01883da4): asMothPause and its static
// gAlysPos$61 (sbss 0x0193cec0, just before MothInfo).

#include "game/asMothPause.h"
#include "game/asMothRouteSearch.h"
#include "game/BRANCH_CLEAR.h"
#include "game/unassigned_data.h"

// CT3 0x01883ca0
void asMothPause(int mode)
{
    static qword gAlysPos;              /* gAlysPos$61, sbss 0x0193cec0: Alyssa's position at the pause */
    int i;

    if (!MothInfo.active || BankInfo[MothInfo.bank].drawOn <= 0)
        return;

    MothInfo.paused = mode;
    if (mode != MOTH_PAUSE_OFF) {
        gAlysPos[0] = Character[0].pos[0];
        gAlysPos[1] = Character[0].pos[1];
        gAlysPos[2] = Character[0].pos[2];
        gAlysPos[3] = Character[0].pos[3];
        return;
    }

    if (asVectorDistance2((const float *)gAlysPos, (const float *)Character[0].pos) > MOTH_PAUSE_MOVED) {
        for (i = 0; i < MothInfo.count; i++) {
            MOTH *moth = &MothInfo.list[i];

            if (moth->state == MOTH_STATE_5) {
                moth->w02 = MOTH_RELEASE_W02;
                moth->w04 = 0;
                moth->w01 = MOTH_RELEASE_W01;
            }
        }
        if (MothInfo.clatter != -1)
            ControllerClatteringFree(MothInfo.clatter);
    }
}
