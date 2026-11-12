// game/tiAvoidObject.c -- Clock Tower 3: the stalkers' AI (ssAI_*; symtab local chunk 74). CodeWarrior C.
// Slice 1 (2026-11-12): the appearance and no-reaction timers, the global init, the save image's part,
// the brain's free, the motion predicates (crawl, door push / pull) and ssAI_GetScnBankID. The frame's
// driver (ssAI_main), the brains' modes and the scene init are later slices: stubs below.

#include "common/alyssapan_stub.h"
#include "game/tiAvoidObject.h"
#include "game/ahalloc.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/BRANCH_CLEAR.h"
#include "game/Character_Init.h"
#include "game/ct3_types.h"
#include "game/fileio.h"
#include "game/LinkMatrixTree.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/wideXcmd_TriggerInit.h"

#include <string.h>

int ssNoReactionTimer;                  /* sbss 0x0193ce90 (symtab local): ticks the stalkers ignore Alyssa */
int AppearanceTimer;                    /* sbss 0x0193ce94 (symtab local): ticks to the next appearance */

// The character's current motion (CHARA_MOTION_SLOTS[0], a motion table index).
static short ssAI_Motion(const CHARACTER *chr)
{
    short m;
    memcpy(&m, (const u_char *)chr + CHARA_MOTION_SLOTS, sizeof(m));
    return m;
}

// CT3 0x01860860: 1 while Alyssa (id 0) plays one of her crawl motions; 0 for anyone else.
int ssAI_IsHaiHaiPlaying(CHARACTER *chr)
{
    short m;

    if (chr->id != CHARA_ID_ALYSSA)
        return 0;
    m = ssAI_Motion(chr);
    if (m == MOTION_HAIHAI_C0 || m == MOTION_HAIHAI_B6 || m == MOTION_HAIHAI_STEP2 || m == MOTION_HAIHAI_STEP ||
        m == MOTION_HAIHAI_11 || m == MOTION_HAIHAI_10)
        return 1;
    return 0;
}

// CT3 0x01860d00
int ssAI_IsDoorOpenPlaying(CHARACTER *chr)
{
    if (ssAI_IsDoorPushOpenPlaying(chr) != 0)
        return 1;
    return ssAI_IsDoorPullOpenPlaying(chr) != 0;
}

// CT3 0x01860d40: 1 while the character plays its door-pull motion (each kind has its own index).
int ssAI_IsDoorPullOpenPlaying(CHARACTER *chr)
{
    switch (chr->id) {
    case CHARA_ID_ALYSSA:
        return ssAI_Motion(chr) == MOTION_DOOR_PULL_ALYSSA;
    case STALKER_ID_2:
        return ssAI_Motion(chr) == MOTION_DOOR_PULL_2;
    case STALKER_ID_3:
    case STALKER_ID_4:
    case STALKER_ID_6:
        return ssAI_Motion(chr) == MOTION_DOOR_PULL_346;
    case STALKER_ID_5:
        return ssAI_Motion(chr) == MOTION_DOOR_PULL_5;
    default:
        return 0;
    }
}

// CT3 0x01860e40: the push counterpart (every index one below its pull).
int ssAI_IsDoorPushOpenPlaying(CHARACTER *chr)
{
    switch (chr->id) {
    case CHARA_ID_ALYSSA:
        return ssAI_Motion(chr) == MOTION_DOOR_PUSH_ALYSSA;
    case STALKER_ID_2:
        return ssAI_Motion(chr) == MOTION_DOOR_PUSH_2;
    case STALKER_ID_3:
    case STALKER_ID_4:
    case STALKER_ID_6:
        return ssAI_Motion(chr) == MOTION_DOOR_PUSH_346;
    case STALKER_ID_5:
        return ssAI_Motion(chr) == MOTION_DOOR_PUSH_5;
    default:
        return 0;
    }
}

// CT3 0x01862770: the first loaded scene bank (BANK_TYPE_SHADOW) whose room is roomHash; -1 for none.
// A bank group is skipped whole (groupCount).
int ssAI_GetScnBankID(u_int roomHash)
{
    int bank = 0;

    do {
        BANK_INFO *bi = &BankInfo[bank];
        if (bi->state != BANK_STATE_FREE && bi->type == BANK_TYPE_SHADOW &&
            (u_long)roomHash == GetRoomName_file(bi->name))
            return bank;
        if (bi->state != BANK_STATE_FREE)
            bank += bi->groupCount - 1;
        bank++;
    } while (bank < BANK_MAX);
    return -1;
}

// CT3 0x01863c20: _SunFree(&gHeapJunc, &chr->brain) -- the ROM's free clears the word.
// PORT: the word is a handle (CharaBrainPtr): free the host block, then clear the handle.
void ssAI_BrainFree(CHARACTER *chr)
{
    if (chr->brain == 0)
        return;
    _SunFree(&gHeapJunc, (void **)&CharaBrainHost[chr->brain - 1]);
    chr->brain = 0;
}

// CT3 0x01865df0: the brain ends (BrainEnd's arguments passed through) and the next stalker may come at once.
void ssAI_ForceBrainEnd(CHARACTER *chr, int motion)
{
    ssAI_BrainEnd(chr, motion);
    ssAI_SetAppearanceTimer(0);
}

// CT3 0x01866090: empty in the ROM.
void ssAI_CharaLoadInit(void)
{
}

// CT3 0x018662a0: the timers into the save image.
void ssAI_MemSaveData(u_char *data)
{
    memcpy(data + MC_DATA_AI_APPEARANCE_TIMER, &AppearanceTimer, sizeof(int));
    memcpy(data + MC_DATA_AI_NO_REACTION_TIMER, &ssNoReactionTimer, sizeof(int));
}

// CT3 0x018662d0: the timers back from a save image; every stalker loses its brain (its event removed,
// without a free: the load rebuilt the heap) and is parked out of the world.
void ssAI_MemLoadInit(u_char *data)
{
    int i;

    memcpy(&AppearanceTimer, data + MC_DATA_AI_APPEARANCE_TIMER, sizeof(int));
    memcpy(&ssNoReactionTimer, data + MC_DATA_AI_NO_REACTION_TIMER, sizeof(int));
    for (i = 0; i < ATK_CHARA_LIST2_COUNT; i++) {
        int slot = ssAtkCharaList2[i];
        CHARACTER *chr = &Character[slot];
        float *pos = (float *)&chr->pos;
        float *posBefore = (float *)&chr->posBefore;
        if (chr->brain != 0) {
            s_char id = chr->id;
            if (id == AI_ID_1F)
                ssTrigRemoveEvent(TRIG_EVENT_AI_1F, 1, -1);
            else if (id == AI_ID_1E)
                ssTrigRemoveEvent(TRIG_EVENT_AI_1E, 1, -1);
            else if (id == AI_ID_1D)
                ssTrigRemoveEvent(TRIG_EVENT_AI_1D, 1, -1);
            else if (id == STALKER_ID_6 || id == STALKER_ID_5 || id == STALKER_ID_4 || id == STALKER_ID_3 ||
                     id == STALKER_ID_2) {
                ssTrigRemoveEvent(TRIG_EVENT_CHASE, 1, -1);
                ssAI_SetAppearanceTimer(1);
            }
            // PORT: the ROM clears the word only; the host clears its slot with it (the block is gone).
            CharaBrainSet(chr, slot, NULL);
        }
        chr->lightBank = -1;
        chr->roomHash = 0;
        chr->h100 = AI_H100_NONE;
        pos[0] = posBefore[0] = AI_PARK_X;
        pos[1] = posBefore[1] = AI_PARK_Y;
        pos[2] = posBefore[2] = 0.0f;
        chr->flags |= CHARACTER_FLAG_AI_PARKED;
        chr->flags &= ~(CHARACTER_FLAG_REFRACTION | CHARACTER_FLAG_SHADOW);
    }
}

// CT3 0x01866490: at system init: the axe throw, the first appearance, the panic events.
void ssAI_GlobalInit(void)
{
    ssAI_AXE_Init();
    ssAI_SetAppearanceTimer(AI_FIRST_APPEARANCE);
    ssTrigRegisterEvent(TRIG_EVENT_PANIC_NO_RELOAD, 1, -1);
    ssTrigRegisterEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
    ssTrigRemoveEvent(TRIG_EVENT_AI_APPEAR, 1, -1);
}

// CT3 0x018664f0
void ssAI_SetNoReactionTimer(int time)
{
    ssNoReactionTimer = time * AI_TICKS_PER_UNIT;
}

// CT3 0x01866510
void ssAI_SetAppearanceTimer(int time)
{
    AppearanceTimer = time * AI_TICKS_PER_UNIT;
}

// CT3 0x01866530: each frame (ssAI_main) unless the game was just reloaded or a chase is on: the
// no-reaction time runs down first (once it is out, TRIG_EVENT_AI_APPEAR_NOW cuts the wait), then the
// appearance time; at its end TRIG_EVENT_AI_APPEAR is set and the next wait is AI_REAPPEARANCE.
void ssAI_CountAppearanceTimer(void)
{
    if (ssTrigCheckEvent(TRIG_EVENT_MC_RELOADED, 1, -1) != 0)
        return;
    if (ssTrigCheckEvent(TRIG_EVENT_CHASE, 1, -1) != 0)
        return;
    if (ssNoReactionTimer < 1) {
        if (ssTrigCheckEvent(TRIG_EVENT_AI_APPEAR_NOW, 1, -1) != 0)
            AppearanceTimer = 0;
    } else {
        ssNoReactionTimer -= gNextTicksInterval;
    }
    AppearanceTimer -= gNextTicksInterval;
    if (AppearanceTimer < 1) {
        ssTrigRegisterEvent(TRIG_EVENT_AI_APPEAR, 1, -1);
        ssAI_SetAppearanceTimer(AI_REAPPEARANCE);
    }
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x01863ed0
void ssAI_main(void)
{
    ALYSSAPAN_STUB("ssAI_main", 0x01863ed0);
}

// stub of CT3 0x018660a0
void ssAI_SceneInit(int scene)
{
    ALYSSAPAN_STUB("ssAI_SceneInit", 0x018660a0);
}

// stub of CT3 0x01865e20
void ssAI_BrainEnd(struct CHARACTER *chara, int brain)
{
    ALYSSAPAN_STUB("ssAI_BrainEnd", 0x01865e20);
}

// stub of CT3 0x0185f710
void ssAI_MonsterDraw(struct CHARACTER *chr, int draw)
{
    ALYSSAPAN_STUB("ssAI_MonsterDraw", 0x0185f710);
}

// stub of CT3 0x018642b0
void ssAI_AxeThrowReset(void)
{
    ALYSSAPAN_STUB("ssAI_AxeThrowReset", 0x018642b0);
}
