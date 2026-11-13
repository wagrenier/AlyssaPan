// game/tiBrain_ChgTrace_Ghost.c -- Clock Tower 3: the ghosts' brain (tiBrain_*_Ghost<n>: the ghost kinds'
// mode functions; tiBrain_Init3, tiBrain_Damage<n>, tiBrain_ChgReturn<n>). Symtab local chunk 81. CodeWarrior C.
// Slice 1 (2026-11-13): the small ones -- the mode changes ChgTrace_Ghost, ChgReturn1 / 2, ChgAttack_Ghost3,
// the bodies Trace_Ghost1, Return_Ghost1, Search_Ghost2, the initialiser Init3 and the helpers tiGetNearPole
// and tiCheckAlysStatusForGhost. A ghost fades in and out through CHARACTER.translucentAlpha.
// Slice 2 (2026-11-13): the Chg{Attack,Approach,Search}_Ghost<n> mode changes, Damage1..3 and Init2.
// Slice 3 (2026-11-13): the wandering bodies Search_Ghost1 / 3, Approach_Ghost1, and the attacks of the ghosts
// that wait at their point, Attack_Ghost3 / 2, and the wanderer's, Attack_Ghost1 (sharing Attack_Ghost4's hold);
// tiBrain_Init1. The TU is complete.

#include "common/alyssapan_stub.h"
#include "game/tiBrain_ChgTrace_Ghost.h"
#include "game/BRANCH_CLEAR.h"
#include "game/LinkMatrixTree.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/sound.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/ssBrain_ChgHideAtk1.h"
#include "game/ssGrpPortalCheck.h"
#include "game/tiAvoidObject.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaSinCos.h"

#include <math.h>
#include <string.h>

#define TRIG_GLOBAL 1

// The brain is host memory (CharaBrainPtr); its words are read unaligned-safe (as ssBrain_ChgHideAtk1.c).
static short tiBrain_Short(const u_char *brain, int off)
{
    short v;
    memcpy(&v, brain + off, sizeof(v));
    return v;
}

static float tiBrain_Float(const u_char *brain, int off)
{
    float v;
    memcpy(&v, brain + off, sizeof(v));
    return v;
}

static void tiBrain_SetShort(u_char *brain, int off, short v)
{
    memcpy(brain + off, &v, sizeof(v));
}

static void tiBrain_SetInt(u_char *brain, int off, int v)
{
    memcpy(brain + off, &v, sizeof(v));
}

static void tiBrain_SetFloat(u_char *brain, int off, float v)
{
    memcpy(brain + off, &v, sizeof(v));
}

// PORT: the bank whose relocated EE range holds `ee` (ahRelocateSunPs2ObjectArry.c's BankOfEeAddr): a
// ghost pole word is BankEeAddr of the script's bank (tiXcmd_SetGhostPole), which CHARA_JUNC does not keep.
static const float *tiGhostEePos(u_int ee)
{
    int bank, best = -1;

    if (ee == 0)
        return NULL;
    for (bank = 0; bank < BANK_MAX; bank++) {
        if (BankImage[bank] != NULL && BankInfo[bank].state != BANK_STATE_FREE && BankImageEE[bank] <= ee &&
            (best < 0 || BankImageEE[bank] > BankImageEE[best]))
            best = bank;
    }
    return best < 0 ? NULL : (const float *)BankEePtr(best, ee);
}

// vsub, vmul, vaddbc x2, vrsqrt, then vdiv 1 / Q: |a - b| over xyz (ssBrain_ChgHideAtk1.c's ssBrain_DistTo).
static float tiDist(const float *a, const float *b)
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    float len2 = dx * dx + dy * dy + dz * dz;
    return len2 > 0.0f ? sqrtf(len2) : 0.0f;
}

// CT3 0x0187afd0
// The ghost pole (junc->poles, tiXcmd_SetGhostPole) nearest to pos: its index; 0 on a tie with pole 0 and
// with one pole. PORT: a pole whose word resolves into no bank counts as infinitely far.
int tiGetNearPole(const float *pos, const CHARA_JUNC *junc)
{
    const float *p = tiGhostEePos(junc->poles[0]);
    float best = p ? tiDist(pos, p) : HUGE_VALF;
    int near = 0;
    int i;

    for (i = 1; i < junc->poleCount; i++) {
        float d;

        p = tiGhostEePos(junc->poles[i]);
        d = p ? tiDist(pos, p) : HUGE_VALF;
        if (d < best) {
            best = d;
            near = i;
        }
    }
    return near;
}

// CT3 0x0187b0e0
// 1 when the ghost leaves Alyssa alone: TRIG_EVENT_PANIC_OFF_63 is on, she has no hp left, or
// TRIG_EVENT_PANIC_ON_4D is on (the ghost kinds' attack changes test it).
int tiCheckAlysStatusForGhost(void)
{
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_OFF_63, TRIG_GLOBAL, 0) == 0 && Character[Player_BankID].hp > 0)
        return ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_4D, TRIG_GLOBAL, 0) != 0;
    return 1;
}

// CT3 0x01875b10: the ghosts' trace has no mode change.
void tiBrain_ChgTrace_Ghost(CHARACTER *chr)
{
    (void)chr;
}

// CT3 0x01875b20
// Out of the stalkers' room: shown again, into an attack (submode 0) with motion GHOST_MOTION_RETURN2_ATTACK.
void tiBrain_ChgReturn2(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (CheckSameRoomAsMonster(chr) == 0) {
        TrgCharacterDraw(1, chr->id);
        brain[BRAIN_MODE] = SS_AI_ATTACK;
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_ROUTE;
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_RETURN2_ATTACK);
    }
}

// CT3 0x01875b80
// Out of the stalkers' room and lit by Alyssa's light bank: shown again, into a search (submode 1) with
// motion GHOST_MOTION_RETURN1_SEARCH.
void tiBrain_ChgReturn1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (CheckSameRoomAsMonster(chr) == 0 && chr->lightBank == Character[Player_BankID].lightBank) {
        TrgCharacterDraw(1, chr->id);
        brain[BRAIN_SUBMODE] = BRAIN_SUBMODE_1;
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_RETURN1_SEARCH);
        brain[BRAIN_MODE] = SS_AI_SEARCH;
    }
}

// CT3 0x01875f00: an attack turns to a trace once BRAIN_GHOST_TRACE_REQ is set.
void tiBrain_ChgAttack_Ghost3(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (tiBrain_Short(brain, BRAIN_GHOST_TRACE_REQ) != 0)
        brain[BRAIN_MODE] = SS_AI_TRACE;
}

// CT3 0x01877050
// The trace fades the ghost out: Alyssa is held (LOCKED_SET_1) while the alpha runs down a step a frame;
// at 0 the ghost is hidden and she is let go (LOCKED_SET_2).
void tiBrain_Trace_Ghost1(CHARACTER *chr)
{
    float *alpha = &chr->translucentAlpha;

    if (0.0f < *alpha) {
        float a;

        asAlyssaLockedByAnything(LOCKED_SET_1);
        a = *alpha - GHOST_FADE_STEP;
        *alpha = a;
        if (a <= 0.0f) {
            *alpha = 0.0f;
            TrgCharacterDraw(0, chr->id);
            asAlyssaLockedByAnything(LOCKED_SET_2);
        }
    }
}

// CT3 0x01877950
// The return drops the ghost's pad rattle (BRAIN_GHOST_CLATTER, freed when it is no longer running) and
// fades the ghost out, hiding it at 0.
void tiBrain_Return_Ghost1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float *alpha = &chr->translucentAlpha;
    int slot;

    memcpy(&slot, brain + BRAIN_GHOST_CLATTER, sizeof(slot));
    if (slot != -1) {
        if (ControllerClatteringGet(slot) == 0)
            ControllerClatteringFree(slot);
        tiBrain_SetInt(brain, BRAIN_GHOST_CLATTER, -1);
    }
    if (0.0f < *alpha) {
        float a = *alpha - GHOST_FADE_STEP;

        *alpha = a;
        if (a <= 0.0f) {
            *alpha = 0.0f;
            TrgCharacterDraw(0, chr->id);
        }
    }
}

// CT3 0x0187a760: the search fades the ghost in by BRAIN_GHOST_ALPHA_STEP a frame, up to BRAIN_GHOST_ALPHA_MAX.
void tiBrain_Search_Ghost2(CHARACTER *chr)
{
    const u_char *brain = CharaBrainPtr(chr);
    float max = tiBrain_Float(brain, BRAIN_GHOST_ALPHA_MAX);

    if (chr->translucentAlpha < max) {
        float a = chr->translucentAlpha + tiBrain_Float(brain, BRAIN_GHOST_ALPHA_STEP);

        chr->translucentAlpha = a;
        if (max < a)
            chr->translucentAlpha = tiBrain_Float(brain, BRAIN_GHOST_ALPHA_MAX);
    }
}

// CT3 0x0187adb0
// Brain kind 2's initialiser (ssAI_BrainInit slot): the trace point is where the ghost stands, its alpha now
// is the fade's ceiling, the search time base GHOST_SEARCH_TIME_BASE; the first motion starts and the ghost
// sound bank Z_REI loads. Always 0.
int tiBrain_Init3(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    tiBrain_SetShort(brain, BRAIN_GHOST_TRACE_REQ, 0);
    tiBrain_SetShort(brain, BRAIN_SEARCH_POLE, 0);
    tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
    memcpy(brain + BRAIN_TRACE_POINT, chr->pos, 16);
    tiBrain_SetFloat(brain, BRAIN_GHOST_ALPHA_MAX, chr->translucentAlpha);
    tiBrain_SetInt(brain, BRAIN_SEARCH_TIME, 0);
    tiBrain_SetInt(brain, BRAIN_SEARCH_TIME_BASE, GHOST_SEARCH_TIME_BASE);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBrain_Short(brain, BRAIN_START_MOTION));
    SSLoadSE(GHOST_SE_BANK, 0);
    brain[BRAIN_TRACE_RATE] = GHOST_TRACE_RATE;
    tiBrain_SetInt(brain, BRAIN_GHOST_W220, 5);
    return 0;
}

// ---- slice 2 (2026-11-13): the Chg* mode changes, Damage1..3, Init2 ----

// The motion chr plays (CHARA_MOTION_SLOTS[0]).
static short tiGhostMotion(const CHARACTER *chr)
{
    short slot;

    memcpy(&slot, (const u_char *)chr + CHARA_MOTION_SLOTS, sizeof(slot));
    return slot;
}

// The one-shot motions no mode change interrupts (6..8, 10, 12..15; GHOST_MOTION_DAMAGE too, except for
// ChgAttack_Ghost4).
static int tiGhostBusy(const CHARACTER *chr, int withDamage)
{
    switch (tiGhostMotion(chr)) {
    case 6:
    case 7:
    case 8:
    case GHOST_MOTION_VANISH:
    case 0xc:
    case 0xd:
    case GHOST_MOTION_APPEAR:
    case GHOST_MOTION_WITHDRAW:
        return 1;
    case GHOST_MOTION_DAMAGE:
        return withDamage;
    }
    return 0;
}

static float tiGhostToPlayer(const CHARACTER *chr)
{
    return tiDist((const float *)Character[Player_BankID].pos, (const float *)chr->pos);
}

// Farther from the ghost area's point (CHARA_JUNC.ghostPoint, when there is one) than its range.
static int tiGhostOutOfArea(const CHARACTER *chr)
{
    const float *p = tiGhostEePos(chr->junc->ghostPoint);

    return p != NULL && chr->junc->ghostRange < tiDist(p, (const float *)chr->pos);
}

// Plays GHOST_MOTION_VANISH, then the float: the start of a return.
static void tiGhostVanish(const CHARACTER *chr)
{
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_VANISH);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_VANISH, GHOST_MOTION_FLOAT, -1);
}

static void tiGhostSearchMotion(const CHARACTER *chr, short slot)
{
    if (slot != GHOST_MOTION_SEARCH)
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_SEARCH);
}

static void tiGhostSetNearPole(u_char *brain, const CHARACTER *chr)
{
    tiBrain_SetShort(brain, BRAIN_SEARCH_POLE, (short)tiGetNearPole((const float *)chr->pos, chr->junc));
}

// The search starts over (submode 0, BRAIN_SEARCH_START from the word at BRAIN_APPROACH_TIMER: open gh5).
static void tiGhostRestartSearch(u_char *brain)
{
    brain[BRAIN_SUBMODE] = 0;
    memcpy(brain + BRAIN_SEARCH_START, brain + BRAIN_APPROACH_TIMER, 4);
}

// Gives the approach up: a search, resting (GHOST_REST_GAVE_UP from 0 ticks).
static void tiGhostGiveUp(u_char *brain)
{
    tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
    brain[BRAIN_GHOST_REST] = GHOST_REST_GAVE_UP;
    brain[BRAIN_MODE] = SS_AI_SEARCH;
}

// Adds this frame's ticks to a rest counter; 1 when it passes its limit.
static int tiGhostRestOver(u_char *brain, int ticksOff, int limitOff)
{
    int ticks, limit;

    memcpy(&ticks, brain + ticksOff, sizeof(ticks));
    ticks += gNextTicksInterval;
    tiBrain_SetInt(brain, ticksOff, ticks);
    memcpy(&limit, brain + limitOff, sizeof(limit));
    return limit < ticks;
}

// The ghost's loop SE, louder the nearer Alyssa is (0 from GHOST_SE_NEAR); its handle is kept to stop it.
static void tiGhostPlayLoop(u_char *brain, float dist)
{
    int handle = SSPlaySE(GHOST_SE_BANK, GHOST_SE_LOOP, 1);
    float v = GHOST_SE_NEAR - dist;

    if (v < 0.0f)
        v = 0.0f;
    SdSetParamSE(handle, SD_SE_PARAM_VOLUME, (int)((v / GHOST_SE_NEAR) * (float)GHOST_SE_VOLUME) & 0xff);
    tiBrain_SetInt(brain, BRAIN_GHOST_SE, handle);
}

static void tiGhostDropClatter(u_char *brain)
{
    int slot;

    memcpy(&slot, brain + BRAIN_GHOST_CLATTER, sizeof(slot));
    if (slot != -1) {
        if (ControllerClatteringGet(slot) == 0)
            ControllerClatteringFree(slot);
        tiBrain_SetInt(brain, BRAIN_GHOST_CLATTER, -1);
    }
}

// The damage handlers' head: under GHOST_FLAG_PANIC_RATE the search motion's rate climbs to GHOST_RATE_MAX
// while TRIG_EVENT_PANIC_3 is on and falls to GHOST_RATE_MIN while it is off (u_char arithmetic, as the ROM).
static void tiGhostPanicRate(const CHARACTER *chr, u_char *brain)
{
    u_char *rate = &brain[BRAIN_TRACE_RATE];
    s_char step = (s_char)brain[BRAIN_GHOST_W220];

    if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_PANIC_RATE) == 0)
        return;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, 0) == 0) {
        if (GHOST_RATE_MIN < *rate) {
            *rate = (u_char)(*rate - step);
            if (*rate < GHOST_RATE_MIN)
                *rate = GHOST_RATE_MIN;
            asChangeCharacterMotionPlayRate(chr->id, GHOST_MOTION_SEARCH, *rate);
        }
    } else if (*rate < GHOST_RATE_MAX) {
        *rate = (u_char)(*rate + step);
        if (GHOST_RATE_MAX < *rate)
            *rate = GHOST_RATE_MAX;
        asChangeCharacterMotionPlayRate(chr->id, GHOST_MOTION_SEARCH, *rate);
    }
}

// Under GHOST_FLAG_FADE_FAR, farther than GHOST_FADE_FAR from Alyssa the ghost fades a step a frame.
static void tiGhostFadeFar(CHARACTER *chr, const u_char *brain)
{
    if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_FADE_FAR) != 0 && GHOST_FADE_FAR < tiGhostToPlayer(chr) &&
        0.0f < chr->translucentAlpha) {
        chr->translucentAlpha -= GHOST_FADE_STEP;
        if (chr->translucentAlpha <= 0.0f)
            chr->translucentAlpha = 0.0f;
    }
}

// What touched chr's character this frame (HIT_TABLE.count of All_table_TBL[bank]->hits).
static int tiGhostHitCount(const CHARACTER *chr)
{
    const HIT_TABLE *hits = BANK_PTR(const HIT_TABLE *, chr->bank, All_table_TBL[chr->bank]->hits);

    return hits != NULL ? hits->count : 0;
}

// A hit outside a return: the rattle is dropped and the damage motion starts (then `next`). 0 when nothing
// hit or the ghost is returning, 1 when the damage starts.
static int tiGhostStartDamage(const CHARACTER *chr, u_char *brain, short next)
{
    if (tiGhostHitCount(chr) == 0 || brain[BRAIN_MODE] == SS_AI_RETURN)
        return 0;
    tiGhostDropClatter(brain);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_DAMAGE);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_DAMAGE, next, -1);
    return 1;
}

// The loop SE stops and the ghost cries (one of two, 2:1); the mode is an attack.
static void tiGhostCry(u_char *brain)
{
    int handle;

    memcpy(&handle, brain + BRAIN_GHOST_SE, sizeof(handle));
    SdStopSE(handle);
    SSPlaySE(GHOST_SE_BANK, tnRand(3) < 2 ? GHOST_SE_CRY_A : GHOST_SE_CRY_B, 1);
    brain[BRAIN_MODE] = SS_AI_ATTACK;
}

// The ghost whose attack event (TRIG_EVENT_GHOST_<id>_ON) chr is; 0 for another character.
static int tiGhostEvent(const CHARACTER *chr)
{
    if (chr->id == CHARA_ID_GHOST_1E)
        return TRIG_EVENT_GHOST_1E_ON;
    if (chr->id == CHARA_ID_GHOST_1D)
        return TRIG_EVENT_GHOST_1D_ON;
    return 0;
}

// CT3 0x01875c20
// An attack (kind 4): a trace request -> trace; out of Alyssa's room or light bank -> vanish into a return
// toward the nearest pole; else, outside the one-shot motions, beyond BRAIN_ATTACK_LEAVE, without
// TRIG_EVENT_GHOST_1E_ON, resting, Alyssa spared or out of its area -> a search toward the nearest pole.
void tiBrain_ChgAttack_Ghost4(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const CHARACTER *player = &Character[Player_BankID];

    if (tiBrain_Short(brain, BRAIN_GHOST_TRACE_REQ) != 0) {
        brain[BRAIN_MODE] = SS_AI_TRACE;
        return;
    }
    if (CheckSameRoomAsMonster(chr) != 0 || player->lightBank != chr->lightBank) {
        tiGhostVanish(chr);
        tiGhostSetNearPole(brain, chr);
        brain[BRAIN_MODE] = SS_AI_RETURN;
        return;
    }
    if (tiGhostBusy(chr, 0))
        return;
    if (tiBrain_Float(brain, BRAIN_ATTACK_LEAVE) < tiGhostToPlayer(chr) ||
        ssTrigCheckEvent(TRIG_EVENT_GHOST_1E_ON, TRIG_GLOBAL, 0) == 0 || brain[BRAIN_GHOST_REST] == GHOST_REST_GAVE_UP ||
        tiCheckAlysStatusForGhost() != 0 || tiGhostOutOfArea(chr)) {
        tiGhostSearchMotion(chr, tiGhostMotion(chr));
        tiGhostSetNearPole(brain, chr);
        brain[BRAIN_MODE] = SS_AI_SEARCH;
    }
}

// CT3 0x01875f60
// An attack (kind 2): a trace request -> trace; outside the one-shot motions, the ghost whose event is off
// withdraws into a search and its loop SE plays at Alyssa's distance.
void tiBrain_ChgAttack_Ghost2(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dist;
    int event;

    if (tiBrain_Short(brain, BRAIN_GHOST_TRACE_REQ) != 0) {
        brain[BRAIN_MODE] = SS_AI_TRACE;
        return;
    }
    dist = tiGhostToPlayer(chr);
    event = tiGhostEvent(chr);
    if (tiGhostBusy(chr, 1) || event == 0 || ssTrigCheckEvent(event, TRIG_GLOBAL, 0) != 0)
        return;
    brain[BRAIN_MODE] = SS_AI_SEARCH;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_WITHDRAW);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_WITHDRAW, GHOST_MOTION_FLOAT, -1);
    tiGhostPlayLoop(brain, dist);
}

// CT3 0x01876190
// An attack (kind 1): a trace request -> trace; out of Alyssa's room, out of her light bank -> vanish into
// a return (both tests run, as the ROM); outside the one-shot motions, beyond BRAIN_ATTACK_LEAVE ->
// approach; with TRIG_EVENT_GHOST_1D_ON, Alyssa spared, without TRIG_EVENT_GHOST_1E_ON or resting -> a
// fresh search.
void tiBrain_ChgAttack_Ghost1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const CHARACTER *player = &Character[Player_BankID];
    short slot;

    if (tiBrain_Short(brain, BRAIN_GHOST_TRACE_REQ) != 0) {
        brain[BRAIN_MODE] = SS_AI_TRACE;
        return;
    }
    if (CheckSameRoomAsMonster(chr) != 0) {
        tiGhostVanish(chr);
        brain[BRAIN_MODE] = SS_AI_RETURN;
    }
    if (player->lightBank != chr->lightBank) {
        tiGhostVanish(chr);
        brain[BRAIN_MODE] = SS_AI_RETURN;
    }
    slot = tiGhostMotion(chr);
    if (tiGhostBusy(chr, 1))
        return;
    if (tiBrain_Float(brain, BRAIN_ATTACK_LEAVE) < tiGhostToPlayer(chr)) {
        tiGhostSearchMotion(chr, slot);
        brain[BRAIN_MODE] = SS_AI_APPROACH;
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_GHOST_1D_ON, TRIG_GLOBAL, 0) != 0 || tiCheckAlysStatusForGhost() != 0 ||
        ssTrigCheckEvent(TRIG_EVENT_GHOST_1E_ON, TRIG_GLOBAL, 0) == 0 || brain[BRAIN_GHOST_REST] != GHOST_REST_NONE) {
        brain[BRAIN_MODE] = SS_AI_SEARCH;
        tiGhostRestartSearch(brain);
        tiGhostSearchMotion(chr, slot);
    }
}

// CT3 0x01876400
// An approach (kind 2): in the stalkers' room -> vanish into a return toward the nearest pole; out of her
// light bank, without TRIG_EVENT_GHOST_1E_ON, beyond BRAIN_LOSE_RANGE, Alyssa spared or out of its area ->
// give up toward the nearest pole; within BRAIN_ATTACK_RANGE -> attack.
void tiBrain_ChgApproach_Ghost2(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const CHARACTER *player = &Character[Player_BankID];
    float dist;

    if (CheckSameRoomAsMonster(chr) != 0) {
        tiGhostVanish(chr);
        tiGhostSetNearPole(brain, chr);
        brain[BRAIN_MODE] = SS_AI_RETURN;
        return;
    }
    dist = tiGhostToPlayer(chr);
    if (player->lightBank != chr->lightBank || ssTrigCheckEvent(TRIG_EVENT_GHOST_1E_ON, TRIG_GLOBAL, 0) == 0 ||
        tiBrain_Float(brain, BRAIN_LOSE_RANGE) < dist || tiCheckAlysStatusForGhost() != 0 || tiGhostOutOfArea(chr)) {
        tiGhostSetNearPole(brain, chr);
        tiGhostSearchMotion(chr, tiGhostMotion(chr));
        tiGhostGiveUp(brain);
        return;
    }
    if (dist < tiBrain_Float(brain, BRAIN_ATTACK_RANGE)) {
        brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
        brain[BRAIN_MODE] = SS_AI_ATTACK;
    }
}

// CT3 0x01876670
// An approach (kind 1): out of Alyssa's room or light bank -> vanish into a return; without
// TRIG_EVENT_GHOST_1E_ON, Alyssa spared or with TRIG_EVENT_GHOST_1D_ON -> give up into a fresh search;
// within BRAIN_ATTACK_RANGE -> attack; beyond BRAIN_LOSE_RANGE -> give up.
void tiBrain_ChgApproach_Ghost1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const CHARACTER *player = &Character[Player_BankID];
    short slot = tiGhostMotion(chr);
    float dist;

    if (CheckSameRoomAsMonster(chr) != 0 || player->lightBank != chr->lightBank) {
        tiGhostVanish(chr);
        brain[BRAIN_MODE] = SS_AI_RETURN;
        return;
    }
    dist = tiGhostToPlayer(chr);
    if (ssTrigCheckEvent(TRIG_EVENT_GHOST_1E_ON, TRIG_GLOBAL, 0) != 0 && tiCheckAlysStatusForGhost() == 0 &&
        ssTrigCheckEvent(TRIG_EVENT_GHOST_1D_ON, TRIG_GLOBAL, 0) == 0) {
        if (dist < tiBrain_Float(brain, BRAIN_ATTACK_RANGE)) {
            brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
            tiGhostSearchMotion(chr, slot);
            brain[BRAIN_MODE] = SS_AI_ATTACK;
            return;
        }
        if (dist <= tiBrain_Float(brain, BRAIN_LOSE_RANGE))
            return;
    }
    tiGhostSearchMotion(chr, slot);
    tiGhostRestartSearch(brain);
    tiGhostGiveUp(brain);
}

// CT3 0x018768f0
// A search (kind 3): out of Alyssa's room or light bank -> vanish into a return; resting until its ticks
// pass BRAIN_GHOST_REST_LIMIT; then, Alyssa not spared, inside its area, within BRAIN_SIGHT and with
// TRIG_EVENT_GHOST_1E_ON -> approach.
void tiBrain_ChgSearch_Ghost3(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const CHARACTER *player = &Character[Player_BankID];
    float dist = tiGhostToPlayer(chr);

    if (CheckSameRoomAsMonster(chr) != 0 || player->lightBank != chr->lightBank) {
        tiGhostVanish(chr);
        brain[BRAIN_MODE] = SS_AI_RETURN;
        return;
    }
    if (brain[BRAIN_GHOST_REST] == GHOST_REST_GAVE_UP) {
        if (!tiGhostRestOver(brain, BRAIN_GHOST_REST_TICKS, BRAIN_GHOST_REST_LIMIT))
            return;
        brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
    }
    if (tiCheckAlysStatusForGhost() != 0 || tiGhostOutOfArea(chr))
        return;
    if (dist < tiBrain_Float(brain, BRAIN_SIGHT) && ssTrigCheckEvent(TRIG_EVENT_GHOST_1E_ON, TRIG_GLOBAL, 0) != 0)
        brain[BRAIN_MODE] = SS_AI_APPROACH;
}

// CT3 0x01876b50
// A search (kind 2): unless the return motion plays or Alyssa is spared, the ghost whose event is on
// appears into an attack (GHOST_MOTION_APPEAR); ghost 1D also cries GHOST_SE_APPEAR and comes out at its
// trace point (x and z) at the fade's ceiling. Its loop SE then plays at her distance.
void tiBrain_ChgSearch_Ghost2(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dist = tiGhostToPlayer(chr);
    int event;

    if (ssAI_IsReturnPlaying(chr) != 0 || tiCheckAlysStatusForGhost() != 0)
        return;
    event = tiGhostEvent(chr);
    if (event == 0 || ssTrigCheckEvent(event, TRIG_GLOBAL, 0) == 0)
        return;
    brain[BRAIN_MODE] = SS_AI_ATTACK;
    brain[BRAIN_SUBMODE] = 0;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_APPEAR);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_APPEAR, GHOST_MOTION_FLOAT, -1);
    if (chr->id == CHARA_ID_GHOST_1D) {
        float *pos = (float *)chr->pos;

        SSPlaySE(GHOST_SE_BANK, GHOST_SE_APPEAR, 1);
        pos[0] = tiBrain_Float(brain, BRAIN_TRACE_POINT + 0);
        pos[2] = tiBrain_Float(brain, BRAIN_TRACE_POINT + 8);
        chr->translucentAlpha = tiBrain_Float(brain, BRAIN_GHOST_ALPHA_MAX);
    }
    tiGhostPlayLoop(brain, dist);
}

// CT3 0x01876d90
// A search (kind 1): out of Alyssa's room or light bank -> vanish into a return; the rests count down;
// with TRIG_EVENT_GHOST_1D_ON a search in submode 1 / 2 plays GHOST_MOTION_FIND and starts over; without
// it, rested, Alyssa not spared, within BRAIN_SIGHT and with TRIG_EVENT_GHOST_1E_ON -> approach.
void tiBrain_ChgSearch_Ghost1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const CHARACTER *player = &Character[Player_BankID];
    float dist = tiGhostToPlayer(chr);

    if (CheckSameRoomAsMonster(chr) != 0 || player->lightBank != chr->lightBank) {
        tiGhostVanish(chr);
        brain[BRAIN_MODE] = SS_AI_RETURN;
        return;
    }
    if (brain[BRAIN_GHOST_REST] == GHOST_REST_2) {
        if (tiGhostRestOver(brain, BRAIN_GHOST_REST2_TICKS, BRAIN_GHOST_REST2_LIMIT))
            brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
    } else if (brain[BRAIN_GHOST_REST] == GHOST_REST_GAVE_UP) {
        if (tiGhostRestOver(brain, BRAIN_GHOST_REST_TICKS, BRAIN_GHOST_REST_LIMIT))
            brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_GHOST_1D_ON, TRIG_GLOBAL, 0) == 0) {
        if (brain[BRAIN_GHOST_REST] == GHOST_REST_NONE && tiCheckAlysStatusForGhost() == 0 &&
            dist < tiBrain_Float(brain, BRAIN_SIGHT) && ssTrigCheckEvent(TRIG_EVENT_GHOST_1E_ON, TRIG_GLOBAL, 0) != 0) {
            asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_SEARCH);
            brain[BRAIN_MODE] = SS_AI_APPROACH;
        }
    } else if (brain[BRAIN_SUBMODE] == 1 || brain[BRAIN_SUBMODE] == 2) {
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_FIND);
        asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_FIND, GHOST_MOTION_FLOAT, -1);
        tiGhostRestartSearch(brain);
    }
}

// CT3 0x018770e0
// Damage (kind 3): the panic rate, then a hit outside a return starts the damage motion (then the float).
// 1 while the damage motion plays or when it starts.
int tiBrain_Damage3(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    tiGhostPanicRate(chr, brain);
    if (ssAI_IsDamagePlaying(chr) != 0)
        return 1;
    return tiGhostStartDamage(chr, brain, GHOST_MOTION_FLOAT);
}

// CT3 0x01877280
// Damage (kind 2): the panic rate and the far fade; while the damage motion plays BRAIN_GHOST_HIT_FADE is
// 1.0 and, in a submode, the ghost fades by it; a hit outside a return stops the loop SE, cries and starts
// the damage motion into an attack (submode 1 when an attack-family motion still plays).
int tiBrain_Damage2(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    tiGhostPanicRate(chr, brain);
    tiGhostFadeFar(chr, brain);
    if (ssAI_IsDamagePlaying(chr) != 0) {
        tiBrain_SetFloat(brain, BRAIN_GHOST_HIT_FADE, 1.0f);
        if (brain[BRAIN_SUBMODE] != 0) {
            float a = chr->translucentAlpha - tiBrain_Float(brain, BRAIN_GHOST_HIT_FADE);

            chr->translucentAlpha = a < 0.0f ? 0.0f : a;
        }
        return 1;
    }
    if (tiGhostHitCount(chr) == 0 || brain[BRAIN_MODE] == SS_AI_RETURN)
        return 0;
    tiGhostDropClatter(brain);
    tiGhostCry(brain);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_DAMAGE);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_DAMAGE, GHOST_MOTION_FLOAT, -1);
    if (ssAI_IsAttackPlaying(chr) != 0 || ssAI_IsPreAttackPlaying(chr) != 0 || ssAI_IsDamagePlaying(chr) != 0 ||
        ssAI_IsActionPlaying(chr) != 0 || ssAI_IsPostAttackPlaying(chr) != 0)
        brain[BRAIN_SUBMODE] = 1;
    return 1;
}

// CT3 0x01877610
// Damage (kind 1): the panic rate; while its light bank is not drawn the ghost is only hidden (when a
// shadow / refraction flag is on) and the frame is taken; else the far fade, and a hit outside a return
// starts the damage motion (then the search motion), stops the loop SE and cries into an attack.
int tiBrain_Damage1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    tiGhostPanicRate(chr, brain);
    // PORT: lightBank -1 (none) would index BankInfo[-1] in the ROM; it counts as not drawn here.
    if (chr->lightBank < 0 || BankInfo[chr->lightBank].drawOn == 0) {
        if ((chr->flags & (CHARACTER_FLAG_REFRACTION | CHARACTER_FLAG_SHADOW)) != 0)
            TrgCharacterDraw(0, chr->id);
        return 1;
    }
    tiGhostFadeFar(chr, brain);
    if (ssAI_IsDamagePlaying(chr) != 0)
        return 1;
    if (tiGhostStartDamage(chr, brain, GHOST_MOTION_SEARCH) == 0)
        return 0;
    tiGhostCry(brain);
    return 1;
}

// CT3 0x0187ae40
// Brain kind 1's initialiser: Init3's without the search times and rest ticks -- the trace point is where
// the ghost stands, its alpha the fade's ceiling, no rest; the first motion starts, Z_REI loads.
int tiBrain_Init2(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    tiBrain_SetShort(brain, BRAIN_GHOST_TRACE_REQ, 0);
    tiBrain_SetFloat(brain, BRAIN_GHOST_ALPHA_MAX, chr->translucentAlpha);
    memcpy(brain + BRAIN_TRACE_POINT, chr->pos, 16);
    brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBrain_Short(brain, BRAIN_START_MOTION));
    SSLoadSE(GHOST_SE_BANK, 0);
    brain[BRAIN_TRACE_RATE] = GHOST_TRACE_RATE;
    tiBrain_SetInt(brain, BRAIN_GHOST_W220, 5);
    return 0;
}

// The symtab's statics (sdata 0x01938664 / 0x01938668): a hovering ghost's rise and fall a frame at most.
static float tiMoveU = 15.0f;
static float tiMoveD = 3.0f;

// The hover follows the floor: a segment from GHOST_FLOOR_ABOVE over to GHOST_FLOOR_BELOW under the ghost.
#define GHOST_FLOOR_ABOVE 200.0f
#define GHOST_FLOOR_BELOW 500.0f
#define GHOST_SE_PERIOD 350             /* BRAIN_SEARCH_TIME_BASE after a loop SE: 350 or 700 ticks (tnRand(2)) */

// The unit vector from chr to Alyssa (vsub, then vrsqrt's 1 / |v| over all four lanes: w is 0 for two
// points). PORT: 0 for a zero vector (the ROM divides by 0).
static void tiGhostDirToPlayer(const CHARACTER *chr, float dir[4])
{
    const float *p = (const float *)Character[Player_BankID].pos;
    const float *c = (const float *)chr->pos;
    float len2, inv;
    int i;

    for (i = 0; i < 4; i++)
        dir[i] = p[i] - c[i];
    len2 = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
    inv = len2 > 0.0f ? 1.0f / sqrtf(len2) : 0.0f;
    for (i = 0; i < 4; i++)
        dir[i] *= inv;
}

// Moves the ghost's height by d, at most tiMoveU up and tiMoveD down a frame.
static void tiGhostRise(float *pos, float d)
{
    float step = absf(d);

    if (d < 0.0f)
        pos[1] -= step <= tiMoveD ? step : tiMoveD;
    else
        pos[1] += step <= tiMoveU ? step : tiMoveU;
}

// The hover follows the floor of its light bank: the hit of a segment straight down through the ghost.
static void tiGhostFollowFloor(CHARACTER *chr)
{
    float *pos = (float *)chr->pos;
    LOS_HIT los;

    memcpy(los.seg.from, pos, sizeof(los.seg.from));
    memcpy(los.seg.to, pos, sizeof(los.seg.to));
    los.seg.from[1] = pos[1] + GHOST_FLOOR_ABOVE;
    los.seg.to[1] = pos[1] - GHOST_FLOOR_BELOW;
    if (CheckLOS(chr->lightBank, &los) != 0)
        tiGhostRise(pos, los.point[1] - pos[1]);
}

// The fade-in a step a frame up to BRAIN_GHOST_ALPHA_MAX (Approach_Ghost2, Attack_Ghost4).
static void tiGhostFadeIn(CHARACTER *chr, const u_char *brain)
{
    float max = tiBrain_Float(brain, BRAIN_GHOST_ALPHA_MAX);

    if (chr->translucentAlpha < max) {
        chr->translucentAlpha += GHOST_FADE_STEP;
        if (max < chr->translucentAlpha)
            chr->translucentAlpha = max;
    }
}

// The grab ends: the rest starts and the ghost vanishes (then the search motion).
static void tiGhostLetGo(const CHARACTER *chr, u_char *brain, int bitten)
{
    (void)bitten;
    brain[BRAIN_GHOST_REST] = GHOST_REST_GAVE_UP;
    tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_VANISH);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_VANISH, GHOST_MOTION_SEARCH, -1);
}

// How a ghost lets Alyssa go (Attack_Ghost4: tiGhostLetGo; Attack_Ghost3 / 2 their own).
typedef void (*GHOST_LET_GO)(const CHARACTER *chr, u_char *brain, int bitten); /* bitten: after the bite */

// Attack_Ghost4 (and 3, 2) while the attack motion plays: the ghost holds Alyssa (locked, TRIG_EVENT_GHOST_GRAB on,
// on her position, panic rising). She breaks free by the rattle (ControllerClatteringGet 1), or it lets go
// after BRAIN_GHOST_GRAB_LIMIT ticks or when she is spared; at the first frame, with TRIG_EVENT_PANIC_3,
// it bites: unless TRIG_EVENT_NO_FATAL_HIT, a barrier charm repels it (TRIG_EVENT_PANIC_OFF_63, the menu
// stopped, a trace request), else it lets go and locks her in its own mode. letGo is the kind's let-go.
static void tiGhostHold(CHARACTER *chr, u_char *brain, GHOST_LET_GO letGo)
{
    const CHARACTER *player = &Character[Player_BankID];
    int clatter, ticks, limit;
    u_short panic;

    asAlyssaLockedByAnything(LOCKED_SET_1);
    ssTrigRegisterEvent(TRIG_EVENT_GHOST_GRAB, TRIG_GLOBAL, 0);
    memcpy(&clatter, brain + BRAIN_GHOST_CLATTER, sizeof(clatter));
    memcpy(&ticks, brain + BRAIN_GHOST_GRAB_TICKS, sizeof(ticks));
    memcpy(&limit, brain + BRAIN_GHOST_GRAB_LIMIT, sizeof(limit));
    if (ControllerClatteringGet(clatter) == 1) {
        tiBrain_SetInt(brain, BRAIN_GHOST_CLATTER, -1);
        ssTrigRemoveEvent(TRIG_EVENT_GHOST_GRAB, TRIG_GLOBAL, 0);
        letGo(chr, brain, 0);
    } else if (limit < ticks || tiCheckAlysStatusForGhost() != 0) {
        ControllerClatteringFree(clatter);
        tiBrain_SetInt(brain, BRAIN_GHOST_CLATTER, -1);
        ssTrigRemoveEvent(TRIG_EVENT_GHOST_GRAB, TRIG_GLOBAL, 0);
        letGo(chr, brain, 0);
    } else if (ticks == 0 && ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, 0) != 0) {
        memcpy(&panic, brain + BRAIN_GHOST_GRAB_PANIC, sizeof(panic));
        PanicSet((float)panic, PANIC_RISE_ATTACK + 1);
        tiGhostDropClatter(brain);
        ssTrigRemoveEvent(TRIG_EVENT_GHOST_GRAB, TRIG_GLOBAL, 0);
        if (ssTrigCheckEvent(TRIG_EVENT_NO_FATAL_HIT, TRIG_GLOBAL, 0) != 0) {
            letGo(chr, brain, 1);
        } else if (Kekkai_Check() == 0) {
            // PORT: another character leaves the ROM's mode register unset; LOCKED_QUERY changes nothing.
            int mode = chr->id == CHARA_ID_GHOST_1F   ? GHOST_LOCKED_1F
                       : chr->id == CHARA_ID_GHOST_1E ? GHOST_LOCKED_1E
                       : chr->id == CHARA_ID_GHOST_1D ? GHOST_LOCKED_1D
                                                      : LOCKED_QUERY;

            letGo(chr, brain, 1);
            asAlyssaLockedByAnything(mode);
        } else {
            ssTrigRegisterEvent(TRIG_EVENT_PANIC_OFF_63, TRIG_GLOBAL, 0);
            TrgCharStopFlag |= TRG_CHAR_STOP_MENU;
            tiBrain_SetShort(brain, BRAIN_GHOST_TRACE_REQ, 1);
        }
    }
    memcpy(&panic, brain + BRAIN_GHOST_GRAB_PANIC, sizeof(panic));
    PanicSet((float)panic, PANIC_RISE_ATTACK);
    memcpy(chr->pos, player->pos, 16);
    tiBrain_SetInt(brain, BRAIN_GHOST_GRAB_TICKS, ticks + gNextTicksInterval);
}

// CT3 0x01877a00
// The attack (kind 4), a lunge and a grab: fading in; nothing while resting. The ghost follows the floor
// (outside an action motion) or Alyssa's height (GHOST_FLAG_HEIGHT). In its one-shot motions: the miss
// (GHOST_MOTION_LUNGE2) rests; the pre-attack turns to her and grabs within GHOST_GRAB_REACH and
// GHOST_GRAB_HEIGHT (GHOST_SE_GRAB, a rattle to break free); the action (the lunge) turns and flies at her
// by BRAIN_GHOST_LUNGE_SPEED, landing on her within BRAIN_GHOST_HOLD_RANGE (TRIG_EVENT_GHOST_GRAB); the
// attack holds her (tiGhostHold). Otherwise, near enough and facing her, it lunges by chance, and turns.
void tiBrain_Attack_Ghost4(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const CHARACTER *player = &Character[Player_BankID];
    float *pos = (float *)chr->pos;
    float dir[4];
    float dist;
    short slot;
    int speed;

    tiGhostFadeIn(chr, brain);
    if (brain[BRAIN_GHOST_REST] == GHOST_REST_GAVE_UP)
        return;
    slot = tiGhostMotion(chr);
    if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_HEIGHT) == 0) {
        if (ssAI_IsActionPlaying(chr) == 0)
            tiGhostFollowFloor(chr);
    } else {
        tiGhostRise(pos, ((const float *)player->pos)[1] - pos[1]);
    }
    tiGhostDirToPlayer(chr, dir);
    dist = tiGhostToPlayer(chr);
    memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
    switch (slot) {
    case GHOST_MOTION_LUNGE:
    case GHOST_MOTION_GRAB:
    case GHOST_MOTION_LUNGE2:
    case GHOST_MOTION_GRAB_START:
    case 0xd:
    case GHOST_MOTION_APPEAR:
    case GHOST_MOTION_WITHDRAW:
        break;
    default: {
        int chance;

        if (tiCheckAlysStatusForGhost() == 0 && dist < tiBrain_Float(brain, BRAIN_GHOST_GRAB_RANGE) &&
            ssAI_IsTrgWithinAngle(dir, chr, tiBrain_Short(brain, BRAIN_GHOST_GRAB_ANGLE)) != 0) {
            if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_PANIC_RATE) != 0 &&
                ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, 0) != 0)
                chance = GHOST_LUNGE_CHANCE_MAX;
            else
                chance = brain[BRAIN_GHOST_LUNGE_CHANCE];
            if (tnRand(GHOST_LUNGE_CHANCE_MAX) < chance) {
                tiBrain_SetInt(brain, BRAIN_GHOST_SE, SSPlaySE(GHOST_SE_BANK, GHOST_SE_LUNGE, 1));
                asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_LUNGE);
                asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_LUNGE, GHOST_MOTION_LUNGE2, -1);
                asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_LUNGE2, GHOST_MOTION_SEARCH, -1);
            }
        }
        ssAI_CompCharaDir(chr, dir, speed, 0);
        return;
    }
    }
    if (slot == GHOST_MOTION_LUNGE2) {
        brain[BRAIN_GHOST_REST] = GHOST_REST_GAVE_UP;
        tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
        return;
    }
    if (ssAI_IsPreAttackPlaying(chr) != 0) {
        CLATTERING_SET set = {-1, -1, -1, -1};

        ssAI_CompCharaDir(chr, dir, speed, 1);
        if (tiCheckAlysStatusForGhost() != 0 || GHOST_GRAB_REACH <= dist ||
            GHOST_GRAB_HEIGHT <= absf(pos[1] - ((const float *)player->pos)[1]))
            return;
        tiBrain_SetInt(brain, BRAIN_GHOST_SE, SSPlaySE(GHOST_SE_BANK, GHOST_SE_GRAB, 1));
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_GRAB_START);
        asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_GRAB_START, GHOST_MOTION_GRAB, -1);
        tiBrain_SetInt(brain, BRAIN_GHOST_CLATTER, ControllerClatteringSet(&set));
        tiBrain_SetInt(brain, BRAIN_GHOST_GRAB_TICKS, 0);
        return;
    }
    if (ssAI_IsActionPlaying(chr) != 0) {
        ssAI_CompCharaDir(chr, dir, speed, 1);
        if (dist <= tiBrain_Float(brain, BRAIN_GHOST_HOLD_RANGE)) {
            // The ROM first steps by dir * dist, then overwrites the position with hers.
            memcpy(chr->pos, player->pos, 16);
            ssTrigRegisterEvent(TRIG_EVENT_GHOST_GRAB, TRIG_GLOBAL, 0);
        } else {
            float step = tiBrain_Float(brain, BRAIN_GHOST_LUNGE_SPEED);
            int i;

            for (i = 0; i < 4; i++)
                pos[i] += dir[i] * step;
        }
        return;
    }
    if (ssAI_IsAttackPlaying(chr) != 0)
        tiGhostHold(chr, brain, tiGhostLetGo);
}

// CT3 0x01879a80
// The approach (kind 2): the ghost fades in a step a frame up to BRAIN_GHOST_ALPHA_MAX; outside its appear /
// withdraw motions it hovers after the floor (unless GHOST_FLAG_HEIGHT: up by tiMoveU, down by tiMoveD at
// most), turns to Alyssa at BRAIN_TURN_SPEED, and every BRAIN_SEARCH_TIME_BASE ticks plays its loop SE at
// her distance, then waits 350 or 700 ticks.
void tiBrain_Approach_Ghost2(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dir[4];
    int time, base;
    short slot;

    tiGhostFadeIn(chr, brain);
    slot = tiGhostMotion(chr);
    if (slot == GHOST_MOTION_WITHDRAW || slot == GHOST_MOTION_APPEAR)
        return;
    if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_HEIGHT) == 0)
        tiGhostFollowFloor(chr);
    tiGhostDirToPlayer(chr, dir);
    {
        int speed;

        memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
        ssAI_CompCharaDir(chr, dir, speed, 0);
    }
    memcpy(&time, brain + BRAIN_SEARCH_TIME, sizeof(time));
    time += gNextTicksInterval;
    tiBrain_SetInt(brain, BRAIN_SEARCH_TIME, time);
    memcpy(&base, brain + BRAIN_SEARCH_TIME_BASE, sizeof(base));
    if (base < time) {
        tiGhostPlayLoop(brain, tiGhostToPlayer(chr));
        tiBrain_SetInt(brain, BRAIN_SEARCH_TIME, 0);
        tiBrain_SetInt(brain, BRAIN_SEARCH_TIME_BASE, (tnRand(2) + 1) * GHOST_SE_PERIOD);
    }
}

// ---- slice 3 (2026-11-13): the wandering bodies -- Search_Ghost3, Search_Ghost1, Approach_Ghost1 ----

// The unit vector from `from` to `to` (vsub, vrsqrt over xyz, all four lanes scaled); answers the distance
// (xyz). PORT: a zero vector stays 0 (the ROM divides by 0).
static float tiGhostUnit(const float *to, const float *from, float dir[4])
{
    float len2, len;
    int i;

    for (i = 0; i < 4; i++)
        dir[i] = to[i] - from[i];
    len2 = dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2];
    len = len2 > 0.0f ? sqrtf(len2) : 0.0f;
    for (i = 0; i < 4; i++)
        dir[i] = len > 0.0f ? dir[i] / len : 0.0f;
    return len;
}

// The hover's height (Search_Ghost1 / 3, Approach_Ghost1): after the floor, or with GHOST_FLAG_HEIGHT
// towards `height` when the caller has one (Search_Ghost3: its pole's), else nothing.
static void tiGhostHover(CHARACTER *chr, const u_char *brain, const float *height)
{
    if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_HEIGHT) == 0)
        tiGhostFollowFloor(chr);
    else if (height != NULL)
        tiGhostRise((float *)chr->pos, *height - ((const float *)chr->pos)[1]);
}

// Every BRAIN_SEARCH_TIME_BASE ticks the loop SE at Alyssa's distance, then 350 or 700 ticks of quiet
// (the search and approach bodies' tail).
static void tiGhostLoopTimer(const CHARACTER *chr, u_char *brain)
{
    int time, base;

    memcpy(&time, brain + BRAIN_SEARCH_TIME, sizeof(time));
    time += gNextTicksInterval;
    tiBrain_SetInt(brain, BRAIN_SEARCH_TIME, time);
    memcpy(&base, brain + BRAIN_SEARCH_TIME_BASE, sizeof(base));
    if (base < time) {
        tiGhostPlayLoop(brain, tiGhostToPlayer(chr));
        tiBrain_SetInt(brain, BRAIN_SEARCH_TIME, 0);
        tiBrain_SetInt(brain, BRAIN_SEARCH_TIME_BASE, (tnRand(2) + 1) * GHOST_SE_PERIOD);
    }
}

// The one-shot motions the wandering bodies wait out (tiGhostBusy's, with the damage).
static int tiGhostWanderBusy(const CHARACTER *chr)
{
    return tiGhostBusy(chr, 1);
}

// A leg lasts GHOST_LEG_UNIT * (2..4) ticks.
static void tiGhostNewLegTime(u_char *brain)
{
    tiBrain_SetInt(brain, BRAIN_GHOST_LEG_TICKS, 0);
    tiBrain_SetInt(brain, BRAIN_GHOST_LEG_LIMIT, (tnRand(3) + 2) * GHOST_LEG_UNIT);
}

// Approach_Ghost1 sets off from standing still: GHOST_MOTION_SET_OFF, then the search motion.
static void tiGhostSetOff(const CHARACTER *chr, const u_char *brain)
{
    if (brain[BRAIN_SUBMODE] == GHOST_SEARCH_STILL) {
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_SET_OFF);
        asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_SET_OFF, GHOST_MOTION_SEARCH, -1);
    }
}

// A new wander leg: a random heading (gaSetRotMatrixY of 0..359 degrees applied to GHOST_WANDER_AHEAD),
// unless it turns back from `cur` (the way to the old point), then `cur`; the point is
// BRAIN_GHOST_WANDER_STEP along it (xyz; w is the heading's, as vmulx.xyz / vadd.xyz leave it). Answers the
// heading in `cur`; the leg's time starts over.
static const float GHOST_ORIGIN[4] = {0.0f, 0.0f, 0.0f, 0.0f};
static const float GHOST_WANDER_AHEAD[4] = {0.0f, 0.0f, 1.0f, 0.0f}; /* rodata 0x01938690 / 0x019386a0 */
// A random heading: GHOST_WANDER_AHEAD turned by gaSetRotMatrixY through 0..359 degrees (vmula / vmadd).
static void tiGhostRandomHeading(float head[4])
{
    float m[4][4];
    int i;

    gaSetRotMatrixY(m, tnRand(360) * GHOST_DEGREE);
    for (i = 0; i < 4; i++)
        head[i] = m[0][i] * GHOST_WANDER_AHEAD[0] + m[1][i] * GHOST_WANDER_AHEAD[1] +
                  m[2][i] * GHOST_WANDER_AHEAD[2] + m[3][i] * GHOST_WANDER_AHEAD[3];
}

static void tiGhostNewLeg(const CHARACTER *chr, u_char *brain, float cur[4])
{
    float head[4], point[4];
    float step = tiBrain_Float(brain, BRAIN_GHOST_WANDER_STEP);
    int i;

    tiGhostRandomHeading(head);
    if (gaInnerProductXYZ(cur, head) < 0.0f)
        memcpy(head, cur, sizeof(head));
    for (i = 0; i < 3; i++)
        point[i] = head[i] * step + ((const float *)chr->pos)[i];
    point[3] = head[3];
    memcpy(brain + BRAIN_NEXT_POINT, point, sizeof(point));
    memcpy(cur, head, sizeof(head));
    tiGhostNewLegTime(brain);
}

// CT3 0x0187a2b0
// The search (kind 3), round the ghost poles: fading in; it hovers after the floor (or with
// GHOST_FLAG_HEIGHT to its pole's height); outside its appear / withdraw motions it heads for its pole
// (BRAIN_SEARCH_POLE) and, nearer than GHOST_POLE_REACH, takes the next (GHOST_FLAG_POLE_SKIP: a random
// 0..count-3 on), turns to it, and plays its loop SE on the timer. PORT: a pole word that resolves into
// no bank leaves the ghost where it is.
void tiBrain_Search_Ghost3(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const CHARA_JUNC *junc = chr->junc;
    const float *pole;
    float dir[4];
    short slot, at;
    int speed;

    pole = tiGhostEePos(junc->poles[tiBrain_Short(brain, BRAIN_SEARCH_POLE)]);
    tiGhostFadeIn(chr, brain);
    tiGhostHover(chr, brain, pole != NULL ? &pole[1] : NULL);
    slot = tiGhostMotion(chr);
    if (slot == GHOST_MOTION_WITHDRAW || slot == GHOST_MOTION_APPEAR || pole == NULL)
        return;
    if (tiGhostUnit(pole, (const float *)chr->pos, dir) < GHOST_POLE_REACH) {
        at = tiBrain_Short(brain, BRAIN_SEARCH_POLE);
        if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_POLE_SKIP) == 0)
            at = (short)((at + 1) % junc->poleCount);
        else
            at = (short)((at + tnRand(junc->poleCount - 2)) % junc->poleCount);
        tiBrain_SetShort(brain, BRAIN_SEARCH_POLE, at);
        pole = tiGhostEePos(junc->poles[at]);
        if (pole != NULL)
            tiGhostUnit(pole, (const float *)chr->pos, dir);
    }
    memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
    ssAI_CompCharaDir(chr, dir, speed, 0);
    tiGhostLoopTimer(chr, brain);
}

// CT3 0x0187a7b0
// The search (kind 1), a wander: fading in; outside its one-shot motions it hovers after the floor and,
// each leg's end (BRAIN_GHOST_LEG_TICKS past BRAIN_GHOST_LEG_LIMIT) or within GHOST_WANDER_REACH of its
// point, draws of 16: under 12 a new leg (GHOST_SEARCH_WANDER), 12..15 Alyssa's position
// (GHOST_SEARCH_TO_ALYSSA), both with the search motion. While TRIG_EVENT_GHOST_1D_ON is on (for either
// ghost, as the ROM) it heads for BRAIN_TRACE_POINT instead (GHOST_SEARCH_TO_TRACE). It turns to its point
// and plays its loop SE on the timer.
void tiBrain_Search_Ghost1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dir[4];
    float point[4];
    short slot = tiGhostMotion(chr);
    int ticks, limit, draw;

    tiGhostFadeIn(chr, brain);
    if (tiGhostWanderBusy(chr))
        return;
    memcpy(&ticks, brain + BRAIN_GHOST_LEG_TICKS, sizeof(ticks));
    ticks += gNextTicksInterval;
    tiBrain_SetInt(brain, BRAIN_GHOST_LEG_TICKS, ticks);
    tiGhostFollowFloor(chr);
    memcpy(&limit, brain + BRAIN_GHOST_LEG_LIMIT, sizeof(limit));
    draw = ticks < limit ? GHOST_DRAW_NONE : tnRand(GHOST_DRAW_OF);
    memcpy(point, brain + BRAIN_NEXT_POINT, sizeof(point));
    if (tiGhostUnit(point, (const float *)chr->pos, dir) < GHOST_WANDER_REACH)
        draw = tnRand(GHOST_DRAW_OF);
    if (ssTrigCheckEvent(TRIG_EVENT_GHOST_1D_ON, TRIG_GLOBAL, 0) != 0) {
        memcpy(brain + BRAIN_NEXT_POINT, brain + BRAIN_TRACE_POINT, 16);
        brain[BRAIN_SUBMODE] = GHOST_SEARCH_TO_TRACE;
    } else if (draw < GHOST_DRAW_WANDER) {
        tiGhostSearchMotion(chr, slot);
        tiGhostNewLeg(chr, brain, dir);
        brain[BRAIN_SUBMODE] = GHOST_SEARCH_WANDER;
    } else if (draw < GHOST_DRAW_OF) {
        memcpy(brain + BRAIN_NEXT_POINT, Character[Player_BankID].pos, 16);
        memcpy(point, brain + BRAIN_NEXT_POINT, sizeof(point));
        tiGhostUnit(point, (const float *)chr->pos, dir);
        tiGhostNewLegTime(brain);
        tiGhostSearchMotion(chr, slot);
        brain[BRAIN_SUBMODE] = GHOST_SEARCH_TO_ALYSSA;
    }
    switch (brain[BRAIN_SUBMODE]) {
    case GHOST_SEARCH_WANDER:
    case GHOST_SEARCH_TO_ALYSSA:
    case GHOST_SEARCH_TO_TRACE: {
        int speed;

        memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
        ssAI_CompCharaDir(chr, dir, speed, 0);
        break;
    }
    }
    tiGhostLoopTimer(chr, brain);
}

// CT3 0x01879d70
// The approach (kind 1), a wander that may stand still: fading in; outside its one-shot motions, once it
// moves (submode not GHOST_SEARCH_STILL) it hovers after the floor and turns to its point. Each leg's end
// it draws of 16: under 16 - (BRAIN_GHOST_STAY_ODDS_A + _B) a new leg (GHOST_SEARCH_WANDER), the rest
// Alyssa's position (GHOST_SEARCH_TO_ALYSSA); from standing still both start GHOST_MOTION_SET_OFF, then
// the search motion. It plays its loop SE on the timer.
void tiBrain_Approach_Ghost1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dir[4];
    float point[4];
    int ticks, limit, draw, i;

    tiGhostFadeIn(chr, brain);
    if (tiGhostWanderBusy(chr))
        return;
    memcpy(&ticks, brain + BRAIN_GHOST_LEG_TICKS, sizeof(ticks));
    ticks += gNextTicksInterval;
    tiBrain_SetInt(brain, BRAIN_GHOST_LEG_TICKS, ticks);
    memcpy(point, brain + BRAIN_NEXT_POINT, sizeof(point));
    for (i = 0; i < 4; i++)
        dir[i] = point[i] - ((const float *)chr->pos)[i];
    if (brain[BRAIN_SUBMODE] != GHOST_SEARCH_STILL) {
        int speed;

        tiGhostFollowFloor(chr);
        tiGhostUnit(dir, GHOST_ORIGIN, dir); /* the way from before the hover, normalised after it */
        memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
        ssAI_CompCharaDir(chr, dir, speed, 0);
    }
    memcpy(&limit, brain + BRAIN_GHOST_LEG_LIMIT, sizeof(limit));
    draw = ticks < limit ? GHOST_DRAW_NONE : tnRand(GHOST_DRAW_OF);
    if (draw < GHOST_DRAW_OF - (brain[BRAIN_GHOST_STAY_ODDS_A] + brain[BRAIN_GHOST_STAY_ODDS_B])) {
        tiGhostNewLeg(chr, brain, dir);
        tiGhostSetOff(chr, brain);
        brain[BRAIN_SUBMODE] = GHOST_SEARCH_WANDER;
    } else if (draw < GHOST_DRAW_OF) {
        tiGhostNewLegTime(brain);
        tiGhostSetOff(chr, brain);
        memcpy(brain + BRAIN_NEXT_POINT, Character[Player_BankID].pos, 16);
        brain[BRAIN_SUBMODE] = GHOST_SEARCH_TO_ALYSSA;
    }
    tiGhostLoopTimer(chr, brain);
}

// ---- slice 3: the attacks of the ghosts that wait at their point (Attack_Ghost3, Attack_Ghost2) ----

// The pre-attack's grab: GHOST_MOTION_GRAB_START then GHOST_MOTION_GRAB, GHOST_SE_GRAB, a rattle to break
// free, the hold's ticks from 0 (Attack_Ghost3 / 2; Attack_Ghost4 adds a height test).
static void tiGhostGrab(const CHARACTER *chr, u_char *brain)
{
    CLATTERING_SET set = {-1, -1, -1, -1};

    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_GRAB_START);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_GRAB_START, GHOST_MOTION_GRAB, -1);
    tiBrain_SetInt(brain, BRAIN_GHOST_SE, SSPlaySE(GHOST_SE_BANK, GHOST_SE_GRAB, 1));
    tiBrain_SetInt(brain, BRAIN_GHOST_CLATTER, ControllerClatteringSet(&set));
    tiBrain_SetInt(brain, BRAIN_GHOST_GRAB_TICKS, 0);
}

// The lunge: GHOST_SE_LUNGE, GHOST_MOTION_LUNGE then GHOST_MOTION_LUNGE2 then the float; the ghost rests.
static void tiGhostLunge(const CHARACTER *chr, u_char *brain)
{
    tiBrain_SetInt(brain, BRAIN_GHOST_SE, SSPlaySE(GHOST_SE_BANK, GHOST_SE_LUNGE, 1));
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_LUNGE);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_LUNGE, GHOST_MOTION_LUNGE2, -1);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_LUNGE2, GHOST_MOTION_FLOAT, -1);
    brain[BRAIN_GHOST_REST] = GHOST_REST_GAVE_UP;
    tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
}

// The lunge in flight: within BRAIN_GHOST_HOLD_RANGE it lands on Alyssa (TRIG_EVENT_GHOST_GRAB; the ROM
// first steps by dir * dist, then overwrites the position with hers), else a BRAIN_GHOST_LUNGE_SPEED step
// (xyz).
static void tiGhostFly(CHARACTER *chr, const u_char *brain, const float dir[4], float dist)
{
    float *pos = (float *)chr->pos;
    int i;

    if (dist <= tiBrain_Float(brain, BRAIN_GHOST_HOLD_RANGE)) {
        memcpy(chr->pos, Character[Player_BankID].pos, 16);
        ssTrigRegisterEvent(TRIG_EVENT_GHOST_GRAB, TRIG_GLOBAL, 0);
    } else {
        float step = tiBrain_Float(brain, BRAIN_GHOST_LUNGE_SPEED);

        for (i = 0; i < 3; i++) /* vmulx.xyz, vadd.xyz: w stays */
            pos[i] += dir[i] * step;
    }
}

// The turn in the pre-attack and the lunge (ssAI_CompCharaDir's last argument 1).
static void tiGhostTurnFast(CHARACTER *chr, const u_char *brain, float dir[4])
{
    int speed;

    memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
    ssAI_CompCharaDir(chr, dir, speed, 1);
}

// Any of the action motions (attack, pre-attack, damage, action, post-attack, find, return) plays.
static int tiGhostActing(CHARACTER *chr)
{
    return ssAI_IsAttackPlaying(chr) != 0 || ssAI_IsPreAttackPlaying(chr) != 0 || ssAI_IsDamagePlaying(chr) != 0 ||
           ssAI_IsActionPlaying(chr) != 0 || ssAI_IsPostAttackPlaying(chr) != 0 || ssAI_IsFindPlaying(chr) != 0 ||
           ssAI_IsReturnPlaying(chr) != 0;
}

// The rests count down: GHOST_REST_2 against BRAIN_GHOST_REST2_LIMIT, GHOST_REST_GAVE_UP against
// BRAIN_GHOST_REST_LIMIT; past it, none.
static void tiGhostRest(u_char *brain)
{
    if (brain[BRAIN_GHOST_REST] == GHOST_REST_2) {
        if (tiGhostRestOver(brain, BRAIN_GHOST_REST2_TICKS, BRAIN_GHOST_REST2_LIMIT))
            brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
    } else if (brain[BRAIN_GHOST_REST] == GHOST_REST_GAVE_UP) {
        if (tiGhostRestOver(brain, BRAIN_GHOST_REST_TICKS, BRAIN_GHOST_REST_LIMIT))
            brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
    }
}

// Attack_Ghost3's let-go: back to the float where it holds her, resting; TRIG_EVENT_GHOST_LET_GO.
static void tiGhostLetGoStay(const CHARACTER *chr, u_char *brain, int bitten)
{
    (void)bitten;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_FLOAT);
    brain[BRAIN_GHOST_REST] = GHOST_REST_GAVE_UP;
    tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
    ssTrigRegisterEvent(TRIG_EVENT_GHOST_LET_GO, TRIG_GLOBAL, 0);
}

// Attack_Ghost2's let-go: it vanishes (then the float), resting, and fades out (GHOST_ATTACK_FADING by
// GHOST_POST_FADE a frame).
static void tiGhostLetGoFade(const CHARACTER *chr, u_char *brain, int bitten)
{
    (void)bitten;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_VANISH);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_VANISH, GHOST_MOTION_FLOAT, -1);
    brain[BRAIN_GHOST_REST] = GHOST_REST_GAVE_UP;
    tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
    brain[BRAIN_SUBMODE] = GHOST_ATTACK_FADING;
    tiBrain_SetFloat(brain, BRAIN_GHOST_HIT_FADE, GHOST_POST_FADE);
}

// CT3 0x01878280
// The attack (kind 3) of a ghost that waits at its point (BRAIN_TRACE_POINT): idle, it stays there, its
// rests count down, and, not resting, within BRAIN_GHOST_GRAB_RANGE and facing Alyssa, it lunges; it turns
// to her. The pre-attack turns (fast) and grabs her within GHOST_GRAB_REACH; the lunge flies at her; the
// attack holds her (tiGhostHold), letting go where it is (tiGhostLetGoStay).
void tiBrain_Attack_Ghost3(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dir[4];
    float dist;
    int speed;

    tiGhostDirToPlayer(chr, dir);
    dist = tiGhostToPlayer(chr);
    memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
    if (!tiGhostActing(chr)) {
        memcpy(chr->pos, brain + BRAIN_TRACE_POINT, 16);
        tiGhostRest(brain);
        if (tiCheckAlysStatusForGhost() == 0 && dist < tiBrain_Float(brain, BRAIN_GHOST_GRAB_RANGE) &&
            brain[BRAIN_GHOST_REST] == GHOST_REST_NONE &&
            ssAI_IsTrgWithinAngle(dir, chr, tiBrain_Short(brain, BRAIN_GHOST_GRAB_ANGLE)) != 0)
            tiGhostLunge(chr, brain);
        ssAI_CompCharaDir(chr, dir, speed, 0);
    } else if (ssAI_IsPreAttackPlaying(chr) != 0) {
        tiGhostTurnFast(chr, brain, dir);
        if (tiCheckAlysStatusForGhost() == 0 && dist < GHOST_GRAB_REACH)
            tiGhostGrab(chr, brain);
    } else if (ssAI_IsActionPlaying(chr) != 0) {
        tiGhostTurnFast(chr, brain, dir);
        tiGhostFly(chr, brain, dir, dist);
    } else if (ssAI_IsAttackPlaying(chr) != 0) {
        tiGhostHold(chr, brain, tiGhostLetGoStay);
    }
}

// CT3 0x01878960
// The attack (kind 2) of a ghost that waits at its point, fading: while shown (submode 0) it fades in by
// BRAIN_GHOST_ALPHA_STEP; idle it stays at BRAIN_TRACE_POINT, or, fading (GHOST_ATTACK_FADING), fades out by
// BRAIN_GHOST_HIT_FADE until gone (then shown again). Its rests count down; not resting, within
// BRAIN_GHOST_GRAB_RANGE and facing Alyssa or within GHOST_ATTACK_NEAR, it lunges by chance
// (BRAIN_GHOST_LUNGE_CHANCE of 16; 16 under GHOST_FLAG_PANIC_RATE and TRIG_EVENT_PANIC_3), else rests
// (GHOST_REST_2); farther than GHOST_ATTACK_NEAR it turns to her. The post-attack fades it out; the
// pre-attack grabs within GHOST_GRAB_REACH; the lunge flies at her; the attack holds her, letting go by
// vanishing (tiGhostLetGoFade).
void tiBrain_Attack_Ghost2(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dir[4];
    float dist;

    tiGhostDirToPlayer(chr, dir);
    dist = tiGhostToPlayer(chr);
    if (brain[BRAIN_SUBMODE] == GHOST_ATTACK_SHOWN) {
        float max = tiBrain_Float(brain, BRAIN_GHOST_ALPHA_MAX);

        if (chr->translucentAlpha < max) {
            chr->translucentAlpha += tiBrain_Float(brain, BRAIN_GHOST_ALPHA_STEP);
            if (max < chr->translucentAlpha)
                chr->translucentAlpha = max;
        }
    }
    if (tiGhostActing(chr)) {
        if (ssAI_IsPostAttackPlaying(chr) != 0) {
            brain[BRAIN_SUBMODE] = GHOST_ATTACK_FADING;
            tiBrain_SetFloat(brain, BRAIN_GHOST_HIT_FADE, GHOST_POST_FADE);
            chr->translucentAlpha -= GHOST_POST_FADE;
            if (chr->translucentAlpha < 0.0f)
                chr->translucentAlpha = 0.0f;
        } else if (ssAI_IsPreAttackPlaying(chr) != 0) {
            tiGhostTurnFast(chr, brain, dir);
            if (tiCheckAlysStatusForGhost() == 0 && dist < GHOST_GRAB_REACH)
                tiGhostGrab(chr, brain);
        } else if (ssAI_IsActionPlaying(chr) != 0) {
            tiGhostTurnFast(chr, brain, dir);
            tiGhostFly(chr, brain, dir, dist);
        } else if (ssAI_IsAttackPlaying(chr) != 0) {
            tiGhostHold(chr, brain, tiGhostLetGoFade);
        }
        return;
    }
    if (brain[BRAIN_SUBMODE] == GHOST_ATTACK_SHOWN) {
        memcpy(chr->pos, brain + BRAIN_TRACE_POINT, 16);
    } else {
        chr->translucentAlpha -= tiBrain_Float(brain, BRAIN_GHOST_HIT_FADE);
        if (chr->translucentAlpha < 0.0f) {
            chr->translucentAlpha = 0.0f;
            brain[BRAIN_SUBMODE] = GHOST_ATTACK_SHOWN;
        }
    }
    tiGhostRest(brain);
    if (tiCheckAlysStatusForGhost() == 0 && dist < tiBrain_Float(brain, BRAIN_GHOST_GRAB_RANGE) &&
        brain[BRAIN_GHOST_REST] == GHOST_REST_NONE &&
        (ssAI_IsTrgWithinAngle(dir, chr, tiBrain_Short(brain, BRAIN_GHOST_GRAB_ANGLE)) != 0 ||
         dist < GHOST_ATTACK_NEAR)) {
        int chance;

        if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_PANIC_RATE) != 0 &&
            ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, 0) != 0)
            chance = GHOST_LUNGE_CHANCE_MAX;
        else
            chance = brain[BRAIN_GHOST_LUNGE_CHANCE];
        if (tnRand(GHOST_LUNGE_CHANCE_MAX) < chance) {
            tiGhostLunge(chr, brain);
        } else {
            brain[BRAIN_GHOST_REST] = GHOST_REST_2;
            tiBrain_SetInt(brain, BRAIN_GHOST_REST2_TICKS, 0);
        }
    }
    if (GHOST_ATTACK_NEAR <= dist) {
        int speed;

        memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
        ssAI_CompCharaDir(chr, dir, speed, 0);
    }
}

// Attack_Ghost1's let-go: it vanishes (then the search motion), resting; when she broke free or was spared
// (not after the bite) it notes where it let go (BRAIN_GHOST_LET_GO_AT).
static void tiGhostLetGoSearch(const CHARACTER *chr, u_char *brain, int bitten)
{
    brain[BRAIN_GHOST_REST] = GHOST_REST_GAVE_UP;
    tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_VANISH);
    asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_VANISH, GHOST_MOTION_SEARCH, -1);
    if (!bitten)
        memcpy(brain + BRAIN_GHOST_LET_GO_AT, chr->pos, 16);
}

// CT3 0x01879200
// The attack (kind 1) of a wandering ghost: fading in; outside the lunge it hovers after the floor. Not in
// a one-shot motion and not resting, between GHOST_ATTACK1_MIN and BRAIN_GHOST_GRAB_RANGE from Alyssa: facing
// her it lunges by chance (BRAIN_GHOST_LUNGE_CHANCE of 16; 16 under GHOST_FLAG_PANIC_RATE and
// TRIG_EVENT_PANIC_3; then the search motion), else -- unfaced or the draw lost -- it rests (GHOST_REST_2);
// farther than GHOST_ATTACK1_TURN it turns to her. Otherwise the post-attack rests; the pre-attack grabs
// within GHOST_GRAB_REACH and GHOST_GRAB_HEIGHT; the lunge turns and flies at her; the attack holds her,
// letting go by vanishing (tiGhostLetGoSearch). The rests count down in ChgAttack_Ghost1.
void tiBrain_Attack_Ghost1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    const float *ppos = (const float *)Character[Player_BankID].pos;
    float dir[4];
    float dist;

    tiGhostFadeIn(chr, brain);
    if (ssAI_IsActionPlaying(chr) == 0)
        tiGhostFollowFloor(chr);
    tiGhostDirToPlayer(chr, dir);
    dist = tiGhostToPlayer(chr);
    if (!tiGhostBusy(chr, 1) && brain[BRAIN_GHOST_REST] == GHOST_REST_NONE) {
        if (tiCheckAlysStatusForGhost() == 0 && dist < tiBrain_Float(brain, BRAIN_GHOST_GRAB_RANGE) &&
            GHOST_ATTACK1_MIN < dist) {
            int lunge = 0;

            if (ssAI_IsTrgWithinAngle(dir, chr, tiBrain_Short(brain, BRAIN_GHOST_GRAB_ANGLE)) != 0) {
                int chance;

                if ((brain[BRAIN_GHOST_FLAGS] & GHOST_FLAG_PANIC_RATE) != 0 &&
                    ssTrigCheckEvent(TRIG_EVENT_PANIC_3, TRIG_GLOBAL, 0) != 0)
                    chance = GHOST_LUNGE_CHANCE_MAX;
                else
                    chance = brain[BRAIN_GHOST_LUNGE_CHANCE];
                lunge = tnRand(GHOST_LUNGE_CHANCE_MAX) < chance;
            }
            if (lunge) {
                tiBrain_SetInt(brain, BRAIN_GHOST_SE, SSPlaySE(GHOST_SE_BANK, GHOST_SE_LUNGE, 1));
                asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, GHOST_MOTION_LUNGE);
                asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_LUNGE, GHOST_MOTION_LUNGE2, -1);
                asChangeMortionInfo_NextMortion(chr->id, GHOST_MOTION_LUNGE2, GHOST_MOTION_SEARCH, -1);
            } else {
                brain[BRAIN_GHOST_REST] = GHOST_REST_2;
                tiBrain_SetInt(brain, BRAIN_GHOST_REST2_TICKS, 0);
            }
        }
        if (GHOST_ATTACK1_TURN < dist) {
            int speed;

            memcpy(&speed, brain + BRAIN_TURN_SPEED, sizeof(speed));
            ssAI_CompCharaDir(chr, dir, speed, 0);
        }
        return;
    }
    if (ssAI_IsPostAttackPlaying(chr) != 0) {
        brain[BRAIN_GHOST_REST] = GHOST_REST_GAVE_UP;
        tiBrain_SetInt(brain, BRAIN_GHOST_REST_TICKS, 0);
    } else if (ssAI_IsPreAttackPlaying(chr) != 0) {
        if (tiCheckAlysStatusForGhost() == 0 && dist < GHOST_GRAB_REACH &&
            absf(((const float *)chr->pos)[1] - ppos[1]) < GHOST_GRAB_HEIGHT)
            tiGhostGrab(chr, brain);
    } else if (ssAI_IsActionPlaying(chr) != 0) {
        tiGhostTurnFast(chr, brain, dir);
        tiGhostFly(chr, brain, dir, dist);
    } else if (ssAI_IsAttackPlaying(chr) != 0) {
        tiGhostHold(chr, brain, tiGhostLetGoSearch);
    }
}

// CT3 0x0187aeb0
// The wandering ghost's brain (kind 1): no trace request; its first leg (GHOST_INIT1_LEG ticks) runs
// BRAIN_GHOST_WANDER_STEP from where it starts (BRAIN_TRACE_POINT) on a random heading (GHOST_SEARCH_WANDER);
// no rest; its alpha now is its fade's ceiling; the loop SE after GHOST_SEARCH_TIME_BASE ticks; the start
// motion; Z_REI's sounds loaded; GHOST_TRACE_RATE; the damage rate step 5.
int tiBrain_Init1(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float head[4], point[4];
    float step;
    int i;

    tiBrain_SetShort(brain, BRAIN_GHOST_TRACE_REQ, 0);
    tiBrain_SetInt(brain, BRAIN_GHOST_LEG_LIMIT, GHOST_INIT1_LEG);
    tiBrain_SetInt(brain, BRAIN_GHOST_LEG_TICKS, 0);
    memcpy(brain + BRAIN_TRACE_POINT, chr->pos, 16);
    tiGhostRandomHeading(head);
    step = tiBrain_Float(brain, BRAIN_GHOST_WANDER_STEP);
    for (i = 0; i < 3; i++) /* vmulx.xyz, vadd.xyz: w is the heading's */
        point[i] = head[i] * step + tiBrain_Float(brain, BRAIN_TRACE_POINT + 4 * i);
    point[3] = head[3];
    memcpy(brain + BRAIN_NEXT_POINT, point, sizeof(point));
    brain[BRAIN_GHOST_REST] = GHOST_REST_NONE;
    brain[BRAIN_SUBMODE] = GHOST_SEARCH_WANDER;
    tiBrain_SetFloat(brain, BRAIN_GHOST_ALPHA_MAX, chr->translucentAlpha);
    tiBrain_SetInt(brain, BRAIN_SEARCH_TIME, 0);
    tiBrain_SetInt(brain, BRAIN_SEARCH_TIME_BASE, GHOST_SEARCH_TIME_BASE);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBrain_Short(brain, BRAIN_START_MOTION));
    SSLoadSE(GHOST_SE_BANK, 0);
    brain[BRAIN_TRACE_RATE] = GHOST_TRACE_RATE;
    tiBrain_SetInt(brain, BRAIN_GHOST_W220, 5);
    return 0;
}
