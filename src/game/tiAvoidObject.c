// game/tiAvoidObject.c -- Clock Tower 3: the stalkers' AI (ssAI_*; symtab local chunk 74). CodeWarrior C.
// Slice 1 (2026-11-12): the appearance and no-reaction timers, the global init, the save image's part,
// the brain's free, the motion predicates (crawl, door push / pull) and ssAI_GetScnBankID. Slice 2: the
// hide poles and the scene init. Slice 3: the frame's driver (ssAI_main), the mode tables and dispatch,
// the facing, the panic rise, the hearing / hide / door checks, the motion predicates. Slice 4: the
// obstacle steering (tiAvoidObject), the frame / group / scene predicates, the axe reset and BrainEndSpecialProc.
// Slice 5: the kinds' motion predicates, the warp test, the arrivals, the group links and the route ring.

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
#include "ga/gaSinCos.h"
#include "game/hhXcmd_DressUp.h"
#include "game/ssSetTexUVforCubeMap.h"
#include "game/Clipping_Portal.h"
#include "game/xcmd_tn.h"

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

// ---- slice 4 (2026-11-12): the obstacle steering, the frame and group / scene predicates, the axe reset ----

// CT3 0x0185ffd0: the frame test of motion `index`'s behaviour (the ROM spells it "Mortion").
int ssAI_IsMortionFramePassed(CHARACTER *chr, int index, u_int frame)
{
    const u_char *rec = ssAI_MotionRecord(chr, index);
    BEHAVIOR *bhv;
    BEHAVIOR_TRACK *track;
    u_int word, now, before;
    u_long hash;
    int bank;

    if (rec == NULL)
        return 0;   // PORT: the ROM reads a record whatever the table word holds
    memcpy(&hash, rec + CHARA_MOTION_BEHAVIOR, sizeof(hash));
    bhv = asFindBehaviorByCharacter(chr->id, hash);
    if (bhv == NULL)
        return 0;
    bank = BankOfHostPtr(bhv);
    track = BANK_PTR(BEHAVIOR_TRACK *, bank, bhv->groups[0].tracks);
    memcpy(&word, (const u_char *)track + TRACK_FRAMES, sizeof(word));
    now = word & 0xffff;
    before = word >> 16;
    if ((track->flags & TRACK_FLAG_NO_FRAMES) != 0)
        return 0;
    if (before < now)
        return before < frame && frame <= now;
    if (now < before)
        return before < frame || frame <= now;     // the motion wrapped
    return before == frame;
}

// CT3 0x018603d0
int ssAI_IsNextGroup(short group, short other, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE_GROUP *groups = BANK_PTR(HIDE_POLE_GROUP *, bank, list->groups);
    int i, j;

    for (i = 0; i < list->groupCount; i++) {
        HIDE_POLE_GROUP *g = &groups[i];
        const short *links;
        if (g->id != group)
            continue;
        links = BANK_PTR(const short *, bank, g->links);
        for (j = 0; j < g->linkCount; j++) {
            if (other == groups[links[j]].id)
                return 1;
        }
    }
    return 0;
}

// CT3 0x01860490
int ssAI_IsNextScene(int bank, u_int room)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);
    int i;

    for (i = 0; i < list->count; i++) {
        HIDE_POLE *p = &poles[i];
        MODEL_PARTS *part = (MODEL_PARTS *)(pModelPartsHeader[bank] + p->parts * PARTS_REC_SIZE);
        if ((part->flags & PARTS_FLAG_HIDDEN) == 0 && (p->flags & HIDE_POLE_FLAG_SHADOW) != 0 && room == p->room)
            return 1;
    }
    return 0;
}

// The VU0 normalisation (vrsqrt of x^2 + y^2 + z^2, vmulq): all four lanes scaled by 1 / |xyz|.
static void ssAI_Unit(float *out, const float *v)
{
    float len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    float q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;
    int i;

    for (i = 0; i < 4; i++)
        out[i] = v[i] * q;
}

// |a - b| (xyz) and the same in the XZ plane (y dropped).
static float ssAI_Dist(const float *a, const float *b)
{
    float d[3] = { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
    return sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

static float ssAI_DistXZ(const float *a, const float *b)
{
    float d[2] = { a[0] - b[0], a[2] - b[2] };
    return sqrtf(d[0] * d[0] + d[1] * d[1]);
}

static void ssAI_RotY(float *out, const float *v, int angle)
{
    alignas(16) float m[4][4];

    gaSetRotMatrixY(m, angle);
    ssAI_ApplyMatrix(out, (const float (*)[4])m, v);
}

// seg->to = seg->from + v.
static void ssAI_ProbeTo(LOS_HIT *seg, const float *v)
{
    int i;

    for (i = 0; i < 4; i++)
        seg->seg.to[i] = seg->seg.from[i] + v[i];
}

// The followed polygon's corners nearest (*near) and farthest (*far) from Alyssa in the XZ plane (all four
// corners: a triangle's fourth is read as the ROM does).
static void ssAI_NearFarCorner(const HIT_DATA *hit, const float *player, int *nearC, int *farC)
{
    float d = ssAI_DistXZ(hit->world[0], player);
    float lo = d, hi = d;
    int i;

    *nearC = *farC = 0;
    for (i = 1; i < 4; i++) {
        d = ssAI_DistXZ(hit->world[i], player);
        if (hi < d) {
            hi = d;
            *farC = i;
        }
        if (d < lo) {
            lo = d;
            *nearC = i;
        }
    }
}

// The XZ unit of a - b.
static void ssAI_EdgeXZ(float *out, const float *a, const float *b)
{
    float e[4] = { a[0] - b[0], 0.0f, a[2] - b[2], a[3] - b[3] };
    ssAI_Unit(out, e);
}

// goal = corner + unit(edge + n) * AVOID_GOAL_OFFSET, its y from CHARA_AVOID_GOAL_Y.
static float ssAI_Goal[4];              /* goalPos$1620: sbss 0x0193ce80 (symtab local of tiAvoidObject) */

static void ssAI_SetGoal(const CHARACTER *chr, const float *corner, const float *edge, const float *n)
{
    float v[4], u[4];
    int i;

    for (i = 0; i < 4; i++)
        v[i] = edge[i] + n[i];
    ssAI_Unit(u, v);
    for (i = 0; i < 4; i++)
        ssAI_Goal[i] = corner[i] + u[i] * AVOID_GOAL_OFFSET;
    memcpy(&ssAI_Goal[1], (const u_char *)chr + CHARA_AVOID_GOAL_Y, sizeof(float));
}

// dir = the XZ unit towards the goal.
static void ssAI_DirToGoal(float *dir, const CHARACTER *chr)
{
    const float *pos = (const float *)&chr->pos;
    float d[4];
    int i;

    for (i = 0; i < 4; i++)
        d[i] = ssAI_Goal[i] - pos[i];
    d[1] = 0.0f;
    d[3] = 0.0f;
    ssAI_Unit(dir, d);
}

// CT3 0x0185e9d0: steer chr round what blocks it (see tiAvoidObject.h). A clear line towards Alyssa
// forgets the followed polygon. A blocked one, with no polygon followed yet, tries probes 15 and 30 degrees
// either side and turns 45 / 15 (the first clear one's side; the matrix the 30-degree probes leave turns
// dir by 30); with all four blocked it follows the polygon: the goal is off its corner nearest Alyssa, or
// the far one when the way along the edge is blocked. With a polygon followed it heads for the goal, or,
// on a new polygon, sets the new goal (dir unchanged, as the ROM does). Then a 60-unit probe 45 degrees to
// the left: blocked turns dir right by 45, else one to the right: blocked turns dir left by 45.
int tiAvoidObject(float *dir, CHARACTER *chr, void **lastHit)
{
    static const int fan[4] = { AVOID_ANGLE_15, AVOID_ANGLE_M15, AVOID_ANGLE_30, AVOID_ANGLE_M30 };
    static const int turn[4] = { AVOID_ANGLE_45, AVOID_ANGLE_M45, AVOID_ANGLE_30, AVOID_ANGLE_M30 };
    const float *pos = (const float *)&chr->pos;
    const float *player = (const float *)&Character[Player_BankID].pos;
    LOS_HIT seg;
    float probe[4], v[4];
    float dist;
    int turned = 0;
    int i;

    memset(&seg, 0, sizeof(seg));
    dist = ssAI_Dist(player, pos);
    if (dist > AVOID_PROBE_MAX)
        dist = AVOID_PROBE_MAX;
    memcpy(seg.seg.from, pos, sizeof(seg.seg.from));
    seg.seg.from[1] = pos[1] + AVOID_EYE_HEIGHT;
    for (i = 0; i < 4; i++)
        probe[i] = dir[i] * dist;
    ssAI_ProbeTo(&seg, probe);
    if (CheckLOS(chr->lightBank, &seg) == 0) {
        *lastHit = NULL;
    } else {
        HIT_DATA *hit = (HIT_DATA *)seg.record;
        float n[4] = { 0.0f, 0.0f, 0.0f, 0.0f }, edge[4];
        int nearC, farC;

        turned = 1;
        if (*lastHit == NULL) {
            for (i = 0; i < 4; i++) {
                ssAI_RotY(v, probe, fan[i]);
                ssAI_ProbeTo(&seg, v);
                if (CheckLOS(chr->lightBank, &seg) == 0) {
                    *lastHit = NULL;
                    ssAI_RotY(dir, dir, turn[i]);
                    break;
                }
            }
            if (i == 4) {
                // All four blocked: follow the polygon the last probe hit.
                LOS_HIT along;
                hit = (HIT_DATA *)seg.record;
                ssAI_NearFarCorner(hit, player, &nearC, &farC);
                ssAI_EdgeXZ(edge, hit->world[nearC], hit->world[farC]);
                gaCopyVectorXYZ(n, hit->worldN);
                n[1] = 0.0f;
                memset(&along, 0, sizeof(along));
                memcpy(along.seg.from, hit->world[farC], sizeof(along.seg.from));
                memcpy(along.seg.to, hit->world[nearC], sizeof(along.seg.to));
                along.seg.from[1] = along.seg.to[1] = pos[1] + AVOID_EYE_HEIGHT;
                for (i = 0; i < 4; i++) {
                    along.seg.from[i] += n[i];
                    along.seg.to[i] += n[i] + edge[i];
                }
                if (CheckLOS(chr->lightBank, &along) == 0) {
                    ssAI_SetGoal(chr, hit->world[nearC], edge, n);
                } else {
                    ssAI_EdgeXZ(edge, hit->world[farC], hit->world[nearC]);
                    ssAI_SetGoal(chr, hit->world[farC], edge, n);
                }
                *lastHit = hit;
                ssAI_DirToGoal(dir, chr);
            }
        } else if (*lastHit == hit) {
            ssAI_DirToGoal(dir, chr);
        } else {
            ssAI_NearFarCorner(hit, player, &nearC, &farC);
            ssAI_EdgeXZ(edge, hit->world[nearC], hit->world[farC]);
            gaCopyVectorXYZ(n, hit->worldN);
            n[1] = 0.0f;
            ssAI_SetGoal(chr, hit->world[nearC], edge, n);
            *lastHit = hit;
        }
    }

    // The sides: a 60-unit probe 45 degrees left, then right.
    for (i = 0; i < 4; i++)
        probe[i] = dir[i] * AVOID_SIDE_PROBE;
    ssAI_RotY(v, probe, AVOID_ANGLE_45);
    ssAI_ProbeTo(&seg, v);
    if (CheckLOS(chr->lightBank, &seg) == 0) {
        ssAI_RotY(v, probe, AVOID_ANGLE_M45);
        ssAI_ProbeTo(&seg, v);
        if (CheckLOS(chr->lightBank, &seg) != 0) {
            ssAI_RotY(dir, dir, AVOID_ANGLE_45);
            turned = 1;
        }
    } else {
        ssAI_RotY(dir, dir, AVOID_ANGLE_M45);
        turned = 1;
    }
    return turned;
}

// The axe stalker's scene bank ("E_14") and the stop-SE command's arguments (@3321..@3323).
static char ssAI_SeOno[] = "E_ONO";     /* data 0x01933a08 */
static char ssAI_SeArg3[] = "3";        /* data 0x01933a10 */
static char ssAI_SeArg1[] = "1";        /* data 0x01933a18 */

// The part named name of the bank's parts header.
static int ssAI_PartID(u_char *header, const char *name)
{
    return (short)asFindModelPartsID(header, ahGetHash(name));
}

// The axe throw's parts (E_14's model; data 0x019339e8..0x01933bd0, Ghidra's strings).
static const char *const ssAI_AxeShownParts[] = { "N_AxeR", "N_AxeL" };
static const char *const ssAI_AxeBlurParts[] = { "N_AxeR001", "N_AxeL01", "N_Axerr01" };
static const char *const ssAI_AxeHiddenParts[] = {
    "ht_dabul_axe", "ht_double_axe_damege", "ht_L_axe", "ht_L_axe_damege", "ht_R_axe", "ht_R_axe_damege",
    "N_AxeL", "N_AxeL01", "N_AxeL_shoot00", "N_AxeR", "N_AxeR001", "N_AxeR01", "N_AxeR_shoot00",
    "N_Axerr01", "N_Axerr_shoot00",
};
static const char *const ssAI_AxeParticles[] = {
    "p_hit_N_AxeR00", "p_hit_N_AxeR01", "p_hit_N_AxeR02", "p_hit_N_AxeL00", "p_hit_N_AxeL01",
    "p_hit_N_AxeL02", "p_hit_N_Axerr00", "p_hit_N_Axerr01", "p_hit_N_Axerr02",
};
static const char *const ssAI_AxeHitParts[] = {
    "hit_N_AxeL00", "hit_N_AxeL01", "hit_N_AxeR00", "hit_N_AxeR01", "hit_N_Axerr00", "hit_N_Axerr01",
};

// CT3 0x018642b0: the axe throw back to rest. The axe stalker's own model shows its two axes again and
// hides the thrown one (N_AxeR01); the axe's scene bank (E_14) resets its triggers, stops the throw's SE,
// ends the blurs, hides every axe part and hit shape, puts out the hit particles and ends the throw.
void ssAI_AxeThrowReset(void)
{
    short bank = Character[HIT_CHARA_ONO].bank;
    size_t i;

    if (bank != -1) {
        u_char *header = pModelPartsHeader[bank];
        ssTrigSetEnable(ssAI_PartID(header, ssAI_AxeShownParts[0]), 1, 0, bank);
        ssTrigSetEnable(ssAI_PartID(header, ssAI_AxeShownParts[1]), 1, 0, bank);
        ssTrigSetEnable(ssAI_PartID(header, "N_AxeR01"), 0, 0, bank);
    }
    {
        int axe = ssGetBankID(ssAI_BankE14);
        char *argv[3];
        u_char *header;

        if (axe == -1)
            return;
        Trigger_reset(axe);
        header = pModelPartsHeader[axe];
        argv[0] = ssAI_SeOno;
        argv[1] = ssAI_SeArg3;
        argv[2] = ssAI_SeArg1;
        hhXcmd_StopSESp1(3, argv, axe);
        for (i = 0; i < sizeof(ssAI_AxeBlurParts) / sizeof(ssAI_AxeBlurParts[0]); i++)
            ssPartMotionBlurEnd(ssAI_PartID(header, ssAI_AxeBlurParts[i]), axe);
        for (i = 0; i < sizeof(ssAI_AxeHiddenParts) / sizeof(ssAI_AxeHiddenParts[0]); i++)
            ssTrigSetEnable(ssAI_PartID(header, ssAI_AxeHiddenParts[i]), 0, 0, axe);
        for (i = 0; i < sizeof(ssAI_AxeParticles) / sizeof(ssAI_AxeParticles[0]); i++)
            ssTrigParticleAllOffImmediately(ssAI_PartID(header, ssAI_AxeParticles[i]), axe);
        for (i = 0; i < sizeof(ssAI_AxeHitParts) / sizeof(ssAI_AxeHitParts[0]); i++)
            ssTrigSetEnable(ssAI_PartID(header, ssAI_AxeHitParts[i]), 0, 0, axe);
        ssXcmd_AXE_ThrowEnd(0, argv, axe);
    }
}

// ---- ssAI_BrainEndSpecialProc: each stalker's effects put back at its brain's end ----

// The commands' arguments (symtab literals @3514..@3574).
static char ssAI_ArgOff[] = "OFF";      /* data 0x01933e90 (@3548) */
static char ssAI_Arg0[] = "0";          /* data 0x01933cc8 (@3515) */
static char ssAI_Arg2[] = "2";          /* data 0x01933cc0 (@3514) */
static char ssAI_Arg4[] = "4";          /* data 0x01933ed8 (@3553) */
static char ssAI_Arg5[] = "5";          /* data 0x01933fa0 (@3567) */
static char ssAI_Arg6[] = "6";          /* data 0x01934008 (@3574) */
static char ssAI_SeRyusan[] = "E_RYUSAN"; /* data 0x01933d00 (@3520): STALKER_ID_3's SE bank */

// What a step does to its part (by name, through the bank's parts header). layout: host -- the ROM writes
// each step as its own call; the table is the port's.
enum SS_AI_PART_OP {
    SS_AI_PART_SHOW,                    /* ssTrigSetEnable(part, 1, 0, bank) */
    SS_AI_PART_HIDE,                    /* ssTrigSetEnable(part, 0, 0, bank) */
    SS_AI_PART_PARTICLE_OFF,            /* ssTrigParticleAllOffImmediately(part, bank) */
};
// layout: host (a port table)
typedef struct SS_AI_PART_STEP {
    u_char op;                          /* SS_AI_PART_* */
    const char *name;
} SS_AI_PART_STEP;

static void ssAI_PartSteps(const SS_AI_PART_STEP *steps, size_t n, int bank)
{
    u_char *header = pModelPartsHeader[bank];
    size_t i;

    for (i = 0; i < n; i++) {
        int part = ssAI_PartID(header, steps[i].name);
        switch (steps[i].op) {
        case SS_AI_PART_SHOW:
            ssTrigSetEnable(part, 1, 0, bank);
            break;
        case SS_AI_PART_HIDE:
            ssTrigSetEnable(part, 0, 0, bank);
            break;
        default:
            ssTrigParticleAllOffImmediately(part, bank);
            break;
        }
    }
}
#define SS_AI_STEPS(a) (a), sizeof(a) / sizeof((a)[0])

// The stalkers' own models (CHARACTER.bank): the weapons back in hand, the attack shapes off.
static const SS_AI_PART_STEP ssAI_End6Own[] = {
    { SS_AI_PART_SHOW, "n_hazami1" }, { SS_AI_PART_SHOW, "n_hasami2" },
    { SS_AI_PART_HIDE, "n_hasami03" }, { SS_AI_PART_HIDE, "n_hazami04" },
};
static const SS_AI_PART_STEP ssAI_End5Own[] = {
    { SS_AI_PART_SHOW, "ha_R_Sword" }, { SS_AI_PART_SHOW, "N_HASAMI02" }, { SS_AI_PART_HIDE, "N_HASAMI03" },
};
static const SS_AI_PART_STEP ssAI_End3Own[] = {
    { SS_AI_PART_PARTICLE_OFF, "particle_acid_01" }, { SS_AI_PART_PARTICLE_OFF, "particle_acid_02" },
    { SS_AI_PART_HIDE, "HIT_side_attack_01" }, { SS_AI_PART_HIDE, "HIT_side_attack_02" },
    { SS_AI_PART_HIDE, "HIT_side_attack_03" }, { SS_AI_PART_HIDE, "HIT_side_attack_04" },
    { SS_AI_PART_SHOW, "ha_Continuation_attack" }, { SS_AI_PART_SHOW, "ha_dash_attack" },
};
static const SS_AI_PART_STEP ssAI_End2Own[] = {
    { SS_AI_PART_PARTICLE_OFF, "HAM_MAGIC_HIT" }, { SS_AI_PART_SHOW, "ha_Grip02" },
    { SS_AI_PART_SHOW, "ha_Hammer01" }, { SS_AI_PART_SHOW, "n_al2_1" }, { SS_AI_PART_SHOW, "n_al2_2" },
    { SS_AI_PART_HIDE, "n_al2_03" }, { SS_AI_PART_HIDE, "n_al2_04" }, { SS_AI_PART_SHOW, "hd_waist" },
};
// Their scene banks (E_05 for kinds 5 and 6, E_04, E_03, E_02): the effects off.
static const SS_AI_PART_STEP ssAI_EndE05[] = {
    { SS_AI_PART_HIDE, "e_05_pme02" }, { SS_AI_PART_PARTICLE_OFF, "e_05_inazumap01" },
    { SS_AI_PART_HIDE, "e_05_light" }, { SS_AI_PART_HIDE, "e_05_01" }, { SS_AI_PART_HIDE, "e_05_02" },
    { SS_AI_PART_PARTICLE_OFF, "e_05_p02" }, { SS_AI_PART_HIDE, "e_05_jp01" }, { SS_AI_PART_HIDE, "e_05_jp02" },
    { SS_AI_PART_HIDE, "e_05_pme01" }, { SS_AI_PART_HIDE, "e_Omni01" },
};
static const SS_AI_PART_STEP ssAI_EndE05Atk6[] = {
    { SS_AI_PART_HIDE, "aratk_scsr00" }, { SS_AI_PART_HIDE, "aratk_scsr01" },
};
static const SS_AI_PART_STEP ssAI_EndE05Atk5[] = {
    { SS_AI_PART_HIDE, "3atk_scsr00" }, { SS_AI_PART_HIDE, "3atk_scsr01" },
};
static const SS_AI_PART_STEP ssAI_EndE04[] = {
    { SS_AI_PART_PARTICLE_OFF, "e_04_p02" }, { SS_AI_PART_HIDE, "e_04_base02" },
    { SS_AI_PART_HIDE, "e_04_base03" }, { SS_AI_PART_HIDE, "e_04_base04" },
};
static const SS_AI_PART_STEP ssAI_EndE03[] = {
    { SS_AI_PART_PARTICLE_OFF, "ehw01" }, { SS_AI_PART_PARTICLE_OFF, "ehw02" }, { SS_AI_PART_PARTICLE_OFF, "ehw03" },
    { SS_AI_PART_PARTICLE_OFF, "ehw04" }, { SS_AI_PART_PARTICLE_OFF, "ehw05" }, { SS_AI_PART_PARTICLE_OFF, "ehw06" },
    { SS_AI_PART_PARTICLE_OFF, "ehw07" }, { SS_AI_PART_PARTICLE_OFF, "ehw08" }, { SS_AI_PART_PARTICLE_OFF, "ehw09" },
    { SS_AI_PART_PARTICLE_OFF, "ehw10" }, { SS_AI_PART_PARTICLE_OFF, "ehw11" }, { SS_AI_PART_PARTICLE_OFF, "ehw12" },
    { SS_AI_PART_PARTICLE_OFF, "ehw13" }, { SS_AI_PART_PARTICLE_OFF, "ehw14" }, { SS_AI_PART_PARTICLE_OFF, "ehw15" },
    { SS_AI_PART_PARTICLE_OFF, "ehw16" }, { SS_AI_PART_PARTICLE_OFF, "ehw17" }, { SS_AI_PART_PARTICLE_OFF, "ehw18" },
    { SS_AI_PART_PARTICLE_OFF, "ehw19" },
};
static const SS_AI_PART_STEP ssAI_EndE02[] = {
    { SS_AI_PART_PARTICLE_OFF, "p_e02_hw00" }, { SS_AI_PART_PARTICLE_OFF, "p_e02_hw01" },
    { SS_AI_PART_PARTICLE_OFF, "p_e02_hw02" }, { SS_AI_PART_PARTICLE_OFF, "p_e02_hw03" },
    { SS_AI_PART_PARTICLE_OFF, "p_e02_hw04" }, { SS_AI_PART_PARTICLE_OFF, "p_e02_hw05" },
    { SS_AI_PART_PARTICLE_OFF, "p_e02_hw06" },
};
// Every scene bank's tail: the fade-away effect off.
static const SS_AI_PART_STEP ssAI_EndFade[] = {
    { SS_AI_PART_HIDE, "doron" }, { SS_AI_PART_HIDE, "doron02" },
    { SS_AI_PART_PARTICLE_OFF, "p_fadeaway00" }, { SS_AI_PART_HIDE, "fadeaway01" },
};

// "CharOmniEffect OFF <chara> 0" on bank.
static void ssAI_OmniOff(char *chara, int bank)
{
    char *argv[3] = { ssAI_ArgOff, chara, ssAI_Arg0 };
    tnXcmd_CharOmniEffect(3, argv, bank);
}

// The translucence off, "Character_Ambient_Change <chara> 0 0 0 0 0" on bank, then the fade-away tail.
static void ssAI_EndTail(const CHARACTER *chr, char *chara, int bank)
{
    char *argv[6] = { chara, ssAI_Arg0, ssAI_Arg0, ssAI_Arg0, ssAI_Arg0, ssAI_Arg0 };

    tiSet_CharaTranslucenceOFF(chr->id, 1);
    tnXcmd_Character_Ambient_Change(6, argv, bank);
    ssAI_PartSteps(SS_AI_STEPS(ssAI_EndFade), bank);
}

// CT3 0x01864950: per stalker kind, its own model's triggers reset and weapons back, then its scene bank's
// triggers reset and effects off (omni light, translucence, ambient, fade-away). The axe stalker also resets
// its axe throw (whether or not E_04 is loaded).
void ssAI_BrainEndSpecialProc(CHARACTER *chr)
{
    int bank;

    switch (chr->id) {
    case STALKER_ID_6:
    case STALKER_ID_5: {
        int six = chr->id == STALKER_ID_6;
        char *chara = six ? ssAI_Arg6 : ssAI_Arg5;
        if (chr->bank != -1) {
            Trigger_reset(chr->bank);
            if (six)
                ssAI_PartSteps(SS_AI_STEPS(ssAI_End6Own), chr->bank);
            else
                ssAI_PartSteps(SS_AI_STEPS(ssAI_End5Own), chr->bank);
        }
        bank = ssGetBankID(ssAI_BankE05);
        if (bank == -1)
            return;
        Trigger_reset(bank);
        ssAI_PartSteps(SS_AI_STEPS(ssAI_EndE05), bank);
        ssAI_OmniOff(chara, bank);
        if (six)
            ssAI_PartSteps(SS_AI_STEPS(ssAI_EndE05Atk6), bank);
        else
            ssAI_PartSteps(SS_AI_STEPS(ssAI_EndE05Atk5), bank);
        ssAI_EndTail(chr, chara, bank);
        break;
    }
    case STALKER_ID_4:
        if (chr->bank != -1)
            Trigger_reset(chr->bank);
        bank = ssGetBankID(ssAI_BankE04);
        if (bank != -1) {
            Trigger_reset(bank);
            ssAI_PartSteps(SS_AI_STEPS(ssAI_EndE04), bank);
            ssAI_EndTail(chr, ssAI_Arg4, bank);
        }
        ssAI_AxeThrowReset();
        break;
    case STALKER_ID_3:
        if (chr->bank != -1) {
            char *argv[3] = { ssAI_SeRyusan, ssAI_Arg0, ssAI_SeArg1 };
            Trigger_reset(chr->bank);
            hhXcmd_StopSE(3, argv, chr->bank);      // "StopSE E_RYUSAN 0 1", then "... 1 1"
            argv[1] = ssAI_SeArg1;
            hhXcmd_StopSE(3, argv, chr->bank);
            ssAI_PartSteps(SS_AI_STEPS(ssAI_End3Own), chr->bank);
        }
        bank = ssGetBankID(ssAI_BankE03);
        if (bank == -1)
            return;
        Trigger_reset(bank);
        ssAI_PartSteps(SS_AI_STEPS(ssAI_EndE03), bank);
        ssAI_OmniOff(ssAI_SeArg3, bank);
        ssAI_EndTail(chr, ssAI_SeArg3, bank);
        break;
    case STALKER_ID_2:
        if (chr->bank != -1) {
            Trigger_reset(chr->bank);
            ssAI_PartSteps(SS_AI_STEPS(ssAI_End2Own), chr->bank);
        }
        bank = ssGetBankID(ssAI_BankE02);
        if (bank == -1)
            return;
        Trigger_reset(bank);
        ssAI_PartSteps(SS_AI_STEPS(ssAI_EndE02), bank);
        ssAI_EndTail(chr, ssAI_Arg2, bank);
        break;
    default:
        break;
    }
}

// ---- slice 5 (2026-11-12): the kinds' motion predicates, the group links, the route ring, the arrivals ----

// CT3 0x018606c0: 1 while a stalker plays one of its kind's forward (walk / run) motions.
int ssAI_IsForwardPlaying(CHARACTER *chr)
{
    static const short st3456[] = { 0x0c, 0x08, 0x05, 0x02 };
    static const short st5[] = { 0x0c, 0x08, 0x4d, 0x02 };
    static const short st2[] = { 0x72, 0x50, 0x4e, 0x05, 0x31 };

    switch (chr->id) {
    case STALKER_ID_6:
    case STALKER_ID_4:
    case STALKER_ID_3:
        return ssAI_MotionIn(chr, st3456, 4);
    case STALKER_ID_5:
        return ssAI_MotionIn(chr, st5, 4);
    case STALKER_ID_2:
        return ssAI_MotionIn(chr, st2, 5);
    default:
        return 0;
    }
}

// CT3 0x01860f40: 1 while the character stands waiting: motion 0 for every kind it knows, and also motion 1
// for the 0x1d kinds and 0x51 for STALKER_ID_5 (the ROM tests the extra one, then falls into the 0 test).
int ssAI_IsWaitPlaying(CHARACTER *chr)
{
    short m = ssAI_Motion(chr);

    switch (chr->id) {
    case AI_ID_1F:
    case AI_ID_1E:
    case AI_ID_1D:
        return m == 1 || m == 0;
    case STALKER_ID_5:
        return m == 0x51 || m == 0;
    case STALKER_ID_6:
    case STALKER_ID_4:
    case STALKER_ID_3:
    case STALKER_ID_2:
        return m == 0;
    default:
        return 0;
    }
}

// CT3 0x01861070: 1 while STALKER_ID_4 plays one of its two far attacks (the axe throw); 0 for every other
// kind.
int ssAI_IsFarAttackPlaying(CHARACTER *chr)
{
    static const short st4[] = { 0x2f, 0x2b };

    return chr->id == STALKER_ID_4 && ssAI_MotionIn(chr, st4, 2);
}

// CT3 0x01861110: 1 while the character plays one of its kind's after-attack motions.
int ssAI_IsPostAttackPlaying(CHARACTER *chr)
{
    static const short kind1d[] = { 0x0a, 0x08 };
    static const short st6[] = { 0x30, 0x43, 0x2d, 0x42, 0x2a, 0x41 };
    static const short st5[] = { 0x2a, 0x2b, 0x2d, 0x2e, 0x30, 0x31 };
    static const short st4[] = { 0x2d, 0x2e, 0x31, 0x32, 0x34, 0x35 };
    static const short st3[] = { 0x29, 0x2a, 0x2c, 0x2d, 0x2f, 0x30 };
    static const short st2[] = { 0x58, 0x59, 0x5a, 0x5b, 0x5d, 0x5e };

    switch (chr->id) {
    case AI_ID_1F:
    case AI_ID_1E:
    case AI_ID_1D:
        return ssAI_MotionIn(chr, kind1d, 2);
    case STALKER_ID_6:
        return ssAI_MotionIn(chr, st6, 6);
    case STALKER_ID_5:
        return ssAI_MotionIn(chr, st5, 6);
    case STALKER_ID_4:
        return ssAI_MotionIn(chr, st4, 6);
    case STALKER_ID_3:
        return ssAI_MotionIn(chr, st3, 6);
    case STALKER_ID_2:
        return ssAI_MotionIn(chr, st2, 6);
    default:
        return 0;
    }
}

// CT3 0x018612d0: 1 while the character plays one of its kind's attack motions.
int ssAI_IsAttackPlaying(CHARACTER *chr)
{
    static const short kind1d[] = { 0x07, 0x0c };
    static const short st6[] = { 0x26, 0x31, 0x2e, 0x2b, 0x28 };
    static const short st5[] = { 0x25, 0x32, 0x2f, 0x2c, 0x29 };
    static const short st4[] = { 0x27, 0x36, 0x33, 0x30, 0x2f, 0x2c, 0x2b };
    static const short st3[] = { 0x1e, 0x25, 0x32, 0x31, 0x2e, 0x2b, 0x28 };
    static const short st2[] = { 0x61, 0x5c, 0x40, 0x1d, 0x27, 0x02 };

    switch (chr->id) {
    case AI_ID_1F:
    case AI_ID_1E:
    case AI_ID_1D:
        return ssAI_MotionIn(chr, kind1d, 2);
    case STALKER_ID_6:
        return ssAI_MotionIn(chr, st6, 5);
    case STALKER_ID_5:
        return ssAI_MotionIn(chr, st5, 5);
    case STALKER_ID_4:
        return ssAI_MotionIn(chr, st4, 7);
    case STALKER_ID_3:
        return ssAI_MotionIn(chr, st3, 7);
    case STALKER_ID_2:
        return ssAI_MotionIn(chr, st2, 6);
    default:
        return 0;
    }
}

// CT3 0x01861510: 1 while the character plays one of its kind's wind-up (before the attack) motions.
int ssAI_IsPreAttackPlaying(CHARACTER *chr)
{
    static const short kind1d[] = { 0x06 };
    static const short st6[] = { 0x43, 0x42, 0x41 };
    static const short st5[] = { 0x30, 0x2d, 0x2a };
    static const short st4[] = { 0x34, 0x31, 0x2d };
    static const short st3[] = { 0x2f, 0x2c, 0x29 };
    static const short st2[] = { 0x15, 0x14 };

    switch (chr->id) {
    case AI_ID_1F:
    case AI_ID_1E:
    case AI_ID_1D:
        return ssAI_MotionIn(chr, kind1d, 1);
    case STALKER_ID_6:
        return ssAI_MotionIn(chr, st6, 3);
    case STALKER_ID_5:
        return ssAI_MotionIn(chr, st5, 3);
    case STALKER_ID_4:
        return ssAI_MotionIn(chr, st4, 3);
    case STALKER_ID_3:
        return ssAI_MotionIn(chr, st3, 3);
    case STALKER_ID_2:
        return ssAI_MotionIn(chr, st2, 2);
    default:
        return 0;
    }
}

// CT3 0x018616b0: 1 while the character plays one of its kind's "found her" motions.
int ssAI_IsFindPlaying(CHARACTER *chr)
{
    static const short kind1d[] = { 0x0e };
    static const short st356[] = { 0x26, 0x25 };
    static const short st4[] = { 0x28, 0x27 };
    static const short st2[] = { 0x4c, 0x3e, 0x3c };

    switch (chr->id) {
    case AI_ID_1F:
    case AI_ID_1E:
    case AI_ID_1D:
        return ssAI_MotionIn(chr, kind1d, 1);
    case STALKER_ID_6:
    case STALKER_ID_5:
    case STALKER_ID_3:
        return ssAI_MotionIn(chr, st356, 2);
    case STALKER_ID_4:
        return ssAI_MotionIn(chr, st4, 2);
    case STALKER_ID_2:
        return ssAI_MotionIn(chr, st2, 3);
    default:
        return 0;
    }
}

// CT3 0x01861a00: 1 while the character plays one of its kind's action motions.
int ssAI_IsActionPlaying(CHARACTER *chr)
{
    static const short kind1d[] = { 0x0c, 0x0d };
    static const short st36[] = { 0x24, 0x23 };
    static const short st5[] = { 0x24, 0x21 };
    static const short st4[] = { 0x26, 0x23 };
    static const short st2[] = { 0x3b, 0x3a, 0x16 };

    switch (chr->id) {
    case AI_ID_1F:
    case AI_ID_1E:
    case AI_ID_1D:
        return ssAI_MotionIn(chr, kind1d, 2);
    case STALKER_ID_6:
    case STALKER_ID_3:
        return ssAI_MotionIn(chr, st36, 2);
    case STALKER_ID_5:
        return ssAI_MotionIn(chr, st5, 2);
    case STALKER_ID_4:
        return ssAI_MotionIn(chr, st4, 2);
    case STALKER_ID_2:
        return ssAI_MotionIn(chr, st2, 3);
    default:
        return 0;
    }
}

// A brain's short / qword-position words (the brain is host memory: CharaBrainPtr).
static void ssAI_BrainVec(float *out, const u_char *brain, int off)
{
    memcpy(out, brain + off, 4 * sizeof(float));
}

// CT3 0x01860540: 1 when target (Alyssa) is out of chr's reach without a warp: chr's brain crossing a scene
// link (BRAIN_SUBMODE_SCN_LINK, two different rooms) reaches her only in either room; otherwise only when
// a shown shadow pole of her bank leads to chr's room.
int ssAI_IsWarpChara(CHARACTER *target, CHARACTER *chr)
{
    const u_char *brain = CharaBrainPtr(chr);
    u_int room, linkRoom;

    if (brain == NULL)
        return 0;                       /* PORT: the ROM reads the brain unguarded */
    memcpy(&room, brain + BRAIN_ROOM, sizeof(room));
    memcpy(&linkRoom, brain + BRAIN_LINK_ROOM, sizeof(linkRoom));
    if (brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_SCN_LINK && linkRoom != room) {
        if ((u_long)linkRoom == target->roomHash || (u_long)room == target->roomHash)
            return 0;
        return 1;
    }
    return ssAI_IsNextScene(target->lightBank, room) == 0;
}

// CT3 0x01861e70: 1 when chr stands within dist of pos.
int ssAI_IsGetToPoint(float dist, CHARACTER *chr, const float *pos)
{
    return ssAI_Dist(pos, (const float *)&chr->pos) < dist;
}

// CT3 0x01861ef0: 1 when chr stands within AI_ARRIVE_DIST of the bank's pole.
int ssAI_IsGetToPole(CHARACTER *chr, short pole, int bank)
{
    float pos[4];

    ssAI_GetPolePos(pos, pole, bank);
    return ssAI_Dist(pos, (const float *)&chr->pos) < AI_ARRIVE_DIST;
}

// CT3 0x01861fa0: 1 when chr stands within AI_ARRIVE_DIST of its brain's scene-end pole (in its own bank).
int ssAI_IsGetToScnEndPole(CHARACTER *chr)
{
    const u_char *brain = CharaBrainPtr(chr);
    float pos[4];

    if (brain == NULL)
        return 0;                       /* PORT: the ROM reads the brain unguarded */
    ssAI_GetPolePos(pos, ssAI_BrainShort(brain, BRAIN_SCN_END_POLE), chr->lightBank);
    return ssAI_Dist(pos, (const float *)&chr->pos) < AI_ARRIVE_DIST;
}

// CT3 0x01862050 / 0x018620e0: 1 when chr stands within AI_ARRIVE_DIST of its brain's point at off.
static int ssAI_IsGetToBrainPoint(CHARACTER *chr, int off)
{
    const u_char *brain = CharaBrainPtr(chr);
    float pos[4];

    if (brain == NULL)
        return 0;                       /* PORT: the ROM reads the brain unguarded */
    ssAI_BrainVec(pos, brain, off);
    return ssAI_Dist(pos, (const float *)&chr->pos) < AI_ARRIVE_DIST;
}

// CT3 0x01862050
int ssAI_IsGetToScnNextPoint(CHARACTER *chr)
{
    return ssAI_IsGetToBrainPoint(chr, BRAIN_SCN_NEXT_POINT);
}

// CT3 0x018620e0
int ssAI_IsGetToNextPoint(CHARACTER *chr)
{
    return ssAI_IsGetToBrainPoint(chr, BRAIN_NEXT_POINT);
}

// CT3 0x01862940: the group-link pole from group `group` to group `other`; -1 for none.
short ssAI_GetGLinkPolID(short group, short other, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);
    short i;

    for (i = 0; i < list->count; i++) {
        HIDE_POLE *p = &poles[i];
        if ((p->flags & HIDE_POLE_FLAG_GRP_LINK) != 0 && group == p->group && other == poles[p->pair].group)
            return i;
    }
    return -1;
}

// CT3 0x01862bd0: the pole this one pairs with, -1 when the pair is not a group link.
short ssAI_GetPairGrpLinkPoleID(short pole, int bank)
{
    short pair = ssAI_Pole(pole, bank)->pair;

    if ((ssAI_Pole(pair, bank)->flags & HIDE_POLE_FLAG_GRP_LINK) == 0)
        pair = -1;
    return pair;
}

// CT3 0x01862cb0 / 0x01862dd0 (the two are the same code): the bank's poles other than `except` whose part
// is shown and whose flags share a bit with `flags`, at most 256 of them; one answers itself, several one
// at random (tnRand), none -1.
static short ssAI_RandomPole(int bank, short except, u_char flags)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);
    short found[AI_RANDOM_POLE_MAX];
    int n = 0;
    short i;

    for (i = 0; i < list->count; i++) {
        HIDE_POLE *p = &poles[i];
        MODEL_PARTS *part = (MODEL_PARTS *)(pModelPartsHeader[bank] + p->parts * PARTS_REC_SIZE);

        if (i != except && (part->flags & PARTS_FLAG_HIDDEN) == 0 && (flags & p->flags) != 0) {
            found[n++] = i;
            if (n >= AI_RANDOM_POLE_MAX)
                break;
        }
    }
    if (n == 0)
        return -1;
    if (n == 1)
        return found[0];
    return found[tnRand(n)];
}

// CT3 0x01862cb0
short ssAI_GetNextTargPoleRandom2(int bank, short except, u_char flags)
{
    return ssAI_RandomPole(bank, except, flags);
}

// CT3 0x01862dd0
short ssAI_GetNextTargPoleRandom(int bank, short except, u_char flags)
{
    return ssAI_RandomPole(bank, except, flags);
}

// CT3 0x01862ef0: the pole's door number (the byte HIDE_POLE.shadowPol holds for a shadow pole).
u_char ssAI_GetPoleDoorNo(int pole, int bank)
{
    return ssAI_Pole(pole, bank)->shadowPol;
}

// CT3 0x01862f50: the middle of the group-link pole from group `group` to group `other`; 0, or -1 for none
// (pos untouched).
int ssAI_GetGLinkPolPos(float *pos, short group, short other, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);
    int i;

    for (i = 0; i < list->count; i++) {
        HIDE_POLE *p = &poles[i];
        if ((p->flags & HIDE_POLE_FLAG_GRP_LINK) != 0 && p->group == group && other == poles[p->pair].group) {
            gaInterVectorXYZ(pos, p->posA, p->posB, 0.5f);
            return 0;
        }
    }
    return -1;
}

// The route ring (HIDE_POLE_LIST.route: groupCount shorts, head .. tail; one slot always stays free).
// CT3 0x01863080: the front entry, taken off; -1 when the ring is empty.
short ssAI_GRS_PopFront(int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    short *ring = BANK_PTR(short *, bank, list->route);
    short head = list->routeHead;

    if (head == list->routeTail)
        return -1;
    list->routeHead = (short)((head + 1) % list->groupCount);
    return ring[head];
}

// CT3 0x018630f0: v onto the back; 0 when the ring is full.
int ssAI_GRS_PushRear(short v, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    short *ring = BANK_PTR(short *, bank, list->route);
    short next = (short)((list->routeTail + 1) % list->groupCount);

    if (next == list->routeHead)
        return 0;
    ring[list->routeTail] = v;
    list->routeTail = next;
    return 1;
}

// CT3 0x01863160: v onto the front; 0 when the ring is full.
int ssAI_GRS_PushFront(short v, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    short *ring = BANK_PTR(short *, bank, list->route);
    short prev = (short)((list->routeHead + list->groupCount - 1) % list->groupCount);

    if (prev == list->routeTail)
        return 0;
    ring[prev] = v;
    list->routeHead = prev;
    return 1;
}

// CT3 0x01863330: the distance from pos to the group link between group records `from` and `to` (indices
// into HIDE_POLE_LIST.groups); 0 when the groups have no link.
float ssAI_GRS_GetTargDistance(short from, short to, const float *pos, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE_GROUP *groups = BANK_PTR(HIDE_POLE_GROUP *, bank, list->groups);
    float link[4];

    if (ssAI_GetGLinkPolPos(link, groups[from].id, groups[to].id, bank) == -1)
        return 0.0f;
    return ssAI_Dist(pos, link);
}

// ---- slice 6 (2026-11-12): the shadow entry, the placement, the sight checks, the group route search ----

static u_int ssAI_BrainWord(const u_char *brain, int off)
{
    u_int v;

    memcpy(&v, brain + off, sizeof(v));
    return v;
}

static void ssAI_BrainSetWord(u_char *brain, int off, u_int v)
{
    memcpy(brain + off, &v, sizeof(v));
}

// CT3 0x01862170
int ssAI_SetMonsterPos(CHARACTER *chr, const float *pos, short group, u_char bank, u_int room, int endProc)
{
    u_char *brain = CharaBrainPtr(chr);

    memcpy(&chr->pos, pos, sizeof(chr->pos));
    memcpy(&chr->posBefore, pos, sizeof(chr->posBefore));
    chr->flags |= CHARACTER_FLAG_AI_PARKED;
    if (brain == NULL)
        return 0;                       /* PORT: the ROM reads the brain unguarded */
    if (endProc) {
        ssAI_BrainEndSpecialProc(chr);
        asCharacterMortionChangeRequest(chr->id, 0, ssAI_BrainShort(brain, BRAIN_END_MOTION));
    }
    chr->h100 = group;
    ssAI_BrainSetShort(brain, BRAIN_POLE_GROUP, group);
    if (room != 0) {
        brain[BRAIN_BANK] = bank;
        chr->lightBank = bank;
        chr->roomHashPrev = chr->roomHash;
        ssAI_BrainSetWord(brain, BRAIN_ROOM, room);
        chr->roomHash = room;
        if (brain[BRAIN_MODE] != SS_AI_TRACE)
            return 0;
        if (ssAI_BrainWord(brain, BRAIN_ROOM) != ssAI_BrainWord(brain, BRAIN_GOAL_ROOM) &&
            ssAI_BrainShort(brain, BRAIN_SCN_END_POLE) == -1 && ssAI_SetTraceMode(chr, NULL) == -1)
            return -1;
    }
    return 0;
}

// CT3 0x0185f970
int ssAI_LetMonsterInFromShadow(CHARACTER *chr, u_char otherBank, u_char bank, s_char shadowPol, short motion)
{
    u_char *brain = CharaBrainPtr(chr);
    float pos[4], dir[4];
    short pole, pair, group, targ;
    u_int room;
    s_char linkBank;

    pole = ssAI_GetShadowPolID(shadowPol, bank);
    if (pole == -1)
        return -1;
    pair = ssAI_GetPairScnLinkPoleID(pole, bank);
    if (pair == -1)
        return -1;
    if (brain == NULL)
        return -1;                      /* PORT: the ROM reads the brain unguarded */
    ssAI_GetPolePos(pos, pole, bank);
    if (!ssAI_CheckPoleFlg(pole, bank, HIDE_POLE_FLAG_CROSS_BANK)) {
        group = ssAI_GetPoleGroupID(pole, bank);
        room = ssAI_GetPoleScnName(pair, bank);
        if (chr->roomHash == (u_long)room)
            room = 0;
        if (ssAI_SetMonsterPos(chr, pos, group, bank, room, 0) == -1)
            return -1;
    } else {
        int closed;

        if (!ssAI_CheckPoleFlg(pole, bank, HIDE_POLE_FLAG_OWN_GROUP)) {
            short other;

            if (BankInfo[otherBank].state < 1)
                return -1;
            other = ssAI_GetShadowPolID(shadowPol, otherBank);
            if (other == -1)
                return -1;
            group = ssAI_GetPoleGroupID(other, otherBank);
        } else {
            group = ssAI_GetPoleGroupID(pole, bank);
        }
        room = ssAI_GetPoleScnName(pole, bank);
        if (chr->roomHash == (u_long)room)
            room = 0;
        if (ssAI_SetMonsterPos(chr, pos, group, otherBank, room, 0) == -1)
            return -1;
        closed = ssAI_IsDoorClose(shadowPol);
        if (closed == -1)
            return -1;
        if (closed != 0) {
            short door = ssAI_CheckPoleFlg(pole, bank, HIDE_POLE_FLAG_DOOR_ALT)
                             ? ssAI_BrainShort(brain, BRAIN_DOOR_MOTION_ALT)
                             : ssAI_BrainShort(brain, BRAIN_DOOR_MOTION);
            short end = ssAI_BrainShort(brain, BRAIN_DOOR_MOTION_END);

            asCharacterMortionChangeRequest(chr->id, 0, door);
            asChangeMortionInfo_NextMortion(chr->id, door, end, -1);
            asChangeMortionInfo_NextMortion(chr->id, end, motion, -1);
        }
    }
    ssAI_BrainSetShort(brain, BRAIN_TARG_POLE, pair);
    ssAI_BrainSetShort(brain, BRAIN_LINK_POLE, pair);
    targ = ssAI_BrainShort(brain, BRAIN_TARG_POLE);
    ssAI_GetPolePos(pos, targ, bank);
    memcpy(brain + BRAIN_NEXT_POINT, pos, sizeof(pos));
    memcpy(brain + BRAIN_SCN_NEXT_POINT, pos, sizeof(pos));
    linkBank = ssAI_GetPoleScnBnkID(targ, bank);
    brain[BRAIN_LINK_BANK] = (u_char)linkBank;
    if (linkBank == -1)
        return -1;
    ssAI_BrainSetWord(brain, BRAIN_LINK_ROOM, ssAI_GetPoleScnName(targ, bank));
    ssAI_DirTo(dir, pos, (const float *)&chr->pos);
    ssAI_CompCharaDir(chr, dir, AI_TURN_FULL, 1);
    brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_SCN_LINK;
    return 0;
}

// CT3 0x018600e0
int ssAI_BarrierCheck(CHARACTER *chr, const float *pos, int bank)
{
    int low[4];
    int n = 0;
    int i, j;
    LOS_HIT los;

    for (i = 0; i < 8; i++) {
        int above = 0;

        for (j = 0; j < 8; j++) {
            if (chr->obb[i][1] < chr->obb[j][1] && ++above == 4) {
                low[n++] = i;
                break;
            }
        }
        if (n == 4)
            break;
    }
    memcpy(los.seg.to, pos, sizeof(los.seg.to));
    los.seg.to[1] += AI_BARRIER_LIFT;
    for (i = 0; i < n; i++) {
        memcpy(los.seg.from, chr->obb[low[i]], sizeof(los.seg.from));
        los.seg.from[1] += AI_BARRIER_LIFT;
        if (CheckLOS(bank, &los) != 0)
            return 1;
    }
    return 0;
}

// CT3 0x018605d0
int ssAI_IsTrgWithinAngle(const float *dir, CHARACTER *chr, int fov)
{
    int heading = (int)(long long)((ssRadianByO(dir, 1) * 65536.0) / 6.283185307179586) & 0xffff;
    int rot = chr->rot;
    int d;

    if (rot < 0)
        rot += AI_TURN_FULL;
    else if (rot >= AI_TURN_FULL)
        rot -= AI_TURN_FULL;
    d = (AI_TURN_FULL - heading) - rot;
    if (d < 0)
        d = -d;
    if (d > AI_TURN_FULL / 2)
        d = AI_TURN_FULL - d;
    return d < fov / 2;
}

// CT3 0x01860250
int ssAI_SerchCheck(float dist, CHARACTER *chr, CHARACTER *target, int fov, int bank)
{
    const float *from = (const float *)&chr->pos;
    const float *to = (const float *)&target->pos;
    float d[4];
    LOS_HIT los;
    int i;

    for (i = 0; i < 4; i++)
        d[i] = to[i] - from[i];
    if (dist < ssAI_Dist(to, from))
        return 0;
    ssAI_Unit(d, d);
    if (!ssAI_IsTrgWithinAngle(d, chr, fov))
        return 0;
    memcpy(los.seg.from, from, sizeof(los.seg.from));
    los.seg.from[1] += AI_SEARCH_EYE;
    memcpy(los.seg.to, to, sizeof(los.seg.to));
    if (CheckLOS(bank, &los) == 0)
        return 1;
    if (ssAI_IsBendPlaying(target) || ssAI_IsDownPlaying(target))
        los.seg.to[1] = to[1] + AI_SEARCH_HEAD_LOW;
    else
        los.seg.to[1] = to[1] + AI_SEARCH_HEAD;
    return CheckLOS(bank, &los) == 0;
}

// CT3 0x01861b90
int ssAI_IsGetToGLinkPole(CHARACTER *chr, short pole, int bank)
{
    HIDE_POLE *p = ssAI_Pole(pole, bank);
    const float *at = (const float *)&chr->pos;
    float ab[4], ac[4], foot[4];
    float len, t;
    int i;

    if ((p->flags & HIDE_POLE_FLAG_GRP_LINK) == 0)
        return -1;
    for (i = 0; i < 4; i++) {
        ab[i] = p->posB[i] - p->posA[i];
        ac[i] = at[i] - p->posA[i];
    }
    len = ssAI_Dist(p->posB, p->posA);
    if (len == 0.0f)
        return 0;
    if (ssAI_Dist(at, p->posA) < AI_ARRIVE_DIST)
        return 1;
    if (ssAI_Dist(at, p->posB) < AI_ARRIVE_DIST)
        return 1;
    t = gaInnerProductXYZ(ac, ab) / (len * len);
    if (t < 0.0f || t > 1.0f)
        return 0;
    for (i = 0; i < 4; i++)
        foot[i] = p->posA[i] + ab[i] * t;
    return ssAI_Dist(at, foot) < AI_ARRIVE_DIST;
}

// CT3 0x01862290 (XZ plane: x the first lane, z the third; y interpolated along x, or along z for a link
// parallel to the z axis).
int ssAI_SetNextGLinkPolePoint(short pole, int bank, CHARACTER *chr, const float *pos)
{
    HIDE_POLE *p = ssAI_Pole(pole, bank);
    u_char *brain = CharaBrainPtr(chr);
    const float *c = (const float *)&chr->pos;
    float ax = p->posA[0], ay = p->posA[1], az = p->posA[2];
    float bx = p->posB[0], by = p->posB[1], bz = p->posB[2];
    float dx = bx - ax, dz = bz - az;
    float px = pos[0], pz = pos[2];
    float cx = c[0], cz = c[2];
    float x, y, z, lo, hi;
    const float *end;
    float out[4];

    if (brain == NULL)
        return -1;                      /* PORT: the ROM reads the brain unguarded */
    // Both on one side of the link: mirror pos across it, so the line crosses it.
    if ((dz * (px - ax) - dx * (pz - az)) * (dz * (cx - ax) - dx * (cz - az)) > 0.0f) {
        float t = (dz * (pz - az) + dx * (px - ax)) / (dz * dz + dx * dx);
        float t2 = t * 2.0f, s2 = (1.0f - t) * 2.0f;

        px = (t2 * bx + s2 * ax) - px;
        pz = (t2 * bz + s2 * az) - pz;
    }
    if (cx - px == 0.0f) {
        // The line runs along z at x = px.
        if (dx == 0.0f) {
            end = fabsf(pz - bz) <= fabsf(pz - az) ? p->posB : p->posA;
            goto at_end;
        }
        if (ax < bx) {
            if (px < ax) { end = p->posA; goto at_end; }
            if (bx < px) { end = p->posB; goto at_end; }
        } else {
            if (ax < px) { end = p->posA; goto at_end; }
            if (px < bx) { end = p->posB; goto at_end; }
        }
        {
            float m = dz / dx;
            x = px;
            z = m * px + (bz - m * bx);
            y = ay + ((ax - px) * (by - ay)) / (ax - bx);
        }
    } else {
        float k = (cz - pz) / (cx - px);
        float b = cz - k * cx;

        if (dx == 0.0f) {
            // The link runs along z at x = ax.
            z = b + k * ax;
            if (az < bz) {
                if (z < az) { end = p->posA; goto at_end; }
                if (bz < z) { end = p->posB; goto at_end; }
            } else {
                if (az < z) { end = p->posA; goto at_end; }
                if (z < bz) { end = p->posB; goto at_end; }
            }
            y = ay + ((az - z) * (by - ay)) / (az - bz);
            x = ax;
        } else {
            float m = dz / dx;
            float c0 = bz - m * bx;

            if (k == m) {
                end = fabsf(pz - bz) <= fabsf(pz - az) ? p->posB : p->posA;
                goto at_end;
            }
            x = (b - c0) / (m - k);
            z = c0 + m * x;
            if (ax < bx) {
                if (x < ax) { end = p->posA; goto at_end; }
                if (bx < x) { end = p->posB; goto at_end; }
            } else {
                if (ax < x) { end = p->posA; goto at_end; }
                if (x < bx) { end = p->posB; goto at_end; }
            }
            y = ay + ((ax - x) * (by - ay)) / (ax - bx);
        }
    }
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = 1.0f;
    memcpy(brain + BRAIN_NEXT_POINT, out, sizeof(out));
    lo = ax < bx ? ax : bx;
    hi = ax < bx ? bx : ax;
    return (lo <= x && x <= hi) ? 0 : -1;

at_end:
    memcpy(brain + BRAIN_NEXT_POINT, end, 4 * sizeof(float));
    return 0;
}

// CT3 0x018629f0
short ssAI_GetScnLinkPoleID(const float *pos, u_int room, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE *poles = BANK_PTR(HIDE_POLE *, bank, list->poles);
    float best = 1e10f;
    short found = -1;
    short i;

    for (i = 0; i < list->count; i++) {
        HIDE_POLE *p = &poles[i];
        MODEL_PARTS *part = (MODEL_PARTS *)(pModelPartsHeader[bank] + p->parts * PARTS_REC_SIZE);
        short pair;

        if ((part->flags & PARTS_FLAG_HIDDEN) == 0 && (p->flags & HIDE_POLE_FLAG_SHADOW) != 0 &&
            room == p->room && (pair = ssAI_GetPairScnLinkPoleID(i, bank)) != -1) {
            float at[4];
            float d;

            ssAI_GetPolePos(at, pair, bank);
            d = ssAI_Dist(pos, at);
            if (d < best) {
                best = d;
                found = pair;
            }
        }
    }
    return found;
}

// CT3 0x01863870: the route ring emptied and every group's search fields cleared.
void ssAI_GRS_InitParam(int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE_GROUP *groups = BANK_PTR(HIDE_POLE_GROUP *, bank, list->groups);
    int i;

    list->routeHead = 0;
    list->routeTail = 0;
    for (i = 0; i < list->groupCount; i++) {
        groups[i].visited = 0;
        groups[i].prev = -1;
        groups[i].cost = 0.0f;
    }
}

// CT3 0x018631d0: the length of the step out of group record `group` through its link `link`: from pos to
// that link pole for the start group (prev -1), else the table's distance from the link it came in by; 0
// when neither is known.
float ssAI_GRS_GetNextDistance(short group, int link, const float *pos, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE_GROUP *groups = BANK_PTR(HIDE_POLE_GROUP *, bank, list->groups);
    HIDE_POLE_GROUP *g = &groups[group];
    short *links = BANK_PTR(short *, bank, g->links);
    int i;

    if (g->prev == -1) {
        float at[4];

        if (ssAI_GetGLinkPolPos(at, g->id, groups[links[link]].id, bank) == -1)
            return 0.0f;
        return ssAI_Dist(pos, at);
    }
    for (i = 0; i < g->linkCount; i++) {
        if (g->prev == links[i])
            return BANK_PTR(float *, bank, g->linkDist)[link + i * g->linkCount];
    }
    return 0.0f;
}

// CT3 0x01863420: relaxes every link out of record `from`, recursing into groups whose cost dropped; the
// target takes the last leg to targPos. (The ROM marks `from`, not the next group, while it recurses.)
void ssAI_GRS_RouteGrpRecur(HIDE_POLE_GROUP *groups, const float *pos, const float *targPos, short from,
                            short to, int bank)
{
    HIDE_POLE_GROUP *g = &groups[from];
    HIDE_POLE_GROUP *target = &groups[to];
    short *links = BANK_PTR(short *, bank, g->links);
    int n = g->linkCount;
    int i;

    for (i = 0; i < n; i++) {
        short next = links[i];
        HIDE_POLE_GROUP *ng = &groups[next];
        float d;

        if (ng->visited == 1)
            continue;
        d = g->cost + ssAI_GRS_GetNextDistance(from, i, pos, bank);
        if (ng == target) {
            float last = ssAI_GRS_GetTargDistance(next, from, targPos, bank);

            if (target->cost == 0.0f || d + last <= target->cost) {
                ng->prev = from;
                ng->cost = d + last;
            }
        } else if ((ng->cost == 0.0f || d <= ng->cost) && (target->cost == 0.0f || d <= target->cost)) {
            ng->prev = from;
            ng->cost = d;
            g->visited = 1;
            ssAI_GRS_RouteGrpRecur(groups, pos, targPos, next, to, bank);
            g->visited = 0;
        }
    }
}

// CT3 0x01863640
float ssAI_GrpRouteSearch(const float *pos, const float *targPos, short group, short targGroup, int bank)
{
    HIDE_POLE_LIST *list = ssAI_PoleList(bank);
    HIDE_POLE_GROUP *groups;
    short from = -1, to = -1;
    short i;

    if (group == targGroup) {
        ssAI_GRS_InitParam(bank);
        return ssAI_Dist(targPos, pos);
    }
    groups = BANK_PTR(HIDE_POLE_GROUP *, bank, list->groups);
    for (i = 0; i < list->groupCount && (from == -1 || to == -1); i++) {
        if (groups[i].id == group)
            from = i;
        else if (groups[i].id == targGroup)
            to = i;
    }
    if (to == -1 || from == -1)
        return AI_ROUTE_NONE;
    ssAI_GRS_InitParam(bank);
    ssAI_GRS_RouteGrpRecur(groups, pos, targPos, from, to, bank);
    for (i = to; groups[i].prev != -1; i = groups[i].prev)
        ssAI_GRS_PushFront(groups[i].id, bank);
    return groups[to].cost;
}
