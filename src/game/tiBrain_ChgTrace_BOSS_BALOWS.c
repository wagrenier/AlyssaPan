// game/tiBrain_ChgTrace_BOSS_BALOWS.c -- Clock Tower 3: the bosses' brain (tiBrain_*_BOSS_<kind>: BALOWS,
// SCISSORS_M / _W, AXE, VITRIOL, HAMM; tiBrain_*_BOSS1; the special-mode, chain and HP helpers tiSet*,
// tiSel*, tiCalc*, tiDefense*, tiCheck*). Symtab local chunk 80. CodeWarrior C.
// Slice 1 (2026-11-13): the mode changes (tiBrain_Chg{Trace,Attack,Approach,Return}_BOSS_*), the trivial
// bodies Trace_BOSS_HAMM and Return_BOSS1, the special-mode pickers tiSetSpecialMode_* and their table
// tiSelSpecialMode. The boss info (pBossInfo) keeps the ROM's layout: its words are read with memcpy.

#include "common/alyssapan_stub.h"
#include "game/tiBrain_ChgTrace_BOSS_BALOWS.h"
#include "game/Clipping_Portal.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ct3_types.h"
#include "game/sound.h"
#include "game/ssBrain_ChgHideAtk1.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/tiAvoidObject.h"
#include "game/tiXcmd_TransItem.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/wideXcmd_TriggerInit.h"
#include "ga/gaInit3dlib.h"

#include <math.h>
#include <string.h>

#define TRIG_GLOBAL 1
#define BRAIN_BOSS_SE 0x278             /* int: the SE handle HAMM's special keeps (the ghosts' BRAIN_GHOST_SE) */
#define BOSS_ATK_PICK_SCISSORS_W 2      /* BRAIN_ATK_PICK ChgApproach_BOSS_SCISSORS_W sets */

// ---- the boss info's words (EE layout, unaligned-safe) ----

static int tiBossInt(int off)
{
    int v;
    memcpy(&v, pBossInfo + off, sizeof(v));
    return v;
}

static void tiBossSetInt(int off, int v)
{
    memcpy(pBossInfo + off, &v, sizeof(v));
}

static u_int tiBossState(void)
{
    return (u_int)tiBossInt(BOSS_INFO_PHASE);
}

static void tiBossSetBits(u_int bits)
{
    tiBossSetInt(BOSS_INFO_PHASE, (int)(tiBossState() | bits));
}

static void tiBossClearBits(u_int bits)
{
    tiBossSetInt(BOSS_INFO_PHASE, (int)(tiBossState() & ~bits));
}

static CHARACTER *tiBossChara(void)
{
    return &Character[pBossInfo[BOSS_INFO_CHARA]];
}

// ---- the mode changes' shared tests ----

static short tiBossMotion(const CHARACTER *chr)
{
    short slot;

    memcpy(&slot, (const u_char *)chr + CHARA_MOTION_SLOTS, sizeof(slot));
    return slot;
}

static short tiBossBrainShort(const u_char *brain, int off)
{
    short v;

    memcpy(&v, brain + off, sizeof(v));
    return v;
}

// The motion chr plays is one of `list` (the one-shots a mode change leaves alone).
static int tiBossBusy(const CHARACTER *chr, const short *list, int count)
{
    short m = tiBossMotion(chr);
    int i;

    for (i = 0; i < count; i++) {
        if (list[i] == m)
            return 1;
    }
    return 0;
}

#define TI_BOSS_BUSY(chr, list) tiBossBusy((chr), (list), (int)(sizeof(list) / sizeof((list)[0])))

// No attack picked (BRAIN_ATK_PICK 0xff): a trace may end.
static int tiBossNoPick(const u_char *brain)
{
    return brain[BRAIN_ATK_PICK] == BRAIN_ATK_NONE;
}

// The first attack or none (the ROM's signed byte < 1).
static int tiBossPickDone(const u_char *brain)
{
    return (s_char)brain[BRAIN_ATK_PICK] < 1;
}

// Into an approach, BOSS_INFO_APPROACH_84 cleared (the trace / attack ends).
static void tiBossToApproach(u_char *brain, int clearSub)
{
    tiBossSetInt(BOSS_INFO_APPROACH_84, 0);
    if (clearSub)
        brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
    brain[BRAIN_MODE] = SS_AI_APPROACH;
}

static void tiBossPick(u_char *brain, int pick, int mode)
{
    brain[BRAIN_ATK_PICK] = (u_char)pick;
    brain[BRAIN_MODE] = (u_char)mode;
}

static int tiBossPlayerAlive(void)
{
    return 0 < Character[Player_BankID].hp;
}

// vsub, vmul, vaddbc x2, vrsqrt, vmulq, vdiv 1 / Q: the distance from chr to Alyssa over xyz, and (dir)
// the unit vector toward her.
static float tiBossToPlayer(const CHARACTER *chr, float *dir)
{
    const float *p = (const float *)Character[Player_BankID].pos;
    const float *c = (const float *)chr->pos;
    float d[3] = { p[0] - c[0], p[1] - c[1], p[2] - c[2] };
    float len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    float inv = len > 0.0f ? 1.0f / len : 0.0f;

    if (dir != NULL) {
        dir[0] = d[0] * inv;
        dir[1] = d[1] * inv;
        dir[2] = d[2] * inv;
        dir[3] = p[3] - c[3];
    }
    return len;
}

// CheckLOS on chr's light bank from `from` to `to`, both raised BOSS_LOS_EYE_Y: != 0 when blocked.
static int tiBossLos(const CHARACTER *chr, const float *from, const float *to)
{
    LOS_HIT hit;

    memcpy(hit.seg.from, from, sizeof(hit.seg.from));
    hit.seg.from[1] += BOSS_LOS_EYE_Y;
    memcpy(hit.seg.to, to, sizeof(hit.seg.to));
    hit.seg.to[1] += BOSS_LOS_EYE_Y;
    return CheckLOS(chr->lightBank, &hit);
}

static int tiBossLosToPlayer(const CHARACTER *chr)
{
    return tiBossLos(chr, (const float *)chr->pos, (const float *)Character[Player_BankID].pos);
}

static int tiBossSpecial(void)
{
    return tiSelSpecialMode[tiBossInt(BOSS_INFO_SPECIAL_KIND)]() != 0;
}

// BALOWS: the second phase's event, deferred past a one-shot (BOSS_STATE_BALOWS_2_DEFER) or due now.
static void tiBossBalows2Fire(u_int clear)
{
    tiBossSetInt(BOSS_INFO_BALOWS_D4, 0);
    tiBossClearBits(clear);
    ssTrigRegisterEvent(TRIG_EVENT_BALOWS_2, TRIG_GLOBAL, 0);
}

static void tiBossBalows2Defer(void)
{
    if ((tiBossState() & BOSS_STATE_BALOWS_2_READY) != 0 && tiBossInt(BOSS_INFO_BALOWS_D4) < 1)
        tiBossSetBits(BOSS_STATE_BALOWS_2_DEFER);
}

// ---- BALOWS ----

static const short BalowsAttackBusy[] = { 0x1e, 0x1d, 0x1c, 0xf, 0xe, 0xd, 0x1b, 0x18, 0xc, 0xb, 0xa, 9, 8 };
static const short BalowsApproachBusy[] = { 0x1f, 0x1c, 0x11, 0x10, 0x19 };

// CT3 0x01867fb0
// No attack picked and neither motion 0x1a nor 7: the second phase's event fires if due, and an approach.
void tiBrain_ChgTrace_BOSS_BALOWS(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    short m = tiBossMotion(chr);

    if (tiBossNoPick(brain) && m != 0x1a && m != 7) {
        if (tiBossInt(BOSS_INFO_BALOWS_D4) < 1)
            tiBossBalows2Fire(BOSS_STATE_BALOWS_2_READY);
        tiBossSetInt(BOSS_INFO_APPROACH_84, 0);
        brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
        brain[BRAIN_MODE] = SS_AI_APPROACH;
        tiBossClearBits(BOSS_STATE_SPECIAL_ON);
    }
}

// CT3 0x01868070
// During a one-shot the second phase is deferred; after it, a deferred phase fires, and with the attack
// done an approach.
void tiBrain_ChgAttack_BOSS_BALOWS(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (TI_BOSS_BUSY(chr, BalowsAttackBusy)) {
        tiBossBalows2Defer();
        return;
    }
    if ((tiBossState() & BOSS_STATE_BALOWS_2_DEFER) != 0)
        tiBossBalows2Fire(BOSS_STATE_BALOWS_2_DEFER | BOSS_STATE_BALOWS_2_READY);
    if (tiBossPickDone(brain)) {
        tiBossSetInt(BOSS_INFO_APPROACH_84, 0);
        brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
        brain[BRAIN_MODE] = SS_AI_APPROACH;
        tiBossClearBits(BOSS_STATE_SPECIAL_ON);
    }
}

// CT3 0x01868200
// With Alyssa alive and outside BOSS_SUB_HOLD_3: a one-shot defers the second phase; else a deferred phase
// fires, then BALOWS_FACING attacks when she is ahead (with the chain only for TRIG_VAR_BALOWS_52 4 / 0xe
// and within BOSS_ATTACK_RANGE_BALOWS_CHAIN, else the facing ends); the special (or BOSS_SUB_SPECIAL)
// traces; anything else attacks.
void tiBrain_ChgApproach_BOSS_BALOWS(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    float dir[4], fwd[4];
    float dist, dot;
    int var;

    if (!tiBossPlayerAlive() || brain[BRAIN_SUBMODE] == BOSS_SUB_HOLD_3)
        return;
    if (TI_BOSS_BUSY(chr, BalowsApproachBusy)) {
        tiBossBalows2Defer();
        return;
    }
    dist = tiBossToPlayer(chr, dir);
    if ((tiBossState() & BOSS_STATE_BALOWS_2_DEFER) != 0)
        tiBossBalows2Fire(BOSS_STATE_BALOWS_2_DEFER | BOSS_STATE_BALOWS_2_READY);
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_BALOWS_FACING) {
        tiGetCharaDirection(fwd, chr);
        dot = gaInnerProductXYZ(dir, fwd);
        if ((tiBossState() & BOSS_STATE_BALOWS_CHAIN) == 0) {
            if (BOSS_FACING_DOT < dot)
                tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_ATTACK);
            return;
        }
        var = ssTrigGetVariable(TRIG_VAR_BALOWS_52, TRIG_GLOBAL, 0);
        if (var == BALOWS_VAR_52_CHAIN_A || var == BALOWS_VAR_52_CHAIN_B) {
            if (dist < BOSS_ATTACK_RANGE_BALOWS_CHAIN && BOSS_FACING_DOT_CHAIN < dot)
                tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_ATTACK);
        } else {
            brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
            tiBossClearBits(BOSS_STATE_BALOWS_CHAIN);
        }
        return;
    }
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_SPECIAL || tiBossSpecial())
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_TRACE);
    else
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_ATTACK);
}

// ---- SCISSORS_M / SCISSORS_W ----

static const short ScissorsMTraceBusy[] = { 0x41, 0x40, 0x3f, 0x3e };
static const short ScissorsMAttackBusy[] = { 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31 };
static const short ScissorsMApproachBusy[] = { 0x44, 0x43, 0x42, 0x50 };

// CT3 0x01868500
void tiBrain_ChgTrace_BOSS_SCISSORS_M(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (tiBossNoPick(brain) && !TI_BOSS_BUSY(chr, ScissorsMTraceBusy)) {
        tiBossClearBits(BOSS_STATE_SPECIAL_ON);
        tiBossToApproach(brain, 1);
    }
}

// CT3 0x01868580
void tiBrain_ChgAttack_BOSS_SCISSORS_M(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (!TI_BOSS_BUSY(chr, ScissorsMAttackBusy) && tiBossPickDone(brain))
        tiBossToApproach(brain, 1);
}

// CT3 0x018685f0
// BOSS_SUB_BALOWS_SPECIAL traces; the special (or BOSS_SUB_SPECIAL) traces and keeps BOSS_SUB_SPECIAL;
// else it attacks when Alyssa is in sight within BOSS_ATTACK_RANGE_SCISSORS_M (BOSS_NEAR_LIMIT for the
// NEAR submodes).
void tiBrain_ChgApproach_BOSS_SCISSORS_M(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    u_char sub = brain[BRAIN_SUBMODE];
    float dist;

    if (!tiBossPlayerAlive() || sub == BOSS_SUB_HOLD_3 || sub == BOSS_SUB_HOLD_5 ||
        TI_BOSS_BUSY(chr, ScissorsMApproachBusy))
        return;
    if (sub == BOSS_SUB_BALOWS_SPECIAL) {
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_TRACE);
        return;
    }
    if (sub == BOSS_SUB_SPECIAL || tiBossSpecial()) {
        brain[BRAIN_SUBMODE] = BOSS_SUB_SPECIAL;
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_TRACE);
        return;
    }
    dist = tiBossToPlayer(chr, NULL);
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_NEAR_4 && !(dist <= BOSS_NEAR_LIMIT))
        return;
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_NEAR_2 && !(dist <= BOSS_NEAR_LIMIT))
        return;
    if (tiBossLosToPlayer(chr) != 0 || !(dist < BOSS_ATTACK_RANGE_SCISSORS_M))
        return;
    tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_ATTACK);
}

// CT3 0x01868860: SCISSORS_W's attack has no mode change.
void tiBrain_ChgAttack_BOSS_SCISSORS_W(CHARACTER *chr)
{
    (void)chr;
}

// CT3 0x01868870
// SCISSORS_W attacks at once: motion BOSS_SCISSORS_W_MOTION_APPROACH, pick 2.
void tiBrain_ChgApproach_BOSS_SCISSORS_W(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    tiBossSetInt(BOSS_INFO_APPROACH_84, 0);
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BOSS_SCISSORS_W_MOTION_APPROACH);
    tiBossPick(brain, BOSS_ATK_PICK_SCISSORS_W, SS_AI_ATTACK);
}

// ---- AXE ----

static const short AxeTraceBusy[] = { 0x41, 0x40, 0x37, 0x2f, 0x2b };
static const short AxeAttackBusy[] = { 0x4d, 0x35, 0x34, 0x33, 0x32, 0x31, 0x45, 0x2e, 0x2d, 0x44 };
static const short AxeApproachBusy[] = { 0x4f, 0x4e, 0x4d, 0x58 };

// CT3 0x018688c0
void tiBrain_ChgTrace_BOSS_AXE(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (tiBossNoPick(brain) && !TI_BOSS_BUSY(chr, AxeTraceBusy)) {
        tiBossClearBits(BOSS_STATE_SPECIAL_ON);
        tiBossSetInt(BOSS_INFO_AXE_F0, 0);
        tiBossToApproach(brain, 1);
    }
}

// CT3 0x01868950
void tiBrain_ChgAttack_BOSS_AXE(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (!TI_BOSS_BUSY(chr, AxeAttackBusy) && tiBossPickDone(brain))
        tiBossToApproach(brain, 1);
}

// CT3 0x01868a00
// The special (or BOSS_SUB_SPECIAL): BOSS_STATE_AXE_SPECIAL, Alyssa's position kept, and a trace; unless
// TRIG_EVENT_BOSS_AXE_NO_STEP is on it checks a BOSS_AXE_STEP step (back when she is ahead, forward when
// behind): clear, it plays the backstep then the attack motion (or the attack motion) with pick 0; blocked,
// pick BOSS_ATK_PICK_BLOCKED. Else it attacks when she is in sight, not throwing, within
// BOSS_ATTACK_RANGE_AXE (BOSS_NEAR_LIMIT for BOSS_SUB_NEAR_4).
void tiBrain_ChgApproach_BOSS_AXE(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    u_char sub = brain[BRAIN_SUBMODE];
    const float *pos = (const float *)chr->pos;
    float dir[4], fwd[4], to[4];
    float dist, step;

    if (!tiBossPlayerAlive() || sub == BOSS_SUB_HOLD_3 || sub == BOSS_SUB_HOLD_5 ||
        TI_BOSS_BUSY(chr, AxeApproachBusy))
        return;
    dist = tiBossToPlayer(chr, dir);
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_SPECIAL || tiBossSpecial()) {
        tiBossSetBits(BOSS_STATE_AXE_SPECIAL);
        memcpy(pBossInfo + BOSS_INFO_SPECIAL_POS, Character[Player_BankID].pos, 16);
        if (ssTrigCheckEvent(TRIG_EVENT_BOSS_AXE_NO_STEP, TRIG_GLOBAL, 0) == 0) {
            tiGetCharaDirection(fwd, chr);
            step = 0.0f < gaInnerProductXYZ(dir, fwd) ? -BOSS_AXE_STEP : BOSS_AXE_STEP;
            memcpy(to, pos, sizeof(to));
            to[0] += fwd[0] * step;
            to[1] += fwd[1] * step;
            to[2] += fwd[2] * step;
            if (tiBossLos(chr, pos, to) == 0) {
                short attack = tiBossBrainShort(brain, BRAIN_ATTACK_MOTION);

                if (step < 0.0f) {
                    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BOSS_AXE_MOTION_BACKSTEP);
                    asChangeMortionInfo_NextMortion(chr->id, BOSS_AXE_MOTION_BACKSTEP, attack, -1);
                } else {
                    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, attack);
                }
                brain[BRAIN_ATK_PICK] = BOSS_ATK_PICK_FIRST;
            } else {
                brain[BRAIN_ATK_PICK] = BOSS_ATK_PICK_BLOCKED;
            }
        } else {
            brain[BRAIN_ATK_PICK] = BOSS_ATK_PICK_BLOCKED;
        }
        brain[BRAIN_MODE] = SS_AI_TRACE;
        return;
    }
    if (tiBossLosToPlayer(chr) == 0 && (brain[BRAIN_SUBMODE] != BOSS_SUB_NEAR_4 || dist <= BOSS_NEAR_LIMIT) &&
        ssAI_AXE_IsThrowing(chr) == 0 && dist < BOSS_ATTACK_RANGE_AXE)
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_ATTACK);
}

// ---- VITRIOL ----

static const short VitriolAttackBusy[] = { 0x30, 0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x28, 0x3a };
static const short VitriolApproachBusy[] = { 0x41, 0x40, 0x3c, 0x4a };
#define VITRIOL_MOTION_SUB_RESET 0x29   /* ChgApproach: playing it resets the submode */

// CT3 0x01868db0
// Alyssa dead: an approach. Else, no attack picked and neither motion 0x3e nor 0x3d: the special's counts
// clear, an approach.
void tiBrain_ChgTrace_BOSS_VITRIOL(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    short m = tiBossMotion(chr);

    if (!tiBossPlayerAlive()) {
        tiBossToApproach(brain, 1);
        return;
    }
    if (tiBossNoPick(brain) && m != 0x3e && m != 0x3d) {
        tiBossSetInt(BOSS_INFO_HITS_70, 0);
        pBossInfo[BOSS_INFO_COUNT_8D] = 0;
        tiBossToApproach(brain, 1);
    }
}

// CT3 0x01868e50
void tiBrain_ChgAttack_BOSS_VITRIOL(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (!TI_BOSS_BUSY(chr, VitriolAttackBusy) && tiBossPickDone(brain))
        tiBossToApproach(brain, 1);
}

// CT3 0x01868f00
// Motion VITRIOL_MOTION_SUB_RESET resets the submode; BOSS_STATE_HOLD_1000 attacks at once; the special
// plays the attack motion and traces; else it attacks when Alyssa is in sight within
// BOSS_ATTACK_RANGE_VITRIOL (BOSS_NEAR_LIMIT for BOSS_SUB_NEAR_4).
void tiBrain_ChgApproach_BOSS_VITRIOL(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    u_char sub = brain[BRAIN_SUBMODE];
    float dist;

    if (!tiBossPlayerAlive() || sub == BOSS_SUB_HOLD_3 || sub == BOSS_SUB_HOLD_5 ||
        TI_BOSS_BUSY(chr, VitriolApproachBusy))
        return;
    if (tiBossMotion(chr) == VITRIOL_MOTION_SUB_RESET) {
        brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
        return;
    }
    if ((tiBossState() & BOSS_STATE_HOLD_1000) != 0) {
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_ATTACK);
        return;
    }
    if (tiBossSpecial()) {
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBossBrainShort(brain, BRAIN_ATTACK_MOTION));
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_TRACE);
        return;
    }
    dist = tiBossToPlayer(chr, NULL);
    if (tiBossLosToPlayer(chr) == 0 && (brain[BRAIN_SUBMODE] != BOSS_SUB_NEAR_4 || dist <= BOSS_NEAR_LIMIT) &&
        dist < BOSS_ATTACK_RANGE_VITRIOL)
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_ATTACK);
}

// ---- HAMM ----

static const short HammAttackBusy[] = { 0x6a, 0x69, 0x57, 0x68, 0x67, 0x66, 0x65, 0x56, 0x55,
                                        0x5e, 0x5d, 0x5c, 0x5b, 0x5a, 0x27, 0x59, 0x58, 0x1d };
static const short HammApproachBusy[] = { 0x6e, 0x6c, 0x6b, 0x51 };

// CT3 0x01869150
// While the special's second motion plays: pick BOSS_ATK_PICK_HAMM_SPECIAL and BOSS_STATE_HAMM_SPECIAL;
// once it is over: the special's counts clear, BOSS_INFO_HAMM_94 comes back, an approach.
void tiBrain_ChgTrace_BOSS_HAMM(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (tiBossMotion(chr) == BOSS_HAMM_MOTION_SPECIAL_2) {
        brain[BRAIN_ATK_PICK] = BOSS_ATK_PICK_HAMM_SPECIAL;
        tiBossSetBits(BOSS_STATE_HAMM_SPECIAL);
    } else if (brain[BRAIN_ATK_PICK] == BOSS_ATK_PICK_HAMM_SPECIAL) {
        tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
        tiBossSetInt(BOSS_INFO_HITS_70, 0);
        tiBossClearBits(BOSS_STATE_SPECIAL_ON | BOSS_STATE_HAMM_SPECIAL);
        pBossInfo[BOSS_INFO_HAMM_94] = pBossInfo[BOSS_INFO_HAMM_95];
        brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
        brain[BRAIN_MODE] = SS_AI_APPROACH;
    }
}

// CT3 0x018691e0: as the others, but the submode stays.
void tiBrain_ChgAttack_BOSS_HAMM(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);

    if (!TI_BOSS_BUSY(chr, HammAttackBusy) && tiBossPickDone(brain))
        tiBossToApproach(brain, 0);
}

// CT3 0x018692f0
// The special: BOSS_INFO_HAMM_94 halved (the old value kept), its SE, the two special motions then the
// search motion, pick 1, a trace, BOSS_STATE_SPECIAL_ON. Else it attacks when Alyssa is in sight within
// BOSS_ATTACK_RANGE_HAMM.
void tiBrain_ChgApproach_BOSS_HAMM(CHARACTER *chr)
{
    u_char *brain = CharaBrainPtr(chr);
    u_char sub = brain[BRAIN_SUBMODE];
    int handle;

    if (!tiBossPlayerAlive() || sub == BOSS_SUB_HOLD_3 || sub == BOSS_SUB_HOLD_5 ||
        TI_BOSS_BUSY(chr, HammApproachBusy))
        return;
    if (tiBossSpecial()) {
        pBossInfo[BOSS_INFO_HAMM_95] = pBossInfo[BOSS_INFO_HAMM_94];
        pBossInfo[BOSS_INFO_HAMM_94] = (u_char)(pBossInfo[BOSS_INFO_HAMM_94] >> 1);
        handle = SSPlaySE(BOSS_HAMM_SE_BANK, BOSS_HAMM_SE_SPECIAL, BOSS_HAMM_SE_ARG);
        memcpy(brain + BRAIN_BOSS_SE, &handle, sizeof(handle));
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BOSS_HAMM_MOTION_SPECIAL);
        asChangeMortionInfo_NextMortion(chr->id, BOSS_HAMM_MOTION_SPECIAL, BOSS_HAMM_MOTION_SPECIAL_2, -1);
        asChangeMortionInfo_NextMortion(chr->id, BOSS_HAMM_MOTION_SPECIAL_2,
                                        tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
        tiBossPick(brain, BOSS_ATK_PICK_BLOCKED, SS_AI_TRACE);
        tiBossSetBits(BOSS_STATE_SPECIAL_ON);
        return;
    }
    if (tiBossLosToPlayer(chr) == 0 && tiBossToPlayer(chr, NULL) < BOSS_ATTACK_RANGE_HAMM)
        tiBossPick(brain, BOSS_ATK_PICK_FIRST, SS_AI_ATTACK);
}

// CT3 0x0186e0c0: HAMM's trace body does nothing (its mode change runs the special).
void tiBrain_Trace_BOSS_HAMM(CHARACTER *chr)
{
    (void)chr;
}

// ---- BOSS1 ----

// CT3 0x01869540: the return has no mode change.
void tiBrain_ChgReturn_BOSS1(CHARACTER *chr)
{
    (void)chr;
}

// CT3 0x0186ec10: the return holds BOSS_RETURN_MOTION.
void tiBrain_Return_BOSS1(CHARACTER *chr)
{
    if (tiBossMotion(chr) != BOSS_RETURN_MOTION)
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BOSS_RETURN_MOTION);
}

// ---- the special-mode pickers (tiSelSpecialMode, by BOSS_INFO_SPECIAL_KIND) ----

// CT3 0x01871eb0
// BALOWS: more than BALOWS_CHAIN_SETS_MIN in byte 0x93 (BOSS_INFO_CHAIN_B_ACTIVE): TRIG_EVENT_BALOWS_SPECIAL
// and the boss's submode BOSS_SUB_BALOWS_SPECIAL.
int tiSetSpecialMode_BALOWS(void)
{
    u_char n = pBossInfo[BOSS_INFO_CHAIN_B_ACTIVE];

    if (BALOWS_CHAIN_SETS_MIN < n) {
        ssTrigRegisterEvent(TRIG_EVENT_BALOWS_SPECIAL, TRIG_GLOBAL, 0);
        CharaBrainPtr(tiBossChara())[BRAIN_SUBMODE] = BOSS_SUB_BALOWS_SPECIAL;
    }
    return BALOWS_CHAIN_SETS_MIN < n;
}

// CT3 0x01871f20
// SCISSORS_M: while TRIG_VAR_SCISSORS_M_03 is below its limit, BOSS_INFO_COUNT_8D above 4 or the special
// count past its mark starts it.
int tiSetSpecialMode_SCISSORS_M(void)
{
    if (ssTrigGetVariable(TRIG_VAR_SCISSORS_M_03, TRIG_GLOBAL, 0) >= SCISSORS_M_VAR_03_LIMIT)
        return 0;
    if (4 < pBossInfo[BOSS_INFO_COUNT_8D] || tiBossInt(BOSS_INFO_SPECIAL_AT) < tiBossInt(BOSS_INFO_SPECIAL_COUNT)) {
        pBossInfo[BOSS_INFO_COUNT_8D] = 0;
        tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
        return 1;
    }
    return 0;
}

// CT3 0x01871f90
// AXE (second form): while the boss throws, the counts clear and nothing starts. Else (byte 0x92 clear
// resets BOSS_INFO_HITS_74) BOSS_INFO_HITS_70 > 7, BOSS_INFO_AXE_88 > 100, the special count past its
// mark, BOSS_STATE_HOLD_1000 or BOSS_STATE_AXE_SPECIAL start it.
int tiSetSpecialMode_AXE2(void)
{
    if (ssAI_AXE_IsThrowing(tiBossChara()) != 0) {
        tiBossSetInt(BOSS_INFO_HITS_70, 0);
        tiBossSetInt(BOSS_INFO_AXE_88, 0);
        tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
        tiBossClearBits(BOSS_STATE_HOLD_1000 | BOSS_STATE_AXE_SPECIAL);
        return 0;
    }
    if (pBossInfo[BOSS_INFO_CHAIN_A_ACTIVE] == 0)
        tiBossSetInt(BOSS_INFO_HITS_74, 0);
    if (tiBossInt(BOSS_INFO_HITS_70) < 8 && tiBossInt(BOSS_INFO_AXE_88) < 0x65 &&
        tiBossInt(BOSS_INFO_SPECIAL_COUNT) <= tiBossInt(BOSS_INFO_SPECIAL_AT) &&
        (tiBossState() & BOSS_STATE_HOLD_1000) == 0 && (tiBossState() & BOSS_STATE_AXE_SPECIAL) == 0)
        return 0;
    tiBossSetInt(BOSS_INFO_HITS_70, 0);
    tiBossSetInt(BOSS_INFO_AXE_88, 0);
    tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
    tiBossClearBits(BOSS_STATE_HOLD_1000);
    return 1;
}

// CT3 0x018720b0
// AXE (first form, Character[BOSS_AXE1_CHARA]): while it throws, the counts clear; else
// BOSS_INFO_HITS_70 > 7, BOSS_INFO_AXE_88 > 200 or BOSS_STATE_AXE_SPECIAL start it.
int tiSetSpecialMode_AXE1(void)
{
    if (ssAI_AXE_IsThrowing(&Character[BOSS_AXE1_CHARA]) != 0) {
        tiBossSetInt(BOSS_INFO_HITS_70, 0);
        tiBossSetInt(BOSS_INFO_AXE_88, 0);
        tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
        tiBossClearBits(BOSS_STATE_HOLD_1000 | BOSS_STATE_AXE_SPECIAL);
        return 0;
    }
    if (7 < tiBossInt(BOSS_INFO_HITS_70) || 200 < tiBossInt(BOSS_INFO_AXE_88) ||
        (tiBossState() & BOSS_STATE_AXE_SPECIAL) != 0) {
        tiBossSetInt(BOSS_INFO_HITS_70, 0);
        tiBossSetInt(BOSS_INFO_AXE_88, 0);
        return 1;
    }
    return 0;
}

// CT3 0x01872150
// VITRIOL: BOSS_INFO_HITS_70 > 9, or BOSS_INFO_COUNT_8D above 2 (3 while byte 0x92 is set).
int tiSetSpecialMode_VITRIOL(void)
{
    u_char limit = pBossInfo[BOSS_INFO_CHAIN_A_ACTIVE] != 0 ? 3 : 2;

    if (9 < tiBossInt(BOSS_INFO_HITS_70) || limit < pBossInfo[BOSS_INFO_COUNT_8D]) {
        tiBossSetInt(BOSS_INFO_HITS_70, 0);
        pBossInfo[BOSS_INFO_COUNT_8D] = 0;
        return 1;
    }
    return 0;
}

// CT3 0x018721a0
// HAMM: BOSS_INFO_HAMM_28 runs only while BOSS_INFO_HITS_74 counts; past BOSS_INFO_HAMM_28_MAX both clear,
// within it more than 2 hits start the special, as do the special count past its mark or
// BOSS_INFO_HITS_70 > 5.
int tiSetSpecialMode_HAMM(void)
{
    int hits = 0;

    if (tiBossInt(BOSS_INFO_HITS_74) == 0)
        tiBossSetInt(BOSS_INFO_HAMM_28, 0);
    if (tiBossInt(BOSS_INFO_HAMM_28) <= BOSS_INFO_HAMM_28_MAX) {
        if (2 < tiBossInt(BOSS_INFO_HITS_74))
            hits = 1;
    } else {
        tiBossSetInt(BOSS_INFO_HAMM_28, 0);
        tiBossSetInt(BOSS_INFO_HITS_74, 0);
    }
    if (tiBossInt(BOSS_INFO_SPECIAL_AT) < tiBossInt(BOSS_INFO_SPECIAL_COUNT) || 5 < tiBossInt(BOSS_INFO_HITS_70) ||
        hits) {
        tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
        tiBossSetInt(BOSS_INFO_HITS_70, 0);
        return 1;
    }
    return 0;
}

// CT3 0x01938640 (.data): by BOSS_INFO_SPECIAL_KIND.
int (*const tiSelSpecialMode[BOSS_SPECIAL_COUNT])(void) = {
    tiSetSpecialMode_HAMM,        /* BOSS_SPECIAL_HAMM */
    tiSetSpecialMode_VITRIOL,     /* BOSS_SPECIAL_VITRIOL */
    tiSetSpecialMode_AXE1,        /* BOSS_SPECIAL_AXE1 */
    tiSetSpecialMode_AXE2,        /* BOSS_SPECIAL_AXE2 */
    tiSetSpecialMode_SCISSORS_M,  /* BOSS_SPECIAL_SCISSORS_M */
    tiSetSpecialMode_BALOWS,      /* BOSS_SPECIAL_BALOWS */
};

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x01874960
int tiBrain_Init_BOSS1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Init_BOSS1", 0x01874960);
    return {};
}

// stub of CT3 0x01874830
int tiBrain_Init_BOSS_SCISSORS_W(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Init_BOSS_SCISSORS_W", 0x01874830);
    return {};
}

// stub of CT3 0x01870e00
void tiBrain_Approach_BOSS1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Approach_BOSS1", 0x01870e00);
}

// stub of CT3 0x0186cac0
void tiBrain_Attack_BOSS_AXE(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_BOSS_AXE", 0x0186cac0);
}

// stub of CT3 0x01869930
void tiBrain_Attack_BOSS_BALOWS(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_BOSS_BALOWS", 0x01869930);
}

// stub of CT3 0x0186e0d0
void tiBrain_Attack_BOSS_HAMM(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_BOSS_HAMM", 0x0186e0d0);
}

// stub of CT3 0x0186af60
void tiBrain_Attack_BOSS_SCISSORS_M(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_BOSS_SCISSORS_M", 0x0186af60);
}

// stub of CT3 0x0186b7a0
void tiBrain_Attack_BOSS_SCISSORS_W(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_BOSS_SCISSORS_W", 0x0186b7a0);
}

// stub of CT3 0x0186d750
void tiBrain_Attack_BOSS_VITRIOL(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_BOSS_VITRIOL", 0x0186d750);
}

// stub of CT3 0x0186ec40
int tiBrain_Damage_BOSS1(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Damage_BOSS1", 0x0186ec40);
    return {};
}

// stub of CT3 0x0186c540
void tiBrain_Trace_BOSS_AXE(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Trace_BOSS_AXE", 0x0186c540);
}

// stub of CT3 0x01869550
void tiBrain_Trace_BOSS_BALOWS(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Trace_BOSS_BALOWS", 0x01869550);
}

// stub of CT3 0x0186ab60
void tiBrain_Trace_BOSS_SCISSORS_M(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Trace_BOSS_SCISSORS_M", 0x0186ab60);
}

// stub of CT3 0x0186cf80
void tiBrain_Trace_BOSS_VITRIOL(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Trace_BOSS_VITRIOL", 0x0186cf80);
}
