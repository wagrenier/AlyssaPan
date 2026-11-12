// game/asUseArrow.c -- Clock Tower 3: Alyssa's bow arrows (symtab local chunk 79). CodeWarrior C.
// Slice 1 (2026-11-11): every function but asArrowOperate (0x1230 bytes: still a stub).
// The arrows are parts of the bank asXcmd_I_have_x_Arrows armed (ArrowInfo.bank); a shot arrow is placed
// at the bow's nock part of the player's model and points at its aim part (asAdjustArrowDirection).

#include "common/alyssapan_stub.h"
#include "game/asUseArrow.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ahalloc.h"
#include "game/ct3_types.h"
#include "game/QuatSlerp.h"
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
    return a->state == ARROW_STATE_SHOT || a->state == ARROW_STATE_FLYING;
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x01866d80
void asArrowOperate(void)
{
    ALYSSAPAN_STUB("asArrowOperate", 0x01866d80);
}
