// game/ssBrain_ChgHideAtk1.c -- Clock Tower 3: the stalkers' brain (ssBrain_*1: one brain's mode functions;
// ssAI_Set*Mode / ssAI_<From>To<To>Mode: entering a mode). Symtab local chunk 82. CodeWarrior C.
// Slice 1 (2026-11-12): the mode changes ssBrain_Chg{HideAtk,Return,Attack,Trace,Approach}1, ssBrain_Return1,
// every transition and ssAI_SetHideAtkMode. Slice 2 (2026-11-12): the setters Set{Trace,Attack,Approach,
// Search}Mode, SearchToApproachMode, ChgSearch1 and Damage1. Left: the mode bodies (Attack1, Trace1,
// Approach1, Search1, HideAtk1) and Init1 (stubs below).

#include "common/alyssapan_stub.h"
#include "game/ssBrain_ChgHideAtk1.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/tiAvoidObject.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"

#include <float.h>
#include <math.h>
#include <string.h>

#define TRIG_GLOBAL 1
#define TRIG_ANY_BANK (-1)

// The brain is host memory (CharaBrainPtr); its words are read unaligned-safe.
static short ssBrain_Short(const u_char *brain, int off)
{
    short v;
    memcpy(&v, brain + off, sizeof(v));
    return v;
}

static int ssBrain_Int(const u_char *brain, int off)
{
    int v;
    memcpy(&v, brain + off, sizeof(v));
    return v;
}

static void ssBrain_SetInt(u_char *brain, int off, int v)
{
    memcpy(brain + off, &v, sizeof(v));
}

static float ssBrain_Float(const u_char *brain, int off)
{
    float v;
    memcpy(&v, brain + off, sizeof(v));
    return v;
}

static void ssBrain_SetShort(u_char *brain, int off, short v)
{
    memcpy(brain + off, &v, sizeof(v));
}

static void ssBrain_AtkState(BRAIN_ATK_STATE *st, const u_char *brain, int i)
{
    memcpy(st, brain + BRAIN_ATK_STATES + i * (int)sizeof(BRAIN_ATK_STATE), sizeof(*st));
}

// The setters' pick: state i by its weight in column `field` (a u_char of BRAIN_ATK_STATE), taken with
// weight / (the weights left, from AI_ATK_WEIGHT); with checkWait a state whose motion is the end motion
// is passed over while chr waits. clearStep: SetAttackMode's picks also clear BRAIN_ATK_STEP. 1 = picked.
static int ssBrain_PickAtk(CHARACTER *chr, u_char *brain, size_t field, int checkWait, int clearStep)
{
    int count = brain[BRAIN_ATK_COUNT];
    int left = AI_ATK_WEIGHT;
    int i;

    for (i = 0; i < count; i++) {
        BRAIN_ATK_STATE st;
        int w;

        ssBrain_AtkState(&st, brain, i);
        w = ((const u_char *)&st)[field];
        if (ssTrigCheckRandom(w * 100 / left) != 0 &&
            (!checkWait || ssAI_IsWaitPlaying(chr) == 0 ||
             (u_short)st.motion != (u_short)ssBrain_Short(brain, BRAIN_END_MOTION))) {
            brain[BRAIN_ATK_PICK] = (u_char)i;
            if (clearStep)
                brain[BRAIN_ATK_STEP] = 0;
            return 1;
        }
        left -= w;
        if (left < 1)
            break;
    }
    return 0;
}

// CHARACTER words outside the struct's named fields (wideXcmd_DebugPrint.h / ahRelocateSunPs2ObjectArry.h).
static short ssBrain_CharaShort(const CHARACTER *chr, int off)
{
    short v;
    memcpy(&v, (const u_char *)chr + off, sizeof(v));
    return v;
}

static void ssBrain_SetCharaInt(CHARACTER *chr, int off, int v)
{
    memcpy((u_char *)chr + off, &v, sizeof(v));
}

// The target's place when a mode is set (Set{Search,Attack,Approach}Mode).
static void ssBrain_NoteTarget(u_char *brain, const CHARACTER *target)
{
    brain[BRAIN_PLAYER_BANK] = (u_char)target->lightBank;
    ssBrain_SetInt(brain, BRAIN_PLAYER_ROOM, (int)(u_int)target->roomHash);
}

// The scene grid of a bank (ALL_TABLE.hits): its cell size scales the search times and tells a wide room.
static const HIT_SCENE_GRID *ssBrain_Grid(int bank)
{
    return BANK_PTR(const HIT_SCENE_GRID *, bank, All_table_TBL[bank]->hits);
}

// The ROM's every mode change: a transition that fails ends the brain with its end motion.
static void ssBrain_EndIfFailed(CHARACTER *chr, const u_char *brain, int result)
{
    if (result == -1)
        ssAI_ForceBrainEnd(chr, ssBrain_Short(brain, BRAIN_END_MOTION));
}

static int ssBrain_BankDrawn(int bank)
{
    return BankInfo[bank].drawOn != 0;
}

// The brain's room (BRAIN_ROOM, a word) against a character's 64-bit roomHash, zero-extended as the ROM does.
static int ssBrain_InRoomOf(const u_char *brain, const CHARACTER *target)
{
    return (u_long)(u_int)ssBrain_Int(brain, BRAIN_ROOM) == target->roomHash;
}

// vsub, vmul, vaddbc x2, vrsqrt, then vdiv 1 / Q: the length of (to - chr's position), xyz. The VU's RSQRT
// of 0 is the largest float, whose reciprocal is (all but) 0.
static float ssBrain_DistTo(const CHARACTER *chr, const float *to)
{
    const float *pos = (const float *)chr->pos;
    float dx = to[0] - pos[0], dy = to[1] - pos[1], dz = to[2] - pos[2];
    float len2 = dx * dx + dy * dy + dz * dz;
    return len2 > 0.0f ? sqrtf(len2) : 0.0f;
}

// The unit direction from chr to `to` (ssAI_DirTo's VU0 sequence in tiAvoidObject.c).
static void ssBrain_DirTo(float *dir, const CHARACTER *chr, const float *to)
{
    const float *pos = (const float *)chr->pos;
    float d[4];
    float len2, q;
    int i;

    for (i = 0; i < 4; i++)
        d[i] = to[i] - pos[i];
    len2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;
    for (i = 0; i < 4; i++)
        dir[i] = d[i] * q;
}

// ---- the mode changes (ssAI_charaMain's ssAI_Chg<Mode>Mode table entries of brain kind 1) ----

// CT3 0x0187b160
// Hiding attack: Alyssa leaves her hiding place -> attack; she hides on but the hide pole is out of
// reach, or she turned invisible -> trace.
void ssBrain_ChgHideAtk1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float pos[4];

    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, TRIG_GLOBAL, TRIG_ANY_BANK) == 0) {
        ssBrain_EndIfFailed(chr, brain, ssAI_HideAtkToAttackMode(chr));
    } else if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, TRIG_GLOBAL, TRIG_ANY_BANK) == 0) {
        ssAI_GetPolePos(pos, ssBrain_Short(brain, BRAIN_LINK_POLE), brain[BRAIN_BANK]);
        if (ssAI_BarrierCheck(chr, pos, brain[BRAIN_BANK]) != 0)
            ssBrain_EndIfFailed(chr, brain, ssAI_HideAtkToTraceMode(chr));
    } else {
        ssBrain_EndIfFailed(chr, brain, ssAI_HideAtkToTraceMode(chr));
    }
}

// CT3 0x0187b260
void ssBrain_ChgReturn1(CHARACTER *chr)
{
    (void)chr;
}

// CT3 0x0187b270
void ssBrain_ChgAttack1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    CHARACTER *player = &Character[Player_BankID];
    const float *playerPos = (const float *)player->pos;
    float pos[4];
    float dy;
    short pole;

    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        // She hides: in her room, a strike from a hide pole if the dice allow and a pole is in reach.
        if (ssBrain_InRoomOf(brain, player) && ssAI_CheckHideAreaAtkRate() != 0) {
            pole = ssAI_GetNextTargPoleRandom(brain[BRAIN_BANK], -1, HIDE_POLE_FLAG_ATTACK);
            if (pole == -1) {
                ssAI_ForceBrainEnd(chr, ssBrain_Short(brain, BRAIN_END_MOTION));
            } else {
                ssAI_GetPolePos(pos, pole, brain[BRAIN_BANK]);
                if (ssAI_BarrierCheck(chr, pos, brain[BRAIN_BANK]) == 0) {
                    ssTrigSetVariable(TRIG_VAR_HIDE_POINT, AI_HIDE_LEVEL_SURE, TRIG_GLOBAL, TRIG_ANY_BANK);
                    ssBrain_EndIfFailed(chr, brain, ssAI_AttackToHideAtkMode(chr));
                } else {
                    ssBrain_EndIfFailed(chr, brain, ssAI_AttackToTraceMode(chr));
                }
            }
        } else {
            ssBrain_EndIfFailed(chr, brain, ssAI_AttackToTraceMode(chr));
        }
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        ssBrain_EndIfFailed(chr, brain, ssAI_AttackToTraceMode(chr));
        return;
    }
    if (ssAI_IsAttackPlaying(chr) == 0 && ssAI_IsPostAttackPlaying(chr) == 0) {
        // Between strikes: out of the drawn world, out of reach or behind something -> approach.
        if (!ssBrain_BankDrawn(brain[BRAIN_BANK])) {
            ssBrain_EndIfFailed(chr, brain, ssAI_AttackToApproachMode(chr));
            return;
        }
        float dist = ssBrain_DistTo(chr, playerPos);
        dy = fabsf(playerPos[1] - ((const float *)chr->pos)[1]);
        if (AI_ATTACK_HEIGHT < dy || ssBrain_Float(brain, BRAIN_ATTACK_LEAVE) < dist) {
            ssBrain_EndIfFailed(chr, brain, ssAI_AttackToApproachMode(chr));
            return;
        }
        if (ssAI_BarrierCheck(chr, playerPos, brain[BRAIN_BANK]) != 0) {
            ssBrain_EndIfFailed(chr, brain, ssAI_AttackToApproachMode(chr));
            return;
        }
    }
    if (ssAI_IsFarAttackPlaying(chr) == 0 && !ssBrain_InRoomOf(brain, player)) {
        // She left the room: a strike in progress puts its effects back first.
        if (ssAI_IsAttackPlaying(chr) != 0 || ssAI_IsPostAttackPlaying(chr) != 0)
            ssAI_BrainEndSpecialProc(chr);
        ssBrain_EndIfFailed(chr, brain, ssAI_AttackToApproachMode(chr));
    }
}

// CT3 0x0187b640
void ssBrain_ChgTrace1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    CHARACTER *player = &Character[Player_BankID];
    int ticks;

    if (ssTrigCheckEvent(TRIG_EVENT_AI_APPEAR_NOW, TRIG_GLOBAL, TRIG_ANY_BANK) != 0)
        ssBrain_SetInt(brain, BRAIN_TRACE_TIMER, 0);
    if (!ssBrain_BankDrawn(brain[BRAIN_BANK])) {
        if (ssBrain_Int(brain, BRAIN_GOAL_ROOM) != ssBrain_Int(brain, BRAIN_PLAYER_ROOM))
            ssBrain_EndIfFailed(chr, brain, ssAI_TraceToSearchMode(chr));
        return;
    }
    ticks = ssBrain_Int(brain, BRAIN_TICKS);
    if (ticks - ssBrain_Int(brain, BRAIN_TRACE_PLAN_TICKS) >= AI_TRACE_REPLAN_TICKS) {
        ssBrain_SetInt(brain, BRAIN_TRACE_PLAN_TICKS, ticks);
        if (ssAI_SetTraceMode(chr, &Character[Player_BankID]) == -1) {
            ssAI_ForceBrainEnd(chr, ssBrain_Short(brain, BRAIN_END_MOTION));
            return;
        }
    }
    if (brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_1 || brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_SCN_LINK)
        return;
    if (!ssBrain_BankDrawn(player->lightBank)) {
        ssAI_ForceBrainEnd(chr, ssBrain_Short(brain, BRAIN_END_MOTION));
        return;
    }
    // Sees her (a narrower look than an approach's): approach.
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, TRIG_GLOBAL, TRIG_ANY_BANK) == 0 &&
        ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, TRIG_GLOBAL, TRIG_ANY_BANK) == 0) {
        float sight = ssBrain_Float(brain, BRAIN_SIGHT) - AI_SIGHT_MARGIN;
        int fov = ssBrain_Int(brain, BRAIN_FOV) - AI_FOV_MARGIN;
        int seen;

        if (ssBrain_InRoomOf(brain, player))
            seen = ssAI_SerchCheck(sight, chr, player, fov, player->lightBank) != 0;
        else
            seen = ssAI_IsNextScene(player->lightBank, (u_int)ssBrain_Int(brain, BRAIN_ROOM)) != 0 &&
                   ssAI_SerchCheck(sight, chr, player, fov, brain[BRAIN_BANK]) != 0 &&
                   ssAI_SerchCheck(sight, chr, player, fov, player->lightBank) != 0;
        if (seen) {
            ssBrain_EndIfFailed(chr, brain, ssAI_TraceToApproachMode(chr));
            return;
        }
    }
    // At the trace's end in its goal room: search, unless she is in plain view.
    if (ssBrain_Int(brain, BRAIN_GOAL_ROOM) != ssBrain_Int(brain, BRAIN_ROOM))
        return;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        if (ssAI_IsGetToPoint(AI_TRACE_ARRIVE_HIDING, chr, (const float *)(brain + BRAIN_TRACE_POINT)) != 0)
            ssBrain_EndIfFailed(chr, brain, ssAI_TraceToSearchMode(chr));
    } else if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        if (ssAI_IsGetToPoint(AI_TRACE_ARRIVE, chr, (const float *)(brain + BRAIN_TRACE_POINT)) != 0)
            ssBrain_EndIfFailed(chr, brain, ssAI_TraceToSearchMode(chr));
    } else if (ssAI_IsGetToPoint(AI_TRACE_ARRIVE, chr, (const float *)(brain + BRAIN_TRACE_POINT)) != 0) {
        if (ssAI_SerchCheck(ssBrain_Float(brain, BRAIN_SIGHT), chr, player, AI_TURN_FULL, chr->lightBank) == 0)
            ssBrain_EndIfFailed(chr, brain, ssAI_TraceToSearchMode(chr));
        else
            ssBrain_EndIfFailed(chr, brain, ssAI_TraceToApproachMode(chr));
    }
}

// CT3 0x0187ba60
void ssBrain_ChgApproach1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    CHARACTER *player = &Character[Player_BankID];
    const float *playerPos = (const float *)player->pos;
    float dist, dy;

    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        // She hid: in her room and with the dice, she is found out (the hiding place's level at its surest).
        if (!ssBrain_InRoomOf(brain, player) || ssAI_CheckHideAreaAtkRate() == 0) {
            ssBrain_EndIfFailed(chr, brain, ssAI_ApproachToTraceMode(chr));
            return;
        }
        ssTrigSetVariable(TRIG_VAR_HIDE_POINT, AI_HIDE_LEVEL_SURE, TRIG_GLOBAL, TRIG_ANY_BANK);
    }
    if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        ssBrain_EndIfFailed(chr, brain, ssAI_ApproachToTraceMode(chr));
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_AI_APPEAR_NOW, TRIG_GLOBAL, TRIG_ANY_BANK) != 0)
        ssBrain_SetInt(brain, BRAIN_APPROACH_TIMER, 0);
    if (!ssBrain_BankDrawn(brain[BRAIN_BANK]) || !ssBrain_BankDrawn(player->lightBank))
        return;
    dist = ssBrain_DistTo(chr, playerPos);
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        if (ssBrain_InRoomOf(brain, player) && brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_HIDE_POLE)
            ssBrain_EndIfFailed(chr, brain, ssAI_ApproachToHideAtkMode(chr));
        return;
    }
    if (ssBrain_InRoomOf(brain, player)) {
        dy = fabsf(playerPos[1] - ((const float *)chr->pos)[1]);
        if (dy < AI_ATTACK_HEIGHT && dist < ssBrain_Float(brain, BRAIN_ATTACK_RANGE) &&
            ssAI_BarrierCheck(chr, playerPos, brain[BRAIN_BANK]) == 0) {
            ssBrain_EndIfFailed(chr, brain, ssAI_ApproachToAttackMode(chr));
            return;
        }
    }
    // Far off: a wider look than a trace's; lost from sight -> trace.
    if (brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_1 || brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_SCN_LINK ||
        !(ssBrain_Float(brain, BRAIN_LOSE_RANGE) < dist))
        return;
    if (!ssBrain_InRoomOf(brain, player)) {
        if (ssAI_IsNextScene(player->lightBank, (u_int)ssBrain_Int(brain, BRAIN_ROOM)) == 0) {
            ssAI_ForceBrainEnd(chr, ssBrain_Short(brain, BRAIN_END_MOTION));
            return;
        }
        if (ssAI_SerchCheck(ssBrain_Float(brain, BRAIN_SIGHT) + AI_SIGHT_MARGIN, chr, player,
                            ssBrain_Int(brain, BRAIN_FOV) + AI_FOV_MARGIN, brain[BRAIN_BANK]) == 0) {
            ssBrain_EndIfFailed(chr, brain, ssAI_ApproachToTraceMode(chr));
            return;
        }
    }
    if (ssAI_SerchCheck(ssBrain_Float(brain, BRAIN_SIGHT) + AI_SIGHT_MARGIN, chr, player, AI_TURN_FULL,
                        player->lightBank) == 0)
        ssBrain_EndIfFailed(chr, brain, ssAI_ApproachToTraceMode(chr));
}

// Is Alyssa's body motion (slot 0's record) of kind AI_MOTION_KIND2_LOUD? PORT: the motion table is an EE
// word of her bank (asMothOperate.c's reading); no table, or no motion in the slot, reads as not loud where
// the ROM would read outside the table.
static int ssBrain_PlayerMotionLoud(const CHARACTER *player)
{
    const u_char *table;
    u_int ee;
    short slot;

    memcpy(&ee, (const u_char *)player + CHARA_MOTION_TABLE, sizeof(ee));
    memcpy(&slot, (const u_char *)player + CHARA_MOTION_SLOTS, sizeof(slot));
    table = (const u_char *)BankEePtr(player->bank, ee);
    if (table == NULL || slot < 0)
        return 0;
    return (s_char)table[slot * CHARA_MOTION_SIZE + MOTION_REC_KIND2] == AI_MOTION_KIND2_LOUD;
}

// CT3 0x0187be60
// Searching: a full panic or an appearance -> trace; seen (the trace's narrower look) -> approach; one
// frame in ten a strike from the search if the first state that allows one reaches her; heard (her loud
// motion or the panic, with TRIG_EVENT_AI_D2) -> trace, from farther only after the find motion.
void ssBrain_ChgSearch1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    CHARACTER *player = &Character[Player_BankID];
    const float *playerPos = (const float *)player->pos;
    u_char pick = BRAIN_ATK_NONE;
    float reach = 0.0f;
    short angle = 0;

    if (brain[BRAIN_BANK] == AI_BANK_NONE)
        return;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_STARTED, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        ssBrain_EndIfFailed(chr, brain, ssAI_SearchToTraceMode(chr, 0));
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_AI_APPEAR_NOW, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
        ssBrain_EndIfFailed(chr, brain, ssAI_SearchToTraceMode(chr, 0));
        ssBrain_SetInt(brain, BRAIN_TRACE_TIMER, 0);
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, TRIG_GLOBAL, TRIG_ANY_BANK) != 0)
        return;
    if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, TRIG_GLOBAL, TRIG_ANY_BANK) == 0 &&
        ssBrain_BankDrawn(brain[BRAIN_BANK]) && ssBrain_BankDrawn(player->lightBank)) {
        if (brain[BRAIN_SUBMODE] != BRAIN_SUBMODE_1 && brain[BRAIN_SUBMODE] != BRAIN_SUBMODE_SCN_LINK) {
            float sight = ssBrain_Float(brain, BRAIN_SIGHT) - AI_SIGHT_MARGIN;
            int fov = ssBrain_Int(brain, BRAIN_FOV) - AI_FOV_MARGIN;
            int seen;

            if (ssBrain_InRoomOf(brain, player))
                seen = ssAI_SerchCheck(sight, chr, player, fov, player->lightBank) != 0;
            else
                seen = ssAI_IsNextScene(player->lightBank, (u_int)ssBrain_Int(brain, BRAIN_ROOM)) != 0 &&
                       ssAI_SerchCheck(sight, chr, player, fov, brain[BRAIN_BANK]) != 0 &&
                       ssAI_SerchCheck(sight, chr, player, fov, player->lightBank) != 0;
            if (seen) {
                ssBrain_EndIfFailed(chr, brain, ssAI_SearchToApproachMode(ssBrain_DistTo(chr, playerPos), chr));
                return;
            }
        }
        if (ssTrigCheckRandom(AI_SEARCH_STRIKE_RATE) != 0) {
            float dir[4];
            float dist = ssBrain_DistTo(chr, playerPos);
            int count = brain[BRAIN_ATK_COUNT];
            int i;

            ssBrain_DirTo(dir, chr, playerPos);
            for (i = 0; i < count; i++) {
                BRAIN_ATK_STATE st;

                ssBrain_AtkState(&st, brain, i);
                if (st.fromSearch != 0) {
                    pick = (u_char)i;
                    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, TRIG_ANY_BANK) == 0) {
                        reach = st.reach;
                        angle = st.angle;
                    } else {
                        angle = st.anglePanic;
                        reach = st.reachPanic;
                    }
                    break;
                }
            }
            // As the ROM tests it: the strike goes when ssAI_IsTrgWithinAngle answers 0.
            if (dist < reach && ssAI_IsTrgWithinAngle(dir, chr, angle) == 0) {
                brain[BRAIN_ATK_PICK] = pick;
                ssBrain_EndIfFailed(chr, brain, ssAI_SearchToAttackMode(chr));
                return;
            }
        }
    }
    if (brain[BRAIN_SUBMODE] != BRAIN_SUBMODE_1 && brain[BRAIN_SUBMODE] != BRAIN_SUBMODE_SCN_LINK &&
        ssTrigCheckEvent(TRIG_EVENT_AI_D2, TRIG_GLOBAL, TRIG_ANY_BANK) != 0 &&
        (ssBrain_PlayerMotionLoud(player) || ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, TRIG_ANY_BANK) != 0)) {
        float dist = ssBrain_DistTo(chr, playerPos);
        float hear = ssBrain_Float(brain, BRAIN_HEAR_RANGE);

        if (dist < hear) {
            if (ssAI_CheckHearingRate(chr, AI_NOISE_LOUD) != 0)
                ssBrain_EndIfFailed(chr, brain, ssAI_SearchToTraceMode(chr, 0));
        } else if (dist < hear * AI_HEAR_FAR && ssAI_CheckHearingRate(chr, AI_NOISE_SOFT) != 0) {
            asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_FIND_MOTION));
            ssBrain_EndIfFailed(chr, brain, ssAI_SearchToTraceMode(chr, 1));
        }
    }
}

// ---- ssBrain_Damage1's pieces ----

// Every mode: the brain's group and room follow chr's. 1 when the group changed.
static int ssBrain_FollowGroup(CHARACTER *chr, u_char *brain)
{
    if (ssBrain_Short(brain, BRAIN_POLE_GROUP) == chr->h100)
        return 0;
    ssBrain_SetShort(brain, BRAIN_POLE_GROUP, chr->h100);
    return 1;
}

static int ssBrain_FollowRoom(CHARACTER *chr, u_char *brain)
{
    if ((u_long)(u_int)ssBrain_Int(brain, BRAIN_ROOM) == chr->roomHash)
        return 0;
    brain[BRAIN_BANK] = (u_char)chr->lightBank;
    ssBrain_SetInt(brain, BRAIN_ROOM, (int)(u_int)chr->roomHash);
    return 1;
}

// The target's place, with the shadow it may have warped through (Damage1's notes).
static void ssBrain_NoteTargetShadow(u_char *brain, const CHARACTER *target)
{
    ssBrain_NoteTarget(brain, target);
    brain[BRAIN_PLAYER_SHADOW] = (u_char)target->b102;
}

// Alyssa warped out of the noted room: the stalker is put at the pole she went through -- the scene link
// towards chr's room, or the pair of the shadow polygon she used -- in her old bank. -1: the brain ends.
static int ssBrain_FollowWarp(CHARACTER *chr, u_char *brain)
{
    int bank = brain[BRAIN_PLAYER_BANK];
    float pos[4];
    short pole;

    if ((s_char)brain[BRAIN_PLAYER_SHADOW] == -1) {
        pole = ssAI_GetScnLinkPoleID((const float *)chr->pos, (u_int)ssBrain_Int(brain, BRAIN_ROOM), bank);
        if (pole == -1)
            return -1;
    } else {
        pole = ssAI_GetShadowPolID((s_char)brain[BRAIN_PLAYER_SHADOW], bank);
        if (pole == -1)
            return -1;
        pole = ssAI_GetPairScnLinkPoleID(pole, bank);
        if (pole == -1)
            return -1;
    }
    ssAI_GetPolePos(pos, pole, bank);
    return ssAI_SetMonsterPos(chr, pos, ssAI_GetPoleGroupID(pole, bank), (u_char)bank,
                              (u_int)ssBrain_Int(brain, BRAIN_PLAYER_ROOM), 1) == -1 ? -1 : 0;
}

static int ssBrain_PlayerLeftNotedRoom(const u_char *brain, const CHARACTER *player)
{
    return (u_long)(u_int)ssBrain_Int(brain, BRAIN_PLAYER_ROOM) != player->roomHash;
}

static int ssBrain_PlayerInOwnRoom(const u_char *brain, const CHARACTER *player)
{
    return player->roomHash == (u_long)(u_int)ssBrain_Int(brain, BRAIN_ROOM);
}

// Each mode's follow-up of the brain's place and Alyssa's moves. -1: a re-plan failed (the brain ends).
static int ssBrain_DamageFollow(CHARACTER *chr, u_char *brain, CHARACTER *player)
{
    short g;

    switch (brain[BRAIN_MODE]) {
    case SS_AI_SEARCH:
        ssBrain_FollowGroup(chr, brain);
        ssBrain_FollowRoom(chr, brain);
        if (!ssBrain_PlayerLeftNotedRoom(brain, player))
            break;
        if (!ssBrain_PlayerInOwnRoom(brain, player)) {
            if (ssAI_IsWarpChara(player, chr) == 0) {
                // She walked on: search towards the scene link to her room.
                short pole = ssAI_GetScnLinkPoleID((const float *)chr->pos, (u_int)player->roomHash,
                                                   brain[BRAIN_BANK]);

                ssBrain_SetShort(brain, BRAIN_SEARCH_POLE, pole);
                if (pole == -1 || ssAI_SetSearchMode(chr, 1) == -1)
                    return -1;
            } else if (ssBrain_FollowWarp(chr, brain) == -1 || ssAI_SetSearchMode(chr, 0) == -1) {
                return -1;
            }
        }
        ssBrain_NoteTargetShadow(brain, player);
        break;
    case SS_AI_APPROACH:
        if (ssBrain_FollowGroup(chr, brain) &&
            (brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_ROUTE || brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_DIRECT) &&
            ssAI_SetApproachMode(chr, Player_BankID) == -1)
            return -1;
        ssBrain_FollowRoom(chr, brain);
        g = player->h100;
        if (ssBrain_Short(brain, BRAIN_PLAYER_GROUP) != g) {
            // She changed groups in chr's room: straight on in chr's group, the route extended by one group
            // into a neighbour, else re-routed.
            if (ssBrain_PlayerInOwnRoom(brain, player)) {
                int bank = brain[BRAIN_BANK];

                if (g == ssBrain_Short(brain, BRAIN_POLE_GROUP)) {
                    if (brain[BRAIN_SUBMODE] != BRAIN_SUBMODE_SCN_LINK) {
                        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_DIRECT;
                        ssAI_GRS_InitParam(bank);
                        ssBrain_SetShort(brain, BRAIN_TARG_POLE, -1);
                    }
                } else if (ssAI_IsNextGroup(ssBrain_Short(brain, BRAIN_PLAYER_GROUP), g, bank) == 0) {
                    ssBrain_SetShort(brain, BRAIN_TARG_POLE, -1);
                    if (brain[BRAIN_SUBMODE] != BRAIN_SUBMODE_SCN_LINK)
                        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
                } else {
                    ssAI_GRS_PushRear(player->h100, bank);
                    if (brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_DIRECT) {
                        short link;

                        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
                        link = ssAI_GetGLinkPolID(ssBrain_Short(brain, BRAIN_POLE_GROUP), ssAI_GRS_PopFront(bank),
                                                  bank);
                        ssBrain_SetShort(brain, BRAIN_TARG_POLE, link);
                        if (link == -1 ||
                            ssAI_SetNextGLinkPolePoint(link, bank, chr, (const float *)player->pos) == -1)
                            return -1;
                    }
                }
            }
            ssBrain_SetShort(brain, BRAIN_PLAYER_GROUP, player->h100);
        }
        if (ssBrain_PlayerLeftNotedRoom(brain, player)) {
            if (!ssBrain_PlayerInOwnRoom(brain, player) && ssAI_IsWarpChara(player, chr) != 0 &&
                ssBrain_FollowWarp(chr, brain) == -1)
                return -1;
            if (ssAI_SetApproachMode(chr, Player_BankID) == -1)
                return -1;
            brain[BRAIN_PLAYER_SHADOW] = (u_char)player->b102;
        }
        break;
    case SS_AI_TRACE:
        if (ssBrain_FollowGroup(chr, brain) &&
            (brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_ROUTE || brain[BRAIN_SUBMODE] == BRAIN_SUBMODE_DIRECT) &&
            ssAI_SetTraceMode(chr, NULL) == -1)
            return -1;
        if (ssBrain_FollowRoom(chr, brain) &&
            ssBrain_Int(brain, BRAIN_ROOM) != ssBrain_Int(brain, BRAIN_GOAL_ROOM) &&
            ssBrain_Short(brain, BRAIN_SCN_END_POLE) == -1 && ssAI_SetTraceMode(chr, NULL) == -1)
            return -1;
        if (ssBrain_PlayerInOwnRoom(brain, player)) {
            // In chr's room: a full panic or an appearance, or a loud enough noise, re-aims the trace at her.
            if ((ssTrigCheckEvent(TRIG_EVENT_PANIC_STARTED, TRIG_GLOBAL, TRIG_ANY_BANK) != 0 ||
                 ssTrigCheckEvent(TRIG_EVENT_AI_APPEAR_NOW, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) &&
                ssAI_SetTraceMode(chr, player) == -1)
                return -1;
            if (ssTrigCheckEvent(TRIG_EVENT_AI_D2, TRIG_GLOBAL, TRIG_ANY_BANK) != 0 &&
                (ssBrain_PlayerMotionLoud(player) ||
                 ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) &&
                ssBrain_DistTo(chr, (const float *)player->pos) < ssBrain_Float(brain, BRAIN_HEAR_RANGE) &&
                ssAI_CheckHearingRate(chr, AI_NOISE_SOFT) != 0 && ssAI_SetTraceMode(chr, player) == -1)
                return -1;
        }
        if (ssBrain_PlayerLeftNotedRoom(brain, player)) {
            if (ssBrain_PlayerInOwnRoom(brain, player)) {
                if (ssAI_SetTraceMode(chr, player) == -1)
                    return -1;
            } else if (ssAI_IsWarpChara(player, chr) != 0) {
                u_int goal;

                if (ssBrain_FollowWarp(chr, brain) == -1)
                    return -1;
                goal = (u_int)ssBrain_Int(brain, BRAIN_GOAL_ROOM);
                // The goal was her old room or is her new one: the trace keeps its goal.
                if ((u_int)ssBrain_Int(brain, BRAIN_PLAYER_ROOM) == goal || player->roomHash == (u_long)goal) {
                    if (ssAI_SetTraceMode(chr, NULL) == -1)
                        return -1;
                } else if (ssAI_SetTraceMode(chr, player) == -1) {
                    return -1;
                }
            }
            ssBrain_NoteTargetShadow(brain, player);
        }
        break;
    case SS_AI_ATTACK:
        ssBrain_FollowGroup(chr, brain);
        ssBrain_FollowRoom(chr, brain);
        if (ssBrain_PlayerLeftNotedRoom(brain, player)) {
            if (!ssBrain_PlayerInOwnRoom(brain, player) && ssAI_IsWarpChara(player, chr) != 0 &&
                ssBrain_FollowWarp(chr, brain) == -1)
                return -1;
            ssBrain_NoteTargetShadow(brain, player);
        }
        break;
    case SS_AI_RETURN:
    case SS_AI_HIDE_ATK:
        ssBrain_FollowGroup(chr, brain);
        ssBrain_FollowRoom(chr, brain);
        break;
    }
    return 0;
}

// Out of the drawn world a shown stalker is hidden.
static void ssBrain_HideIfUndrawn(CHARACTER *chr, u_char *brain)
{
    if (!ssBrain_BankDrawn(brain[BRAIN_BANK]) && brain[BRAIN_SHOWN] != 0) {
        ssAI_MonsterDraw(chr, 0);
        brain[BRAIN_SHOWN] = 0;
    }
}

// CT3 0x0187c4a0
// ssAI_DamageMode's entry: every frame before the mode, the brain follows chr's and Alyssa's moves (per
// mode), then the hits: a character's hit takes the frame (1) -- Alyssa's AI_ALYSSA_STRIKE plays the damage
// motion, then the mode's motion -- and an attack mode drops its attack. 1 also while the damage motion
// plays; the first frame after a strike registers TRIG_EVENT_PANIC_STARTED.
int ssBrain_Damage1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    CHARACTER *player = &Character[Player_BankID];
    const HIT_TABLE *hits;
    const HIT_REC *recs;
    short next = -1;
    int i;

    if (((const float *)chr->pos)[1] < AI_FELL_Y) {
        ssAI_BrainEnd(chr, ssBrain_Short(brain, BRAIN_END_MOTION));
        return 1;
    }
    if (ssBrain_DamageFollow(chr, brain, player) == -1) {
        ssAI_ForceBrainEnd(chr, ssBrain_Short(brain, BRAIN_END_MOTION));
        return 1;
    }
    if (ssAI_IsDamagePlaying(chr) != 0) {
        ssBrain_HideIfUndrawn(chr, brain);
        return 1;
    }
    hits = BANK_PTR(const HIT_TABLE *, chr->bank, All_table_TBL[chr->bank]->hits);
    recs = BANK_PTR(const HIT_REC *, chr->bank, hits->recs);
    for (i = 0; i < hits->count; i++) {
        const HIT_REC *rec = &recs[i];

        if (rec->isObject != 0)
            continue;
        switch (brain[BRAIN_MODE]) {
        case SS_AI_SEARCH:
            next = ssBrain_Short(brain, BRAIN_SEARCH_MOTION);
            break;
        case SS_AI_APPROACH:
            next = ssBrain_Short(brain, BRAIN_APPROACH_MOTION);
            break;
        case SS_AI_TRACE:
            next = ssBrain_Short(brain, BRAIN_TRACE_MOTION);
            break;
        case SS_AI_ATTACK:
            next = ssBrain_Short(brain, BRAIN_ATTACK_MOTION);
            brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
            brain[BRAIN_ATTACK_ON] = 0;
            ssTrigRemoveEvent(TRIG_EVENT_AI_ATTACK, TRIG_GLOBAL, TRIG_ANY_BANK);
            break;
        case SS_AI_RETURN:
        case SS_AI_HIDE_ATK:
            return 0;
        }
        if (rec->who == 0 && rec->attack == AI_ALYSSA_STRIKE) {
            asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_DAMAGE_MOTION));
            if (next != -1)
                asChangeMortionInfo_NextMortion(chr->id, ssBrain_Short(brain, BRAIN_DAMAGE_MOTION), next, -1);
            brain[BRAIN_STRUCK] = 1;
        }
        ssBrain_HideIfUndrawn(chr, brain);
        return 1;
    }
    if (brain[BRAIN_STRUCK] == 1) {
        brain[BRAIN_STRUCK] = 0;
        ssTrigRegisterEvent(TRIG_EVENT_PANIC_STARTED, TRIG_GLOBAL, TRIG_ANY_BANK);
    }
    return 0;
}

// CT3 0x0187d6e0
// Returning: hidden once its room is no longer drawn; the brain ends when the return motion is over.
void ssBrain_Return1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (!ssBrain_BankDrawn(brain[BRAIN_BANK]) && brain[BRAIN_SHOWN] != 0) {
        ssAI_MonsterDraw(chr, 0);
        brain[BRAIN_SHOWN] = 0;
    }
    if (ssAI_IsReturnPlaying(chr) == 0)
        ssAI_BrainEnd(chr, ssBrain_Short(brain, BRAIN_END_MOTION));
}

// ---- entering a mode ----

// CT3 0x01880e10
// SET_HIDE_ATK_POLE: a random attack pole of chr's light bank to go to (-1 without one). SET_HIDE_ATK_PICK:
// a HIDE_ATK record by weight -- record i is taken with weight[i] / (the weights left) -- except a record
// whose motion is the end motion while chr waits; none taken leaves BRAIN_HIDE_ATK_NONE.
int ssAI_SetHideAtkMode(CHARACTER *chr, int what)
{
    u_char *brain = CharaBrainPtr(chr);

    brain[BRAIN_MODE] = SS_AI_HIDE_ATK;
    if (what == SET_HIDE_ATK_POLE) {
        short pole;

        ssBrain_SetInt(brain, BRAIN_HIDE_ATK_TIMER, 0);
        pole = ssAI_GetNextTargPoleRandom(chr->lightBank, -1, HIDE_POLE_FLAG_ATTACK);
        ssBrain_SetShort(brain, BRAIN_LINK_POLE, pole);
        if (pole == -1)
            return -1;
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_HIDE_POLE;
    } else if (what == SET_HIDE_ATK_PICK) {
        if (ssBrain_PickAtk(chr, brain, offsetof(BRAIN_ATK_STATE, wNear), 1, 0))
            return 0;
    }
    brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
    return 0;
}

// The setters' straight path: no route, no group link to head for.
static void ssBrain_GoDirect(CHARACTER *chr, u_char *brain)
{
    ssAI_GRS_InitParam(chr->lightBank);
    ssBrain_SetShort(brain, BRAIN_TARG_POLE, -1);
}

// The setters' route: from chr's group to toGroup through the group links; BRAIN_TARG_POLE becomes the
// first link and the next point is set towards `to`. -1 when there is no route; *routeDist its length.
static int ssBrain_PlanRoute(CHARACTER *chr, u_char *brain, const float *to, short toGroup, float *routeDist)
{
    int bank = chr->lightBank;
    float d = ssAI_GrpRouteSearch((const float *)chr->pos, to, chr->h100, toGroup, bank);
    short link;

    if (d == AI_ROUTE_NONE)
        return -1;
    link = ssAI_GetGLinkPolID(chr->h100, ssAI_GRS_PopFront(bank), bank);
    ssBrain_SetShort(brain, BRAIN_TARG_POLE, link);
    if (link == -1)
        return -1;
    if (ssAI_SetNextGLinkPolePoint(link, bank, chr, to) == -1)
        return -1;
    *routeDist = d;
    return 0;
}

// A mode's timer from the distance to go: distance / (speed / AI_SPEED_DIV), and when it started.
static void ssBrain_SetTimer(u_char *brain, int timerOff, int speedOff, int startOff, float dist)
{
    ssBrain_SetInt(brain, timerOff, (int)(dist / (ssBrain_Float(brain, speedOff) / AI_SPEED_DIV)));
    ssBrain_SetInt(brain, startOff, ssBrain_Int(brain, BRAIN_TICKS));
}

// CT3 0x018817b0
// Trace: target (when given, and in chr's room or the next one) becomes the goal; in chr's room straight
// there or by route; else to the scene link towards the goal's room, with a timer for the way.
int ssAI_SetTraceMode(CHARACTER *chr, CHARACTER *target)
{
    u_char *brain = CharaBrainPtr(chr);
    const float *goal = (const float *)(brain + BRAIN_TRACE_POINT);
    float pole[4];
    float d;
    short group, end, endGroup;

    brain[BRAIN_MODE] = SS_AI_TRACE;
    if (target != NULL &&
        (target->roomHash == chr->roomHash || ssAI_IsNextScene(target->lightBank, (u_int)chr->roomHash) != 0)) {
        ssBrain_SetShort(brain, BRAIN_GOAL_GROUP, target->h100);
        brain[BRAIN_GOAL_BANK] = (u_char)target->lightBank;
        ssBrain_SetInt(brain, BRAIN_GOAL_ROOM, (int)(u_int)target->roomHash);
        memcpy(brain + BRAIN_TRACE_POINT, &target->pos, sizeof(float) * 4);
        ssBrain_SetInt(brain, BRAIN_TRACE_PLAN_TICKS, ssBrain_Int(brain, BRAIN_TICKS));
    }
    group = chr->h100;
    if (chr->roomHash == (u_long)(u_int)ssBrain_Int(brain, BRAIN_GOAL_ROOM)) {
        ssBrain_SetShort(brain, BRAIN_SCN_END_POLE, -1);
        if (group == -1 || ssBrain_Short(brain, BRAIN_GOAL_GROUP) == group) {
            ssBrain_GoDirect(chr, brain);
            brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_DIRECT;
        } else {
            if (ssBrain_PlanRoute(chr, brain, goal, ssBrain_Short(brain, BRAIN_GOAL_GROUP), &d) == -1)
                return -1;
            brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
        }
        return 0;
    }
    end = ssAI_GetScnLinkPoleID(goal, (u_int)ssBrain_Int(brain, BRAIN_GOAL_ROOM), chr->lightBank);
    ssBrain_SetShort(brain, BRAIN_SCN_END_POLE, end);
    if (end == -1)
        return -1;
    ssAI_GetPolePos(pole, end, chr->lightBank);
    endGroup = ssAI_GetPoleGroupID(end, chr->lightBank);
    if (group == -1 || endGroup == group) {
        d = ssBrain_DistTo(chr, pole);
        ssBrain_GoDirect(chr, brain);
        ssBrain_SetTimer(brain, BRAIN_TRACE_TIMER, BRAIN_TRACE_SPEED, BRAIN_TRACE_START, d);
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_DIRECT;
    } else {
        if (ssBrain_PlanRoute(chr, brain, pole, endGroup, &d) == -1)
            return -1;
        ssBrain_SetTimer(brain, BRAIN_TRACE_TIMER, BRAIN_TRACE_SPEED, BRAIN_TRACE_START, d);
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
    }
    return 0;
}

// CT3 0x01881280
// Attack Character[chara]: the hit flag raised, the attack event off, then the attack state by `how`.
int ssAI_SetAttackMode(CHARACTER *chr, int chara, int how)
{
    u_char *brain = CharaBrainPtr(chr);
    CHARACTER *target = &Character[(short)chara];
    int count, i;

    ssBrain_SetCharaInt(chr, CHARA_HIT_LANDED, 1);
    brain[BRAIN_MODE] = SS_AI_ATTACK;
    brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
    ssBrain_NoteTarget(brain, target);
    brain[BRAIN_ATTACK_ON] = 0;
    ssTrigRemoveEvent(TRIG_EVENT_AI_ATTACK, TRIG_GLOBAL, TRIG_ANY_BANK);
    count = brain[BRAIN_ATK_COUNT];
    if (how == SET_ATK_SEARCH) {
        size_t field;
        int checkWait = 0;

        if (ssAI_IsBendPlaying(target) != 0 || ssAI_IsDownPlaying(target) != 0) {
            field = offsetof(BRAIN_ATK_STATE, wDown);
        } else if (ssBrain_Float(brain, BRAIN_ATK_NEAR) < ssBrain_DistTo(chr, (const float *)target->pos)) {
            field = offsetof(BRAIN_ATK_STATE, wFar);
        } else if (ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, TRIG_ANY_BANK) != 0) {
            field = offsetof(BRAIN_ATK_STATE, wPanic);
        } else {
            // ROM: the grid of the target's bank is read before the tests; read here, where it is used.
            const HIT_SCENE_GRID *grid = ssBrain_Grid(brain[BRAIN_PLAYER_BANK]);

            if (AI_ATK_WIDE_ROOM < grid->cellW * grid->cellD) {
                field = offsetof(BRAIN_ATK_STATE, wWide);
            } else {
                field = offsetof(BRAIN_ATK_STATE, wNear);
                checkWait = 1;
            }
        }
        if (ssBrain_PickAtk(chr, brain, field, checkWait, 1))
            return 0;
    } else if (how == SET_ATK_QUEUED) {
        short queued = ssBrain_CharaShort(chr, CHARA_MOTION_QUEUE);
        u_short want = queued == -1 ? (u_short)ssBrain_Short(brain, BRAIN_START_MOTION) : (u_short)queued;

        ssBrain_SetInt(brain, BRAIN_HIDE_ATK_TIMER, 0);
        for (i = 0; i < count; i++) {
            BRAIN_ATK_STATE st;

            ssBrain_AtkState(&st, brain, i);
            if ((u_short)st.motion == want) {
                brain[BRAIN_ATK_PICK] = (u_char)i;
                brain[BRAIN_ATK_STEP] = 0;
                return 0;
            }
        }
    } else if (how == SET_ATK_FRESH) {
        ssBrain_SetInt(brain, BRAIN_HIDE_ATK_TIMER, 0);
        brain[BRAIN_ATK_STEP] = 0;
    }
    brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
    return 0;
}

// CT3 0x01881b10
// Approach Character[chara]. With no room yet the stalker comes out of its shadow polygon (hidden first);
// else as a trace: in the target's room straight there or by route, otherwise to the scene link, timed.
int ssAI_SetApproachMode(CHARACTER *chr, int chara)
{
    u_char *brain = CharaBrainPtr(chr);
    CHARACTER *target = &Character[(short)chara];
    const float *to = (const float *)target->pos;
    float pole[4];
    float d;
    short end, endGroup;

    brain[BRAIN_MODE] = SS_AI_APPROACH;
    if (ssBrain_Int(brain, BRAIN_ROOM) == 0) {
        if (brain[BRAIN_SHOWN] != 0) {
            ssAI_MonsterDraw(chr, 0);
            brain[BRAIN_SHOWN] = 0;
        }
        ssBrain_SetInt(brain, BRAIN_APPROACH_TIMER, 0);
        ssBrain_SetInt(brain, BRAIN_APPROACH_START, ssBrain_Int(brain, BRAIN_TICKS));
        ssBrain_SetShort(brain, BRAIN_PLAYER_GROUP, target->h100);
        ssBrain_NoteTarget(brain, target);
        end = ssAI_GetShadowPolID((s_char)brain[BRAIN_SHADOW_POL], chr->lightBank);
        ssBrain_SetShort(brain, BRAIN_SCN_END_POLE, end);
        if (end == -1)
            return -1;
        end = ssAI_GetPairScnLinkPoleID(end, chr->lightBank);
        ssBrain_SetShort(brain, BRAIN_SCN_END_POLE, end);
        if (end == -1)
            return -1;
        ssBrain_SetShort(brain, BRAIN_TARG_POLE, -1);
        if (ssAI_LetMonsterInFromShadow(chr, (u_char)chr->lightBank, brain[BRAIN_PLAYER_BANK],
                                        (s_char)brain[BRAIN_SHADOW_POL],
                                        ssBrain_Short(brain, BRAIN_APPROACH_MOTION)) == -1)
            return -1;
        return 0;
    }
    ssBrain_SetShort(brain, BRAIN_PLAYER_GROUP, target->h100);
    ssBrain_NoteTarget(brain, target);
    if (chr->roomHash == (u_long)(u_int)ssBrain_Int(brain, BRAIN_PLAYER_ROOM)) {
        ssBrain_SetShort(brain, BRAIN_SCN_END_POLE, -1);
        if (ssBrain_Short(brain, BRAIN_PLAYER_GROUP) == chr->h100) {
            ssBrain_GoDirect(chr, brain);
            brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_DIRECT;
        } else {
            if (ssBrain_PlanRoute(chr, brain, to, ssBrain_Short(brain, BRAIN_PLAYER_GROUP), &d) == -1)
                return -1;
            brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
        }
        return 0;
    }
    end = ssAI_GetScnLinkPoleID(to, (u_int)ssBrain_Int(brain, BRAIN_PLAYER_ROOM), chr->lightBank);
    ssBrain_SetShort(brain, BRAIN_SCN_END_POLE, end);
    if (end == -1)
        return -1;
    ssAI_GetPolePos(pole, end, chr->lightBank);
    endGroup = ssAI_GetPoleGroupID(end, chr->lightBank);
    if (endGroup == chr->h100) {
        d = ssBrain_DistTo(chr, pole);
        ssBrain_GoDirect(chr, brain);
        ssBrain_SetTimer(brain, BRAIN_APPROACH_TIMER, BRAIN_APPROACH_SPEED, BRAIN_APPROACH_START, d);
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_DIRECT;
    } else {
        if (ssBrain_PlanRoute(chr, brain, pole, endGroup, &d) == -1)
            return -1;
        ssBrain_SetTimer(brain, BRAIN_APPROACH_TIMER, BRAIN_APPROACH_SPEED, BRAIN_APPROACH_START, d);
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
    }
    return 0;
}

// CT3 0x01881f90
// Search. With no room yet the stalker comes out of its shadow polygon. Else a random search pole of chr's
// bank (or the one kept), straight there in its group or by route; a new search scales its times by the
// room's grid cells: base * (((cellW + cellD) - AI_SEARCH_ROOM_BASE) / AI_SEARCH_ROOM_SCALE + 1).
int ssAI_SetSearchMode(CHARACTER *chr, int keepPole)
{
    u_char *brain = CharaBrainPtr(chr);
    CHARACTER *player = &Character[Player_BankID];
    const HIT_SCENE_GRID *grid;
    float pole[4];
    float d, scale;
    short p;

    brain[BRAIN_MODE] = SS_AI_SEARCH;
    if (ssBrain_Int(brain, BRAIN_ROOM) == 0) {
        ssBrain_SetInt(brain, BRAIN_SEARCH_TIME, 0);
        ssBrain_SetInt(brain, BRAIN_SEARCH_START, ssBrain_Int(brain, BRAIN_TICKS));
        ssBrain_NoteTarget(brain, player);
        if (ssAI_LetMonsterInFromShadow(chr, (u_char)chr->lightBank, brain[BRAIN_PLAYER_BANK],
                                        (s_char)brain[BRAIN_SHADOW_POL],
                                        ssBrain_Short(brain, BRAIN_SEARCH_MOTION)) == -1)
            return -1;
        return 0;
    }
    if (keepPole == 0) {
        p = ssAI_GetNextTargPoleRandom(chr->lightBank, -1, SEARCH_POLE_FLAG);
        ssBrain_SetShort(brain, BRAIN_SEARCH_POLE, p);
        if (p == -1)
            return -1;
    }
    ssBrain_SetShort(brain, BRAIN_SEARCH_GROUP,
                     ssAI_GetPoleGroupID(ssBrain_Short(brain, BRAIN_SEARCH_POLE), chr->lightBank));
    brain[BRAIN_SEARCH_BANK] = (u_char)chr->lightBank;
    ssBrain_SetInt(brain, BRAIN_SEARCH_ROOM, (int)(u_int)chr->roomHash);
    ssBrain_NoteTarget(brain, player);
    if (ssBrain_Short(brain, BRAIN_SEARCH_GROUP) == chr->h100) {
        ssBrain_SetShort(brain, BRAIN_TARG_POLE, ssBrain_Short(brain, BRAIN_SEARCH_POLE));
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_DIRECT;
    } else {
        ssAI_GetPolePos(pole, ssBrain_Short(brain, BRAIN_SEARCH_POLE), chr->lightBank);
        if (ssBrain_PlanRoute(chr, brain, pole, ssBrain_Short(brain, BRAIN_SEARCH_GROUP), &d) == -1)
            return -1;
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
    }
    if (keepPole != 0)
        return 0;
    grid = ssBrain_Grid(brain[BRAIN_SEARCH_BANK]);
    scale = ((grid->cellW + grid->cellD) - AI_SEARCH_ROOM_BASE) / AI_SEARCH_ROOM_SCALE + 1.0f;
    ssBrain_SetInt(brain, BRAIN_SEARCH_TIME, (int)((float)ssBrain_Int(brain, BRAIN_SEARCH_TIME_BASE) * scale));
    ssBrain_SetInt(brain, BRAIN_SEARCH_TIME2, (int)((float)ssBrain_Int(brain, BRAIN_SEARCH_TIME2_BASE) * scale));
    ssBrain_SetInt(brain, BRAIN_SEARCH_START, ssBrain_Int(brain, BRAIN_TICKS));
    return 0;
}

// ---- the transitions ----

// CT3 0x01882290 (a tail jump in the ROM)
int ssAI_ApproachToHideAtkMode(CHARACTER *chr)
{
    return ssAI_AttackToHideAtkMode(chr);
}

// CT3 0x018822a0
// Into a hide attack: the attack event off, a hide pole picked, chr turned to Alyssa at once.
int ssAI_AttackToHideAtkMode(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dir[4];

    brain[BRAIN_ATTACK_ON] = 0;
    ssTrigRemoveEvent(TRIG_EVENT_AI_ATTACK, TRIG_GLOBAL, TRIG_ANY_BANK);
    if (ssAI_SetHideAtkMode(chr, SET_HIDE_ATK_POLE) == -1)
        return -1;
    ssBrain_DirTo(dir, chr, (const float *)Character[Player_BankID].pos);
    ssAI_CompCharaDir(chr, dir, AI_TURN_FULL, 1);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_HIDE_ATK_MOTION));
    asChangeMortionInfo_NextMortion(chr->id, ssBrain_Short(brain, BRAIN_HIDE_ATK_MOTION),
                                    ssBrain_Short(brain, BRAIN_ATTACK_MOTION), -1);
    asChangeCharacterMotionSettingPlayRate(chr->id, ssBrain_Short(brain, BRAIN_ATTACK_MOTION),
                                           brain[BRAIN_ATTACK_RATE]);
    return 0;
}

// CT3 0x018823b0
int ssAI_HideAtkToAttackMode(CHARACTER *chr)
{
    return ssAI_SetAttackMode(chr, Player_BankID, 0) != -1 ? 0 : -1;
}

// CT3 0x018823f0 (a tail jump in the ROM)
int ssAI_HideAtkToTraceMode(CHARACTER *chr)
{
    return ssAI_ApproachToTraceMode(chr);
}

// CT3 0x01882400
int ssAI_AttackToTraceMode(CHARACTER *chr)
{
    CharaBrainPtr(chr)[BRAIN_ATTACK_ON] = 0;
    ssTrigRemoveEvent(TRIG_EVENT_AI_ATTACK, TRIG_GLOBAL, TRIG_ANY_BANK);
    return ssAI_ApproachToTraceMode(chr);
}

// CT3 0x01882440
int ssAI_AttackToApproachMode(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    brain[BRAIN_ATTACK_ON] = 0;
    ssTrigRemoveEvent(TRIG_EVENT_AI_ATTACK, TRIG_GLOBAL, TRIG_ANY_BANK);
    if (ssAI_SetApproachMode(chr, Player_BankID) == -1)
        return -1;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_APPROACH_MOTION));
    return 0;
}

// CT3 0x018824c0
int ssAI_TraceToApproachMode(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (ssAI_SetApproachMode(chr, Player_BankID) == -1)
        return -1;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_APPROACH_MOTION));
    return 0;
}

// CT3 0x01882520
int ssAI_TraceToSearchMode(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (ssAI_SetSearchMode(chr, 0) == -1)
        return -1;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY,
                                    ssBrain_Short(brain, BRAIN_TRACE_TO_SEARCH_MOTION));
    asChangeMortionInfo_NextMortion(chr->id, ssBrain_Short(brain, BRAIN_TRACE_TO_SEARCH_MOTION),
                                    ssBrain_Short(brain, BRAIN_SEARCH_MOTION), -1);
    return 0;
}

// CT3 0x01882590
int ssAI_ApproachToAttackMode(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (ssAI_SetAttackMode(chr, Player_BankID, 0) == -1)
        return -1;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_ATTACK_MOTION));
    asChangeCharacterMotionSettingPlayRate(chr->id, ssBrain_Short(brain, BRAIN_ATTACK_MOTION),
                                           brain[BRAIN_ATTACK_RATE]);
    return 0;
}

// CT3 0x01882600
int ssAI_ApproachToTraceMode(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (ssAI_SetTraceMode(chr, &Character[Player_BankID]) == -1)
        return -1;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_TRACE_MOTION));
    asChangeCharacterMotionSettingPlayRate(chr->id, ssBrain_Short(brain, BRAIN_TRACE_MOTION),
                                           brain[BRAIN_TRACE_RATE]);
    return 0;
}

// CT3 0x01882690
int ssAI_SearchToReturnMode(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    brain[BRAIN_MODE] = SS_AI_RETURN;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_RETURN_MOTION));
    return 0;
}

// CT3 0x018826d0
// found: the find motion leads into the trace motion; otherwise the trace motion starts at once.
int ssAI_SearchToTraceMode(CHARACTER *chr, int found)
{
    u_char *brain = CharaBrainPtr(chr);

    if (ssAI_SetTraceMode(chr, &Character[Player_BankID]) == -1)
        return -1;
    if (found == 0)
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, ssBrain_Short(brain, BRAIN_TRACE_MOTION));
    else
        asChangeMortionInfo_NextMortion(chr->id, ssBrain_Short(brain, BRAIN_FIND_MOTION),
                                        ssBrain_Short(brain, BRAIN_TRACE_MOTION), -1);
    asChangeCharacterMotionSettingPlayRate(chr->id, ssBrain_Short(brain, BRAIN_TRACE_MOTION),
                                           brain[BRAIN_TRACE_RATE]);
    return 0;
}

// CT3 0x01882790
int ssAI_SearchToAttackMode(CHARACTER *chr)
{
    return ssAI_SetAttackMode(chr, Player_BankID, SET_ATK_SEARCH) != -1 ? 0 : -1;
}

// CT3 0x018827d0
// Seen from a search: near, the near start motion; farther, BRAIN_HIDE_ATK_MOTION; either leads into the
// approach motion.
int ssAI_SearchToApproachMode(float dist, CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    short start;

    if (ssAI_SetApproachMode(chr, Player_BankID) == -1)
        return -1;
    start = dist < ssBrain_Float(brain, BRAIN_APPROACH_NEAR) ? ssBrain_Short(brain, BRAIN_APPROACH_NEAR_MOTION)
                                                             : ssBrain_Short(brain, BRAIN_HIDE_ATK_MOTION);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, start);
    asChangeMortionInfo_NextMortion(chr->id, start, ssBrain_Short(brain, BRAIN_APPROACH_MOTION), -1);
    return 0;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x01880b60
int ssBrain_Init1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssBrain_Init1", 0x01880b60);
    return {};
}

// stub of CT3 0x0187f000
void ssBrain_Approach1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssBrain_Approach1", 0x0187f000);
}

// stub of CT3 0x0187d770
void ssBrain_Attack1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssBrain_Attack1", 0x0187d770);
}


// stub of CT3 0x0187d310
void ssBrain_HideAtk1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssBrain_HideAtk1", 0x0187d310);
}

// stub of CT3 0x0187fbe0
void ssBrain_Search1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssBrain_Search1", 0x0187fbe0);
}

// stub of CT3 0x0187e3d0
void ssBrain_Trace1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssBrain_Trace1", 0x0187e3d0);
}
