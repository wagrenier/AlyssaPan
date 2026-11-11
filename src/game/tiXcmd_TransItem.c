// game/tiXcmd_TransItem.c -- Clock Tower 3: the trigger script's x commands by ti (tiXcmd_*), with a few
// by tk / wide (0x018e0e80..0x018e90a0). Each takes the command's argument strings and the bank whose
// script runs it; ssTrigExecuteActCmd (ssGrpPortalCheck.c) calls them through ssXcmd_functions.
// Slice 1 (2026-11-11): the item examination moves, the scene / map commands, the free slots, the range
// and earthquake checks, the shade-off, the cross-fade break, the pendulum reset, the reserved camera
// change. Slice 2 (2026-11-11): OperateItem, the fades, the translucence / edge-shading animations, the
// key map, the letter list. Left: Balows2th, and the boss commands whose pointer words wait for the boss
// TU's host layout (SetWarp, the SCISSORS_MAN attack, SetChain*, GetBloodAttackObject), and the rest.
// Slice 3 (2026-11-11): the key-config commands, life and status bar, item slots, memory-card ids, stereo.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "common/alyssapan_stub.h"
#include "sdk/libpad.h"
#include "game/tiXcmd_TransItem.h"
#include "game/xcmd_tn.h"
#include "game/Clipping_Portal.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/LinkMatrixTree.h"
#include "game/main.h"
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
// CT3 0x018e1f80
int tiXcmd___________Free6(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018e1f90
int tiXcmd___________Free5(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018e1fa0
int tiXcmd___________Free4(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018e1fb0
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

// CT3 0x018e4b30
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

// Slice 2 (2026-11-11): the pad-driven item examination, the fades, the translucence and edge-shading
// animations, the key map, the letter list.

// deg (0..360) as the game's 16-bit angle (65536 a turn), as the ROM rounds it: (int)(deg * 65536 / 360).
static int TiDegToAngle(float deg)
{
    return (int)((deg * 65536.0f) / 360.0f) & 0xffff;
}

// m = the unit matrix (the ROM builds it from vf0 rotated through VMR32 before each gaSetRotMatrix*).
static void TiUnitMatrix(float (*m)[4])
{
    memset(m, 0, sizeof(float[4][4]));
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
}

// The examined item one step along DirectToCamera (step > 0: toward the eye), the sum kept in
// 0..TI_ITEM_TRANS_MAX; at a bound the sum is clamped, the part stays and the command waits.
static int TiItemStep(MODEL_PARTS *part, float step)
{
    trans_add_value += step;
    if (step > 0.0f && trans_add_value > TI_ITEM_TRANS_MAX) {
        trans_add_value = TI_ITEM_TRANS_MAX;
        return XCMD_WAIT;
    }
    if (step < 0.0f && trans_add_value < 0.0f) {
        trans_add_value = 0.0f;
        return XCMD_WAIT;
    }
    TiItemMove(part, step);
    return XCMD_DONE;
}

// CT3 0x018e0fa0: the player handles the examined item: part argv[0] moves toward / away from the eye,
// part argv[1] turns. Runs every frame until cross (pad_trg_button) ends it. Left stick: turns by the
// stick (x about its cosine, y about its sine, TI_ITEM_STICK_TURN degrees at full tilt); without it the
// d-pad turns by TI_ITEM_PAD_TURN. Right stick up (or right, within 90 degrees of up) / triangle: toward
// the eye; down / square: away, TI_ITEM_STEP a frame.
int tiXcmd_OperateItem(int argc, char **argv, int bank)
{
    MODEL_PARTS *item;
    MODEL_PARTS *turn;
    float (*m)[4];
    float rot[4][4];
    float deg;
    float mag;
    int turnIdx;

    (void)argc;
    if (pad_trg_button[0] & TI_ITEM_EXIT_BUTTON)
        return XCMD_DONE;
    item = TiPart(bank, atoi(argv[0]));
    turnIdx = atoi(argv[1]);
    if (item->kind == PARTS_KIND_ITEM_LOCKED)
        return XCMD_DONE;
    item->flags43 |= PARTS_FLAG43_WARPED;
    turn = TiPart(bank, turnIdx);
    if (turn->kind == PARTS_KIND_ITEM_LOCKED)
        return XCMD_DONE;
    turn->flags43 |= PARTS_FLAG43_WARPED;
    m = turn->placeRot;     // placeRot rows 0..2 and drawOnPos: the part's 4x4 placement

    asGetAnalogStickInformation(TI_ITEM_STICK_TURN_ID, TI_ITEM_STICK_DEAD, &deg, &mag);
    if (mag > 0.0f) {
        TiUnitMatrix(rot);
        gaSetRotMatrixX(rot, TiDegToAngle(mag * ahCos(TiDegToAngle(deg)) * TI_ITEM_STICK_TURN));
        gaMulMatrix(m, rot, m);
        TiUnitMatrix(rot);
        gaSetRotMatrixY(rot, TiDegToAngle(mag * ahSin(TiDegToAngle(deg)) * TI_ITEM_STICK_TURN));
        gaMulMatrix(m, rot, m);
    } else {
        TiUnitMatrix(rot);
        if (pad_cnt_button[0] & SCE_PADLright) {
            gaSetRotMatrixY(rot, TI_ITEM_PAD_TURN);
            gaMulMatrix(m, rot, m);
        } else if (pad_cnt_button[0] & SCE_PADLleft) {
            gaSetRotMatrixY(rot, -TI_ITEM_PAD_TURN & 0xffff);
            gaMulMatrix(m, rot, m);
        } else if (pad_cnt_button[0] & SCE_PADLup) {
            gaSetRotMatrixX(rot, TI_ITEM_PAD_TURN);
            gaMulMatrix(m, rot, m);
        } else if (pad_cnt_button[0] & SCE_PADLdown) {
            gaSetRotMatrixX(rot, -TI_ITEM_PAD_TURN & 0xffff);
            gaMulMatrix(m, rot, m);
        }
    }

    asGetAnalogStickInformation(TI_ITEM_STICK_MOVE_ID, TI_ITEM_STICK_DEAD, &deg, &mag);
    if (mag > 0.0f) {
        if (deg <= TI_ITEM_STICK_TOWARD_LO || deg >= TI_ITEM_STICK_TOWARD_HI)
            TiItemStep(item, TI_ITEM_STEP);
        else
            TiItemStep(item, -TI_ITEM_STEP);
        return XCMD_WAIT;
    }
    // Both buttons held: away, then toward (the ROM's order); a bound ends the frame early.
    if ((pad_cnt_button[0] & TI_ITEM_AWAY_BUTTON) && TiItemStep(item, -TI_ITEM_STEP) == XCMD_WAIT)
        return XCMD_WAIT;
    if (pad_cnt_button[0] & TI_ITEM_TOWARD_BUTTON)
        TiItemStep(item, TI_ITEM_STEP);
    return XCMD_WAIT;
}

// CT3 0x018e47b0: starts a fade (tiFade_Start). argv[1] "IN" / "OUT" / "OUT2" / "IN2", argv[2] "BLACK"
// or "WHITE" (anything else: black; an unknown direction: FADE_TYPE_IN_BLACK, no event). The OUTs
// register TRIG_EVENT_FADED_OUT, the INs remove it. Time: argv[0] seconds (3 arguments), or the variable
// argv[0] (global argv[3]).
int tiXcmd_FadeStart(int argc, char **argv, int bank)
{
    int white;
    int type = FADE_TYPE_IN_BLACK;

    white = strcmp(argv[2], "BLACK") != 0 && strcmp(argv[2], "WHITE") == 0;
    if (strcmp(argv[1], "IN") == 0) {
        type = white ? FADE_TYPE_IN_WHITE : FADE_TYPE_IN_BLACK;
        ssTrigRemoveEvent(TRIG_EVENT_FADED_OUT, 1, 0);
    } else if (strcmp(argv[1], "OUT") == 0) {
        type = white ? FADE_TYPE_OUT_WHITE : FADE_TYPE_OUT_BLACK;
        ssTrigRegisterEvent(TRIG_EVENT_FADED_OUT, 1, 0);
    } else if (strcmp(argv[1], "OUT2") == 0) {
        type = white ? FADE_TYPE_OUT2_WHITE : FADE_TYPE_OUT2_BLACK;
        ssTrigRegisterEvent(TRIG_EVENT_FADED_OUT, 1, 0);
    } else if (strcmp(argv[1], "IN2") == 0) {
        type = white ? FADE_TYPE_IN2_WHITE : FADE_TYPE_IN2_BLACK;
        ssTrigRemoveEvent(TRIG_EVENT_FADED_OUT, 1, 0);
    }
    if (argc == FADE_START_ARGC_TIME)
        tiFade_Start(atoff(argv[0]), type);
    else
        tiFade_Start((float)ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[3]), bank), type);
    return XCMD_DONE;
}

// One step of an animated value toward target: "+" adds step until it reaches target, "-" takes it off;
// at target it is clamped and the step is done (1); short of it 0. Any other op: done, untouched.
static int TiAnimFloat(float *v, const char *op, float step, float target)
{
    if (strcmp(op, "+") == 0) {
        *v += step;
        if (*v < target)
            return 0;
        *v = target;
    } else if (strcmp(op, "-") == 0) {
        *v -= step;
        if (target < *v)
            return 0;
        *v = target;
    }
    return 1;
}

// CT3 0x018e4a20: character argv[0]'s translucence alpha, argv[1] "+" / "-" argv[2] a frame toward
// argv[3]; the command waits until it gets there.
// callcheck: strcmp (in TiAnimFloat, which the ROM has inline)
int tiXcmd_TranslucenceAlphaAnim(int argc, char **argv, int bank)
{
    CHARACTER *chr = &Character[atoi(argv[0])];
    float target = atoff(argv[3]);

    (void)argc;
    (void)bank;
    return TiAnimFloat(&chr->translucentAlpha, argv[1], atoff(argv[2]), target) ? XCMD_DONE : XCMD_WAIT;
}

// CT3 0x018e43d0: animates character argv[0]'s edge shading, four groups of (switch, op, step, target):
// argv[1..4] the translucence alpha (float), argv[5..8] edgeLevel (u_char), argv[9..12] / argv[13..16]
// the two edge parameters (short h536[CHARA_EDGE_PARAM_A / _B]). A group runs when its switch is "ON";
// the command waits until every running group is at its target. As the ROM: the last group's "-" waits
// while the value is below the target (the others: above), and the byte compares unsigned.
int tiXcmd_CharaEdgeShadeAnim(int argc, char **argv, int bank)
{
    CHARACTER *chr = &Character[atoi(argv[0])];
    float alphaTarget = atoff(argv[4]);
    u_char levelTarget = (u_char)atoi(argv[8]);
    short aTarget = (short)atoi(argv[12]);
    short bTarget = (short)atoi(argv[16]);
    short *a = &chr->h536[CHARA_EDGE_PARAM_A];
    short *b = &chr->h536[CHARA_EDGE_PARAM_B];
    int done = XCMD_DONE;

    (void)argc;
    (void)bank;
    if (strcmp(argv[1], "ON") == 0 &&
        !TiAnimFloat(&chr->translucentAlpha, argv[2], atoff(argv[3]), alphaTarget))
        done = XCMD_WAIT;
    if (strcmp(argv[5], "ON") == 0) {
        if (strcmp(argv[6], "+") == 0) {
            chr->edgeLevel += (u_char)atoi(argv[7]);
            if (chr->edgeLevel < levelTarget)
                done = XCMD_WAIT;
            else
                chr->edgeLevel = levelTarget;
        } else if (strcmp(argv[6], "-") == 0) {
            chr->edgeLevel -= (u_char)atoi(argv[7]);
            if (levelTarget < chr->edgeLevel)
                done = XCMD_WAIT;
            else
                chr->edgeLevel = levelTarget;
        }
    }
    if (strcmp(argv[9], "ON") == 0) {
        if (strcmp(argv[10], "+") == 0) {
            *a += (short)atoi(argv[11]);
            if (*a < aTarget)
                done = XCMD_WAIT;
            else
                *a = aTarget;
        } else if (strcmp(argv[10], "-") == 0) {
            *a -= (short)atoi(argv[11]);
            if (aTarget < *a)
                done = XCMD_WAIT;
            else
                *a = aTarget;
        }
    }
    if (strcmp(argv[13], "ON") == 0) {
        if (strcmp(argv[14], "+") == 0) {
            *b += (short)atoi(argv[15]);
            if (*b < bTarget)
                done = XCMD_WAIT;
            else
                *b = bTarget;
        } else if (strcmp(argv[14], "-") == 0) {
            *b -= (short)atoi(argv[15]);
            if (*b < bTarget)     // sic: the ROM's test, not the mirror of the others
                done = XCMD_WAIT;
            else
                *b = bTarget;
        }
    }
    return done;
}

// CT3 0x018e4b40: argv[2] "ON": character argv[0] drawn edge-shaded (the ghost pass) at level argv[1],
// edgeLevel argv[3]; anything else: off.
int tiXcmd_SetCharacterEdgeShading(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    if (strcmp(argv[2], "ON") == 0) {
        tiSet_CharaTranslucenceON(atoi(argv[0]), atoi(argv[1]), CHARA_TRANS_GHOST);
        Character[atoi(argv[0])].edgeLevel = (u_char)atoi(argv[3]);
    } else {
        tiSet_CharaTranslucenceOFF(atoi(argv[0]), CHARA_TRANS_GHOST);
    }
    return XCMD_DONE;
}

// CT3 0x018e4c00: a cross-fade out over argv[0] seconds, kind argv[1].
int tiXcmd_XFadeOut2Start(int argc, char **argv, int bank)
{
    (void)argc;
    (void)bank;
    tiXFadeOut_Start2(atoff(argv[0]), (u_char)atoi(argv[1]));
    return XCMD_DONE;
}

// CT3 0x018e4c90: reserves a camera change (camera argv[0], type argv[1], time argv[2], this bank) for
// tiXcmd_ChangeCam_Execute; TRIG_EVENT_CAM_RESERVED says one waits.
int tiXcmd_ChangeCam_Reserve(int argc, char **argv, int bank)
{
    (void)argc;
    tiReserveBank = bank;
    tiReserveCamID = (short)atoi(argv[0]);
    tiReserveType = (short)atoi(argv[1]);
    tiReserveTime = atoi(argv[2]);
    ssTrigRegisterEvent(TRIG_EVENT_CAM_RESERVED, 1, 0);
    return XCMD_DONE;
}

// The letters list of category c holds a letter in some slot.
static int TiLetterCategoryUsed(int c)
{
    int i;

    for (i = 0; i < LETTER_LIST_SLOTS; i++)
        if (LetterList[c][i] != 0)
            break;
    return i != LETTER_LIST_SLOTS;
}

// The index of category c's last letter (the slot before the first empty one; LETTER_LIST_SLOTS when
// all are full).
static int TiLetterLastIndex(int c)
{
    int i;

    for (i = 0; i < LETTER_LIST_SLOTS; i++)
        if (LetterList[c][i] == 0)
            return i - 1;
    return i;
}

// CT3 0x018e4cf0: the letters menu turns to the next category that holds a letter: the variable
// argv[2] (global argv[3]) is the category, argv[6] 0: backward, else forward, wrapping over the
// LETTER_CATEGORIES. When one is found the variable takes it and the variable argv[4] (global argv[5])
// -- the letter index -- is held to its last letter; event argv[0] (global argv[1]) is registered when
// another category was found, removed when none was.
int tiXcmd_CheckinLetterList(int argc, char **argv, int bank)
{
    int catVar = atoi(argv[2]) & 0xffff;
    int catGlobal = atoi(argv[3]);
    int cur = ssTrigGetVariable(catVar, catGlobal, bank);
    int c = cur - 1;
    int idxVar;
    int idxGlobal;
    int last;

    (void)argc;
    if (atoi(argv[6]) == 0) {
        if (c < 0)
            c = LETTER_CATEGORIES - 1;
        for (; c != cur; ) {
            if (TiLetterCategoryUsed(c))
                break;
            if (--c < 0)
                c = LETTER_CATEGORIES - 1;
        }
    } else {
        for (c = (cur + 1) % LETTER_CATEGORIES; c != cur; ) {
            if (TiLetterCategoryUsed(c))
                break;
            if (++c == LETTER_CATEGORIES)
                c = 0;
        }
    }
    if (c != cur) {
        ssTrigSetVariable(catVar, c, catGlobal, bank);
        idxVar = atoi(argv[4]) & 0xffff;
        idxGlobal = atoi(argv[5]);
        last = TiLetterLastIndex(c);
        if (last < ssTrigGetVariable(idxVar, idxGlobal, bank))
            ssTrigSetVariable(idxVar, last, idxGlobal, bank);
        ssTrigRegisterEvent(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    } else {
        ssTrigRemoveEvent(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    }
    return XCMD_DONE;
}

// CT3 0x018e5030: the variable argv[0] (global argv[1]) = the slot of category (variable argv[4],
// global argv[5]) that holds letter (variable argv[2], global argv[3]); LETTER_LIST_SLOTS when none.
int tiXcmd_GetINDEX_FromLetterList(int argc, char **argv, int bank)
{
    int letter = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int c = ssTrigGetVariable(atoi(argv[4]) & 0xffff, atoi(argv[5]), bank);
    int i;

    (void)argc;
    for (i = 0; i < LETTER_LIST_SLOTS; i++)
        if (LetterList[c][i] == letter)
            break;
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, i, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e5120: the variable argv[0] (global argv[1]) = field argv[4] ("CATEGORY", "NUMBER",
// "PAGE_NUMBER", "START_PAGE", "END_PAGE": LetterData bytes 0..4) of letter (variable argv[2], global
// argv[3]); another name leaves it.
int tiXcmd_GetLetterData(int argc, char **argv, int bank)
{
    static const char *const kFields[LETTER_DATA_FIELDS] = {
        "CATEGORY", "NUMBER", "PAGE_NUMBER", "START_PAGE", "END_PAGE",
    };
    int letter = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int f;

    (void)argc;
    for (f = 0; f < LETTER_DATA_FIELDS; f++) {
        if (strcmp(argv[4], kFields[f]) == 0) {
            ssTrigSetVariable(atoi(argv[0]) & 0xffff, LetterData[letter][f], atoi(argv[1]), bank);
            break;
        }
    }
    return XCMD_DONE;
}

// CT3 0x018e54c0: the player's key assignment back to the defaults (KEY_MAP[35..69]).
int tiXcmd_ResetKeyMap(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    memcpy(&KEY_MAP[0], &KEY_MAP[KEY_MAP_DEFAULTS], KEY_MAP_ACTIONS * sizeof(KEY_MAP[0]));
    return XCMD_DONE;
}

// CT3 0x018e54f0: the variable argv[2] (global argv[3]) = KEY_MAP[the variable argv[0] (global
// argv[1])]: an action's button bits.
int tiXcmd_GetKeyFromKeyMap(int argc, char **argv, int bank)
{
    int dst = atoi(argv[2]) & 0xffff;
    int action = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);

    (void)argc;
    ssTrigSetVariable(dst, KEY_MAP[action], atoi(argv[3]), bank);
    return XCMD_DONE;
}

// CT3 0x018e7580: kept its slot, lost its body.
int tiXcmd_SetParaDefault(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018e5360: the variable argv[0] (global argv[1]) = the button picture's frame for the button
// bits in the variable argv[2] (global argv[3]) (the key-config screen): KEY_FRAME_*, 0 for anything else.
int tiXcmd_KeyNumToFrame(int argc, char **argv, int bank)
{
    static const struct {
        int bits;
        int frame;
    } kFrames[] = {
        { SCE_PADh, KEY_FRAME_START },  { SCE_PADk, KEY_FRAME_SELECT }, { SCE_PADR2, KEY_FRAME_R2 },
        { SCE_PADR1, KEY_FRAME_R1 },    { SCE_PADL2, KEY_FRAME_L2 },    { SCE_PADL1, KEY_FRAME_L1 },
        { SCE_PADRup, KEY_FRAME_TRIANGLE }, { SCE_PADRleft, KEY_FRAME_SQUARE },
        { SCE_PADRdown, KEY_FRAME_CROSS }, { SCE_PADRright, KEY_FRAME_CIRCLE },
    };
    int bits = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int frame = KEY_FRAME_CIRCLE;
    size_t i;

    (void)argc;
    for (i = 0; i < sizeof(kFrames) / sizeof(kFrames[0]); i++) {
        if (bits == kFrames[i].bits) {
            frame = kFrames[i].frame;
            break;
        }
    }
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, frame, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e5590: assigns the button bits in the variable argv[2] (global argv[3]) to the action in the
// variable argv[0] (global argv[1]). An action among KEY_MAP_CONFIG_FIRST..LAST (not the two linked
// slots) that already has those bits takes the action's old bits: the two swap.
int tiXcmd_SetKeyToKeyMap(int argc, char **argv, int bank)
{
    int action = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    u_short bits = (u_short)ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int i;

    (void)argc;
    for (i = KEY_MAP_CONFIG_FIRST; i <= KEY_MAP_CONFIG_LAST; i++) {
        if ((u_int)(i - KEY_MAP_LINKED_FIRST) > 1 && KEY_MAP[i] == bits) {
            KEY_MAP[i] = KEY_MAP[action];
            KEY_MAP[action] = bits;
            return XCMD_DONE;
        }
    }
    KEY_MAP[action] = bits;
    return XCMD_DONE;
}

// CT3 0x018e56a0: the variable argv[0] (global argv[1]) = Alyssa's life (Character[0], not the player's
// slot).
int tiXcmd_GetAlyssaLife(int argc, char **argv, int bank)
{
    (void)argc;
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, Character[0].hp, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e5700: the event argv[0] (global argv[1]) is registered while the player has no life left,
// removed while it has some.
int tiXcmd_CheckLife(int argc, char **argv, int bank)
{
    (void)argc;
    if (Character[Player_BankID].hp < 1)
        ssTrigRegisterEvent(atoi(argv[0]), atoi(argv[1]), bank);
    else
        ssTrigRemoveEvent(atoi(argv[0]), atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e57b0: the status bar (part argv[0]'s animation in this bank) stops at Alyssa's life in
// percent: frame percent * XCMD_FRAME_TIME, played from the current frame (-1).
int tiXcmd_SetStatusBar(int argc, char **argv, int bank)
{
    int percent = (int)((float)Character[0].hp * 100.0f / (float)Character[0].hpMax);

    (void)argc;
    asSceneAnimationStart(1.0f, 0, All_table_TBL[bank], STATUS_BAR_FROM_NOW, percent * XCMD_FRAME_TIME,
                          atoi(argv[0]) & 0xffff, 1, 0, 0, 0, 1, 0, 1, 0);
    return XCMD_DONE;
}

// CT3 0x018e5870: swaps the item slots the variables argv[0] / argv[2] (globals argv[1] / argv[3]) hold
// (1-based).
int tiXcmd_MoveItem(int argc, char **argv, int bank)
{
    int a = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    int b = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int t;

    (void)argc;
    t = ItemList[a - 1];
    ItemList[a - 1] = ItemList[b - 1];
    ItemList[b - 1] = t;
    return XCMD_DONE;
}

// CT3 0x018e5930: the variable argv[0] (global argv[1]) = the last held slot of ItemList (left as it was
// when none is held). argv[2] 1 or 0 picks the list: the ROM has two copies of the same loop, both over
// ItemList; any other value does nothing.
int tiXcmd_GetItemListLastIndex(int argc, char **argv, int bank)
{
    int list = atoi(argv[2]);
    int i;

    (void)argc;
    if (list != ITEM_LIST_LAST_A && list != ITEM_LIST_LAST_B)
        return XCMD_DONE;
    for (i = 0; i < ITEM_LIST_SLOTS; i++) {
        if (ItemList[i] > 0)
            ssTrigSetVariable(atoi(argv[0]) & 0xffff, i, atoi(argv[1]), bank);
    }
    return XCMD_DONE;
}

// The 0x18-byte item row's fields (ITEM_FIELD_*): two bytes, then ints at +4..+0x10. The row is a
// u_char array here (as the save image copies it), so the ints go through memcpy.
static u_char *ItemTableRow(int table, int row)
{
    if (table == ITEM_TABLE_KEY_ITEMS)
        return KeyItemData[row];
    if (table == ITEM_TABLE_ITEMS)
        return ItemData[row];
    return NULL;
}

static int ItemFieldOffset(int field)
{
    return 4 * (field - ITEM_FIELD_04 + 1);     /* ITEM_FIELD_04.. -> +4, +8, +0xc, +0x10 */
}

// CT3 0x018e5a50: row = the variable argv[2] (global argv[3]); field argv[4] of table argv[5] (items or
// key items) = the variable argv[0] (global argv[1]). The byte fields keep the low byte; field +4 takes
// the value & 0xff as a whole int. Any other table or field does nothing.
int tiXcmd_SetItemData(int argc, char **argv, int bank)
{
    int row = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int value = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    int table = atoi(argv[5]);
    int field = atoi(argv[4]);
    u_char *rec = ItemTableRow(table, row);
    int v;

    (void)argc;
    if (rec == NULL)
        return XCMD_DONE;
    switch (field) {
    case ITEM_FIELD_KIND:
    case ITEM_FIELD_HELD:
        rec[field] = (u_char)value;
        break;
    case ITEM_FIELD_04:
        v = value & 0xff;
        memcpy(rec + ItemFieldOffset(field), &v, sizeof(v));
        break;
    case ITEM_FIELD_08:
    case ITEM_FIELD_0C:
    case ITEM_FIELD_10:
        memcpy(rec + ItemFieldOffset(field), &value, sizeof(value));
        break;
    default:
        break;
    }
    return XCMD_DONE;
}

// CT3 0x018e5ce0: the variable argv[0] (global argv[1]) = field argv[4] of row (the variable argv[2],
// global argv[3]) of table argv[5]. The arrows' table answers only ITEM_FIELD_HELD (ArrowData[row]);
// the byte fields read unsigned. Any other table or field leaves the variable alone.
int tiXcmd_GetItemData(int argc, char **argv, int bank)
{
    int row = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int table = atoi(argv[5]);
    int field = atoi(argv[4]);
    u_char *rec;
    int value;

    (void)argc;
    if (table == ITEM_TABLE_ARROWS) {
        if (field == ITEM_FIELD_HELD)
            ssTrigSetVariable(atoi(argv[0]) & 0xffff, ArrowData[row], atoi(argv[1]), bank);
        return XCMD_DONE;
    }
    rec = ItemTableRow(table, row);
    if (rec == NULL)
        return XCMD_DONE;
    switch (field) {
    case ITEM_FIELD_KIND:
    case ITEM_FIELD_HELD:
        value = rec[field];
        break;
    case ITEM_FIELD_04:
    case ITEM_FIELD_08:
    case ITEM_FIELD_0C:
    case ITEM_FIELD_10:
        memcpy(&value, rec + ItemFieldOffset(field), sizeof(value));
        break;
    default:
        return XCMD_DONE;
    }
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, value, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e7e20: event argv[0] (global argv[1]) is set when the bank running the script is the
// current scene, cleared otherwise.
int tiXcmd_CheckCurrentScene(int argc, char **argv, int bank)
{
    (void)argc;
    if (bank == idCurrentScene)
        ssTrigRegisterEvent(atoi(argv[0]), atoi(argv[1]), bank);
    else
        ssTrigRemoveEvent(atoi(argv[0]), atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e7eb0: compares the variables argv[2] (global argv[3]) and argv[4] (global argv[5]) with
// the operator argv[6] ("==", "!=", ">", ">=", "<", "<="): event argv[0] (global argv[1]) is set when
// it holds, cleared when not. Another operator does nothing.
int tiXcmd_CompareVar2(int argc, char **argv, int bank)
{
    int a = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int b = ssTrigGetVariable(atoi(argv[4]) & 0xffff, atoi(argv[5]), bank);
    const char *op = argv[6];
    int holds;

    (void)argc;
    if (strcmp(op, "==") == 0)
        holds = a == b;
    else if (strcmp(op, "!=") == 0)
        holds = a != b;
    else if (strcmp(op, ">") == 0)
        holds = b < a;
    else if (strcmp(op, ">=") == 0)
        holds = !(a < b);
    else if (strcmp(op, "<") == 0)
        holds = a < b;
    else if (strcmp(op, "<=") == 0)
        holds = !(b < a);
    else
        return XCMD_DONE;
    if (holds)
        ssTrigRegisterEvent(atoi(argv[0]), atoi(argv[1]), bank);
    else
        ssTrigRemoveEvent(atoi(argv[0]), atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e8260: the variable argv[0] (global argv[1]) = COMPARE_* of a against b: a is the variable
// argv[2] (global argv[3]) or, with the global XCMD_SCOPE_LITERAL, the number argv[2]; b likewise from
// argv[4] / argv[5].
int tiXcmd_CompareVariable(int argc, char **argv, int bank)
{
    int a, b, answer;

    (void)argc;
    if (atoi(argv[3]) == XCMD_SCOPE_LITERAL)
        a = atoi(argv[2]);
    else
        a = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    if (atoi(argv[5]) == XCMD_SCOPE_LITERAL)
        b = atoi(argv[4]);
    else
        b = ssTrigGetVariable(atoi(argv[4]) & 0xffff, atoi(argv[5]), bank);
    if (a == b)
        answer = COMPARE_EQUAL;
    else if (b < a)
        answer = COMPARE_GREATER;
    else
        answer = COMPARE_LESS;
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, answer, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e85b0: the variable argv[0] (global argv[1]) -= the number argv[2].
int tiXcmd_SubVariable(int argc, char **argv, int bank)
{
    int var = atoi(argv[0]) & 0xffff;
    int global = atoi(argv[1]);
    int value = ssTrigGetVariable(var, global, bank);

    (void)argc;
    ssTrigSetVariable(var, value - atoi(argv[2]), global, bank);
    return XCMD_DONE;
}

// CT3 0x018e8890: asks for the soft reset (fReset), as the ROM returning 1.
int tiXcmd_SetReset(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    fReset = 1;
    return XCMD_DONE;
}

// CT3 0x018e88a0: the variable argv[0] (global argv[1]) = the variable argv[2] (global argv[3]). As
// the ROM, the source's number is not masked to 16 bits.
int tiXcmd_CopyVariable(int argc, char **argv, int bank)
{
    int var = atoi(argv[0]) & 0xffff;
    int value = ssTrigGetVariable(atoi(argv[2]), atoi(argv[3]), bank);

    (void)argc;
    ssTrigSetVariable(var, value, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e6f50 / 0x018e6fb0 / 0x018e7010 / 0x018e7070 / 0x018e70d0: the memory-card screens' ids
// (Character_Init.c's MemorycardUpdate reads them) = the variable argv[0] (global argv[1]).
// CT3 0x018e6f50
int tiXcmd_SetmcAccess_ID(int argc, char **argv, int bank)
{
    (void)argc;
    mcAccess_ID = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e6fb0
int tiXcmd_SetmcErr_ID(int argc, char **argv, int bank)
{
    (void)argc;
    mcErr_ID = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e7010
int tiXcmd_SetmcFile_ID(int argc, char **argv, int bank)
{
    (void)argc;
    mcFile_ID = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e7070
int tiXcmd_SetmcSlot_ID(int argc, char **argv, int bank)
{
    (void)argc;
    mcSlot_ID = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e70d0
int tiXcmd_SetmcMain_ID(int argc, char **argv, int bank)
{
    (void)argc;
    mcMain_ID = ssTrigGetVariable(atoi(argv[0]) & 0xffff, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e7590 / 0x018e75a0: the stereo commands kept their slots and lost their bodies.
int tiXcmd_StereoCheckFlg(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// CT3 0x018e75a0
int tiXcmd_StereoON_OFF(int argc, char **argv, int bank)
{
    (void)argc;
    (void)argv;
    (void)bank;
    return XCMD_DONE;
}

// An item row's event word (+0x14: the event GetItem registers when the item is picked up).
static int ItemRowEvent(const u_char *rec)
{
    int ev;

    memcpy(&ev, rec + ITEM_ROW_EVENT, sizeof(ev));
    return ev;
}

// A letter's event word (LetterData +8) and its category (byte 0).
static int LetterEvent(int letter)
{
    int ev;

    memcpy(&ev, LetterData[letter] + LETTER_DATA_EVENT, sizeof(ev));
    return ev;
}

// CT3 0x018e6160: the variable argv[0] (global argv[1]) = slot (the variable argv[2], global argv[3]) of
// list argv[4] (ITEM_TABLE_*: ItemList, KeyItemList, LetterList[category: the variable argv[5] / argv[6]],
// ArrowList). Any other list does nothing.
int tiXcmd_GetIDfromList(int argc, char **argv, int bank)
{
    int slot = ssTrigGetVariable(atoi(argv[2]) & 0xffff, atoi(argv[3]), bank);
    int list = atoi(argv[4]);
    int id;

    (void)argc;
    switch (list) {
    case ITEM_TABLE_ARROWS:
        id = ArrowList[slot];
        break;
    case ITEM_TABLE_LETTERS:
        id = LetterList[ssTrigGetVariable(atoi(argv[5]) & 0xffff, atoi(argv[6]), bank)][slot];
        break;
    case ITEM_TABLE_KEY_ITEMS:
        id = KeyItemList[slot];
        break;
    case ITEM_TABLE_ITEMS:
        id = ItemList[slot];
        break;
    default:
        return XCMD_DONE;
    }
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, id, atoi(argv[1]), bank);
    return XCMD_DONE;
}

// CT3 0x018e6bf0: Alyssa picks up item (the variable argv[0], global argv[1]) of kind argv[2]
// (ITEM_TABLE_*). TRIG_VAR_GOT_ITEM_KIND (global) = the kind; the item goes to the first free slot of its
// list, its pick-up event (the row's ITEM_ROW_EVENT / the letter's) is registered and the next frame
// pauses (pause_reservation). A full list sets the item variable to -1 instead (items and key items).
int tiXcmd_GetItem(int argc, char **argv, int bank)
{
    int var = atoi(argv[0]) & 0xffff;
    int global = atoi(argv[1]);
    int kind = atoi(argv[2]);
    int id = ssTrigGetVariable(var, global, bank);
    int i;

    (void)argc;
    ssTrigSetVariable(TRIG_VAR_GOT_ITEM_KIND, kind, 1, 0);
    switch (kind) {
    case ITEM_TABLE_ARROWS:
        if (id != ARROW_UNLISTED) {
            for (i = 0; i < ARROW_LIST_SLOTS; i++) {
                if (ArrowList[i] == 0) {
                    ArrowList[i] = id;
                    break;
                }
            }
        }
        // PORT: the ROM adds to ArrowData[id] unchecked; ArrowData has ARROW_KINDS ints here (sbss
        // 0x0193f858..0x0193f864; 0x0193f864..0x0193f870 is unnamed): ids past them are not counted.
        if (id >= 0 && id < ARROW_KINDS)
            ArrowData[id]++;
        pause_reservation = 1;
        break;
    case ITEM_TABLE_LETTERS: {
        int *list = LetterList[LetterData[id][0]];   /* byte 0: the category */

        for (i = 0; i < LETTER_LIST_SLOTS; i++) {
            if (list[i] == 0) {
                list[i] = id;
                break;
            }
        }
        ssTrigRegisterEvent(LetterEvent(id), 1, 0);
        pause_reservation = 1;
        break;
    }
    case ITEM_TABLE_KEY_ITEMS:
        for (i = 0; i < KEY_ITEM_LIST_SLOTS; i++) {
            if (KeyItemList[i] == 0) {
                KeyItemList[i] = id;
                break;
            }
        }
        if (i == KEY_ITEM_LIST_SLOTS) {
            ssTrigSetVariable(var, -1, global, bank);
        } else {
            pause_reservation = 1;
            ssTrigRegisterEvent(ItemRowEvent(KeyItemData[id]), 1, 0);
        }
        break;
    case ITEM_TABLE_ITEMS:
        if (id < ITEM_FIXED_SLOTS) {
            // The first items have a slot of their own: ItemList[id - 1]. PORT: id 0 would write the
            // word before ItemList (KeyItemList[23] in the ROM's sbss): not written here.
            if (id >= 1)
                ItemList[id - 1] = id;
            ItemData[id][ITEM_FIELD_HELD]++;
            ssTrigRegisterEvent(ItemRowEvent(ItemData[id]), 1, 0);
            pause_reservation = 1;
            break;
        }
        // The others take the first free slot from ITEM_FIXED_SLOTS - 1 on; one already listed stays
        // where it is.
        for (i = ITEM_FIXED_SLOTS - 1; i < ITEM_LIST_SLOTS; i++) {
            if (ItemList[i] == id)
                break;
            if (ItemList[i] == 0) {
                ItemList[i] = id;
                break;
            }
        }
        if (i == ITEM_LIST_SLOTS) {
            ssTrigSetVariable(var, -1, global, bank);
        } else {
            ItemData[id][ITEM_FIELD_HELD]++;
            pause_reservation = 1;
            ssTrigRegisterEvent(ItemRowEvent(ItemData[id]), 1, 0);
        }
        break;
    default:
        break;
    }
    return XCMD_DONE;
}

// CT3 0x018e7130: the memory-card screens' ids back to variables argv[0..4] (each in the bank's own
// scope): main, slot, file, error, access.
int tiXcmd_GetMCflag(int argc, char **argv, int bank)
{
    (void)argc;
    ssTrigSetVariable(atoi(argv[0]) & 0xffff, mcMain_ID, 0, bank);
    ssTrigSetVariable(atoi(argv[1]) & 0xffff, mcSlot_ID, 0, bank);
    ssTrigSetVariable(atoi(argv[2]) & 0xffff, mcFile_ID, 0, bank);
    ssTrigSetVariable(atoi(argv[3]) & 0xffff, mcErr_ID, 0, bank);
    ssTrigSetVariable(atoi(argv[4]) & 0xffff, mcAccess_ID, 0, bank);
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

// stub of CT3 0x018e71f0
int tiXcmd_CheckMCFile(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CheckMCFile", 0x018e71f0);
    return {};
}

// stub of CT3 0x018e83d0
int tiXcmd_CopySunPs2Object(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_CopySunPs2Object", 0x018e83d0);
    return {};
}

// stub of CT3 0x018e2ae0
int tiXcmd_End_SPAttack_SCISSORS_MAN(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_End_SPAttack_SCISSORS_MAN", 0x018e2ae0);
    return {};
}

// stub of CT3 0x018e7b70
int tiXcmd_FreeGhostPole(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_FreeGhostPole", 0x018e7b70);
    return {};
}

// stub of CT3 0x018e20c0
int tiXcmd_GetBloodAttackObject(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_GetBloodAttackObject", 0x018e20c0);
    return {};
}

// stub of CT3 0x018e2b30
int tiXcmd_Init_SPAttack_SCISSORS_MAN(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_Init_SPAttack_SCISSORS_MAN", 0x018e2b30);
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

// stub of CT3 0x018e7d10
int tiXcmd_SetPlayerChara(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_SetPlayerChara", 0x018e7d10);
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

// stub of CT3 0x018e8b70
int tiXcmd_TEST00(int argc, char **argv, int bank)
{
    ALYSSAPAN_STUB("tiXcmd_TEST00", 0x018e8b70);
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
