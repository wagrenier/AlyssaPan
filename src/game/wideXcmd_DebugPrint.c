// game/wideXcmd_DebugPrint.c -- Clock Tower 3, 0x018f3f60..0x018fe500: the wide xcmd debug print, then the as* player
// operation (asCharacterOperate's frame, hits and damage motions, the key tables asPIA*_KEY_TBL_*, the pad
// move, panic, the motion request queue). 109 functions; tumap.py keyed the file by its first function,
// and the as* run is probably its own object (or several): the slices that read them decide.
//
// Slice 1 (2026-11-10): asCharacterOperate (main's and QuatSlerp's per-frame call), the vibration
// helpers, Alyssa's hit reactions (Hiting, Hirumi), the key enable, the motion request queue, asClearBehaviorChangeFlag and the empty functions.
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

static const char kBreathSe[] = "c_0000"; /* @3214, sdata 0x01937280 */

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

#undef IN_PASS

// CT3 0x018f7ec0
void asPIA1_KEY_TBL_PULLED(void)
{
}

// CT3 0x018f8030
void asPIA1_KEY_TBL_WATER(void)
{
}

// CT3 0x018f8610
void asPIA1_KEY_TBL_PMOVEEND(void)
{
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

// CT3 0x018fe4b0
void ChrMoveControllerMode_init(void)
{
    controller_type = CONTROLLER_TYPE_DEFAULT;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018fbb50
void asAlyssaLockedByAnything(int mode)
{
    ALYSSAPAN_STUB("asAlyssaLockedByAnything", 0x018fbb50);
}

// stub of CT3 0x018f4c60
void asCharacterForceMoveOperate(CHARACTER *chr)
{
    ALYSSAPAN_STUB("asCharacterForceMoveOperate", 0x018f4c60);
}

// stub of CT3 0x018f4bd0
void asCharacterPositionReviewInPausing(CHARACTER *chr)
{
    ALYSSAPAN_STUB("asCharacterPositionReviewInPausing", 0x018f4bd0);
}

// stub of CT3 0x018f6f80
void asCharacterPositioning(CHARACTER *chr, int a1, int a2)
{
    ALYSSAPAN_STUB("asCharacterPositioning", 0x018f6f80);
}

// stub of CT3 0x018f40d0
void asKatchuKekkaiCheck(int a0)
{
    ALYSSAPAN_STUB("asKatchuKekkaiCheck", 0x018f40d0);
}

// stub of CT3 0x018fb940
void asMonsterDistanceStack_inPanic(int a0, int a1, int a2)
{
    ALYSSAPAN_STUB("asMonsterDistanceStack_inPanic", 0x018fb940);
}

// stub of CT3 0x018f42e0
void asPanicStatusCheck(int mode)
{
    ALYSSAPAN_STUB("asPanicStatusCheck", 0x018f42e0);
}

// stub of CT3 0x018fb4c0
void asRapidLeverChangeOperate(int a0, int a1, int a2, int a3, int a4)
{
    ALYSSAPAN_STUB("asRapidLeverChangeOperate", 0x018fb4c0);
}

// stub of CT3 0x018f73b0
void asResetCharacterRotation(int a0, int a1, int a2)
{
    ALYSSAPAN_STUB("asResetCharacterRotation", 0x018f73b0);
}

// stub of CT3 0x018fe380
void asInitPlayerBehaviorControler(CHARACTER *chr)
{
    ALYSSAPAN_STUB("asInitPlayerBehaviorControler", 0x018fe380);
}

// stub of CT3 0x018f7040
void as1CharacterPositioning(CHARACTER *chr)
{
    ALYSSAPAN_STUB("as1CharacterPositioning", 0x018f7040);
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

// stub of CT3 0x018f7cd0
void asCharacterAutoAiming(CHARACTER *ch)
{
    ALYSSAPAN_STUB("asCharacterAutoAiming", 0x018f7cd0);
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
