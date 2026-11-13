// game/tiBrain_ChgTrace_Ghost.c -- Clock Tower 3: the ghosts' brain (tiBrain_*_Ghost<n>: the ghost kinds'
// mode functions; tiBrain_Init3, tiBrain_Damage<n>, tiBrain_ChgReturn<n>). Symtab local chunk 81. CodeWarrior C.
// Slice 1 (2026-11-13): the small ones -- the mode changes ChgTrace_Ghost, ChgReturn1 / 2, ChgAttack_Ghost3,
// the bodies Trace_Ghost1, Return_Ghost1, Search_Ghost2, the initialiser Init3 and the helpers tiGetNearPole
// and tiCheckAlysStatusForGhost. A ghost fades in and out through CHARACTER.translucentAlpha.

#include "common/alyssapan_stub.h"
#include "game/tiBrain_ChgTrace_Ghost.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ct3_types.h"
#include "game/fileio.h"
#include "game/main2.h"
#include "game/sound.h"
#include "game/ssBrain_ChgHideAtk1.h"
#include "game/ssGrpPortalCheck.h"
#include "game/tiAvoidObject.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"

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
    tiBrain_SetInt(brain, BRAIN_GHOST_W204, 0);
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x0187aeb0
int tiBrain_Init1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Init1", 0x0187aeb0);
    return {};
}

// stub of CT3 0x0187ae40
int tiBrain_Init2(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Init2", 0x0187ae40);
    return {};
}

// stub of CT3 0x01879d70
void tiBrain_Approach_Ghost1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Approach_Ghost1", 0x01879d70);
}

// stub of CT3 0x01879a80
void tiBrain_Approach_Ghost2(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Approach_Ghost2", 0x01879a80);
}

// stub of CT3 0x01879200
void tiBrain_Attack_Ghost1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_Ghost1", 0x01879200);
}

// stub of CT3 0x01878960
void tiBrain_Attack_Ghost2(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_Ghost2", 0x01878960);
}

// stub of CT3 0x01878280
void tiBrain_Attack_Ghost3(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_Ghost3", 0x01878280);
}

// stub of CT3 0x01877a00
void tiBrain_Attack_Ghost4(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_Ghost4", 0x01877a00);
}

// stub of CT3 0x01876670
void tiBrain_ChgApproach_Ghost1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_ChgApproach_Ghost1", 0x01876670);
}

// stub of CT3 0x01876400
void tiBrain_ChgApproach_Ghost2(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_ChgApproach_Ghost2", 0x01876400);
}

// stub of CT3 0x01876190
void tiBrain_ChgAttack_Ghost1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_ChgAttack_Ghost1", 0x01876190);
}

// stub of CT3 0x01875f60
void tiBrain_ChgAttack_Ghost2(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_ChgAttack_Ghost2", 0x01875f60);
}

// stub of CT3 0x01875c20
void tiBrain_ChgAttack_Ghost4(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_ChgAttack_Ghost4", 0x01875c20);
}

// stub of CT3 0x01876d90
void tiBrain_ChgSearch_Ghost1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_ChgSearch_Ghost1", 0x01876d90);
}

// stub of CT3 0x01876b50
void tiBrain_ChgSearch_Ghost2(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_ChgSearch_Ghost2", 0x01876b50);
}

// stub of CT3 0x018768f0
void tiBrain_ChgSearch_Ghost3(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_ChgSearch_Ghost3", 0x018768f0);
}

// stub of CT3 0x01877610
int tiBrain_Damage1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Damage1", 0x01877610);
    return {};
}

// stub of CT3 0x01877280
int tiBrain_Damage2(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Damage2", 0x01877280);
    return {};
}

// stub of CT3 0x018770e0
int tiBrain_Damage3(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Damage3", 0x018770e0);
    return {};
}

// stub of CT3 0x0187a7b0
void tiBrain_Search_Ghost1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Search_Ghost1", 0x0187a7b0);
}

// stub of CT3 0x0187a2b0
void tiBrain_Search_Ghost3(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Search_Ghost3", 0x0187a2b0);
}

