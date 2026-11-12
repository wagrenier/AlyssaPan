// game/asUseArrow.c -- Clock Tower 3: Alyssa's bow arrows (symtab local chunk 79). CodeWarrior C.
// Slice 1 (2026-11-11): every function but asArrowOperate; slice 2 (2026-11-11): asArrowOperate.
// The arrows are parts of the bank asXcmd_I_have_x_Arrows armed (ArrowInfo.bank); a shot arrow is placed
// at the bow's nock part of the player's model and points at its aim part (asAdjustArrowDirection).

#include "game/asUseArrow.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ahalloc.h"
#include "game/fileio.h"
#include "game/LinkMatrixTree.h"
#include "game/main2.h"
#include "game/sound.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/tiAvoidObject.h"
#include "game/ct3_types.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ssGrpPortalCheck.h"
#include "game/tiXcmd_TransItem.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaSinCos.h"

#include <math.h>
#include <string.h>

ARROW_INFO ArrowInfo;                   /* sbss 0x0193cea0 */

// The player's bow parts (asFindModelParts hashes of Character[0]'s shape).
#define ARROW_PART_HASH_NOCK 0xc84149031610af72ull  /* where a shot arrow starts */
#define ARROW_PART_HASH_AIM 0xc84131031610af72ull   /* dir = aim - nock while the bow is up */
#define ARROW_HIDDEN_Y -1.0e8f          /* 0xccbebc20: an arrow part's drawOnPos y out of sight */
#define ARROW_MARK_DIST 2000.0f         /* markPart: this far ahead of the nock along dir */
#define ARROW_STEEP_DOT 0.5f            /* asGetTheArrowRad: dir . up from here (30 degrees up) */
// Character[0]'s motion (CHARA_MOTION_SLOTS[0]) while the bow is up (wideXcmd_DebugPrint.c's names).
#define MOTION_BOW_EMPTY 0x35
#define MOTION_BOW_AIM 0x36
#define MOTION_BOW_SHOOT 0x37

static const float ArrowUp[4] = {0.0f, 1.0f, 0.0f, 0.0f};  /* @393 (rodata 0x019385d0) */

static int ArrowIsOut(const ARROW *a)
{
    return a->state == ARROW_STATE_SHOT || a->state == ARROW_STATE_CHARGE;
}

// The player's current motion (Character[0], slot 0).
static short PlayerMotion(void)
{
    short m;
    memcpy(&m, (const u_char *)&Character[0] + CHARA_MOTION_SLOTS, sizeof(m));
    return m;
}

// Part `index` of bank `bank` (MODEL_PARTS is indexed from the header).
static MODEL_PARTS *ArrowBankPart(int bank, int index)
{
    return &((MODEL_PARTS *)pModelPartsHeader[bank])[index];
}

// CT3 0x018665e0: ArrowList loses its first entry (the list shifts down one, the last slot cleared),
// kind `arrow` one held arrow; global event TRIG_EVENT_ARROW_USED.
void asUseArrow(int arrow)
{
    int i;

    ArrowList[0] = 0;
    for (i = 1; i < ARROW_LIST_SLOTS; i++) {
        ArrowList[i - 1] = ArrowList[i];
        ArrowList[i] = 0;
    }
    ArrowData[arrow]--;
    ssTrigRegisterEvent(TRIG_EVENT_ARROW_USED, XCMD_SCOPE_GLOBAL, 0);
}

// CT3 0x018666c0
void asInitArrowInfo(void)
{
    ArrowInfo.list = NULL;
    ArrowInfo.active = 0;
    ArrowInfo.bank = 0;
    ArrowInfo.count = 0;
}

// CT3 0x018666e0
void asDestroyArrowInfo(void)
{
    if (ArrowInfo.list != NULL) {
        _SunFree(&gHeapJunc, (void **)&ArrowInfo.list);
    }
    ArrowInfo.list = NULL;
    ArrowInfo.active = 0;
    ArrowInfo.bank = 0;
    ArrowInfo.count = 0;
}

// CT3 0x01866720: the first arrow out: dir . up (f0); from ARROW_STEEP_DOT the arrow becomes
// ARROW_KIND_STEEP. 0 with no arrow out or none armed.
float asGetTheArrowRad(void)
{
    float dot = 0.0f;
    float up[4];
    int i;

    memcpy(up, ArrowUp, sizeof(up));
    if (ArrowInfo.active) {
        for (i = 0; i < ArrowInfo.count; i++) {
            if (ArrowIsOut(&ArrowInfo.list[i])) {
                dot = gaInnerProductXYZ(up, ArrowInfo.list[i].dir);
                break;
            }
        }
        if (dot >= ARROW_STEEP_DOT) {
            ArrowInfo.list[i].kind = ARROW_KIND_STEEP;
        }
    }
    return dot;
}

// CT3 0x01866810
int asGetTheArrowLevel(void)
{
    int i;

    if (ArrowInfo.active) {
        for (i = 0; i < ArrowInfo.count; i++) {
            if (ArrowIsOut(&ArrowInfo.list[i])) {
                return ArrowInfo.list[i].level;
            }
        }
    }
    return -1;
}

// CT3 0x01866890: the arrow scene hit `id` of ArrowInfo.bank carries (its object's HIT_OBJ_ID).
// PORT: the hit records and their objects are read as EE words of ArrowInfo.bank (BANK_PTR).
ARROW *asAttackedArrowInformation(int id)
{
    int bank = ArrowInfo.bank;
    const HIT_TABLE *hits = BANK_PTR(const HIT_TABLE *, bank, All_table_TBL[bank]->hits);
    int count = hits->objHitCount;
    const HIT_OBJ_REC *recs = BANK_PTR(const HIT_OBJ_REC *, bank, hits->objHits);
    int i;

    for (i = 0; i < count; i++) {
        if (recs[i].id == id) {
            const u_char *obj = BANK_PTR(const u_char *, bank, recs[i].object);
            short hitId;
            memcpy(&hitId, obj + HIT_OBJ_ID, sizeof(hitId));
            return asArrowInformation(hitId);
        }
    }
    return NULL;
}

// CT3 0x01866910
ARROW *asArrowInformation(short id)
{
    u_short i;

    for (i = 0; i < ArrowInfo.count; i++) {
        if (ArrowInfo.list[i].hitId == id) {
            return &ArrowInfo.list[i];
        }
    }
    return NULL;
}

// CT3 0x01866980
int asChangeTheArrowState(int state)
{
    int i;

    if (ArrowInfo.active) {
        for (i = 0; i < ArrowInfo.count; i++) {
            ARROW *a = &ArrowInfo.list[i];
            if (ArrowIsOut(a) && a->state != state) {
                if (state == ARROW_STATE_ASK) {
                    return i;
                }
                a->state = (u_char)state;
                a->counter = 0;
                a->timer = 0;
                return i;
            }
        }
    }
    return -1;
}

// CT3 0x01866a30: point the arrow from the bow's nock: towards the aim part while the bow is up
// (MOTION_BOW_EMPTY..SHOOT), else along the player's facing (row 2 of her Y rotation). markPart goes
// ARROW_MARK_DIST ahead; `carry` also moves the arrow (pos and its part) onto the nock.
void asAdjustArrowDirection(ARROW *arrow, int carry)
{
    u_char *header = pModelPartsHeader[Character[0].bank];
    u_char *rec = asFindModelParts(header, ARROW_PART_HASH_NOCK);
    const MODEL_PARTS *nock;
    MODEL_PARTS *part;
    short motion;
    int k;

    if (rec == NULL) {
        return;
    }
    nock = (const MODEL_PARTS *)(rec - PARTS_HDR_RECORDS);
    motion = PlayerMotion();
    if (motion == MOTION_BOW_EMPTY || motion == MOTION_BOW_AIM || motion == MOTION_BOW_SHOOT) {
        // PORT: the ROM uses the aim part's lookup unchecked.
        u_char *aimRec = asFindModelParts(header, ARROW_PART_HASH_AIM);
        const MODEL_PARTS *aim = aimRec != NULL ? (const MODEL_PARTS *)(aimRec - PARTS_HDR_RECORDS) : nock;
        float len2, q;
        for (k = 0; k < 4; k++) {
            arrow->dir[k] = aim->pos[k] - nock->pos[k];
        }
        len2 = arrow->dir[0] * arrow->dir[0] + arrow->dir[1] * arrow->dir[1] + arrow->dir[2] * arrow->dir[2];
        q = 1.0f / sqrtf(len2);        /* vrsqrt Q, vf0w, len2 */
        for (k = 0; k < 3; k++) {
            arrow->dir[k] *= q;
        }
    } else {
        float m[4][4];
        gaSetRotMatrixY(m, Character[0].rot);
        memcpy(arrow->dir, m[2], sizeof(arrow->dir));
    }
    arrow->dir[3] = 1.0f;

    part = ArrowBankPart(ArrowInfo.bank, arrow->markPart);
    for (k = 0; k < 3; k++) {
        part->drawOnPos[k] = nock->pos[k] + arrow->dir[k] * ARROW_MARK_DIST;
    }
    part->drawOnPos[3] = nock->pos[3];
    part->flags43 = PARTS_FLAG43_WARPED;

    if (carry) {
        memcpy(arrow->pos, nock->pos, sizeof(arrow->pos));
        part = ArrowBankPart(ArrowInfo.bank, arrow->part);
        part->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
        memcpy(part->drawOnPos, arrow->pos, sizeof(part->drawOnPos));
    }
}

// CT3 0x01866c10: the first ready arrow is shot: kind, the arguments' low bytes, its part at the nock
// but out of sight (ARROW_HIDDEN_Y), then aimed. -1 with no ready arrow, or (the slot already taken)
// when the player's model has no nock.
int asShootArrowSetting(u_char kind, u_short a1, u_short a2)
{
    ARROW *a;
    MODEL_PARTS *part;
    u_char *rec;
    int i;

    if (!ArrowInfo.active) {
        return -1;
    }
    for (i = 0; i < ArrowInfo.count && ArrowInfo.list[i].state != ARROW_STATE_READY; i++) {
    }
    if (i == ArrowInfo.count) {
        return -1;
    }
    a = &ArrowInfo.list[i];
    a->state = ARROW_STATE_SHOT;
    a->kind = kind;
    a->shootArg1 = a1 & 0xff;
    a->shootArg2 = a2 & 0xff;
    a->counter = 0;
    a->timer = 0;
    a->level = 0;
    rec = asFindModelParts(pModelPartsHeader[Character[0].bank], ARROW_PART_HASH_NOCK);
    if (rec == NULL) {
        return -1;
    }
    part = ArrowBankPart(ArrowInfo.bank, a->part);
    part->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
    memcpy(part->drawOnPos, ((const MODEL_PARTS *)(rec - PARTS_HDR_RECORDS))->pos, sizeof(part->drawOnPos));
    part->drawOnPos[1] = ARROW_HIDDEN_Y;
    asAdjustArrowDirection(a, 0);
    return i;
}

// ---- asArrowOperate (slice 2, 2026-11-11) ----------------------------------------------------------------

// Character[0]'s motions around the bow beyond the three above (wideXcmd_DebugPrint.c's names).
#define MOTION_BOW_RAISE 0x56
#define MOTION_BOW_DRAW 0x57
#define MOTION_BOW_RAISE2 0x58
#define MOTION_BOW_HOLD 0x5d

// The arrow sounds: bank "T_0010" (0x01934030 in this TU's .rodata), ids by the arrow's level; the
// channel argument is the arrow's index + 1.
#define SE_BANK_ARROW "T_0010"
#define SE_ARROW_DRAW 5                 /* ARROW_STATE_SHOT's first frame; stopped on leaving the bow */
#define SE_ARROW_SHOT_WEAK 7            /* the release: level 0..1 */
#define SE_ARROW_SHOT_MID 8             /* level 2 */
#define SE_ARROW_SHOT_STRONG 9          /* level 3..4 */
#define SE_ARROW_SHOT_MAX 10            /* above */
#define SE_ARROW_HIT_WEAK 0xb           /* stuck in a plane: level 0..1 */
#define SE_ARROW_HIT_MID 0xc            /* level 2..3 */
#define SE_ARROW_HIT_STRONG 0xd         /* above */
#define SE_ONO_ARROW_LANDED 8           /* SE_BANK_E_ONO (this TU's copy at 0x01934038) on stage 3 */

// Trigger variables and events (global scope, bank 0).
#define TRIG_VAR_BOW_53 0x53            /* wideXcmd_DebugPrint.c's name: the arrow pointing up sets it 1 */
#define TRIG_VAR_ARROW_LEVEL 3          /* asGetTheArrowLevel's answer each frame (0 with none) */
#define TRIG_VAR_ARROW_STATES 0x33      /* 4 bits an arrow (index * 4): the state entered this frame */
#define TRIG_VAR_ARROW_LEVELS 0x35      /* 4 bits an arrow: its level then (ARROW_NIBBLE_KIND1 / _KIND2) */
#define TRIG_EVENT_ARROW_LANDED 0x65    /* an arrow stopped with ARROW_HIT_END */
#define ARROW_NIBBLE_KIND1 9            /* TRIG_VAR_ARROW_LEVELS for a kind 1 arrow (BOW_ARROW_LIST) */
#define ARROW_NIBBLE_KIND2 10           /* for kind 2 */
#define AXE_STAGE 3                     /* TRIG_VAR_STAGE 3: an arrow pointing up resets the axe throw AI */

#define ARROW_STUCK_BACK 80.0f          /* a stuck arrow's part: this far back from the plane along dir */
#define ARROW_TICKS_PER_STEP 5.0f       /* the flight: dir * speed * ticks / 5 a frame */
#define ARROW_CHARGE_FIRST 0x95         /* ARROW_STATE_CHARGE: level 0 -> 1 and 1 -> 2 past this many ticks */
#define ARROW_CHARGE_NEXT 299           /* 2 -> 3, 3 -> 4, 4 -> 5 past this many */
#define ARROW_LEVEL_MAX 5

// The release scales the speed and power by the kind (kinds 1 and 2) or by the level (kind 0).
#define ARROW_SPEED_STEEP 40            /* kind 3 (ARROW_KIND_STEEP) */
#define ARROW_POWER_STEEP 10
#define ARROW_SPEED_KIND2 30
#define ARROW_POWER_KIND2 90
#define ARROW_SPEED_KIND1 22
#define ARROW_POWER_KIND1 180
static const u_char ArrowLevelSpeed[ARROW_LEVEL_MAX + 1] = {10, 12, 15, 20, 25, 30};
static const u_char ArrowLevelPower[ARROW_LEVEL_MAX + 1] = {10, 15, 30, 45, 60, 90};
// @61 (rodata 0x019385a0): {10, 20, 40, 80, 120, 180} is copied to the stack and read only by a block
// the compiler left unreachable. @62 (0x019385c0) is ArrowUp again.

// The attack each stage's arrow lands on the target (TRIG_VAR_STAGE; 4 lands two).
#define ARROW_ATTACK_STAGE5 0x20
#define ARROW_ATTACK_STAGE4_A 5
#define ARROW_ATTACK_STAGE4_B 6
#define ARROW_ATTACK_STAGE3 4
#define ARROW_ATTACK_STAGE2 3
#define ARROW_ATTACK_STAGE1 2
#define ARROW_ATTACK_STAGE3_BANK 0x12   /* stage 3's last argument (the others -1) */

static void ArrowEnter(ARROW *a, int state)
{
    a->state = (u_char)state;
    a->counter = 0;
    a->timer = 0;
}

static void ArrowSound(int id, int i)
{
    SSPlaySE(SE_BANK_ARROW, id, i + 1);
}

// The EE record word pSrcMPstock / pDstMPstock holds (tiXcmd_CheckRange: header + 0x40 + i * 0x380) as
// its part's position. PORT: the word carries no bank; the bank is the loaded image it falls in.
static const float *ArrowStockPos(u_int ee)
{
    int bank, best = -1;

    for (bank = 0; bank < BANK_MAX; bank++) {
        if (BankImage[bank] != NULL && BankImageEE[bank] <= ee &&
            (best < 0 || BankImageEE[bank] > BankImageEE[best])) {
            best = bank;
        }
    }
    if (best < 0) {
        return NULL;
    }
    return ((const MODEL_PARTS *)((u_char *)BankEePtr(best, ee) - PARTS_HDR_RECORDS))->pos;
}

// The release's scaling and sound (ARROW_STATE_FLYING's first frame); its level nibble.
static u_int ArrowRelease(ARROW *a)
{
    u_int nibble = 0;

    a->hitKind = 0;
    switch (a->kind) {
    case ARROW_KIND_STEEP:
        nibble = a->level;
        a->shootArg1 *= ARROW_SPEED_STEEP;
        a->shootArg2 *= ARROW_POWER_STEEP;
        break;
    case 2:
        nibble = ARROW_NIBBLE_KIND2;
        a->shootArg1 *= ARROW_SPEED_KIND2;
        a->shootArg2 *= ARROW_POWER_KIND2;
        asUseArrow(a->kind);
        break;
    case 1:
        nibble = ARROW_NIBBLE_KIND1;
        a->shootArg1 *= ARROW_SPEED_KIND1;
        a->shootArg2 *= ARROW_POWER_KIND1;
        asUseArrow(a->kind);
        break;
    case 0:
        nibble = a->level;
        if (a->level <= ARROW_LEVEL_MAX) {
            a->shootArg1 *= ArrowLevelSpeed[a->level];
            a->shootArg2 *= ArrowLevelPower[a->level];
        }
        break;
    }
    return nibble;
}

static void ArrowReleaseSound(const ARROW *a, int i)
{
    switch (a->level) {
    case 0:
    case 1:
        ArrowSound(SE_ARROW_SHOT_WEAK, i);
        break;
    case 2:
        ArrowSound(SE_ARROW_SHOT_MID, i);
        break;
    case 3:
    case 4:
        ArrowSound(SE_ARROW_SHOT_STRONG, i);
        break;
    default:
        ArrowSound(SE_ARROW_SHOT_MAX, i);
        break;
    }
}

// The level nibble of a kind (HIT, CHARGE): kinds 1 / 2 their fixed values, else the level.
static int ArrowKindNibble(const ARROW *a, u_int *nibble)
{
    switch (a->kind) {
    case 2:
        *nibble = ARROW_NIBBLE_KIND2;
        return 1;
    case 1:
        *nibble = ARROW_NIBBLE_KIND1;
        return 1;
    case 0:
        *nibble = a->level;
        return 1;
    }
    return 0;
}

// The flight's pass by the TransItem stock parts (tiXcmd_CheckRange's pair): a plain arrow of level < 2
// that crossed the source's ring (XZ distances: nearer to it than the destination is now, not before)
// turns from where it was, away from the source, at twice the speed; its mark goes ARROW_MARK_DIST ahead.
static void ArrowStockTurn(ARROW *a)
{
    const float *src = ArrowStockPos(pSrcMPstock);
    const float *dst = ArrowStockPos(pDstMPstock);
    float s[4], d[4], p[4], q[4];
    float range, now, before, len2, inv, speed;
    MODEL_PARTS *mark;
    int k;

    if (src == NULL || dst == NULL) {
        return;
    }
    memcpy(s, src, sizeof(s));
    memcpy(d, dst, sizeof(d));
    memcpy(p, a->pos, sizeof(p));
    memcpy(q, a->posPrev, sizeof(q));
    s[1] = d[1] = p[1] = q[1] = 0.0f;
    range = asVectorDistance2(s, d);
    now = asVectorDistance2(s, p);
    before = asVectorDistance2(s, q);
    if (!(now < range && range <= before)) {
        return;
    }
    for (k = 0; k < 4; k++) {
        a->dir[k] = a->posPrev[k] - src[k];       /* vsub.xyzw */
    }
    len2 = a->dir[0] * a->dir[0] + a->dir[1] * a->dir[1] + a->dir[2] * a->dir[2];
    inv = 1.0f / sqrtf(len2);
    for (k = 0; k < 3; k++) {
        a->dir[k] *= inv;                         /* vmulq.xyz: w keeps the difference */
    }
    speed = (float)a->shootArg1 * 2.0f;
    a->shootArg1 = (u_short)(u_int)speed;         /* the ROM's float -> unsigned conversion */
    memcpy(a->pos, a->posPrev, sizeof(a->pos));
    mark = ArrowBankPart(ArrowInfo.bank, a->markPart);
    for (k = 0; k < 4; k++) {
        mark->pos[k] = a->posPrev[k] + a->dir[k] * ARROW_MARK_DIST;
    }
    mark->pos[3] = 1.0f;
    memcpy(mark->drawOnPos, mark->pos, sizeof(mark->drawOnPos));
}

// The attack a flying arrow's object lands on `target` (the first live character) when the arrow
// bank's attacker list holds its object.
static void ArrowAttack(const ARROW *a, int target)
{
    int bank = ArrowInfo.bank;
    const HIT_TABLE *hits = BANK_PTR(const HIT_TABLE *, bank, All_table_TBL[bank]->hits);
    const u_int *objs = BANK_PTR(const u_int *, bank, hits->attackObjs);
    int count = hits->attackObjCount;
    int i;

    for (i = 0; i < count; i++) {
        const u_char *obj = BANK_PTR(const u_char *, bank, objs[i]);
        short id;
        memcpy(&id, obj + HIT_OBJ_ID, sizeof(id));
        if ((int)(u_short)a->hitId != (int)id) {   /* the ROM: zero- against sign-extended */
            continue;
        }
        switch (ssTrigGetVariable(TRIG_VAR_STAGE, XCMD_SCOPE_GLOBAL, -1)) {
        case 5:
            ssHitAddAttackRequest(i, bank, 1, ARROW_ATTACK_STAGE5, target, 0, 0, -1);
            break;
        case 4:
            ssHitAddAttackRequest(i, bank, 1, ARROW_ATTACK_STAGE4_A, target, 0, 0, -1);
            ssHitAddAttackRequest(i, bank, 1, ARROW_ATTACK_STAGE4_B, target, 0, 0, -1);
            break;
        case 3:
            ssHitAddAttackRequest(i, bank, 1, ARROW_ATTACK_STAGE3, target, 1, 0, ARROW_ATTACK_STAGE3_BANK);
            break;
        case 2:
            ssHitAddAttackRequest(i, bank, 1, ARROW_ATTACK_STAGE2, target, 0, 0, -1);
            break;
        case 1:
            ssHitAddAttackRequest(i, bank, 1, ARROW_ATTACK_STAGE1, target, 0, 0, -1);
            break;
        }
    }
}

// This frame's scene hits of the arrow bank: an arrow whose object was struck stops (ARROW_STATE_HIT);
// a kind 0 record leaves it stuck ARROW_STUCK_BACK short of the plane of the character it hit.
static void ArrowSceneHits(void)
{
    int bank = ArrowInfo.bank;
    const HIT_TABLE *hits = BANK_PTR(const HIT_TABLE *, bank, All_table_TBL[bank]->hits);
    int count = hits->objHitCount;
    const HIT_OBJ_REC *recs = BANK_PTR(const HIT_OBJ_REC *, bank, hits->objHits);
    int r, i, k;

    for (r = 0; r < count; r++) {
        const u_char *obj = BANK_PTR(const u_char *, bank, recs[r].object);
        short id;
        memcpy(&id, obj + HIT_OBJ_ID, sizeof(id));
        for (i = 0; i < ArrowInfo.count; i++) {
            ARROW *a = &ArrowInfo.list[i];
            if ((int)(u_short)a->hitId != (int)id) {   /* zero- against sign-extended, as the ROM */
                continue;
            }
            a->state = ARROW_STATE_HIT;
            a->timer = 1;
            if (recs[r].kind == 1) {
                a->hitKind = ARROW_HIT_END;
            } else if (recs[r].kind == 0) {
                MODEL_PARTS *part = ArrowBankPart(bank, a->part);
                float cross[4];
                a->hitKind = ARROW_HIT_STUCK;
                asGetCrossPointInPlaneWithLine(cross, a->posPrev, part->drawOnPos, a->dir,
                                               (float *)&Character[recs[r].chara].pos);
                for (k = 0; k < 3; k++) {
                    part->drawOnPos[k] = cross[k] - a->dir[k] * ARROW_STUCK_BACK;  /* vsub.xyz */
                }
                part->drawOnPos[3] = cross[3];
                part->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
                switch (a->level) {
                case 0:
                case 1:
                    ArrowSound(SE_ARROW_HIT_WEAK, i);
                    break;
                case 2:
                case 3:
                    ArrowSound(SE_ARROW_HIT_MID, i);
                    break;
                default:
                    ArrowSound(SE_ARROW_HIT_STRONG, i);
                    break;
                }
            }
        }
    }
}

// CT3 0x01866d80: every frame from asCharacterOperate's player pass. While the arrow bank still holds
// the file it was armed with (else the arrows are dropped): the scene hits, then each arrow's state --
// on the bow (SHOT / CHARGE, following Character[0]'s bow motion), FLYING (moved, attacks, stops past
// its mark), HIT, RETURN -- and the global variables the scripts read: the states entered this frame
// and their levels (a nibble an arrow), the out arrow's level.
void asArrowOperate(void)
{
    short motion = PlayerMotion();
    const BANK_INFO *bi;
    u_int states = 0, levels = 0;
    int target, i, k, level;

    if (!ArrowInfo.active) {
        return;
    }
    bi = &BankInfo[ArrowInfo.bank];
    if (!(bi->drawOn == DRAW_BANK_ON || bi->state == BANK_STATE_LOADED) ||
        strcmp(bi->name, ArrowInfo.bankName) != 0) {
        asDestroyArrowInfo();
        return;
    }

    // The target: the first character past Alyssa that casts a shadow (is live) and may be hit.
    for (target = 1; target < CHARACTER_MAX; target++) {
        u_int f = Character[target].flags;
        if ((f & CHARACTER_FLAG_SHADOW) && !(f & CHARACTER_FLAG_NO_HIT)) {
            break;
        }
    }

    ArrowSceneHits();

    for (i = 0; i < ArrowInfo.count; i++) {
        ARROW *a = &ArrowInfo.list[i];
        u_int shift = (u_int)(i & 7) * 4;
        u_int nibble;

        memcpy(a->posPrev, a->pos, sizeof(a->posPrev));
        switch (a->state) {
        case ARROW_STATE_SHOT:
            asAdjustArrowDirection(a, 1);
            if (a->timer == 0) {
                a->counter = 0;
                states |= (u_int)ARROW_STATE_SHOT << shift;
            }
            if (a->timer == 1) {
                ArrowSound(SE_ARROW_DRAW, i);
            }
            if (motion == MOTION_BOW_SHOOT) {
                ArrowEnter(a, ARROW_STATE_FLYING);
                SSStopSE(SE_BANK_ARROW, SE_ARROW_DRAW, i + 1);
            } else if (motion == MOTION_BOW_AIM || motion == MOTION_BOW_EMPTY) {
                /* stays on the bow */
            } else if (motion == MOTION_BOW_DRAW) {
                // The draw starts at a level fixed by the kind (kind 3: stays on the bow).
                static const u_char start[3] = {0, 2, ARROW_LEVEL_MAX};
                if (a->kind <= 2) {
                    ArrowEnter(a, ARROW_STATE_CHARGE);
                    a->level = start[a->kind];
                }
            } else if (motion != MOTION_BOW_RAISE2 && motion != MOTION_BOW_RAISE) {
                ArrowEnter(a, ARROW_STATE_RETURN);  /* the ROM's else (FLYING) cannot be taken here */
                SSStopSE(SE_BANK_ARROW, SE_ARROW_DRAW, i + 1);
            }
            break;

        case ARROW_STATE_CHARGE:
            asAdjustArrowDirection(a, 1);
            if (a->timer == 1 && ArrowKindNibble(a, &nibble)) {
                states |= (u_int)ARROW_STATE_CHARGE << shift;
                levels |= nibble << shift;
            }
            if (motion == MOTION_BOW_DRAW) {
                if (a->kind == 0 && a->level < ARROW_LEVEL_MAX &&
                    a->counter > (a->level < 2 ? ARROW_CHARGE_FIRST : ARROW_CHARGE_NEXT)) {
                    a->counter = 0;
                    a->timer = 0;
                    a->level++;
                }
            } else {
                ArrowEnter(a, motion == MOTION_BOW_HOLD ? ARROW_STATE_FLYING : ARROW_STATE_RETURN);
                SSStopSE(SE_BANK_ARROW, SE_ARROW_DRAW, i + 1);
            }
            break;

        case ARROW_STATE_FLYING: {
            float up[4], target3[4], step[4], d[4];
            float t, before, after;
            MODEL_PARTS *part;

            if (a->timer == 1) {
                states |= (u_int)ARROW_STATE_FLYING << shift;
                levels |= ArrowRelease(a) << shift;
                ArrowReleaseSound(a, i);
            }
            memcpy(up, ArrowUp, sizeof(up));                       /* @62 */
            if (gaInnerProductXYZ(up, a->dir) >= ARROW_STEEP_DOT &&
                ssTrigGetVariable(TRIG_VAR_BOW_53, XCMD_SCOPE_GLOBAL, 0) == 0) {
                ssTrigSetVariable(TRIG_VAR_BOW_53, 1, XCMD_SCOPE_GLOBAL, 0);
                if (ssTrigGetVariable(TRIG_VAR_STAGE, XCMD_SCOPE_GLOBAL, 0) == AXE_STAGE) {
                    ssAI_AxeThrowReset();
                }
            }
            memcpy(target3, ArrowBankPart(ArrowInfo.bank, a->markPart)->pos, sizeof(target3));
            t = gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / ARROW_TICKS_PER_STEP;
            for (k = 0; k < 4; k++) {
                step[k] = a->dir[k] * ((float)a->shootArg1 * t);
                d[k] = target3[k] - a->pos[k];
            }
            before = gaInnerProductXYZ(d, d);
            for (k = 0; k < 3; k++) {
                a->pos[k] += step[k];                               /* vadd.xyz */
            }
            for (k = 0; k < 4; k++) {
                d[k] = target3[k] - a->pos[k];
            }
            after = gaInnerProductXYZ(d, d);
            part = ArrowBankPart(ArrowInfo.bank, a->part);
            part->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
            memcpy(part->drawOnPos, a->pos, sizeof(part->drawOnPos));
            if (before < after) {                                   /* past the mark */
                ArrowEnter(a, ARROW_STATE_HIT);
                a->hitKind = ARROW_HIT_END;
            }
            if (pSrcMPstock != 0 && pDstMPstock != 0 && a->kind == 0 && a->level < 2) {
                ArrowStockTurn(a);
            }
            if (a->state != ARROW_STATE_HIT && target != CHARACTER_MAX) {
                ArrowAttack(a, target);
            }
            break;
        }

        case ARROW_STATE_HIT:
            if (a->timer == 1) {
                states |= (u_int)ARROW_STATE_HIT << shift;
                if (a->kind == ARROW_KIND_STEEP) {
                    levels |= (u_int)a->level << shift;
                } else if (ArrowKindNibble(a, &nibble)) {
                    levels |= nibble << shift;
                }
                if (a->hitKind != 0) {
                    if (a->hitKind == ARROW_HIT_END && a->kind != ARROW_KIND_STEEP) {
                        ssTrigRegisterEvent(TRIG_EVENT_ARROW_LANDED, XCMD_SCOPE_GLOBAL, 0);
                        if (ssTrigGetVariable(TRIG_VAR_STAGE, XCMD_SCOPE_GLOBAL, -1) == AXE_STAGE) {
                            SSPlaySE(SE_BANK_E_ONO, SE_ONO_ARROW_LANDED, i + 1);
                        }
                    }
                    a->hitKind = 0;
                }
            }
            if ((u_short)a->timer > ARROW_HIT_LINGER) {
                ArrowEnter(a, ARROW_STATE_RETURN);
            }
            break;

        case ARROW_STATE_RETURN:
            if (a->timer == 1) {
                MODEL_PARTS *part = ArrowBankPart(ArrowInfo.bank, a->part);
                states |= (u_int)ARROW_STATE_RETURN << shift;
                part->drawOnPos[1] = ARROW_HIDDEN_Y;
                part->flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
            }
            ArrowEnter(a, ARROW_STATE_READY);
            break;
        }
        a->counter += gGlobalNextTicksInterval;
        a->timer++;
        if (a->timer == -1) {
            a->timer = -2;                  /* the timer stops at 0xfffe */
        }
    }

    ssTrigSetVariable(TRIG_VAR_ARROW_STATES, (int)states, XCMD_SCOPE_GLOBAL, 0);
    ssTrigSetVariable(TRIG_VAR_ARROW_LEVELS, (int)levels, XCMD_SCOPE_GLOBAL, 0);
    level = asGetTheArrowLevel();
    if (level < 0) {
        level = 0;
    }
    ssTrigSetVariable(TRIG_VAR_ARROW_LEVEL, level, XCMD_SCOPE_GLOBAL, 0);
}
