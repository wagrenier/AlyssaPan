// game/tiXcmd_TransItem.c -- Clock Tower 3: the trigger script's x commands by ti (tiXcmd_*), with a few
// by tk / wide (0x018e0e80..0x018e90a0). Each takes the command's argument strings and the bank whose
// script runs it; ssTrigExecuteActCmd (ssGrpPortalCheck.c) calls them through ssXcmd_functions.
// Slice 1 (2026-11-11): the item examination moves, the scene / map commands, the free slots, the range
// and earthquake checks, the shade-off, the cross-fade break, the pendulum reset, the reserved camera
// change. Left: OperateItem, Balows2th, and the boss commands whose pointer words wait for the boss
// TU's host layout (SetWarp, the SCISSORS_MAN attack, SetChain*, GetBloodAttackObject), and the rest.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "common/alyssapan_stub.h"
#include "game/tiXcmd_TransItem.h"
#include "game/xcmd_tn.h"
#include "game/Clipping_Portal.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/LinkMatrixTree.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaSinCos.h"

// newlib's atoff (float atof), which the ROM links; the host libc has atof.
static float atoff(const char *s)
{
    return (float)atof(s);
}

// The file's statics (the symtab's LOCAL objects).
static float trans_add_value;           /* sbss 0x01944880: the examined item's move sum, 0..100 */
alignas(16) static float DirectToCamera[4]; /* sbss 0x01944870: unit vector item -> eye */
static int tiReserveTime;               /* sbss 0x01944888: tiXcmd_ChangeCam_Reserve's change */
static short tiReserveType;             /* sbss 0x0194488c */
static short tiReserveCamID;            /* sbss 0x01944890 */
static int tiReserveBank;               /* sbss 0x01944894 */

// The bank's part i (the ROM: header + i * 0x380; the record proper starts at +0x40, MODEL_PARTS keeps
// the header-relative offsets).
static MODEL_PARTS *TiPart(int bank, int i)
{
    return (MODEL_PARTS *)(pModelPartsHeader[bank] + i * PARTS_REC_SIZE);
}

// The EE address of part i's record (header + 0x40 + i * 0x380), as the ROM keeps it in pSrcMPstock /
// pDstMPstock.
static u_int TiPartEe(int bank, int i)
{
    return BankEeAddr(bank, pModelPartsHeader[bank] + PARTS_HDR_RECORDS + i * PARTS_REC_SIZE);
}

// |a - b| over x, y, z (the ROM: VSUB.xyz, VMUL.xyz, VADDy/z.x, VRSQRT, then VDIV 1 / that).
static float TiDistXYZ(const float *a, const float *b)
{
    float d[3];

    d[0] = a[0] - b[0];
    d[1] = a[1] - b[1];
    d[2] = a[2] - b[2];
    return 1.0f / (1.0f / sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]));
}

// v (x, y, z) scaled to length 1 (VRSQRT then VMULq.xyz; w kept).
static void TiNormalizeXYZ(float *v)
{
    float q = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= q;
    v[1] *= q;
    v[2] *= q;
}

// The item under examination moves along DirectToCamera: its placement row 3 (drawOnPos) by step.
static void TiItemMove(MODEL_PARTS *part, float step)
{
    part->drawOnPos[0] += DirectToCamera[0] * step;
    part->drawOnPos[1] += DirectToCamera[1] * step;
    part->drawOnPos[2] += DirectToCamera[2] * step;
}

// CT3 0x018e0e80: moves the examined item part argv[0] toward the camera by argv[1] (trans_add_value
// keeps the sum in 0..100; at a bound the sum is clamped and the part stays).
int tiXcmd_TransItem(int argc, char **argv, int bank)
{
    MODEL_PARTS *part = TiPart(bank, atoi(argv[0]));
    float step;

    (void)argc;
    if (part->kind != PARTS_KIND_ITEM_LOCKED) {
        step = atoff(argv[1]);
        part->flags43 |= PARTS_FLAG43_WARPED;
        trans_add_value += step;
        if (trans_add_value < 0.0f)
            trans_add_value = 0.0f;
        else if (trans_add_value > TI_ITEM_TRANS_MAX)
            trans_add_value = TI_ITEM_TRANS_MAX;
        else
            TiItemMove(part, step);
    }
    return XCMD_DONE;
}

// CT3 0x018e1550: before examining item part argv[0]: the move sum to 0 and DirectToCamera = the unit
// vector from the part (drawOnPos) to Cam[0]'s eye.
int tiXcmd_ItemCheckInit(int argc, char **argv, int bank)
{
    MODEL_PARTS *part = TiPart(bank, atoi(argv[0]));
    float eye[4];

    (void)argc;
    if (part->kind == PARTS_KIND_ITEM_LOCKED)
        return XCMD_DONE;
    trans_add_value = 0.0f;
    // The ROM passes one stack qword for all four outputs; pos is written last, so only it survives.
    asViewMatrix2PosRUF(&Cam[TI_ITEM_CAMERA].view[0][0], eye, NULL, NULL, NULL);
    DirectToCamera[0] = eye[0] - part->drawOnPos[0];
    DirectToCamera[1] = eye[1] - part->drawOnPos[1];
    DirectToCamera[2] = eye[2] - part->drawOnPos[2];
    DirectToCamera[3] = eye[3];
    TiNormalizeXYZ(DirectToCamera);
    return XCMD_DONE;
}

// CT3 0x018e1610: the variable argv[0] (global) gets the current scene's map: 0 for "0A..", 1 "1A..",
// 2 "1C..", 3 "5A.." (the scene file's first two characters); any other scene leaves it.
int tiXcmd_SetSceneID(int argc, char **argv, int bank)
{
    static const char kScenes[][2] = { { '0', 'A' }, { '1', 'A' }, { '1', 'C' }, { '5', 'A' } };
    int i;

    (void)argc;
    for (i = 0; i < (int)(sizeof(kScenes) / sizeof(kScenes[0])); i++) {
        if (BankInfo[idCurrentScene].name[0] == kScenes[i][0] && BankInfo[idCurrentScene].name[1] == kScenes[i][1])
            ssTrigSetVariable(atoi(argv[0]) & 0xffff, i, 1, bank);
    }
    return XCMD_DONE;
}

// CT3 0x018e1810: places the map pieces argv[1] / argv[2] of this bank where character argv[0] stands
// on its room's map: the offset from the room's MAP_B part to the character (y dropped) over argv[3],
// turned so that MAP_B -> MAP_D points along +z, around this bank's part named after the room.
int tiXcmd_SetMapData(int argc, char **argv, int bank)
{
    float rot[4][4];
    float rotY[4][4];
    float offset[4], toChara[4], toD[4], axis[4];
    const float zAxis[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
    char room[XCMD_ROOM_NAME_LEN + 1];
    CHARACTER *ch = &Character[atoi(argv[0])];
    MODEL_PARTS *pieceA = TiPart(bank, atoi(argv[1]));
    MODEL_PARTS *pieceB = TiPart(bank, atoi(argv[2]));
    u_char *roomHeader;
    MODEL_PARTS *mapB, *mapD, *center;
    float scale;

    (void)argc;
    pieceA->flags43 = PARTS_FLAG43_WARPED;
    pieceB->flags43 = PARTS_FLAG43_WARPED;
    roomHeader = pModelPartsHeader[ch->lightBank];
    mapB = (MODEL_PARTS *)(roomHeader + asFindModelPartsID(roomHeader, ahGetHash("MAP_B")) * PARTS_REC_SIZE);
    mapD = (MODEL_PARTS *)(roomHeader + asFindModelPartsID(roomHeader, ahGetHash("MAP_D")) * PARTS_REC_SIZE);
    memset(room, 0, sizeof(room));
    strncpy(room, BankInfo[ch->lightBank].name, XCMD_ROOM_NAME_LEN);
    center = TiPart(bank, asFindModelPartsID(pModelPartsHeader[bank], ahGetHash(room)));

    offset[0] = ((const float *)ch->pos)[0] - mapB->pos[0];
    offset[1] = 0.0f;
    offset[2] = ((const float *)ch->pos)[2] - mapB->pos[2];
    offset[3] = 0.0f;
    memcpy(toChara, offset, sizeof(toChara));
    TiNormalizeXYZ(toChara);            /* the ROM stores it and never reads it */
    toD[0] = mapD->pos[0] - mapB->pos[0];
    toD[1] = 0.0f;
    toD[2] = mapD->pos[2] - mapB->pos[2];
    toD[3] = 0.0f;
    TiNormalizeXYZ(toD);
    // VOPMULA / VOPMSUB: axis = zAxis x toD.
    axis[0] = zAxis[1] * toD[2] - zAxis[2] * toD[1];
    axis[1] = zAxis[2] * toD[0] - zAxis[0] * toD[2];
    axis[2] = zAxis[0] * toD[1] - zAxis[1] * toD[0];
    axis[3] = 0.0f;
    asMakeRotMatrixByQuartanion(rot, axis, ahAcos(gaInnerProductXYZ((float *)zAxis, toD)));

    scale = 1.0f / atoff(argv[3]);
    offset[0] = center->pos[0] + offset[0] * scale;
    offset[1] = center->pos[1] + offset[1] * scale;
    offset[2] = center->pos[2] + offset[2] * scale;

    memcpy(pieceA->placeRot, pieceA->initRot, sizeof(pieceA->initRot));
    memcpy(pieceA->drawOnPos, pieceA->transInit, sizeof(pieceA->transInit));
    memcpy(pieceB->placeRot, pieceB->initRot, sizeof(pieceB->initRot));
    memcpy(pieceB->drawOnPos, pieceB->transInit, sizeof(pieceB->transInit));
    gaSetRotMatrixY(rotY, ch->rot);
    gaMulMatrix(pieceA->placeRot, rotY, pieceA->placeRot);
    tiGetTransRotMatrix(rot, center->pos, rot);
    gaMulMatrix(pieceA->placeRot, rot, pieceA->placeRot);
    gaCopyVectorXYZ(pieceA->drawOnPos, offset);
    gaCopyVectorXYZ(pieceB->drawOnPos, offset);
    BuildMatrixTree(0, 2, 0);
    return XCMD_DONE;
}

// CT3 0x018e1ed0: character argv[0]'s ghost area: range argv[1] around part argv[2] of this bank.
int tiXcmd_SetPointForGhostArea(int argc, char **argv, int bank)
{
    CHARA_JUNC *junc = Character[atoi(argv[0])].junc;

    (void)argc;
    junc->ghostRange = atoff(argv[1]);
    junc->ghostPoint = BankEeAddr(bank, &TiPart(bank, atoi(argv[2]))->pos[0]);
    return XCMD_DONE;
}

// CT3 0x018e1f80 .. 0x018e20b0: the free slots of the table. Free1 / Free2 were left with a job: a
// character's translucent drawing on (argv[0], alpha argv[1]) / off.
int tiXcmd___________Free6(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

int tiXcmd___________Free5(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

int tiXcmd___________Free4(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

int tiXcmd___________Free3(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018e1fc0
int tiXcmd___________Free2(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);

    (void)argc;
    (void)bank;
    if (chara >= 0 && chara < CHARACTER_MAX)
        Character[chara].translucent = 0;
    return XCMD_DONE;
}

// CT3 0x018e2020
int tiXcmd___________Free1(int argc, char **argv, int bank)
{
    int chara = atoi(argv[0]);

    (void)argc;
    (void)bank;
    if (chara >= 0 && chara < CHARACTER_MAX) {
        Character[chara].translucent = CHARA_TRANS_TRANSLUCENT;
        Character[chara].translucentAlpha = atoff(argv[1]);
    }
    return XCMD_DONE;
}

// CT3 0x018e20b0
int tiXcmd___________Free0(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018e2170: the boss's strike event once the player is no farther from part argv[0] than part
// argv[1] is (the two parts' records kept in pSrcMPstock / pDstMPstock).
int tiXcmd_CheckRange(int argc, char **argv, int bank)
{
    int src = atoi(argv[0]);
    int dst = atoi(argv[1]);
    float range, dist;

    (void)argc;
    pSrcMPstock = TiPartEe(bank, src);
    pDstMPstock = TiPartEe(bank, dst);
    range = TiDistXYZ(TiPart(bank, dst)->pos, TiPart(bank, src)->pos);
    dist = TiDistXYZ((const float *)Character[Player_BankID].pos, TiPart(bank, src)->pos);
    if (dist <= range)
        ssTrigRegisterEvent(TRIG_EVENT_BOSS_STRIKE, 1, 0);
    return XCMD_DONE;
}

// CT3 0x018e2d10: a quake spreading from where character CHARA_QUAKE_SOURCE stood when it started, at
// argv[0] / 5 per tick: the strike event once it reaches the player; done then, or past
// TI_QUAKE_RANGE_MAX; else waits a frame.
int tiXcmd_CheckEarthquake(int argc, char **argv, int bank)
{
    static u_char start;                /* start$891, sbss 0x01944860 */
    static int Ticks;                   /* Ticks$892, sbss 0x01944884 */
    CHARACTER *src = &Character[CHARA_QUAKE_SOURCE];
    float speed = atoff(argv[0]) / TI_QUAKE_SPEED_DIV;
    float dist, radius;

    (void)argc;
    (void)bank;
    if (start == 0) {
        start = 1;
        memcpy(src->strikeFrom, src->pos, sizeof(src->strikeFrom));
        Ticks = 0;
    }
    dist = TiDistXYZ((const float *)Character[Player_BankID].pos, (const float *)src->strikeFrom);
    radius = speed * (float)Ticks;
    if (dist <= radius) {
        ssTrigRegisterEvent(TRIG_EVENT_BOSS_STRIKE, 1, 0);
        start = 0;
        return XCMD_DONE;
    }
    if (radius > TI_QUAKE_RANGE_MAX) {
        start = 0;
        return XCMD_DONE;
    }
    Ticks += gNextTicksInterval;
    return XCMD_WAIT;
}

// CT3 0x018e2e70
int tiXcmd_BOSS_BattleEnd(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    tiFreeBossData();
    return XCMD_DONE;
}

// CT3 0x018e33d0: the boss's six hit-back values (floats argv[0..5]).
int tiXcmd_BOSS_SetHitBacks(int argc, char **argv, int bank)
{
    float v;
    int i;

    (void)argc;
    (void)bank;
    for (i = 0; i < BOSS_INFO_HIT_BACK_COUNT; i++) {
        v = atoff(argv[i]);
        memcpy(pBossInfo + BOSS_INFO_HIT_BACKS + i * (int)sizeof(float), &v, sizeof(v));
    }
    return XCMD_DONE;
}

// CT3 0x018e41f0: the shade-off (main.c counts nowShadeOffFrame down while fShadeOff).
// "ON" frames level slide depth keep [var-global]: starts it; frames is a variable (argv[1], global
// argv[6]) when argc is 7; keep "KEEP" holds the screen, "OFF" lets it go.
// "OFF" frames [global]: ends it over frames (a variable, global argv[2], when argc is 3).
int tiXcmd_ShadeOff(int argc, char **argv, int bank)
{
    int level;

    if (strcmp(argv[0], "ON") == 0) {
        fShadeOff = 1;
        fKeepScreen = 0;
        level = atoi(argv[2]);
        if (argc == SHADE_OFF_ON_ARGC_VAR)
            nowShadeOffFrame = ssTrigGetVariable(atoi(argv[1]) & 0xffff, atoi(argv[6]), bank);
        else
            nowShadeOffFrame = atoi(argv[1]);
        ShadeOff_CapLevel = (char)(SHADE_OFF_LEVELS - level);
        ShadeOffFrame = nowShadeOffFrame;
        ShadeOff_SlidePixel = atoff(argv[3]);
        ShadeOff_Level = (u_char)(atoi(argv[4]) << 2);
        if (strcmp(argv[5], "KEEP") == 0)
            fKeepScreen |= KEEP_SCREEN_HOLD;
        else if (strcmp(argv[5], "OFF") == 0)
            fKeepScreen = 0;
    } else if (strcmp(argv[0], "OFF") == 0) {
        fKeepScreen |= KEEP_SCREEN_RELEASE;
        if (argc == SHADE_OFF_OFF_ARGC_VAR)
            ShadeOffFrame = ssTrigGetVariable(atoi(argv[1]) & 0xffff, atoi(argv[2]), bank);
        else
            ShadeOffFrame = atoi(argv[1]);
        nowShadeOffFrame = ShadeOffFrame;
    }
    return XCMD_DONE;
}

// CT3 0x018e4390: breaks off a cross-fade (a pausing one gives its reserved pause back).
int tiXcmd_XFade_Break(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    if (XFade_Type == XFADE_TYPE_PAUSE || XFade_Type == XFADE_TYPE_PAUSE2)
        pause_reservation = 0;
    XFade_Type = XFADE_TYPE_NONE;
    fXFade = 0;
    EffectBuffer_Use = 0;
    return XCMD_DONE;
}

// CT3 0x018e4990: argv[0] 1: the pendulums of character argv[1]'s bank; 0: this bank's pendulum argv[1]
// (0: all of them).
int tiXcmd_HurikoReset(int argc, char **argv, int bank)
{
    int byChara = atoi(argv[0]);

    (void)argc;
    if (byChara == 1)
        tiHurikoReset_On(atoi(argv[1]), 0, 1);
    else if (byChara == 0)
        tiHurikoReset_On(bank, atoi(argv[1]), 0);
    return XCMD_DONE;
}

// CT3 0x018e4b20 / 0x018e4b30: the light belt commands kept their slots and lost their bodies.
int tiXcmd_LightBeltEnd(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

int tiXcmd_LightBeltStart(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018e4c50: the camera change tiXcmd_ChangeCam_Reserve kept, now; its event removed.
int tiXcmd_ChangeCam_Execute(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    CamChange_S(tiReserveBank, tiReserveCamID, (u_char)tiReserveType, tiReserveTime);
    ssTrigRemoveEvent(TRIG_EVENT_CAM_RESERVED, 1, 0);
    return XCMD_DONE;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018e8640
int tiXcmd_AddVariable(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_AddVariable", 0x018e8640);
    return {};
}

// stub of CT3 0x018e3460
int tiXcmd_BOSS_BattleStart(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_BOSS_BattleStart", 0x018e3460);
    return {};
}

// stub of CT3 0x018e31d0
int tiXcmd_BOSS_SetChain(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_BOSS_SetChain", 0x018e31d0);
    return {};
}

// stub of CT3 0x018e2f70
int tiXcmd_BOSS_SetChain_Alyssa(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_BOSS_SetChain_Alyssa", 0x018e2f70);
    return {};
}

// stub of CT3 0x018e2e90
int tiXcmd_BOSS_SetWarp(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_BOSS_SetWarp", 0x018e2e90);
    return {};
}

// stub of CT3 0x018e1bc0
int tiXcmd_Balows2th(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_Balows2th", 0x018e1bc0);
    return {};
}

// stub of CT3 0x018e4c90
int tiXcmd_ChangeCam_Reserve(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_ChangeCam_Reserve", 0x018e4c90);
    return {};
}

// stub of CT3 0x018e43d0
int tiXcmd_CharaEdgeShadeAnim(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CharaEdgeShadeAnim", 0x018e43d0);
    return {};
}

// stub of CT3 0x018e7e20
int tiXcmd_CheckCurrentScene(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CheckCurrentScene", 0x018e7e20);
    return {};
}

// stub of CT3 0x018e5700
int tiXcmd_CheckLife(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CheckLife", 0x018e5700);
    return {};
}

// stub of CT3 0x018e71f0
int tiXcmd_CheckMCFile(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CheckMCFile", 0x018e71f0);
    return {};
}

// stub of CT3 0x018e4cf0
int tiXcmd_CheckinLetterList(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CheckinLetterList", 0x018e4cf0);
    return {};
}

// stub of CT3 0x018e7eb0
int tiXcmd_CompareVar2(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CompareVar2", 0x018e7eb0);
    return {};
}

// stub of CT3 0x018e8260
int tiXcmd_CompareVariable(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CompareVariable", 0x018e8260);
    return {};
}

// stub of CT3 0x018e83d0
int tiXcmd_CopySunPs2Object(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CopySunPs2Object", 0x018e83d0);
    return {};
}

// stub of CT3 0x018e88a0
int tiXcmd_CopyVariable(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CopyVariable", 0x018e88a0);
    return {};
}

// stub of CT3 0x018e2ae0
int tiXcmd_End_SPAttack_SCISSORS_MAN(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_End_SPAttack_SCISSORS_MAN", 0x018e2ae0);
    return {};
}

// stub of CT3 0x018e47b0
int tiXcmd_FadeStart(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_FadeStart", 0x018e47b0);
    return {};
}

// stub of CT3 0x018e7b70
int tiXcmd_FreeGhostPole(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_FreeGhostPole", 0x018e7b70);
    return {};
}

// stub of CT3 0x018e56a0
int tiXcmd_GetAlyssaLife(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetAlyssaLife", 0x018e56a0);
    return {};
}

// stub of CT3 0x018e20c0
int tiXcmd_GetBloodAttackObject(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetBloodAttackObject", 0x018e20c0);
    return {};
}

// stub of CT3 0x018e6160
int tiXcmd_GetIDfromList(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetIDfromList", 0x018e6160);
    return {};
}

// stub of CT3 0x018e5030
int tiXcmd_GetINDEX_FromLetterList(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetINDEX_FromLetterList", 0x018e5030);
    return {};
}

// stub of CT3 0x018e6bf0
int tiXcmd_GetItem(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetItem", 0x018e6bf0);
    return {};
}

// stub of CT3 0x018e5ce0
int tiXcmd_GetItemData(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetItemData", 0x018e5ce0);
    return {};
}

// stub of CT3 0x018e5930
int tiXcmd_GetItemListLastIndex(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetItemListLastIndex", 0x018e5930);
    return {};
}

// stub of CT3 0x018e54f0
int tiXcmd_GetKeyFromKeyMap(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetKeyFromKeyMap", 0x018e54f0);
    return {};
}

// stub of CT3 0x018e5120
int tiXcmd_GetLetterData(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetLetterData", 0x018e5120);
    return {};
}

// stub of CT3 0x018e7130
int tiXcmd_GetMCflag(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetMCflag", 0x018e7130);
    return {};
}

// stub of CT3 0x018e2b30
int tiXcmd_Init_SPAttack_SCISSORS_MAN(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_Init_SPAttack_SCISSORS_MAN", 0x018e2b30);
    return {};
}

// stub of CT3 0x018e5360
int tiXcmd_KeyNumToFrame(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_KeyNumToFrame", 0x018e5360);
    return {};
}

// stub of CT3 0x018e5870
int tiXcmd_MoveItem(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_MoveItem", 0x018e5870);
    return {};
}

// stub of CT3 0x018e0fa0
int tiXcmd_OperateItem(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_OperateItem", 0x018e0fa0);
    return {};
}

// stub of CT3 0x018e54c0
int tiXcmd_ResetKeyMap(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_ResetKeyMap", 0x018e54c0);
    return {};
}

// stub of CT3 0x018e22f0
int tiXcmd_SPAttack_SCISSORS_MAN(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SPAttack_SCISSORS_MAN", 0x018e22f0);
    return {};
}

// stub of CT3 0x018e86d0
int tiXcmd_SceneAnimationSkipToVariable(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SceneAnimationSkipToVariable", 0x018e86d0);
    return {};
}

// stub of CT3 0x018e4b40
int tiXcmd_SetCharacterEdgeShading(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetCharacterEdgeShading", 0x018e4b40);
    return {};
}

// stub of CT3 0x018e8210
int tiXcmd_SetDrawMode_Global(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetDrawMode_Global", 0x018e8210);
    return {};
}

// stub of CT3 0x018e7bf0
int tiXcmd_SetGhostPole(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetGhostPole", 0x018e7bf0);
    return {};
}

// stub of CT3 0x018e5a50
int tiXcmd_SetItemData(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetItemData", 0x018e5a50);
    return {};
}

// stub of CT3 0x018e5590
int tiXcmd_SetKeyToKeyMap(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetKeyToKeyMap", 0x018e5590);
    return {};
}

// stub of CT3 0x018e7580
int tiXcmd_SetParaDefault(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetParaDefault", 0x018e7580);
    return {};
}

// stub of CT3 0x018e7d10
int tiXcmd_SetPlayerChara(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetPlayerChara", 0x018e7d10);
    return {};
}

// stub of CT3 0x018e8890
int tiXcmd_SetReset(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetReset", 0x018e8890);
    return {};
}

// stub of CT3 0x018e89d0
int tiXcmd_SetRotation(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetRotation", 0x018e89d0);
    return {};
}

// stub of CT3 0x018e8930
int tiXcmd_SetScale(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetScale", 0x018e8930);
    return {};
}

// stub of CT3 0x018e57b0
int tiXcmd_SetStatusBar(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetStatusBar", 0x018e57b0);
    return {};
}

// stub of CT3 0x018e6f50
int tiXcmd_SetmcAccess_ID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetmcAccess_ID", 0x018e6f50);
    return {};
}

// stub of CT3 0x018e6fb0
int tiXcmd_SetmcErr_ID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetmcErr_ID", 0x018e6fb0);
    return {};
}

// stub of CT3 0x018e7010
int tiXcmd_SetmcFile_ID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetmcFile_ID", 0x018e7010);
    return {};
}

// stub of CT3 0x018e70d0
int tiXcmd_SetmcMain_ID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetmcMain_ID", 0x018e70d0);
    return {};
}

// stub of CT3 0x018e7070
int tiXcmd_SetmcSlot_ID(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetmcSlot_ID", 0x018e7070);
    return {};
}

// stub of CT3 0x018e7590
int tiXcmd_StereoCheckFlg(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_StereoCheckFlg", 0x018e7590);
    return {};
}

// stub of CT3 0x018e75a0
int tiXcmd_StereoON_OFF(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_StereoON_OFF", 0x018e75a0);
    return {};
}

// stub of CT3 0x018e85b0
int tiXcmd_SubVariable(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SubVariable", 0x018e85b0);
    return {};
}

// stub of CT3 0x018e8b70
int tiXcmd_TEST00(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_TEST00", 0x018e8b70);
    return {};
}

// stub of CT3 0x018e4a20
int tiXcmd_TranslucenceAlphaAnim(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_TranslucenceAlphaAnim", 0x018e4a20);
    return {};
}

// stub of CT3 0x018e6330
int tiXcmd_UseItem(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_UseItem", 0x018e6330);
    return {};
}

// stub of CT3 0x018e77b0
int tiXcmd_VibrationCheckFlg(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_VibrationCheckFlg", 0x018e77b0);
    return {};
}

// stub of CT3 0x018e75b0
int tiXcmd_VibrationEnd(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_VibrationEnd", 0x018e75b0);
    return {};
}

// stub of CT3 0x018e7810
int tiXcmd_VibrationON_OFF(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_VibrationON_OFF", 0x018e7810);
    return {};
}

// stub of CT3 0x018e75e0
int tiXcmd_VibrationStart(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_VibrationStart", 0x018e75e0);
    return {};
}

// stub of CT3 0x018e7870
int tiXcmd_WaitUntilBrainFree(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_WaitUntilBrainFree", 0x018e7870);
    return {};
}

// stub of CT3 0x018e4c00
int tiXcmd_XFadeOut2Start(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_XFadeOut2Start", 0x018e4c00);
    return {};
}

// stub of CT3 0x018e8bc0
int tkXcmd_CaptionFlag(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tkXcmd_CaptionFlag", 0x018e8bc0);
    return {};
}

// stub of CT3 0x018e8d20
int tkXcmd_CharSceneChange(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tkXcmd_CharSceneChange", 0x018e8d20);
    return {};
}

// stub of CT3 0x018e8f00
int wideXcmd_Delay(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_Delay", 0x018e8f00);
    return {};
}

// stub of CT3 0x018e78c0
int wideXcmd_GetCharaData(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_GetCharaData", 0x018e78c0);
    return {};
}

// stub of CT3 0x018e7d60
int wideXcmd_GetNumberOfBeam(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_GetNumberOfBeam", 0x018e7d60);
    return {};
}

// stub of CT3 0x018e79d0
int wideXcmd_GetRate(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("wideXcmd_GetRate", 0x018e79d0);
    return {};
}
