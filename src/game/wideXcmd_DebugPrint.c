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
#include "game/main2.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018f4c60
void asCharacterForceMoveOperate(CHARACTER *chr)
{
    ALYSSAPAN_STUB("asCharacterForceMoveOperate", 0x018f4c60);
}

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

// stub of CT3 0x018f7430
int asCharacterAnimationExecute(CHARACTER *ch)
{
    ALYSSAPAN_STUB("asCharacterAnimationExecute", 0x018f7430);
    return {};
}

// stub of CT3 0x018f79f0
void asPlayerInputAnalyze(CHARACTER *ch)
{
    ALYSSAPAN_STUB("asPlayerInputAnalyze", 0x018f79f0);
}


// stub of CT3 0x018fbc20
void asRequestExecute(CHARACTER *ch)
{
    ALYSSAPAN_STUB("asRequestExecute", 0x018fbc20);
}

// stub of CT3 0x018fc2f0
void asCheckCharacterHited(int chara)
{
    ALYSSAPAN_STUB("asCheckCharacterHited", 0x018fc2f0);
}
