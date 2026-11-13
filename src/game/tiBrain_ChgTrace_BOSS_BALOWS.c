// game/tiBrain_ChgTrace_BOSS_BALOWS.c -- Clock Tower 3: the bosses' brain (tiBrain_*_BOSS_<kind>: BALOWS,
// SCISSORS_M / _W, AXE, VITRIOL, HAMM; tiBrain_*_BOSS1; the special-mode, chain and HP helpers tiSet*,
// tiSel*, tiCalc*, tiDefense*, tiCheck*). Symtab local chunk 80. CodeWarrior C.
// Slice 1 (2026-11-13): the mode changes (tiBrain_Chg{Trace,Attack,Approach,Return}_BOSS_*), the trivial
// bodies Trace_BOSS_HAMM and Return_BOSS1, the special-mode pickers tiSetSpecialMode_* and their table
// tiSelSpecialMode. The boss info (pBossInfo) keeps the ROM's layout: its words are read with memcpy.

#include "common/alyssapan_stub.h"
#include "game/tiBrain_ChgTrace_BOSS_BALOWS.h"
#include "game/BRANCH_CLEAR.h"
#include "game/Character_Init.h"
#include "game/Clipping_Portal.h"
#include "game/LinkMatrixTree.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/asUseArrow.h"
#include "game/ct3_types.h"
#include "game/main2.h"
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
#include "ga/gaSinCos.h"

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

static int tiBrainInt(const u_char *brain, int off)
{
    int v;

    memcpy(&v, brain + off, sizeof(v));
    return v;
}

static void tiBrainSetInt(u_char *brain, int off, int v)
{
    memcpy(brain + off, &v, sizeof(v));
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

// ---- slice 2 (2026-11-13): the HP, charge, defense and chain helpers, the BALOWS / SCISSORS_M traces ----

// CT3 0x01875060: chr's hp over its hpMax.
float tiCalcHPRate(CHARACTER *chr)
{
    return (float)chr->hp / (float)chr->hpMax;
}

// CT3 0x01875090: the charge time (ticks) for `value`, in steps of 300 (BOSS_CHARGE_*).
int tiCalcChargeTime(int value)
{
    if (value < BOSS_CHARGE_STEP)
        return BOSS_CHARGE_TIME_0;
    if (value < BOSS_CHARGE_STEP * 2)
        return BOSS_CHARGE_TIME_1;
    if (value < BOSS_CHARGE_STEP * 3)
        return BOSS_CHARGE_TIME_2;
    if (value < BOSS_CHARGE_STEP * 4)
        return BOSS_CHARGE_TIME_3;
    if (value < BOSS_CHARGE_STEP * 5)
        return BOSS_CHARGE_TIME_4;
    return BOSS_CHARGE_TIME_5;
}

// CT3 0x01875100: a boss's hit takes `damage` off Alyssa's hp -- not under TRIG_EVENT_NO_FATAL_HIT or
// TRIG_EVENT_MUTEKI, nor once the boss (chr) is down; halved while she plays a bend, half again more after
// the game was cleared once (the ROM's float multiply, truncated).
void tiCalcPlayerHitPoint(int damage, CHARACTER *chr)
{
    if (ssTrigCheckEvent(TRIG_EVENT_NO_FATAL_HIT, TRIG_GLOBAL, 0) != 0 || chr->hp < 1 ||
        ssTrigCheckEvent(TRIG_EVENT_MUTEKI, TRIG_GLOBAL, 0) != 0)
        return;
    if (ssAI_IsBendPlaying(&Character[Player_BankID]) != 0)
        damage /= 2;
    if (ssTrigCheckEvent(TRIG_EVENT_CLEARED, TRIG_GLOBAL, 0) != 0)
        damage = (int)((float)damage * BOSS_CLEARED_DAMAGE_SCALE);
    Character[Player_BankID].hp -= damage;
}

// CT3 0x01874a70: BALOWS's three attack weights (out[0..2]; Attack_BOSS_BALOWS draws from them) by
// Alyssa's distance (BALOWS_RATE_DIST_*), set A's chains (BOSS_INFO_CHAIN_A_ACTIVE), the second phase
// waiting (BOSS_STATE_BALOWS_2_READY) and Character[BALOWS_CHARA]'s hp rate. The ROM's 0.5 / 0.25 tests
// give the same numbers in several branches: kept as it writes them.
void tiCalcBalowsAttackRate(float dist, int *rate0, int *rate1, int *rate2)
{
    // [distance band][case]: case 0 no chain, phase 1, hp > 0.75; 1 the same, hp lower; 2 no chain,
    // phase 2 waits, hp > 0.75; 3 the same, lower; 4 chain up, hp > 0.75; 5 > 0.5; 6 > 0.25; 7 lower.
    static const signed char rates[4][8][3] = {
        { { 8, 0xb, 0 }, { 8, 0xb, 0 }, { 8, 0xd, 0 }, { 8, 0xd, 0 },
          { 8, 0xd, 0 }, { 8, 0xd, 0 }, { 8, 0xd, 0 }, { 8, 0xd, 0 } },
        { { 6, 9, 0 }, { 4, 7, 9 }, { 6, 0xc, 0 }, { 2, 7, 0xc },
          { 6, 0xc, 0 }, { 3, 8, 0xd }, { 3, 8, 0xd }, { 3, 8, 0xd } },
        { { 3, 7, 0 }, { 1, 4, 7 }, { 3, 9, 0 }, { 0, 5, 10 },
          { 3, 9, 0 }, { 1, 6, 0xb }, { 1, 6, 0xb }, { 1, 7, 0xc } },
        { { 0, 5, 0 }, { 0, 4, 7 }, { 0, 7, 0 }, { 0, 5, 10 },
          { 0, 9, 9 }, { 0, 6, 0xb }, { 0, 6, 0xb }, { 0, 6, 0xc } },
    };
    float hp = tiCalcHPRate(&Character[BALOWS_CHARA]);
    int band, c;

    if (dist < BALOWS_RATE_DIST_0)
        band = 0;
    else if (dist < BALOWS_RATE_DIST_1)
        band = 1;
    else if (dist < BALOWS_RATE_DIST_2)
        band = 2;
    else
        band = 3;
    if (pBossInfo[BOSS_INFO_CHAIN_A_ACTIVE] == 0) {
        c = (tiBossState() & BOSS_STATE_BALOWS_2_READY) == 0 ? 0 : 2;
        if (!(BOSS_HP_RATE_HIGH < hp))
            c++;
    } else if (BOSS_HP_RATE_HIGH < hp) {
        c = 4;
    } else if (BOSS_HP_RATE_MID < hp) {
        c = 5;
    } else if (BOSS_HP_RATE_LOW < hp) {
        c = 6;
    } else {
        c = 7;
    }
    *rate0 = rates[band][c][0];
    *rate1 = rates[band][c][1];
    *rate2 = rates[band][c][2];
}

// CT3 0x01871b70: BALOWS's defense: in a special trace (BOSS_SUB_SPECIAL) before its motion turned
// (BRAIN_ATK_PICK < 2), unless BOSS_STATE_HOLD_1000, or while motion 0x1a plays, the hit is guarded
// (*react = 0, answer 1).
int tiDefenseMode_BALOWS(CHARACTER *chr, const u_char *hit, u_char *react)
{
    const u_char *brain = CharaBrainPtr(chr);

    if ((brain[BRAIN_MODE] == SS_AI_TRACE && brain[BRAIN_SUBMODE] == BOSS_SUB_SPECIAL &&
         hit[BOSS_HIT_KIND] < 2 && (tiBossState() & BOSS_STATE_HOLD_1000) == 0) ||
        tiBossMotion(chr) == BALOWS_MOTION_BREAK) {
        *react = 0;
        return 1;
    }
    return 0;
}

// CT3 0x01871be0: SCISSORS_MAN's defense: the hit's damage comes off chr's hp; still standing, in a trace
// other than BOSS_SUB_BALOWS_SPECIAL at step 2 (BRAIN_ATK_PICK) and a hit kind other than 5, the special's
// time runs out (BOSS_INFO_SPECIAL_TIME = BOSS_INFO_SPECIAL_TIME_MAX) and the hit is guarded.
int tiDefenseMode_SCISSORS_MAN(CHARACTER *chr, const u_char *hit, u_char *react)
{
    const u_char *brain = CharaBrainPtr(chr);
    u_short damage;

    memcpy(&damage, hit + BOSS_HIT_DAMAGE, sizeof(damage));
    chr->hp -= damage;
    if (chr->hp < 1 || brain[BRAIN_MODE] != SS_AI_TRACE)
        return 0;
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_BALOWS_SPECIAL || brain[BRAIN_ATK_PICK] != BOSS_TRACE_STEP_WAIT ||
        hit[BOSS_HIT_KIND] == BOSS_HIT_KIND_5)
        return 0;
    tiBossSetInt(BOSS_INFO_SPECIAL_TIME, tiBossInt(BOSS_INFO_SPECIAL_TIME_MAX));
    *react = 0;
    return 1;
}

// ---- slice 4: the AXE's defense and the special motion pickers ----

static const short AxeDefenseOpen[] = {
    AXE_MOTION_OPEN_2C, AXE_MOTION_OPEN_30, AXE_MOTION_DOWN, AXE_MOTION_ATK_0, AXE_MOTION_ATK_1
};

// The AXE's guard: it turns away from the blow (the hit's direction negated, its Y-axis angle less 90
// degrees, wrapped, in 1/182-degree units) into BRAIN_RETURN_MOTION, then BRAIN_SEARCH_MOTION, back to an
// approach from its first attack.
static void tiAxeGuardTurn(CHARACTER *chr, u_char *brain, const u_char *hit)
{
    float away[4];
    int deg, k;

    for (k = 0; k < 4; k++) {
        float v;

        memcpy(&v, hit + BOSS_HIT_DIR + k * 4, sizeof(v));
        away[k] = v * -1.0f;
    }
    deg = (int)tiGetDegFromVec(away, AXIS_Y) - 90;
    if (deg < 0)
        deg += 360;
    chr->rot = deg * ANGLE_UNITS_PER_DEGREE;
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBossBrainShort(brain, BRAIN_RETURN_MOTION));
    asChangeMortionInfo_NextMortion(chr->id, tiBossBrainShort(brain, BRAIN_RETURN_MOTION),
                                    tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
    brain[BRAIN_MODE] = SS_AI_APPROACH;
    brain[BRAIN_ATK_PICK] = BOSS_ATK_PICK_FIRST;
    brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
    tiBrainSetInt(brain, BRAIN_HIDE_ATK_TIMER, 0);
}

// CT3 0x01871c70: the AXE's defense. Not reeling and with set A's chains down: a blow from behind
// (facing . hit direction above AXE_GUARD_DOT) outside the special only marks BOSS_STATE_HOLD_1000; else,
// once the game was cleared (TRIG_EVENT_BOSS_AXE_NO_STEP) or outside the open motions, the AXE turns from
// the blow (*react = 1, answer 1).
int tiDefenseMode_AXE(CHARACTER *chr, const u_char *hit, u_char *react)
{
    // callcheck: asCharacterMortionChangeRequest, asChangeMortionInfo_NextMortion, tiGetDegFromVec (tiAxeGuardTurn makes them)
    u_char *brain = CharaBrainPtr(chr);
    float fwd[4], dir[4];

    if (ssAI_IsDamagePlaying(chr) != 0 || pBossInfo[BOSS_INFO_CHAIN_A_ACTIVE] != 0)
        return 0;
    tiGetCharaDirection(fwd, chr);
    memcpy(dir, hit + BOSS_HIT_DIR, sizeof(dir));
    if (!(gaInnerProductXYZ(fwd, dir) <= AXE_GUARD_DOT) && (tiBossState() & BOSS_STATE_AXE_SPECIAL) == 0) {
        tiBossSetBits(BOSS_STATE_HOLD_1000);
        return 0;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_AXE_NO_STEP, TRIG_GLOBAL, 0) == 0 && TI_BOSS_BUSY(chr, AxeDefenseOpen))
        return 0;
    tiAxeGuardTurn(chr, brain, hit);
    *react = 1;
    return 1;
}

// The boss behind Alyssa: the boss faces her (facing . to-her above BOSS_SP_BEHIND_DOT) and she faces
// away (both facings' dot above it). The ROM computes the distance too and drops it.
static int tiBossBehindPlayer(void)
{
    CHARACTER *boss = tiBossChara();
    float bossFwd[4], toPlayer[4], playerFwd[4];

    tiGetCharaDirection(bossFwd, boss);
    (void)tiBossToPlayer(boss, toPlayer);
    tiGetCharaDirection(playerFwd, &Character[Player_BankID]);
    return BOSS_SP_BEHIND_DOT < gaInnerProductXYZ(bossFwd, toPlayer) &&
           BOSS_SP_BEHIND_DOT < gaInnerProductXYZ(bossFwd, playerFwd);
}

// The pickers (one per boss) write the special motion's pick to *pick when they choose one.
// CT3 0x01872240 / 0x01872880: BALOWS's and HAMM's always answer 1 and pick none.
int tiSelSPMotion_BALOWS(u_char *pick)
{
    (void)pick;
    return 1;
}

int tiSelSPMotion_HAMM(u_char *pick)
{
    (void)pick;
    return 1;
}

// CT3 0x01872250: SCISSORS_M behind Alyssa picks SP_PICK_BEHIND; with set A's chains up, SP_PICK_CHAIN
// as soon as one holding chain's first part is farther than SCISSORS_M_SP_CHAIN_FAR from her. Answers 1.
int tiSelSPMotion_SCISSORS_M(u_char *pick)
{
    // callcheck: tiGetCharaDirection, gaInnerProductXYZ (tiBossBehindPlayer makes them)
    const float *p = (const float *)Character[Player_BankID].pos;
    const u_char *on;
    float **from;
    int i;

    if (!tiBossBehindPlayer())
        return 1;
    if (pBossInfo[BOSS_INFO_CHAIN_A_ACTIVE] == 0) {
        *pick = SP_PICK_BEHIND;
        return 1;
    }
    on = (const u_char *)BossInfoGetPtr(BOSS_INFO_CHAIN_A_ON);
    from = (float **)BossInfoGetPtr(BOSS_INFO_CHAIN_A_FROM);
    for (i = 0; i < pBossInfo[BOSS_INFO_CHAIN_A_COUNT]; i++) {
        if (on != NULL && from != NULL && on[i] == 1) {
            float d[3] = { from[i][0] - p[0], from[i][1] - p[1], from[i][2] - p[2] };

            if (SCISSORS_M_SP_CHAIN_FAR < sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2])) {
                *pick = SP_PICK_CHAIN;
                return 1;
            }
        }
    }
    *pick = SP_PICK_BEHIND;
    return 1;
}

// CT3 0x018724a0: the second AXE behind Alyssa (she standing) picks SP_PICK_BEHIND. Answers 1.
int tiSelSPMotion_AXE2(u_char *pick)
{
    // callcheck: tiGetCharaDirection, gaInnerProductXYZ (tiBossBehindPlayer makes them)
    if (ssAI_IsDownPlaying(&Character[Player_BankID]) == 0 && tiBossBehindPlayer())
        *pick = SP_PICK_BEHIND;
    return 1;
}

// CT3 0x018725d0: the first AXE behind Alyssa (she standing) picks SP_PICK_BEHIND -- once the game was
// cleared always, else on half the draws, SP_PICK_1 on the rest -- and answers 0; otherwise 1.
int tiSelSPMotion_AXE1(u_char *pick)
{
    // callcheck: tiGetCharaDirection, gaInnerProductXYZ (tiBossBehindPlayer makes them)
    if (ssAI_IsDownPlaying(&Character[Player_BankID]) != 0 || !tiBossBehindPlayer())
        return 1;
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_AXE_NO_STEP, TRIG_GLOBAL, 0) != 0)
        *pick = SP_PICK_BEHIND;
    else
        *pick = tnRand(16) < 8 ? SP_PICK_BEHIND : SP_PICK_1;
    return 0;
}

// CT3 0x01872750: VITRIOL with an arrow on Alyssa's bow picks SP_PICK_VITRIOL or SP_PICK_1 on a draw of
// 16 whose odds grow below half of Character[BOSS_VITRIOL_CHARA]'s hp and once the game was cleared;
// answers 0. No arrow: 1.
int tiSelSPMotion_VITRIOL(u_char *pick)
{
    int r, odds, low;

    if (asGetTheArrowLevel() < 1)
        return 1;
    r = tnRand(16);
    low = tiCalcHPRate(&Character[BOSS_VITRIOL_CHARA]) < BOSS_HP_RATE_MID;
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_AXE_NO_STEP, TRIG_GLOBAL, 0) != 0)
        odds = low ? 12 : 8;
    else
        odds = low ? 8 : 5;
    *pick = r < odds ? SP_PICK_VITRIOL : SP_PICK_1;
    return 0;
}

// out = m * v, the VU0 vmulax / vmadday / vmaddaz / vmaddw sequence (Clipping_Portal.c's PortApplyMatrix,
// which is static there).
static void tiBossApplyMatrix(float *out, float (*m)[4], const float *v)
{
    int j;

    for (j = 0; j < 4; j++)
        out[j] = m[0][j] * v[0] + m[1][j] * v[1] + m[2][j] * v[2] + m[3][j] * v[3];
}

// CT3 0x01875930: chain `i` of set A (setB 0) or set B: its belt's half vectors from the set's source
// (the boss's / Alyssa's spine) to the chain's first part, halved, in each end part's frame (halfB the
// negation), as tiLightBelt2Init's link case does between two parts.
void tiSetChainData(int i, int setB)
{
    LIGHT_BELT2 **belts = (LIGHT_BELT2 **)BossInfoGetPtr(setB ? BOSS_INFO_CHAIN_B_BELTS : BOSS_INFO_CHAIN_A_BELTS);
    const float *src = (const float *)BossInfoGetPtr(setB ? BOSS_INFO_CHAIN_B_SRC : BOSS_INFO_CHAIN_A_SRC);
    float **from = (float **)BossInfoGetPtr(setB ? BOSS_INFO_CHAIN_B_FROM : BOSS_INFO_CHAIN_A_FROM);
    int bank = tiBossInt(setB ? BOSS_INFO_CHAIN_B_BANK : BOSS_INFO_CHAIN_A_BANK);
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    LIGHT_BELT2 *belt = belts[i];
    float inv[4][4], v[4];
    int k;

    for (k = 0; k < 3; k++)
        belt->halfA[k] = (src[k] - from[i][k]) * 0.5f;
    belt->halfA[3] = 0.0f;
    for (k = 0; k < 4; k++)
        belt->halfB[k] = belt->halfA[k] * -1.0f;
    gaInversMatrix(inv, parts[belt->partA].matrix);
    memcpy(v, belt->halfA, sizeof(v));
    tiBossApplyMatrix(belt->halfA, inv, v);
    gaInversMatrix(inv, parts[belt->partB].matrix);
    memcpy(v, belt->halfB, sizeof(v));
    tiBossApplyMatrix(belt->halfB, inv, v);
}

// CT3 0x01875630: chain `i` (0..9) holds (on) or lets go: its data set (tiSetChainData) and its trigger
// event registered, or the event removed. Chains 0..4 use set B's event (TRIG_EVENT_CHAIN_B_FIRST + i) when
// setB, else set A's; 5..9 always set A's (TRIG_EVENT_CHAIN_A_FIRST + i), with setB's data.
void tiSetChainFlag(int i, int on, int setB)
{
    int ev;

    if (i < 0 || i > TRIG_EVENT_CHAIN_A_LAST - TRIG_EVENT_CHAIN_A_FIRST)
        return;
    ev = TRIG_EVENT_CHAIN_A_FIRST + i;
    if (i <= TRIG_EVENT_CHAIN_B_LAST - TRIG_EVENT_CHAIN_B_FIRST && setB)
        ev = TRIG_EVENT_CHAIN_B_FIRST + i;
    if (!on) {
        ssTrigRemoveEvent(ev, TRIG_GLOBAL, 0);
        return;
    }
    tiSetChainData(i, setB);
    ssTrigRegisterEvent(ev, TRIG_GLOBAL, 0);
}

// Every chain of a set lets go: its holding flags cleared, each chain's event removed (setB's events).
static void tiBossChainsRelease(int countOff, int onOff, int setB)
{
    u_char *on = (u_char *)BossInfoGetPtr(onOff);
    int n = pBossInfo[countOff];
    int i;

    for (i = 0; i < n; i++) {
        on[i] = 0;
        tiSetChainFlag(i, 0, setB);
    }
}

// The boss info word BOSS_INFO_AVOID_HIT is tiAvoidObject's followed polygon (the ROM keeps the pointer
// there). PORT: the host pointer stays here and the EE word holds BOSS_INFO_PTR_SET or 0, so a trace's end
// clearing the word (tiBrain_ChgTrace_BOSS_AXE) forgets the polygon as on the PS2.
static void *tiBossAvoidHit;

static int tiBossAvoid(float *dir, CHARACTER *chr)
{
    int r;

    if (tiBossInt(BOSS_INFO_AVOID_HIT) == 0)
        tiBossAvoidHit = NULL;
    r = tiAvoidObject(dir, chr, &tiBossAvoidHit);
    tiBossSetInt(BOSS_INFO_AVOID_HIT, tiBossAvoidHit != NULL ? (int)BOSS_INFO_PTR_SET : 0);
    return r;
}

static void tiBossSetSE(u_char *brain, int handle)
{
    memcpy(brain + BRAIN_BOSS_SE, &handle, sizeof(handle));
}

static int tiBossTurnSpeed(const u_char *brain)
{
    int v;

    memcpy(&v, brain + BRAIN_TURN_SPEED, sizeof(v));
    return v;
}

// CT3 0x01869550: BALOWS's trace. Its special (BOSS_SUB_SPECIAL): step 0 roars (E_BAL 6), plays the
// guard motion and starts the count; step 1, once BOSS_INFO_SPECIAL_COUNT passes BOSS_INFO_SPECIAL_AT, it
// breaks set A's chains (motion 0x1a, then its search motion) and the trace may end. Its chain throw
// (BOSS_SUB_BALOWS_SPECIAL): step 0 turns toward Alyssa, at BALOWS_FACE_DOT throws (motion 0xb); step 1
// waits for TRIG_VAR_BALOWS_54 to reach BALOWS_VAR_54_PULL: set B's chains let go, the pull's damage
// lands on Alyssa, and the trace may end.
void tiBrain_Trace_BOSS_BALOWS(CHARACTER *chr)
{
    // callcheck: tiSetChainFlag (tiBossChainsRelease makes it)
    u_char *brain = CharaBrainPtr(chr);
    u_char *step = brain + BRAIN_ATK_PICK;
    float dir[4], facing[4];

    tiBossToPlayer(chr, dir);
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_BALOWS_SPECIAL) {
        if (*step == BOSS_TRACE_STEP_RUN) {
            if (ssTrigGetVariable(TRIG_VAR_BALOWS_54, TRIG_GLOBAL, 0) < BALOWS_VAR_54_THROWN)
                ssTrigSetVariable(TRIG_VAR_BALOWS_54, BALOWS_VAR_54_STARTED, TRIG_GLOBAL, 0);
            if (ssTrigGetVariable(TRIG_VAR_BALOWS_54, TRIG_GLOBAL, 0) == BALOWS_VAR_54_PULL) {
                pBossInfo[BOSS_INFO_CHAIN_B_ACTIVE] = 0;
                tiBossSetInt(BOSS_INFO_CHAIN_B_TIME, 0);
                tiBossChainsRelease(BOSS_INFO_CHAIN_B_COUNT, BOSS_INFO_CHAIN_B_ON, 1);
                ssTrigSetVariable(TRIG_VAR_BALOWS_54, -1, TRIG_GLOBAL, 0);
                tiCalcPlayerHitPoint(BALOWS_PULL_DAMAGE, chr);
                asChangeAlyssaMortionByDamaging((const float *)Character[BALOWS_CHARA].pos, 0);
                SSStopSE(BALOWS_SE_BANK, BALOWS_SE_CHAIN_LOOP, 1);
                SSStopSE(BALOWS_SE_ALYSSA_BANK, BALOWS_SE_ALYSSA_PULLED, 2);
                ssTrigRemoveEvent(TRIG_EVENT_BALOWS_SPECIAL, TRIG_GLOBAL, 0);
                asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
                *step = BRAIN_ATK_NONE;
            }
        } else if (*step == BOSS_TRACE_STEP_START) {
            tiGetCharaDirection(facing, chr);
            if (gaInnerProductXYZ(dir, facing) < BALOWS_FACE_DOT) {
                ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
            } else {
                asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BALOWS_MOTION_THROW);
                tiBossSetSE(brain, SSPlaySE(BALOWS_VOICE_BANK, BALOWS_VOICE_THROW, BOSS_HAMM_SE_ARG));
                *step = BOSS_TRACE_STEP_RUN;
            }
        }
    } else if (brain[BRAIN_SUBMODE] == BOSS_SUB_SPECIAL) {
        if (*step == BOSS_TRACE_STEP_RUN) {
            if (tiBossInt(BOSS_INFO_SPECIAL_AT) < tiBossInt(BOSS_INFO_SPECIAL_COUNT)) {
                tiBossSetSE(brain, SSPlaySE(BALOWS_VOICE_BANK, BALOWS_VOICE_BREAK, BOSS_HAMM_SE_ARG));
                SSPlaySE(BALOWS_SE_BANK, BALOWS_SE_BREAK, 1);
                asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BALOWS_MOTION_BREAK);
                asChangeMortionInfo_NextMortion(chr->id, BALOWS_MOTION_BREAK,
                                                tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
                pBossInfo[BOSS_INFO_CHAIN_A_ACTIVE] = 0;
                tiBossSetInt(BOSS_INFO_CHAIN_A_TIME, 0);
                tiBossChainsRelease(BOSS_INFO_CHAIN_A_COUNT, BOSS_INFO_CHAIN_A_ON, 0);
                SSStopSE(BALOWS_CHAIN_SE_BANK, BALOWS_CHAIN_SE_HOLD, 1);
                *step = BRAIN_ATK_NONE;
            }
        } else if (*step == BOSS_TRACE_STEP_START) {
            SSPlaySE(BALOWS_SE_BANK, BALOWS_SE_ROAR, 1);
            asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BALOWS_MOTION_SPECIAL);
            tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
            *step = BOSS_TRACE_STEP_RUN;
            tiBossSetBits(BOSS_STATE_SPECIAL_ON);
        }
    }
}

// CT3 0x0186ab60: SCISSORS_M's trace. BOSS_SUB_BALOWS_SPECIAL: while the cut (motion 0x3e) plays it turns
// and the trace ends; else it heads for Alyssa round obstacles until it faces her (SCISSORS_M_FACE_DOT),
// then cuts (E_SMA 0x21), raging (BOSS_STATE_SCISSORS_M_RAGE) at random by its hp rate or always after the
// game was cleared once. Otherwise the special: step 0 opens (motion 0x3f, B_4B02 0xb), step 1 holds it
// with BOSS_INFO_SPECIAL_TIME at 0, step 2 (motion 0x40) waits for the time to run out or
// BOSS_STATE_SCISSORS_M_END: the counters clear, motion 0x41, then its search motion (step 3; motion 0x41
// ends the trace).
void tiBrain_Trace_BOSS_SCISSORS_M(CHARACTER *chr)
{
    // callcheck: tiAvoidObject (tiBossAvoid makes it)
    u_char *brain = CharaBrainPtr(chr);
    u_char *step = brain + BRAIN_ATK_PICK;
    short m = tiBossMotion(chr);
    float dir[4], facing[4];

    tiBossToPlayer(chr, dir);
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_BALOWS_SPECIAL) {
        int odds;

        if (m == SCISSORS_M_MOTION_CUT) {
            ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
            *step = BRAIN_ATK_NONE;
        }
        if (*step == BRAIN_ATK_NONE)
            return;
        tiGetCharaDirection(facing, chr);
        if (!(SCISSORS_M_FACE_DOT <= gaInnerProductXYZ(dir, facing))) {
            if (m != tiBossBrainShort(brain, BRAIN_SEARCH_MOTION))
                asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
            ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), (u_char)tiBossAvoid(dir, chr));
            return;
        }
        {
            float hp = tiCalcHPRate(chr);

            if (BOSS_HP_RATE_HIGH < hp)
                odds = 0;
            else if (BOSS_HP_RATE_MID < hp)
                odds = SCISSORS_M_RAGE_ODDS_MID;
            else
                odds = SCISSORS_M_RAGE_ODDS_LOW;
        }
        if (tnRand(SCISSORS_M_RAGE_DIE) < odds)
            tiBossSetBits(BOSS_STATE_SCISSORS_M_RAGE);
        if (ssTrigCheckEvent(TRIG_EVENT_CLEARED, TRIG_GLOBAL, 0) != 0)
            tiBossSetBits(BOSS_STATE_SCISSORS_M_RAGE);
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, SCISSORS_M_MOTION_CUT);
        asChangeMortionInfo_NextMortion(chr->id, SCISSORS_M_MOTION_CUT, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
        SSPlaySE(SCISSORS_M_SE_BANK, SCISSORS_M_SE_CUT, 1);
        return;
    }
    if (m == SCISSORS_M_MOTION_CLOSE)
        *step = BRAIN_ATK_NONE;
    else if (m == SCISSORS_M_MOTION_WAIT)
        *step = BOSS_TRACE_STEP_WAIT;
    if (*step == BOSS_TRACE_STEP_WAIT) {
        if (tiBossInt(BOSS_INFO_SPECIAL_TIME_MAX) <= tiBossInt(BOSS_INFO_SPECIAL_TIME) ||
            (tiBossState() & BOSS_STATE_SCISSORS_M_END) != 0) {
            tiBossSetSE(brain, SSPlaySE(SCISSORS_M_VOICE_BANK, SCISSORS_M_VOICE_CLOSE, BOSS_HAMM_SE_ARG));
            tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
            tiBossSetInt(BOSS_INFO_HITS_70, 0);
            tiBossSetInt(BOSS_INFO_HITS_74, 0);
            pBossInfo[BOSS_INFO_COUNT_8D] = 0;
            tiBossSetInt(BOSS_INFO_AXE_88, 0);
            tiBossClearBits(BOSS_STATE_SCISSORS_M_END);
            asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, SCISSORS_M_MOTION_CLOSE);
            asChangeMortionInfo_NextMortion(chr->id, SCISSORS_M_MOTION_CLOSE, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
            *step = BOSS_TRACE_STEP_CLOSING;
        }
    } else if (*step == BOSS_TRACE_STEP_RUN) {
        tiBossSetInt(BOSS_INFO_SPECIAL_TIME, 0);
        if (m != SCISSORS_M_MOTION_OPEN)
            asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, SCISSORS_M_MOTION_OPEN);
    } else if (*step == BOSS_TRACE_STEP_START) {
        tiBossSetSE(brain, SSPlaySE(SCISSORS_M_VOICE_BANK, SCISSORS_M_VOICE_OPEN, BOSS_HAMM_SE_ARG));
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, SCISSORS_M_MOTION_OPEN);
        tiBossClearBits(BOSS_STATE_SCISSORS_M_END | BOSS_STATE_SCISSORS_M_400000);
        tiBossSetBits(BOSS_STATE_SPECIAL_ON);
        *step = BOSS_TRACE_STEP_RUN;
    }
}

// ---- slice 3 (2026-11-13): BALOWS's attack ----

static const short BalowsAttackBodyBusy[] = { 0x1e, 0x1d, 0xf, 0xe, 0xd, 0x1b, 0x18, 0xc, 0xa, 9, 0x1c, 8 };

// Attack state n's field at `field` (BRAIN_ATK_STATE, ssBrain_ChgHideAtk1.h).
static u_char *tiBalowsState(u_char *brain, int n)
{
    return brain + BRAIN_ATK_STATES + n * (int)sizeof(BRAIN_ATK_STATE);
}

static int tiBalowsAngleOk(u_char *brain, int n, const float *dir, CHARACTER *chr)
{
    const BRAIN_ATK_STATE *st = (const BRAIN_ATK_STATE *)tiBalowsState(brain, n);

    return ssAI_IsTrgWithinAngle(dir, chr, st->angle) != 0;
}

// The state's timer (.b00 as an int) cleared, and BOSS_INFO_APPROACH_84 set from its .panic.
static void tiBalowsStartTimed(u_char *brain, int n)
{
    const BRAIN_ATK_STATE *st = (const BRAIN_ATK_STATE *)tiBalowsState(brain, n);

    tiBrainSetInt(brain, BRAIN_ATK_STATES + n * (int)sizeof(BRAIN_ATK_STATE), 0);
    tiBossSetInt(BOSS_INFO_APPROACH_84, st->panic);
}

static void tiBalowsPlay(CHARACTER *chr, int motion, int next)
{
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, motion);
    asChangeMortionInfo_NextMortion(chr->id, motion, next, -1);
}

// The grab's turn, lunge and hold.
static void tiBalowsGrab(CHARACTER *chr)
{
    asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BALOWS_MOTION_GRAB_TURN);
    asChangeMortionInfo_NextMortion(chr->id, BALOWS_MOTION_GRAB_TURN, BALOWS_MOTION_GRAB_LUNGE, -1);
    asChangeMortionInfo_NextMortion(chr->id, BALOWS_MOTION_GRAB_LUNGE, BALOWS_MOTION_GRAB, -1);
}

// A timed attack (stamp, charge, down hit) whose state n's timer and BOSS_INFO_APPROACH_84 start.
static void tiBalowsTimedAttack(CHARACTER *chr, u_char *brain, int n, int motion, int special)
{
    tiBalowsPlay(chr, motion, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
    tiBalowsStartTimed(brain, n);
    if (special)
        tiBossSetBits(BOSS_STATE_SPECIAL_ON);
    tiBossClearBits(BOSS_STATE_BALOWS_FOLLOWED);
    brain[BRAIN_ATK_PICK] = 1;
}

// Alyssa's current motion record's MOTION_REC_KIND2 (-1 without a table; the ROM reads it regardless).
static int tiBalowsAlyssaKind2(const CHARACTER *ch)
{
    const u_char *table;
    u_int ee;
    short slot;

    memcpy(&ee, (const u_char *)ch + CHARA_MOTION_TABLE, sizeof(ee));
    memcpy(&slot, (const u_char *)ch + CHARA_MOTION_SLOTS, sizeof(slot));
    table = (const u_char *)BankEePtr(ch->bank, ee);
    return table != NULL ? table[slot * CHARA_MOTION_SIZE + MOTION_REC_KIND2] : -1;
}

// A timed one-shot runs: state n's timer counts, chr turns toward Alyssa until it passes the state's limit
// (.chain); a landed blow (CHARA_HIT_LANDED) decides once whether the follow-up motion comes next.
static void tiBalowsTimedRun(CHARACTER *chr, u_char *brain, const float *dir, int n, int motion, int coin)
{
    const BRAIN_ATK_STATE *st = (const BRAIN_ATK_STATE *)tiBalowsState(brain, n);
    int off = BRAIN_ATK_STATES + n * (int)sizeof(BRAIN_ATK_STATE);
    int landed;

    tiBrainSetInt(brain, off, tiBrainInt(brain, off) + gNextTicksInterval);
    if (tiBrainInt(brain, off) < st->chain)
        ssAI_CompCharaDir(chr, (float *)dir, tiBossTurnSpeed(brain), 1);
    memcpy(&landed, (const u_char *)chr + CHARA_HIT_LANDED, sizeof(landed));
    if (landed == 0 || (tiBossState() & BOSS_STATE_BALOWS_FOLLOWED) != 0)
        return;
    if (!coin) {
        asChangeMortionInfo_NextMortion(chr->id, motion, tiBossBrainShort(brain, BRAIN_BALOWS_FOLLOW_MOTION), -1);
        tiBossSetBits(BOSS_STATE_BALOWS_FOLLOW | BOSS_STATE_BALOWS_FOLLOWED);
        return;
    }
    if (tnRand(10) < 5) {
        asChangeMortionInfo_NextMortion(chr->id, motion, tiBossBrainShort(brain, BRAIN_BALOWS_FOLLOW_MOTION), -1);
        tiBossSetBits(BOSS_STATE_BALOWS_FOLLOW);
    } else {
        asChangeMortionInfo_NextMortion(chr->id, motion, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
    }
    tiBossSetBits(BOSS_STATE_BALOWS_FOLLOWED);
}

// The grab ends (rattle won, lost, or Alyssa down): the slot dropped, BALOWS_GRABBED, the grab state's
// timer cleared, the grab's SE stopped; won or lost, the grab's time feeds the second phase.
static void tiBalowsGrabEnd(u_char *brain, int charge)
{
    tiBrainSetInt(brain, BRAIN_BALOWS_CLATTER, -1);
    tiBossClearBits(BOSS_STATE_BALOWS_GRAB);
    tiBossSetBits(BOSS_STATE_BALOWS_GRABBED);
    tiBrainSetInt(brain, BRAIN_ATK_STATES + BALOWS_ATK_GRAB * (int)sizeof(BRAIN_ATK_STATE), 0);
    if (charge) {
        tiBossSetInt(BOSS_INFO_BALOWS_D4,
                     tiBossInt(BOSS_INFO_BALOWS_D4) + tiCalcChargeTime(tiBossInt(BOSS_INFO_GRAB_TICKS)));
        tiBossSetBits(BOSS_STATE_BALOWS_2_READY);
    }
    SSStopSE(BALOWS_SE_BANK, BALOWS_SE_GRAB, 1);
}

// BALOWS_MOTION_GRAB: Alyssa held (BALOWS_LOCK), the bite's damage on her, twice that healed (to hpMax).
// The first frame opens the rattle; then the grab's ticks count until she wins it (BALOWS_MOTION_GRAB_FREE),
// loses it (BALOWS_MOTION_GRAB_DROP, the bite timer from 0) or is down (the drop, no charge).
static void tiBalowsGrabHold(CHARACTER *chr, u_char *brain)
{
    const BRAIN_ATK_STATE *st = (const BRAIN_ATK_STATE *)tiBalowsState(brain, BALOWS_ATK_GRAB);
    int slot, got;

    asAlyssaLockedByAnything(BALOWS_LOCK | LOCKED_SET_1);
    tiCalcPlayerHitPoint(st->panic, chr);
    chr->hp += st->panic * 2;
    if (chr->hpMax < chr->hp)
        chr->hp = chr->hpMax;
    if ((tiBossState() & BOSS_STATE_BALOWS_GRAB) == 0) {
        CLATTERING_SET set = { st->wFar, -1, -1, -1 };

        tiBrainSetInt(brain, BRAIN_BALOWS_CLATTER, ControllerClatteringSet(&set));
        tiBossSetBits(BOSS_STATE_BALOWS_GRAB);
        tiBossClearBits(BOSS_STATE_BALOWS_2_DEFER);
        tiBossSetInt(BOSS_INFO_GRAB_TICKS, 0);
        SSPlaySE(BALOWS_SE_BANK, BALOWS_SE_GRAB, 1);
        return;
    }
    tiBossSetInt(BOSS_INFO_GRAB_TICKS, tiBossInt(BOSS_INFO_GRAB_TICKS) + gNextTicksInterval);
    slot = tiBrainInt(brain, BRAIN_BALOWS_CLATTER);
    if (slot == -1)
        return;
    got = ControllerClatteringGet(slot);
    if (Character[Player_BankID].hp < 0) {
        if (got == CLATTERING_RUNNING)
            ControllerClatteringFree(slot);
        asAlyssaLockedByAnything(BALOWS_LOCK | LOCKED_SET_2);
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BALOWS_MOTION_GRAB_DROP);
        tiBalowsGrabEnd(brain, 0);
        return;
    }
    if (got == CLATTERING_WON) {
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BALOWS_MOTION_GRAB_FREE);
        tiBalowsGrabEnd(brain, 1);
    } else if (got == CLATTERING_LOST) {
        asAlyssaLockedByAnything(BALOWS_LOCK | LOCKED_SET_3);
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BALOWS_MOTION_GRAB_DROP);
        tiBossSetInt(BOSS_INFO_BITE_TICKS, 0);
        tiBalowsGrabEnd(brain, 1);
    }
}

// BALOWS_MOTION_BLOOD: the blood part (BOSS_INFO_BLOOD_PART) lands on Alyssa (odds by hp rate) or
// BALOWS_BLOOD_AHEAD ahead of her, raised; TRIG_VAR_BALOWS_52 tells the script which, the strike's event
// goes. PORT: no blood part set skips the placement (the ROM writes through a null pointer).
static void tiBalowsBlood(CHARACTER *chr, u_char *brain)
{
    const CHARACTER *player = &Character[Player_BankID];
    u_char *part = (u_char *)BossInfoGetPtr(BOSS_INFO_BLOOD_PART);
    int odds = BALOWS_BLOOD_HP_RATE < tiCalcHPRate(chr) ? BALOWS_BLOOD_ODDS_HIGH : BALOWS_BLOOD_ODDS_LOW;
    float pos[4];
    int var;

    if (tnRand(AI_ATK_WEIGHT) < odds) {
        memcpy(pos, player->pos, sizeof(pos));
    } else {
        float fwd[4];
        int k;

        tiGetCharaDirection(fwd, player);
        for (k = 0; k < 3; k++)
            pos[k] = fwd[k] * BALOWS_BLOOD_AHEAD + ((const float *)player->pos)[k];
        pos[1] += BALOWS_BLOOD_RAISE;
    }
    pos[3] = 1.0f;
    if (part != NULL) {
        memcpy(part + PARTS_ANIM_BASE_POS - PARTS_HDR_RECORDS, pos, sizeof(pos));
        part[PARTS_ANIM_FLAGS - PARTS_HDR_RECORDS] = BALOWS_BLOOD_FLAGS;
    }
    var = BALOWS_VAR_52_BLOOD;
    if ((tiBossState() & BOSS_STATE_BALOWS_2_READY) != 0 && (tiBossState() & BOSS_STATE_BALOWS_NEAR_BLOOD) != 0) {
        var = BALOWS_VAR_52_BLOOD_NEAR;
        tiBossClearBits(BOSS_STATE_BALOWS_NEAR_BLOOD);
    }
    ssTrigSetVariable(TRIG_VAR_BALOWS_52, var, TRIG_GLOBAL, 0);
    tiBossSetSE(brain, SSPlaySE(BALOWS_VOICE_BANK, BALOWS_VOICE_BLOOD, BOSS_HAMM_SE_ARG));
    ssTrigRemoveEvent(TRIG_EVENT_BOSS_STRIKE, TRIG_GLOBAL, 0);
    brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
}

// BALOWS_MOTION_SWING: state 0's timer turns it; the first frame (pick 1) sets TRIG_VAR_BALOWS_56 by the
// phase, hp and the cleared flag and makes the voice due, which comes past BALOWS_VOICE_DELAY.
static void tiBalowsSwing(CHARACTER *chr, u_char *brain, const float *dir)
{
    const BRAIN_ATK_STATE *st = (const BRAIN_ATK_STATE *)tiBalowsState(brain, BALOWS_ATK_SWING);
    int off = BRAIN_ATK_STATES + BALOWS_ATK_SWING * (int)sizeof(BRAIN_ATK_STATE);

    tiBrainSetInt(brain, off, tiBrainInt(brain, off) + gNextTicksInterval);
    if (tiBrainInt(brain, off) < st->chain)
        ssAI_CompCharaDir(chr, (float *)dir, tiBossTurnSpeed(brain), 1);
    if (brain[BRAIN_ATK_PICK] == 1) {
        float hp;
        int v;

        tiBossClearBits(BOSS_STATE_BALOWS_100000);
        ssTrigSetVariable(TRIG_VAR_BALOWS_08, -1, TRIG_GLOBAL, 0);
        hp = tiCalcHPRate(chr);
        if ((tiBossState() & BOSS_STATE_BALOWS_2_READY) == 0)
            v = ssTrigCheckEvent(TRIG_EVENT_CLEARED, TRIG_GLOBAL, 0) != 0 ? 1 : 0;
        else if (hp <= BOSS_HP_RATE_MID)
            v = tnRand(10) < 5 ? 1 : 2;
        else
            v = ssTrigCheckEvent(TRIG_EVENT_CLEARED, TRIG_GLOBAL, 0) != 0 ? 2 : 1;
        ssTrigSetVariable(TRIG_VAR_BALOWS_56, v, TRIG_GLOBAL, 0);
        tiBossSetBits(BOSS_STATE_BALOWS_FOLLOW);
        brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
    }
    if ((tiBossState() & BOSS_STATE_BALOWS_FOLLOW) != 0 && BALOWS_VOICE_DELAY <= tiBrainInt(brain, off)) {
        tiBossSetSE(brain, SSPlaySE(BALOWS_VOICE_BANK, BALOWS_VOICE_SWING, BOSS_HAMM_SE_ARG));
        tiBossClearBits(BOSS_STATE_BALOWS_FOLLOW);
    }
}

// No one-shot playing: pick the next attack. BALOWS_FACING swings (or, chained, grabs); near, the down hit
// when Alyssa is down, else the grab once, else the stamp; farther, tiCalcBalowsAttackRate's weights pick
// the blood throw, the swing, or the charge / stamp.
static void tiBalowsPickAttack(CHARACTER *chr, u_char *brain, const float *dir, float dist)
{
    CHARACTER *player = &Character[Player_BankID];
    u_char *pick = brain + BRAIN_ATK_PICK;
    int r, r0, r1, r2, n;

    if (*pick == BRAIN_ATK_NONE)
        return;
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_BALOWS_FACING) {
        if ((tiBossState() & BOSS_STATE_BALOWS_CHAIN) == 0) {
            tiBalowsPlay(chr, BALOWS_MOTION_SWING, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
            tiBrainSetInt(brain, BRAIN_ATK_STATES + BALOWS_ATK_SWING * (int)sizeof(BRAIN_ATK_STATE), 0);
        } else {
            tiBossClearBits(BOSS_STATE_BALOWS_CHAIN);
            if (ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, TRIG_GLOBAL, 0) == 0)
                tiBalowsGrab(chr);
            tiBossClearBits(BOSS_STATE_BALOWS_ATTACKING);
        }
        *pick = 1;
        brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
        return;
    }
    r = tnRand(AI_ATK_WEIGHT);
    if (dist < BALOWS_NEAR) {
        if (ssAI_IsDownPlaying(player) != 0 || tiBalowsAlyssaKind2(player) == ALYSSA_MOTION_KIND2_HELD) {
            if (tiBalowsAngleOk(brain, BALOWS_ATK_DOWN_HIT, dir, chr))
                tiBalowsTimedAttack(chr, brain, BALOWS_ATK_DOWN_HIT, BALOWS_MOTION_DOWN_HIT, 0);
            return;
        }
        if ((tiBossState() & BOSS_STATE_BALOWS_GRABBED) == 0 &&
            ssTrigCheckEvent(TRIG_EVENT_BALOWS_NO_GRAB, TRIG_GLOBAL, 0) == 0 &&
            ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, TRIG_GLOBAL, 0) == 0) {
            if (!tiBalowsAngleOk(brain, BALOWS_ATK_GRAB, dir, chr))
                return;
            tiBalowsGrab(chr);
            tiBossClearBits(BOSS_STATE_BALOWS_FOLLOWED);
            *pick = 1;
            tiBossClearBits(BOSS_STATE_BALOWS_ATTACKING);
            return;
        }
        if ((tiBossState() & BOSS_STATE_BALOWS_ATTACKING) != 0 || !tiBalowsAngleOk(brain, BALOWS_ATK_STAMP, dir, chr))
            return;
        tiBalowsTimedAttack(chr, brain, BALOWS_ATK_STAMP, BALOWS_MOTION_STAMP, 1);
        return;
    }
    if ((tiBossState() & BOSS_STATE_BALOWS_ATTACKING) != 0)
        return;
    tiCalcBalowsAttackRate(dist, &r0, &r1, &r2);
    n = r < r0 ? 0 : r < r1 ? 1 : r < r2 ? 2 : -1;
    tiBossClearBits(BOSS_STATE_BALOWS_NEAR_BLOOD);
    {
        const float *blood = (const float *)BossInfoGetPtr(BOSS_INFO_BLOOD_POS);
        float near;

        memcpy(&near, pBossInfo + BOSS_INFO_BLOOD_NEAR, sizeof(near));
        if (blood != NULL) {
            const float *p = (const float *)player->pos;
            float d[3] = { p[0] - blood[0], p[1] - blood[1], p[2] - blood[2] };

            if (sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) < near)
                tiBossSetBits(BOSS_STATE_BALOWS_NEAR_BLOOD);
        }
    }
    if (0 < ssTrigGetVariable(TRIG_VAR_BALOWS_52, TRIG_GLOBAL, 0) && n == 2)
        n = 1;
    if (ssAI_IsDownPlaying(player) == 0) {
        if (n == 2) {
            if (tiBalowsAngleOk(brain, BALOWS_ATK_BLOOD, dir, chr)) {
                tiBalowsPlay(chr, BALOWS_MOTION_BLOOD, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
                *pick = 1;
            }
        } else if (n == 1) {
            if (tiBalowsAngleOk(brain, BALOWS_ATK_SWING, dir, chr)) {
                tiBalowsPlay(chr, BALOWS_MOTION_SWING, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
                tiBrainSetInt(brain, BRAIN_ATK_STATES + BALOWS_ATK_SWING * (int)sizeof(BRAIN_ATK_STATE), 0);
                *pick = 1;
            }
        } else if (n == 0) {
            int odds = dist < BALOWS_CHARGE_NEAR ? BALOWS_CHARGE_ODDS_NEAR
                     : dist < BALOWS_CHARGE_MID  ? BALOWS_CHARGE_ODDS_MID
                                                 : BALOWS_CHARGE_ODDS_FAR;

            if (tnRand(AI_ATK_WEIGHT) < odds) {
                if (tiBalowsAngleOk(brain, BALOWS_ATK_CHARGE, dir, chr))
                    tiBalowsTimedAttack(chr, brain, BALOWS_ATK_CHARGE, BALOWS_MOTION_CHARGE, 1);
            } else if (tiBalowsAngleOk(brain, BALOWS_ATK_STAMP, dir, chr)) {
                tiBalowsTimedAttack(chr, brain, BALOWS_ATK_STAMP, BALOWS_MOTION_STAMP, 1);
            }
        }
    }
    if (*pick != 0) {
        tiBrainSetInt(brain, BRAIN_BALOWS_ATK_TICKS, 0);
        tiBossSetBits(BOSS_STATE_BALOWS_ATTACKING);
    }
}

// CT3 0x01869930: BALOWS's attack. With no one-shot playing it picks the next attack
// (tiBalowsPickAttack); else it runs the one playing: the swing, charge, stamp and down hit turn on their
// timers and may chain a follow-up; the grab's lunge holds Alyssa within reach, its hold runs the rattle;
// the drop bites on a timer; the recover / free motions return to the search motion; the blood throw
// places the blood.
void tiBrain_Attack_BOSS_BALOWS(CHARACTER *chr)
{
    // callcheck: ControllerClatteringFree, ControllerClatteringGet, ControllerClatteringSet, SSStopSE (tiBalowsGrabHold makes them)
    // callcheck: asCharacterMortionChangeRequest, ssAI_IsDownPlaying, ssTrigCheckEvent, ssTrigGetVariable (tiBalowsPlay / PickAttack)
    // callcheck: ssTrigRemoveEvent, ssTrigSetVariable, tiCalcBalowsAttackRate, tiCalcChargeTime (tiBalowsBlood / Swing / PickAttack)
    // callcheck: tiGetCharaDirection, tnRand (tiBalowsBlood / PickAttack)
    u_char *brain = CharaBrainPtr(chr);
    u_char *pick = brain + BRAIN_ATK_PICK;
    short m = tiBossMotion(chr);
    float dir[4];
    float dist = tiBossToPlayer(chr, dir);

    if (!TI_BOSS_BUSY(chr, BalowsAttackBodyBusy)) {
        tiBalowsPickAttack(chr, brain, dir, dist);
        return;
    }
    switch (m) {
    case BALOWS_MOTION_GRAB_DROP:
        if (*pick != BRAIN_ATK_NONE) {
            tiBossSetInt(BOSS_INFO_BITE_TICKS, tiBossInt(BOSS_INFO_BITE_TICKS) + gNextTicksInterval);
            if (BALOWS_BITE_TICKS < tiBossInt(BOSS_INFO_BITE_TICKS)) {
                tiCalcPlayerHitPoint(BALOWS_BITE_DAMAGE, chr);
                *pick = BRAIN_ATK_NONE;
            }
        }
        asChangeMortionInfo_NextMortion(chr->id, BALOWS_MOTION_GRAB_DROP, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
        brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
        return;
    case BALOWS_MOTION_RECOVER:
    case BALOWS_MOTION_GRAB_FREE:
        if (m == BALOWS_MOTION_RECOVER && (tiBossState() & BOSS_STATE_BALOWS_FOLLOW) != 0) {
            tiBossSetInt(BOSS_INFO_BALOWS_10, 0);
            tiBossSetBits(BOSS_STATE_BALOWS_RECOVERED);
            tiBossSetSE(brain, SSPlaySE(BALOWS_SE_BANK, BALOWS_SE_RECOVER, BOSS_HAMM_SE_ARG));
            tiBossClearBits(BOSS_STATE_BALOWS_FOLLOW);
        }
        asChangeMortionInfo_NextMortion(chr->id, m, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
        brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
        *pick = BRAIN_ATK_NONE;
        return;
    case BALOWS_MOTION_GRAB:
        tiBalowsGrabHold(chr, brain);
        return;
    case BALOWS_MOTION_GRAB_LUNGE:
        if (*pick != 2) {
            if (BALOWS_GRAB_REACH < dist || ssAI_IsTrgWithinAngle(dir, chr, BALOWS_GRAB_ANGLE) == 0) {
                tiBalowsPlay(chr, BALOWS_MOTION_GRAB_MISS, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
                *pick = BRAIN_ATK_NONE;
                return;
            }
            tiBossSetSE(brain, SSPlaySE(BALOWS_VOICE_BANK,
                                        BALOWS_LUNGE_HP_RATE < tiCalcHPRate(chr) ? BALOWS_VOICE_LUNGE_HIGH
                                                                                 : BALOWS_VOICE_LUNGE_LOW,
                                        BOSS_HAMM_SE_ARG));
            *pick = 2;
        }
        asAlyssaLockedByAnything(BALOWS_LOCK | LOCKED_SET_1);
        return;
    case BALOWS_MOTION_GRAB_TURN:
        ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
        return;
    case BALOWS_MOTION_SWING:
        tiBalowsSwing(chr, brain, dir);
        return;
    case BALOWS_MOTION_BLOOD:
        if (*pick != BRAIN_ATK_NONE)
            tiBalowsBlood(chr, brain);
        return;
    case BALOWS_MOTION_DOWN_HIT:
        tiBalowsTimedRun(chr, brain, dir, BALOWS_ATK_DOWN_HIT, BALOWS_MOTION_DOWN_HIT, 0);
        *pick = BRAIN_ATK_NONE;
        return;
    case BALOWS_MOTION_STAMP:
        tiBalowsTimedRun(chr, brain, dir, BALOWS_ATK_STAMP, BALOWS_MOTION_STAMP, 1);
        if (*pick != BRAIN_ATK_NONE)
            tiBossSetSE(brain, SSPlaySE(BALOWS_SE_BANK, BALOWS_SE_STAMP, BOSS_HAMM_SE_ARG));
        *pick = BRAIN_ATK_NONE;
        return;
    case BALOWS_MOTION_CHARGE:
        tiBalowsTimedRun(chr, brain, dir, BALOWS_ATK_CHARGE, BALOWS_MOTION_CHARGE, 1);
        if (*pick != BRAIN_ATK_NONE)
            tiBossSetSE(brain, SSPlaySE(BALOWS_VOICE_BANK, BALOWS_VOICE_CHARGE, BOSS_HAMM_SE_ARG));
        *pick = BRAIN_ATK_NONE;
        return;
    default:                            /* BALOWS_MOTION_GRAB_MISS */
        return;
    }
}

// ---- slice 3: SCISSORS_M's attack ----

static const short ScissorsMAttackBodyBusy[] = { 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31 };

// A timed one-shot of state n with its end motion: its timer turns chr; a landed blow (once) queues the end
// motion, then the search motion.
static void tiScissorsTimedRun(CHARACTER *chr, u_char *brain, float *dir, int n, int motion, int end)
{
    const BRAIN_ATK_STATE *st = (const BRAIN_ATK_STATE *)tiBalowsState(brain, n);
    int off = BRAIN_ATK_STATES + n * (int)sizeof(BRAIN_ATK_STATE);
    int landed;

    tiBrainSetInt(brain, off, tiBrainInt(brain, off) + gNextTicksInterval);
    if (tiBrainInt(brain, off) < st->chain)
        ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
    memcpy(&landed, (const u_char *)chr + CHARA_HIT_LANDED, sizeof(landed));
    if (landed != 0 && (tiBossState() & BOSS_STATE_BALOWS_FOLLOWED) == 0) {
        asChangeMortionInfo_NextMortion(chr->id, motion, end, -1);
        asChangeMortionInfo_NextMortion(chr->id, end, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
        tiBossSetBits(BOSS_STATE_BALOWS_FOLLOWED);
    }
}

// The attack's start: motion, state n's timer and BOSS_INFO_APPROACH_84 (tiBalowsStartTimed), pick 1,
// one more toward the special (BOSS_INFO_COUNT_8D).
static void tiScissorsStart(CHARACTER *chr, u_char *brain, int n, int motion)
{
    tiBalowsPlay(chr, motion, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
    tiBalowsStartTimed(brain, n);
    tiBossClearBits(BOSS_STATE_BALOWS_FOLLOWED);
    brain[BRAIN_ATK_PICK] = 1;
    pBossInfo[BOSS_INFO_COUNT_8D]++;
}

// A fresh slash (pick 0) draws its combo's length.
static void tiScissorsComboDraw(const u_char *brain)
{
    static const int lengths[8] = { 1, 1, 2, 2, 3, 4, 5, 6 };

    if (brain[BRAIN_ATK_PICK] != BOSS_ATK_PICK_FIRST)
        return;
    tiBossSetInt(BOSS_INFO_COMBO_LEN, lengths[tnRand(8)]);
    tiBossSetInt(BOSS_INFO_COMBO_DONE, 1);
}

// CT3 0x0186af60: SCISSORS_M's attack. A combo under way slashes again while Alyssa is ahead. Else: Alyssa
// down within SCISSORS_M_NEAR, the stab; near, the slash (a fresh combo); BOSS_SUB_NEAR_2, the thrust;
// farther (no attack running), the slash at odds by distance and set A's chains. The one-shots turn on
// their timers and queue their end motion when the blow lands.
void tiBrain_Attack_BOSS_SCISSORS_M(CHARACTER *chr)
{
    // callcheck: asCharacterMortionChangeRequest, ssAI_IsTrgWithinAngle (tiBalowsPlay / tiBalowsAngleOk make them)
    u_char *brain = CharaBrainPtr(chr);
    u_char *pick = brain + BRAIN_ATK_PICK;
    short m = tiBossMotion(chr);
    float dir[4], fwd[4];
    float dist = tiBossToPlayer(chr, dir);
    int odds;

    if (TI_BOSS_BUSY(chr, ScissorsMAttackBodyBusy)) {
        if (m == SCISSORS_M_MOTION_STAB || m == SCISSORS_M_MOTION_THRUST) {
            if (m == SCISSORS_M_MOTION_STAB)
                tiScissorsTimedRun(chr, brain, dir, SCISSORS_M_ATK_STAB, m, SCISSORS_M_MOTION_STAB_END);
            else
                tiScissorsTimedRun(chr, brain, dir, SCISSORS_M_ATK_THRUST, m, SCISSORS_M_MOTION_THRUST_END);
            if (*pick == BRAIN_ATK_NONE)
                return;
            brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
            *pick = BRAIN_ATK_NONE;
            return;
        }
        if (m == SCISSORS_M_MOTION_SLASH) {
            const BRAIN_ATK_STATE *st = (const BRAIN_ATK_STATE *)tiBalowsState(brain, SCISSORS_M_ATK_SLASH);
            int off = BRAIN_ATK_STATES + SCISSORS_M_ATK_SLASH * (int)sizeof(BRAIN_ATK_STATE);
            int landed;

            tiBrainSetInt(brain, off, tiBrainInt(brain, off) + gNextTicksInterval);
            if (tiBrainInt(brain, off) < st->chain)
                ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
            memcpy(&landed, (const u_char *)chr + CHARA_HIT_LANDED, sizeof(landed));
            if (landed == 0 || (tiBossState() & BOSS_STATE_BALOWS_FOLLOWED) != 0) {
                if (tiBossInt(BOSS_INFO_COMBO_LEN) <= tiBossInt(BOSS_INFO_COMBO_DONE))
                    tiBossSetInt(BOSS_INFO_COMBO_LEN, 0);
            } else {
                asChangeMortionInfo_NextMortion(chr->id, SCISSORS_M_MOTION_SLASH, SCISSORS_M_MOTION_SLASH_END, -1);
                asChangeMortionInfo_NextMortion(chr->id, SCISSORS_M_MOTION_SLASH_END,
                                                tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
                tiBossSetBits(BOSS_STATE_BALOWS_FOLLOWED);
                pBossInfo[BOSS_INFO_COUNT_8D] = 0;
                tiBossSetInt(BOSS_INFO_COMBO_LEN, 0);
                brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
            }
            *pick = BRAIN_ATK_NONE;
        }
        return;
    }
    if (*pick == BRAIN_ATK_NONE)
        return;
    if (tiBossInt(BOSS_INFO_COMBO_LEN) != 0) {
        tiGetCharaDirection(fwd, chr);
        if (gaInnerProductXYZ(dir, fwd) < 0.0f) {
            *pick = BRAIN_ATK_NONE;
            return;
        }
        tiBalowsPlay(chr, SCISSORS_M_MOTION_SLASH, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
        tiBalowsStartTimed(brain, SCISSORS_M_ATK_SLASH);
        tiBossSetInt(BOSS_INFO_COMBO_DONE, tiBossInt(BOSS_INFO_COMBO_DONE) + 1);
        tiBossClearBits(BOSS_STATE_BALOWS_FOLLOWED);
        return;
    }
    (void)tnRand(AI_ATK_WEIGHT);        /* drawn and unused (the ROM's) */
    if (ssAI_IsDownPlaying(&Character[Player_BankID]) != 0) {
        if (dist < SCISSORS_M_NEAR && tiBalowsAngleOk(brain, SCISSORS_M_ATK_STAB, dir, chr))
            tiScissorsStart(chr, brain, SCISSORS_M_ATK_STAB, SCISSORS_M_MOTION_STAB);
        return;
    }
    if (dist < SCISSORS_M_NEAR) {
        if (!tiBalowsAngleOk(brain, SCISSORS_M_ATK_SLASH, dir, chr))
            return;
        tiScissorsComboDraw(brain);
        tiScissorsStart(chr, brain, SCISSORS_M_ATK_SLASH, SCISSORS_M_MOTION_SLASH);
        return;
    }
    if (brain[BRAIN_SUBMODE] == BOSS_SUB_NEAR_2) {
        if (tiBalowsAngleOk(brain, SCISSORS_M_ATK_THRUST, dir, chr))
            tiScissorsStart(chr, brain, SCISSORS_M_ATK_THRUST, SCISSORS_M_MOTION_THRUST);
        return;
    }
    if ((tiBossState() & BOSS_STATE_BALOWS_ATTACKING) != 0)
        return;
    if (pBossInfo[BOSS_INFO_CHAIN_A_ACTIVE] == 0)
        odds = dist < SCISSORS_M_DIST_1 ? 5 : dist < SCISSORS_M_DIST_2 ? 3 : dist < SCISSORS_M_DIST_3 ? 2 : 0;
    else
        odds = dist < SCISSORS_M_DIST_1 ? 3 : dist < SCISSORS_M_DIST_2 ? 1 : 0;
    if (tnRand(AI_ATK_WEIGHT) < odds && tiBalowsAngleOk(brain, SCISSORS_M_ATK_SLASH, dir, chr)) {
        tiScissorsComboDraw(brain);
        tiScissorsStart(chr, brain, SCISSORS_M_ATK_SLASH, SCISSORS_M_MOTION_SLASH);
    }
    tiBrainSetInt(brain, BRAIN_BALOWS_ATK_TICKS, 0);
    tiBossSetBits(BOSS_STATE_BALOWS_ATTACKING);
}

// ---- slice 4: SCISSORS_W's attack ----

static const short ScissorsWAttackBodyBusy[] = {
    0x14, 0x13, 0x40, 0x3f, 0x3a, 0x39, 0x38, 0x30, 0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x28
};

// |a - b| over xyz (the vsub / vrsqrt / vdiv sequence).
static float tiBossVecDist(const float *a, const float *b)
{
    float d[3] = { a[0] - b[0], a[1] - b[1], a[2] - b[2] };

    return sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
}

// Attack state n's strike: its timer turns chr; the first landed blow queues its end motion.
static void tiScissorsWStrike(CHARACTER *chr, u_char *brain, float *dir, int n, int motion, int end)
{
    const BRAIN_ATK_STATE *st = (const BRAIN_ATK_STATE *)tiBalowsState(brain, n);
    int off = BRAIN_ATK_STATES + n * (int)sizeof(BRAIN_ATK_STATE);
    int landed;

    tiBrainSetInt(brain, off, tiBrainInt(brain, off) + gNextTicksInterval);
    if (tiBrainInt(brain, off) < st->chain)
        ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
    memcpy(&landed, (const u_char *)chr + CHARA_HIT_LANDED, sizeof(landed));
    if (landed != 0 && (tiBossState() & BOSS_STATE_BALOWS_FOLLOWED) == 0) {
        tiBossSetBits(BOSS_STATE_BALOWS_FOLLOWED);
        asChangeMortionInfo_NextMortion(chr->id, motion, end, -1);
    }
}

// The strike of state n: motion, then `next`; its timer and BOSS_INFO_APPROACH_84 start (pick 1).
static void tiBossStrikeStart(CHARACTER *chr, u_char *brain, int n, int motion, int next)
{
    tiBalowsPlay(chr, motion, next);
    tiBalowsStartTimed(brain, n);
    tiBossClearBits(BOSS_STATE_BALOWS_FOLLOWED);
    brain[BRAIN_ATK_PICK] = SCISSORS_W_PICK_STRIKE;
}

// SCISSORS_W's strikes end in the vanish.
static void tiScissorsWStart(CHARACTER *chr, u_char *brain, int n, int motion)
{
    tiBossStrikeStart(chr, brain, n, motion, SCISSORS_W_MOTION_VANISH);
}

// Copy a warp point's position (the ROM's four words) to chr.
static void tiScissorsWPlace(CHARACTER *chr, const float *at)
{
    memcpy(&chr->pos, at, sizeof(float) * 4);
}

// The warp out of sight. Fewer than SCISSORS_W_WARP_NEAR_HITS hits: the next warp point. Else, on 2 of 10
// with Alyssa not bending and nothing in the way, beside her: 200 ahead at 60, 180 or 300 degrees from her
// facing (from her eyes, SCISSORS_W_EYE_Y up) clear of the scene, the boss lands 120 that way; else the
// nearest point to her (the one after it when she already stands there) on 3 of 10, or the next point.
static void tiScissorsWWarp(CHARACTER *chr)
{
    float **warp = (float **)BossInfoGetPtr(BOSS_INFO_WARP_POS);
    int count = tiBossInt(BOSS_INFO_WARP_COUNT);
    CHARACTER *pl = &Character[Player_BankID];
    const float *p = (const float *)pl->pos;
    int idx = tiBossInt(BOSS_INFO_WARP_INDEX);
    int r, k;

    if (warp == NULL || count < 1)
        return;                         /* PORT: no warp list (the ROM reads through null) */
    if (tiBossInt(BOSS_INFO_HITS_70) < SCISSORS_W_WARP_NEAR_HITS) {
        idx = (idx + 1) % count;
        tiBossSetInt(BOSS_INFO_WARP_INDEX, idx);
        tiScissorsWPlace(chr, warp[idx]);
        return;
    }
    {
        LOS_HIT los;
        float rot[4][4], fwd[4], side[4];
        int a;

        memcpy(los.seg.from, p, sizeof(los.seg.from));
        los.seg.from[1] += SCISSORS_W_EYE_Y;
        r = tnRand(10);
        a = r < 3 ? 60 : r < 6 ? 300 : 180;
        gaSetRotMatrixY(rot, a * ANGLE_UNITS_PER_DEGREE);
        tiGetCharaDirection(fwd, pl);
        tiBossApplyMatrix(side, rot, fwd);
        for (k = 0; k < 3; k++)         /* vadd.xyz: the w words stay as they were */
            los.seg.to[k] = los.seg.from[k] + side[k] * SCISSORS_W_SIDE_LOOK;
        los.seg.to[3] = los.seg.from[3];
        r = tnRand(10);
        if (ssAI_IsBendPlaying(pl) == 0 && r < 2 && CheckLOS(chr->lightBank, &los) == 0) {
            float at[4];

            for (k = 0; k < 3; k++)
                at[k] = los.seg.from[k] + side[k] * SCISSORS_W_SIDE_LAND;
            at[3] = los.seg.from[3];
            tiScissorsWPlace(chr, at);
            return;
        }
    }
    if (r < 5) {
        int nearest = 0;
        float best = tiBossVecDist(p, warp[0]);

        for (k = 1; k < count; k++) {
            float d = tiBossVecDist(p, warp[k]);

            if (d < best) {
                best = d;
                nearest = k;
            }
        }
        idx = idx == nearest ? (nearest + 1) % count : nearest;
    } else {
        idx = (idx + 1) % count;
    }
    tiBossSetInt(BOSS_INFO_WARP_INDEX, idx);
    tiScissorsWPlace(chr, warp[idx]);
}

// Hidden (SCISSORS_W_MOTION_HIDDEN) at the warp step: warp, then either the special (every 4th / 6th warp
// by hp, Alyssa alive, far, still, not bending and seen long enough: BOSS_STATE_SCISSORS_W_SPECIAL, the
// special's motion next) or back to the attack motion with a voice on 6 of 10; then the hidden delay drawn,
// the boss hidden, Alyssa's position kept.
static void tiScissorsWHide(CHARACTER *chr, u_char *brain)
{
    static const int delays[8] = { 0, 0, 0, 150, 150, 300, 600, 900 };
    CHARACTER *pl = &Character[Player_BankID];
    float kept[4];
    int warps, every, r;
    float moved, dist;

    tiScissorsWWarp(chr);
    warps = tiBossInt(BOSS_INFO_WARP_TIMES) + 1;
    tiBossSetInt(BOSS_INFO_WARP_TIMES, warps);
    every = BOSS_HP_RATE_MID < tiCalcHPRate(chr) ? 6 : 4;
    memcpy(kept, pBossInfo + BOSS_INFO_SPECIAL_POS, sizeof(kept));
    moved = tiBossVecDist(kept, (const float *)pl->pos);
    dist = tiBossVecDist((const float *)pl->pos, (const float *)chr->pos);
    if (dist <= SCISSORS_W_SPECIAL_FAR && warps % every == 0) {
        warps--;
        tiBossSetInt(BOSS_INFO_WARP_TIMES, warps);
    }
    if (pl->hp < 1 || dist <= SCISSORS_W_SPECIAL_FAR || warps == 0 ||
        (warps % every != 0 && (ssAI_IsBendPlaying(pl) != 0 || SCISSORS_W_STILL <= moved ||
                                tiBossInt(BOSS_INFO_SHOWN_TICKS) < SCISSORS_W_SPECIAL_SHOWN))) {
        if (0 < pl->hp) {
            r = tnRand(10);
            if (r < 3)
                tiBossSetSE(brain, SSPlaySE(SCISSORS_W_VOICE_BANK, SCISSORS_W_VOICE_WARP, BOSS_HAMM_SE_ARG));
            else if (r < 6)
                tiBossSetSE(brain, SSPlaySE(SCISSORS_W_SE_BANK, SCISSORS_W_SE_WARP, BOSS_HAMM_SE_ARG));
        }
        asChangeMortionInfo_NextMortion(chr->id, SCISSORS_W_MOTION_HIDDEN, tiBossBrainShort(brain, BRAIN_ATTACK_MOTION), -1);
    } else {
        tiBossSetSE(brain, SSPlaySE(SCISSORS_W_VOICE_BANK, SCISSORS_W_VOICE_SPECIAL, BOSS_HAMM_SE_ARG));
        tiBossSetBits(BOSS_STATE_SCISSORS_W_SPECIAL);
        ssTrigSetVariable(TRIG_VAR_BALOWS_52, 0, TRIG_GLOBAL, 0);
        asChangeMortionInfo_NextMortion(chr->id, SCISSORS_W_MOTION_HIDDEN, SCISSORS_W_MOTION_SPECIAL, -1);
        tiBossSetInt(BOSS_INFO_HITS_74, 0);
        tiBossSetInt(BOSS_INFO_WARP_TIMES, 0);
    }
    tiBossSetInt(BOSS_INFO_HIDDEN_TICKS, 0);
    tiBossSetInt(BOSS_INFO_HIDDEN_DELAY, delays[tnRand(8)]);
    TrgCharacterDraw(0, chr->id);
    memcpy(pBossInfo + BOSS_INFO_SPECIAL_POS, pl->pos, sizeof(float) * 4);
    brain[BRAIN_ATK_PICK] = BOSS_ATK_PICK_FIRST;
}

// Not in a strike: far (SCISSORS_W_REACH) or outside state 0's angle, chr turns toward Alyssa around
// what is in the way; near, she down: state 3's strike; standing: state 0's or state 1's at even odds.
// Seen longer than BOSS_INFO_THRESHOLDS[BOSS_INFO_SHOWN_PICK]: the vanish.
static void tiScissorsWPickAttack(CHARACTER *chr, u_char *brain, float *dir, float dist)
{
    int shownMax;

    if (SCISSORS_W_REACH < dist || !tiBalowsAngleOk(brain, SCISSORS_W_ATK_0, dir, chr)) {
        ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), tiBossAvoid(dir, chr));
    } else if (ssAI_IsDownPlaying(&Character[Player_BankID]) != 0) {
        if (tiBalowsAngleOk(brain, SCISSORS_W_ATK_DOWN, dir, chr))
            tiScissorsWStart(chr, brain, SCISSORS_W_ATK_DOWN, SCISSORS_W_MOTION_DOWN);
    } else if (tnRand(16) < 8) {
        if (tiBalowsAngleOk(brain, SCISSORS_W_ATK_0, dir, chr))
            tiScissorsWStart(chr, brain, SCISSORS_W_ATK_0, SCISSORS_W_MOTION_ATK_0);
    } else {
        if (tiBalowsAngleOk(brain, SCISSORS_W_ATK_1, dir, chr))
            tiScissorsWStart(chr, brain, SCISSORS_W_ATK_1, SCISSORS_W_MOTION_ATK_1);
    }
    memcpy(&shownMax, pBossInfo + BOSS_INFO_THRESHOLDS + tiBossInt(BOSS_INFO_SHOWN_PICK) * 4, sizeof(shownMax));
    if (shownMax < tiBossInt(BOSS_INFO_SHOWN_TICKS)) {
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, SCISSORS_W_MOTION_VANISH);
        brain[BRAIN_ATK_PICK] = SCISSORS_W_PICK_STRIKE;
    }
}

// CT3 0x0186b7a0: SCISSORS_W's attack. She strikes (states 0, 1 and 3, each with its end motion), vanishes
// (SCISSORS_W_MOTION_VANISH), and while hidden (SCISSORS_W_MOTION_HIDDEN) warps once (tiScissorsWHide) and
// reappears when the hidden delay ran out, facing Alyssa; the special waits for TRIG_VAR_BALOWS_52 999.
void tiBrain_Attack_BOSS_SCISSORS_W(CHARACTER *chr)
{
    // callcheck: CheckLOS, gaSetRotMatrixY, ssAI_IsBendPlaying, tiGetCharaDirection (tiScissorsWWarp makes them)
    // callcheck: SSPlaySE, TrgCharacterDraw, ssTrigSetVariable, tiCalcHPRate (tiScissorsWHide makes them)
    // callcheck: ssAI_IsDownPlaying, ssAI_IsTrgWithinAngle, tiAvoidObject (tiScissorsWPickAttack / tiBossAvoid)
    // callcheck: asCharacterMortionChangeRequest, ssAI_CompCharaDir (tiBalowsPlay / tiScissorsWStrike / tiScissorsWPickAttack)
    u_char *brain = CharaBrainPtr(chr);
    u_char *pick = brain + BRAIN_ATK_PICK;
    short m = tiBossMotion(chr);
    float dir[4];
    float dist = tiBossToPlayer(chr, dir);

    if (!TI_BOSS_BUSY(chr, ScissorsWAttackBodyBusy)) {
        tiScissorsWPickAttack(chr, brain, dir, dist);
        return;
    }
    switch (m) {
    case SCISSORS_W_MOTION_HIDDEN:
        if (*pick == SCISSORS_W_PICK_HIDDEN)
            tiScissorsWHide(chr, brain);
        if ((chr->flags & CHARACTER_FLAG_SHADOW) == 0 &&
            tiBossInt(BOSS_INFO_HIDDEN_DELAY) < tiBossInt(BOSS_INFO_HIDDEN_TICKS)) {
            if (Character[Player_BankID].hp < 1)
                return;
            TrgCharacterDraw(1, chr->id);
            tiBossSetInt(BOSS_INFO_SHOWN_TICKS, 0);
            tiBossSetInt(BOSS_INFO_SHOWN_PICK, tnRand(7));
        }
        chr->rot = ((int)tiGetDegFromVec(dir, AXIS_Y) - 90) * ANGLE_UNITS_PER_DEGREE;
        return;
    case SCISSORS_W_MOTION_VANISH:
        if (*pick == SCISSORS_W_PICK_STRIKE || *pick == SCISSORS_W_PICK_ENDED) {
            asChangeMortionInfo_NextMortion(chr->id, SCISSORS_W_MOTION_VANISH, SCISSORS_W_MOTION_HIDDEN, -1);
            *pick = SCISSORS_W_PICK_HIDDEN;
        }
        return;
    case SCISSORS_W_MOTION_DOWN_END:
    case SCISSORS_W_MOTION_ATK_1_END:
    case SCISSORS_W_MOTION_ATK_0_END:
        if (*pick == SCISSORS_W_PICK_STRIKE) {
            tiBossSetSE(brain, SSPlaySE(SCISSORS_W_VOICE_BANK, SCISSORS_W_VOICE_END, BOSS_HAMM_SE_ARG));
            asChangeMortionInfo_NextMortion(chr->id, m, SCISSORS_W_MOTION_VANISH, -1);
            asChangeMortionInfo_NextMortion(chr->id, SCISSORS_W_MOTION_VANISH, SCISSORS_W_MOTION_VANISH, -1);
            (*pick)++;
        }
        return;
    case SCISSORS_W_MOTION_SPECIAL_END:
        tiBossClearBits(BOSS_STATE_SCISSORS_W_SPECIAL);
        tiBossSetInt(BOSS_INFO_SHOWN_TICKS, SCISSORS_W_SHOWN_FOREVER);
        return;
    case SCISSORS_W_MOTION_SPECIAL_WAIT:
        if (ssTrigGetVariable(TRIG_VAR_BALOWS_52, TRIG_GLOBAL, 0) == SCISSORS_W_VAR_52_DONE ||
            (tiBossState() & BOSS_STATE_SCISSORS_W_SPECIAL_DONE) != 0) {
            tiBossClearBits(BOSS_STATE_SCISSORS_W_SPECIAL_DONE);
            tiBalowsPlay(chr, SCISSORS_W_MOTION_SPECIAL_END, tiBossBrainShort(brain, BRAIN_ATTACK_MOTION));
        }
        return;
    case SCISSORS_W_MOTION_DOWN:
        tiScissorsWStrike(chr, brain, dir, SCISSORS_W_ATK_DOWN, m, SCISSORS_W_MOTION_DOWN_END);
        return;
    case SCISSORS_W_MOTION_ATK_1:
        tiScissorsWStrike(chr, brain, dir, SCISSORS_W_ATK_1, m, SCISSORS_W_MOTION_ATK_1_END);
        return;
    case SCISSORS_W_MOTION_ATK_0:
        tiScissorsWStrike(chr, brain, dir, SCISSORS_W_ATK_0, m, SCISSORS_W_MOTION_ATK_0_END);
        return;
    default:                            /* 0x13, 0x14, the special 0x3a, 0x2a, 0x2d, 0x30: nothing */
        return;
    }
}

// ---- slice 4: the AXE's trace ----

// The AXE's special run (step BOSS_TRACE_STEP_START): within AXE_RUN_NEAR of where Alyssa stood when the
// special began it runs on along its facing (around what is in the way) in its attack motion until a wall
// AXE_RUN_LOOK ahead (from its eyes) stops it; farther, or blocked: the turn step.
static void tiAxeSpecialRun(CHARACTER *chr, u_char *brain, short m)
{
    float at[4], fwd[4];
    LOS_HIT los;
    int k;

    memcpy(at, pBossInfo + BOSS_INFO_SPECIAL_POS, sizeof(at));
    if (!(tiBossVecDist(at, (const float *)chr->pos) < AXE_RUN_NEAR)) {
        brain[BRAIN_ATK_PICK] = BOSS_TRACE_STEP_RUN;
        return;
    }
    memcpy(los.seg.from, chr->pos, sizeof(los.seg.from));
    los.seg.from[1] += BOSS_LOS_EYE_Y;
    tiGetCharaDirection(fwd, chr);
    for (k = 0; k < 3; k++)             /* vadd.xyz: the ROM leaves to's w unset */
        los.seg.to[k] = los.seg.from[k] + fwd[k] * AXE_RUN_LOOK;
    los.seg.to[3] = los.seg.from[3];
    if (CheckLOS(chr->lightBank, &los) != 0)
        brain[BRAIN_ATK_PICK] = BOSS_TRACE_STEP_RUN;
    ssAI_CompCharaDir(chr, fwd, tiBossTurnSpeed(brain), 1);
    if (m != tiBossBrainShort(brain, BRAIN_ATTACK_MOTION))
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBossBrainShort(brain, BRAIN_ATTACK_MOTION));
}

// The turn step: Alyssa behind (facing . to-her below AXE_TURN_BACK_DOT) the turn-about motion; off line,
// a turn; in line (AXE_TURN_AIM_DOT), the strike -- with BOSS_STATE_BIT_04, the voiced one at odds of 10
// by hp -- then the search motion; BOSS_STATE_SPECIAL_ON marks it.
static void tiAxeSpecialAim(CHARACTER *chr, u_char *brain)
{
    float dir[4], fwd[4], dot, hp;
    int r, odds, motion = AXE_MOTION_SPECIAL_HIT;

    (void)tiBossToPlayer(chr, dir);
    tiGetCharaDirection(fwd, chr);
    dot = gaInnerProductXYZ(dir, fwd);
    if (dot < AXE_TURN_AIM_DOT) {
        if (dot < AXE_TURN_BACK_DOT)
            asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, BOSS_AXE_MOTION_BACKSTEP);
        else
            ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 0);
        return;
    }
    r = tnRand(10);
    if ((tiBossState() & BOSS_STATE_BIT_04) != 0) {
        hp = tiCalcHPRate(chr);
        odds = !(hp <= 0.9f) ? 0 : !(hp <= 0.8f) ? 2 : !(hp <= BOSS_HP_RATE_HIGH) ? 5 : !(hp <= BOSS_HP_RATE_MID) ? 5 : 7;
        if (r < odds) {
            tiBossSetSE(brain, SSPlaySE(AXE_VOICE_BANK, AXE_VOICE_SPECIAL, BOSS_HAMM_SE_ARG));
            motion = AXE_MOTION_SPECIAL_HIT_VOICED;
        }
    }
    tiBalowsPlay(chr, motion, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
    tiBossSetBits(BOSS_STATE_SPECIAL_ON);
}

// CT3 0x0186c540: the AXE's trace (its special). In a strike (0x2b, 0x2f, 0x37) it keeps facing Alyssa
// while BOSS_STATE_AXE_SPECIAL holds; TRIG_VAR_BALOWS_52 1 (the scene's) ends the special and resets its
// counters; the trace may end. The turn-about motion waits. Else the run, then the aim.
void tiBrain_Trace_BOSS_AXE(CHARACTER *chr)
{
    // callcheck: CheckLOS, tiGetCharaDirection (tiAxeSpecialRun makes them)
    // callcheck: SSPlaySE, asChangeMortionInfo_NextMortion, gaInnerProductXYZ, tiCalcHPRate, tnRand (tiAxeSpecialAim)
    // callcheck: asCharacterMortionChangeRequest (tiAxeSpecialRun / tiAxeSpecialAim)
    u_char *brain = CharaBrainPtr(chr);
    short m = tiBossMotion(chr);
    float dir[4];

    if (m == AXE_MOTION_SPECIAL_HIT_VOICED || m == AXE_MOTION_SPECIAL_HIT || m == AXE_MOTION_SPECIAL_2B) {
        (void)tiBossToPlayer(chr, dir);
        if ((tiBossState() & BOSS_STATE_AXE_SPECIAL) != 0)
            ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
        if (ssTrigGetVariable(TRIG_VAR_BALOWS_52, TRIG_GLOBAL, 0) == AXE_VAR_52_END) {
            ssTrigSetVariable(TRIG_VAR_BALOWS_52, -1, TRIG_GLOBAL, 0);
            tiBossClearBits(BOSS_STATE_AXE_SPECIAL);
            tiBossSetInt(BOSS_INFO_SPECIAL_COUNT, 0);
            tiBossSetInt(BOSS_INFO_HITS_70, 0);
            pBossInfo[BOSS_INFO_COUNT_8D] = 0;
            tiBossSetInt(BOSS_INFO_AXE_88, 0);
        }
        brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
        return;
    }
    if (m == BOSS_AXE_MOTION_BACKSTEP)
        return;
    if (brain[BRAIN_ATK_PICK] == BOSS_TRACE_STEP_RUN)
        tiAxeSpecialAim(chr, brain);
    else if (brain[BRAIN_ATK_PICK] == BOSS_TRACE_STEP_START)
        tiAxeSpecialRun(chr, brain, m);
}

// ---- slice 4: the AXE's attack ----

static const short AxeAttackBodyBusy[] = {
    AXE_MOTION_DOWN_END, AXE_MOTION_DOWN_HIT, AXE_MOTION_DOWN, AXE_MOTION_ATK_1_END, AXE_MOTION_ATK_1_HIT,
    AXE_MOTION_ATK_1, AXE_MOTION_ATK_0_END, AXE_MOTION_ATK_0_HIT, AXE_MOTION_ATK_0
};

// CT3 0x0186cac0: the AXE's attack, while an attack is picked. A strike's end (missed or landed) returns to
// the search motion and ends the attack; a strike turns on its timer and on a landed blow queues its
// landed end. Else: Alyssa down, state 3's strike; standing, state 0's or state 1's at even odds (the
// draw made either way), each within its angle.
void tiBrain_Attack_BOSS_AXE(CHARACTER *chr)
{
    // callcheck: asCharacterMortionChangeRequest, ssAI_CompCharaDir, ssAI_IsTrgWithinAngle (tiBossStrikeStart / tiScissorsWStrike / tiBalowsAngleOk)
    u_char *brain = CharaBrainPtr(chr);
    short m = tiBossMotion(chr);
    float dir[4];
    int r;

    if (brain[BRAIN_ATK_PICK] == BRAIN_ATK_NONE)
        return;
    (void)tiBossToPlayer(chr, dir);
    if (TI_BOSS_BUSY(chr, AxeAttackBodyBusy)) {
        switch (m) {
        case AXE_MOTION_DOWN:
            tiScissorsWStrike(chr, brain, dir, AXE_ATK_DOWN, m, AXE_MOTION_DOWN_HIT);
            return;
        case AXE_MOTION_ATK_1:
            tiScissorsWStrike(chr, brain, dir, AXE_ATK_1, m, AXE_MOTION_ATK_1_HIT);
            return;
        case AXE_MOTION_ATK_0:
            tiScissorsWStrike(chr, brain, dir, AXE_ATK_0, m, AXE_MOTION_ATK_0_HIT);
            return;
        default:                        /* the six end motions */
            asChangeMortionInfo_NextMortion(chr->id, m, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
            brain[BRAIN_SUBMODE] = BOSS_SUB_NONE;
            brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
            return;
        }
    }
    r = tnRand(16);
    if (ssAI_IsDownPlaying(&Character[Player_BankID]) != 0) {
        if (tiBalowsAngleOk(brain, AXE_ATK_DOWN, dir, chr))
            tiBossStrikeStart(chr, brain, AXE_ATK_DOWN, AXE_MOTION_DOWN, AXE_MOTION_DOWN_END);
    } else if (r < 8) {
        if (tiBalowsAngleOk(brain, AXE_ATK_0, dir, chr))
            tiBossStrikeStart(chr, brain, AXE_ATK_0, AXE_MOTION_ATK_0, AXE_MOTION_ATK_0_END);
    } else {
        if (tiBalowsAngleOk(brain, AXE_ATK_1, dir, chr))
            tiBossStrikeStart(chr, brain, AXE_ATK_1, AXE_MOTION_ATK_1, AXE_MOTION_ATK_1_END);
    }
}

// ---- slice 4: VITRIOL's trace (its special: the acid sprays) ----

static const short VitriolTraceBusy[] = {
    VITRIOL_MOTION_ROAR, VITRIOL_MOTION_SPRAY_HIT, VITRIOL_MOTION_RETREAT, VITRIOL_MOTION_SPRAY
};

// A spray: its motion, then the attack motion; the next step; state VITRIOL_ATK_SPRAY's timer and
// BOSS_INFO_APPROACH_84 start.
static void tiVitriolSpray(CHARACTER *chr, u_char *brain)
{
    tiBalowsPlay(chr, VITRIOL_MOTION_SPRAY, tiBossBrainShort(brain, BRAIN_ATTACK_MOTION));
    brain[BRAIN_ATK_PICK]++;
    tiBalowsStartTimed(brain, VITRIOL_ATK_SPRAY);
    tiBossClearBits(BOSS_STATE_BALOWS_FOLLOWED);
}

// Chasing: the attack motion (when another plays) and a turn toward Alyssa around what is in the way.
static void tiVitriolChase(CHARACTER *chr, u_char *brain, float *dir, short m)
{
    if (m != tiBossBrainShort(brain, BRAIN_ATTACK_MOTION))
        asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBossBrainShort(brain, BRAIN_ATTACK_MOTION));
    ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), tiBossAvoid(dir, chr));
}

// Alyssa within VITRIOL_SPRAY_REACH, straight ahead (VITRIOL_SPRAY_DOT) and in sight (eye to eye).
static int tiVitriolInLine(CHARACTER *chr, float *dir, float dist)
{
    float fwd[4], from[4], to[4];

    tiGetCharaDirection(fwd, chr);
    memcpy(from, chr->pos, sizeof(from));
    memcpy(to, Character[Player_BankID].pos, sizeof(to));
    if (VITRIOL_SPRAY_REACH < dist || gaInnerProductXYZ(dir, fwd) < VITRIOL_SPRAY_DOT)
        return 0;
    return tiBossLos(chr, from, to) == 0;
}

// The spray under way: before a blow landed its timer (limit BOSS_INFO_VITRIOL_TURN[step]) turns it; a
// miss returns to the attack motion; a hit plays the hit motion, then the search motion (step done).
static void tiVitriolSprayRun(CHARACTER *chr, u_char *brain, float *dir)
{
    int off = BRAIN_ATK_STATES + VITRIOL_ATK_SPRAY * (int)sizeof(BRAIN_ATK_STATE);
    int limit, landed;

    if ((tiBossState() & BOSS_STATE_BALOWS_FOLLOWED) != 0)
        return;
    tiBrainSetInt(brain, off, tiBrainInt(brain, off) + gNextTicksInterval);
    memcpy(&limit, pBossInfo + BOSS_INFO_VITRIOL_TURN + (s_char)brain[BRAIN_ATK_PICK] * 4, sizeof(limit));
    if (tiBrainInt(brain, off) < limit)
        ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
    memcpy(&landed, (const u_char *)chr + CHARA_HIT_LANDED, sizeof(landed));
    if (landed == 0) {
        asChangeMortionInfo_NextMortion(chr->id, VITRIOL_MOTION_SPRAY, tiBossBrainShort(brain, BRAIN_ATTACK_MOTION), -1);
        return;
    }
    tiBossSetBits(BOSS_STATE_BALOWS_FOLLOWED);
    asChangeMortionInfo_NextMortion(chr->id, VITRIOL_MOTION_SPRAY, VITRIOL_MOTION_SPRAY_HIT, -1);
    asChangeMortionInfo_NextMortion(chr->id, VITRIOL_MOTION_SPRAY_HIT, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION), -1);
    brain[BRAIN_ATK_PICK] = VITRIOL_STEP_DONE;
}

// CT3 0x0186cf80: VITRIOL's trace. Step 0 turns to Alyssa and roars (VITRIOL_MOTION_ROAR, B_2D01 10,
// BOSS_INFO_HAMM_94 = VITRIOL_94_ROAR); steps 1..3 chase her and spray when she is in line (or nearer
// than VITRIOL_SPRAY_NEAR), BOSS_INFO_HAMM_94 back from 0x95; steps 4 and 5 spray in line, though at step
// 4 only on 5 of 10 below half hp -- otherwise the retreat ends it (VITRIOL_STEP_DONE, then none).
void tiBrain_Trace_BOSS_VITRIOL(CHARACTER *chr)
{
    // callcheck: CheckLOS, tiGetCharaDirection, gaInnerProductXYZ (tiVitriolInLine / tiBossLos)
    // callcheck: tiAvoidObject (tiBossAvoid)
    // callcheck: tiCalcPlayerHitPoint (the ROM's unreachable motion-5 case)
    // callcheck: asChangeMortionInfo_NextMortion (tiBalowsPlay / tiVitriolSprayRun)
    u_char *brain = CharaBrainPtr(chr);
    s_char step = (s_char)brain[BRAIN_ATK_PICK];
    short m = tiBossMotion(chr);
    float dir[4], fwd[4];
    float dist;

    if (brain[BRAIN_ATK_PICK] == BRAIN_ATK_NONE)
        return;
    dist = tiBossToPlayer(chr, dir);
    if (TI_BOSS_BUSY(chr, VitriolTraceBusy)) {
        // The ROM has a case for motion 5 here (a landed blow: tiCalcPlayerHitPoint(20), FOLLOWED) that
        // the busy list never lets through.
        if (m == VITRIOL_MOTION_SPRAY_HIT) {
            if (brain[BRAIN_ATK_PICK] != BRAIN_ATK_NONE) {
                tiBossSetSE(brain, SSPlaySE(VITRIOL_SE_BANK, VITRIOL_SE_SPRAY_HIT, BOSS_HAMM_SE_ARG));
                tiBossClearBits(BOSS_STATE_BALOWS_FOLLOWED);
                brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
            }
        } else if (m == VITRIOL_MOTION_RETREAT) {
            brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
            tiBossClearBits(BOSS_STATE_BALOWS_FOLLOWED);
        } else if (m == VITRIOL_MOTION_SPRAY) {
            tiVitriolSprayRun(chr, brain, dir);
        } else {                        /* VITRIOL_MOTION_ROAR */
            ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), 1);
        }
        return;
    }
    if (Character[Player_BankID].hp < 1) {
        if (m != tiBossBrainShort(brain, BRAIN_SEARCH_MOTION))
            asCharacterMortionChangeRequest(chr->id, CHARA_MOTION_REQ_PLAY, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
        return;
    }
    switch (step) {
    case VITRIOL_STEP_DONE:
        brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
        return;
    case 4:
    case 5:
        if (!tiVitriolInLine(chr, dir, dist)) {
            tiVitriolChase(chr, brain, dir, m);
        } else if (step != 4 || !(tiCalcHPRate(chr) < BOSS_HP_RATE_MID) || 4 < tnRand(10)) {
            tiBalowsPlay(chr, VITRIOL_MOTION_RETREAT, tiBossBrainShort(brain, BRAIN_SEARCH_MOTION));
            brain[BRAIN_ATK_PICK] = VITRIOL_STEP_DONE;
        } else {
            tiVitriolSpray(chr, brain);
        }
        return;
    case 1:
    case 2:
    case 3:
        pBossInfo[BOSS_INFO_HAMM_94] = pBossInfo[BOSS_INFO_HAMM_95];
        if (tiVitriolInLine(chr, dir, dist) || dist < VITRIOL_SPRAY_NEAR)
            tiVitriolSpray(chr, brain);
        else
            tiVitriolChase(chr, brain, dir, m);
        return;
    case 0:
        tiGetCharaDirection(fwd, chr);
        if (gaInnerProductXYZ(dir, fwd) < VITRIOL_ROAR_DOT) {
            ssAI_CompCharaDir(chr, dir, tiBossTurnSpeed(brain), tiBossAvoid(dir, chr));
            return;
        }
        tiBalowsPlay(chr, VITRIOL_MOTION_ROAR, tiBossBrainShort(brain, BRAIN_ATTACK_MOTION));
        brain[BRAIN_ATK_PICK] = 1;
        tiBossSetSE(brain, SSPlaySE(VITRIOL_VOICE_BANK, VITRIOL_VOICE_ROAR, BOSS_HAMM_SE_ARG));
        pBossInfo[BOSS_INFO_HAMM_94] = VITRIOL_94_ROAR;
        return;
    default:
        brain[BRAIN_ATK_PICK] = BRAIN_ATK_NONE;
        return;
    }
}

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


// stub of CT3 0x0186e0d0
void tiBrain_Attack_BOSS_HAMM(struct CHARACTER *chr)
{
    ALYSSAPAN_STUB("tiBrain_Attack_BOSS_HAMM", 0x0186e0d0);
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


