// game/asXcmd.c -- Clock Tower 3: the trigger script's x commands by "as" (0x018dab60..0x018dc1e0): the
// panic sources, the moths and the arrows a stage arms, hit points, slow motion, the camera's quake,
// reset and look-around stick, attaching a part to another bank's part, a character's auto-aim, seek
// bars, motions by the playing motion, the shadow's draw order, and the attack-target list main.c
// clears. Each command takes its argument strings (argv[0..argc-1]) and the bank whose script runs it;
// ssTrigExecuteActCmd (ssGrpPortalCheck.c) calls them through ssXcmd_functions and each answers
// XCMD_DONE. Written 2026-11-11: all 20 functions.
//
// PORT: the character's motion table (CHARACTER +CHARA_MOTION_TABLE) is an EE word of the character's
// bank (asInitPlayerBehaviorControler, wideXcmd_DebugPrint.c); XcmdCharaMotionTable resolves it.

#include <stdlib.h>
#include <string.h>
#include "game/asXcmd.h"
#include "game/asMothRouteSearch.h"
#include "game/asUseArrow.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/ahalloc.h"
#include "game/ct3_types.h"
#include "game/fileio.h"
#include "game/hhXcmd_DressUp.h"
#include "game/main.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/xcmd_tn.h"
#include "sdk/newlib.h"

ATTACK_TARGET AttackTargetStack[ATTACK_TARGET_MAX];    /* sbss 0x0193fa30 */
static int TrgCharStopFlag_Stack;                      /* CT3 0x019447c8 (TrgCharStopFlag_Stack$279):
                                                          TrgCharStopFlag before ALYSSA_ComeHere */

// newlib's atoff (float atof), which the ROM links; the host libc has atof (tiXcmd_TransItem.c's too).
static float atoff(const char *s)
{
    return (float)atof(s);
}

// The CHARACTER behaviour-block words (wideXcmd_DebugPrint.h's offsets: the block is a byte array).
static short CharaGetShort(const CHARACTER *ch, int off)
{
    short v;
    memcpy(&v, (const u_char *)ch + off, sizeof(v));
    return v;
}

static void CharaSetShort(CHARACTER *ch, int off, short v)
{
    memcpy((u_char *)ch + off, &v, sizeof(v));
}

static int CharaGetInt(const CHARACTER *ch, int off)
{
    int v;
    memcpy(&v, (const u_char *)ch + off, sizeof(v));
    return v;
}

static void CharaSetInt(CHARACTER *ch, int off, int v)
{
    memcpy((u_char *)ch + off, &v, sizeof(v));
}

// Part `index` of bank `bank` (MODEL_PARTS is indexed from the header).
static MODEL_PARTS *XcmdBankPart(int bank, int index)
{
    return &((MODEL_PARTS *)pModelPartsHeader[bank])[index];
}

// CT3 @349..@357 (rodata 0x01936a00..0x01936af0): asXcmd_PanicSet's names, indexed by PanicSet's source.
static const char *const PanicSourceName[PANIC_SOURCE_NAMES] = {
    "PursuitPanicPriority",
    "HidingPanicPriority",
    "ApproachesPanicPriority",
    "NoPanicPriority",
    "NormalPanicPriority",
    "NoPanicNormalPanicPriority",
    "NormalPanicOnPriority",
    "HitPanicPriority",
    "PanicValuePriority",                               /* PANIC_RISE_GAUGE */
};

// CT3 0x018dab60: PanicSet(argv[1], the source named argv[0]); an unknown name does nothing. The ROM
// tests the names one by one in this order.
int asXcmd_PanicSet(int argc, char **argv, int bank)
{
    int source;

    (void)argc;
    (void)bank;
    for (source = 0; source < PANIC_SOURCE_NAMES; source++) {
        if (strcmp(argv[0], PanicSourceName[source]) == 0) {
            PanicSet(atoff(argv[1]), source);
            break;
        }
    }
    return XCMD_DONE;
}

// CT3 0x018dad50: script variable argv[0] (global argv[1]) = the distance between Character[argv[2]]
// and Character[argv[3]] (3D, truncated).
int asXcmd_DistanceBetweenChar(int argc, char **argv, int bank)
{
    float a[4], b[4], d[4];
    int i;
    float len;

    (void)argc;
    memcpy(a, &Character[atoi(argv[2])].pos, sizeof(a));
    memcpy(b, &Character[atoi(argv[3])].pos, sizeof(b));
    for (i = 0; i < 4; i++) {
        d[i] = a[i] - b[i];
    }
    len = asVectorLength3D(d);
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, (int)len, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018dae40: argv[0] != 0 (and none running): Alyssa (Character[0]) turns to part argv[1] of the
// script's bank and plays motion argv[2] with the menu stopped; argv[0] == 0, or a second call while one
// runs, ends it (the stop flags back as they were).
int asXcmd_ALYSSA_ComeHere(int argc, char **argv, int bank)
{
    CHARACTER *alyssa = &Character[0];
    int motion;

    (void)argc;
    if (atoi(argv[0]) == 0 || CharaGetInt(alyssa, CHARA_COME_HERE_MOTION) != -1) {
        TrgCharStopFlag = TrgCharStopFlag_Stack;
        CharaSetInt(alyssa, CHARA_COME_HERE_MOTION, -1);
    } else {
        TrgCharStopFlag_Stack = TrgCharStopFlag;
        CharaSetShort(alyssa, CHARA_AIM_SPEED, COME_HERE_AIM_SPEED);
        CharaSetShort(alyssa, CHARA_AIM_BANK, (short)bank);
        CharaSetInt(alyssa, CHARA_AIM_PART, atoi(argv[1]));
        motion = asFindMotionID(0, ahGetHash(argv[2]));
        CharaSetInt(alyssa, CHARA_COME_HERE_MOTION, motion);
        if (motion >= 0) {
            TrgCharStopFlag |= TRG_CHAR_STOP_MENU;
            asCharacterMortionChangeRequest(0, 0, motion);
            return XCMD_DONE;
        }
    }
    CharaSetInt(alyssa, CHARA_AIM_PART, 0);
    return XCMD_DONE;
}

// CT3 0x018daf30: asForceOperateMirroring_Character(argv[0], argv[1]).
int asXcmd_ForceOperateMirroring(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    asForceOperateMirroring_Character(atoi(argv[0]), atoi(argv[1]));
    return XCMD_DONE;
}

// CT3 0x018daf80: argv[1] == 0: the scene bank named argv[0]; else Character[argv[0]]: force the
// animation mode argv[2].
int asXcmd_ForceOperateAnimation(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    if (atoi(argv[1]) == 0) {
        asForceOperateAnimation_Scene(ssGetBankID(argv[0]), atoi(argv[2]));
    } else {
        asForceOperateAnimation_Character(atoi(argv[0]), atoi(argv[2]));
    }
    return XCMD_DONE;
}

// CT3 0x018db000: moth argv[0] of the armed bank: its route (any old one freed). With no route numbers
// (argc 4: argv[1..3] unused) it rests (MOTH_STATE_REST); else it takes the script's bank, argv[1..3], and the
// route argv[4..] (routeCount points, routeStep the step), and next MOTH_STATE_WAIT starts it.
int asXcmd_SetMoth(int argc, char **argv, int bank)
{
    MOTH *moth;
    short *route;
    int i;

    if (MothInfo.active == 0) {
        return XCMD_DONE;
    }
    moth = &MothInfo.list[atoi(argv[0])];
    if (moth->mem != NULL) {
        _SunFree(&gHeapJunc, &moth->mem);
        moth->mem = NULL;
        moth->frames = 0;
        moth->speed = 0;
        moth->angle = 0;
        moth->radius = 0;
        moth->timer = 0;
        memset(moth->target, 0, sizeof(moth->target));
    }
    moth->routeCount = (short)(argc - 4);
    if (moth->routeCount == 0) {
        moth->state = MOTH_STATE_REST;
        moth->next = 0;
        moth->bank = 0;
        moth->home[0] = 0;
        moth->home[1] = 0;
        moth->home[2] = 0;
        moth->routeStep = 0;
    } else {
        moth->bank = (u_char)bank;
        moth->home[0] = (short)atoi(argv[1]);
        moth->home[1] = (short)atoi(argv[2]);
        moth->home[2] = (short)atoi(argv[3]);
        moth->mem = _SunAlloc(&gHeapJunc, (u_short)moth->routeCount * 2, 2);
        route = (short *)moth->mem;
        for (i = 0; i < (u_short)moth->routeCount; i++) {
            route[i] = (short)atoi(argv[i + 4]);
        }
        moth->routeStep = 0;
        moth->next = MOTH_STATE_WAIT;
    }
    return XCMD_DONE;
}

// CT3 0x018db160: arm the script's bank's parts as moths, four numbers each (MOTH.arg); argc 0 drops
// them (and loads MOTH_SE_BANK_GONE back). Arming loads the moth SEs and removes the global event
// TRIG_EVENT_MOTHS_ARMED.
int asXcmd_I_have_x_Mothes(int argc, char **argv, int bank)
{
    static char seBank[] = MOTH_SE_BANK_ARMED;
    char *seArgv[1] = {seBank};
    MOTH *moth;
    int i;

    if (argc == 0) {
        asDestroyMothInfo(1);
        return XCMD_DONE;
    }
    MothInfo.bank = (u_char)bank;
    MothInfo.active = 1;
    MothInfo.count = (u_char)(argc >> 2);
    strcpy(MothInfo.bankName, BankInfo[bank].name);
    MothInfo.stage = strcmp(MothInfo.bankName, MOTH_STAGE_1A_BANK) == 0 ? MOTH_STAGE_1A : MOTH_STAGE_OTHER;
    MothInfo.list = (MOTH *)_SunAlloc(&gHeapJunc, MothInfo.count * (int)sizeof(MOTH), 0x50);
    MothInfo.shown = 0;
    MothInfo.clatter = -1;
    for (i = 0; i < MothInfo.count; i++) {
        moth = &MothInfo.list[i];
        moth->arg[0] = (short)atoi(argv[i * 4 + 0]);
        moth->arg[1] = (short)atoi(argv[i * 4 + 1]);
        moth->arg[2] = (short)atoi(argv[i * 4 + 2]);
        moth->arg[3] = (short)atoi(argv[i * 4 + 3]);
        moth->state = MOTH_STATE_REST;
        moth->frames = 0;
        moth->speed = 0;
        moth->angle = 0;
        moth->radius = 0;
        moth->bank = 0;
        moth->home[0] = 0;
        moth->home[1] = 0;
        moth->home[2] = 0;
        moth->mem = NULL;
        moth->routeCount = 0;
        moth->routeStep = 0;
        moth->next = 0;
        moth->timer = 0;
        memset(moth->target, 0, sizeof(moth->target));
    }
    hhXcmd_LoadSE(1, seArgv, 0);
    ssTrigRemoveEvent(TRIG_EVENT_MOTHS_ARMED, 1, 0);
    return XCMD_DONE;
}

// CT3 0x018db340: the attack targets (AttackTargetStack). argc ATTACK_TARGET_CLEAR_ALL frees every
// entry; argc >= ATTACK_TARGET_DROP_BANK frees bank's and those whose bank is no longer loaded; else
// each argv[i] is added for bank unless there already (the list full: the rest are dropped). Only the
// bank half of an entry is cleared.
int asXcmd_AddForLockToTargChara(int argc, char **argv, int bank)
{
    int i, j;
    u_int target;

    if (argc == ATTACK_TARGET_CLEAR_ALL) {
        for (j = 0; j < ATTACK_TARGET_MAX; j++) {
            AttackTargetStack[j].bank = 0;
        }
    } else if ((u_int)argc < ATTACK_TARGET_DROP_BANK) {
        for (i = 0; i < argc; i++) {
            target = (u_int)atoi(argv[i]);
            for (j = 0; j < ATTACK_TARGET_MAX; j++) {
                if (AttackTargetStack[j].bank == (u_int)bank && AttackTargetStack[j].target == target) {
                    break;
                }
            }
            if (j < ATTACK_TARGET_MAX) {
                continue;
            }
            for (j = 0; j < ATTACK_TARGET_MAX; j++) {
                if (AttackTargetStack[j].bank == 0) {
                    AttackTargetStack[j].bank = (u_short)bank;
                    AttackTargetStack[j].target = (u_short)target;
                    break;
                }
            }
            if (j >= ATTACK_TARGET_MAX) {
                return XCMD_DONE;
            }
        }
    } else {
        for (j = 0; j < ATTACK_TARGET_MAX; j++) {
            if (AttackTargetStack[j].bank == (u_int)bank
                || BankInfo[AttackTargetStack[j].bank].state != BANK_STATE_LOADED) {
                AttackTargetStack[j].bank = 0;
            }
        }
    }
    return XCMD_DONE;
}

// CT3 0x018db510: the script's bank's camera at its cut's destination at once: eye, focus point and up
// = their *To, the live focal length = the target one.
int asXcmd_SettingCameraReset(int argc, char **argv, int bank)
{
    CAMERA *cam = &Cam[asCheckBankID(bank)];

    (void)argc;
    (void)argv;
    memcpy(cam->eye, cam->eyeTo, sizeof(cam->eye));
    memcpy(cam->focus, cam->focusTo, sizeof(cam->focus));
    memcpy(cam->up, cam->upTo, sizeof(cam->up));
    cam->focal[0] = cam->focal[2];
    return XCMD_DONE;
}

// CT3 0x018db580: Character[argv[0]]'s hit points: argv[1] "/n", "*n", "-n", "+n" or "n" (set), then
// kept in [0, hpMax].
int asXcmd_SetHitPoint(int argc, char **argv, int bank)
{
    CHARACTER *ch = &Character[atoi(argv[0])];
    const char *v = argv[1];

    (void)argc;
    (void)bank;
    switch (v[0]) {
    case '/': {
        int n = atoi(v + 1);
        // PORT: the EE's division by 0 does not trap; the host leaves hp as it is.
        if (n != 0) {
            ch->hp /= n;
        }
        break;
    }
    case '*':
        ch->hp *= atoi(v + 1);
        break;
    case '-':
        ch->hp -= atoi(v + 1);
        break;
    case '+':
        ch->hp += atoi(v + 1);
        break;
    default:
        ch->hp = atoi(v);
        break;
    }
    if (ch->hp >= ch->hpMax) {
        ch->hp = ch->hpMax;
    } else if (ch->hp < 0) {
        ch->hp = 0;
    }
    return XCMD_DONE;
}

// CT3 0x018db770: the animation speed in percent (anim_play_slow, 16 bits); 0 = normal (100).
int asXcmd_SlowMotion(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    anim_play_slow = atoi(argv[0]) & 0xffff;
    if (anim_play_slow == 0) {
        anim_play_slow = 100;
    }
    return XCMD_DONE;
}

// CT3 0x018db7b0: shake axis argv[0] (CamQuake): amplitude argv[1], period argv[2], fade in argv[3],
// hold argv[4], fade out argv[5], phase argv[6] (0 with six arguments); it runs from its start.
int asXcmd_CameraQuake(int argc, char **argv, int bank)
{
    CAM_QUAKE *q = &CamQuake[atoi(argv[0])];

    (void)bank;
    q->amp = atoff(argv[1]);
    q->period = (u_short)atoi(argv[2]);
    q->fadeIn = (u_short)atoi(argv[3]);
    q->hold = (u_short)atoi(argv[4]);
    q->fadeOut = (u_short)atoi(argv[5]);
    q->duration = (u_short)(q->fadeOut + q->fadeIn + q->hold);
    q->time = 0;
    if (argc == CAM_QUAKE_ARGC_NO_PHASE) {
        q->phase = 0;
    } else {
        q->phase = (u_short)atoi(argv[6]);
    }
    return XCMD_DONE;
}

// CT3 0x018db880: part argv[0] of the script's bank becomes an attached part (PARTS_KIND_ATTACHED,
// shown). ATTACH_MODE_HIDE hides it; modes 0 / 1 attach it to part argv[4] (by name) of the bank
// argv[3] (argv[2] == ATTACH_TO_BANK) or of Character[argv[3]]'s bank, when that bank is loaded.
// Another mode leaves it unattached.
int asXcmd_AttachObject(int argc, char **argv, int bank)
{
    MODEL_PARTS *part = XcmdBankPart(bank, atoi(argv[0]));
    u_char *rec = (u_char *)part;
    short to;
    short id;
    int mode;

    (void)argc;
    part->flags &= ~PARTS_FLAG_HIDDEN;
    part->kind = PARTS_KIND_ATTACHED;
    id = 0;
    memcpy(rec + PARTS_ATTACH_MODE, &id, sizeof(id));
    memcpy(rec + PARTS_ATTACH_PART, &id, sizeof(id));
    mode = atoi(argv[1]);
    if (mode == ATTACH_MODE_HIDE) {
        if (part->kind == PARTS_KIND_ATTACHED) {
            part->flags |= PARTS_FLAG_HIDDEN;
        }
        return XCMD_DONE;
    }
    if (mode != ATTACH_MODE_0) {
        if (mode != ATTACH_MODE_1) {
            return XCMD_DONE;
        }
        id = 1;
        memcpy(rec + PARTS_ATTACH_MODE, &id, sizeof(id));
    }
    if (atoi(argv[2]) == ATTACH_TO_BANK) {
        to = (short)atoi(argv[3]);
    } else {
        to = Character[atoi(argv[3])].bank;
    }
    memcpy(rec + PARTS_ATTACH_BANK, &to, sizeof(to));
    if (to != -1 && BankInfo[to].state == BANK_STATE_LOADED) {
        id = (short)asFindModelPartsID(pModelPartsHeader[to], ahGetHash(argv[4]));
        memcpy(rec + PARTS_ATTACH_PART, &id, sizeof(id));
    }
    return XCMD_DONE;
}

// CT3 0x018dba20: nothing (asShootArrowSetting shoots).
int asXcmd_ShootArrow(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018dba30: arm the script's bank's parts as arrows, three numbers each (part, mark part, hit
// id); any arrows armed before are dropped first, and argc 0 only drops them. Each arrow part waits
// parked (ARROW_PART_PARKED_Y); the bow's SEs are loaded.
int asXcmd_I_have_x_Arrows(int argc, char **argv, int bank)
{
    static char seBank[] = ARROW_SE_BANK;
    char *seArgv[1] = {seBank};
    ARROW *arrow;
    MODEL_PARTS *part;
    int i;

    if (ArrowInfo.list != NULL) {
        asDestroyArrowInfo();
    }
    if (argc == 0) {
        asDestroyArrowInfo();
        return XCMD_DONE;
    }
    ArrowInfo.active = 1;
    ArrowInfo.bank = (u_char)bank;
    strcpy(ArrowInfo.bankName, BankInfo[bank].name);
    ArrowInfo.count = (u_char)((u_int)argc / 3);
    ArrowInfo.list = (ARROW *)_SunAlloc(&gHeapJunc, ArrowInfo.count * (int)sizeof(ARROW), 0x50);
    for (i = 0; i < ArrowInfo.count; i++) {
        arrow = &ArrowInfo.list[i];
        arrow->state = ARROW_STATE_READY;
        arrow->level = 0;
        arrow->kind = 0;
        arrow->shootArg1 = 0;
        arrow->counter = 0;
        arrow->part = (u_short)atoi(argv[i * 3 + 0]);
        arrow->markPart = (u_short)atoi(argv[i * 3 + 1]);
        arrow->hitId = (short)atoi(argv[i * 3 + 2]);
        arrow->dir[0] = 0.0f;                           /* vf0 */
        arrow->dir[1] = 0.0f;
        arrow->dir[2] = 0.0f;
        arrow->dir[3] = 1.0f;
        part = XcmdBankPart(bank, arrow->part);
        part->flags43 = ARROW_PART_FLAGS43_ARMED;
        part->drawOnPos[1] = ARROW_PART_PARKED_Y;
    }
    hhXcmd_LoadSE(1, seArgv, 0);
    return XCMD_DONE;
}

// CT3 0x018dbc20: Character[argv[0]]'s auto-aim: turn speed argv[3] degrees (CHARA_AIM_SPEED = its
// 16-bit angle / 300; 0 = off, no part), at part argv[2] (by name) of the script's bank (argv[1] ==
// AUTO_AIM_OWN_BANK) or of Character[argv[1]]'s bank.
int asXcmd_CharacterAutoAiming(int argc, char **argv, int bank)
{
    CHARACTER *ch = &Character[atoi(argv[0])];
    int other;
    short aimBank;
    int part;

    (void)argc;
    CharaSetShort(ch, CHARA_AIM_SPEED, (short)(XCMD_DEG_TO_ANGLE(atoi(argv[3])) / AUTO_AIM_SPEED_DIV));
    if (CharaGetShort(ch, CHARA_AIM_SPEED) == 0) {
        CharaSetInt(ch, CHARA_AIM_PART, 0);
        return XCMD_DONE;
    }
    other = atoi(argv[1]);
    aimBank = other == AUTO_AIM_OWN_BANK ? (short)bank : Character[other].bank;
    CharaSetShort(ch, CHARA_AIM_BANK, aimBank);
    part = asFindModelPartsID(pModelPartsHeader[(u_short)aimBank], ahGetHash(argv[2]));
    if (part < -1) {
        part = 0;
    }
    CharaSetInt(ch, CHARA_AIM_PART, part);
    return XCMD_DONE;
}

// CT3 0x018dbd80: the script's bank's seek bar on part argv[0]: its plane is part argv[2] of
// Character[argv[1]]'s bank (by name), or, argv[1] < 0, the bar's bank's part argv[2] (a number); then
// (argc - 3) / 4 animations of four numbers (id, mode, frame at t 0, frame at t 1, frames * 10).
int asXcmd_SeekBarSetting(int argc, char **argv, int bank)
{
    SEEKBAR_X *x = SeekBarX[bank];
    SEEKBAR *bar = NULL;
    SEEKBAR_ANIM *anim;
    int i;

    // PORT: the ROM reads the bank's ALL_TABLE +0xe8 word, set for every scene; the host's SeekBarX is
    // NULL for a bank asSeekBarInitializeForX gave none.
    if (x == NULL) {
        return XCMD_DONE;
    }
    for (i = 0; i < x->count; i++) {
        if (x->bars[i].part == (u_int)atoi(argv[0])) {
            bar = &x->bars[i];
            break;
        }
    }
    if (bar == NULL) {
        return XCMD_DONE;
    }
    bar->chara = (short)atoi(argv[1]);
    if (bar->chara < 0) {
        bar->planePart = (u_short)atoi(argv[2]);
    } else {
        bar->hash = ahGetHash(argv[2]);
    }
    bar->animCount = (u_short)((u_int)(argc - 3) >> 2);
    if (bar->anims != NULL) {
        _SunFree(&gHeapJunc, (void **)&bar->anims);
    }
    bar->anims = (SEEKBAR_ANIM *)_SunAlloc(&gHeapJunc, bar->animCount * (int)sizeof(SEEKBAR_ANIM), 4);
    for (i = 0; i < bar->animCount; i++) {
        char **a = &argv[3 + i * 4];
        anim = &bar->anims[i];
        anim->id = (u_short)atoi(a[0]);
        anim->mode = (short)atoi(a[1]);
        anim->frameA = atoi(a[2]) * SEEKBAR_FRAME_SCALE;
        anim->frameB = atoi(a[3]) * SEEKBAR_FRAME_SCALE;
    }
    return XCMD_DONE;
}

// CT3 0x018dbf40: the look-around stick: argv[0] 0 / 1 picks the stick pair (CameGri_Mode) with the
// full-deflection pitch argv[1] and yaw argv[2] in degrees; CAMEGRI_CMD_OFF turns it off;
// CAMEGRI_CMD_RECENTRE zeroes the eased deflection. Other values do nothing.
int asXcmd_CameraGuriGuriSetting(int argc, char **argv, int bank)
{
    int mode = atoi(argv[0]);

    (void)argc;
    (void)bank;
    if (mode == 1 || mode == 0) {
        CameGri_Mode = (u_char)mode;
        CameGri_DR = (u_int)XCMD_DEG_TO_ANGLE(atoi(argv[1]));
        CameGri_DU = (u_int)XCMD_DEG_TO_ANGLE(atoi(argv[2]));
    } else if (mode == CAMEGRI_CMD_OFF) {
        CameGri_Mode = CAMEGRI_MODE_OFF;
    } else if (mode == CAMEGRI_CMD_RECENTRE) {
        CameGri_dvsX = 0.0f;
        CameGri_dvsZ = 0.0f;
    }
    return XCMD_DONE;
}

// PORT: CHARACTER +CHARA_MOTION_TABLE is an EE word of the character's bank (asInitPlayerBehaviorControler
// writes it); NULL without one.
static const u_char *XcmdCharaMotionTable(const CHARACTER *ch)
{
    u_int ee;
    if (ch->bank < 0)
        return NULL;
    memcpy(&ee, (const u_char *)ch + CHARA_MOTION_TABLE, sizeof(ee));
    return (const u_char *)BankEePtr(ch->bank, ee);
}

// CT3 0x018dc050: Character[argv[0]] (operated: CHARACTER_FLAG_REFRACTION): the pairs (argv[1], argv[2]),
// (argv[3], argv[4]), ...: the first whose first motion plays in one of its slots requests the second.
int asXcmd_PlayMotionByNowMotion(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);
    CHARACTER *ch = &Character[chara];
    const u_char *table;
    u_long now, rec;
    short idx;
    int i, slot;

    (void)bank;
    if ((ch->flags & CHARACTER_FLAG_REFRACTION) == 0) {
        return XCMD_DONE;
    }
    table = XcmdCharaMotionTable(ch);
    if (table == NULL) {
        return XCMD_DONE;
    }
    for (i = 0; i < (int)((u_int)(argc - 1) >> 1); i++) {
        now = ahGetHash(argv[1 + i * 2]);
        for (slot = 0; slot < CHARA_MOTION_SLOT_COUNT; slot++) {
            idx = CharaGetShort(ch, CHARA_MOTION_SLOTS + slot * 2);
            if (idx == -1) {
                continue;
            }
            memcpy(&rec, table + idx * CHARA_MOTION_SIZE, sizeof(rec));
            if (now == rec) {
                asCharacterMortionChangeRequestHash(chara, CHARA_MOTION_REQ_PLAY, ahGetHash(argv[2 + i * 2]));
                return XCMD_DONE;
            }
        }
    }
    return XCMD_DONE;
}

// CT3 0x018dc1b0: the shadow's translucent draw order (asShadowDrawing_SetTransOrderBW, the low byte).
int asXcmd_ShadowOrderBW(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    asShadowDrawing_SetTransOrderBW((char)(atoi(argv[0]) & 0xff));
    return XCMD_DONE;
}
