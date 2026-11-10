// game/wideXcmd_DebugPrint.c -- Clock Tower 3, 0x018f3f60..0x018fe500: the wide xcmd debug print, then the as* player
// operation (asCharacterOperate's frame, hits and damage motions, the key tables asPIA*_KEY_TBL_*, the pad
// move, panic, the motion request queue). 109 functions; tumap.py keyed the file by its first function,
// and the as* run is probably its own object (or several): the slices that read them decide.
//
// Slice 1 (2026-11-10): asCharacterOperate (main's and QuatSlerp's per-frame call), the vibration
// helpers, Alyssa's hit reactions (Hiting, Hirumi), the key enable, the motion request queue, asClearBehaviorChangeFlag and the empty functions.
// Slice 2 (2026-11-10): the positioning (asCharacterPositioning, as1CharacterPositioning, the pause
// review, asResetCharacterRotation), the panic / barrier / lock states, the lever mash, the monster
// distance stack, asSetCharacterIdentity, asInitPlayerBehaviorControler (it sets the motion table:
// MotionRecord reads it), the stairway check, the delta move, the auto aim, the next-motion writer and
// the small key-table entries (asPIA0_KEY_TBL_HAIHAI / WATER / LOCK / PMOVEEND / PRUN).
// Slice 3 (2026-11-10): the motion request queue's consumer (asRequestExecute: slot 0 and the blend
// slot 5), asResetCharacterPosition (the BasePoint's root through the motion's matrix), the ridden
// part (asCharacterForceMoveOperate) and the cat walk's random events.
// Slice 4 (2026-11-10): the per-frame motion play (asCharacterAnimationExecute: the slots, what follows
// a motion's end, root motion), Alyssa's damage reaction (asChangeAlyssaMortionByDamaging) and eleven
// small key-table entries (HAIHAI's step, PSITGO, PSIT, PSTAND, KAIDAN, DOOR, SITGONOUP, GOUP1M).
// Slice 5 (2026-11-10): asCheckCharacterHited (the boss battle's invulnerability and scripted strikes,
// then Alyssa's hit table: each attacker's reaction, SE and her cry).
//
// PORT: CHARACTER.w300[0] (CHARA_TARGET) is an EE address no written TU sets yet (Character_All_Init
// clears it): CharaTarget answers none until its writer decides the host form (open-items.md 1 h).

#include "game/wideXcmd_DebugPrint.h"

#include "common/alyssapan_stub.h"
#include "game/Clipping_Portal.h"
#include "game/LinkMatrixTree.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/asMothOperate.h"
#include "game/asUseArrow.h"
#include "game/main.h"
#include "game/ahalloc.h"
#include "game/fileio.h"
#include "game/main2.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/tiAvoidObject.h"
#include "game/unassigned_data.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaSinCos.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

int MsgCnt;                             /* sbss 0x01944958 */
int sndKAKOKYU;                         /* sbss 0x01944950 */
int bCharKeyEnable;                     /* sbss 0x01944a04 */
u_int pDstMPstock;                      /* sbss 0x01944a40 */
u_int pSrcMPstock;                      /* sbss 0x01944a80 */

static int iLocked;                     /* iLocked$529, sbss 0x01944970 */
// The monster distance stack: (id, distance) pairs, _SunAlloc'd from gHeapJunc. layout: host (the ROM
// keeps the EE address in pMDS$545; the 8-byte entries are the same).
typedef struct MDS_ENTRY {
    int id;
    float dist;
} MDS_ENTRY;
static MDS_ENTRY *pMDS;                 /* pMDS$545, sbss 0x01944978 */
static int iMaxSize;                    /* iMaxSize$546, sbss 0x0194497c */
static int iUseSize;                    /* iUseSize$547, sbss 0x01944980 */
static int iRLC_ID = CLATTERING_FREE;   /* iRLC_ID$667, sdata 0x0193bad4 */
static u_int iPanicStack;               /* iPanicStack$2856, sbss 0x019449c4 */
static int LastTicks;                   /* LastTicks$2901, sbss 0x019449c8 */

static const char kBreathSe[] = "c_0000"; /* @3214, sdata 0x01937280 */
static const char kCalhShape[] = "CALH.BIN"; /* @3757, sdata 0x01937410 */

static int CharaInt(const CHARACTER *ch, int off)
{
    int v;
    memcpy(&v, (const u_char *)ch + off, sizeof(v));
    return v;
}

static void SetCharaInt(CHARACTER *ch, int off, int v)
{
    memcpy((u_char *)ch + off, &v, sizeof(v));
}

static short CharaShort(const CHARACTER *ch, int off)
{
    short v;
    memcpy(&v, (const u_char *)ch + off, sizeof(v));
    return v;
}

static void SetCharaShort(CHARACTER *ch, int off, short v)
{
    memcpy((u_char *)ch + off, &v, sizeof(v));
}

// The object CHARA_TARGET names (an EE word); NULL for none. PORT: see the file comment.
static const u_char *CharaTarget(const CHARACTER *ch)
{
    (void)ch;
    return NULL;
}

// The object CHARA_TARGET2 (w300[1]) names; NULL for none. PORT: as CharaTarget.
static const u_char *CharaTarget2(const CHARACTER *ch)
{
    (void)ch;
    return NULL;
}

static u_int TargetFlags(const u_char *target)
{
    u_int f;
    memcpy(&f, target + CHARA_TARGET_FLAGS, sizeof(f));
    return f;
}

// Record `index` of the character's motion table (CHARA_MOTION_TABLE, an EE word of its bank: set by
// asInitPlayerBehaviorControler); NULL without a table.
static u_char *MotionRecord(const CHARACTER *ch, int index)
{
    u_char *table;
    if (ch->bank < 0)
        return NULL;
    table = BANK_PTR(u_char *, ch->bank, (u_int)CharaInt(ch, CHARA_MOTION_TABLE));
    return table != NULL ? table + index * CHARA_MOTION_SIZE : NULL;
}

// CT3 0x018f3f60
int wideXcmd_DebugPrint(void)
{
    printf("TEST\n");
    return 1;
}

// CT3 0x018f3f90
void WideErrorMsgDebug(void)
{
}

// CT3 0x018f3fa0
void asDualShockVibrationOff(void)
{
    tiVibrationOff(0);
}

// CT3 0x018f3fb0
void asDualShockVibrationOn_Light(int time)
{
    int on = ssTrigCheckEvent(TRIG_EVENT_PANIC_VIBRATE, 1, 0);
    tiVibrationOn(0, on, ssTrigGetVariable(TRIG_VAR_VIB_LIGHT, 1, 0), time);
}

// CT3 0x018f4010
void asDualShockVibrationOn_Midium(int time)
{
    int on = ssTrigCheckEvent(TRIG_EVENT_PANIC_VIBRATE, 1, 0);
    tiVibrationOn(0, on, ssTrigGetVariable(TRIG_VAR_VIB_MIDIUM, 1, 0), time);
}

// CT3 0x018f4070
void asDualShockVibrationOn_SuperLarge(int time)
{
    int on = ssTrigCheckEvent(TRIG_EVENT_PANIC_VIBRATE, 1, 0);
    tiVibrationOn(0, on, ssTrigGetVariable(TRIG_VAR_PANIC_VIBRATE, 1, 0), time);
}

// CT3 0x018f40d0
int asKatchuKekkaiCheck(int ticks)
{
    if (ticks < 0) {
        LastTicks = 0;
        return 0;
    }
    if (LastTicks + ticks > gTotalTicks)
        return 1;
    if (Kekkai_Check() != 0)
        return 0;
    LastTicks = gTotalTicks;
    return 1;
}

// CT3 0x018f42e0
u_int asPanicStatusCheck(int mode)
{
    if (mode == PANIC_STATUS_PUSH)
        iPanicStack = iPanicStack << 1 | (PanicInfoData.mode & PANIC_MODE_FULL);
    else if (mode == PANIC_STATUS_RESET)
        iPanicStack = 0;
    return iPanicStack;
}

// CT3 0x018f4340
void asSetCharacterIdentity(CHARACTER *ch, int player)
{
    if (player == 0)
        ch->flags &= ~CHARACTER_FLAG_PLAYER;
    else
        ch->flags |= CHARACTER_FLAG_PLAYER;
}

// CT3 0x018f4380
void asChangeMortionInfo_NextMortion(int chara, int index, short next, short nextMode)
{
    u_char *rec = MotionRecord(&Character[chara], index);
    if (rec == NULL)
        return;     // PORT: the ROM writes through the word whatever it is
    memcpy(rec + MOTION_REC_NEXT, &next, sizeof(next));
    memcpy(rec + MOTION_REC_NEXT_MODE, &nextMode, sizeof(nextMode));
}

// CT3 0x018f43d0
void asChangeAlyssaMortionByHiting(const float *hit, int mode)
{
    CHARACTER *a = &Character[CHARA_ALYSSA];
    float *pos = (float *)a->pos;
    float step, d[4], q;
    int panicked, front, back, k;
    short motion;
    if (mode == ALYSSA_HIT_PULL)
        panicked = (PanicInfoData.mode & PANIC_MODE_FULL) != 0;
    else
        panicked = (PanicInfoData.mode & PANIC_MODE_FULL) != 0 ? (u_char)Kekkai_Check() : 0;
    if (GlobalCameraMode != CAMERA_MODE_PLAY)
        return;
    step = mode == ALYSSA_HIT_PULL ? -ALYSSA_HIT_PUSH : ALYSSA_HIT_PUSH;
    // VU0: xyz normalised (w kept), then all four scaled and added.
    for (k = 0; k < 4; k++)
        d[k] = hit[k] - pos[k];
    q = 1.0f / sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    for (k = 0; k < 3; k++)
        d[k] *= q;
    for (k = 0; k < 4; k++)
        pos[k] += d[k] * step;
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) != 0) {
        back = MOTION_FATAL_BACK;
        front = MOTION_FATAL_FRONT;
        asDualShockVibrationOn_SuperLarge(ALYSSA_HIT_VIBRATE_TIME);
    } else if (!panicked || mode == ALYSSA_HIT_PLAIN) {
        if (a->targetState & 1) {
            back = MOTION_FLINCH_BACK;
            front = MOTION_FLINCH_FRONT;
        } else {
            back = MOTION_FLINCH2_BACK;
            front = MOTION_FLINCH2_FRONT;
        }
        asDualShockVibrationOn_Midium(ALYSSA_HIT_VIBRATE_TIME);
    } else if (mode == ALYSSA_HIT_LETHAL) {
        back = MOTION_FATAL_BACK;
        front = MOTION_FATAL_FRONT;
        asDualShockVibrationOn_SuperLarge(ALYSSA_HIT_VIBRATE_TIME);
        if (ssTrigCheckEvent(TRIG_EVENT_NO_FATAL_HIT, 1, 0) == 0)
            PlayerDamage(ALYSSA_FATAL_DAMAGE);
    } else {
        back = MOTION_FLINCH_BACK;
        front = MOTION_FLINCH_FRONT;
        asDualShockVibrationOn_Midium(ALYSSA_HIT_VIBRATE_TIME);
    }
    motion = CharaShort(a, CHARA_MOTION_SLOTS);
    if (motion == back || motion == front)
        return;
    // Queue `front` when the hit lies within 90 degrees of her facing (asRadianByVectors about Y), else
    // `back`, and turn so the hit is straight ahead (front) or straight behind (back).
    {
        const float *from = (const float *)a->posBefore;
        alignas(16) float t[4], face[4], m[4][4];
        u_int angle;
        for (k = 0; k < 4; k++)
            t[k] = hit[k] - from[k];
        gaSetRotMatrixY(m, a->rot);
        memcpy(face, m[2], sizeof(face));
        angle = (u_int)asRadianByVectors(t, face, ROTATE_AXIS_Y) & 0xffff;
        face[0] = face[1] = face[2] = 0.0f;
        face[3] = 1.0f;
        if (angle < 0x4000 || angle >= 0xc000) {
            asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, front);
            face[2] = 1.0f;
        } else {
            asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, back);
            face[2] = -1.0f;
        }
        a->rot = asRadianByVectors(t, face, ROTATE_AXIS_Y) & 0xffff;
    }
}

// CT3 0x018f4ab0
void asChangeAlyssaMortionByHirumi(const float *hit)
{
    CHARACTER *a = &Character[CHARA_ALYSSA];
    int k;
    short motion;
    if (GlobalCameraMode != CAMERA_MODE_PLAY)
        return;
    motion = CharaShort(a, CHARA_MOTION_SLOTS);
    asDualShockVibrationOn_Midium(ALYSSA_HIT_VIBRATE_TIME);
    if (motion == MOTION_FLINCH_BACK || motion == MOTION_FLINCH_FRONT)
        return;
    // Queue `front` when the hit lies within 90 degrees of her facing (asRadianByVectors about Y), else
    // `back`, and turn so the hit is straight ahead (front) or straight behind (back).
    {
        const float *from = (const float *)a->posBefore;
        alignas(16) float t[4], face[4], m[4][4];
        u_int angle;
        for (k = 0; k < 4; k++)
            t[k] = hit[k] - from[k];
        gaSetRotMatrixY(m, a->rot);
        memcpy(face, m[2], sizeof(face));
        angle = (u_int)asRadianByVectors(t, face, ROTATE_AXIS_Y) & 0xffff;
        face[0] = face[1] = face[2] = 0.0f;
        face[3] = 1.0f;
        if (angle < 0x4000 || angle >= 0xc000) {
            asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, MOTION_FLINCH_FRONT);
            face[2] = 1.0f;
        } else {
            asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, MOTION_FLINCH_BACK);
            face[2] = -1.0f;
        }
        a->rot = asRadianByVectors(t, face, ROTATE_AXIS_Y) & 0xffff;
    }
}

// CT3 0x018f4bd0
void asCharacterPositionReviewInPausing(CHARACTER *chr)
{
    int i;
    if ((pause != 0 && (pause & PAUSE_RUNNING) == 0) || pause_reservation == 0)
        return;
    for (i = 0; i < CHARACTER_MAX; i++) {
        CHARACTER *ch = &chr[i];
        if (ch->id != -1 && (ch->flags & CHARACTER_FLAG_REFRACTION) != 0)
            memcpy(&ch->pos, &ch->posBefore, sizeof(ch->pos));
    }
}

// CT3 0x018f5440
int asCheckAlyssaOnStairway(void)
{
    CHARACTER *a = &Character[CHARA_ALYSSA];
    const u_char *rec, *t1, *t2;
    int on;
    if (a->id == -1 || (a->flags & CHARACTER_FLAG_REFRACTION) == 0)
        return 0;
    if (CharaShort(a, CHARA_MOTION_SLOTS) < 0)
        return 0;
    rec = MotionRecord(a, CharaShort(a, CHARA_MOTION_SLOTS));
    if (rec == NULL)
        return 0;   // PORT: no table on the host yet (the ROM reads through the word)
    if (rec[MOTION_REC_KIND] != MOTION_KIND_STAIRS && rec[MOTION_REC_KIND] != MOTION_KIND_STAIRS2)
        return 0;
    t1 = CharaTarget(a);
    t2 = CharaTarget2(a);
    if (t2 == NULL || t1 == NULL)
        on = t2 == NULL && t1 != NULL && (TargetFlags(t1) & CHARA_TARGET_FLAG_F4) != 0;
    else if ((TargetFlags(t2) & CHARA_TARGET_FLAG_F4) != 0 && (TargetFlags(t1) & CHARA_TARGET_FLAG_F4) == 0)
        on = 1;
    else
        on = (TargetFlags(t2) & CHARA_TARGET_FLAG_F4) != 0 && (TargetFlags(t1) & CHARA_TARGET_FLAG_F4) != 0;
    if (on)
        memcpy(&a->pos, &a->posBefore, sizeof(a->pos));
    return on;
}

// Is the character in this call's pass? (the ROM reads the flags again at every test)
#define IN_PASS(ch) ((others != 0 && ((ch)->flags & CHARACTER_FLAG_PLAYER) == 0) || \
                     (player != 0 && ((ch)->flags & CHARACTER_FLAG_PLAYER) != 0))

// CT3 0x018f4fc0
void asCharacterOperate(CHARACTER *chr, int others, int player, int animate)
{
    int i;
    if (ssTrigGetVariable(TRIG_VAR_PANIC_OFF, 1, 0) != -1)
        MsgCnt = FrameCounter;
    for (i = 0; i < CHARACTER_MAX; i++) {
        CHARACTER *ch = &chr[i];
        if (ch->id == -1 || (ch->flags & CHARACTER_FLAG_REFRACTION) == 0) {
            // Not operated: only its behaviours' requests run, when it has a bank and a behaviour.
            int b, any = 0;
            if (ch->id == -1 || !IN_PASS(ch) || ch->bank < 0)
                continue;
            for (b = 0; b < 10; b++)
                if (ch->behaviorBank[b] >= 0)
                    any = 1;
            if (any)
                asRequestExecute(ch);
            continue;
        }
        if (asCheckAnimationExecutable_Scene(ch->bank) == 0)
            continue;
        if (others != 0 && (ch->flags & CHARACTER_FLAG_PLAYER) == 0) {
            asClearBehaviorChangeFlag(ch);
            asCheckCharacterHited(i);
            asRequestExecute(ch);
            if (animate != 0 && asCharacterAnimationExecute(ch) != 0)
                asRequestExecute(ch);
        }
        if (player != 0 && (ch->flags & CHARACTER_FLAG_PLAYER) != 0) {
            asClearBehaviorChangeFlag(ch);
            asCheckCharacterHited(i);
            asRequestExecute(ch);
            asPlayerInputAnalyze(ch);
            asRequestExecute(ch);
            if (animate != 0 && asCharacterAnimationExecute(ch) != 0)
                asRequestExecute(ch);
            asArrowOperate();
            asMothOperate();
            pSrcMPstock = 0;
            pDstMPstock = 0;
        }
        if (animate != 0 && IN_PASS(ch)) {
            const u_char *target;
            asCharacterAutoAiming(ch);
            asMotionAdderByNowMotion(ch);
            asCharacterMortionFunctionOperation(ch);
            target = ch->w300[0] != 0 ? CharaTarget(ch) : NULL;
            if (target == NULL) {
                ch->targetState = 0;
            } else {
                alignas(16) float rot[4][4];
                u_int tflags;
                memcpy(&tflags, target + CHARA_TARGET_FLAGS, sizeof(tflags));
                if ((tflags & CHARA_TARGET_FLAG_F4) == 0)
                    ch->targetState &= ~1;
                else
                    ch->targetState |= 1;
                gaSetRotMatrixY(rot, ch->rot);
                ch->targetDot = gaInnerProductXYZ(rot[2], (float *)(target + CHARA_TARGET_VEC));
            }
            if (asCharacterKeyEnable() == 0)
                chr->keyTicks = 0;      // sic: the ROM clears the first slot's (chr), not ch's
            else
                ch->keyTicks += gGlobalNextTicksInterval;
        }
    }
    // The player pass keeps Alyssa's breath SE with her breathing motion; one of characters 29..31
    // (cast shadows, playing CHARA_MOTION_BREATH_STOP) silences it.
    if (player != 0) {
        short motion = CharaShort(&Character[0], CHARA_MOTION_SLOTS);
        if (sndKAKOKYU == 0) {
            if (motion == CHARA_MOTION_KAKOKYU) {
                SSPlaySE(kBreathSe, SE_KAKOKYU, 1);
                sndKAKOKYU = 1;
            }
        } else if (motion == CHARA_MOTION_KAKOKYU) {
            int c;
            for (c = CHARA_BREATH_STOP_FIRST; c < CHARA_BREATH_STOP_FIRST + CHARA_BREATH_STOP_COUNT; c++) {
                const CHARACTER *o = &Character[c];
                if ((o->flags & CHARACTER_FLAG_SHADOW) != 0 &&
                    CharaShort(o, CHARA_MOTION_SLOTS) == CHARA_MOTION_BREATH_STOP) {
                    SSStopSE(kBreathSe, SE_KAKOKYU, 1);
                    sndKAKOKYU = 0;
                    break;
                }
            }
        } else {
            SSStopSE(kBreathSe, SE_KAKOKYU, 1);
            sndKAKOKYU = 0;
        }
    }
}

// CT3 0x018f6f00
void asCharacterPositioningOnlyDeltaPos(int bank, const float *delta)
{
    MODEL_PARTS *p0 = (MODEL_PARTS *)pModelPartsHeader[bank];
    int k;
    if (asCheckAlyssaOnStairway() != 0)
        return;
    for (k = 0; k < 3; k++)     // VU0 vadd.xyz
        p0->drawOnPos[k] += delta[k];
    p0->flags43 |= PARTS_FLAG43_WARPED;
    BuildMatrixTree(bank, 2, 0);
}

// CT3 0x018f6f80
void asCharacterPositioning(CHARACTER *chr, int others, int player)
{
    int i;
    for (i = 0; i < CHARACTER_MAX; i++) {
        CHARACTER *ch = &chr[i];
        if (ch->id != -1 && (ch->flags & CHARACTER_FLAG_REFRACTION) != 0 && IN_PASS(ch))
            as1CharacterPositioning(ch);
    }
}

#undef IN_PASS

// CT3 0x018f7040
void as1CharacterPositioning(CHARACTER *chr)
{
    u_char *hdr;
    MODEL_PARTS *p0;
    u_char *rec;
    if (chr->bank < 0)
        return;     // PORT: the ROM reads pModelPartsHeader[bank] whatever the bank (-1: the word before)
    hdr = pModelPartsHeader[chr->bank];
    if (hdr == NULL)
        return;
    p0 = (MODEL_PARTS *)hdr;
    rec = asFindModelParts(hdr, PARTS_HASH_BASE_POINT);
    if (rec == NULL) {
        gaSetRotMatrixY(p0->placeRot, chr->rot);
        memcpy(p0->drawOnPos, &chr->pos, sizeof(p0->drawOnPos));
    } else {
        // VU0: the BasePoint's placement (xyz negated, w kept) through part 0's new placement matrix
        // (rows placeRot, drawOnPos), then the character's position added (xyz).
        const MODEL_PARTS *bp = (const MODEL_PARTS *)(rec - PARTS_RECORDS);
        const float *pos = (const float *)&chr->pos;
        float (*m)[4] = p0->placeRot;   // 4 rows: placeRot[0..2], drawOnPos
        float v[4], r[4];
        int k;
        v[0] = -bp->drawOnPos[0];
        v[1] = -bp->drawOnPos[1];
        v[2] = -bp->drawOnPos[2];
        v[3] = bp->drawOnPos[3];
        gaSetRotMatrixY(m, chr->rot - CharaInt(chr, CHARA_BASE_ROT));
        for (k = 0; k < 4; k++)
            r[k] = m[0][k] * v[0] + m[1][k] * v[1] + m[2][k] * v[2] + m[3][k] * v[3];
        for (k = 0; k < 3; k++)
            r[k] += pos[k];
        memcpy(p0->drawOnPos, r, sizeof(r));
    }
    p0->flags43 |= PARTS_FLAG43_WARPED;
    if ((chr->flags & CHARACTER_FLAG_BUILD_NEG) == 0)
        BuildMatrixTree(chr->bank, 2, 0);
    else
        BuildMatrixTree(-chr->bank, 2, 0);
    chr->flags &= ~CHARACTER_FLAG_BUILD_NEG;
    if ((chr->flagsPrev & CHARACTER_FLAG_SHADOW) == 0 && (chr->flags & CHARACTER_FLAG_SHADOW) != 0)
        asCopyDoubleBufferL2W(BANK_PTR(u_char *, chr->bank, All_table_TBL[chr->bank]->partsHeader));
}

// CT3 0x018f73b0
// The records' fields, as offsets from a record (PARTS_REC): MODEL_PARTS' offsets - PARTS_RECORDS.
#define REC_TYPE 0x00                   /* short: < 0 after the last record */
#define REC_TYPE_TURN 1                 /* the type asResetCharacterRotation turns */
#define REC_FLAGS43 0x03                /* MODEL_PARTS.flags43 */
#define REC_PLACE_ROT 0x280             /* MODEL_PARTS.placeRot */
void asResetCharacterRotation(int unused, u_char *recs, int rot)
{
    alignas(16) float m[4][4];
    (void)unused;
    if (recs == NULL)
        return;
    gaSetRotMatrixY(m, rot);
    for (;; recs += PARTS_REC_SIZE) {
        short type;
        memcpy(&type, recs + REC_TYPE, sizeof(type));
        if (type < 0)
            break;
        if (type == REC_TYPE_TURN) {
            float (*place)[4] = (float (*)[4])(recs + REC_PLACE_ROT);
            gaMulMatrix(place, m, place);
            recs[REC_FLAGS43] |= PARTS_FLAG43_WARPED;
        }
    }
}
#undef REC_TYPE
#undef REC_TYPE_TURN
#undef REC_FLAGS43
#undef REC_PLACE_ROT

// CT3 0x018f7cd0
void asCharacterAutoAiming(CHARACTER *ch)
{
    const u_char *rec;
    const float *pos = (const float *)&ch->pos;
    float aimW, to[4];
    int back, want, diff, step, k;
    u_short speed;
    memcpy(&aimW, (u_char *)ch + CHARA_AIM_W, sizeof(aimW));
    if (CharaInt(ch, CHARA_AIM_PART) == 0 && aimW != 1.0f)
        return;
    // A backward-facing motion aims with its back: the turn is computed on rot + AIM_BACK_ROT.
    rec = MotionRecord(ch, CharaShort(ch, CHARA_MOTION_SLOTS));
    back = 0;   // PORT: no table on the host yet faces forward (the ROM reads through the word)
    if (rec != NULL && rec[MOTION_REC_FACING] >= MOTION_FACING_BACK_MIN &&
        rec[MOTION_REC_FACING] <= MOTION_FACING_BACK_MAX)
        back = AIM_BACK_ROT;
    ch->rot += back;
    if (aimW == 1.0f) {
        const float *point = (const float *)((u_char *)ch + CHARA_AIM_POINT);
        for (k = 0; k < 4; k++)
            to[k] = point[k] - pos[k];
    } else {
        u_short bank;
        const MODEL_PARTS *parts;
        memcpy(&bank, (u_char *)ch + CHARA_AIM_BANK, sizeof(bank));
        parts = (const MODEL_PARTS *)pModelPartsHeader[bank];
        for (k = 0; k < 4; k++)
            to[k] = parts[CharaInt(ch, CHARA_AIM_PART)].pos[k] - pos[k];
    }
    want = -((int)((asRadianByO(to, ROTATE_AXIS_Y) * 65536.0f) / 6.2831855f) & 0xffff);
    diff = asISubRotate16((u_int)want, (u_int)ch->rot);
    memcpy(&speed, (u_char *)ch + CHARA_AIM_SPEED, sizeof(speed));
    step = speed * gGlobalNextTicksInterval;
    if (-step < diff && diff < step)
        ch->rot = want;
    else if (diff < 0)
        ch->rot -= step;
    else
        ch->rot += step;
    ch->rot -= back;
}

// CT3 0x018f7ec0
void asPIA1_KEY_TBL_PULLED(void)
{
}

// CT3 0x018f8020
void asPIA0_KEY_TBL_HAIHAI(CHARACTER *ch, u_char *key)
{
    short on = 1;
    (void)ch;
    memcpy(key + PIA_KEY_H0C, &on, sizeof(on));
}

// CT3 0x018f8030
void asPIA1_KEY_TBL_WATER(void)
{
}

// CT3 0x018f8040
int asPIA0_KEY_TBL_WATER(CHARACTER *ch)
{
    (void)ch;
    return 0;
}

// CT3 0x018f8600
int asPIA0_KEY_TBL_LOCK(CHARACTER *ch)
{
    (void)ch;
    return 1;
}

// CT3 0x018f8610
void asPIA1_KEY_TBL_PMOVEEND(void)
{
}

// CT3 0x018f8620
int asPIA0_KEY_TBL_PMOVEEND(CHARACTER *ch)
{
    (void)ch;
    return 1;
}

// CT3 0x018f86d0
int asPIA0_KEY_TBL_PRUN(CHARACTER *ch)
{
    const u_char *rec = MotionRecord(ch, CharaShort(ch, CHARA_MOTION_SLOTS));
    // PORT: no table on the host yet answers 1 (the ROM reads through the word)
    return rec != NULL && rec[MOTION_REC_RUN] == MOTION_RUN_FAST ? 2 : 1;
}

// CT3 0x018fb4c0
int asRapidLeverChangeOperate(int mode, int time, int decay, int w08, int goal)
{
    int state = 0;
    if (mode == RLC_FREE) {
        if (iRLC_ID != CLATTERING_FREE) {
            ControllerClatteringFree(iRLC_ID);
            state = 1;
            iRLC_ID = CLATTERING_FREE;
        }
    } else if (mode == RLC_SET) {
        if (iRLC_ID == CLATTERING_FREE) {
            CLATTERING_SET set;
            set.time = time;
            set.decayFrames = decay;
            set.w08 = w08;
            set.goal = goal;
            iRLC_ID = ControllerClatteringSet(&set);
        }
        state = ControllerClatteringGet(iRLC_ID);
        if (state > 0)
            iRLC_ID = CLATTERING_FREE;
    }
    return state;
}

// CT3 0x018fb940
int asMonsterDistanceStack_inPanic(int mode, int *id, float *dist)
{
    int i, found;
    switch (mode) {
    case MDS_NEAREST:
        if (iUseSize == 0)
            break;
        found = -1;
        for (i = 0; i < iUseSize; i++) {
            if (pMDS[i].dist <= MDS_FAR) {
                *id = pMDS[i].id;
                *dist = pMDS[i].dist;
                found = *id;
            }
        }
        return found;
    case MDS_FREE:
        if (pMDS != NULL)
            _SunFree(&gHeapJunc, (void **)&pMDS);
        pMDS = NULL;
        iMaxSize = 0;
        iUseSize = 0;
        break;
    case MDS_PUSH:
        if (iMaxSize == 0 || pMDS == NULL) {
            iMaxSize = MDS_GROW;
            iUseSize = 0;
            pMDS = (MDS_ENTRY *)_SunAlloc(&gHeapJunc, MDS_GROW * sizeof(MDS_ENTRY), 16);
        }
        if (iUseSize == iMaxSize) {
            MDS_ENTRY *grown;
            iMaxSize += MDS_GROW;
            grown = (MDS_ENTRY *)_SunAlloc(&gHeapJunc, iMaxSize * (int)sizeof(MDS_ENTRY), 16);
            // PORT: the ROM copies iMaxSize entries (the new size: 16 past the old block); the port copies
            // the old block only.
            memcpy(grown, pMDS, (size_t)iUseSize * sizeof(MDS_ENTRY));
            _SunFree(&gHeapJunc, (void **)&pMDS);
            pMDS = grown;
        }
        pMDS[iUseSize].id = *id;
        pMDS[iUseSize].dist = *dist;
        iUseSize++;
        break;
    case MDS_FORGET:
        pMDS = NULL;
        iMaxSize = 0;
        iUseSize = 0;
        break;
    }
    return -1;
}

// CT3 0x018fbb50
int asAlyssaLockedByAnything(int mode)
{
    int was = iLocked;
    switch (mode & 0xff) {
    case LOCKED_SET_3:
        return iLocked = (mode & LOCKED_KEEP_MASK) | 3;
    case LOCKED_SET_2:
        return iLocked = (mode & LOCKED_KEEP_MASK) | 2;
    case LOCKED_SET_1:
        return iLocked = (mode & LOCKED_KEEP_MASK) | 1;
    case LOCKED_TAKE:
        iLocked = 0;
        return was;
    case LOCKED_QUERY:
        if ((iLocked & LOCKED_KEEP_MASK) == 0 && ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) != 0)
            return 0;
        return iLocked;
    }
    return -1;
}

// CT3 0x018fe0c0
void asClearBehaviorChangeFlag(CHARACTER *ch)
{
    memset((u_char *)ch + CHARA_BEHAVIOR_CHANGE, 0, CHARA_BEHAVIOR_CHANGE_COUNT);
}

// CT3 0x018fe1f0
void asCharacterMortionStopRequest(int chara, int slot)
{
    asCharacterMortionChangeRequest(chara, CHARA_MOTION_REQ_STOP,
                                    CharaShort(&Character[chara], CHARA_MOTION_SLOTS + slot * 2));
}

// CT3 0x018fe230
u_long asCharacterMortionChangeRequestHash(int chara, int mode, u_long hash)
{
    int index = asFindMotionID(chara, hash);
    if (index == -1)
        return 1;
    asCharacterMortionChangeRequest(chara, mode, index);
    return 0;
}

// CT3 0x018fe290
u_long asCharacterMortionChangeRequest(int chara, int mode, int index)
{
    CHARACTER *ch = &Character[chara];
    int i, free = -1;
    for (i = 0; i < CHARA_MOTION_QUEUE_COUNT; i++) {
        if (CharaShort(ch, CHARA_MOTION_QUEUE + i * 2) == -1) {
            free = i;
            break;
        }
    }
    if (free == -1)
        return 1;
    SetCharaShort(ch, CHARA_MOTION_QUEUE + free * 2, (short)index);
    SetCharaShort(ch, CHARA_MOTION_QUEUE_MODE + free * 2, (short)mode);
    return 0;
}

// CT3 0x018fe330
int asCharacterKeyEnable(void)
{
    if (SkipMain != 0)
        return 0;
    if (DemoSkip != 0)
        return 0;
    return bCharKeyEnable;
}

// CT3 0x018fe370
void asSetCharacterKeyEnable(int enable)
{
    bCharKeyEnable = enable;
}

// CT3 0x018fe380
void asInitPlayerBehaviorControler(CHARACTER *chr)
{
    static const u_char kBytes[8] = {0, 0, 0, 0, 1, 0xff, 0, 6};
    u_char *c = (u_char *)chr;
    u_int table;
    int i, j;
    memset(c + CHARA_BHV_Q310, 0, 0x20);
    memcpy(c + CHARA_BHV_BYTES, kBytes, sizeof(kBytes));
    for (i = 0; i < CHARA_BHV_SLOT_COUNT; i++) {
        SetCharaShort(chr, CHARA_BHV_H38C + i * 2, -1);
        SetCharaShort(chr, CHARA_MOTION_SLOTS + i * 2, -1);
        c[CHARA_BHV_B3A4 + i] = 0;
        c[CHARA_BEHAVIOR_CHANGE + i] = 0;
        for (j = 0; j < CHARA_BHV_QUEUE_PER_SLOT; j++) {
            int q = j + i * CHARA_BHV_QUEUE_PER_SLOT;
            SetCharaShort(chr, CHARA_MOTION_QUEUE + q * 2, -1);
            SetCharaShort(chr, CHARA_MOTION_QUEUE_MODE + q * 2, -1);
        }
        SetCharaInt(chr, CHARA_BHV_W374 + i * 4, 0);
        SetCharaInt(chr, CHARA_BHV_W35C + i * 4, 0);
        SetCharaInt(chr, CHARA_BHV_W374 + i * 4, 0);    // sic: the ROM clears it twice
        SetCharaInt(chr, CHARA_BHV_W458 + i * 4, 0);
        SetCharaInt(chr, CHARA_BHV_W470 + i * 4, 0);
    }
    // The motion table: the bank's ALL_TABLE word at +0xb8 (its `fog` for scene banks), an EE address.
    if (chr->bank < 0 || chr->id == CHARA_NO_MOTION_TABLE)
        table = 0;
    else
        table = All_table_TBL[chr->bank]->fog;
    SetCharaInt(chr, CHARA_MOTION_TABLE, (int)table);
    SetCharaInt(chr, CHARA_AIM_PART, 0);
    SetCharaInt(chr, CHARA_BHV_W518, -1);
    if (strcmp(chr->shapeName, kCalhShape) == 0) {
        u_char *t = BANK_PTR(u_char *, chr->bank, table);
        short zero = 0;
        if (t != NULL)      // PORT: the ROM writes through the word whatever it is
            memcpy(t + MOTION_TABLE_H12, &zero, sizeof(zero));
    }
}

// CT3 0x018fe4b0
void ChrMoveControllerMode_init(void)
{
    controller_type = CONTROLLER_TYPE_DEFAULT;
}

// ---- slice 3 (2026-11-10): the request queue's consumer, the root reset, the cat walk ----

static u_short iNowTime;                /* iNowTime$681, sbss 0x01944984 */
static u_short iBeforeTime;             /* iBeforeTime$682, sbss 0x01944988 */
static const u_short kCatWalkTimes[CATWALK_TIMES] = { /* @683, sdata 0x0193bad8 */
    900, 1200, 1500, 1800, 2100, 2400, 2700,
};

static void SetCharaByte(CHARACTER *ch, int off, u_char v)
{
    ((u_char *)ch)[off] = v;
}

static short RecShort(const u_char *rec, int off)
{
    short v;
    memcpy(&v, rec + off, sizeof(v));
    return v;
}

static u_long RecHash(const u_char *rec)
{
    u_long v;
    memcpy(&v, rec + CHARA_MOTION_BEHAVIOR, sizeof(v));
    return v;
}

// PORT: asGetBehaviorPlayableTicks takes (bank, anim); the ROM passes the found behaviour.
static int BehaviorPlayable(BEHAVIOR *bhv)
{
    int bank = BankOfHostPtr(bhv);
    return asGetBehaviorPlayableTicks(bank, BankEeAddr(bank, bhv));
}

// CT3 0x018f7210
void asResetCharacterPosition(CHARACTER *ch)
{
    ALL_TABLE *tbl;
    u_char *hdr, *rec;
    BEHAVIOR *bhv;
    float one = 1.0f;
    if (CharaShort(ch, CHARA_MOTION_SLOTS) == -1)
        return;
    if (ch->bank < 0 || (tbl = All_table_TBL[ch->bank]) == NULL)
        return;     // PORT: the ROM reads All_table_TBL[bank] whatever it holds
    hdr = BANK_PTR(u_char *, ch->bank, tbl->partsHeader);
    rec = MotionRecord(ch, CharaShort(ch, CHARA_MOTION_SLOTS));
    bhv = rec != NULL ? asFindBehaviorByCharacter(ch->id, RecHash(rec)) : NULL;
    if (bhv != NULL) {
        alignas(16) float m[4][4];
        u_char *part;
        int bank = BankOfHostPtr(bhv);
        ExecuteAnimation(bank, BankEeAddr(bank, bhv), tbl, m, 0, 0, 1, 0);
        part = hdr != NULL ? asFindModelParts(hdr, PARTS_HASH_BASE_POINT) : NULL;
        if (part != NULL) {
            // m[3][3]'s word holds ExecuteAnimation's done bytes; the VU0 product takes w = 1.0.
            u_int done, rot;
            int old;
            memcpy(&done, &m[3][3], sizeof(done));
            m[3][3] = 1.0f;
            if (EA_DONE_ROT_BYTES(done) == 0)
                rot = (u_int)CharaInt(ch, CHARA_BASE_ROT);
            else
                rot = (u_int)(int)((-asRadianByO(m[1], ROTATE_AXIS_Y) * ROT_FULL_TURN) / 6.2831855f) & 0xffff;
            old = CharaInt(ch, CHARA_BASE_ROT);
            SetCharaInt(ch, CHARA_BASE_ROT, (int)rot);
            if (done != 0) {
                float v[4], r[4];
                int k;
                memcpy(v, part + PARTS_REC_ROOT_VEC, sizeof(v));
                for (k = 0; k < 4; k++)
                    r[k] = m[0][k] * v[0] + m[1][k] * v[1] + m[2][k] * v[2] + m[3][k] * v[3];
                memcpy((u_char *)ch + CHARA_MOTION_ROOT, r, sizeof(r));
            }
            asResetCharacterRotation(0, hdr + PARTS_RECORDS, (int)rot - old);
        }
    }
    memcpy((u_char *)ch + CHARA_MOTION_ROOT_W, &one, sizeof(one));
}

// A slot's fade: length ticks * 100, the time left the same.
static void SetSlotFade(CHARACTER *ch, int slot, int len)
{
    SetCharaInt(ch, CHARA_SLOT_FADE_LEN + slot * 4, len);
    SetCharaInt(ch, CHARA_SLOT_FADE + slot * 4, len);
}

// The blend slot's motion gone (modes 0 and 4): its motion becomes its previous one.
static void DropBlendSlot(CHARACTER *ch)
{
    SetCharaShort(ch, CHARA_SLOT_PREV + CHARA_SLOT_BLEND * 2, CharaShort(ch, CHARA_MOTION_SLOTS + CHARA_SLOT_BLEND * 2));
    SetCharaShort(ch, CHARA_MOTION_SLOTS + CHARA_SLOT_BLEND * 2, -1);
}

// The slot's motion becomes `motion` (the old one its previous), marked changed, its length kept.
static void SetSlotMotion(CHARACTER *ch, int slot, short motion, BEHAVIOR *bhv, u_char change, int keepPrev)
{
    if (keepPrev)
        SetCharaShort(ch, CHARA_SLOT_PREV + slot * 2, CharaShort(ch, CHARA_MOTION_SLOTS + slot * 2));
    SetCharaShort(ch, CHARA_MOTION_SLOTS + slot * 2, motion);
    SetCharaByte(ch, CHARA_BEHAVIOR_CHANGE + slot, change);
    SetCharaInt(ch, CHARA_SLOT_PLAYABLE + slot * 4, BehaviorPlayable(bhv));
}

// CT3 0x018fbc20
// Runs the character's motion request queue (asCharacterMortionChangeRequest's) in order, each entry by
// its mode, then empties it; CHARA_MOTION_GROUP follows slot 0's motion.
// PORT: the behaviours are host pointers (asFindBehaviorByCharacter); a request whose record or
// behaviour is missing goes the ROM's way (the as*Behavior* calls take NULL as nothing).
void asRequestExecute(CHARACTER *ch)
{
    int q;
    void *go;
    // callcheck: asGetBehaviorPlayableTicks (through BehaviorPlayable, the (bank, anim) form)
    if (CharaInt(ch, CHARA_MOTION_TABLE) == 0)
        return;
    go = ch->bank >= 0 ? pModelPartsHeader[ch->bank] : NULL;
    for (q = 0; q < CHARA_MOTION_QUEUE_COUNT; q++) {
        short motion = CharaShort(ch, CHARA_MOTION_QUEUE + q * 2);
        short blendMotion = CharaShort(ch, CHARA_MOTION_SLOTS + CHARA_SLOT_BLEND * 2);
        const u_char *rec;
        BEHAVIOR *bhv, *blend = NULL;
        int slot;
        if (motion == -1)
            break;
        rec = MotionRecord(ch, motion);
        if (rec == NULL)
            break;  // PORT: no table on the host (the ROM reads through the word)
        slot = rec[MOTION_REC_SLOT];
        bhv = asFindBehaviorByCharacter(ch->id, RecHash(rec));
        if (blendMotion != -1) {
            const u_char *brec = MotionRecord(ch, blendMotion);
            blend = brec != NULL ? asFindBehaviorByCharacter(ch->id, RecHash(brec)) : NULL;
        }
        switch (CharaShort(ch, CHARA_MOTION_QUEUE_MODE + q * 2)) {
        case CHARA_MOTION_REQ_PLAY: {
            short cur = CharaShort(ch, CHARA_MOTION_SLOTS + slot * 2);
            if (slot == CHARA_SLOT_PANIC && ch->id == CHARA_ALYSSA &&
                (ch->hp == 0 || ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) != 0))
                break;
            if (cur == motion)
                break;
            if (slot == CHARA_SLOT_BODY) {
                SetCharaInt(ch, CHARA_SLOT_FADE_LEN + CHARA_SLOT_BLEND * 4, 0);
                SetCharaInt(ch, CHARA_SLOT_FADE + CHARA_SLOT_BLEND * 4, 0);
                SetSlotFade(ch, CHARA_SLOT_BODY, RecShort(rec, MOTION_REC_BLEND_IN) * 100);
                if (blend != NULL) {
                    asBehaviorStopAnimation(blend);
                    DropBlendSlot(ch);
                    SetCharaInt(ch, CHARA_SLOT_PLAYABLE + CHARA_SLOT_BLEND * 4, 0);
                }
                ch->keyTicks = 0;
            } else {
                SetSlotFade(ch, slot, RecShort(rec, MOTION_REC_BLEND_IN) * 100);
            }
            asBehaviorStartAnimationFromTop(bhv, go != NULL, 1);
            asChangeBehaviorAnimationPlayRate(bhv, rec[MOTION_REC_PLAY_RATE]);
            SetSlotMotion(ch, slot, motion, bhv, CHARA_SLOT_CHANGE_START, 1);
            if (slot == CHARA_SLOT_BODY)
                asResetCharacterPosition(ch);
            if (rec[MOTION_REC_KIND2] == MOTION_REC_KIND2_RESET)
                SetCharaInt(ch, CHARA_BHV_W51C, 0);
            break;
        }
        case CHARA_MOTION_REQ_RESTART:
            if (CharaShort(ch, CHARA_MOTION_SLOTS + slot * 2) == motion)
                break;
            // Slot 0 restarts without a fade while the blend slot fades (its fade left non-zero).
            if (slot == CHARA_SLOT_BODY && CharaInt(ch, CHARA_SLOT_FADE + CHARA_SLOT_BLEND * 4) != 0)
                SetSlotFade(ch, slot, 0);
            else
                SetSlotFade(ch, slot, RecShort(rec, MOTION_REC_BLEND_RESTART) * 100);
            asBehaviorStartAnimationFromTop(bhv, go != NULL, 0);
            asChangeBehaviorAnimationPlayRate(bhv, rec[MOTION_REC_PLAY_RATE]);
            SetSlotMotion(ch, slot, motion, bhv, CHARA_SLOT_CHANGE_START, 0);
            if (slot == CHARA_SLOT_BODY)
                asResetCharacterPosition(ch);
            break;
        case CHARA_MOTION_REQ_STOP:
            if (bhv != NULL) {
                asBehaviorStopAnimation(bhv);
                SetCharaShort(ch, CHARA_SLOT_PREV + slot * 2, CharaShort(ch, CHARA_MOTION_SLOTS + slot * 2));
                SetCharaShort(ch, CHARA_MOTION_SLOTS + slot * 2, -1);
                SetCharaInt(ch, CHARA_SLOT_PLAYABLE + slot * 4, 0);
            }
            break;
        case CHARA_MOTION_REQ_RESUME:
            if (CharaShort(ch, CHARA_MOTION_SLOTS + slot * 2) == motion)
                break;
            SetSlotFade(ch, slot, 0);
            asBehaviorStartAnimation(bhv, go != NULL);
            asChangeBehaviorAnimationPlayRate(bhv, rec[MOTION_REC_PLAY_RATE]);
            SetSlotMotion(ch, slot, motion, bhv, CHARA_SLOT_CHANGE_RESUME, 1);
            if (slot == CHARA_SLOT_BODY)
                asResetCharacterPosition(ch);
            if (rec[MOTION_REC_KIND2] == MOTION_REC_KIND2_RESET)
                SetCharaInt(ch, CHARA_BHV_W51C, 0);
            break;
        case CHARA_MOTION_REQ_STOP_BLEND:
            asBehaviorStopAnimation(blend);
            DropBlendSlot(ch);
            SetCharaInt(ch, CHARA_SLOT_PLAYABLE + CHARA_SLOT_BLEND * 4, 0);
            SetCharaInt(ch, CHARA_SLOT_FADE_LEN + CHARA_SLOT_BLEND * 4, 0);
            SetCharaInt(ch, CHARA_SLOT_FADE + CHARA_SLOT_BLEND * 4, 0);
            break;
        case CHARA_MOTION_REQ_BLEND: {
            short body = CharaShort(ch, CHARA_MOTION_SLOTS);
            if (body == motion || slot != CHARA_SLOT_BODY)
                break;
            if (motion == blendMotion) {
                // Back to the motion slot 0 blends out of: the two swap, the fade turned round.
                int len = RecShort(rec, MOTION_REC_BLEND_IN) * 100, t;
                SetCharaShort(ch, CHARA_MOTION_SLOTS + CHARA_SLOT_BLEND * 2, body);
                SetCharaShort(ch, CHARA_SLOT_PREV, body);
                SetCharaShort(ch, CHARA_MOTION_SLOTS, motion);
                SetCharaByte(ch, CHARA_BEHAVIOR_CHANGE, CHARA_SLOT_CHANGE_START);
                SetSlotFade(ch, CHARA_SLOT_BODY, 0);
                t = CharaInt(ch, CHARA_SLOT_PLAYABLE + CHARA_SLOT_BLEND * 4);
                SetCharaInt(ch, CHARA_SLOT_PLAYABLE + CHARA_SLOT_BLEND * 4, CharaInt(ch, CHARA_SLOT_PLAYABLE));
                SetCharaInt(ch, CHARA_SLOT_PLAYABLE, t);
                SetCharaByte(ch, CHARA_BEHAVIOR_CHANGE + CHARA_SLOT_BLEND, CHARA_SLOT_CHANGE_START);
                SetCharaInt(ch, CHARA_SLOT_FADE_LEN + CHARA_SLOT_BLEND * 4, len);
                SetCharaInt(ch, CHARA_SLOT_FADE + CHARA_SLOT_BLEND * 4, -len);
                asResetCharacterPosition(ch);
                break;
            }
            if (body == -1) {
                SetCharaInt(ch, CHARA_SLOT_FADE_LEN + CHARA_SLOT_BLEND * 4, 0);
                SetCharaInt(ch, CHARA_SLOT_FADE + CHARA_SLOT_BLEND * 4, 0);
                DropBlendSlot(ch);
            } else {
                // Slot 0's motion moves to the blend slot with its time and fades out there.
                int len = RecShort(rec, MOTION_REC_BLEND_IN) * 100;
                SetCharaInt(ch, CHARA_SLOT_FADE_LEN + CHARA_SLOT_BLEND * 4, len);
                SetCharaInt(ch, CHARA_SLOT_FADE + CHARA_SLOT_BLEND * 4, -len);
                SetCharaShort(ch, CHARA_SLOT_PREV + CHARA_SLOT_BLEND * 2, blendMotion);
                SetCharaShort(ch, CHARA_MOTION_SLOTS + CHARA_SLOT_BLEND * 2, body);
                SetCharaInt(ch, CHARA_SLOT_PLAYABLE + CHARA_SLOT_BLEND * 4, CharaInt(ch, CHARA_SLOT_PLAYABLE));
                SetCharaInt(ch, CHARA_SLOT_TIME + CHARA_SLOT_BLEND * 4, CharaInt(ch, CHARA_SLOT_TIME));
                SetCharaByte(ch, CHARA_BEHAVIOR_CHANGE + CHARA_SLOT_BLEND, CHARA_SLOT_CHANGE_START);
            }
            SetSlotFade(ch, CHARA_SLOT_BODY, 0);
            asBehaviorStartAnimationFromTop(bhv, go != NULL, 1);
            asChangeBehaviorAnimationPlayRate(bhv, rec[MOTION_REC_PLAY_RATE]);
            SetSlotMotion(ch, CHARA_SLOT_BODY, motion, bhv, CHARA_SLOT_CHANGE_START, 1);
            {
                // The new motion starts as far in as the old one was (time / playable of the blend slot).
                int bl = CharaInt(ch, CHARA_SLOT_PLAYABLE + CHARA_SLOT_BLEND * 4);
                int bt = CharaInt(ch, CHARA_SLOT_TIME + CHARA_SLOT_BLEND * 4);
                if (bl != 0 && bt < bl)
                    asChangeBehaviorAnimationCurrent(bhv, bt * CharaInt(ch, CHARA_SLOT_PLAYABLE) / bl);
            }
            asResetCharacterPosition(ch);
            break;
        }
        case CHARA_MOTION_REQ_BLEND_ONLY:
            if (blendMotion == motion || slot != CHARA_SLOT_BODY)
                break;
            asBehaviorStartAnimationFromTop(bhv, go != NULL, 1);
            asChangeBehaviorAnimationPlayRate(bhv, rec[MOTION_REC_PLAY_RATE]);
            SetCharaShort(ch, CHARA_SLOT_PREV + CHARA_SLOT_BLEND * 2, blendMotion);
            SetCharaShort(ch, CHARA_MOTION_SLOTS + CHARA_SLOT_BLEND * 2, motion);
            SetCharaByte(ch, CHARA_BEHAVIOR_CHANGE + CHARA_SLOT_BLEND, CHARA_SLOT_CHANGE_START);
            SetCharaInt(ch, CHARA_SLOT_PLAYABLE + CHARA_SLOT_BLEND * 4, BehaviorPlayable(bhv));
            break;
        }
        SetCharaShort(ch, CHARA_MOTION_QUEUE + q * 2, -1);
        SetCharaShort(ch, CHARA_MOTION_QUEUE_MODE + q * 2, -1);
    }
    {
        short body = CharaShort(ch, CHARA_MOTION_SLOTS);
        const u_char *rec = body != -1 ? MotionRecord(ch, body) : NULL;
        SetCharaByte(ch, CHARA_MOTION_GROUP, rec != NULL ? rec[MOTION_REC_GROUP] : 0xff);
    }
}

// CT3 0x018fb3b0
int asCatWalkEventOperation(int mode)
{
    float r = rndf();   // drawn on every call, as the ROM does
    int i;
    if (mode == CATWALK_CHECK) {
        for (i = 0; i < CATWALK_TIMES; i++) {
            if (iBeforeTime < kCatWalkTimes[i] && kCatWalkTimes[i] <= iNowTime && (int)(r * 16.0f) <= i)
                return 1;
        }
        return kCatWalkTimes[CATWALK_TIMES - 1] <= iNowTime;
    }
    if (mode == CATWALK_TICK) {
        iBeforeTime = iNowTime;
        iNowTime = (u_short)(iNowTime + gGlobalNextTicksInterval);
    } else if (mode == CATWALK_RESET) {
        iNowTime = 0;
        iBeforeTime = 0;
    }
    return 0;
}

// CT3 0x018f4c60
// The character rides the part it stands on (CHARA_TARGET's FORCE_MOVE_TARGET_PART of its lightBank)
// in the one room that has such a part (FORCE_MOVE_ROOM_HASH): its position goes through the part's
// move since last frame (world matrix * inverse of last frame's), its height kept; standing still
// (CHARA_MOVE_DELTA zero), it keeps its distance to the part's centre in the ground plane; its rot
// turns by the part's turn about Y.
void asCharacterForceMoveOperate(CHARACTER *ch)
{
    const u_char *target;
    short bank, idx;
    const u_char *hdr;
    const MODEL_PARTS *part;
    alignas(16) float inv[4][4];
    float *pos = (float *)&ch->pos;
    float y, dist = 0.0f, keep[4] = {0, 0, 0, 0}, moved[4], dir[4], axis[4] = {1.0f, 0, 0, 0}, t[4], len;
    float delta[3];
    int still, k, turn;
    if (ch->id == -1 || (ch->flags & CHARACTER_FLAG_REFRACTION) == 0 || asCheckAnimationExecutable_Character(ch->id) == 0)
        return;
    if (ch->roomHash != FORCE_MOVE_ROOM_HASH || (target = CharaTarget(ch)) == NULL)
        return;
    bank = ch->lightBank;
    memcpy(&idx, target + FORCE_MOVE_TARGET_PART, sizeof(idx));
    if (bank < 0 || idx < 0 || BankInfo[bank].state != BANK_STATE_LOADED)
        return;
    hdr = All_table_TBL[bank] != NULL ? BANK_PTR(const u_char *, bank, All_table_TBL[bank]->partsHeader) : NULL;
    if (hdr == NULL)
        return;     // PORT: the ROM reads through the word whatever it holds
    part = (const MODEL_PARTS *)(hdr + idx * PARTS_REC_SIZE);
    if (part->kind != PARTS_KIND_FORCE_MOVE)
        return;
    memcpy(inv, (const u_char *)part + PARTS_PREV_MATRIX, sizeof(inv));
    gaInversMatrix(inv, inv);
    y = pos[1];
    pos[3] = 1.0f;
    memcpy(delta, (const u_char *)ch + CHARA_MOVE_DELTA, sizeof(delta));
    still = delta[0] == 0.0f && delta[1] == 0.0f && delta[2] == 0.0f;
    if (still) {
        float flat[4] = {pos[0], 0.0f, pos[2], pos[3]};
        keep[0] = part->pos[0];
        keep[2] = part->pos[2];
        keep[3] = part->pos[3];
        dist = asVectorDistance2(flat, keep);
    }
    // VU0: the position and the X axis through the inverse of last frame's matrix, then this frame's.
    for (k = 0; k < 4; k++)
        t[k] = inv[0][k] * pos[0] + inv[1][k] * pos[1] + inv[2][k] * pos[2] + inv[3][k] * pos[3];
    for (k = 0; k < 4; k++)
        moved[k] = part->matrix[0][k] * t[0] + part->matrix[1][k] * t[1] + part->matrix[2][k] * t[2] +
                   part->matrix[3][k] * t[3];
    memcpy(pos, moved, sizeof(moved));
    for (k = 0; k < 4; k++)
        t[k] = inv[0][k] * axis[0] + inv[1][k] * axis[1] + inv[2][k] * axis[2] + inv[3][k] * axis[3];
    for (k = 0; k < 4; k++)
        dir[k] = part->matrix[0][k] * t[0] + part->matrix[1][k] * t[1] + part->matrix[2][k] * t[2] +
                 part->matrix[3][k] * t[3];
    if (still) {
        float d[4];
        pos[1] = 0.0f;
        for (k = 0; k < 4; k++)
            d[k] = pos[k] - keep[k];
        len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);   // VU0 vrsqrt of x + y + z squared
        for (k = 0; k < 4; k++)
            pos[k] = keep[k] + d[k] / len * dist;
    }
    pos[1] = y;
    pos[3] = 0.0f;
    // The turn: the X axis' ground-plane direction, its angle from X by the side of Z.
    dir[1] = 0.0f;
    dir[3] = 0.0f;
    len = sqrtf(dir[0] * dir[0] + dir[2] * dir[2]);
    turn = ahAcos(dir[0] / len);
    if (dir[2] / len > 0.0f)
        ch->rot += turn;
    else if (dir[2] / len < 0.0f)
        ch->rot -= turn;
}

// ---- slice 4 (2026-11-10): the per-frame motion play, the small key-table entries ----

// The slot's motion ended this frame: queue what follows it. A record whose MOTION_REC_NEXT_MODE is
// >= 0 loops (stop, then `restart` the same motion); else its MOTION_REC_NEXT plays (-1: it just stops).
// The blend slot stops with CHARA_MOTION_REQ_STOP_BLEND and restarts with _BLEND_ONLY; slot 0..4
// stop with _STOP and restart with _RESTART, and a next motion comes as a plain _PLAY (no stop first).
static void QueueAfterMotionEnd(CHARACTER *ch, int slot, short motion, const u_char *rec)
{
    short next = RecShort(rec, MOTION_REC_NEXT);
    if (slot == CHARA_SLOT_BLEND) {
        asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_STOP_BLEND, motion);
        if (RecShort(rec, MOTION_REC_NEXT_MODE) >= 0)
            asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_BLEND_ONLY, motion);
        else if (next != -1)
            asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_BLEND_ONLY, next);
        return;
    }
    if (RecShort(rec, MOTION_REC_NEXT_MODE) >= 0) {
        asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_STOP, motion);
        asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_RESTART, motion);
    } else if (next == -1) {
        asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_STOP, motion);
    } else {
        asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_PLAY, next);
    }
}

// CT3 0x018f7430
// Plays every slot's motion one frame (ExecuteAnimation's play pass, its fade through the parts
// header's PARTS_HDR_ANIM_* words), queues what follows a motion that ended (CHARA_SLOT_ENDED and
// the slot's CHARA_BEHAVIOR_CHANGE set), then carries the BasePoint's root: slot 0's matrix gives the
// new root (CHARA_MOTION_ROOT) and base rot; with root motion on (CHARA_ROOT_MOVE * the record's
// MOTION_REC_ROOT_MOVE), the root's move since last frame, turned by rot - base rot, moves pos and
// rot (CHARA_MOVE_DELTA = the move), else CHARA_MOVE_DELTA = (0, 0, 0, 1). 1 when a motion ended.
int asCharacterAnimationExecute(CHARACTER *ch)
{
    // _1845 (sdata 0x0193bc20): the identity ExecuteAnimation's matrix starts from; the slots share it.
    alignas(16) float m[4][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}};
    int ended = 0, rootMove = 0, slot, k;
    ALL_TABLE *tbl;
    u_char *hdr;
    const u_char *rec0;
    if (ch->bank < 0 || (tbl = All_table_TBL[ch->bank]) == NULL)
        return 0;   // PORT: the ROM reads All_table_TBL[bank] whatever it holds
    hdr = BANK_PTR(u_char *, ch->bank, tbl->partsHeader);
    // PORT: the ROM reads slot 0's record with a table whatever slot 0 holds (-1: the word before it).
    rec0 = CharaShort(ch, CHARA_MOTION_SLOTS) != -1 ? MotionRecord(ch, CharaShort(ch, CHARA_MOTION_SLOTS)) : NULL;
    if (rec0 != NULL)
        rootMove = ((const u_char *)ch)[CHARA_ROOT_MOVE] * rec0[MOTION_REC_ROOT_MOVE];
    for (slot = 0; slot < CHARA_BHV_SLOT_COUNT; slot++) {
        short motion;
        SetCharaByte(ch, CHARA_SLOT_ENDED + slot, 0);
        motion = CharaShort(ch, CHARA_MOTION_SLOTS + slot * 2);
        if (motion != -1) {
            const u_char *rec = MotionRecord(ch, motion);
            BEHAVIOR *bhv = rec != NULL ? asFindBehaviorByCharacter(ch->id, RecHash(rec)) : NULL;
            int bank = bhv != NULL ? BankOfHostPtr(bhv) : -1;
            u_int anim = bhv != NULL ? BankEeAddr(bank, bhv) : 0;
            int abs = 0, sign = 0, done = 0, step;
            AbsSignI32(CharaInt(ch, CHARA_SLOT_FADE + slot * 4), &abs, &sign);
            step = CharaInt(ch, CHARA_SLOT_FADE_LEN + slot * 4);
            if (hdr != NULL) {
                memcpy(hdr + PARTS_HDR_ANIM_ABS, &abs, sizeof(abs));
                memcpy(hdr + PARTS_HDR_ANIM_SIGN, &sign, sizeof(sign));
                memcpy(hdr + PARTS_HDR_ANIM_STEP, &step, sizeof(step));
            }
            // PORT: no behaviour or no table on the host plays nothing (the ROM goes on with the word).
            if (bhv != NULL && rec != NULL) {
                int atKey = (((const u_char *)ch)[CHARA_BEHAVIOR_CHANGE + slot] & CHARA_SLOT_CHANGE_AT_KEY) != 0;
                done = ExecuteAnimation(bank, anim, tbl, m, 1, 0, 0, atKey);
            }
            if (hdr != NULL) {
                // the fade left back from the header (ExecuteAnimation counted it down)
                memcpy(&abs, hdr + PARTS_HDR_ANIM_ABS, sizeof(abs));
                memcpy(&sign, hdr + PARTS_HDR_ANIM_SIGN, sizeof(sign));
                SetCharaInt(ch, CHARA_SLOT_FADE + slot * 4, abs * sign);
                step = 0;
                memcpy(hdr + PARTS_HDR_ANIM_STEP, &step, sizeof(step));
            }
            if (slot == CHARA_SLOT_BLEND && CharaInt(ch, CHARA_SLOT_FADE + CHARA_SLOT_BLEND * 4) == 0) {
                // the crossfade is over: the blend slot's motion goes
                asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_STOP_BLEND, motion);
            } else {
                if (done == 1 && rec != NULL) {
                    QueueAfterMotionEnd(ch, slot, motion, rec);
                    ended = 1;
                    SetCharaByte(ch, CHARA_SLOT_ENDED + slot, 1);
                    SetCharaByte(ch, CHARA_BEHAVIOR_CHANGE + slot, CHARA_SLOT_CHANGE_START);
                }
                SetCharaInt(ch, CHARA_SLOT_TIME + slot * 4, bhv != NULL ? asGetBehaviorPlayingTicks(bank, anim) / 100 : 0);
            }
        }
        if (slot == CHARA_SLOT_BODY && hdr != NULL) {
            u_char *part;
            memcpy((u_char *)ch + CHARA_ROOT_PREV, (u_char *)ch + CHARA_MOTION_ROOT, 16);
            SetCharaInt(ch, CHARA_BASE_ROT_PREV, CharaInt(ch, CHARA_BASE_ROT));
            part = asFindModelParts(hdr, PARTS_HASH_BASE_POINT);
            if (part != NULL) {
                // m[3][3]'s word holds ExecuteAnimation's done bytes; the VU0 product takes w = 1.0.
                u_int doneBytes;
                memcpy(&doneBytes, &m[3][3], sizeof(doneBytes));
                m[3][3] = 1.0f;
                if (EA_DONE_ROT_BYTES(doneBytes) != 0)
                    SetCharaInt(ch, CHARA_BASE_ROT,
                                (int)((u_int)(int)((-asRadianByO(m[1], ROTATE_AXIS_Y) * ROT_FULL_TURN) / 6.2831855f) & 0xffff));
                if (doneBytes != 0) {
                    float v[4], r[4];
                    memcpy(v, part + PARTS_REC_ROOT_VEC, sizeof(v));
                    for (k = 0; k < 4; k++)
                        r[k] = m[0][k] * v[0] + m[1][k] * v[1] + m[2][k] * v[2] + m[3][k] * v[3];
                    memcpy((u_char *)ch + CHARA_MOTION_ROOT, r, sizeof(r));
                }
                if (rootMove == 0) {
                    const float vf0[4] = {0.0f, 0.0f, 0.0f, 1.0f};
                    memcpy((u_char *)ch + CHARA_MOVE_DELTA, vf0, sizeof(vf0));
                } else {
                    alignas(16) float rm[4][4];
                    float root[4], prev[4], d[4], mv[4];
                    float *pos = (float *)&ch->pos;
                    gaSetRotMatrixY(rm, ch->rot - CharaInt(ch, CHARA_BASE_ROT));
                    memcpy(root, (u_char *)ch + CHARA_MOTION_ROOT, sizeof(root));
                    memcpy(prev, (u_char *)ch + CHARA_ROOT_PREV, sizeof(prev));
                    for (k = 0; k < 4; k++)
                        d[k] = root[k] - prev[k];
                    for (k = 0; k < 4; k++)
                        mv[k] = rm[0][k] * d[0] + rm[1][k] * d[1] + rm[2][k] * d[2] + rm[3][k] * d[3];
                    for (k = 0; k < 4; k++)
                        pos[k] += mv[k];
                    ch->rot += CharaInt(ch, CHARA_BASE_ROT) - CharaInt(ch, CHARA_BASE_ROT_PREV);
                    memcpy((u_char *)ch + CHARA_MOVE_DELTA, mv, sizeof(mv));
                }
            }
        }
    }
    return ended;
}

// Damaging's turn: Alyssa's motion becomes `front` when the hit (from last frame's position) lies
// within 90 degrees of her facing, else `back`, and she turns to face it (back: away from it). The
// facing is row 2 of her Y rotation; the new rot is the hit's angle from +Z or -Z.
static void TurnToDamage(CHARACTER *a, const float *hit, int front, int back, int vibrate, int jitter)
{
    const float *from = (const float *)a->posBefore;
    alignas(16) float t[4], face[4], m[4][4];
    u_int angle;
    int k;
    for (k = 0; k < 4; k++)
        t[k] = hit[k] - from[k];
    gaSetRotMatrixY(m, a->rot);
    memcpy(face, m[2], sizeof(face));
    angle = (u_int)asRadianByVectors(t, face, ROTATE_AXIS_Y) & 0xffff;
    face[0] = face[1] = face[2] = 0.0f;
    face[3] = 1.0f;
    if (angle < 0x4000 || angle >= 0xc000) {
        asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, front);
        face[2] = 1.0f;
    } else {
        asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, back);
        face[2] = -1.0f;
    }
    if (vibrate)
        asDualShockVibrationOn_SuperLarge(ALYSSA_HIT_VIBRATE_TIME);
    a->rot = asRadianByVectors(t, face, ROTATE_AXIS_Y) & 0xffff;
    if (jitter)
        a->rot += (FrameCounter & 1) != 0 ? DAMAGE_JITTER_ROT : -DAMAGE_JITTER_ROT;
}

// CT3 0x018f46e0
// Alyssa damaged (the BALOWS boss' brain): in two motions a fixed follow-up (MOTION_DAMAGE_EF ->
// _EF_NEXT, MOTION_DAMAGE_68 -> _68_NEXT); else a push of 10 away from the hit and, with no panic
// barrier, targetState bit 0 clear and no boss battle, the damage motion (MOTION_DAMAGE_FRONT / _BACK,
// a super-large vibration, the turn; flags' DAMAGE_JITTER: 1/8 turn either way by the frame's parity);
// else Hiting's reactions: the boss battle's fatal motion, the flinch (barrier down, bit 0 set), or the
// fatal blow (PlayerDamage unless TRIG_EVENT_NO_FATAL_HIT).
void asChangeAlyssaMortionByDamaging(const float *hit, u_long flags)
{
    CHARACTER *a = &Character[CHARA_ALYSSA];
    float *pos = (float *)a->pos;
    float d[4], q;
    int panicked, front, back, k;
    short motion;
    panicked = (PanicInfoData.mode & PANIC_MODE_FULL) != 0 ? (u_char)Kekkai_Check() : 0;
    if (GlobalCameraMode != CAMERA_MODE_PLAY)
        return;
    motion = CharaShort(a, CHARA_MOTION_SLOTS);
    if (motion == MOTION_DAMAGE_EF) {
        asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, MOTION_DAMAGE_EF_NEXT);
        return;
    }
    if (motion == MOTION_DAMAGE_68) {
        asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, MOTION_DAMAGE_68_NEXT);
        return;
    }
    // VU0: d = hit - pos, xyz normalised (vrsqrt of x + y + z squared), times 10, added to pos.
    for (k = 0; k < 4; k++)
        d[k] = hit[k] - pos[k];
    q = 1.0f / sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    for (k = 0; k < 3; k++)
        d[k] *= q;
    for (k = 0; k < 4; k++)
        pos[k] += d[k] * ALYSSA_HIT_PUSH;
    if (!panicked && (a->targetState & 1) == 0 && ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) == 0) {
        if (motion != MOTION_DAMAGE_BACK && motion != MOTION_DAMAGE_FRONT)
            TurnToDamage(a, hit, MOTION_DAMAGE_FRONT, MOTION_DAMAGE_BACK, 1, (flags & DAMAGE_JITTER) != 0);
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) != 0) {
        back = MOTION_FATAL_BACK;
        front = MOTION_FATAL_FRONT;
        asDualShockVibrationOn_SuperLarge(ALYSSA_HIT_VIBRATE_TIME);
    } else if (!panicked && (a->targetState & 1) != 0) {
        back = MOTION_FLINCH_BACK;
        front = MOTION_FLINCH_FRONT;
        asDualShockVibrationOn_Midium(ALYSSA_HIT_VIBRATE_TIME);
    } else {
        back = MOTION_FATAL_BACK;
        front = MOTION_FATAL_FRONT;
        asDualShockVibrationOn_SuperLarge(ALYSSA_HIT_VIBRATE_TIME);
        if (ssTrigCheckEvent(TRIG_EVENT_NO_FATAL_HIT, 1, 0) == 0)
            PlayerDamage(ALYSSA_FATAL_DAMAGE);
    }
    if (motion != back && motion != front)
        TurnToDamage(a, hit, front, back, 0, 0);
}

// CT3 0x018f7fa0
// Crawling (HAIHAI): with the key's PIA_KEY_H0A at 1 in the crawl motion, a step: the second motion
// while PIA_KEY_H0C is set, else the first.
void asPIA1_KEY_TBL_HAIHAI(CHARACTER *ch, u_char *key)
{
    short h0a, h0c;
    memcpy(&h0a, key + PIA_KEY_H0A, sizeof(h0a));
    memcpy(&h0c, key + PIA_KEY_H0C, sizeof(h0c));
    if (h0a == 1 && CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_HAIHAI)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, h0c == 0 ? MOTION_HAIHAI_STEP : MOTION_HAIHAI_STEP2);
}

// CT3 0x018f8710
void asPIA1_KEY_TBL_PSITGO(void)
{
}

// CT3 0x018f8720
int asPIA0_KEY_TBL_PSITGO(CHARACTER *ch)
{
    (void)ch;
    return 1;
}

// CT3 0x018f8730
void asPIA1_KEY_TBL_PSIT(void)
{
}

// CT3 0x018f8740
int asPIA0_KEY_TBL_PSIT(CHARACTER *ch)
{
    (void)ch;
    return 1;
}

// CT3 0x018f8850
int asPIA0_KEY_TBL_PSTAND(CHARACTER *ch)
{
    (void)ch;
    return 1;
}

// CT3 0x018f8fb0
int asPIA0_KEY_TBL_KAIDAN(CHARACTER *ch)
{
    (void)ch;
    return 1;
}

// CT3 0x018f8fc0
void asPIA1_KEY_TBL_DOOR(void)
{
}

// CT3 0x018f8fd0
int asPIA0_KEY_TBL_DOOR(CHARACTER *ch)
{
    (void)ch;
    return 0;
}

// CT3 0x018f96b0
int asPIA0_KEY_TBL_SITGONOUP(CHARACTER *ch)
{
    (void)ch;
    return 1;
}

// CT3 0x018f9730
void asPIA1_KEY_TBL_GOUP1M(void)
{
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018f55d0
void asCharacterMortionFunctionOperation(CHARACTER *ch)
{
    ALYSSAPAN_STUB("asCharacterMortionFunctionOperation", 0x018f55d0);
}

// stub of CT3 0x018f6b50
void asMotionAdderByNowMotion(CHARACTER *ch)
{
    ALYSSAPAN_STUB("asMotionAdderByNowMotion", 0x018f6b50);
}

// stub of CT3 0x018f79f0
void asPlayerInputAnalyze(CHARACTER *ch)
{
    ALYSSAPAN_STUB("asPlayerInputAnalyze", 0x018f79f0);
}


// ---- asCheckCharacterHited's helpers (the ROM inlines each at every use) --------------------------------

// Her cry: one of three voices at random.
static void AlyssaScream(void)
{
    SdReqPlaySE(PANIC_SE_JLS, (int)SdRnd() % ALYSSA_SCREAM_COUNT + ALYSSA_SCREAM_SE, ALYSSA_SCREAM_ARG);
}

// The attacker's HIT_KIND_* byte. PORT: rec->attacker is an EE word; the attacker belongs to Character[who]'s
// bank (open-items.md 1 ap): resolved there, and a character without a bank answers 0 (no hit).
static int HitAttackerKind(const HIT_REC *rec)
{
    int bank = Character[rec->who].bank;
    const u_char *obj;
    if (bank < 0)
        return 0;
    obj = BANK_PTR(const u_char *, bank, rec->attacker);
    return obj != NULL ? obj[HIT_ATTACKER_KIND] : 0;
}

// Whether a character's blow hurts now: HIT_KIND_PANIC in a boss battle or a full panic, HIT_KIND_NORMAL
// out of both (the ROM asks TRIG_EVENT_BOSS_BATTLE twice; it has no side effect).
static int HitHurts(const HIT_REC *rec)
{
    int kind = HitAttackerKind(rec);
    int boss = ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) != 0;
    int panic = (PanicInfoData.mode & PANIC_MODE_FULL) != 0;
    if (kind == HIT_KIND_PANIC && (boss || panic))
        return 1;
    return !panic && !boss && kind == HIT_KIND_NORMAL;
}

// Marks the attacker's blow as landed.
static void HitLanded(int who)
{
    SetCharaInt(&Character[who], CHARA_HIT_LANDED, 1);
}

// The first four bytes of the bank's name as one word (HIT_BANK_*).
static u_int BankName4(int bank)
{
    u_int w;
    memcpy(&w, BankInfo[bank].name, sizeof(w));
    return w;
}

// CT3 0x018fc2f0
void asCheckCharacterHited(int chara)
{
    CHARACTER *ch = &Character[chara];
    CHARACTER *alyssa = &Character[CHARA_ALYSSA];
    short motion;
    const ALL_TABLE *tbl;
    const HIT_TABLE *hits;
    const HIT_REC *recs;
    const u_char *mrec;
    int count, i, canFlinch, notStairs, kind;

    if (ch->bank == -1)
        return;
    if (ssAI_IsDoorOpenPlaying(ch) != 0)
        return;
    motion = CharaShort(alyssa, CHARA_MOTION_SLOTS);
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) == 0) {
        if (chara == CHARA_ALYSSA) {
            // Out of the boss battle: the invulnerability after a panic started, then the no-hit states.
            vsBossMuteki = 0;
            ssTrigRemoveEvent(TRIG_EVENT_MUTEKI, 1, 0);
            if (AfterPanicStartMuteki > MUTEKI_MAX - 1)
                AfterPanicStartMuteki = MUTEKI_MAX;
            if (AfterPanicStartMuteki - gGlobalNextTicksInterval >= 0) {
                AfterPanicStartMuteki -= gGlobalNextTicksInterval;
                return;
            }
            AfterPanicStartMuteki = 0;
            if (ssTrigCheckEvent(TRIG_EVENT_NO_HIT_6F, 1, 0) != 0 ||
                ssTrigCheckEvent(TRIG_EVENT_NO_HIT_88, 1, 0) != 0 ||
                ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_8A, 1, 0) != 0 ||
                ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_4D, 1, 0) != 0 ||
                ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, 1, 0) != 0 ||
                ssTrigCheckEvent(TRIG_EVENT_NO_HIT_B0, 1, 0) != 0 ||
                ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_257, 1, 0) != 0)
                return;
            if (motion == MOTION_NO_HIT)
                return;
            if ((alyssa->flags & CHARACTER_FLAG_NO_HIT) != 0)
                return;
        }
    } else if (chara == CHARA_ALYSSA) {
        // The boss battle: its invulnerability and the stage boss's scripted strike.
        if (ssTrigCheckEvent(TRIG_EVENT_NO_HIT_B0, 1, 0) != 0)
            return;
        AfterPanicStartMuteki = 0;
        mrec = MotionRecord(alyssa, motion);
        if (mrec != NULL && mrec[MOTION_REC_B16] == MOTION_B16_NO_MUTEKI) {
            ssTrigRemoveEvent(TRIG_EVENT_MUTEKI, 1, 0);
        } else {
            if (vsBossMuteki > MUTEKI_MAX - 1)
                vsBossMuteki = MUTEKI_MAX;
            vsBossMuteki -= gGlobalNextTicksInterval;
            if (vsBossMuteki < 0) {
                vsBossMuteki = 0;
                ssTrigRemoveEvent(TRIG_EVENT_MUTEKI, 1, 0);
            } else {
                ssTrigRegisterEvent(TRIG_EVENT_MUTEKI, 1, 0);
            }
        }
        switch (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0)) {
        case 5:
            if (ssTrigCheckEvent(TRIG_EVENT_BOSS_STRIKE, 1, 0) != 0) {
                int action = ssTrigGetVariable(TRIG_VAR_BOSS_ACTION, 1, 0);
                if ((action == BOSS_ACTION_4 || action == BOSS_ACTION_E) && motion != MOTION_BOSS5_STRUCK)
                    asCharacterMortionChangeRequest(CHARA_ALYSSA, CHARA_MOTION_REQ_PLAY, MOTION_BOSS5_STRUCK);
            }
            break;
        case 4:
            if (ssTrigCheckEvent(TRIG_EVENT_BOSS_STRIKE, 1, 0) != 0) {
                ssTrigRemoveEvent(TRIG_EVENT_BOSS_STRIKE, 1, 0);
                mrec = MotionRecord(alyssa, motion);
                if (mrec == NULL || mrec[MOTION_REC_KIND] != MOTION_KIND_STAIRS) {
                    asChangeAlyssaMortionByHiting(
                        (const float *)((const u_char *)&Character[HIT_CHARA_SWO] + CHARA_BOSS_STRIKE),
                        ALYSSA_HIT_LETHAL);
                    ssTrigRegisterEvent(TRIG_EVENT_BOSS_STRUCK, 1, 0);
                    return;
                }
            }
            break;
        case 1:
            if (ssTrigCheckEvent(TRIG_EVENT_BOSS_STRIKE, 1, 0) != 0) {
                ssTrigRemoveEvent(TRIG_EVENT_BOSS_STRIKE, 1, 0);
                mrec = MotionRecord(alyssa, motion);
                kind = mrec != NULL ? mrec[MOTION_REC_KIND] : 0;
                if (kind != MOTION_KIND_STAIRS2 && kind != MOTION_KIND_STAIRS) {
                    asChangeAlyssaMortionByHiting(
                        (const float *)((const u_char *)&Character[HIT_CHARA_HAMMER] + CHARA_BOSS_STRIKE),
                        ALYSSA_HIT_LETHAL);
                    ssTrigRegisterEvent(TRIG_EVENT_BOSS_STRUCK, 1, 0);
                    return;
                }
            }
            break;
        default:                        /* 2, 3 and the rest: no scripted strike */
            break;
        }
        if (ssTrigCheckEvent(TRIG_EVENT_MUTEKI, 1, 0) != 0)
            return;
    }

    // The hit table (the ROM reads it before testing chara: no side effect).
    if (chara != CHARA_ALYSSA || (tbl = All_table_TBL[ch->bank]) == NULL)
        return;     // PORT: the ROM reads All_table_TBL[bank] whatever it holds
    hits = BANK_PTR(const HIT_TABLE *, ch->bank, tbl->hits);
    if (hits == NULL || (count = hits->count) <= 0)
        return;
    recs = BANK_PTR(const HIT_REC *, ch->bank, hits->recs);
    mrec = MotionRecord(alyssa, motion);
    kind = mrec != NULL ? mrec[MOTION_REC_KIND] : 0;
    // On the stairs the weaker blows do nothing (canFlinch), kind 3 also the middle ones (notStairs).
    canFlinch = kind != MOTION_KIND_STAIRS2 && kind != MOTION_KIND_STAIRS;
    notStairs = kind != MOTION_KIND_STAIRS;

    for (i = 0; i < count; i++) {
        const HIT_REC *rec = &recs[i];
        int who = rec->who;
        int attack = rec->attack;
        int landed = 0;

        if (rec->isObject == 0) {
            const float *pos;
            if (who == HIT_CHARA_32) {
                if (HitAttackerKind(rec) != HIT_KIND_PANIC)
                    continue;
            } else if (who >= HIT_CHARA_HAMMER && who <= HIT_CHARA_SWO) {
                if (!HitHurts(rec))
                    continue;
            } else {
                continue;
            }
            // The player's keys are stopped: the blow lands only once she has fallen in the boss battle.
            if (ssTrigCheckEvent(TRIG_EVENT_PLAYER_STOP, 1, 0) != 0) {
                if (ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) == 0)
                    return;
                HitLanded(who);
                return;
            }
            pos = (const float *)&Character[who].pos;
            switch (who) {
            case HIT_CHARA_32:
                if (attack == 0x1e || (attack == 0x1d && notStairs) || (attack == 8 && canFlinch)) {
                    HitLanded(who);
                    asChangeAlyssaMortionByDamaging(pos, DAMAGE_JITTER);
                    AlyssaScream();
                    landed = 1;
                }
                break;
            case HIT_CHARA_SWO:
            case HIT_CHARA_SMA: {
                // The two share their shape: a damage blow, a middle set, a flinch and a weak flinch.
                const char *se = who == HIT_CHARA_SWO ? "E_SWO" : "E_SMA";   /* @ 0x01937520 / 0x01937518 */
                int heavy = who == HIT_CHARA_SWO ? 0x2e : 0x2f;
                int flinch = who == HIT_CHARA_SWO ? 0x20 : 0x1e;
                int weak = who == HIT_CHARA_SWO ? 0x2b : 0x2c;
                int mid = who == HIT_CHARA_SWO ? (attack == 0x32 || attack == 0x31 || attack == 0x28)
                                                : (attack == 0x33 || attack == 0x32 || attack == 0x29);
                if (attack == heavy || (mid && notStairs)) {
                    HitLanded(who);
                    asChangeAlyssaMortionByDamaging(pos, DAMAGE_JITTER);
                    SSPlaySE(se, HIT_SE_ID, 1);
                    AlyssaScream();
                    landed = 1;
                } else if (attack == flinch) {
                    if (notStairs) {
                        HitLanded(who);
                        asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_PULL);
                        // The ROM tests attack == weak here, which cannot hold: no sound.
                        landed = 1;
                    }
                } else if (attack == weak && canFlinch) {
                    HitLanded(who);
                    asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_LETHAL);
                    SSPlaySE(se, HIT_SE_ID, 1);
                    AlyssaScream();
                    landed = 1;
                }
                break;
            }
            case HIT_CHARA_ONO:
                if (attack == 0x33) {
                    HitLanded(who);
                    asChangeAlyssaMortionByDamaging(pos, DAMAGE_JITTER);
                    AlyssaScream();
                    landed = 1;
                } else if (attack == 0x44 || attack == 0x37 || attack == 0x36 || attack == 0x2c) {
                    if (notStairs) {
                        HitLanded(who);
                        asChangeAlyssaMortionByDamaging(pos, DAMAGE_JITTER);
                        SSPlaySE("E_ONO", HIT_SE_ID, 1);                  /* @ 0x01937510 */
                        AlyssaScream();
                        landed = 1;
                    }
                } else if (attack == 0x20) {
                    if (notStairs) {
                        HitLanded(who);
                        asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_PULL);
                        landed = 1;
                    }
                } else if ((attack == 0x45 || attack == 0x30) && canFlinch) {
                    HitLanded(who);
                    asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_LETHAL);
                    SSPlaySE("E_ONO", HIT_SE_ID, 1);
                    AlyssaScream();
                    landed = 1;
                }
                break;
            case HIT_CHARA_RYUSAN:
                if (attack == 0x2e) {
                    HitLanded(who);
                    asChangeAlyssaMortionByDamaging(pos, DAMAGE_JITTER);
                    AlyssaScream();
                    landed = 1;
                } else if (attack == 0x31 || attack == 0x28) {
                    if (notStairs) {
                        HitLanded(who);
                        asChangeAlyssaMortionByDamaging(pos, DAMAGE_JITTER);
                        AlyssaScream();
                        landed = 1;
                    }
                } else if (attack == 0x20) {
                    if (notStairs) {
                        HitLanded(who);
                        asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_PULL);
                        AlyssaScream();
                        landed = 1;
                    }
                } else if (attack == 5 || attack == 0x3b || attack == 0x3a || attack == 0x1e || attack == 0x32) {
                    if (canFlinch) {
                        HitLanded(who);
                        asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_LETHAL);
                        AlyssaScream();
                        landed = 1;
                    }
                } else if (attack == 0x2b) {
                    // Out of the boss battle a flinch; in it, damage.
                    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) == 0) {
                        if (canFlinch) {
                            HitLanded(who);
                            asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_LETHAL);
                            AlyssaScream();
                            landed = 1;
                        }
                    } else if (notStairs) {
                        HitLanded(who);
                        asChangeAlyssaMortionByDamaging(pos, DAMAGE_JITTER);
                        AlyssaScream();
                        landed = 1;
                    }
                }
                break;
            case HIT_CHARA_HAMMER:
                if (attack == 0x5c || ((attack == 0x40 || attack == 0x61 || attack == 0x48 || attack == 0x1d) &&
                                       notStairs)) {
                    HitLanded(who);
                    asChangeAlyssaMortionByDamaging(pos, DAMAGE_JITTER);
                    SSPlaySE("E_HAMMER", HIT_SE_ID_HAMMER, 1);           /* @ 0x01937500 */
                    AlyssaScream();
                    landed = 1;
                } else if (attack == 0x11) {
                    if (notStairs) {
                        HitLanded(who);
                        asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_PULL);
                        SSPlaySE("E_HAMMER", HIT_SE_ID_HAMMER, 1);
                        AlyssaScream();
                        landed = 1;
                    }
                } else if (attack == 0x27 && canFlinch) {
                    HitLanded(who);
                    asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_LETHAL);
                    SSPlaySE("E_HAMMER", HIT_SE_ID_HAMMER, 1);
                    AlyssaScream();
                    landed = 1;
                }
                break;
            }
            if (landed)
                break;      /* one character blow a frame */
        } else {
            // A part of an object bank: the bank's name says what it is. No break: every record counts.
            int bank = rec->bank;
            const MODEL_PARTS *parts = (const MODEL_PARTS *)pModelPartsHeader[bank];
            const float *pos = parts != NULL ? parts[who].pos : NULL;
            u_int name = BankName4(bank);
            if (pos == NULL)
                continue;   // PORT: the ROM reads the part whatever the header holds
            if (name == HIT_BANK_E_93) {
                if (attack == 0 && canFlinch) {
                    asChangeAlyssaMortionByDamaging(pos, 0);
                    ssTrigSetVariable(TRIG_VAR_HIT_ATTACK, attack, 1, 0);
                    AlyssaScream();
                }
            } else if (name == HIT_BANK_E_14) {
                if (attack == 2 || attack == 0) {
                    HitLanded(HIT_CHARA_ONO);
                    asChangeAlyssaMortionByDamaging(pos, 0);
                    ssTrigSetVariable(TRIG_VAR_HIT_ATTACK, attack, 1, 0);
                    SSPlaySE("E_ONO", HIT_SE_ID, 1);
                    AlyssaScream();
                } else if (attack == 1 && canFlinch) {
                    HitLanded(HIT_CHARA_ONO);
                    asChangeAlyssaMortionByHiting(pos, ALYSSA_HIT_LETHAL);
                    ssTrigSetVariable(TRIG_VAR_HIT_ATTACK, attack, 1, 0);
                    SSPlaySE("E_ONO", HIT_SE_ID, 1);
                    AlyssaScream();
                }
            } else if (name == HIT_BANK_E_95) {
                if (attack >= 0 && attack < 0x12 && notStairs) {
                    asChangeAlyssaMortionByDamaging(pos, 0);
                    ssTrigSetVariable(TRIG_VAR_HIT_ATTACK, attack, 1, 0);
                    SSPlaySE("E_SMA", HIT_SE_ID, 1);
                    AlyssaScream();
                }
            } else if (name == HIT_BANK_SCSR) {
                if (attack > 0x11 && attack < 0x26) {
                    asChangeAlyssaMortionByDamaging(pos, 0);
                    ssTrigSetVariable(TRIG_VAR_HIT_ATTACK, attack, 1, 0);
                    SSPlaySE("E_SMA", HIT_SE_ID, 1);
                    AlyssaScream();
                }
            } else if (name == HIT_BANK_E_97) {
                if (attack >= 0 && attack <= 4 && canFlinch) {
                    asChangeAlyssaMortionByHirumi((const float *)&Character[HIT_CHARA_32].pos);
                    ssTrigSetVariable(TRIG_VAR_HIT_ATTACK, attack, 1, 0);
                    AlyssaScream();
                }
            }
        }
    }
}
