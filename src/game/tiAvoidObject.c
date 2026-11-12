// game/tiAvoidObject.c -- Clock Tower 3: the stalkers' AI (ssAI_*; symtab local chunk 74). CodeWarrior C.
// Slice 1 (2026-11-12): the appearance and no-reaction timers, the global init, the save image's part,
// the brain's free, the motion predicates (crawl, door push / pull) and ssAI_GetScnBankID. Slice 2: the
// hide poles and the scene init. Slice 3: the frame's driver (ssAI_main), the mode tables and dispatch,
// the facing, the panic rise, the hearing / hide / door checks, the motion predicates. Stubs below.

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
#include "game/main.h"
#include "game/main2.h"
#include "game/ssBrain_ChgHideAtk1.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/tiBrain_ChgTrace_BOSS_BALOWS.h"
#include "game/tiBrain_ChgTrace_Ghost.h"
#include "ga/gaInit3dlib.h"

#include <float.h>
#include <math.h>
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

// ---- slice 2 (2026-11-12): the hide poles and the scene init ----

// The bank's hide poles (ALL_TABLE.hidePoles: { records, count }, relocated bank data).
static HIDE_POLE_LIST *ssAI_PoleList(int bank)
{
    return BANK_PTR(HIDE_POLE_LIST *, bank, All_table_TBL[bank]->hidePoles);
}

static HIDE_POLE *ssAI_Pole(int pole, int bank)
{
    return BANK_PTR(HIDE_POLE *, bank, ssAI_PoleList(bank)->poles) + pole;
}

// The VU0 macro sequence vmulax / vmadday / vmaddaz / vmaddw: out = m[0]*x + m[1]*y + m[2]*z + m[3]*w.
static void ssAI_ApplyMatrix(float *out, const float (*m)[4], const float *v)
{
    float r[4];
    int i;

    for (i = 0; i < 4; i++)
        r[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
    memcpy(out, r, sizeof(r));
}

// CT3 0x018641e0: every hide pole's ends into the world with its part's world matrix.
void ssAI_TransAllPoleData(int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);
    int i;

    for (i = 0; i < list->count; i++) {
        HIDE_POLE *p = &poles[i];
        MODEL_PARTS *part = (MODEL_PARTS *)(pModelPartsHeader[bank] + p->parts * PARTS_REC_SIZE);
        ssAI_ApplyMatrix(p->posA, (const float (*)[4])part->matrix, p->localA);
        ssAI_ApplyMatrix(p->posB, (const float (*)[4])part->matrix, p->localB);
    }
}

// CT3 0x01863010: the pole's position: end A, or the middle of A and B for a segment.
void ssAI_GetPolePos(float *pos, int pole, int bank)
{
    HIDE_POLE *p = ssAI_Pole(pole, bank);

    if ((p->flags & HIDE_POLE_FLAG_SEGMENT) == 0)
        memcpy(pos, p->posA, sizeof(p->posA));
    else
        gaInterVectorXYZ(pos, p->posA, p->posB, 0.5f);
}

// CT3 0x01862f20
short ssAI_GetPoleGroupID(int pole, int bank)
{
    return ssAI_Pole(pole, bank)->group;
}

// CT3 0x01862830
u_int ssAI_GetPoleScnName(int pole, int bank)
{
    return ssAI_Pole(pole, bank)->room;
}

// CT3 0x01862860: the loaded scene bank of the pole's room (the ROM returns ssAI_GetScnBankID's answer as
// a char: -1 stays -1).
s_char ssAI_GetPoleScnBnkID(int pole, int bank)
{
    return (s_char)ssAI_GetScnBankID(ssAI_Pole(pole, bank)->room);
}

// CT3 0x018628b0: the first shadow pole of the bank on shadow polygon shadowPol; -1 for none.
short ssAI_GetShadowPolID(s_char shadowPol, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);
    short i;

    for (i = 0; i < list->count; i++) {
        if ((poles[i].flags & HIDE_POLE_FLAG_SHADOW) != 0 && shadowPol == poles[i].shadowPol)
            return i;
    }
    return -1;
}

// CT3 0x01862c40: the pole a scene link pairs this one with, -1 when the pair is not a scene link.
short ssAI_GetPairScnLinkPoleID(short pole, int bank)
{
    short pair = ssAI_Pole(pole, bank)->pair;

    if ((ssAI_Pole(pair, bank)->flags & HIDE_POLE_FLAG_SCN_LINK) == 0)
        pair = -1;
    return pair;
}

// CT3 0x018600a0
int ssAI_CheckPoleFlg(short pole, int bank, u_char flag)
{
    return (flag & ssAI_Pole(pole, bank)->flags) != 0;
}

// CT3 0x0185ff40: 1 when the bank has a shadow pole on shadow polygon shadowPol (0 for no bank).
int ssAI_IsGetShadowPol(u_char shadowPol, int bank)
{
    if (bank < AI_BANK_NONE) {
        HIDE_POLE_LIST *list = ssAI_PoleList(bank);
        HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);
        short i;

        for (i = 0; i < list->count; i++) {
            if ((poles[i].flags & HIDE_POLE_FLAG_SHADOW) != 0 && shadowPol == poles[i].shadowPol)
                return 1;
        }
    }
    return 0;
}

// CT3 0x018660a0: a scene bank loaded: its hide poles into the world, then each group portal's quad,
// normal and plane distance from its part's matrix (PARTS_PORTAL_MATRIX).
void ssAI_SceneInit(int scene)
{
    u_int *portals;
    int i;

    if (BankInfo[scene].type == BANK_TYPE_SHADOW)
        ssAI_TransAllPoleData(scene);
    if (BankInfo[scene].type != BANK_TYPE_SHADOW)
        return;
    portals = BANK_PTR(u_int *, scene, All_table_TBL[scene]->groupPortals);
    for (i = 0; portals[i] != 0; i++) {
        GRP_PORTAL *gp = BANK_PTR(GRP_PORTAL *, scene, portals[i]);
        u_int matEE;
        const float (*m)[4];

        memcpy(&matEE, pModelPartsHeader[scene] + gp->part * PARTS_REC_SIZE + PARTS_PORTAL_MATRIX,
               sizeof(matEE));
        m = BANK_PTR(const float (*)[4], scene, matEE);
        ssAI_ApplyMatrix(gp->normal, m, gp->localNormal);
        ssAI_ApplyMatrix(gp->corners[0], m, gp->localCorners[0]);
        ssAI_ApplyMatrix(gp->corners[1], m, gp->localCorners[1]);
        ssAI_ApplyMatrix(gp->corners[2], m, gp->localCorners[2]);
        ssAI_ApplyMatrix(gp->corners[3], m, gp->localCorners[3]);
        gp->dist = gaInnerProductXYZ(gp->normal, gp->corners[0]);
    }
}

// ---- slice 3 (2026-11-12): the frame's driver, the mode dispatch, the facing, the panic, the predicates ----

#define MOTION_KIND_BEND 1              /* MOTION_REC_KIND: ssAI_IsBendPlaying (asCheckAlyssaOnStairway's
                                           MOTION_KIND_STAIRS2, the same value) */
#define MOTION_KIND_DOWN 3              /* ssAI_IsDownPlaying (MOTION_KIND_STAIRS) */

// The mode tables (data 0x01938430..0x01938574, symtab globals): ssAI_charaMain indexes them by the
// brain's BRAIN_DAMAGE_FN / BRAIN_MODE_FN + mode / BRAIN_CHG_MODE_FN + mode bytes.
typedef int (*SS_AI_DAMAGE_FN)(CHARACTER *chr);
typedef void (*SS_AI_MODE_FN)(CHARACTER *chr);
SS_AI_DAMAGE_FN ssAI_DamageMode[5] = {                                 // CT3 0x01938430
    ssBrain_Damage1, tiBrain_Damage1, tiBrain_Damage2, tiBrain_Damage3, tiBrain_Damage_BOSS1,
};
SS_AI_MODE_FN ssAI_SearchMode[4] = {                                   // CT3 0x01938450
    ssBrain_Search1, tiBrain_Search_Ghost1, tiBrain_Search_Ghost2, tiBrain_Search_Ghost3,
};
SS_AI_MODE_FN ssAI_ApproachMode[4] = {                                 // CT3 0x01938460
    ssBrain_Approach1, tiBrain_Approach_Ghost1, tiBrain_Approach_BOSS1, tiBrain_Approach_Ghost2,
};
SS_AI_MODE_FN ssAI_TraceMode[7] = {                                    // CT3 0x01938470
    ssBrain_Trace1, tiBrain_Trace_BOSS_HAMM, tiBrain_Trace_BOSS_VITRIOL, tiBrain_Trace_Ghost1,
    tiBrain_Trace_BOSS_AXE, tiBrain_Trace_BOSS_SCISSORS_M, tiBrain_Trace_BOSS_BALOWS,
};
SS_AI_MODE_FN ssAI_AttackMode[11] = {                                  // CT3 0x01938490
    ssBrain_Attack1, tiBrain_Attack_Ghost1, tiBrain_Attack_Ghost2, tiBrain_Attack_Ghost3,
    tiBrain_Attack_BOSS_HAMM, tiBrain_Attack_BOSS_VITRIOL, tiBrain_Attack_BOSS_AXE,
    tiBrain_Attack_BOSS_SCISSORS_W, tiBrain_Attack_BOSS_SCISSORS_M, tiBrain_Attack_Ghost4,
    tiBrain_Attack_BOSS_BALOWS,
};
SS_AI_MODE_FN ssAI_ReturnMode[3] = {                                   // CT3 0x019384c0
    ssBrain_Return1, tiBrain_Return_Ghost1, tiBrain_Return_BOSS1,
};
SS_AI_MODE_FN ssAI_HideAtkMode[1] = { ssBrain_HideAtk1 };              // CT3 0x019384cc
SS_AI_MODE_FN ssAI_ChgSearchMode[4] = {                                // CT3 0x019384d0
    ssBrain_ChgSearch1, tiBrain_ChgSearch_Ghost1, tiBrain_ChgSearch_Ghost2, tiBrain_ChgSearch_Ghost3,
};
SS_AI_MODE_FN ssAI_ChgApproachMode[9] = {                              // CT3 0x019384e0
    ssBrain_ChgApproach1, tiBrain_ChgApproach_Ghost1, tiBrain_ChgApproach_BOSS_HAMM,
    tiBrain_ChgApproach_BOSS_VITRIOL, tiBrain_ChgApproach_BOSS_AXE, tiBrain_ChgApproach_BOSS_SCISSORS_W,
    tiBrain_ChgApproach_BOSS_SCISSORS_M, tiBrain_ChgApproach_Ghost2, tiBrain_ChgApproach_BOSS_BALOWS,
};
SS_AI_MODE_FN ssAI_ChgTraceMode[7] = {                                 // CT3 0x01938510
    ssBrain_ChgTrace1, tiBrain_ChgTrace_BOSS_HAMM, tiBrain_ChgTrace_BOSS_VITRIOL, tiBrain_ChgTrace_BOSS_AXE,
    tiBrain_ChgTrace_BOSS_SCISSORS_M, tiBrain_ChgTrace_BOSS_BALOWS, tiBrain_ChgTrace_Ghost,
};
SS_AI_MODE_FN ssAI_ChgAttackMode[11] = {                               // CT3 0x01938530
    ssBrain_ChgAttack1, tiBrain_ChgAttack_Ghost1, tiBrain_ChgAttack_Ghost2, tiBrain_ChgAttack_BOSS_HAMM,
    tiBrain_ChgAttack_Ghost3, tiBrain_ChgAttack_BOSS_VITRIOL, tiBrain_ChgAttack_BOSS_AXE,
    tiBrain_ChgAttack_BOSS_SCISSORS_W, tiBrain_ChgAttack_BOSS_SCISSORS_M, tiBrain_ChgAttack_Ghost4,
    tiBrain_ChgAttack_BOSS_BALOWS,
};
SS_AI_MODE_FN ssAI_ChgReturnMode[4] = {                                // CT3 0x01938560
    ssBrain_ChgReturn1, tiBrain_ChgReturn1, tiBrain_ChgReturn2, tiBrain_ChgReturn_BOSS1,
};
SS_AI_MODE_FN ssAI_ChgHideAtkMode[1] = { ssBrain_ChgHideAtk1 };        // CT3 0x01938570

// The six modes' tables in SS_AI_* order (the ROM switches on the mode; a mode past 5 runs nothing).
static SS_AI_MODE_FN *const ssAI_ModeTable[SS_AI_MODE_COUNT] = {
    ssAI_SearchMode, ssAI_ApproachMode, ssAI_TraceMode, ssAI_AttackMode, ssAI_ReturnMode, ssAI_HideAtkMode,
};
static SS_AI_MODE_FN *const ssAI_ChgModeTable[SS_AI_MODE_COUNT] = {
    ssAI_ChgSearchMode, ssAI_ChgApproachMode, ssAI_ChgTraceMode, ssAI_ChgAttackMode, ssAI_ChgReturnMode,
    ssAI_ChgHideAtkMode,
};

static short ssAI_BrainShort(const u_char *brain, int off)
{
    short v;
    memcpy(&v, brain + off, sizeof(v));
    return v;
}

static void ssAI_BrainSetShort(u_char *brain, int off, short v)
{
    memcpy(brain + off, &v, sizeof(v));
}

static float ssAI_BrainFloat(const u_char *brain, int off)
{
    float v;
    memcpy(&v, brain + off, sizeof(v));
    return v;
}

// The character's motion record (CHARA_MOTION_TABLE, relocated bank data: BankEePtr); NULL without one.
static const u_char *ssAI_MotionRecord(const CHARACTER *chr, int index)
{
    u_int table;
    const u_char *base;

    if (chr->bank < 0)
        return NULL;
    memcpy(&table, (const u_char *)chr + CHARA_MOTION_TABLE, sizeof(table));
    base = BANK_PTR(const u_char *, chr->bank, table);
    return base != NULL ? base + index * CHARA_MOTION_SIZE : NULL;
}

// The current motion record's kind (MOTION_REC_KIND); -1 when there is no record (the ROM reads it
// unconditionally).
static int ssAI_MotionKind(const CHARACTER *chr)
{
    const u_char *rec = ssAI_MotionRecord(chr, ssAI_Motion(chr));
    return rec != NULL ? rec[MOTION_REC_KIND] : -1;
}

static int ssAI_IsStalker(s_char id)
{
    return id == STALKER_ID_2 || id == STALKER_ID_3 || id == STALKER_ID_4 || id == STALKER_ID_5 ||
           id == STALKER_ID_6;
}

static int ssAI_IsKind1D(s_char id)
{
    return id == AI_ID_1F || id == AI_ID_1E || id == AI_ID_1D;
}

// 1 when the character's current motion is one of the n in list.
static int ssAI_MotionIn(const CHARACTER *chr, const short *list, int n)
{
    short m = ssAI_Motion(chr);
    int i;

    for (i = 0; i < n; i++)
        if (m == list[i])
            return 1;
    return 0;
}

// vsub, vmul, vaddbc x2, vrsqrt, vmulq: (to - from) xyz scaled to unit length, w 0 (the positions' w
// cancel); the VU's RSQRT of 0 is the largest float.
static void ssAI_DirTo(float *dir, const float *to, const float *from)
{
    float d[3];
    float len2, q;
    int i;

    for (i = 0; i < 3; i++)
        d[i] = to[i] - from[i];
    len2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;
    for (i = 0; i < 3; i++)
        dir[i] = d[i] * q;
    dir[3] = (to[3] - from[3]) * q;
}

// CT3 0x01863ed0: each frame (main), unless paused (PAUSE_RUNNING still runs): every stalker slot. One
// without a brain that left (h100 AI_H100_NONE) is marked gone, and the frame after loses its light bank,
// room and shadow flags. One with a brain, once Alyssa is down outside a boss battle, halts (the halt
// motion, then BRAIN_END_MOTION) and keeps facing her; otherwise its brain runs while the characters are
// not stopped, and its panic and ticks follow. Then the appearance timer. Events 0xd1..0xd3 last a frame.
void ssAI_main(void)
{
    int i;

    if (pause == 0 || (pause & PAUSE_RUNNING) != 0) {
        for (i = 0; i < ATK_CHARA_LIST2_COUNT; i++) {
            CHARACTER *chr = &Character[ssAtkCharaList2[i]];
            u_char *brain;

            if (chr->brain == 0) {
                if (chr->lightBank != -1 && chr->h100 == AI_H100_NONE) {
                    chr->h100 = AI_H100_GONE;
                } else if (chr->lightBank != -1 && chr->h100 == AI_H100_GONE) {
                    chr->lightBank = -1;
                    chr->roomHash = 0;
                    chr->flags &= ~(CHARACTER_FLAG_REFRACTION | CHARACTER_FLAG_SHADOW);
                }
                continue;
            }
            brain = CharaBrainPtr(chr);
            if (ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, -1) != 0 &&
                ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, -1) == 0 && ssAI_IsStalker(chr->id)) {
                float dir[4];

                if (brain[BRAIN_MODE] != SS_AI_HALTED) {
                    short cur = ssAI_Motion(chr);
                    short halt = ssAI_BrainShort(brain, BRAIN_HALT_MOTION);
                    const u_char *rec = ssAI_MotionRecord(chr, cur);
                    short next = -1;

                    brain[BRAIN_MODE] = SS_AI_HALTED;
                    if (rec != NULL)
                        memcpy(&next, rec + MOTION_REC_NEXT_MODE, sizeof(next));
                    if (next == -1)                       // nothing queued: the halt follows this motion
                        asChangeMortionInfo_NextMortion(chr->id, cur, halt, -1);
                    else
                        asCharacterMortionChangeRequest(chr->id, 0, halt);
                    asChangeMortionInfo_NextMortion(chr->id, halt, ssAI_BrainShort(brain, BRAIN_END_MOTION), -1);
                }
                ssAI_DirTo(dir, (const float *)&Character[Player_BankID].pos, (const float *)&chr->pos);
                ssAI_CompCharaDir(chr, dir, AI_FACE_TURN_SPEED, 1);
                continue;
            }
            if ((TrgCharStopFlag & TRG_CHAR_STOP_CHAR) == 0) {
                if (ssAI_BrainShort(brain, BRAIN_STOP_MOTION) != -1)
                    ssAI_BrainSetShort(brain, BRAIN_STOP_MOTION, -1);
                ssAI_charaMain(chr);
                if (chr->brain != 0) {                    // the brain may have ended
                    int ticks;
                    brain = CharaBrainPtr(chr);
                    ssAI_SetPanicGaugeAndBGMVol(chr);
                    memcpy(&ticks, brain + BRAIN_TICKS, sizeof(ticks));
                    ticks += gNextTicksInterval;
                    memcpy(brain + BRAIN_TICKS, &ticks, sizeof(ticks));
                }
            } else if (ssAI_BrainShort(brain, BRAIN_STOP_MOTION) == -1) {
                ssAI_BrainSetShort(brain, BRAIN_STOP_MOTION, ssAI_Motion(chr));
            }
        }
        ssAI_CountAppearanceTimer();
    }
    ssTrigRemoveEvent(TRIG_EVENT_AI_D2, 1, -1);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_STARTED, 1, -1);
    ssTrigRemoveEvent(TRIG_EVENT_AI_APPEAR_NOW, 1, -1);
}

// CT3 0x01863c30: the brain's damage handler first (non-zero: it took the frame), then its mode's
// function; then, while the brain lives and mode changes are allowed (TRIG_EVENT_AI_NO_MODE_CHANGE clear,
// and after a load only for the 0x1d..0x1f kinds), the mode's change function. Both read the mode the
// brain had before the mode function ran.
void ssAI_charaMain(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    u_char mode;

    if (ssAI_DamageMode[brain[BRAIN_DAMAGE_FN]](chr) != 0)
        return;
    mode = brain[BRAIN_MODE];
    if (mode < SS_AI_MODE_COUNT)
        ssAI_ModeTable[mode][brain[BRAIN_MODE_FN + mode]](chr);
    if (chr->brain == 0 || ssTrigCheckEvent(TRIG_EVENT_AI_NO_MODE_CHANGE, 1, -1) != 0)
        return;
    if (ssTrigCheckEvent(TRIG_EVENT_MC_LOADED, 1, -1) != 0 && !ssAI_IsKind1D(chr->id))
        return;
    // PORT: the ROM keeps the brain pointer it read first; a mode function that ended the brain (freed,
    // handle 0) returns above, so the block read here is still the live one.
    mode = brain[BRAIN_MODE];
    if (mode < SS_AI_MODE_COUNT)
        ssAI_ChgModeTable[mode][brain[BRAIN_CHG_MODE_FN + mode]](chr);
}

// CT3 0x01863980: turn chr->rot (0..AI_TURN_FULL) towards dir's heading (ssRadianByO about Y) by at most
// speed, the short way round; within speed it snaps. Unless noMotion or a turn plays already, a turn of a
// quarter to a half plays BRAIN_TURN_MOTION, a half to three quarters BRAIN_TURN_MOTION_BACK, each going
// back to the current motion.
void ssAI_CompCharaDir(CHARACTER *chr, const float *dir, int speed, int noMotion)
{
    u_char *brain = CharaBrainPtr(chr);
    int goal = AI_TURN_FULL - ((u_int)(int)((ssRadianByO(dir, 1) * 65536.0) / 6.283185307179586) & 0xffff);
    int diff, mag;

    if (chr->rot < 0)
        chr->rot += AI_TURN_FULL;
    else if (chr->rot > AI_TURN_FULL - 1)
        chr->rot -= AI_TURN_FULL;
    diff = goal - chr->rot;
    mag = diff < 0 ? -diff : diff;
    if (mag < speed || AI_TURN_FULL - speed < mag) {
        chr->rot = goal;
    } else if ((diff > 0 && diff <= 0x8000) || diff < -0x8000) {
        chr->rot += speed;
        if (chr->rot > AI_TURN_FULL)
            chr->rot -= AI_TURN_FULL;
    } else {
        chr->rot -= speed;
        if (chr->rot < 0)
            chr->rot += AI_TURN_FULL;
    }
    if (noMotion != 0 || ssAI_IsRotationPlaying(chr) != 0)
        return;
    if ((diff > 0x4000 && diff < 0x8000) || (diff > -0xc000 && diff < -0x8000)) {
        short m = ssAI_BrainShort(brain, BRAIN_TURN_MOTION);
        asCharacterMortionChangeRequest(chr->id, 0, m);
        asChangeMortionInfo_NextMortion(chr->id, m, ssAI_Motion(chr), -1);
    } else if ((diff > -0x8001 && diff < -0x4000) || (diff > 0x7fff && diff < 0xc000)) {
        short m = ssAI_BrainShort(brain, BRAIN_TURN_MOTION_BACK);
        asCharacterMortionChangeRequest(chr->id, 0, m);
        asChangeMortionInfo_NextMortion(chr->id, m, ssAI_Motion(chr), -1);
    }
}

// CT3 0x0185f470: the stalker's distance to Alyssa (Character[0]) goes on the panic distance stack; in
// her room, within BRAIN_PANIC_FAR and not reeling, the threat rises by the near / mid / far rise (all
// ranges tripled while she hides), cut to a half above 100 and a third above 200 units of height. The
// rise feeds threat source 2, source 1 (x1.4) while she hides, and source 0 (x3) for an approaching or
// attacking stalker (not the 0x1d..0x1f kinds).
void ssAI_SetPanicGaugeAndBGMVol(CHARACTER *chr)
{
    const u_char *brain = CharaBrainPtr(chr);
    const float *alyssa = (const float *)&Character[CHARA_ID_ALYSSA].pos;
    const float *pos = (const float *)&chr->pos;
    float dx = alyssa[0] - pos[0], dy = alyssa[1] - pos[1], dz = alyssa[2] - pos[2];
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);
    float far = ssAI_BrainFloat(brain, BRAIN_PANIC_FAR);
    float mid = ssAI_BrainFloat(brain, BRAIN_PANIC_MID);
    float near = ssAI_BrainFloat(brain, BRAIN_PANIC_NEAR);
    float rise;
    int id = chr->id;

    asMonsterDistanceStack_inPanic(MDS_PUSH, &id, &dist);
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, 1, -1) != 0) {
        far *= 3.0f;
        mid *= 3.0f;
        near *= 3.0f;
    }
    if (chr->roomHash != Character[CHARA_ID_ALYSSA].roomHash || far <= dist || ssAI_IsDamagePlaying(chr) != 0) {
        rise = 0.0f;
    } else {
        float dh;
        if (dist < near)
            rise = ssAI_BrainFloat(brain, BRAIN_PANIC_RISE_NEAR);
        else if (dist < mid)
            rise = ssAI_BrainFloat(brain, BRAIN_PANIC_RISE_MID);
        else
            rise = ssAI_BrainFloat(brain, BRAIN_PANIC_RISE_FAR);
        dh = pos[1] - alyssa[1];
        if (dh < 0.0f)
            dh = -dh;
        if (dh > 200.0f)
            rise *= 0.33333334f;
        else if (dh > 100.0f)
            rise *= 0.5f;
    }
    ssAI_PanicSet(rise, chr, PANIC_RISE_THREAT + 2);
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, 1, -1) != 0)
        ssAI_PanicSet(rise * 1.4f, chr, PANIC_RISE_THREAT + 1);
    if (!ssAI_IsKind1D(chr->id) && (brain[BRAIN_MODE] == SS_AI_ATTACK || brain[BRAIN_MODE] == SS_AI_APPROACH))
        ssAI_PanicSet(rise * 3.0f, chr, PANIC_RISE_THREAT);
}

// CT3 0x0185f930
void ssAI_PanicSet(float rise, CHARACTER *chr, int source)
{
    const u_char *brain = CharaBrainPtr(chr);

    if (brain[BRAIN_MODE] != SS_AI_RETURN)
        PanicSet(rise * ssAI_BrainFloat(brain, BRAIN_PANIC_SCALE), source);
}

// CT3 0x0185fd40: 1 when the stalker hears a noise of strength noise: in full panic at once (unless she
// is invisible), else at a chance of noise times the brain's rate (its invisible rate while invisible).
int ssAI_CheckHearingRate(CHARACTER *chr, int noise)
{
    const u_char *brain = CharaBrainPtr(chr);
    int invisible;

    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_FULL, 1, -1) != 0) {
        if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, -1) == 0)
            return 1;
        invisible = 1;
    } else {
        invisible = ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, -1) != 0;
    }
    return ssTrigCheckRandom(noise * brain[invisible ? BRAIN_HEAR_RATE_INVISIBLE : BRAIN_HEAR_RATE]) != 0;
}

// CT3 0x0185fe10: 1 when a stalker strikes at Alyssa's hiding place, by its level (TRIG_VAR_HIDE_POINT).
int ssAI_CheckHideAreaAtkRate(void)
{
    int level = ssTrigGetVariable(TRIG_VAR_HIDE_POINT, 1, -1);

    if (level >= AI_HIDE_LEVEL_SURE)
        return 1;
    if (level == 3)
        return ssTrigCheckRandom(AI_HIDE_ATK_RATE_3) != 0;
    if (level == 2)
        return ssTrigCheckRandom(AI_HIDE_ATK_RATE_2) != 0;
    if (level == 1)
        return ssTrigCheckRandom(AI_HIDE_ATK_RATE_1) != 0;
    return 0;
}

// CT3 0x0185feb0: the door's open-full event (its low byte); -1 for a door out of range.
int ssAI_IsDoorOpenFull(int door)
{
    if (door < 0 || door > AI_DOOR_MAX)
        return -1;
    return ssTrigCheckEvent(door + TRIG_EVENT_DOOR_OPEN_FULL, 1, -1) & 0xff;
}

// CT3 0x0185fef0: 1 when the door's closed event is clear (sic: the ROM's name and test); -1 out of range.
int ssAI_IsDoorClose(int door)
{
    if (door < 0 || door > AI_DOOR_MAX)
        return -1;
    return ssTrigCheckEvent(door + TRIG_EVENT_DOOR_CLOSED, 1, -1) == 0;
}

// CT3 0x018608e0
int ssAI_IsBendPlaying(CHARACTER *chr)
{
    return ssAI_MotionKind(chr) == MOTION_KIND_BEND;
}

// CT3 0x01860910
int ssAI_IsDownPlaying(CHARACTER *chr)
{
    return ssAI_MotionKind(chr) == MOTION_KIND_DOWN;
}

// CT3 0x01860940: 1 while the character plays its kind's return motion.
int ssAI_IsReturnPlaying(CHARACTER *chr)
{
    switch (chr->id) {
    case AI_ID_1D:
    case AI_ID_1E:
    case AI_ID_1F:
        return ssAI_Motion(chr) == 0x0f;
    case STALKER_ID_3:
    case STALKER_ID_5:
    case STALKER_ID_6:
        return ssAI_Motion(chr) == 0x27;
    case STALKER_ID_4:
        return ssAI_Motion(chr) == 0x29;
    case STALKER_ID_2:
        return ssAI_Motion(chr) == 0x3d;
    default:
        return 0;
    }
}

// CT3 0x01860a70: 1 while the character plays one of its kind's damage motions.
int ssAI_IsDamagePlaying(CHARACTER *chr)
{
    static const short kind1d[] = { 0x09 };
    static const short boss[] = { 0x18, 0x17, 0x16, 0x15, 0x14 };
    static const short st6[] = { 0x36, 0x35, 0x33 };
    static const short st5[] = { 0x47, 0x46, 0x38, 0x37, 0x34 };
    static const short st4[] = { 0x4a, 0x52, 0x51, 0x3b, 0x3a, 0x38 };
    static const short st3[] = { 0x44, 0x43, 0x36, 0x33, 0x35 };
    static const short st2[] = { 0x71, 0x70, 0x43, 0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29 };

    switch (chr->id) {
    case AI_ID_1D:
    case AI_ID_1E:
    case AI_ID_1F:
        return ssAI_MotionIn(chr, kind1d, 1);
    case AI_ID_20:
        return ssAI_MotionIn(chr, boss, 5);
    case STALKER_ID_6:
        return ssAI_MotionIn(chr, st6, 3);
    case STALKER_ID_5:
        return ssAI_MotionIn(chr, st5, 5);
    case STALKER_ID_4:
        return ssAI_MotionIn(chr, st4, 6);
    case STALKER_ID_3:
        return ssAI_MotionIn(chr, st3, 5);
    case STALKER_ID_2:
        return ssAI_MotionIn(chr, st2, 10);
    default:
        return 0;
    }
}

// CT3 0x01861830: 1 while the character plays one of its kind's two turn motions.
int ssAI_IsRotationPlaying(CHARACTER *chr)
{
    static const short kind1d[] = { 0x11, 0x10 };
    static const short boss[] = { 0x05, 0x04 };
    static const short st36[] = { 0x14, 0x13 };
    static const short st5[] = { 0x3d, 0x3c };
    static const short st4[] = { 0x41, 0x40 };
    static const short st2[] = { 0x0b, 0x0a };

    switch (chr->id) {
    case AI_ID_1D:
    case AI_ID_1E:
    case AI_ID_1F:
        return ssAI_MotionIn(chr, kind1d, 2);
    case AI_ID_20:
        return ssAI_MotionIn(chr, boss, 2);
    case STALKER_ID_3:
    case STALKER_ID_6:
        return ssAI_MotionIn(chr, st36, 2);
    case STALKER_ID_5:
        return ssAI_MotionIn(chr, st5, 2);
    case STALKER_ID_4:
        return ssAI_MotionIn(chr, st4, 2);
    case STALKER_ID_2:
        return ssAI_MotionIn(chr, st2, 2);
    default:
        return 0;
    }
}

// The stalkers' scene banks (data 0x019338b0..0x019338d0: the file names TrgSceneDraw and ssGetBankID take).
static char ssAI_BankE02[] = "E_02";    /* STALKER_ID_2 */
static char ssAI_BankE03[] = "E_03";    /* STALKER_ID_3 */
static char ssAI_BankE04[] = "E_04";    /* STALKER_ID_4 */
static char ssAI_BankE14[] = "E_14";    /* STALKER_ID_4's second bank (its axe: AxeThrowReset follows it) */
static char ssAI_BankE05[] = "E_05";    /* STALKER_ID_5 and STALKER_ID_6 */

// The stalker's first scene bank's name; NULL for the other kinds.
static char *ssAI_StalkerBank(s_char id)
{
    switch (id) {
    case STALKER_ID_6:
    case STALKER_ID_5:
        return ssAI_BankE05;
    case STALKER_ID_4:
        return ssAI_BankE04;
    case STALKER_ID_3:
        return ssAI_BankE03;
    case STALKER_ID_2:
        return ssAI_BankE02;
    default:
        return NULL;
    }
}

// Part 0 of the bank parked out of the world (drawOnPos (1e7, -1e7, 0), placed and trans set).
static void ssAI_ParkBank(const char *name)
{
    int bank = ssGetBankID(name);
    MODEL_PARTS *part0;

    if (bank == -1)
        return;
    part0 = (MODEL_PARTS *)pModelPartsHeader[bank];
    part0->drawOnPos[0] = AI_PARK_X;
    part0->drawOnPos[1] = AI_PARK_Y;
    part0->drawOnPos[2] = 0.0f;
    part0->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
}

// CT3 0x01865e20: a brain ends: the kind's event removed (a stalker's: the chase, its no-reaction time set
// from the brain and its special end run), the motion requested, the character parked out of the world
// (pos and posBefore (1e7, -1e7, 0), h100 AI_H100_NONE, CHARACTER_FLAG_AI_PARKED) with its scene banks,
// hidden, and the brain freed.
void ssAI_BrainEnd(CHARACTER *chr, int motion)
{
    float *pos = (float *)&chr->pos;
    float *posBefore = (float *)&chr->posBefore;
    s_char id = chr->id;

    if (id == AI_ID_1F) {
        ssTrigRemoveEvent(TRIG_EVENT_AI_1F, 1, -1);
    } else if (id == AI_ID_1E) {
        ssTrigRemoveEvent(TRIG_EVENT_AI_1E, 1, -1);
    } else if (id == AI_ID_1D) {
        ssTrigRemoveEvent(TRIG_EVENT_AI_1D, 1, -1);
    } else if (ssAI_IsStalker(id)) {
        int noReaction;
        memcpy(&noReaction, CharaBrainPtr(chr) + BRAIN_NO_REACTION_TIME, sizeof(noReaction));
        ssTrigRemoveEvent(TRIG_EVENT_CHASE, 1, -1);
        ssAI_SetNoReactionTimer(noReaction);
        ssAI_BrainEndSpecialProc(chr);
    }
    asCharacterMortionChangeRequest(chr->id, 0, motion);
    chr->h100 = AI_H100_NONE;
    pos[0] = posBefore[0] = AI_PARK_X;
    pos[1] = posBefore[1] = AI_PARK_Y;
    pos[2] = posBefore[2] = 0.0f;
    chr->flags |= CHARACTER_FLAG_AI_PARKED;
    if (ssAI_StalkerBank(chr->id) != NULL)
        ssAI_ParkBank(ssAI_StalkerBank(chr->id));
    if (chr->id == STALKER_ID_4)
        ssAI_ParkBank(ssAI_BankE14);
    ssAI_MonsterDraw(chr, 0);
    ssAI_BrainFree(chr);
}

// CT3 0x0185f710: shows (draw 1) or hides (0) a stalker with a brain: its shadow flag and its scene banks
// (TrgSceneDraw at the bank's origin). Shown only while its light bank (CHARACTER.lightBank, a bank past
// the first) is drawn; one shown while playing its return motion ends its brain instead. Hiding the axe
// stalker resets its axe throw.
void ssAI_MonsterDraw(CHARACTER *chr, int draw)
{
    char *name;
    int bank;

    if (chr->brain == 0)
        return;
    if (draw == 0) {
        chr->flags &= ~CHARACTER_FLAG_SHADOW;
    } else {
        if (chr->lightBank < 1 || BankInfo[chr->lightBank].drawOn == 0)
            return;
        chr->flags |= CHARACTER_FLAG_SHADOW;
        if (ssAI_IsReturnPlaying(chr) != 0) {
            ssAI_BrainEnd(chr, ssAI_BrainShort(CharaBrainPtr(chr), BRAIN_END_MOTION));
            return;
        }
    }
    name = ssAI_StalkerBank(chr->id);
    if (name == NULL)
        return;
    bank = ssGetBankID(name);
    if (bank != -1)
        TrgSceneDraw(draw, 0, SCENE_PLACE_ORIGIN, name, bank);
    if (chr->id == STALKER_ID_4) {
        bank = ssGetBankID(ssAI_BankE14);
        if (bank != -1)
            TrgSceneDraw(draw, 0, SCENE_PLACE_ORIGIN, ssAI_BankE14, bank);
        if (draw == 0)
            ssAI_AxeThrowReset();
    }
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018642b0
void ssAI_AxeThrowReset(void)
{
    ALYSSAPAN_STUB("ssAI_AxeThrowReset", 0x018642b0);
}

// stub of CT3 0x01864950
void ssAI_BrainEndSpecialProc(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssAI_BrainEndSpecialProc", 0x01864950);
}
