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
// Slice 6 (2026-11-10): the player's key dispatch (asPlayerInputAnalyze and its tables asPIA0 / asPIA1 /
// asPIAK: every entry now takes (CHARACTER *, PIA_KEY *)), the small key-table entries, the boss's chains
// (asChainLengthCheck, asPIA0_KEY_TBL_PULLED) and the holy water check (asWater_Run_Check).
// Slice 7 (2026-11-10): the pad move (asCharacterMoveByPad: tank and camera-relative steering, the
// turn-around flag; asCharacterMoveByPadUD), the panic events (asPanicEventOperation), the face motions
// and blinks (asMotionAdderByNowMotion with the AM / pAM / AME / pAME tables) and the play-rate setter.
// Slice 10 (2026-11-11): what slot 0's motion record turns on (asCharacterMortionFunctionOperation:
// attack and no-hit windows, the run events, Alyssa's stairs / panic triggers, turn speed, lever mash,
// idle fidgets, panic motions and the catchers' holds) and asChangeCharacterMotionPlayRate: the TU's
// code is complete.
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
#include "game/asMothRouteSearch.h"
#include "game/asUseArrow.h"
#include "game/main.h"
#include "game/ahalloc.h"
#include "game/fileio.h"
#include "game/main2.h"
#include "game/sound.h"
#include "game/ssGrpPortalCheck.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/tiAvoidObject.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_TriggerInit.h"
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
void asPIA1_KEY_TBL_PULLED(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f8020
int asPIA0_KEY_TBL_HAIHAI(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    key->h0c = 1;
    return PIA_MOVE;
}

// CT3 0x018f8030
void asPIA1_KEY_TBL_WATER(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f8040
int asPIA0_KEY_TBL_WATER(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 0;
}

// CT3 0x018f8600
int asPIA0_KEY_TBL_LOCK(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 1;
}

// CT3 0x018f8610
void asPIA1_KEY_TBL_PMOVEEND(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f8620
int asPIA0_KEY_TBL_PMOVEEND(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 1;
}

// CT3 0x018f86d0
int asPIA0_KEY_TBL_PRUN(CHARACTER *ch, PIA_KEY *key)
{
    (void)key;
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

// asPanicEventOperation's state (shorts: the ROM counts ticks in 16 bits).
static short iRunCount;                 /* iRunCount$577, sbss 0x019449f4: ticks run in panic */
static short iLastRunCount;             /* iLastRunCount$578, sbss 0x019449f0 */
static short iNoMoveCount;              /* iNoMoveCount$579, sbss 0x019449ec: ticks in panic */
static short iLastNoMoveCount;          /* iLastNoMoveCount$580, sbss 0x019449e8 */
static short iScreamCount;              /* iScreamCount$581, sbss 0x019449e4 */
static short iWaitCount;                /* iWaitCount$582, sbss 0x019449e0: PANIC_WAIT after an event */
static short ExecTumble;                /* ExecTumble$583, sbss 0x019449dc: armed */
static short ExecNoMove;                /* ExecNoMove$584, sbss 0x019449d8: armed */
static short ExecScream;                /* ExecScream$585, sbss 0x019449d4: armed */
// The run times a tumble may arm at (@586) and the panic times a freeze may arm at (@587), in ticks;
// time i arms only when the frame's draw (0..15) is <= i, so the later times are likelier. The 17th
// entries are never read.
static const u_short kTumbleTimes[PANIC_TIMES] = { /* @586, sdata 0x0193ba80 */
    0x258, 0x2ee, 0x384, 0x384, 0x41a, 0x4b0, 0x546, 0x5dc, 0x672,
    0x708, 0x79e, 0x834, 0x8ca, 0x960, 0x9f6, 0xa8c, 0xb22,
};
static const u_short kFreezeTimes[PANIC_TIMES] = { /* @587, sdata 0x0193bab0 */
    0x384, 0x41a, 0x4b0, 0x4b0, 0x546, 0x5dc, 0x672, 0x708, 0x79e,
    0x834, 0x8ca, 0x960, 0x9f6, 0xa8c, 0xb22, 0xbb8, 0xc4e,
};

// 1 when one of the times falls in [last, now) at the frame's draw (the ROM's two loops).
static int PanicTimePassed(const u_short *times, short last, short now, int draw)
{
    int i, hit = 0;
    for (i = 0; i < PANIC_TIMES_READ; i++)
        if (last <= (int)times[i] && (int)times[i] < now && draw <= i)
            hit = 1;
    return hit;
}

// CT3 0x018fb560
// Alyssa's random panic events: PANIC_EV_TICK counts the ticks and arms a scream (every
// PANIC_SCREAM_PERIOD ticks, likelier the nearer the closest monster), a freeze (at kFreezeTimes) and,
// while she runs, a tumble (at kTumbleTimes); the PANIC_EV_TAKE_* modes hand one armed event out (then
// PANIC_WAIT ticks before the next can arm).
int asPanicEventOperation(int mode, int group)
{
    short ticks = (short)gGlobalNextTicksInterval;
    int draw = (int)(rndf() * 16.0f);   // drawn on every call, as the ROM does
    int ev = PANIC_EVENT_NONE;

    if (ssTrigCheckEvent(TRIG_EVENT_NO_PANIC_EVENT, 1, 0) != 0)
        return PANIC_EVENT_NONE;
    if (mode == PANIC_EV_TAKE_RUN) {
        if (iWaitCount < 1) {
            if (ExecScream != 0) {
                ExecScream = 0;
                iWaitCount = PANIC_WAIT;
                ev = PANIC_EVENT_SCREAM;
            } else if (ExecTumble != 0) {
                ExecTumble = 0;
                iRunCount = 0;
                iWaitCount = PANIC_WAIT;
                ev = PANIC_EVENT_TUMBLE;
            } else if (ExecNoMove != 0) {
                ExecNoMove = 0;
                iNoMoveCount = 0;
                iWaitCount = PANIC_WAIT;
                ev = PANIC_EVENT_FREEZE;
            }
        }
    } else if (mode == PANIC_EV_TAKE_STOP2 || mode == PANIC_EV_TAKE_STOP) {
        if (iWaitCount < 1) {
            if (ExecScream != 0) {
                ExecScream = 0;
                iWaitCount = PANIC_WAIT;
                ev = PANIC_EVENT_SCREAM;
            } else if (ExecNoMove != 0) {
                ExecNoMove = 0;
                iNoMoveCount = 0;
                iWaitCount = PANIC_WAIT;
                ev = PANIC_EVENT_FREEZE;
            }
        }
    } else if (mode == PANIC_EV_RESET) {
        ExecTumble = 0;
        ExecNoMove = 0;
        ExecScream = 0;
        iRunCount = 0;
        iNoMoveCount = 0;
        iWaitCount = 0;
    } else if (mode == PANIC_EV_TICK) {
        iWaitCount = (short)(iWaitCount - ticks);
        if (iWaitCount < 0) {
            iWaitCount = 0;
            if (ExecScream == 0) {
                iScreamCount = (short)(iScreamCount + ticks);
                if (iScreamCount > PANIC_SCREAM_PERIOD) {
                    int id;
                    float d;
                    iScreamCount = 0;
                    if (asMonsterDistanceStack_inPanic(MDS_NEAREST, &id, &d) > -1) {
                        // the nearer the monster, the likelier: always under 1, then 8 / 3 / 2 / 1
                        // chances in 16 under 2 / 3 / 4 / 5
                        if (d < 1.0f || (d < 2.0f && draw <= 7) || (d < 3.0f && draw <= 2) ||
                            (d < 4.0f && draw <= 1) || (d < 5.0f && draw < 1))
                            ExecScream = 1;
                    }
                }
            }
            if (ExecNoMove == 0) {
                short now = (short)(iNoMoveCount + ticks);
                iLastNoMoveCount = iNoMoveCount;
                if (PanicTimePassed(kFreezeTimes, iNoMoveCount, now, draw))
                    ExecNoMove = 1;
                iNoMoveCount = now;
            }
            if (ExecTumble == 0 && group == MOTION_GROUP_RUN) {
                short now = (short)(iRunCount + ticks);
                iLastRunCount = iRunCount;
                if (PanicTimePassed(kTumbleTimes, iRunCount, now, draw))
                    ExecTumble = 1;
                iRunCount = now;
            }
        }
    } else {
        return PANIC_EVENT_NONE;
    }
    return ev;
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

// CT3 0x018fe0e0
void asChangeCharacterMotionSettingPlayRate(int chara, int index, u_char rate)
{
    u_char *rec = MotionRecord(&Character[chara], index);
    if (rec != NULL)                    // PORT: no motion table on the host yet: nothing to set
        rec[MOTION_REC_PLAY_RATE] = rate;
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
// Crawling (HAIHAI): with the key's h0a at 1 in the crawl motion, a step: the second motion while h0c
// is set, else the first.
void asPIA1_KEY_TBL_HAIHAI(CHARACTER *ch, PIA_KEY *key)
{
    if (key->h0a == 1 && CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_HAIHAI)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, key->h0c == 0 ? MOTION_HAIHAI_STEP : MOTION_HAIHAI_STEP2);
}

// CT3 0x018f8710
void asPIA1_KEY_TBL_PSITGO(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f8720
int asPIA0_KEY_TBL_PSITGO(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 1;
}

// CT3 0x018f8730
void asPIA1_KEY_TBL_PSIT(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f8740
int asPIA0_KEY_TBL_PSIT(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 1;
}

// CT3 0x018f8850
int asPIA0_KEY_TBL_PSTAND(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 1;
}

// CT3 0x018f8fb0
int asPIA0_KEY_TBL_KAIDAN(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 1;
}

// CT3 0x018f8fc0
void asPIA1_KEY_TBL_DOOR(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f8fd0
int asPIA0_KEY_TBL_DOOR(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 0;
}

// CT3 0x018f96b0
int asPIA0_KEY_TBL_SITGONOUP(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return 1;
}

// CT3 0x018f9730
void asPIA1_KEY_TBL_GOUP1M(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// ---- slice 6 (2026-11-10): the player's key dispatch and the small key-table entries -------------------

// Motion indices the small entries test (names from these uses).
#define MOTION_STAND_HOLD 0x0c          /* asPIA0_KEY_TBL_STAND: no pad move in it */
#define MOTION_WALK_HOLD 0xc6           /* asPIA0_KEY_TBL_WALK: no pad move in it */
#define MOTION_RUN_HOLD 0xc7            /* asPIA0_KEY_TBL_RUN: no pad move in it */
#define MOTION_DOWN 0xd9                /* lying down: DOWN's gate */
#define MOTION_DOWN_GETUP_TIME 201      /* asPIA0_KEY_TBL_DOWN: slot 0's time from which the pad moves her */
#define MOTION_SITNOUP_STEP 0xb5        /* asPIA1_KEY_TBL_SITNOUP with h0c clear */
#define MOTION_SITNOUP_STEP2 0xc1       /* ... with h0c set */

// The record kinds (MOTION_REC_FACING) whose keys asPlayerInputAnalyze clears before the pad move, and
// those that do not move by the pad (no asPIAK entry); bit k = kind k.
#define PIA_KIND_NO_KEYS ((1u << 3) | (1u << 4) | (1u << 5) | (1u << 9) | (1u << 10) | (1u << 11) | (1u << 15) | (1u << 16))
#define PIA_KIND_NO_MOVE ((1u << 3) | (1u << 6) | (1u << 9) | (1u << 12))
// The kinds (and the motion groups 0x10 / 0x11) for which the targets are copied this frame.
#define PIA_KIND_COPY_TARGETS ((1u << 0) | (1u << 4) | (1u << 7) | (1u << 10) | (1u << 13))
#define PIA_GROUP_COPY_TARGETS 0x10
#define PIA_GROUP_COPY_TARGETS2 0x11

int iGacha;                             /* sbss 0x01944a08 */

typedef int (*PIA0_FUNC)(CHARACTER *ch, PIA_KEY *key);
typedef void (*PIA1_FUNC)(CHARACTER *ch, PIA_KEY *key);

// asPIA0 / asPIA1 (sdata 0x0193baf0 / 0x0193bb60), by CHARA_MOTION_GROUP.
static const PIA0_FUNC asPIA0[28] = {
    asPIA0_KEY_TBL_STAND,     asPIA0_KEY_TBL_SIT,      asPIA0_KEY_TBL_SITGO,     asPIA0_KEY_TBL_WALK,
    asPIA0_KEY_TBL_RUN,       asPIA0_KEY_TBL_MOVEEND,  asPIA0_KEY_TBL_CATCH,     asPIA0_KEY_TBL_HASHIGO,
    asPIA0_KEY_TBL_JUMPDOWN,  asPIA0_KEY_TBL_GOUP1M,   asPIA0_KEY_TBL_SITGONOUP, asPIA0_KEY_TBL_SITNOUP,
    asPIA0_KEY_TBL_DOWN,      asPIA0_KEY_TBL_YUMI,     asPIA0_KEY_TBL_DOOR,      asPIA0_KEY_TBL_KAIDAN,
    asPIA0_KEY_TBL_BEHIND,    asPIA0_KEY_TBL_SITBEHIND, asPIA0_KEY_TBL_PSTAND,   asPIA0_KEY_TBL_PSIT,
    asPIA0_KEY_TBL_PSITGO,    asPIA0_KEY_TBL_PRUN,     asPIA0_KEY_TBL_PMOVEEND,  asPIA0_KEY_TBL_LOCK,
    asPIA0_KEY_TBL_CATWALK,   asPIA0_KEY_TBL_WATER,    asPIA0_KEY_TBL_HAIHAI,    asPIA0_KEY_TBL_PULLED,
};
static const PIA1_FUNC asPIA1[28] = {
    asPIA1_KEY_TBL_STAND,     asPIA1_KEY_TBL_SIT,      asPIA1_KEY_TBL_SITGO,     asPIA1_KEY_TBL_WALK,
    asPIA1_KEY_TBL_RUN,       asPIA1_KEY_TBL_MOVEEND,  asPIA1_KEY_TBL_CATCH,     asPIA1_KEY_TBL_HASHIGO,
    asPIA1_KEY_TBL_JUMPDOWN,  asPIA1_KEY_TBL_GOUP1M,   asPIA1_KEY_TBL_SITGONOUP, asPIA1_KEY_TBL_SITNOUP,
    asPIA1_KEY_TBL_DOWN,      asPIA1_KEY_TBL_YUMI,     asPIA1_KEY_TBL_DOOR,      asPIA1_KEY_TBL_KAIDAN,
    asPIA1_KEY_TBL_BEHIND,    asPIA1_KEY_TBL_SITBEHIND, asPIA1_KEY_TBL_PSTAND,   asPIA1_KEY_TBL_PSIT,
    asPIA1_KEY_TBL_PSITGO,    asPIA1_KEY_TBL_PRUN,     asPIA1_KEY_TBL_PMOVEEND,  asPIA1_KEY_TBL_LOCK,
    asPIA1_KEY_TBL_CATWALK,   asPIA1_KEY_TBL_WATER,    asPIA1_KEY_TBL_HAIHAI,    asPIA1_KEY_TBL_PULLED,
};
// asPIAK (sdata 0x0193bbd0), by the motion record's MOTION_REC_FACING; NULL for the PIA_KIND_NO_MOVE kinds.
static const PIA1_FUNC asPIAK[PIAK_KIND_COUNT] = {
    asPIAK_BHVC1_MoveLR,  asPIAK_BHVC1_MoveUD,  asPIAK_BHVC1_Stop,     NULL,
    asPIAK_BHVC1_MoveLR,  asPIAK_BHVC1_MoveUD,  NULL,
    asPIAK_BHVC1_MoveLR,  asPIAK_BHVC1_MoveUD,  NULL,
    asPIAK_BHVC1_MoveLR,  asPIAK_BHVC1_MoveUD,  NULL,
    asPIAK_BHVC1_MoveLR,  asPIAK_BHVC1_MoveUD,  asPIAK_BHVC1_MoveBLR,
    asPIAK_BHVC1_MoveBLR, asPIAK_BHVC1_MoveBLR, asPIAK_BHVC1_MoveBLR,
};

static int KindIn(u_int set, int kind)
{
    return kind >= 0 && kind < 32 && (set >> kind & 1u) != 0;
}

// Copies a target object's vector, flags and part into the character at `off`; part -1 with none.
static void CopyTarget(CHARACTER *ch, int off, const u_char *target)
{
    if (target == NULL) {
        SetCharaShort(ch, off + CHARA_TARGET_COPY_PART, -1);
        return;
    }
    memcpy((u_char *)ch + off, target + CHARA_TARGET_VEC, sizeof(qword));
    SetCharaInt(ch, off + CHARA_TARGET_COPY_FLAGS, (int)TargetFlags(target));
    memcpy((u_char *)ch + off + CHARA_TARGET_COPY_PART, target + CHARA_TARGET_PART, sizeof(short));
}

// CT3 0x018f79f0
// The player's keys this frame: nothing while a forced action holds (CHARA_BHV_W518 != -1); with no
// motion group, the free control (PlayerControlUpdate2(1)). Else the targets are copied for the kinds
// that aim, asPIA0[group] decides, asPIAK[kind] moves by the pad, asPIA1[group] reacts.
void asPlayerInputAnalyze(CHARACTER *ch)
{
    PIA_KEY key;
    const u_char *rec;
    int group, kind, kind2, answer;

    key.h08 = 0;
    if (CharaInt(ch, CHARA_BHV_W518) != -1)
        return;
    group = (s_char)((const u_char *)ch)[CHARA_MOTION_GROUP];
    if (group == CHARA_MOTION_NONE_GROUP) {
        iGacha = 0;
        PlayerControlUpdate2(1);
        return;
    }
    PlayerControlUpdate2(0);
    rec = MotionRecord(ch, CharaShort(ch, CHARA_MOTION_SLOTS));
    // PORT: no table on the host yet: kind 0 (the ROM reads through the word)
    kind = rec != NULL ? (s_char)rec[MOTION_REC_FACING] : 0;
    kind2 = rec != NULL ? (s_char)rec[MOTION_REC_KIND] : 0;
    if (KindIn(PIA_KIND_COPY_TARGETS, kind) || group == PIA_GROUP_COPY_TARGETS || group == PIA_GROUP_COPY_TARGETS2) {
        if (ch->w300[1] == 0) {
            SetCharaShort(ch, CHARA_TARGET2_COPY + CHARA_TARGET_COPY_PART, -1);
        } else {
            memcpy((u_char *)ch + CHARA_TARGET2_POS, &ch->pos, sizeof(qword));
            SetCharaInt(ch, CHARA_TARGET2_ROT, ch->rot);
            CopyTarget(ch, CHARA_TARGET2_COPY, CharaTarget2(ch));
        }
        if (ch->w300[0] == 0)
            SetCharaShort(ch, CHARA_TARGET_COPY + CHARA_TARGET_COPY_PART, -1);
        else
            CopyTarget(ch, CHARA_TARGET_COPY, CharaTarget(ch));
    }
    key.h0a = 0;
    key.h0c = 0;
    key.pad0e = 0;
    key.cnt = pad_cnt_button[0];
    key.trg = pad_trg_button[0];
    answer = asPIA0[group](ch, &key);
    if (!asCharacterKeyEnable())
        return;
    if (answer == PIA_HOLD) {
        key.h0a = 0;
    } else if (answer == PIA_MOVE) {
        if (KindIn(PIA_KIND_NO_KEYS, kind)) {
            key.cnt = 0;
            key.trg = 0;
        }
        if (kind < -1 || kind >= PIAK_KIND_COUNT) {
            /* outside the ROM's jump table: straight to asPIA1 */
        } else if (kind == -1 || KindIn(PIA_KIND_NO_MOVE, kind)) {
            key.h0a = 0;
        } else if ((Character[0].targetState & 1) != 0 && kind2 == 1) {
            key.h0a = 0;
        } else if (ssTrigCheckEvent(TRIG_EVENT_NO_PAD_MOVE, 1, 0) != 0) {
            key.h0a = 0;
        } else {
            asPIAK[kind](ch, &key);
        }
    } else if (answer == PIA_NONE) {
        return;
    }
    asPIA1[(s_char)((const u_char *)ch)[CHARA_MOTION_GROUP]](ch, &key);
}

// CT3 0x018f9540
// Lying down: unless the pad moved her, MOTION_DOWN asks for motion 0.
void asPIA1_KEY_TBL_DOWN(CHARACTER *ch, PIA_KEY *key)
{
    if (key->h0a != 0)
        return;
    if (CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_DOWN)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, 0);
}

// CT3 0x018f9590
// The pad moves her once MOTION_DOWN has played MOTION_DOWN_GETUP_TIME.
int asPIA0_KEY_TBL_DOWN(CHARACTER *ch, PIA_KEY *key)
{
    (void)key;
    if (CharaShort(ch, CHARA_MOTION_SLOTS) != MOTION_DOWN)
        return PIA_NONE;
    return CharaInt(ch, CHARA_SLOT_TIME) < MOTION_DOWN_GETUP_TIME ? PIA_NONE : PIA_MOVE;
}

// CT3 0x018f96c0
void asPIA1_KEY_TBL_SITNOUP(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    if (key->h0a != 1)
        return;
    asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, key->h0c != 0 ? MOTION_SITNOUP_STEP2 : MOTION_SITNOUP_STEP);
}

// CT3 0x018f9720
int asPIA0_KEY_TBL_SITNOUP(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return PIA_MOVE;
}

// CT3 0x018f9740
int asPIA0_KEY_TBL_GOUP1M(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return PIA_NONE;
}

// CT3 0x018f9750
void asPIA1_KEY_TBL_JUMPDOWN(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f9760
int asPIA0_KEY_TBL_JUMPDOWN(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return PIA_NONE;
}

// CT3 0x018f9770
void asPIA1_KEY_TBL_HASHIGO(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f9780
int asPIA0_KEY_TBL_HASHIGO(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return PIA_MOVE;
}

// CT3 0x018f9790
void asPIA1_KEY_TBL_CATCH(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
}

// CT3 0x018f97a0
int asPIA0_KEY_TBL_CATCH(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    (void)key;
    return PIA_MOVE;
}

// Walk and run: PIA_HOLD in a fast-run record, PIA_MOVE otherwise, none in the hold motion; h0c set.
static int WalkRunAnswer(CHARACTER *ch, PIA_KEY *key, short hold)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    const u_char *rec = MotionRecord(ch, motion);
    // PORT: no table on the host yet answers PIA_MOVE (the ROM reads through the word)
    int answer = rec != NULL && (s_char)rec[MOTION_REC_RUN] == MOTION_RUN_FAST ? PIA_HOLD : PIA_MOVE;
    if (motion == hold)
        answer = PIA_NONE;
    key->h0c = 1;
    return answer;
}

// CT3 0x018f9ee0
int asPIA0_KEY_TBL_RUN(CHARACTER *ch, PIA_KEY *key)
{
    return WalkRunAnswer(ch, key, MOTION_RUN_HOLD);
}

// CT3 0x018fa1e0
int asPIA0_KEY_TBL_WALK(CHARACTER *ch, PIA_KEY *key)
{
    return WalkRunAnswer(ch, key, MOTION_WALK_HOLD);
}

// CT3 0x018fa380
int asPIA0_KEY_TBL_SITGO(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    key->h0c = 1;
    return PIA_MOVE;
}

// CT3 0x018fa8e0
int asPIA0_KEY_TBL_STAND(CHARACTER *ch, PIA_KEY *key)
{
    key->h0c = 1;
    return CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_STAND_HOLD ? PIA_NONE : PIA_MOVE;
}

// CT3 0x018fa900
void asPIAK_BHVC1_MoveBLR(CHARACTER *ch, PIA_KEY *key)
{
    asCharacterMoveByPad(ch, key, -1);
}

// CT3 0x018fa910
void asPIAK_BHVC1_Stop(CHARACTER *ch, PIA_KEY *key)
{
    asCharacterMoveByPad(ch, key, 1);
}

// CT3 0x018fa920
void asPIAK_BHVC1_MoveUD(CHARACTER *ch, PIA_KEY *key)
{
    (void)ch;
    asCharacterMoveByPadUD(key);
}

// CT3 0x018fa930
void asPIAK_BHVC1_MoveLR(CHARACTER *ch, PIA_KEY *key)
{
    asCharacterMoveByPad(ch, key, 1);
}

// The boss's chains (pBossInfo; names from asChainLengthCheck's use, the boss TU confirms them).
#define BOSS_INFO_CHAIN_COUNT 0x91      /* u_char: chains (main.c's light belt test reads the same byte) */
#define BOSS_INFO_CHAIN_POS 0x100       /* EE word -> EE word[count] -> qword: each chain's anchor */
#define BOSS_INFO_CHAIN_ON 0x110        /* EE word -> u_char[count]: the chain holds Alyssa */
#define BOSS_INFO_PULL_TIME 0x44        /* int: += CHAIN_PULL_LENGTH when a chain pulls her */
#define CHAIN_PULL_LENGTH 300           /* the distance from which one or two chains pull her (and the add) */
#define CHAIN_MANY 3                    /* this many chains: she is held (MOTION_CHAINED) */
#define MOTION_PULLED 0x41              /* pulled by a chain (asPIA0_KEY_TBL_PULLED aims away from it) */
#define MOTION_CHAINED 0x03             /* held by CHAIN_MANY chains */
#define MOTION_PULLED_FREE 0x08         /* asPIA0_KEY_TBL_PULLED: no chain holds her any more */
#define PULLED_AIM_SPEED 0x6d
// Holy water (asWater_Run_Check; names from this use).
#define TRIG_VAR_WATER_STATE 7          /* the trigger variable that must read WATER_READY */
#define WATER_READY 6
#define TRIG_VAR_WATER_USED 0x34        /* set to 1 when she drinks */
#define TRIG_VAR_MAGIC_WATER 0x3e       /* the holy water count MagicWaterCount takes */
#define MOTION_WATER 0x51               /* drinking holy water */

int MagicWaterCount;                    /* sbss 0x01944954 */

// The boss info's EE word at `off` as a host pointer; NULL for none. PORT: pBossInfo's pointer words
// have no host form until the boss TU that fills them decides it (as CharaTarget): no chain is seen.
static const u_char *BossInfoPtr(int off)
{
    (void)off;
    return NULL;
}

// Chain i's anchor (BOSS_INFO_CHAIN_POS's word [i]); NULL for none. PORT: as BossInfoPtr.
static const float *ChainAnchor(int i)
{
    const u_char *words = BossInfoPtr(BOSS_INFO_CHAIN_POS);
    (void)i;
    (void)words;
    return NULL;
}

// CT3 0x018f4140
// The chains holding Alyssa (the boss battle): -1 with none (or no boss). One or two: the farthest
// chain's index when `query`, else its distance, and from CHAIN_PULL_LENGTH on she is pulled
// (MOTION_PULLED, the boss's pull time grows). Three or more: she is held (MOTION_CHAINED), 0 when
// `query`, else CHAIN_PULL_LENGTH.
int asChainLengthCheck(int query)
{
    const u_char *on;
    float alyssa[4];
    float far = 0.0f;
    int i, count = 0, farthest = -1;

    if (pBossInfo == NULL)
        return -1;
    memcpy(alyssa, &Character[0].pos, sizeof(alyssa));
    on = BossInfoPtr(BOSS_INFO_CHAIN_ON);
    for (i = 0; i < pBossInfo[BOSS_INFO_CHAIN_COUNT]; i++) {
        const float *anchor;
        float d;
        if (on == NULL || on[i] == 0)
            continue;
        count++;
        anchor = ChainAnchor(i);
        if (anchor == NULL)
            continue;
        d = asVectorDistance2(alyssa, anchor);
        if (!(d < far)) {
            far = d;
            farthest = i;
        }
    }
    if (count == 0)
        return -1;
    if (count >= CHAIN_MANY) {
        if (CharaShort(&Character[0], CHARA_MOTION_SLOTS) != MOTION_CHAINED)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CHAINED);
        return query != 0 ? 0 : CHAIN_PULL_LENGTH;
    }
    if (query != 0)
        return farthest;
    if ((int)far >= CHAIN_PULL_LENGTH && CharaShort(&Character[0], CHARA_MOTION_SLOTS) != MOTION_PULLED) {
        int t;
        memcpy(&t, pBossInfo + BOSS_INFO_PULL_TIME, sizeof(t));
        t += CHAIN_PULL_LENGTH;
        memcpy(pBossInfo + BOSS_INFO_PULL_TIME, &t, sizeof(t));
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_PULLED);
    }
    return (int)far;
}

// CT3 0x018f7ed0
// Pulled by a chain: in MOTION_PULLED she aims away from the farthest chain (index 0 excepted, as the
// ROM's > 0 test); held (MOTION_CHAINED) with no chain left, she is freed. Always PIA_MOVE.
int asPIA0_KEY_TBL_PULLED(CHARACTER *ch, PIA_KEY *key)
{
    int chain = asChainLengthCheck(1);
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    (void)key;
    if (motion == MOTION_PULLED) {
        if (chain > 0) {
            const float *anchor = ChainAnchor(chain);
            if (anchor != NULL) {
                float a[4], p[4], aim[4];
                float w = 1.0f;
                u_short speed = PULLED_AIM_SPEED;
                int k;
                memcpy(a, anchor, sizeof(a));
                memcpy(p, &ch->pos, sizeof(p));
                for (k = 0; k < 4; k++)
                    aim[k] = p[k] + (p[k] - a[k]);
                memcpy((u_char *)ch + CHARA_AIM_POINT, aim, sizeof(aim));
                memcpy((u_char *)ch + CHARA_AIM_W, &w, sizeof(w));
                memcpy((u_char *)ch + CHARA_AIM_SPEED, &speed, sizeof(speed));
            }
        }
    } else if (motion == MOTION_CHAINED && chain == -1) {
        float w = 0.0f;
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_PULLED_FREE);
        memcpy((u_char *)ch + CHARA_AIM_W, &w, sizeof(w));
    }
    return PIA_MOVE;
}

// CT3 0x018f8050
// Holy water: when the trigger state allows it (WATER_READY), she is not panicking and not drinking
// already, she drinks (MOTION_WATER) and MagicWaterCount takes the trigger's count. 1 when she drinks.
int asWater_Run_Check(CHARACTER *ch)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    if (ssTrigGetVariable(TRIG_VAR_WATER_STATE, 1, 0) != WATER_READY)
        return 0;
    if ((PanicInfoData.mode & PANIC_MODE_FULL) != 0 || motion == MOTION_WATER)
        return 0;
    ssTrigSetVariable(TRIG_VAR_WATER_USED, 1, 1, 0);
    asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_WATER);
    MagicWaterCount = ssTrigGetVariable(TRIG_VAR_MAGIC_WATER, 1, 0);
    return 1;
}

// ---- stubs (tools/reverse/stubgen.py) ----


// ---- asMotionAdderByNowMotion (slice 7, 2026-11-10): the face motions and the blinks ------------------
static const char kAlysShape[] = "ALYS.BIN"; /* @3756, sdata 0x01937400 */
static const char kCaljShape[] = "CALJ.BIN"; /* @3758, sdata 0x01937420 */
static int EyeType;                     /* EyeType$1998, sbss 0x019449bc: pAME's list */
static int EyeTicks;                    /* EyeTicks$1999, sbss 0x019449c0 */

// AM (sdata 0x0192ffb0, 263 records): the lists pAM points into; records { time, h12, motion }, each list descending, ended by time -1.
const MOTION_ADD AM[263] = { /* sdata 0x0192ffb0 */
    { -1, -1, -1 }, /* 0x0192ffb0 */
    { 0, 0, 200 }, /* 0x0192ffb8 */
    { -1, -1, -1 }, /* 0x0192ffc0 */
    { 0, 0, 201 }, /* 0x0192ffc8 */
    { -1, -1, -1 }, /* 0x0192ffd0 */
    { 0, 0, 202 }, /* 0x0192ffd8 */
    { -1, -1, -1 }, /* 0x0192ffe0 */
    { 0, 0, 203 }, /* 0x0192ffe8 */
    { -1, -1, -1 }, /* 0x0192fff0 */
    { 0, 0, 204 }, /* 0x0192fff8 */
    { -1, -1, -1 }, /* 0x01930000 */
    { 0, 0, 205 }, /* 0x01930008 */
    { -1, -1, -1 }, /* 0x01930010 */
    { 0, 0, 206 }, /* 0x01930018 */
    { -1, -1, -1 }, /* 0x01930020 */
    { 0, 0, 207 }, /* 0x01930028 */
    { -1, -1, -1 }, /* 0x01930030 */
    { 0, 0, 208 }, /* 0x01930038 */
    { -1, -1, -1 }, /* 0x01930040 */
    { 0, 0, 209 }, /* 0x01930048 */
    { -1, -1, -1 }, /* 0x01930050 */
    { 0, 0, 210 }, /* 0x01930058 */
    { -1, -1, -1 }, /* 0x01930060 */
    { 210, 60, 204 }, /* 0x01930068 */
    { 60, 60, 202 }, /* 0x01930070 */
    { 0, 0, 204 }, /* 0x01930078 */
    { -1, -1, -1 }, /* 0x01930080 */
    { 240, 60, 208 }, /* 0x01930088 */
    { 30, 60, 210 }, /* 0x01930090 */
    { 0, 0, 208 }, /* 0x01930098 */
    { -1, -1, -1 }, /* 0x019300a0 */
    { 270, 60, 202 }, /* 0x019300a8 */
    { 60, 90, 204 }, /* 0x019300b0 */
    { 0, 0, 202 }, /* 0x019300b8 */
    { -1, -1, -1 }, /* 0x019300c0 */
    { 240, 70, 210 }, /* 0x019300c8 */
    { 90, 60, 208 }, /* 0x019300d0 */
    { 0, 0, 210 }, /* 0x019300d8 */
    { -1, -1, -1 }, /* 0x019300e0 */
    { 180, 60, 200 }, /* 0x019300e8 */
    { 60, 60, 201 }, /* 0x019300f0 */
    { 0, 0, 200 }, /* 0x019300f8 */
    { -1, -1, -1 }, /* 0x01930100 */
    { 150, 90, 206 }, /* 0x01930108 */
    { 60, 30, 210 }, /* 0x01930110 */
    { 0, 0, 206 }, /* 0x01930118 */
    { -1, -1, -1 }, /* 0x01930120 */
    { 90, 90, 205 }, /* 0x01930128 */
    { 0, 0, 200 }, /* 0x01930130 */
    { -1, -1, -1 }, /* 0x01930138 */
    { 0, 0, 209 }, /* 0x01930140 */
    { -1, -1, -1 }, /* 0x01930148 */
    { 30, 60, 205 }, /* 0x01930150 */
    { 0, 0, 200 }, /* 0x01930158 */
    { -1, -1, -1 }, /* 0x01930160 */
    { 0, 0, 209 }, /* 0x01930168 */
    { -1, -1, -1 }, /* 0x01930170 */
    { 0, 60, 200 }, /* 0x01930178 */
    { -1, -1, -1 }, /* 0x01930180 */
    { 0, 0, 209 }, /* 0x01930188 */
    { -1, -1, -1 }, /* 0x01930190 */
    { 90, 90, 203 }, /* 0x01930198 */
    { 0, 0, 200 }, /* 0x019301a0 */
    { -1, -1, -1 }, /* 0x019301a8 */
    { 90, 90, 209 }, /* 0x019301b0 */
    { 0, 0, 206 }, /* 0x019301b8 */
    { -1, -1, -1 }, /* 0x019301c0 */
    { 60, 60, 200 }, /* 0x019301c8 */
    { 0, 0, 203 }, /* 0x019301d0 */
    { -1, -1, -1 }, /* 0x019301d8 */
    { 60, 60, 206 }, /* 0x019301e0 */
    { 0, 0, 209 }, /* 0x019301e8 */
    { -1, -1, -1 }, /* 0x019301f0 */
    { 450, 30, 200 }, /* 0x019301f8 */
    { 390, 30, 201 }, /* 0x01930200 */
    { 330, 30, 200 }, /* 0x01930208 */
    { 30, 60, 201 }, /* 0x01930210 */
    { 0, 0, 200 }, /* 0x01930218 */
    { -1, -1, -1 }, /* 0x01930220 */
    { 330, 90, 206 }, /* 0x01930228 */
    { 30, 90, 207 }, /* 0x01930230 */
    { 0, 0, 206 }, /* 0x01930238 */
    { -1, -1, -1 }, /* 0x01930240 */
    { 0, 0, 200 }, /* 0x01930248 */
    { -1, -1, -1 }, /* 0x01930250 */
    { 120, 90, 209 }, /* 0x01930258 */
    { 0, 0, 206 }, /* 0x01930260 */
    { -1, -1, -1 }, /* 0x01930268 */
    { 210, 30, 204 }, /* 0x01930270 */
    { 150, 30, 202 }, /* 0x01930278 */
    { 0, 0, 200 }, /* 0x01930280 */
    { -1, -1, -1 }, /* 0x01930288 */
    { 240, 60, 208 }, /* 0x01930290 */
    { 150, 60, 210 }, /* 0x01930298 */
    { 90, 30, 208 }, /* 0x019302a0 */
    { 0, 0, 206 }, /* 0x019302a8 */
    { -1, -1, -1 }, /* 0x019302b0 */
    { 240, 60, 200 }, /* 0x019302b8 */
    { 180, 60, 202 }, /* 0x019302c0 */
    { 0, 0, 204 }, /* 0x019302c8 */
    { -1, -1, -1 }, /* 0x019302d0 */
    { 180, 60, 206 }, /* 0x019302d8 */
    { 30, 30, 210 }, /* 0x019302e0 */
    { 0, 0, 208 }, /* 0x019302e8 */
    { -1, -1, -1 }, /* 0x019302f0 */
    { 270, 30, 200 }, /* 0x019302f8 */
    { 30, 120, 204 }, /* 0x01930300 */
    { 0, 0, 202 }, /* 0x01930308 */
    { -1, -1, -1 }, /* 0x01930310 */
    { 0, 0, 206 }, /* 0x01930318 */
    { -1, -1, -1 }, /* 0x01930320 */
    { 390, 120, 201 }, /* 0x01930328 */
    { 270, 60, 200 }, /* 0x01930330 */
    { 120, 60, 201 }, /* 0x01930338 */
    { 30, 60, 200 }, /* 0x01930340 */
    { 0, 0, 201 }, /* 0x01930348 */
    { -1, -1, -1 }, /* 0x01930350 */
    { 270, 150, 206 }, /* 0x01930358 */
    { 60, 90, 207 }, /* 0x01930360 */
    { 0, 0, 206 }, /* 0x01930368 */
    { -1, -1, -1 }, /* 0x01930370 */
    { 750, 270, 200 }, /* 0x01930378 */
    { 600, 80, 201 }, /* 0x01930380 */
    { 480, 90, 200 }, /* 0x01930388 */
    { 360, 90, 201 }, /* 0x01930390 */
    { 180, 150, 200 }, /* 0x01930398 */
    { 30, 120, 201 }, /* 0x019303a0 */
    { 0, 0, 200 }, /* 0x019303a8 */
    { -1, -1, -1 }, /* 0x019303b0 */
    { 810, 240, 206 }, /* 0x019303b8 */
    { 630, 50, 207 }, /* 0x019303c0 */
    { 480, 120, 206 }, /* 0x019303c8 */
    { 330, 120, 207 }, /* 0x019303d0 */
    { 180, 120, 206 }, /* 0x019303d8 */
    { 30, 120, 207 }, /* 0x019303e0 */
    { 0, 0, 206 }, /* 0x019303e8 */
    { -1, -1, -1 }, /* 0x019303f0 */
    { 30, 90, 201 }, /* 0x019303f8 */
    { 0, 0, 200 }, /* 0x01930400 */
    { -1, -1, -1 }, /* 0x01930408 */
    { 30, 90, 207 }, /* 0x01930410 */
    { 0, 0, 206 }, /* 0x01930418 */
    { -1, -1, -1 }, /* 0x01930420 */
    { 780, 30, 200 }, /* 0x01930428 */
    { 120, 90, 201 }, /* 0x01930430 */
    { 0, 0, 200 }, /* 0x01930438 */
    { -1, -1, -1 }, /* 0x01930440 */
    { 780, 30, 206 }, /* 0x01930448 */
    { 120, 90, 207 }, /* 0x01930450 */
    { 0, 0, 206 }, /* 0x01930458 */
    { -1, -1, -1 }, /* 0x01930460 */
    { 90, 60, 201 }, /* 0x01930468 */
    { 30, 30, 200 }, /* 0x01930470 */
    { 0, 0, 201 }, /* 0x01930478 */
    { -1, -1, -1 }, /* 0x01930480 */
    { 90, 30, 206 }, /* 0x01930488 */
    { 30, 30, 207 }, /* 0x01930490 */
    { 0, 0, 206 }, /* 0x01930498 */
    { -1, -1, -1 }, /* 0x019304a0 */
    { 270, 30, 200 }, /* 0x019304a8 */
    { 150, 60, 202 }, /* 0x019304b0 */
    { 30, 90, 204 }, /* 0x019304b8 */
    { 0, 0, 202 }, /* 0x019304c0 */
    { -1, -1, -1 }, /* 0x019304c8 */
    { 150, 60, 206 }, /* 0x019304d0 */
    { 90, 30, 210 }, /* 0x019304d8 */
    { 30, 30, 208 }, /* 0x019304e0 */
    { 0, 0, 210 }, /* 0x019304e8 */
    { -1, -1, -1 }, /* 0x019304f0 */
    { 240, 60, 202 }, /* 0x019304f8 */
    { 60, 60, 204 }, /* 0x01930500 */
    { 0, 0, 200 }, /* 0x01930508 */
    { -1, -1, -1 }, /* 0x01930510 */
    { 120, 60, 210 }, /* 0x01930518 */
    { 30, 30, 208 }, /* 0x01930520 */
    { 0, 0, 206 }, /* 0x01930528 */
    { -1, -1, -1 }, /* 0x01930530 */
    { 0, 0, 200 }, /* 0x01930538 */
    { -1, -1, -1 }, /* 0x01930540 */
    { 300, 90, 206 }, /* 0x01930548 */
    { 180, 90, 210 }, /* 0x01930550 */
    { 60, 90, 207 }, /* 0x01930558 */
    { 0, 0, 206 }, /* 0x01930560 */
    { -1, -1, -1 }, /* 0x01930568 */
    { 540, 60, 200 }, /* 0x01930570 */
    { 360, 190, 201 }, /* 0x01930578 */
    { 0, 0, 200 }, /* 0x01930580 */
    { -1, -1, -1 }, /* 0x01930588 */
    { 510, 90, 206 }, /* 0x01930590 */
    { 240, 30, 207 }, /* 0x01930598 */
    { 150, 60, 206 }, /* 0x019305a0 */
    { 60, 90, 207 }, /* 0x019305a8 */
    { 0, 0, 206 }, /* 0x019305b0 */
    { -1, -1, -1 }, /* 0x019305b8 */
    { 300, 90, 200 }, /* 0x019305c0 */
    { 210, 60, 201 }, /* 0x019305c8 */
    { 120, 30, 204 }, /* 0x019305d0 */
    { 60, 30, 201 }, /* 0x019305d8 */
    { 0, 0, 200 }, /* 0x019305e0 */
    { -1, -1, -1 }, /* 0x019305e8 */
    { 330, 60, 206 }, /* 0x019305f0 */
    { 240, 60, 210 }, /* 0x019305f8 */
    { 30, 30, 207 }, /* 0x01930600 */
    { 0, 0, 206 }, /* 0x01930608 */
    { -1, -1, -1 }, /* 0x01930610 */
    { 30, 30, 200 }, /* 0x01930618 */
    { 0, 0, 201 }, /* 0x01930620 */
    { -1, -1, -1 }, /* 0x01930628 */
    { 30, 30, 206 }, /* 0x01930630 */
    { 0, 0, 207 }, /* 0x01930638 */
    { -1, -1, -1 }, /* 0x01930640 */
    { 90, 30, 200 }, /* 0x01930648 */
    { 30, 30, 201 }, /* 0x01930650 */
    { 0, 0, 200 }, /* 0x01930658 */
    { -1, -1, -1 }, /* 0x01930660 */
    { 90, 30, 206 }, /* 0x01930668 */
    { 30, 30, 207 }, /* 0x01930670 */
    { 0, 0, 206 }, /* 0x01930678 */
    { -1, -1, -1 }, /* 0x01930680 */
    { 210, 60, 200 }, /* 0x01930688 */
    { 0, 0, 201 }, /* 0x01930690 */
    { -1, -1, -1 }, /* 0x01930698 */
    { 210, 60, 206 }, /* 0x019306a0 */
    { 0, 0, 207 }, /* 0x019306a8 */
    { -1, -1, -1 }, /* 0x019306b0 */
    { 60, 60, 201 }, /* 0x019306b8 */
    { 0, 0, 200 }, /* 0x019306c0 */
    { -1, -1, -1 }, /* 0x019306c8 */
    { 60, 60, 207 }, /* 0x019306d0 */
    { 0, 0, 206 }, /* 0x019306d8 */
    { -1, -1, -1 }, /* 0x019306e0 */
    { 0, 0, 200 }, /* 0x019306e8 */
    { -1, -1, -1 }, /* 0x019306f0 */
    { 90, 60, 206 }, /* 0x019306f8 */
    { 30, 30, 207 }, /* 0x01930700 */
    { 0, 0, 206 }, /* 0x01930708 */
    { -1, -1, -1 }, /* 0x01930710 */
    { 210, 90, 200 }, /* 0x01930718 */
    { 120, 60, 201 }, /* 0x01930720 */
    { 0, 0, 200 }, /* 0x01930728 */
    { -1, -1, -1 }, /* 0x01930730 */
    { 180, 60, 206 }, /* 0x01930738 */
    { 120, 30, 207 }, /* 0x01930740 */
    { 0, 0, 206 }, /* 0x01930748 */
    { -1, -1, -1 }, /* 0x01930750 */
    { 240, 90, 200 }, /* 0x01930758 */
    { 120, 60, 201 }, /* 0x01930760 */
    { 0, 0, 200 }, /* 0x01930768 */
    { -1, -1, -1 }, /* 0x01930770 */
    { 240, 60, 206 }, /* 0x01930778 */
    { 120, 60, 207 }, /* 0x01930780 */
    { 0, 0, 206 }, /* 0x01930788 */
    { -1, -1, -1 }, /* 0x01930790 */
    { 180, 60, 200 }, /* 0x01930798 */
    { 90, 60, 201 }, /* 0x019307a0 */
    { 30, 30, 203 }, /* 0x019307a8 */
    { 0, 0, 202 }, /* 0x019307b0 */
    { -1, -1, -1 }, /* 0x019307b8 */
    { 180, 60, 206 }, /* 0x019307c0 */
    { 90, 60, 207 }, /* 0x019307c8 */
    { 30, 30, 209 }, /* 0x019307d0 */
    { 0, 0, 208 }, /* 0x019307d8 */
    { -1, -1, -1 }, /* 0x019307e0 */
};
// pAM (sdata 0x019307f0, 233 pairs): per body motion, the face lists of slots 2 and 3.
const MOTION_ADD *const pAM[PAM_COUNT][2] = {
    { AM + 1, AM + 13 }, /* 0 */
    { AM + 1, AM + 13 }, /* 1 */
    { AM + 1, AM + 13 }, /* 2 */
    { AM + 5, AM + 17 }, /* 3 */
    { AM + 61, AM + 64 }, /* 4 */
    { AM + 7, AM + 19 }, /* 5 */
    { AM + 67, AM + 70 }, /* 6 */
    { AM + 0, AM + 0 }, /* 7 */
    { AM + 253, AM + 258 }, /* 8 */
    { AM + 0, AM + 0 }, /* 9 */
    { AM + 0, AM + 0 }, /* 10 */
    { AM + 0, AM + 0 }, /* 11 */
    { AM + 7, AM + 19 }, /* 12 */
    { AM + 1, AM + 13 }, /* 13 */
    { AM + 0, AM + 0 }, /* 14 */
    { AM + 0, AM + 0 }, /* 15 */
    { AM + 23, AM + 27 }, /* 16 */
    { AM + 31, AM + 35 }, /* 17 */
    { AM + 0, AM + 0 }, /* 18 */
    { AM + 5, AM + 17 }, /* 19 */
    { AM + 0, AM + 0 }, /* 20 */
    { AM + 0, AM + 0 }, /* 21 */
    { AM + 1, AM + 13 }, /* 22 */
    { AM + 0, AM + 0 }, /* 23 */
    { AM + 0, AM + 0 }, /* 24 */
    { AM + 0, AM + 0 }, /* 25 */
    { AM + 0, AM + 0 }, /* 26 */
    { AM + 1, AM + 13 }, /* 27 */
    { AM + 231, AM + 233 }, /* 28 */
    { AM + 0, AM + 0 }, /* 29 */
    { AM + 0, AM + 0 }, /* 30 */
    { AM + 0, AM + 0 }, /* 31 */
    { AM + 1, AM + 13 }, /* 32 */
    { AM + 0, AM + 0 }, /* 33 */
    { AM + 0, AM + 0 }, /* 34 */
    { AM + 1, AM + 13 }, /* 35 */
    { AM + 0, AM + 0 }, /* 36 */
    { AM + 0, AM + 0 }, /* 37 */
    { AM + 0, AM + 0 }, /* 38 */
    { AM + 0, AM + 0 }, /* 39 */
    { AM + 0, AM + 0 }, /* 40 */
    { AM + 0, AM + 0 }, /* 41 */
    { AM + 0, AM + 0 }, /* 42 */
    { AM + 151, AM + 155 }, /* 43 */
    { AM + 0, AM + 0 }, /* 44 */
    { AM + 0, AM + 0 }, /* 45 */
    { AM + 0, AM + 0 }, /* 46 */
    { AM + 3, AM + 15 }, /* 47 */
    { AM + 3, AM + 15 }, /* 48 */
    { AM + 23, AM + 27 }, /* 49 */
    { AM + 31, AM + 35 }, /* 50 */
    { AM + 0, AM + 0 }, /* 51 */
    { AM + 0, AM + 0 }, /* 52 */
    { AM + 0, AM + 0 }, /* 53 */
    { AM + 0, AM + 0 }, /* 54 */
    { AM + 0, AM + 0 }, /* 55 */
    { AM + 0, AM + 0 }, /* 56 */
    { AM + 0, AM + 0 }, /* 57 */
    { AM + 0, AM + 0 }, /* 58 */
    { AM + 0, AM + 0 }, /* 59 */
    { AM + 3, AM + 17 }, /* 60 */
    { AM + 0, AM + 0 }, /* 61 */
    { AM + 0, AM + 0 }, /* 62 */
    { AM + 5, AM + 13 }, /* 63 */
    { AM + 0, AM + 0 }, /* 64 */
    { AM + 0, AM + 0 }, /* 65 */
    { AM + 0, AM + 0 }, /* 66 */
    { AM + 0, AM + 0 }, /* 67 */
    { AM + 0, AM + 0 }, /* 68 */
    { AM + 0, AM + 0 }, /* 69 */
    { AM + 0, AM + 0 }, /* 70 */
    { AM + 0, AM + 0 }, /* 71 */
    { AM + 0, AM + 0 }, /* 72 */
    { AM + 0, AM + 0 }, /* 73 */
    { AM + 0, AM + 0 }, /* 74 */
    { AM + 0, AM + 0 }, /* 75 */
    { AM + 0, AM + 0 }, /* 76 */
    { AM + 0, AM + 0 }, /* 77 */
    { AM + 0, AM + 0 }, /* 78 */
    { AM + 0, AM + 0 }, /* 79 */
    { AM + 0, AM + 0 }, /* 80 */
    { AM + 5, AM + 13 }, /* 81 */
    { AM + 0, AM + 0 }, /* 82 */
    { AM + 61, AM + 19 }, /* 83 */
    { AM + 0, AM + 0 }, /* 84 */
    { AM + 67, AM + 19 }, /* 85 */
    { AM + 47, AM + 50 }, /* 86 */
    { AM + 0, AM + 0 }, /* 87 */
    { AM + 52, AM + 55 }, /* 88 */
    { AM + 0, AM + 19 }, /* 89 */
    { AM + 0, AM + 19 }, /* 90 */
    { AM + 0, AM + 19 }, /* 91 */
    { AM + 0, AM + 0 }, /* 92 */
    { AM + 57, AM + 59 }, /* 93 */
    { AM + 0, AM + 0 }, /* 94 */
    { AM + 1, AM + 13 }, /* 95 */
    { AM + 1, AM + 13 }, /* 96 */
    { AM + 0, AM + 0 }, /* 97 */
    { AM + 0, AM + 0 }, /* 98 */
    { AM + 0, AM + 0 }, /* 99 */
    { AM + 5, AM + 17 }, /* 100 */
    { AM + 0, AM + 0 }, /* 101 */
    { AM + 1, AM + 13 }, /* 102 */
    { AM + 1, AM + 13 }, /* 103 */
    { AM + 1, AM + 13 }, /* 104 */
    { AM + 39, AM + 43 }, /* 105 */
    { AM + 0, AM + 0 }, /* 106 */
    { AM + 0, AM + 0 }, /* 107 */
    { AM + 0, AM + 0 }, /* 108 */
    { AM + 0, AM + 0 }, /* 109 */
    { AM + 1, AM + 13 }, /* 110 */
    { AM + 0, AM + 0 }, /* 111 */
    { AM + 0, AM + 0 }, /* 112 */
    { AM + 0, AM + 0 }, /* 113 */
    { AM + 0, AM + 0 }, /* 114 */
    { AM + 0, AM + 0 }, /* 115 */
    { AM + 0, AM + 0 }, /* 116 */
    { AM + 0, AM + 0 }, /* 117 */
    { AM + 0, AM + 0 }, /* 118 */
    { AM + 0, AM + 0 }, /* 119 */
    { AM + 0, AM + 0 }, /* 120 */
    { AM + 0, AM + 0 }, /* 121 */
    { AM + 0, AM + 0 }, /* 122 */
    { AM + 1, AM + 13 }, /* 123 */
    { AM + 0, AM + 0 }, /* 124 */
    { AM + 0, AM + 0 }, /* 125 */
    { AM + 0, AM + 0 }, /* 126 */
    { AM + 0, AM + 0 }, /* 127 */
    { AM + 0, AM + 0 }, /* 128 */
    { AM + 0, AM + 0 }, /* 129 */
    { AM + 0, AM + 0 }, /* 130 */
    { AM + 0, AM + 0 }, /* 131 */
    { AM + 0, AM + 0 }, /* 132 */
    { AM + 0, AM + 0 }, /* 133 */
    { AM + 0, AM + 0 }, /* 134 */
    { AM + 0, AM + 0 }, /* 135 */
    { AM + 0, AM + 0 }, /* 136 */
    { AM + 1, AM + 13 }, /* 137 */
    { AM + 1, AM + 13 }, /* 138 */
    { AM + 0, AM + 0 }, /* 139 */
    { AM + 0, AM + 0 }, /* 140 */
    { AM + 0, AM + 0 }, /* 141 */
    { AM + 0, AM + 0 }, /* 142 */
    { AM + 1, AM + 19 }, /* 143 */
    { AM + 1, AM + 13 }, /* 144 */
    { AM + 97, AM + 101 }, /* 145 */
    { AM + 88, AM + 92 }, /* 146 */
    { AM + 0, AM + 0 }, /* 147 */
    { AM + 0, AM + 0 }, /* 148 */
    { AM + 0, AM + 0 }, /* 149 */
    { AM + 5, AM + 17 }, /* 150 */
    { AM + 3, AM + 15 }, /* 151 */
    { AM + 3, AM + 15 }, /* 152 */
    { AM + 3, AM + 15 }, /* 153 */
    { AM + 1, AM + 13 }, /* 154 */
    { AM + 0, AM + 0 }, /* 155 */
    { AM + 105, AM + 109 }, /* 156 */
    { AM + 1, AM + 13 }, /* 157 */
    { AM + 3, AM + 13 }, /* 158 */
    { AM + 1, AM + 15 }, /* 159 */
    { AM + 1, AM + 13 }, /* 160 */
    { AM + 0, AM + 0 }, /* 161 */
    { AM + 5, AM + 17 }, /* 162 */
    { AM + 0, AM + 0 }, /* 163 */
    { AM + 151, AM + 155 }, /* 164 */
    { AM + 0, AM + 0 }, /* 165 */
    { AM + 111, AM + 117 }, /* 166 */
    { AM + 0, AM + 0 }, /* 167 */
    { AM + 0, AM + 0 }, /* 168 */
    { AM + 0, AM + 0 }, /* 169 */
    { AM + 0, AM + 0 }, /* 170 */
    { AM + 0, AM + 0 }, /* 171 */
    { AM + 0, AM + 0 }, /* 172 */
    { AM + 0, AM + 0 }, /* 173 */
    { AM + 0, AM + 0 }, /* 174 */
    { AM + 1, AM + 13 }, /* 175 */
    { AM + 0, AM + 0 }, /* 176 */
    { AM + 1, AM + 13 }, /* 177 */
    { AM + 0, AM + 0 }, /* 178 */
    { AM + 5, AM + 17 }, /* 179 */
    { AM + 1, AM + 13 }, /* 180 */
    { AM + 88, AM + 92 }, /* 181 */
    { AM + 23, AM + 27 }, /* 182 */
    { AM + 97, AM + 101 }, /* 183 */
    { AM + 1, AM + 13 }, /* 184 */
    { AM + 1, AM + 13 }, /* 185 */
    { AM + 1, AM + 13 }, /* 186 */
    { AM + 1, AM + 13 }, /* 187 */
    { AM + 121, AM + 129 }, /* 188 */
    { AM + 137, AM + 140 }, /* 189 */
    { AM + 88, AM + 92 }, /* 190 */
    { AM + 159, AM + 164 }, /* 191 */
    { AM + 31, AM + 35 }, /* 192 */
    { AM + 88, AM + 92 }, /* 193 */
    { AM + 159, AM + 164 }, /* 194 */
    { AM + 7, AM + 19 }, /* 195 */
    { AM + 151, AM + 155 }, /* 196 */
    { AM + 0, AM + 0 }, /* 197 */
    { AM + 7, AM + 19 }, /* 198 */
    { AM + 7, AM + 19 }, /* 199 */
    { AM + 0, AM + 0 }, /* 200 */
    { AM + 0, AM + 0 }, /* 201 */
    { AM + 0, AM + 0 }, /* 202 */
    { AM + 0, AM + 0 }, /* 203 */
    { AM + 0, AM + 0 }, /* 204 */
    { AM + 0, AM + 0 }, /* 205 */
    { AM + 0, AM + 0 }, /* 206 */
    { AM + 0, AM + 0 }, /* 207 */
    { AM + 0, AM + 0 }, /* 208 */
    { AM + 0, AM + 0 }, /* 209 */
    { AM + 0, AM + 0 }, /* 210 */
    { AM + 0, AM + 0 }, /* 211 */
    { AM + 0, AM + 0 }, /* 212 */
    { AM + 1, AM + 13 }, /* 213 */
    { AM + 1, AM + 13 }, /* 214 */
    { AM + 1, AM + 13 }, /* 215 */
    { AM + 177, AM + 179 }, /* 216 */
    { AM + 1, AM + 13 }, /* 217 */
    { AM + 1, AM + 13 }, /* 218 */
    { AM + 1, AM + 13 }, /* 219 */
    { AM + 184, AM + 188 }, /* 220 */
    { AM + 194, AM + 200 }, /* 221 */
    { AM + 205, AM + 208 }, /* 222 */
    { AM + 211, AM + 215 }, /* 223 */
    { AM + 219, AM + 222 }, /* 224 */
    { AM + 225, AM + 228 }, /* 225 */
    { AM + 1, AM + 13 }, /* 226 */
    { AM + 245, AM + 249 }, /* 227 */
    { AM + 245, AM + 249 }, /* 228 */
    { AM + 5, AM + 17 }, /* 229 */
    { AM + 5, AM + 17 }, /* 230 */
    { AM + 5, AM + 17 }, /* 231 */
    { AM + 7, AM + 19 }, /* 232 */
};
// AME (sdata 0x0193b910, 16 records): the blink lists pAME points at: { from, to, which (an index into the pair, -1 none) }.
const MOTION_ADD AME[16] = { /* sdata 0x0193b910 */
    { 0, 700, -1 }, /* 0x0193b910 */
    { 700, 730, 1 }, /* 0x0193b918 */
    { 730, 760, 0 }, /* 0x0193b920 */
    { -1, -1, -1 }, /* 0x0193b928 */
    { 0, 400, -1 }, /* 0x0193b930 */
    { 400, 430, 1 }, /* 0x0193b938 */
    { 430, 460, 0 }, /* 0x0193b940 */
    { -1, -1, -1 }, /* 0x0193b948 */
    { 0, 400, -1 }, /* 0x0193b950 */
    { 400, 430, 1 }, /* 0x0193b958 */
    { 430, 600, 0 }, /* 0x0193b960 */
    { 600, 630, 1 }, /* 0x0193b968 */
    { 630, 660, 0 }, /* 0x0193b970 */
    { 660, 690, 1 }, /* 0x0193b978 */
    { 690, 710, 0 }, /* 0x0193b980 */
    { -1, -1, -1 }, /* 0x0193b988 */
};
const MOTION_ADD *const pAME[PAME_COUNT] = { /* sdata 0x0193b990 */
    AME + 0, AME + 4, AME + 8,
};
// One of pAM's lists for Alyssa: the first entry the slot's time has reached starts its motion (unless
// the slot plays it, or plays none and played it last) and sets the record's MOTION_TABLE_H12.
// cmpMotion is what the ROM compares the entry's motion with: slot 2's motion for the first list, but
// slot 3's *time* for the second (the ROM reloads 0x464, not the slot's motion 0x39e: kept).
static void MotionAddList(CHARACTER *ch, const MOTION_ADD *e, int time, int cmpMotion, int slot)
{
    for (; e->time >= 0; e++) {
        if (e->time <= time) {
            if (cmpMotion != e->motion &&
                (cmpMotion != -1 || CharaShort(ch, CHARA_SLOT_PREV + slot * 2) != e->motion)) {
                u_char *rec;
                short arg = e->arg;
                asCharacterMortionChangeRequest(CHARA_ID_ALYSSA, CHARA_MOTION_REQ_PLAY, e->motion);
                rec = MotionRecord(ch, e->motion);
                if (rec != NULL)        // PORT: no motion table on the host yet: nothing to set
                    memcpy(rec + MOTION_TABLE_H12, &arg, sizeof(arg));
            }
            break;
        }
    }
}

// CT3 0x018f6b50
// Alyssa's face motions by her body motion (pAM, slots 2 and 3), then the blinks of the three faces
// that have them (Alyssa while she has hp, CALH, CALJ: their shape's bank name and id): pAME's list
// over EyeTicks plays the pair's entry in slot 4; at the list's end a new list is drawn.
void asMotionAdderByNowMotion(CHARACTER *ch)
{
    int which = -1;
    int pair[2];

    if (ch->id == CHARA_ID_ALYSSA) {
        short body = CharaShort(ch, CHARA_MOTION_SLOTS);
        if (body != -1 && body < PAM_MOTION_LIMIT) {
            MotionAddList(ch, pAM[body][0], CharaInt(ch, CHARA_SLOT_TIME + MOTION_ADD_SLOT_A * 4),
                          CharaShort(ch, CHARA_MOTION_SLOTS + MOTION_ADD_SLOT_A * 2), MOTION_ADD_SLOT_A);
            body = CharaShort(ch, CHARA_MOTION_SLOTS);
            {
                int t = CharaInt(ch, CHARA_SLOT_TIME + MOTION_ADD_SLOT_B * 4);
                MotionAddList(ch, pAM[body][1], t, t, MOTION_ADD_SLOT_B);
            }
        }
    }
    if (ch->bank < 0)
        return;
    pair[0] = -1;
    pair[1] = -1;
    if (ch->id == CHARA_ID_ALYSSA && strcmp(BankInfo[ch->bank].name, kAlysShape) == 0 && ch->hp > 0) {
        pair[0] = MOTION_EYE_ALYS_OPEN;
        which = 0;
        pair[1] = MOTION_EYE_ALYS_SHUT;
    } else if (ch->id == CHARA_ID_CALH && strcmp(BankInfo[ch->bank].name, kCalhShape) == 0) {
        pair[0] = MOTION_EYE_CALH_OPEN;
        which = 1;
        pair[1] = MOTION_EYE_CALH_SHUT;
    } else if (ch->id == CHARA_ID_CALJ && strcmp(BankInfo[ch->bank].name, kCaljShape) == 0) {
        pair[0] = MOTION_EYE_CALJ_OPEN;
        which = 2;
        pair[1] = MOTION_EYE_CALJ_SHUT;
    }
    if (which == -1)
        return;
    if (which == 2 && (CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_ADD_BODY_KEEP1 ||
                       CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_ADD_BODY_KEEP7)) {
        asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_STOP, pair[0]);
        asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_STOP, pair[1]);
        return;
    }
    {
        const MOTION_ADD *e = pAME[EyeType];
        for (; e->time >= 0; e++) {
            if (e->time <= EyeTicks && EyeTicks < e->arg) {
                if (e->motion != -1 && CharaShort(ch, CHARA_MOTION_SLOTS + EYE_SLOT * 2) != pair[e->motion])
                    asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_PLAY, pair[e->motion]);
                break;
            }
        }
        if (gGlobalNextTicksInterval > 0)
            EyeTicks += gGlobalNextTicksInterval;
        if (e->time == -1) {
            EyeTicks = 0;
            EyeType = tnRand(PAME_COUNT);
            asCharacterMortionChangeRequest(ch->id, CHARA_MOTION_REQ_PLAY, pair[0]);
        }
    }
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

// ---- the pad move (slice 7, 2026-11-10) ---------------------------------------------------------------
s_char iCamChangeFlag;                  /* sbss 0x01944968: type 2 steers by the last cut's axes */
s_char iCamChangeCount;                 /* sbss 0x0194496c: frames the U-turn stays off after that */
int iCamChangeRot;                      /* sbss 0x019449f8: her rot when the cut changed */
short iCameraID_Stock;                  /* sbss 0x019449fc: Cam[6].cut last frame */
short iCameraBank_Stock;                /* sbss 0x01944a00: Cam[6].bank last frame */

static float CharaFloat(const CHARACTER *ch, int off)
{
    float v;
    memcpy(&v, (const u_char *)ch + off, sizeof(v));
    return v;
}

static void SetCharaFloat(CHARACTER *ch, int off, float v)
{
    memcpy((u_char *)ch + off, &v, sizeof(v));
}

// This frame's global ticks in 5-tick (one 60 Hz frame) units; 0 when the interval is negative.
static float PadTickStep(void)
{
    return gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / 5.0f;
}

// A stick byte past the dead zone [lo, hi], in stick units (/ 128, signed); 0 inside it.
static float PadStickAxis(u_int v, u_int lo, u_int hi)
{
    if (v < lo)
        return (float)(int)(v - lo) / PAD_STICK_SCALE;
    if (hi < v)
        return (float)(int)(v - hi) / PAD_STICK_SCALE;
    return 0.0f;
}

static u_int PadStickByte(int off)
{
    return ((const u_char *)&pad[0])[off];
}

// The VU's xyz normalise (vmul, the sum, vrsqrt, vmulq); w kept. PORT: a zero vector stays zero (the
// VU's rsqrt of 0 gives its largest float, times 0: zero too; the host would make NaN).
static void PadNormalize3(float *out, const float *v)
{
    float l = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    float r = l > 0.0f ? 1.0f / sqrtf(l) : 0.0f;
    out[0] = v[0] * r;
    out[1] = v[1] * r;
    out[2] = v[2] * r;
    out[3] = v[3];
}

// CT3 0x018fa940
void asCharacterMoveByPadUD(PIA_KEY *key)
{
    u_int y = PadStickByte(PAD_STICK_LY);
    u_int dead = KEY_MAP[KEY_MAP_STICK_DEAD];
    float v = 0.0f;
    // the ROM tests the byte itself against the move's minimum length (not its distance from the centre)
    if ((float)y >= PAD_STICK_MIN_LEN)
        v = PadStickAxis(y, (PAD_STICK_CENTRE - dead) & 0xff, (PAD_STICK_CENTRE + dead) & 0xff);
    if (v == 0.0f)
        key->h0a = PAD_MOVE_NONE;
    else
        key->h0a = (short)(int)(v < 0.0f ? -1.0f : 1.0f);
}

// CT3 0x018faa80
// The stick moves her: controller_type CONTROLLER_TANK turns her by x and sets the walk direction by y;
// the camera types turn her toward the stick's direction in Cam[6]'s view (dir -1: backward, the turn
// half a circle off), at most her turn speed a frame, and set PIA_KEY.h0c when the stick points behind
// her (no turn then while it stays set). The move itself is the motion's (root motion): only CHARA_PAD_MOVE.
void asCharacterMoveByPad(CHARACTER *ch, PIA_KEY *key, int dir)
{
    u_int x = PadStickByte(PAD_STICK_LX);
    u_int y = PadStickByte(PAD_STICK_LY);
    int dead = (s_char)KEY_MAP[KEY_MAP_STICK_DEAD];
    u_int lo = (PAD_STICK_CENTRE - dead) & 0xff;
    u_int hi = (PAD_STICK_CENTRE + dead) & 0xff;
    u_short turnSpeed;
    int turn, flip = 0, noTurn;
    short wasBack = key->h0c;
    float sx = 0.0f, sy = 0.0f;
    float mv[4] = { 0.0f, 0.0f, 0.0f, 1.0f };   // vf0
    float len[4];

    memcpy(&turnSpeed, (const u_char *)ch + CHARA_TURN_SPEED, sizeof(turnSpeed));
    turn = (int)((float)turnSpeed * PadTickStep());
    noTurn = ssTrigCheckEvent(TRIG_EVENT_NO_HIT_6F, 1, 0) & 0xff;
    if (dir != 1 && dir == -1)
        flip = ANGLE_HALF_TURN;
    if (turn > PAD_TURN_MAX)
        turn = PAD_TURN_MAX;

    len[0] = (float)x - PAD_STICK_SCALE;
    len[1] = (float)y - PAD_STICK_SCALE;
    len[2] = 0.0f;                      // PORT: the ROM leaves z (sp+0x168) as the stack had it
    len[3] = 0.0f;
    if (asVectorLength3D(len) >= PAD_STICK_MIN_LEN) {
        sx = -PadStickAxis(x, lo, hi);
        sy = PadStickAxis(y, lo, hi);
    }
    if (sx == 0.0f && sy == 0.0f) {
        key->h0a = PAD_MOVE_NONE;
        SetCharaFloat(ch, CHARA_PAD_MOVE, 0.0f);
    } else {
        key->h0a = PAD_MOVE_ON;
        SetCharaFloat(ch, CHARA_PAD_MOVE, 1.0f);
        mv[0] = sx;
        mv[2] = sy;
    }

    if (controller_type == CONTROLLER_TANK) {
        if (key->h0a != PAD_MOVE_NONE) {
            float t = -mv[0] * PadTickStep() * PAD_TANK_TURN;
            if (mv[2] < 0.0f) {
                key->h0a = PAD_MOVE_BACK;
                t *= -1.0f;
            } else if (mv[2] > 0.0f) {
                key->h0a = PAD_MOVE_ON;
            } else if (mv[2] == 0.0f) {
                key->h0a = mv[0] == 0.0f ? PAD_MOVE_NONE : PAD_MOVE_TURN;
            }
            ch->rot += (int)t;
        }
    } else if (controller_type == CONTROLLER_CAMERA_HOLD || controller_type == CONTROLLER_CAMERA) {
        const CAMERA *cam = &Cam[CAMERA_SHARED];
        const float (*m)[4] = iCamChangeFlag == 0 ? cam->view2 : cam->viewPrev;
        float row0[4], row2[4], n[4], axis[4], p[4], d[4];
        float scale, step;
        const float *pos = (const float *)&ch->pos;
        int i;

        PadNormalize3(n, mv);
        scale = key->h0a == PAD_MOVE_ON ? CharaFloat(ch, CHARA_PAD_MOVE) : 0.0f;
        for (i = 0; i < 3; i++)
            n[i] *= scale;
        memcpy(row0, m[0], sizeof(row0));
        memcpy(row2, m[2], sizeof(row2));

        if (controller_type == CONTROLLER_CAMERA) {
            iCamChangeFlag = 0;
        } else if (controller_type == CONTROLLER_CAMERA_HOLD &&
                   (iCameraID_Stock != cam->cut || iCameraBank_Stock != cam->bank)) {
            iCamChangeFlag = 1;
            iCamChangeRot = ch->rot;
        }

        // the stick's y along the view's z axis, its x along the view's x axis, both flat (y = 0)
        axis[0] = row0[2];
        axis[1] = 0.0f;
        axis[2] = row2[2];
        axis[3] = 0.0f;
        PadNormalize3(axis, axis);
        step = PadTickStep();
        for (i = 0; i < 3; i++)
            p[i] = pos[i] + axis[i] * n[2] * step;
        axis[0] = row0[0];
        axis[1] = 0.0f;
        axis[2] = row2[0];
        axis[3] = 0.0f;
        PadNormalize3(axis, axis);
        for (i = 0; i < 3; i++)
            p[i] += axis[i] * n[0] * step;
        for (i = 0; i < 3; i++)
            d[i] = p[i] - pos[i];
        d[3] = 0.0f;

        if (asVectorLength(d) > 0.0f) {
            float a = asRadianByO(d, RADIAN_AXIS_Y);
            u_int want = (((u_int)(int)((-a * ANGLE_FULL_TURN_F) / 6.2831855f) & 0xffff) + flip) & 0xffff;
            u_int delta = (want - (u_int)ch->rot) & 0xffff;
            int camBusy = (int)iCamChangeCount + (int)iCamChangeFlag != 0;
            if (noTurn == 0) {
                if (delta < ANGLE_HALF_TURN) {
                    key->h0c = (delta < PAD_TURN_BACK_LO || camBusy) ? 0 : 1;
                    if (key->h0c == 0 || wasBack == 0) {
                        if ((int)delta < turn)
                            ch->rot = (int)want;
                        else
                            ch->rot += turn;
                    }
                } else {
                    key->h0c = (delta < PAD_TURN_BACK_HI && !camBusy) ? 1 : 0;
                    if (key->h0c == 0 || wasBack == 0) {
                        if (0x10000 - turn < (int)delta)
                            ch->rot = (int)want;
                        else
                            ch->rot -= turn;
                    }
                }
            } else if (delta < ANGLE_HALF_TURN) {
                key->h0c = (delta < PAD_TURN_BACK_LO_FIXED || camBusy) ? 0 : 1;
            } else {
                key->h0c = (delta < PAD_TURN_BACK_HI_FIXED && !camBusy) ? 1 : 0;
            }
        }

        if (controller_type == CONTROLLER_CAMERA_HOLD) {
            int s = asISubRotate16((u_int)iCamChangeRot, (u_int)ch->rot);
            if (((s + CAM_CHANGE_SETTLE) & 0xffff) >= 2 * CAM_CHANGE_SETTLE) {
                if (iCamChangeFlag != 0)
                    iCamChangeCount = CAM_CHANGE_COUNT;
                iCamChangeFlag = 0;
            }
            if (key->h0a == PAD_MOVE_NONE)
                iCamChangeFlag = 0;
            iCameraBank_Stock = cam->bank;
            iCameraID_Stock = cam->cut;
            if (iCamChangeCount > 0)
                iCamChangeCount--;
        }
    }
}

// ---- slice 8 (2026-11-10): the cat walk, the lock, panic, hiding, stairs and bow key-table entries -------

// KEY_MAP's actions these entries test (names from these uses).
#define KEY_MAP_GETUP_FIRST 12          /* KEY_MAP[12..15]: any of them held gets her up from MOTION_KNOCKED */
#define KEY_MAP_GETUP_COUNT 4
#define KEY_MAP_ARROW2 17               /* the second arrow (asYumi_Run_Check(.., BOW_ARROW_LIST): needs ArrowList) */
#define KEY_MAP_ITEM 18                 /* the bow (BOW_ARROW_PLAIN), else the holy water */
#define KEY_MAP_RUN 21                  /* held on the stairs: the faster stair motions */
#define KEY_MAP_HIDE 25                 /* held behind cover: she stays hidden */

// The cat walk (asPIA0 / asPIA1_KEY_TBL_CATWALK; names from these uses).
#define MOTION_CATWALK 0xb8             /* on the beam */
#define MOTION_CATWALK_STEP 0xb9        /* the step (PAD_MOVE 1 in MOTION_CATWALK) */
#define MOTION_CATWALK_STEP2 0xba
#define MOTION_CATWALK_STOP 0xbb        /* from a step when the pad lets go */
#define MOTION_CATWALK_SWAY 0xbc        /* the random event (asCatWalkEventOperation(CATWALK_CHECK)) */
#define MOTION_CATWALK_FALL 0xbd        /* off the beam: no target part under her */
#define MOTION_FALL_DEAD 0x64           /* a fall of CATWALK_FALL_SAFE or more: hp 0 */
#define CATWALK_FALL_SAFE 300.0f        /* the drop (fFallStart - pos.y) below which she lands */
#define SE_CATWALK_FALL 0x3f            /* SSPlaySE id in the "C_000" bank (@4024) */
static const char kSeBankC000[] = "C_000"; /* @4024, sdata 0x019374d0 */
static short iFeetMaxID;                /* iFeetMaxID$1645, sbss 0x019449b0: the part under her (CHARA_TARGET_PART) */
static short iDisableMaxID;             /* iDisableMaxID$1646, sbss 0x019449b4: the part hidden while she falls */
static float fFallStart;                /* fFallStart$1647, sbss 0x019449b8: pos.y when the fall began */

// The locks and the panic events (asPIA1_KEY_TBL_LOCK / PRUN / PSTAND).
#define MOTION_LOCK_1B 0x1b             /* a lever mash (RLC_SET 60, 10, 0x70, 10) */
#define MOTION_KNOCKED 0xd7             /* knocked down: a mash, or KEY_MAP_GETUP lays her down (MOTION_DOWN) */
#define MOTION_PANIC_SCREAM 0xaf        /* PANIC_EVENT_SCREAM's motion */
#define MOTION_PANIC_TUMBLE 0xa7        /* PANIC_EVENT_TUMBLE's */
#define MOTION_PANIC_FREEZE CHARA_MOTION_KAKOKYU /* PANIC_EVENT_FREEZE's: the breathing */
#define MOTION_BOSS5_DEAD 0x7b          /* MOTION_BOSS5_STRUCK with no hp left */
#define TRIG_EVENT_LOCK_9A 0x9a         /* MOTION_DAMAGE_68's mash: set, the short one while she has hp */
#define LOCK_MUTEKI 999999999           /* vsBossMuteki through MOTION_DAMAGE_68 */
#define LOCK_LONG_TIME 30000            /* the mash that cannot end in time (MOTION_DAMAGE_68's) */
#define MOTION_PSTAND_WAIT 0x89         /* panic stand: after PSTAND_WAIT_TIME, MOTION_PSTAND_NEXT */
#define MOTION_PSTAND_NEXT 0xa2
#define PSTAND_WAIT_TIME 0x95           /* slot 0's time it must pass */
#define MOTION_PSTAND_MOVE 0xa4         /* PAD_MOVE 1 in panic stand */

// Hiding behind cover (asPIA0 / asPIA1_KEY_TBL_BEHIND and _SITBEHIND).
#define TRIG_EVENT_HIDE_HELD 0x87       /* set: she stays hidden without KEY_MAP_HIDE (SITBEHIND), or leaves (BEHIND) */
#define TRIG_EVENT_HIDE_66 0x66         /* set: facing the cover, she stays in it rather than peek */
#define MOTION_BEHIND_IN 0x96           /* the motions that take her behind (asPIA0 aims her at CHARA_TARGET2) */
#define MOTION_BEHIND_IN2 0x1f
#define MOTION_BEHIND_IN3 0x21
#define MOTION_BEHIND 0x97              /* standing behind cover */
#define MOTION_BEHIND_TURN_L 0x98       /* rot below iRad_Behind */
#define MOTION_BEHIND_TURN_R 0x99
#define MOTION_BEHIND_PEEK 0xde         /* facing it (TRIG_EVENT_HIDE_66 clear) */
#define MOTION_BEHIND_OUT 0xe0          /* leaving it */
#define MOTION_SITBEHIND_IN 0x9c        /* the same crouched */
#define MOTION_SITBEHIND_IN2 0x24
#define MOTION_SITBEHIND_IN3 0x25
#define MOTION_SITBEHIND 0x9d
#define MOTION_SITBEHIND_TURN_L 0x9e
#define MOTION_SITBEHIND_TURN_R 0x9f
#define MOTION_SITBEHIND_PEEK 0xdf
#define MOTION_SITBEHIND_OUT 0xe1
#define BEHIND_ROT_SAME 0x10            /* |rot - iRad_Behind| within it: facing the cover */
#define BEHIND_ROT_OPPOSITE 0x7ff0      /* beyond it: facing away (treated as staying) */
static int iRad_Behind;                 /* sbss 0x019449a8: the rot that faces CHARA_TARGET2's vector */
static int iRadSet;                     /* sbss 0x019449ac: CHARA_TARGET2's part then (read nowhere here) */

// The stairs (asPIA1_KEY_TBL_KAIDAN): targetState bit 0 set, the stair's direction by targetDot.
#define STAIR_DOT 0.3f
#define MOTION_STAIR_WALK_POS 0x16      /* targetDot > STAIR_DOT */
#define MOTION_STAIR_WALK_NEG 0x13      /* targetDot < -STAIR_DOT */
#define MOTION_STAIR_WALK 0x20
#define MOTION_STAIR_RUN_POS 0x17       /* the same with KEY_MAP_RUN held */
#define MOTION_STAIR_RUN_NEG 0x14
#define MOTION_STAIR_RUN 0x12
#define MOTION_STAIR_OFF_WALK 5         /* targetState bit 0 clear: blended off the stairs */
#define MOTION_STAIR_OFF_RUN 2
#define MOTION_STAIR_STOP 0xa1

// The bow (asPIA1_KEY_TBL_YUMI, asYumi_Run_Check).
#define ITEM_STATE_BOW 2                /* TRIG_VAR_WATER_STATE (7): the bow is ready (WATER_READY: the water) */
#define TRIG_VAR_BOW_53 0x53            /* 0: the bow's other mode (BOW_ARROW_NONE: MOTION_BOW_EMPTY) */
#define BOW_ARROW_PLAIN 0               /* iArrowType: KEY_MAP_ITEM's arrow */
#define BOW_ARROW_LIST 1                /* KEY_MAP_ARROW2's: needs ArrowList[0] */
#define BOW_ARROW_NONE 3                /* TRIG_VAR_BOW_53 at 0 */
#define MOTION_BOW_EMPTY 0x35
#define MOTION_BOW_AIM 0x36             /* aiming: the buttons let go with the draw at BOW_SHOOT_RAD shoots */
#define MOTION_BOW_SHOOT 0x37
#define MOTION_BOW_RAISE 0x56           /* raising it; letting go of the arrow's button lowers it (motion 0) */
#define MOTION_BOW_DRAW 0x57
#define MOTION_BOW_RAISE2 0x58          /* raising it again from MOTION_BOW_HOLD */
#define MOTION_BOW_HOLD 0x5d
#define BOW_SHOOT_RAD 0.5f              /* asGetTheArrowRad's draw */
#define BOW_AIM_H12 300                 /* MOTION_BOW_AIM's record MOTION_TABLE_H12 with TRIG_VAR_BOW_53 at 0 */
#define BOW_LOCK_ARGC 2                 /* ssXcmd_LockToTargChara's arguments: "0", "4" */
static int iArrowType;                  /* sbss 0x019449d0: BOW_ARROW_* of the last check */

// Sit-go with no getting up (asPIA1_KEY_TBL_SITGONOUP).
#define MOTION_SITGONOUP 0xb6
#define MOTION_SITGONOUP2 0xc0          /* PIA_KEY.h0c toggles the two (blended) */
#define MOTION_SITGONOUP_STOP 0xb7
#define MOTION_SITGONOUP2_STOP 0xc2
#define SITGONOUP_FREE_FRAMES 2         /* frames with no pad move before she stops */
static int iFreeCount_1277;             /* iFreeCount$1277, sbss 0x0194499c */

// The character's position y (pos is a qword of floats).
static float PosY(const CHARACTER *ch)
{
    float y;
    memcpy(&y, (const u_char *)&ch->pos + 4, sizeof(y));
    return y;
}

static int KeyMapHeld(u_int buttons, int action)
{
    return (buttons & KEY_MAP[action]) != 0;
}

// The rot (0..0xffff) that faces CHARA_TARGET2's copied vector (asPIA0_KEY_TBL_BEHIND / _SITBEHIND).
static void AimBehind(CHARACTER *ch)
{
    float r = asRadianByO((float *)((u_char *)ch + CHARA_TARGET2_COPY), ROTATE_AXIS_Y);
    iRad_Behind = (int)((-r * 65536.0f) / 6.2831855f) & 0xffff;
    iRadSet = CharaShort(ch, CHARA_TARGET2_COPY + CHARA_TARGET_COPY_PART);
}

// CT3 0x018f8110
// On the beam: the sway event takes over (once per check); else a pad move steps, a stop ends a step.
void asPIA1_KEY_TBL_CATWALK(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    if (motion == MOTION_CATWALK_SWAY)
        return;
    if (asCatWalkEventOperation(CATWALK_CHECK) != 0) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CATWALK_SWAY);
        return;
    }
    if (key->h0a == 1) {
        if (motion == MOTION_CATWALK)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CATWALK_STEP);
    } else if (key->h0a == 0) {
        if (motion == MOTION_CATWALK_STEP || motion == MOTION_CATWALK_STEP2)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CATWALK_STOP);
        else
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CATWALK);
    }
}

// The flags word of part `part` in the bank's parts header; NULL without one.
static u_int *PartFlags(int bank, int part)
{
    if (bank < 0 || pModelPartsHeader[bank] == NULL)
        return NULL;
    return &((MODEL_PARTS *)pModelPartsHeader[bank])[part].flags;
}

// CT3 0x018f81e0
// The beam under her (CHARA_TARGET): with none she falls (the part she stood on hidden, the fall's
// start kept, its SE); landing again, the part shows, and a drop of CATWALK_FALL_SAFE or more kills her.
// The sway ends its event. PIA_MOVE, or PIA_NONE while she falls.
int asPIA0_KEY_TBL_CATWALK(CHARACTER *ch, PIA_KEY *key)
{
    const u_char *target = CharaTarget(ch);
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    int answer = PIA_MOVE;
    u_int *flags;
    (void)key;

    if (target == NULL) {
        if (motion != MOTION_CATWALK_FALL) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CATWALK_FALL);
            fFallStart = PosY(ch);
            iDisableMaxID = iFeetMaxID;
            flags = PartFlags(ch->lightBank, iDisableMaxID);
            if (flags != NULL)
                *flags |= PARTS_FLAG_HIDDEN;
            SSPlaySE(kSeBankC000, SE_CATWALK_FALL, 1);
        }
        answer = PIA_NONE;
    } else {
        memcpy(&iFeetMaxID, target + CHARA_TARGET_PART, sizeof(iFeetMaxID));
        if (motion == MOTION_CATWALK_FALL) {
            flags = PartFlags(ch->lightBank, iDisableMaxID);
            if (flags != NULL)
                *flags &= ~PARTS_FLAG_HIDDEN;
            if (fFallStart - PosY(ch) < CATWALK_FALL_SAFE) {
                iFeetMaxID = 0;
                iDisableMaxID = 0;
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, 0);
            } else {
                ch->hp = 0;
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_FALL_DEAD);
                answer = PIA_NONE;
            }
        }
    }
    if (CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_CATWALK_SWAY)
        asCatWalkEventOperation(CATWALK_RESET);
    return answer;
}

// CT3 0x018f8350
// Held: each lock motion is a lever mash with its own settings; once it is over (> 0), the motion's
// next (MOTION_REC_NEXT) plays. Motions with no mash go on at once. MOTION_BOSS5_STRUCK with no hp
// left ends in MOTION_BOSS5_DEAD; MOTION_KNOCKED gets up with any KEY_MAP_GETUP button.
void asPIA1_KEY_TBL_LOCK(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    int over = 1;

    if (motion == MOTION_BOSS5_STRUCK) {
        over = asRapidLeverChangeOperate(RLC_SET, 150, 10, 0x70, 50);
        if (ch->hp < 1) {
            asRapidLeverChangeOperate(RLC_FREE, 0, 0, 0, 0);
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOSS5_DEAD);
        }
    } else if (motion == MOTION_LOCK_1B) {
        over = asRapidLeverChangeOperate(RLC_SET, 60, 10, 0x70, 10);
    } else if (motion == MOTION_KNOCKED) {
        u_short any = 0;
        int i;
        for (i = 0; i < KEY_MAP_GETUP_COUNT; i++)
            any |= KEY_MAP[KEY_MAP_GETUP_FIRST + i];
        over = asRapidLeverChangeOperate(RLC_SET, 60, 1, 0xc0, 100);
        if ((key->cnt & any) != 0) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_DOWN);
            over = 0;
        }
    } else if (motion == MOTION_PANIC_SCREAM) {
        over = asRapidLeverChangeOperate(RLC_SET, 90, 10, 0x70, 10);
    } else if (motion == MOTION_PANIC_FREEZE) {
        over = asRapidLeverChangeOperate(RLC_SET, 90, 10, 0x70, 15);
    } else if (motion == MOTION_PANIC_TUMBLE) {
        over = asRapidLeverChangeOperate(RLC_SET, 90, 10, 0x70, 10);
    } else if (motion == MOTION_DAMAGE_68) {
        if (ssTrigCheckEvent(TRIG_EVENT_NO_FATAL_HIT, 1, 0) != 0)
            over = asRapidLeverChangeOperate(RLC_SET, 60, 10, 0x70, 10);
        else if (ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) == 0 || ch->hp < 1)
            over = asRapidLeverChangeOperate(RLC_SET, LOCK_LONG_TIME, 1, 0xc0, 100);
        else
            over = asRapidLeverChangeOperate(RLC_SET, 60, 10, 0x70, 10);
        vsBossMuteki = LOCK_MUTEKI;
    }
    if (over > 0) {
        const u_char *rec = MotionRecord(ch, CharaShort(ch, CHARA_MOTION_SLOTS));
        short next;
        if (rec == NULL)
            return;
        memcpy(&next, rec + MOTION_REC_NEXT, sizeof(next));
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, next);
    }
}

// The panic event's motion; 0 for none.
static int PanicEventMotion(int event)
{
    switch (event) {
    case PANIC_EVENT_FREEZE: return MOTION_PANIC_FREEZE;
    case PANIC_EVENT_TUMBLE: return MOTION_PANIC_TUMBLE;
    case PANIC_EVENT_SCREAM: return MOTION_PANIC_SCREAM;
    default: return 0;
    }
}

// CT3 0x018f8630
// Running in panic (PAD_MOVE 0 or 1): an armed scream, tumble or freeze plays.
void asPIA1_KEY_TBL_PRUN(CHARACTER *ch, PIA_KEY *key)
{
    int motion;
    (void)ch;
    if (key->h0a != 1 && key->h0a != 0)
        return;
    motion = PanicEventMotion(asPanicEventOperation(PANIC_EV_TAKE_RUN, 0));
    if (motion != 0)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, motion);
}

// CT3 0x018f8750
// Standing in panic: MOTION_PSTAND_WAIT goes on after PSTAND_WAIT_TIME (not while EV 0x68 is set) and
// takes no keys; else a pad move moves, none takes an armed scream or freeze (no tumble standing).
void asPIA1_KEY_TBL_PSTAND(CHARACTER *ch, PIA_KEY *key)
{
    if (CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_PSTAND_WAIT) {
        if (ssTrigCheckEvent(EV_FORCE_MATRIX_OPERATION, 1, 0) == 0 && CharaInt(ch, CHARA_SLOT_TIME) > PSTAND_WAIT_TIME)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_PSTAND_NEXT);
        return;
    }
    if (key->h0a == 1) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_PSTAND_MOVE);
    } else if (key->h0a == 0) {
        int event = asPanicEventOperation(PANIC_EV_TAKE_STOP, 0);
        if (event == PANIC_EVENT_FREEZE || event == PANIC_EVENT_SCREAM)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, PanicEventMotion(event));
    }
}

// Behind cover with the pad moving: turn toward iRad_Behind, stay when facing it or away from it, or
// peek (`peek`) when facing it with TRIG_EVENT_HIDE_66 clear.
static int BehindTurnMotion(CHARACTER *ch, int stay, int left, int right, int peek)
{
    int d = asISubRotate16((u_int)ch->rot, (u_int)iRad_Behind);
    if (d >= BEHIND_ROT_OPPOSITE || d <= -BEHIND_ROT_OPPOSITE)
        return stay;
    if (d >= BEHIND_ROT_SAME)
        return right;
    if (d >= -BEHIND_ROT_SAME)
        return ssTrigCheckEvent(TRIG_EVENT_HIDE_66, 1, 0) == 0 ? peek : stay;
    return left;
}

// CT3 0x018f8860
// Crouched behind cover: held (KEY_MAP_HIDE, or TRIG_EVENT_HIDE_HELD), she turns or peeks with the pad
// and stays without; let go, she leaves. Her rot is set to iRad_Behind each frame.
void asPIA1_KEY_TBL_SITBEHIND(CHARACTER *ch, PIA_KEY *key)
{
    int held = 1;
    if (ssTrigCheckEvent(TRIG_EVENT_HIDE_HELD, 1, 0) == 0)
        held = KeyMapHeld(key->cnt, KEY_MAP_HIDE);
    if (key->h0a == 1) {
        int motion = held ? BehindTurnMotion(ch, MOTION_SITBEHIND, MOTION_SITBEHIND_TURN_L,
                                             MOTION_SITBEHIND_TURN_R, MOTION_SITBEHIND_PEEK)
                          : MOTION_SITBEHIND_OUT;
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, motion);
    } else if (key->h0a == 0) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, held ? MOTION_SITBEHIND : MOTION_SITBEHIND_OUT);
    }
    ch->rot = iRad_Behind;
}

// CT3 0x018f8a00
// Going behind cover crouched: the entry motions aim her at CHARA_TARGET2; while entering, peeking or
// leaving, PIA_NONE.
int asPIA0_KEY_TBL_SITBEHIND(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    (void)key;
    if (motion == MOTION_SITBEHIND_IN || motion == MOTION_SITBEHIND_IN2 || motion == MOTION_SITBEHIND_IN3)
        AimBehind(ch);
    if (motion == MOTION_SITBEHIND_IN || motion == MOTION_SITBEHIND_IN2 || motion == MOTION_SITBEHIND_IN3 ||
        motion == MOTION_SITBEHIND_PEEK || motion == MOTION_SITBEHIND_OUT || motion == MOTION_BEHIND_OUT)
        return PIA_NONE;
    return PIA_MOVE;
}

// CT3 0x018f8af0
// Standing behind cover: KEY_MAP_HIDE held (TRIG_EVENT_HIDE_HELD clear) she leaves; else she turns or
// peeks with the pad and stays without. Her rot is set to iRad_Behind each frame.
void asPIA1_KEY_TBL_BEHIND(CHARACTER *ch, PIA_KEY *key)
{
    int leave = 0;
    if (ssTrigCheckEvent(TRIG_EVENT_HIDE_HELD, 1, 0) == 0)
        leave = KeyMapHeld(key->cnt, KEY_MAP_HIDE);
    if (key->h0a == 1) {
        int motion = leave ? MOTION_BEHIND_OUT
                           : BehindTurnMotion(ch, MOTION_BEHIND, MOTION_BEHIND_TURN_L, MOTION_BEHIND_TURN_R,
                                              MOTION_BEHIND_PEEK);
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, motion);
    } else if (key->h0a == 0) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, leave ? MOTION_BEHIND_OUT : MOTION_BEHIND);
    }
    ch->rot = iRad_Behind;
}

// CT3 0x018f8c90
// Going behind cover standing: the entry motions aim her at CHARA_TARGET2; while entering or peeking,
// PIA_NONE.
int asPIA0_KEY_TBL_BEHIND(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    (void)key;
    if (motion == MOTION_BEHIND_IN || motion == MOTION_BEHIND_IN2 || motion == MOTION_BEHIND_IN3)
        AimBehind(ch);
    if (motion == MOTION_BEHIND_IN || motion == MOTION_BEHIND_IN2 || motion == MOTION_BEHIND_IN3 ||
        motion == MOTION_BEHIND_PEEK)
        return PIA_NONE;
    return PIA_MOVE;
}

// CT3 0x018f8d70
// On the stairs: KEY_MAP_ITEM pressed raises the bow or drinks holy water, KEY_MAP_ARROW2 the second
// arrow; else a pad move climbs (by the stair's direction, faster with KEY_MAP_RUN; off the stair
// target, blended to the walk / run) and none stops.
void asPIA1_KEY_TBL_KAIDAN(CHARACTER *ch, PIA_KEY *key)
{
    int run;
    if (KeyMapHeld(key->trg, KEY_MAP_ITEM)) {
        if (asYumi_Run_Check(ch, BOW_ARROW_PLAIN) != 0 || asWater_Run_Check(ch) != 0)
            return;
    } else if (KeyMapHeld(key->trg, KEY_MAP_ARROW2) && asYumi_Run_Check(ch, BOW_ARROW_LIST) != 0) {
        return;
    }
    if (key->h0a == 0) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_STAIR_STOP);
        return;
    }
    if (key->h0a != 1)
        return;
    run = KeyMapHeld(key->cnt, KEY_MAP_RUN);
    if ((ch->targetState & 1) == 0) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, run ? MOTION_STAIR_OFF_RUN : MOTION_STAIR_OFF_WALK);
    } else if (ch->targetDot > STAIR_DOT) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, run ? MOTION_STAIR_RUN_POS : MOTION_STAIR_WALK_POS);
    } else if (ch->targetDot < -STAIR_DOT) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, run ? MOTION_STAIR_RUN_NEG : MOTION_STAIR_WALK_NEG);
    } else {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, run ? MOTION_STAIR_RUN : MOTION_STAIR_WALK);
    }
}

// The button of the arrow type in use (iArrowType).
static int ArrowKeyHeld(u_int buttons)
{
    return KeyMapHeld(buttons, iArrowType == BOW_ARROW_PLAIN ? KEY_MAP_ITEM : KEY_MAP_ARROW2);
}

// CT3 0x018f8fe0
// The bow: aiming, both buttons let go with the draw at BOW_SHOOT_RAD, she shoots; held (MOTION_BOW_HOLD),
// a button raises it again; drawn, she aims (TRIG_VAR_BOW_53 at 0: at once, the record's h12 set) or
// holds while the arrow's button is let go; raising, letting go lowers it.
void asPIA1_KEY_TBL_YUMI(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);

    if (motion == MOTION_BOW_AIM) {
        if (!KeyMapHeld(key->cnt, KEY_MAP_ARROW2) && !KeyMapHeld(key->cnt, KEY_MAP_ITEM) &&
            asGetTheArrowRad() >= BOW_SHOOT_RAD)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOW_SHOOT);
    } else if (motion == MOTION_BOW_HOLD) {
        if (KeyMapHeld(key->trg, KEY_MAP_ITEM))
            asYumi_Run_Check(ch, BOW_ARROW_PLAIN);
        else if (KeyMapHeld(key->trg, KEY_MAP_ARROW2))
            asYumi_Run_Check(ch, BOW_ARROW_LIST);
    } else if (motion == MOTION_BOW_DRAW) {
        u_char *aim = MotionRecord(&Character[0], MOTION_BOW_AIM);
        short h12;
        if (ssTrigGetVariable(TRIG_VAR_BOW_53, 1, 0) == 0) {
            h12 = BOW_AIM_H12;
            if (aim != NULL)
                memcpy(aim + MOTION_TABLE_H12, &h12, sizeof(h12));
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOW_AIM);
        } else {
            h12 = 0;
            if (aim != NULL)
                memcpy(aim + MOTION_TABLE_H12, &h12, sizeof(h12));
            if (!ArrowKeyHeld(key->cnt))
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOW_HOLD);
        }
    } else if (motion == MOTION_BOW_RAISE) {
        if (!ArrowKeyHeld(key->cnt))
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, 0);
    }
}

// CT3 0x018f93e0
// Raise the bow with arrow `type` (BOW_ARROW_*): only with the bow ready (ITEM_STATE_BOW), out of full
// panic, and for BOW_ARROW_LIST with an arrow in ArrowList; TRIG_VAR_BOW_53 at 0 makes it BOW_ARROW_NONE.
// It locks on (ssXcmd_LockToTargChara "0" "4" in Alyssa's bank) and plays the raise. iArrowType keeps
// the type either way. 1 when she raised it.
int asYumi_Run_Check(CHARACTER *ch, int type)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    int ok = 1;
    char arg0[4], arg1[4];
    char *argv[BOW_LOCK_ARGC] = { arg0, arg1 }; /* @1314, sbss 0x019449a0: the initializer */

    if (ssTrigGetVariable(TRIG_VAR_WATER_STATE, 1, 0) != ITEM_STATE_BOW || (PanicInfoData.mode & PANIC_MODE_FULL) != 0)
        ok = 0;
    if (type != BOW_ARROW_PLAIN && ArrowList[0] == 0)
        ok = 0;
    if (ssTrigGetVariable(TRIG_VAR_BOW_53, 1, 0) == 0)
        type = BOW_ARROW_NONE;
    if (ok) {
        sprintf(arg0, "%d", 0); /* @4317 */
        sprintf(arg1, "%d", 4);
        ssXcmd_LockToTargChara(BOW_LOCK_ARGC, argv, Character[0].bank);
        if (type == BOW_ARROW_NONE)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOW_EMPTY);
        else if (motion == MOTION_BOW_HOLD)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOW_RAISE2);
        else
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOW_RAISE);
    }
    iArrowType = type;
    return ok;
}

// CT3 0x018f95c0
// Sit-go with no getting up: a pad move toggles (with PIA_KEY.h0c) between the two motions, blended;
// SITGONOUP_FREE_FRAMES frames past without one, she stops.
void asPIA1_KEY_TBL_SITGONOUP(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    if (key->h0a == 1) {
        iFreeCount_1277 = 0;
        if (key->h0c != 0) {
            if (motion == MOTION_SITGONOUP)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_SITGONOUP2);
            else if (motion == MOTION_SITGONOUP2)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_SITGONOUP);
        }
    } else if (key->h0a == 0 && ++iFreeCount_1277 > SITGONOUP_FREE_FRAMES) {
        if (motion == MOTION_SITGONOUP)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_SITGONOUP_STOP);
        else if (motion == MOTION_SITGONOUP2)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_SITGONOUP2_STOP);
    }
}

// ---- slice 9 (2026-11-10): standing, walking, running, crouching and the bow's pre-entry -------------------

// The ground motions (names from these uses: the walk / run entries and their stops).
#define MOTION_STAND 0                  /* standing still */
#define MOTION_RUN 2                    /* = MOTION_STAIR_OFF_RUN: KEY_MAP_RUN held */
#define MOTION_WALK_START 4             /* from standing (asPIA1_KEY_TBL_STAND) */
#define MOTION_WALK 5                   /* = MOTION_STAIR_OFF_WALK */
#define MOTION_RUN_STOP 6               /* stopping after RUN_STOP_COUNT frames or more of it */
#define MOTION_TURN_BACK 0x0c           /* PIA_KEY.h0c set: the turn-around */
#define MOTION_WALK_TURN_BACK 0xc6      /* the same walking off the stairs */
#define MOTION_RUN_TURN_BACK 0xc7       /* running */
#define MOTION_STAND_1 1                /* a stand STAND does not blend back to MOTION_STAND (only use) */
#define MOTION_STAND_9A 0x9a            /* a stand STAND does not blend back to MOTION_STAND (only use) */
#define RUN_STOP_COUNT 7                /* iRunCount below it stops at once */
#define MOVE_FREE_FRAMES 2              /* frames with no pad move before a walk / run / crawl stops */

// TRIG_EVENT_LOCK_9A set: her other set of ground motions (hurt, a guess; with MOTION_DAMAGE_68's mash).
#define MOTION_9A_WALK_START 0x53
#define MOTION_9A_WALK 0x54
#define MOTION_9A_RUN_STOP 0x55
#define MOTION_9A_CROUCH 0x59
#define MOTION_9A_CROUCHED 0x5a         /* from MOTION_9A_SIT_0D when asPIA0_KEY_TBL_SIT sees the event */
#define MOTION_9A_STAND_UP 0x5b
#define MOTION_9A_SIT_0D 0x0d
#define MOTION_9A_TURN_BACK 0xc3        /* running */

// Crouching (asPIA1_KEY_TBL_SIT / _SITGO; KEY_MAP_HIDE is the crouch button: held, she stays down).
#define KEY_MAP_NO_CROUCH 24            /* held: MOVEEND does not crouch on KEY_MAP_HIDE (name from this use) */
#define MOTION_CROUCH 0xa0              /* going down (MOVEEND / STAND with KEY_MAP_HIDE) */
#define MOTION_STAND_UP 0x90            /* getting up; asPIA0_KEY_TBL_SIT holds it SIT_STAND_UP_TIME */
#define MOTION_SIT_GO 0x91              /* crouched, moving (h0c clear) */
#define MOTION_SIT_GO2 0xbe             /* h0c set */
#define MOTION_CRAWL 0x2f               /* low (TRIG_EVENT_CRAWL_88 at the stop) */
#define MOTION_CRAWL_GO 0x10            /* moving from MOTION_CRAWL; PIA_KEY.h0c toggles the two */
#define MOTION_CRAWL_GO2 0x11
#define MOTION_CRAWL_STAND_UP 0x92      /* getting up from MOTION_CRAWL; MOTION_CRAWL_GO's stop */
#define MOTION_CRAWL_STAND_UP2 0xbf     /* MOTION_CRAWL_GO2's stop */
#define TRIG_EVENT_CRAWL_88 0x88        /* set: a crawl's stop goes back down to MOTION_CRAWL */
#define SIT_STAND_UP_TIME 0x79          /* asPIA0_KEY_TBL_SIT: MOTION_STAND_UP takes no move before it */
#define RUN_STOP_TIME 0x5b              /* asPIA0_KEY_TBL_MOVEEND: MOTION_RUN_STOP (/ _9A_) before it */
#define CHAIN_HELD_LENGTH 300           /* asChainLengthCheck(0) from it: a chain holds her (no move) */
#define MSG_KEY_GRACE 12                /* frames after a message (MsgCnt) when STAND reads the press, not the hold */

// The bow before the keys (asPIA0_KEY_TBL_YUMI).
#define MOTION_BOW_ARROW_RATE 0x38      /* the motion whose play rate an empty bow sets (Character[0]'s table) */
#define BOW_RATE_EMPTY_HELD 0x14        /* a button held */
#define BOW_RATE_EMPTY_FREE 100         /* none */
#define BOW_RAISE_TIME 0x27             /* slot 0 past it: MOTION_BOW_RAISE is up */
#define BOW_RAISE2_TIME 0x31            /* MOTION_BOW_RAISE2 */
#define BOW_EMPTY_TIME 0x185            /* MOTION_BOW_EMPTY */
#define ARROW_STATE_ASK 1               /* asChangeTheArrowState: only ask */
#define ARROW_SHOOT_EMPTY 3             /* asShootArrowSetting's kind with no arrow (BOW_ARROW_NONE) */

static int iFreeCount_993;              /* iFreeCount$993, sbss 0x0194498c (SITGO) */
static u_short iFreeCount_1031;         /* iFreeCount$1031, sbss 0x01944990 (WALK) */
static u_short iFreeCount_1088;         /* iFreeCount$1088, sbss 0x01944994 (RUN) */
static u_short iRunCount_1089;          /* iRunCount$1089, sbss 0x01944998 (RUN): frames of MOTION_WALK / _9A_WALK */

// KEY_MAP_ITEM pressed: the bow, else the holy water; else KEY_MAP_ARROW2: the second arrow. 1 when one
// of them took over (the walk / run / stand / moveend entries share it).
static int ItemKeyTaken(CHARACTER *ch, u_int pressed)
{
    if (KeyMapHeld(pressed, KEY_MAP_ITEM))
        return asYumi_Run_Check(ch, BOW_ARROW_PLAIN) != 0 || asWater_Run_Check(ch) != 0;
    return KeyMapHeld(pressed, KEY_MAP_ARROW2) && asYumi_Run_Check(ch, BOW_ARROW_LIST) != 0;
}

// The stair motion by the stair's direction (targetDot), blended (the walk / run entries on stairs).
static void StairMove(CHARACTER *ch, int run)
{
    if (ch->targetDot > STAIR_DOT)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, run ? MOTION_STAIR_RUN_POS : MOTION_STAIR_WALK_POS);
    else if (ch->targetDot < -STAIR_DOT)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, run ? MOTION_STAIR_RUN_NEG : MOTION_STAIR_WALK_NEG);
    else
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_STAIR_STOP);
}

// The slot 0 record's MOTION_REC_RUN is MOTION_RUN_FAST (a motion that keeps the pad off).
static int MotionRunsFast(const CHARACTER *ch, int motion)
{
    const u_char *rec = MotionRecord(ch, motion);
    // PORT: no table on the host yet reads as not fast (the ROM reads through the word)
    return rec != NULL && (s_char)rec[MOTION_REC_RUN] == MOTION_RUN_FAST;
}

// CT3 0x018f91d0
// The bow before the keys: drawn with TRIG_VAR_BOW_53 at 0, she aims at once. Once a raise is up (or
// drawn, or aiming) and no arrow is in flight (asChangeTheArrowState ask -1), the arrow is set: aiming
// the empty kind; with no arrow (BOW_ARROW_NONE) the empty kind on a button, the arrow motion's rate by
// whether one is held; else the arrow type's button. An empty bow let go goes back to standing. PIA_MOVE.
int asPIA0_KEY_TBL_YUMI(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    int time = CharaInt(ch, CHARA_SLOT_TIME);

    if (motion == MOTION_BOW_DRAW && ssTrigGetVariable(TRIG_VAR_BOW_53, 1, 0) == 0)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOW_AIM);
    if ((motion == MOTION_BOW_RAISE && time > BOW_RAISE_TIME) || (motion == MOTION_BOW_RAISE2 && time > BOW_RAISE2_TIME) ||
        (motion == MOTION_BOW_EMPTY && time > BOW_EMPTY_TIME) || motion == MOTION_BOW_DRAW || motion == MOTION_BOW_AIM) {
        if (asChangeTheArrowState(ARROW_STATE_ASK) != -1)
            return PIA_MOVE;
        if (motion == MOTION_BOW_AIM) {
            asShootArrowSetting(ARROW_SHOOT_EMPTY, 1, 1);
        } else if (iArrowType == BOW_ARROW_NONE) {
            u_char *rec = MotionRecord(&Character[0], MOTION_BOW_ARROW_RATE);
            u_char rate = BOW_RATE_EMPTY_HELD;
            if (KeyMapHeld(key->cnt, KEY_MAP_ITEM) || KeyMapHeld(key->cnt, KEY_MAP_ARROW2))
                asShootArrowSetting(ARROW_SHOOT_EMPTY, 1, 1);
            else
                rate = BOW_RATE_EMPTY_FREE;
            if (rec != NULL)
                rec[MOTION_REC_PLAY_RATE] = rate;
        } else if (iArrowType == BOW_ARROW_LIST) {
            if (KeyMapHeld(key->cnt, KEY_MAP_ARROW2))
                asShootArrowSetting((u_char)ArrowList[0], 1, 1); /* the ROM reads its low byte (lbu) */
        } else if (iArrowType == BOW_ARROW_PLAIN && KeyMapHeld(key->cnt, KEY_MAP_ITEM)) {
            asShootArrowSetting(BOW_ARROW_PLAIN, 1, 1);
        }
    } else if (motion == MOTION_BOW_EMPTY && !KeyMapHeld(key->cnt, KEY_MAP_ARROW2) && !KeyMapHeld(key->cnt, KEY_MAP_ITEM)) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_STAND);
    }
    return PIA_MOVE;
}

// CT3 0x018f97b0
// A move just ended: the crouch button (KEY_MAP_HIDE, unless TRIG_EVENT_HIDE_HELD) crouches her, not on
// the stairs nor with KEY_MAP_NO_CROUCH; else, not chained, a pad move walks or runs on (blended), or
// turns around.
void asPIA1_KEY_TBL_MOVEEND(CHARACTER *ch, PIA_KEY *key)
{
    int go = 1, crouch = 0;
    if (ssTrigCheckEvent(TRIG_EVENT_HIDE_HELD, 1, 0) == 0)
        crouch = KeyMapHeld(key->cnt, KEY_MAP_HIDE);
    if (ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) == 0) {
        if (!KeyMapHeld(key->cnt, KEY_MAP_NO_CROUCH) && crouch && (ch->targetState & 1) == 0) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CROUCH);
            go = 0;
        }
    } else if (crouch) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_9A_CROUCH);
        go = 0;
    }
    if (asChainLengthCheck(0) >= CHAIN_HELD_LENGTH)
        go = 0;
    if (!go || key->h0a != PAD_MOVE_ON)
        return;
    if (key->h0c != 0)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_TURN_BACK);
    else if (ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) != 0)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_9A_WALK);
    else
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, KeyMapHeld(key->cnt, KEY_MAP_RUN) ? MOTION_RUN : MOTION_WALK);
}

// CT3 0x018f9980
// A move just ended, before the keys: a pressed item key that took over answers PIA_NONE; the run's
// stop (MOTION_RUN_STOP, or _9A_RUN_STOP with TRIG_EVENT_LOCK_9A) holds before RUN_STOP_TIME, a
// fast-run motion holds. h0c set to 1.
int asPIA0_KEY_TBL_MOVEEND(CHARACTER *ch, PIA_KEY *key)
{
    int answer = PIA_MOVE;
    short motion;
    int stop;

    if (ItemKeyTaken(ch, key->trg))
        return PIA_NONE;
    motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    stop = ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) == 0 ? MOTION_RUN_STOP : MOTION_9A_RUN_STOP;
    if (motion == stop) {
        if (CharaInt(ch, CHARA_SLOT_TIME) < RUN_STOP_TIME)
            answer = PIA_NONE;
    } else if (MotionRunsFast(ch, motion)) {
        answer = PIA_NONE;
    }
    key->h0c = 1;
    return answer;
}

// CT3 0x018f9b00
// Running: a pressed item key takes over; a chain holds her. A pad move goes on (the turn-around with
// h0c; off the stairs KEY_MAP_RUN keeps the run blended; on them the stair motion), counting the walk's
// frames; with none, a walk start stands, a walk stops after MOVE_FREE_FRAMES (the run's stop after
// RUN_STOP_COUNT frames of it). TRIG_EVENT_LOCK_9A plays the hurt set.
void asPIA1_KEY_TBL_RUN(CHARACTER *ch, PIA_KEY *key)
{
    int hurt;
    short motion;

    if (ItemKeyTaken(ch, key->trg) | (asChainLengthCheck(0) >= CHAIN_HELD_LENGTH)) /* both run, as in the ROM */
        return;
    motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    if (key->h0a == PAD_MOVE_ON) {
        hurt = ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) != 0;
        iFreeCount_1088 = 0;
        if (motion == (hurt ? MOTION_9A_WALK : MOTION_WALK))
            iRunCount_1089++;
        if (hurt) {
            if (key->h0c != 0)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_9A_TURN_BACK);
        } else if ((ch->targetState & 1) == 0) {
            if (key->h0c != 0)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_RUN_TURN_BACK);
            if (KeyMapHeld(key->cnt, KEY_MAP_RUN))
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_RUN);
        } else {
            StairMove(ch, KeyMapHeld(key->cnt, KEY_MAP_RUN));
        }
    } else if (key->h0a == PAD_MOVE_NONE) {
        hurt = ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) != 0;
        if (motion == (hurt ? MOTION_9A_WALK_START : MOTION_WALK_START)) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_STAND);
        } else if (motion == (hurt ? MOTION_9A_WALK : MOTION_WALK) && ++iFreeCount_1088 > MOVE_FREE_FRAMES) {
            if (iRunCount_1089 < RUN_STOP_COUNT)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_STAND);
            else
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, hurt ? MOTION_9A_RUN_STOP : MOTION_RUN_STOP);
            iRunCount_1089 = 0;
        }
    }
}

// CT3 0x018f9f30
// Walking: a pressed item key takes over. A pad move walks or (KEY_MAP_RUN) runs, blended, or turns
// around (h0c); on the stairs the stair motion. With none, the run stands after MOVE_FREE_FRAMES, the
// rest at once (blended).
void asPIA1_KEY_TBL_WALK(CHARACTER *ch, PIA_KEY *key)
{
    if (ItemKeyTaken(ch, key->trg) || key->h0a == PAD_MOVE_TURN || key->h0a == PAD_MOVE_BACK)
        return;
    if (key->h0a == PAD_MOVE_ON) {
        iFreeCount_1031 = 0;
        if ((ch->targetState & 1) != 0)
            StairMove(ch, KeyMapHeld(key->cnt, KEY_MAP_RUN));
        else if (key->h0c != 0)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_WALK_TURN_BACK);
        else
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, KeyMapHeld(key->cnt, KEY_MAP_RUN) ? MOTION_RUN : MOTION_WALK);
    } else if (key->h0a == PAD_MOVE_NONE) {
        if (CharaShort(ch, CHARA_MOTION_SLOTS) != MOTION_RUN || ++iFreeCount_1031 > MOVE_FREE_FRAMES)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_STAND);
    }
}

// CT3 0x018fa230
// Crawling (MOTION_CRAWL_GO / _GO2; not while getting up): a pad move toggles the two with h0c,
// blended; MOVE_FREE_FRAMES frames with none, she gets up (or, TRIG_EVENT_CRAWL_88 set, lies low again).
void asPIA1_KEY_TBL_SITGO(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    if (motion == MOTION_CRAWL_STAND_UP || motion == MOTION_CRAWL_STAND_UP2)
        return;
    if (key->h0a == PAD_MOVE_ON) {
        iFreeCount_993 = 0;
        if (key->h0c != 0) {
            if (motion == MOTION_CRAWL_GO)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_CRAWL_GO2);
            else if (motion == MOTION_CRAWL_GO2)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_CRAWL_GO);
        }
    } else if (key->h0a == PAD_MOVE_NONE && ++iFreeCount_993 > MOVE_FREE_FRAMES) {
        if (ssTrigCheckEvent(TRIG_EVENT_CRAWL_88, 1, 0) != 0)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CRAWL);
        else if (motion == MOTION_CRAWL_GO)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CRAWL_STAND_UP);
        else if (motion == MOTION_CRAWL_GO2)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CRAWL_STAND_UP2);
    }
}

// CT3 0x018fa390
// Crouched: the crouch button let go (unless TRIG_EVENT_HIDE_HELD) gets her up (from MOTION_CRAWL its own
// way; hurt, MOTION_9A_STAND_UP); held, a pad move moves her crouched (crawling from MOTION_CRAWL), h0c
// picking the step. Nothing while going down or getting up, or chained, or hurt.
void asPIA1_KEY_TBL_SIT(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    int held = 1, move;

    if (ssTrigCheckEvent(TRIG_EVENT_HIDE_HELD, 1, 0) == 0)
        held = KeyMapHeld(key->cnt, KEY_MAP_HIDE);
    move = ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) == 0;
    if (motion == MOTION_CROUCH || motion == MOTION_STAND_UP) {
        move = 0;
    } else if (!held) {
        if (ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) != 0)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_9A_STAND_UP);
        else
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, motion == MOTION_CRAWL ? MOTION_CRAWL_STAND_UP : MOTION_STAND_UP);
        move = 0;
    }
    if (asChainLengthCheck(0) >= CHAIN_HELD_LENGTH)
        move = 0;
    if (!move || key->h0a != PAD_MOVE_ON || ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) != 0)
        return;
    if (motion == MOTION_CRAWL)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, key->h0c == 0 ? MOTION_CRAWL_GO : MOTION_CRAWL_GO2);
    else
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, key->h0c == 0 ? MOTION_SIT_GO : MOTION_SIT_GO2);
}

// CT3 0x018fa570
// Crouched, before the keys: hurt (TRIG_EVENT_LOCK_9A) holds, MOTION_9A_SIT_0D going on to
// MOTION_9A_CROUCHED; getting up holds before SIT_STAND_UP_TIME, a fast-run motion holds. h0c set to 1.
int asPIA0_KEY_TBL_SIT(CHARACTER *ch, PIA_KEY *key)
{
    int answer = PIA_MOVE;
    short motion;

    if (ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) != 0) {
        answer = PIA_HOLD;
        if (CharaShort(ch, CHARA_MOTION_SLOTS) == MOTION_9A_SIT_0D)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_9A_CROUCHED);
    }
    motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    if (motion == MOTION_STAND_UP) {
        if (CharaInt(ch, CHARA_SLOT_TIME) < SIT_STAND_UP_TIME)
            answer = PIA_HOLD;
    } else if (MotionRunsFast(ch, motion)) {
        answer = PIA_HOLD;
    }
    key->h0c = 1;
    return answer;
}

// CT3 0x018fa640
// Standing: the crouch button (held, or pressed within MSG_KEY_GRACE frames of a message; not with
// TRIG_EVENT_HIDE_HELD, not on the stairs) crouches her; else a pressed item key takes over; else, not
// chained, a pad move starts the walk or run (blended) or turns around, and none settles to standing
// (on the stairs, the stair stop).
void asPIA1_KEY_TBL_STAND(CHARACTER *ch, PIA_KEY *key)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    int crouch = 0, go = 1;

    if (ssTrigCheckEvent(TRIG_EVENT_HIDE_HELD, 1, 0) == 0)
        crouch = KeyMapHeld((u_int)MsgCnt + MSG_KEY_GRACE < (u_int)FrameCounter ? key->cnt : key->trg, KEY_MAP_HIDE);
    if (crouch && (ch->targetState & 1) == 0) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY,
                                        ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) == 0 ? MOTION_CROUCH : MOTION_9A_CROUCH);
        go = 0;
    }
    if (go && ItemKeyTaken(ch, key->trg))
        go = 0;
    if (asChainLengthCheck(0) >= CHAIN_HELD_LENGTH)
        go = 0;
    if (!go)
        return;
    if (key->h0a == PAD_MOVE_ON) {
        if (key->h0c != 0)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_TURN_BACK);
        else if (ssTrigCheckEvent(TRIG_EVENT_LOCK_9A, 1, 0) != 0)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_9A_WALK_START);
        else
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, KeyMapHeld(key->cnt, KEY_MAP_RUN) ? MOTION_RUN : MOTION_WALK_START);
    } else if (key->h0a == PAD_MOVE_NONE) {
        if ((ch->targetState & 1) != 0)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_STAIR_STOP);
        else if (motion != MOTION_STAND_1 && motion != MOTION_STAND_9A)
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_BLEND, MOTION_STAND);
    }
}

// ---- asCharacterMortionFunctionOperation (slice 10, 2026-11-11): what slot 0's motion record turns on ----
// The record fields this function reads (names from this use; MOTION_REC_KIND2 / _RUN / _GROUP / _FACING
// above carry their other uses).
#define MOTION_REC_HIT_START 0x20       /* int: slot 0's time (CHARA_SLOT_TIME) the record's window opens */
#define MOTION_REC_HIT_END 0x24         /* int: ... and closes (inclusive) */
#define MOTION_REC_HIT_ATTACKS 0x28     /* u_int: bit i = the shape's attack i strikes in the window */
#define MOTION_REC_HIT_POWER 0x2c       /* short: ssHitAddAttackRequest's fourth argument */
// MOTION_REC_KIND2 values.
#define MOTION_KIND2_TURN_SLOW 1        /* Alyssa's turn speed TURN_SPEED_DEFAULT (the default's value too) */
#define MOTION_KIND2_STAIRS_TURN 3      /* turn TURN_SPEED_STAIRS (TURN_SPEED_SLOWEST in the TURN_SLOWEST_* motions);
                                           with TRIG_EVENT_HIDE_HELD: TRIG_EVENT_PANIC_ON_8A */
#define MOTION_KIND2_NO_HIT 4           /* in the window: CHARACTER_FLAG_NO_HIT; Alyssa: TRIG_EVENT_STAIRS_4C */
#define MOTION_KIND2_ATTACK MOTION_REC_KIND2_RESET /* 10: in the window the record's attacks strike */
#define MOTION_KIND2_PANIC_4D 0x0c      /* TRIG_EVENT_PANIC_ON_4D, turn TURN_SPEED_FAST */
#define MOTION_KIND2_PANIC_D 0x0d       /* 0x0d..0x0f: asPanicEventOperation ticks with the kind as group */
#define MOTION_KIND2_PANIC_E 0x0e       /* back to MOTION_STAND_PANIC_E when the panic is off */
#define MOTION_KIND2_PANIC_F 0x0f       /* ... MOTION_STAND_PANIC_F; also turn as MOTION_KIND2_STAIRS_TURN */
#define MOTION_KIND2_TURN_MID 0x10      /* turn TURN_SPEED_MID */
#define MOTION_KIND2_PANIC_8A 0x11      /* TRIG_EVENT_PANIC_ON_8A */
#define MOTION_RUN_EVENT 4              /* MOTION_REC_RUN: the character's trigger event (MFO_EVENT_*) is on */
// MOTION_REC_GROUP values.
#define MOTION_GROUP_MOVE_A 4           /* 4 and 5: MoveFrame counts (the boss's BOSS_INFO_MOVE_TICKS) */
#define MOTION_GROUP_MOVE_B 5
#define MOTION_GROUP_FEET 0x0f          /* footAdjust = both feet */
#define MOTION_GROUP_NO_HIT_A 0x10      /* Alyssa: CHARACTER_FLAG_NO_HIT */
#define MOTION_GROUP_NO_HIT_B 0x11
#define MOTION_GROUP_MASH 0x17          /* the lever mash goes on (asRapidLeverChangeOperate not freed) */
#define MOTION_GROUP_CATWALK 0x18       /* asCatWalkEventOperation(CATWALK_TICK) */
#define MOTION_GROUP_AIM 0x1b           /* CHARA_AIM_W kept */
// MOTION_REC_FACING values: these set CHARACTER_FLAG_FACING_HELD, 2 stocks the camera, the rest clear it.
#define MOTION_FACING_STOCK_CAMERA 2
#define CHARACTER_FLAG_FACING_HELD 0x08000000u /* set while slot 0's facing is 1, 3..8, 0x0f or 0x11 (name from this use) */
// The turn speeds (CHARA_TURN_SPEED, u_short).
#define TURN_SPEED_DEFAULT 0x600
#define TURN_SPEED_MID 0x800
#define TURN_SPEED_STAIRS 0x100
#define TURN_SPEED_SLOWEST 0x80
#define TURN_SPEED_FAST 0x4000
#define TURN_SLOWEST_1 0xc1             /* = MOTION_SITNOUP_STEP2 */
#define TURN_SLOWEST_2 0xb5             /* = MOTION_SITNOUP_STEP */
#define TURN_SLOWEST_3 0xbe
#define TURN_SLOWEST_4 0x91
// Alyssa's triggers this function keeps (names from this use).
#define TRIG_EVENT_RUN_EVENT_ALYSSA 0x55 /* her record's MOTION_RUN_EVENT */
#define TRIG_EVENT_STAIRS_4B 0x4b       /* MOTION_KIND_STAIRS2 (1) plays */
#define TRIG_EVENT_STAIRS_4C 0x4c       /* MOTION_KIND_STAIRS (3) or MOTION_KIND2_NO_HIT plays */
#define TRIG_VAR_WATER_DONE 2           /* TRIG_VAR_WATER_USED when MOTION_WATER ends */
#define BOSS_ACTION_4_DONE 5            /* TRIG_VAR_BOSS_ACTION when MOTION_BOSS5_STRUCK ends */
#define BOSS_ACTION_E_DONE 0x0f
#define BOSS_INFO_MOVE_TICKS 0x44       /* pBossInfo: int, += BOSS_MOVE_TICKS every MOVE_FRAME_MAX + 1 moving frames */
#define BOSS_MOVE_TICKS 30
#define MOVE_FRAME_MAX 4
#define FALL_OUT_Y -30000.0f            /* below it: the fatal motion and death */
#define IDLE_KEY_TICKS 3000             /* keyTicks before the idle fidgets */
#define IDLE_CHANCE 0.8f                /* every 32 frames, rndf() above it fidgets ... */
#define IDLE_PICK 0.5f                  /* ... MOTION_IDLE_A above it, else MOTION_IDLE_B */
#define MOTION_IDLE_A 1
#define MOTION_IDLE_B 0x9a
#define MUTEKI_FOREVER 999999999
#define MOTION_STAND_PANIC_F 0x0d
#define MOTION_STAND_PANIC_E 2
#define MOTION_STAND 0                  /* = the standing motion other entries request */
#define PSTAND_MOVE_STOP 0.3f           /* MOTION_PSTAND_MOVE: every 4 frames, NEXT_MODE -1 at or below it */
#define MOTION_CAUGHT2_RELEASE 0xe3     /* the catchers let go: vsBossMuteki forever while these play */
#define MOTION_CAUGHT32_RELEASE 0xe7
#define MOTION_CAUGHT2_HOLD 0xe2        /* unlocked in these (or MOTION_CAUGHT2): MOTION_CAUGHT2_RELEASE */
#define MOTION_CAUGHT2_HOLD2 MOTION_CAUGHT2
#define MOTION_CAUGHT32_HOLD 0xe6       /* ... in these (or MOTION_CAUGHT32): MOTION_CAUGHT32_RELEASE */
#define MOTION_CAUGHT32_HOLD2 MOTION_CAUGHT32
#define MOTION_LOCKED_SHAKE 0xb3        /* locked, kind 1 with no catcher: stops on the unlock */
#define MOTION_LOCKED_GRAB 0x18         /* locked, kind 2 with no catcher; her death when it runs past GRAB_FATAL_TIME */
#define MOTION_LOCKED_GRAB2 0x19
#define GRAB_FATAL_TIME 0x293
#define GRAB_SHAKE_TIME 30000           /* asDualShockVibrationOn_Light's time for MOTION_LOCKED_SHAKE */
// asAlyssaLockedByAnything's high byte: who holds her (the catcher's Character index, its catch motions).
#define LOCKED_BY_CHARA2 0x100
#define LOCKED_BY_CHARA32 0x200
#define LOCKED_KIND_MASK 0xff
#define LOCKED_KIND_CATCH 1
#define LOCKED_KIND_2 2
#define LOCKED_KIND_3 3
#define CATCHER_2 2
#define CATCHER_32 32
#define MOTION_CAUGHT2 0x23             /* caught by CATCHER_2 (MOTION_CAUGHT2_STAIRS on stairs) ... */
#define MOTION_CAUGHT2_STAIRS 0x3a
#define MOTION_CAUGHT2_HELD 0x29
#define MOTION_CAUGHT2_K3 0x2c          /* kind 3 (0x2d also counts as playing it) */
#define MOTION_CAUGHT2_K3B 0x2d
#define MOTION_CAUGHT32 0xe5            /* caught by CATCHER_32 (MOTION_CAUGHT32_STAIRS on stairs) */
#define MOTION_CAUGHT32_STAIRS 0x2e
#define MOTION_CAUGHT32_K3 0x40
#define CATCH_LOOP_2 0x19               /* frames she is pinned to the catcher's part */
#define CATCH_LOOP_32 0x2e
#define PART_HASH_CATCH 0x70e586bd2a900b2bull /* the catcher's part she is pinned to */
#define PART_HASH_STAIRS 0x1d3acfe1972cbull   /* a part of Alyssa's light bank: flag bit 0 off on stairs */
#define PARTS_REC_FLAGS10 0x10          /* part record: u_int, bit 0 cleared on the stairs (name from this use) */
#define PARTS_REC_TRANS_INIT 0xf0       /* part record: MODEL_PARTS.transInit, float[4] */
#define PARTS_REC_DRAW_ON_POS 0x2f0     /* part record: MODEL_PARTS.drawOnPos */

int ForceStandup;                       /* sbss 0x01944964 */
int MoveFrame;                          /* sbss 0x01944a84 */
static int iCatchLoop;                  /* iCatchLoop$2065, sbss 0x019449cc */
static const char kZRei[] = "Z_REI";    /* @3672, sdata 0x01937288 */

// The per-character trigger events (MOTION_RUN_EVENT): Character ids and their events, in the ROM's order.
static const struct {
    s_char id;
    short event;
} kRunEvents[] = {
    {33, 0x60}, {32, 0x5f}, {31, 0x5e}, {30, 0x5d}, {29, 0x5c}, {6, 0x5b},
    {5, 0x5a}, {4, 0x59}, {3, 0x58}, {2, 0x57}, {1, 0x56},
};

static u_char *PartOfBank(int bank, u_long hash)
{
    if (bank < 0 || pModelPartsHeader[bank] == NULL)
        return NULL;    // PORT: the ROM reads the header whatever the bank
    return asFindModelParts(pModelPartsHeader[bank], hash);
}

static void SetPartFlag0(u_char *part, int on)
{
    u_int f;
    if (part == NULL)
        return;
    memcpy(&f, part + PARTS_REC_FLAGS10, sizeof(f));
    f = on ? (f | 1u) : (f & ~1u);
    memcpy(part + PARTS_REC_FLAGS10, &f, sizeof(f));
}

static int RecInt(const u_char *rec, int off)
{
    int v;
    memcpy(&v, rec + off, sizeof(v));
    return v;
}

// Pinned by a catcher: for `loop` frames her xz follow the catcher's PART_HASH_CATCH part; the first
// frame (loop == first) takes its whole position and faces the catcher.
static void CatchPin(CHARACTER *ch, int catcher, int first)
{
    const CHARACTER *c = &Character[catcher];
    u_char *part = PartOfBank(c->bank, PART_HASH_CATCH);
    float p[4];
    if (part != NULL) {
        memcpy(p, part + PARTS_REC_TRANS_INIT, sizeof(p));
        if (iCatchLoop == first) {
            float d[4], up[4] = {0.0f, 0.0f, 1.0f, 1.0f};
            int k;
            float cp[4];
            memcpy(&ch->pos, p, sizeof(p));
            memcpy(cp, &c->pos, sizeof(cp));
            for (k = 0; k < 4; k++)
                d[k] = cp[k] - p[k];
            ch->rot = asRadianByVectors(d, up, 1) & 0xffff;
        } else {
            float me[4];
            memcpy(me, &ch->pos, sizeof(me));
            me[0] = p[0];
            me[2] = p[2];
            memcpy(&ch->pos, me, sizeof(me));
        }
    }
    iCatchLoop--;
}

// CT3 0x018f55d0
// What slot 0's motion record turns on each frame (asCharacterOperate): the facing flag, Alyssa's no-hit
// windows, the attack windows (ssHitAddAttackRequest per attack bit, the moth's bank too), each
// character's run event; for Alyssa then the fall out of the world, the stairs / panic / hide triggers,
// her turn speed, the lever mash's end, the boss's move ticks, and with no forced motion (CHARA_BHV_W518
// -1) the idle fidgets, the panic motions, the boss guards and the catchers' holds.
void asCharacterMortionFunctionOperation(CHARACTER *ch)
{
    short motion = CharaShort(ch, CHARA_MOTION_SLOTS);
    short prev = CharaShort(ch, CHARA_SLOT_PREV);
    int time = CharaInt(ch, CHARA_SLOT_TIME);
    u_char *rec = NULL;
    int kind = -1, facing = 3, kind2 = -1, run = -1, group = -1;
    int keep, i;
    u_int locked;

    if (CharaInt(ch, CHARA_MOTION_TABLE) == 0)
        return;
    if (motion != -1)
        rec = MotionRecord(ch, motion);
    if (rec != NULL) {
        kind = (s_char)rec[MOTION_REC_KIND];
        kind2 = (s_char)rec[MOTION_REC_KIND2];
        facing = (s_char)rec[MOTION_REC_FACING];
        run = (s_char)rec[MOTION_REC_RUN];
        group = (s_char)rec[MOTION_REC_GROUP];
    }
    ch->footAdjust = group == MOTION_GROUP_FEET ? (u_char)(ch->footAdjust | CHARA_FOOT_ADJUST_R | CHARA_FOOT_ADJUST_L) : 0;

    switch (facing) {
    case 1: case 3: case 4: case 5: case 6: case 7: case 8: case 0x0f: case 0x11:
        ch->flags |= CHARACTER_FLAG_FACING_HELD;
        break;
    case MOTION_FACING_STOCK_CAMERA:
        iCamChangeFlag = 0;
        iCameraBank_Stock = (short)Cam[CAMERA_SHARED].bank;
        iCameraID_Stock = Cam[CAMERA_SHARED].cut;
        // fall through
    case -1: case 0: case 9: case 10: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x10: case 0x12:
        ch->flags &= ~CHARACTER_FLAG_FACING_HELD;
        break;
    default:
        break;
    }
    ch->flags &= ~CHARACTER_FLAG_NO_HIT;
    if (ch->id == CHARA_ALYSSA && (group == MOTION_GROUP_NO_HIT_A || group == MOTION_GROUP_NO_HIT_B))
        ch->flags |= CHARACTER_FLAG_NO_HIT;

    if (kind2 == MOTION_KIND2_ATTACK) {
        if ((ch->id != CHARA_ALYSSA || motion != MOTION_WATER || MagicWaterCount != 0) &&
            CharaInt(ch, CHARA_HIT_LANDED) == 0) {
            int lo = RecInt(rec, MOTION_REC_HIT_START), hi = RecInt(rec, MOTION_REC_HIT_END);
            if (lo <= time && time <= hi) {
                int prevTime = time - gGlobalNextTicksInterval;
                int first = prevTime < lo || hi < prevTime;
                u_int bits = (u_int)RecInt(rec, MOTION_REC_HIT_ATTACKS);
                short power = RecShort(rec, MOTION_REC_HIT_POWER);
                for (i = 0; i < 32; i++) {
                    if ((bits & (1u << i)) == 0)
                        continue;
                    ssHitAddAttackRequest(ch->id, i, 0, power, -1, 1, motion, ch->lightBank);
                    if (first && ch->bank >= 0 && pModelPartsHeader[ch->bank] != NULL) {
                        // the attack's part starts its sweep where it stands
                        int idx = ssHit_GetMaxIDFromAtkID(ch->bank, i);
                        u_char *part = PARTS_REC(pModelPartsHeader[ch->bank], idx);
                        memmove(part + PARTS_REC_DRAW_ON_POS, part + PARTS_REC_TRANS_INIT, 16);
                    }
                    if (MothInfo.bank != 0)
                        ssHitAddAttackRequest(ch->id, i, 0, power, -1, 1, motion, MothInfo.bank);
                }
            }
        }
    } else if (kind2 == MOTION_KIND2_NO_HIT && RecInt(rec, MOTION_REC_HIT_START) <= time &&
               time <= RecInt(rec, MOTION_REC_HIT_END)) {
        if (ch->id == CHARA_ALYSSA && motion >= MOTION_BOW_EMPTY && motion <= MOTION_BOW_EMPTY + 3) {
            int v = ssTrigGetVariable(TRIG_VAR_BOW_53, 1, 0);
            if (v >= 0 && v < 3)
                ch->flags |= CHARACTER_FLAG_NO_HIT;
        } else {
            ch->flags |= CHARACTER_FLAG_NO_HIT;
        }
    }

    for (i = 0; i < (int)(sizeof(kRunEvents) / sizeof(kRunEvents[0])); i++) {
        if (ch->id != kRunEvents[i].id)
            continue;
        if (run == MOTION_RUN_EVENT)
            ssTrigRegisterEvent(kRunEvents[i].event, 1, 0);
        else
            ssTrigRemoveEvent(kRunEvents[i].event, 1, 0);
    }
    if (ch->id != CHARA_ALYSSA)
        return;

    // ---- Alyssa ----
    float pos[4];
    memcpy(pos, &ch->pos, sizeof(pos));
    if (pos[1] < FALL_OUT_Y) {
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_FATAL_FRONT);
        PlayerDamage(ALYSSA_FATAL_DAMAGE);
    }
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) != 0 && ssTrigCheckEvent(TRIG_EVENT_HIDE_HELD, 1, 0) != 0)
        ssTrigRemoveEvent(TRIG_EVENT_HIDE_HELD, 1, 0);
    {
        int stand = ssTrigCheckEvent(TRIG_EVENT_NO_HIT_B0, 1, 0) != 0;
        if (ForceStandup != 0 && !stand) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_STAND);
            asRapidLeverChangeOperate(RLC_FREE, 0, 0, 0, 0);
        }
        ForceStandup = stand;
    }
    if (run == MOTION_RUN_EVENT)
        ssTrigRegisterEvent(TRIG_EVENT_RUN_EVENT_ALYSSA, 1, 0);
    else
        ssTrigRemoveEvent(TRIG_EVENT_RUN_EVENT_ALYSSA, 1, 0);

    {
        u_short turn = TURN_SPEED_DEFAULT;
        if (kind2 == MOTION_KIND2_TURN_MID)
            turn = TURN_SPEED_MID;
        else if (kind2 == MOTION_KIND2_PANIC_F || kind2 == MOTION_KIND2_STAIRS_TURN)
            turn = (motion == TURN_SLOWEST_1 || motion == TURN_SLOWEST_2 || motion == TURN_SLOWEST_3 ||
                    motion == TURN_SLOWEST_4) ? TURN_SPEED_SLOWEST : TURN_SPEED_STAIRS;
        else if (kind2 == MOTION_KIND2_PANIC_4D)
            turn = TURN_SPEED_FAST;
        else if (kind2 == MOTION_KIND2_TURN_SLOW)
            turn = TURN_SPEED_DEFAULT;
        memcpy((u_char *)ch + CHARA_TURN_SPEED, &turn, sizeof(turn));
    }

    {
        u_char *part = PartOfBank(Character[CHARA_ALYSSA].lightBank, PART_HASH_STAIRS);
        SetPartFlag0(part, 1);
        if (kind == MOTION_KIND_STAIRS2) {
            ssTrigRegisterEvent(TRIG_EVENT_STAIRS_4B, 1, 0);
            SetPartFlag0(part, 0);
        } else {
            ssTrigRemoveEvent(TRIG_EVENT_STAIRS_4B, 1, 0);
        }
        ssTrigRemoveEvent(TRIG_EVENT_STAIRS_4C, 1, 0);
        if (kind == MOTION_KIND_STAIRS) {
            ssTrigRegisterEvent(TRIG_EVENT_STAIRS_4C, 1, 0);
            SetPartFlag0(part, 0);
        }
        if (kind2 == MOTION_KIND2_NO_HIT) {
            ssTrigRegisterEvent(TRIG_EVENT_STAIRS_4C, 1, 0);
            SetPartFlag0(part, 0);
        }
    }
    if (kind2 == MOTION_KIND2_PANIC_4D)
        ssTrigRegisterEvent(TRIG_EVENT_PANIC_ON_4D, 1, 0);
    else
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_ON_4D, 1, 0);
    if (kind2 == MOTION_KIND2_PANIC_8A ||
        (kind2 == MOTION_KIND2_STAIRS_TURN && ssTrigCheckEvent(TRIG_EVENT_HIDE_HELD, 1, 0) != 0))
        ssTrigRegisterEvent(TRIG_EVENT_PANIC_ON_8A, 1, 0);
    else
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_ON_8A, 1, 0);
    asCatWalkEventOperation(group == MOTION_GROUP_CATWALK ? CATWALK_TICK : CATWALK_RESET);

    if (prev == MOTION_WATER && motion != MOTION_WATER)
        ssTrigSetVariable(TRIG_VAR_WATER_USED, TRIG_VAR_WATER_DONE, 1, 0);
    keep = 0;
    if (motion == MOTION_BOSS5_STRUCK) {
        keep = 1;
    } else if (prev == MOTION_BOSS5_STRUCK) {
        int v = ssTrigGetVariable(TRIG_VAR_BOSS_ACTION, 1, 0);
        if (v == BOSS_ACTION_4)
            ssTrigSetVariable(TRIG_VAR_BOSS_ACTION, BOSS_ACTION_4_DONE, 1, 0);
        else if (v == BOSS_ACTION_E)
            ssTrigSetVariable(TRIG_VAR_BOSS_ACTION, BOSS_ACTION_E_DONE, 1, 0);
        asRapidLeverChangeOperate(RLC_FREE, 0, 0, 0, 0);
    }
    if ((motion == MOTION_FLINCH_BACK || motion == MOTION_FLINCH_FRONT) &&
        ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) != 0)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOSS5_DEAD);
    if (motion == MOTION_PULLED)
        keep = 1;
    else if (prev == MOTION_PULLED)
        asRapidLeverChangeOperate(RLC_FREE, 0, 0, 0, 0);
    if (group != MOTION_GROUP_MASH && !keep)
        asRapidLeverChangeOperate(RLC_FREE, 0, 0, 0, 0);

    MoveFrame = (group == MOTION_GROUP_MOVE_B || group == MOTION_GROUP_MOVE_A) ? MoveFrame + 1 : 0;
    if (MoveFrame > MOVE_FRAME_MAX) {
        if (pBossInfo != NULL) {
            int t;
            memcpy(&t, pBossInfo + BOSS_INFO_MOVE_TICKS, sizeof(t));
            t += BOSS_MOVE_TICKS;
            memcpy(pBossInfo + BOSS_INFO_MOVE_TICKS, &t, sizeof(t));
        }
        MoveFrame = 0;
    }

    if (CharaInt(ch, CHARA_BHV_W518) != -1)
        return;

    if (motion == MOTION_STAND && ch->keyTicks - IDLE_KEY_TICKS >= 0 && (FrameCounter & 0x1f) == 0 &&
        rndf() > IDLE_CHANCE && ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) == 0)
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, rndf() > IDLE_PICK ? MOTION_IDLE_A : MOTION_IDLE_B);
    if (motion == MOTION_PSTAND_WAIT)
        AfterPanicStartMuteki = MUTEKI_FOREVER;

    if ((asPanicStatusCheck(PANIC_STATUS_READ) & 3) == 1) {
        // the panic just started: the panic stand, and its record goes on to nothing
        u_char *wait = MotionRecord(&Character[CHARA_ALYSSA], MOTION_PSTAND_WAIT);
        short none = -1;
        if (wait != NULL)
            memcpy(wait + MOTION_REC_NEXT_MODE, &none, sizeof(none));
        asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_PSTAND_WAIT);
    } else {
        int done = Character[CHARA_ALYSSA].hp < 1;
        if (!done) {
            switch (group) {
            case 1: case 2: case 3: case 4: case 5: case 0x0f:
                asPanicEventOperation(PANIC_EV_RESET, 0);
                if ((asPanicStatusCheck(PANIC_STATUS_READ) & 3) == 3)
                    asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_PSTAND_NEXT);
                done = 1;
                break;
            case 6: case 7: case 8: case 9: case 10: case 0x0b: case 0x0c: case 0x0e:
            case 0x10: case 0x11: case 0x18: case 0x1a:
                done = 1;
                break;
            case 0x12: case 0x13: case 0x14: case 0x15: case 0x16:
                asPanicEventOperation(PANIC_EV_TICK, kind2);
                if ((asPanicStatusCheck(PANIC_STATUS_READ) & 1) == 0)
                    asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY,
                                                    kind2 == MOTION_KIND2_PANIC_F ? MOTION_STAND_PANIC_F :
                                                    kind2 == MOTION_KIND2_PANIC_E ? MOTION_STAND_PANIC_E : MOTION_STAND);
                done = 1;
                break;
            default:
                break;
            }
        }
        if (!done) {
            switch (kind2) {
            case 0: case 1: case 2: case 3: case 8: case 9: case 10: case 0x0b:
                asPanicEventOperation(PANIC_EV_RESET, 0);
                if ((asPanicStatusCheck(PANIC_STATUS_READ) & 3) == 3)
                    asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_PSTAND_NEXT);
                break;
            case MOTION_KIND2_PANIC_D: case MOTION_KIND2_PANIC_E: case MOTION_KIND2_PANIC_F:
                asPanicEventOperation(PANIC_EV_TICK, kind2);
                if ((asPanicStatusCheck(PANIC_STATUS_READ) & 1) == 0)
                    asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY,
                                                    kind2 == MOTION_KIND2_PANIC_F ? MOTION_STAND_PANIC_F : MOTION_STAND);
                break;
            default:
                break;
            }
        }
        if (motion == MOTION_PSTAND_MOVE && (FrameCounter & 3) == 0 && rec != NULL) {
            short mode = rndf() <= PSTAND_MOVE_STOP ? -1 : 0;
            memcpy(rec + MOTION_REC_NEXT_MODE, &mode, sizeof(mode));
        }
    }
    if (motion == MOTION_CAUGHT2_RELEASE || motion == MOTION_CAUGHT32_RELEASE)
        vsBossMuteki = MUTEKI_FOREVER;

    locked = (u_int)asAlyssaLockedByAnything(LOCKED_QUERY);
    if (locked == 0) {
        if (motion == MOTION_LOCKED_SHAKE) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_STAND);
            asDualShockVibrationOff();
        } else if (motion == MOTION_CAUGHT2_HOLD || motion == MOTION_CAUGHT2_HOLD2) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CAUGHT2_RELEASE);
            vsBossMuteki = MUTEKI_FOREVER;
        } else if (motion == MOTION_CAUGHT32_HOLD || motion == MOTION_CAUGHT32_HOLD2) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CAUGHT32_RELEASE);
            vsBossMuteki = MUTEKI_FOREVER;
        }
        iCatchLoop = 0;
    }
    if ((locked & LOCKED_KEEP_MASK) == LOCKED_BY_CHARA32) {
        u_int k = locked & LOCKED_KIND_MASK;
        if (k == LOCKED_KIND_3) {
            if (motion != MOTION_CAUGHT32_K3)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CAUGHT32_K3);
        } else if (k == LOCKED_KIND_2) {
            if (motion != MOTION_BOSS5_DEAD)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOSS5_DEAD);
        } else if (k == LOCKED_KIND_CATCH) {
            if (motion != MOTION_CAUGHT32 && motion != MOTION_CAUGHT32_HOLD && motion != MOTION_CAUGHT32_STAIRS) {
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY,
                                                kind == MOTION_KIND_STAIRS2 ? MOTION_CAUGHT32_STAIRS : MOTION_CAUGHT32);
                iCatchLoop = CATCH_LOOP_32;
            }
            if (iCatchLoop != 0)
                CatchPin(ch, CATCHER_32, CATCH_LOOP_32);
        }
    } else if ((locked & LOCKED_KEEP_MASK) == LOCKED_BY_CHARA2) {
        u_int k = locked & LOCKED_KIND_MASK;
        if (k == LOCKED_KIND_3) {
            if (motion != MOTION_CAUGHT2_K3 && motion != MOTION_CAUGHT2_K3B)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_CAUGHT2_K3);
        } else if (k == LOCKED_KIND_2) {
            if (motion != MOTION_BOSS5_DEAD)
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_BOSS5_DEAD);
        } else if (k == LOCKED_KIND_CATCH) {
            if (motion != MOTION_CAUGHT2 && motion != MOTION_CAUGHT2_STAIRS && motion != MOTION_CAUGHT2_HELD &&
                motion != MOTION_CAUGHT2_HOLD) {
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY,
                                                kind == MOTION_KIND_STAIRS2 ? MOTION_CAUGHT2_STAIRS : MOTION_CAUGHT2);
                iCatchLoop = CATCH_LOOP_2;
            }
            if (iCatchLoop != 0)
                CatchPin(ch, CATCHER_2, CATCH_LOOP_2);
        }
    } else if ((locked & LOCKED_KEEP_MASK) == 0 && ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_4D, 1, 0) == 0 &&
               ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_8A, 1, 0) == 0) {
        if ((locked & LOCKED_KIND_MASK) == LOCKED_KIND_2) {
            if (motion != MOTION_LOCKED_GRAB && motion != MOTION_LOCKED_GRAB2) {
                asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_LOCKED_GRAB);
                asDualShockVibrationOff();
            }
        } else if ((locked & LOCKED_KIND_MASK) == LOCKED_KIND_CATCH && motion != MOTION_LOCKED_SHAKE) {
            asCharacterMortionChangeRequest(0, CHARA_MOTION_REQ_PLAY, MOTION_LOCKED_SHAKE);
            asDualShockVibrationOn_Light(GRAB_SHAKE_TIME);
        }
    }
    asAlyssaLockedByAnything(LOCKED_TAKE);

    if (motion == MOTION_LOCKED_GRAB && time > GRAB_FATAL_TIME && Character[Player_BankID].hp > 0) {
        PlayerDamage(ALYSSA_FATAL_DAMAGE);
        SSPlaySE(kZRei, 5, 1);
    }
    if (group != MOTION_GROUP_AIM)
        SetCharaInt(ch, CHARA_AIM_W, 0);    // 0.0f
}

// CT3 0x018fe120
// The play rate of the motion `index` while one of the character's slots plays it: 0 when set, 1 when no
// slot plays it (tiBrain_Damage1..3).
int asChangeCharacterMotionPlayRate(int chara, int index, u_char rate)
{
    CHARACTER *ch = &Character[chara];
    const u_char *rec;
    BEHAVIOR *bhv;
    int slot;
    for (slot = 0; slot < CHARA_BHV_SLOT_COUNT; slot++) {
        if (CharaShort(ch, CHARA_MOTION_SLOTS + slot * 2) == index)
            break;
    }
    if (slot == CHARA_BHV_SLOT_COUNT)
        return 1;
    rec = MotionRecord(ch, index);
    bhv = rec != NULL ? asFindBehaviorByCharacter(chara, RecHash(rec)) : NULL;
    if (bhv != NULL)
        asChangeBehaviorAnimationPlayRate(bhv, rate);
    return 0;
}
