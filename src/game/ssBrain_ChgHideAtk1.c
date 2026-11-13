// game/ssBrain_ChgHideAtk1.c -- Clock Tower 3: the stalkers' brain (ssBrain_*1: one brain's mode functions;
// ssAI_Set*Mode / ssAI_<From>To<To>Mode: entering a mode). Symtab local chunk 82. CodeWarrior C.
// Slice 1 (2026-11-12): the mode changes ssBrain_Chg{HideAtk,Return,Attack,Trace,Approach}1, ssBrain_Return1,
// every transition and ssAI_SetHideAtkMode. Left: the mode bodies (Attack1, Trace1, Approach1, Search1,
// HideAtk1), ChgSearch1, Damage1, Init1 and the setters SetAttackMode / SetTraceMode / SetApproachMode /
// SetSearchMode (stubs below).

#include "common/alyssapan_stub.h"
#include "game/ssBrain_ChgHideAtk1.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ct3_types.h"
#include "game/ssGrpPortalCheck.h"
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
        if (ssBrain_Int(brain, BRAIN_GOAL_ROOM) != ssBrain_Int(brain, BRAIN_TRACE_ROOM))
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
    int count = brain[BRAIN_HIDE_ATK_COUNT];
    int left = AI_HIDE_ATK_WEIGHT;
    int i;

    brain[BRAIN_MODE] = SS_AI_HIDE_ATK;
    if (what == SET_HIDE_ATK_POLE) {
        short pole;

        ssBrain_SetInt(brain, BRAIN_HIDE_ATK_TIMER, 0);
        pole = ssAI_GetNextTargPoleRandom(chr->lightBank, -1, HIDE_POLE_FLAG_ATTACK);
        memcpy(brain + BRAIN_LINK_POLE, &pole, sizeof(pole));
        if (pole == -1)
            return -1;
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_HIDE_POLE;
    } else if (what == SET_HIDE_ATK_PICK) {
        for (i = 0; i < count; i++) {
            const u_char *rec = brain + BRAIN_HIDE_ATK + i * BRAIN_HIDE_ATK_SIZE;

            if (ssTrigCheckRandom((int)rec[0] * 100 / left) != 0 &&
                (ssAI_IsWaitPlaying(chr) == 0 || (u_short)rec[1] != (u_short)ssBrain_Short(brain, BRAIN_END_MOTION))) {
                brain[BRAIN_HIDE_ATK_PICK] = (u_char)i;
                return 0;
            }
            left -= rec[0];
            if (left < 1)
                break;
        }
    }
    brain[BRAIN_HIDE_ATK_PICK] = BRAIN_HIDE_ATK_NONE;
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
    return ssAI_SetAttackMode(chr, Player_BankID, 1) != -1 ? 0 : -1;
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

// stub of CT3 0x0187be60
void ssBrain_ChgSearch1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssBrain_ChgSearch1", 0x0187be60);
}

// stub of CT3 0x0187c4a0
int ssBrain_Damage1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("ssBrain_Damage1", 0x0187c4a0);
    return {};
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

// stub of CT3 0x018817b0
int ssAI_SetTraceMode(struct CHARACTER *chr, struct CHARACTER *target)
{
    ALYSSAPAN_STUB("ssAI_SetTraceMode", 0x018817b0);
    return {};
}

// stub of CT3 0x01881280
int ssAI_SetAttackMode(struct CHARACTER *chr, int chara, int fromSearch)
{
    ALYSSAPAN_STUB("ssAI_SetAttackMode", 0x01881280);
    return {};
}

// stub of CT3 0x01881b10
int ssAI_SetApproachMode(struct CHARACTER *chr, int chara)
{
    ALYSSAPAN_STUB("ssAI_SetApproachMode", 0x01881b10);
    return {};
}

// stub of CT3 0x01881f90
int ssAI_SetSearchMode(struct CHARACTER *chr, int a1)
{
    ALYSSAPAN_STUB("ssAI_SetSearchMode", 0x01881f90);
    return {};
}
