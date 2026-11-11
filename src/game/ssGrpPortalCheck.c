// game/ssGrpPortalCheck.c -- Clock Tower 3: the trigger system and the room portals (EE 018d2ee0..018d9a40).
// Each bank carries trigger data (detectors, conditions, actors, its own event flags and variables);
// Trigger_main runs them every frame, ProcessPortal moves the characters between rooms.
// Slice 1 (2026-11-10): the event / variable API, the script byte readers, the enable checks,
// InitTriggerDetectorShape, CheckRoomEntry and the portals (ProcessPortal, ssScnPortalCheck,
// CheckPlaneEntry, ssGrpPortalCheck). Slice 2 (2026-11-10): Trigger_init / _deinit, TriggerGlobalInit,
// the trigger block getters and readers, the delay-enable ring. Slice 3 (2026-11-11): Trigger_main, the
// detector walk (ssTrigProcDetRecur, ssTrigProceedDetector, ssTrigCheckDetector and its shapes), the
// actor runners, the waits and the small checks. Slice 4 (2026-11-11): ssTrigCheckConditions and its
// checks (ssTrigCheckInput, ssTrigCheckVariable, ssTrigLookAtCheck, ssTrigAttackHitCheck),
// ssTrigSetEnable and Trigger_reset; ssTrigExecuteActCmd is a stub.

#include <math.h>
#include <string.h>

#include "common/alyssapan_stub.h"
#include "game/ssGrpPortalCheck.h"
#include "ga/gaInit3dlib.h"
#include "game/ahalloc.h"
#include "game/Character_Init.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/sound.h"
#include "game/ssSetTexUVforCubeMap.h"
#include "game/unassigned_data.h"
#include "game/LinkMatrixTree.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/wideXcmd_DebugPrint.h"

// ---- data (symtab names) -------------------------------------------------------------------------------
short ssScnShiftCharList[PORTAL_CHARA_COUNT] = {0, 2, 3, 4, 5, 6};   // CT3 0x01939d68
short ssGrpShiftCharList[PORTAL_CHARA_COUNT] = {0, 2, 3, 4, 5, 6};   // CT3 0x01939d78

// The per-bank temp sets. layout: host -- Trigger_init fills them with _SunAlloc blocks (the ROM's
// arrays of EE words; the blocks are the TU's own, never bank data).
static int *TempVariable[BANK_MAX];             // CT3 0x019440e0 (LOCAL, 344 bytes)
static u_char *TempEventFlag[BANK_MAX];         // CT3 0x01944240 (LOCAL, 344 bytes)

// The bank's trigger blocks (ALL_TABLE.trigger's three words, as host pointers into the bank image) and
// the per-bank work Trigger_init builds from them. layout: host (arrays of EE words in the ROM).
static const u_char *OthData[BANK_MAX];         // CT3 0x019446a0 (LOCAL, 344 bytes)
static const u_char *DetData[BANK_MAX];         // CT3 0x01944760 (LOCAL, 344 bytes)
static const u_char *ActData[BANK_MAX];         // CT3 0x01944600 (LOCAL, 344 bytes)
TRIG_WORK TrigWork[BANK_MAX];                   // CT3 0x01ad7810 (1032 bytes)
TRIG_DET_WORK *DetWork[BANK_MAX];               // CT3 0x01940db0 (344 bytes)
TRIG_ACT_WORK *ActWork[BANK_MAX];               // CT3 0x01940f10 (344 bytes)
static TRIG_DELAY_ENABLE *DlyEnaWork[BANK_MAX]; // CT3 0x01944080 (LOCAL, 344 bytes)
static u_short ssDERingFront[BANK_MAX];         // CT3 0x01943fd0 (LOCAL, 172 bytes)
static u_short ssDERingRear[BANK_MAX];          // CT3 0x01943f20 (LOCAL, 172 bytes)
static u_short numMaxDECount[BANK_MAX];         // CT3 0x01943e70 (LOCAL, 172 bytes)
static TRIG_CUR_ACT curActInfo;                 // CT3 0x01943d60 (LOCAL, 8 bytes)

// ---- portals -------------------------------------------------------------------------------------------

// The VU0 transform the portal checks repeat: out = m * v (rows 0..3 weighted by v's x, y, z, w).
static void PortalTransform(float out[4], const float m[4][4], const float v[4])
{
    for (int i = 0; i < 4; i++) {
        out[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
    }
}

// VU0's normalize (vmul / vaddy / vaddz / vrsqrt / vmulq.xyz): xyz scaled to length 1, w kept.
static void PortalNormalizeXYZ(float v[4])
{
    float q = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] *= q;
    v[1] *= q;
    v[2] *= q;
}

// The point-in-quad test both portal kinds share, as each ROM function inlines it: the four corners
// seen from the point, normalized, their neighbours' angles (ahAcos units) summed.
static int PortalCornerAngles(const float *const c[4], const float hit[4])
{
    float e[4][4];
    for (int k = 0; k < 4; k++) {
        for (int j = 0; j < 4; j++) {
            e[k][j] = c[k][j] - hit[j];
        }
        PortalNormalizeXYZ(e[k]);
    }
    float cosines[4];
    cosines[0] = gaInnerProductXYZ(e[1], e[0]);
    cosines[1] = gaInnerProductXYZ(e[2], e[1]);
    cosines[2] = gaInnerProductXYZ(e[3], e[2]);
    cosines[3] = gaInnerProductXYZ(e[0], e[3]);
    gaClampVector(cosines, cosines, -1.0f, 1.0f);
    int sum = ahAcos(cosines[0]);
    sum += ahAcos(cosines[1]);
    sum += ahAcos(cosines[2]);
    sum += ahAcos(cosines[3]);
    return sum;
}

// CT3 0x018d2ee0
// The group portals of the character's room: crossing one (this frame's step through its plane, inside
// its quad) sets CHARACTER.h100 to the portal's room id for the side it came from.
void ssGrpPortalCheck(CHARACTER *chara)
{
    int bank = chara->lightBank;
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    u_int *list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->groupPortals);
    if (list == NULL) {
        return;                     // PORT: the ROM would read EE address 0
    }
    float before[4], now[4];
    memcpy(before, &chara->posBefore, sizeof(before));
    memcpy(now, &chara->pos, sizeof(now));
    for (int i = 0; list[i] != 0; i++) {
        GRP_PORTAL *g = BANK_PTR(GRP_PORTAL *, bank, list[i]);
        if ((parts[g->part].flags & PARTS_FLAG_HIDDEN) != 0) {
            continue;
        }
        float sBefore = gaInnerProductXYZ(g->normal, before) - g->dist;
        float sNow = gaInnerProductXYZ(g->normal, now) - g->dist;
        if (!(sBefore * sNow <= 0.0f)) {
            continue;
        }
        float dir[4] = {now[0] - before[0], now[1] - before[1], now[2] - before[2], now[3] - before[3]};
        float t = -sBefore / gaInnerProductXYZ(g->normal, dir);
        float hit[4] = {dir[0] * t + before[0], dir[1] * t + before[1], dir[2] * t + before[2], dir[3]};
        const float *c[4] = {g->corners[0], g->corners[1], g->corners[2], g->corners[3]};
        if ((float)PortalCornerAngles(c, hit) < PORTAL_INSIDE_ANGLE) {
            continue;
        }
        short roomId = sBefore < 0.0f ? g->roomIdBack : g->roomIdFront;
        if (roomId != ROOM_ID_KEEP) {
            chara->h100 = roomId;
        }
    }
    // callcheck: ahAcos, gaClampVector (in PortalCornerAngles, which the ROM inlines)
}

// CT3 0x018d3240
// A character entering one of its room's portals (a detector with a room hash whose planes it crossed)
// moves into that room: the room hash, its bank, the ambient and direct light; Alyssa (id 0) also
// changes the current scene, its sounds and the floor.
void ssScnPortalCheck(CHARACTER *chara)
{
    int bank = chara->lightBank;    // the list is the bank's at entry (the ROM keeps its EE address)
    u_int *list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->detectors);
    if (list == NULL) {
        return;                     // PORT: the ROM would read EE address 0
    }
    for (int i = 0; list[i] != 0; i++) {
        TRIG_DETECTOR *det = BANK_PTR(TRIG_DETECTOR *, bank, list[i]);
        if (det->roomHash == 0 || CheckPlaneEntry(chara->lightBank, chara->id, det->part) == 0) {
            continue;
        }
        if (chara->id == 0) {
            idCurrentScene = det->bank;
            SSAutoStartSoundRoomIn(idCurrentScene);
        }
        chara->roomHashPrev = chara->roomHash;
        chara->roomHash = det->roomHash;
        if (chara->id == 0) {
            RoomInFlagOn();
        }
        chara->lightBank = det->bank;
        chara->h100 = det->roomId;
        short set[4] = {chara->id, det->bank, PORTAL_AMBIENT_FADE, 0};
        tnChangeCharAmbientColorSet(set);
        DirectLightCharaCounter[chara->id] = PORTAL_DIRECT_LIGHT_FRAMES;
        CharDirectLightSet[chara->id][CHAR_DLS_ON] = 0;
        CharDirectLightSet[chara->id][CHAR_DLS_FADE] = 0;
        chara->b102 = (s_char)ssTrigGetVariable(TRIG_VAR_ROOM_ENTRY, TRIG_SET_GLOBAL, -1);
        if (chara->id == 0) {
            GetAreaFloor(chara);
        }
    }
}

// CT3 0x018d33d0
// The portal pass: each listed character that is live (CHARACTER_FLAG_REFRACTION: the same top bit
// main's draw passes test) and stands in a drawn bank runs the room portals, then the group portals.
void ProcessPortal(void)
{
    for (int i = 0; i < PORTAL_CHARA_COUNT; i++) {
        CHARACTER *chara = &Character[ssScnShiftCharList[i]];
        if ((chara->flags & CHARACTER_FLAG_REFRACTION) != 0 && chara->lightBank >= 0 &&
            BankInfo[chara->lightBank].drawOn != 0) {
            ssScnPortalCheck(chara);
        }
    }
    for (int i = 0; i < PORTAL_CHARA_COUNT; i++) {
        CHARACTER *chara = &Character[ssGrpShiftCharList[i]];
        if ((chara->flags & CHARACTER_FLAG_REFRACTION) != 0 && chara->lightBank >= 0 &&
            BankInfo[chara->lightBank].drawOn != 0) {
            ssGrpPortalCheck(chara);
        }
    }
}

// CT3 0x018d3770
// Did the character cross the bank's detector of this part this frame: last frame's position in front of
// its plane, this frame's behind, and the crossing point inside its quad (the four corner angles seen
// from the point sum to a full turn)? The first detector of the part decides; a switched-off part
// (PARTS_FLAG_HIDDEN) never.
int CheckPlaneEntry(int bank, int chara, int part)
{
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    if ((parts[part].flags & PARTS_FLAG_HIDDEN) != 0) {
        return 0;
    }
    float before[4], now[4];
    memcpy(before, &Character[chara].posBefore, sizeof(before));
    memcpy(now, &Character[chara].pos, sizeof(now));
    u_int *list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->detectors);
    if (list == NULL) {
        return 0;                   // PORT: the ROM would read EE address 0
    }
    for (int i = 0; list[i] != 0; i++) {
        TRIG_DETECTOR *det = BANK_PTR(TRIG_DETECTOR *, bank, list[i]);
        if (det->part != part) {
            continue;
        }
        float (*m)[4] = BANK_PTR(float (*)[4], bank, det->matrix);
        const float *normalLocal = BANK_PTR(const float *, bank, det->normal);
        const float (*corners)[4] = BANK_PTR(const float (*)[4], bank, det->corners);
        float normal[4], c0[4];
        PortalTransform(normal, m, normalLocal);
        PortalTransform(c0, m, corners[0]);
        float d = gaInnerProductXYZ(normal, c0);
        float sBefore = gaInnerProductXYZ(normal, before) - d;
        float sNow = gaInnerProductXYZ(normal, now) - d;
        if (sBefore < 0.0f || !(sNow < 0.0f)) {
            return 0;
        }
        float dir[4] = {now[0] - before[0], now[1] - before[1], now[2] - before[2], now[3] - before[3]};
        float t = -sBefore / gaInnerProductXYZ(normal, dir);
        float hit[4] = {dir[0] * t + before[0], dir[1] * t + before[1], dir[2] * t + before[2], dir[3]};
        float c1[4], c2[4], c3[4];
        PortalTransform(c1, m, corners[1]);
        PortalTransform(c2, m, corners[2]);
        PortalTransform(c3, m, corners[3]);
        const float *c[4] = {c0, c1, c2, c3};
        int sum = PortalCornerAngles(c, hit);
        return (float)sum < PORTAL_INSIDE_ANGLE ? 0 : 1;
    }
    return 0;
    // callcheck: ahAcos, gaClampVector (in PortalCornerAngles, which the ROM inlines)
}

// CT3 0x018d3d90
// Points each detector of the bank at its part's world matrix (an EE word of the bank's data).
void InitTriggerDetectorShape(int scene)
{
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[scene];
    u_int *list = BANK_PTR(u_int *, scene, All_table_TBL[scene]->detectors);
    if (list == NULL) {
        return;                     // PORT: the ROM would read EE address 0
    }
    for (int i = 0; list[i] != 0; i++) {
        TRIG_DETECTOR *det = BANK_PTR(TRIG_DETECTOR *, scene, list[i]);
        det->matrix = BankEeAddr(scene, parts[det->part].matrix);
    }
}

// ---- enable checks -------------------------------------------------------------------------------------

// CT3 0x018d89c0
// Non-zero while none of mask's TRG_CHAR_STOP_* bits stop the characters.
int ssTrigCheckInputEnable(u_int mask)
{
    return (TrgCharStopFlag & mask) == 0;
}

// CT3 0x018d8e80
// Non-zero while the bank's part is not switched off (PARTS_FLAG_HIDDEN).
int ssTrigCheckEnable(int part, int bank)
{
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    return (parts[(u_short)part].flags & PARTS_FLAG_HIDDEN) == 0;
}

// CT3 0x018d9160
int CheckRoomEntry(int chara, u_long roomHash)
{
    return roomHash == Character[chara].roomHash;
}

// ---- event flags and variables -------------------------------------------------------------------------

// CT3 0x018d9100
int ssTrigCheckEvent(int event, int global, int bank)
{
    u_int bits;
    if (global == TRIG_SET_TEMP) {
        if (TempEventFlag[bank] == NULL) {
            return 0;               // PORT: no trigger data for the bank yet (Trigger_init)
        }
        bits = TempEventFlag[bank][event >> 3];
    } else {
        bits = GlobalEventFlag[event >> 3];
    }
    return (int)(bits >> (event & 7)) & 1;
}

// CT3 0x018d9430
int ssTrigGetVariable(int var, int global, int bank)
{
    if (global == TRIG_SET_TEMP) {
        if (TempVariable[bank] == NULL) {
            return 0;               // PORT: no trigger data for the bank yet (Trigger_init)
        }
        return TempVariable[bank][var];
    }
    return GlobalVariable[var];
}

// CT3 0x018d9470
void ssTrigSetVariable(int var, int value, int global, int bank)
{
    if (global == TRIG_SET_TEMP) {
        if (TempVariable[bank] == NULL) {
            return;                 // PORT: no trigger data for the bank yet (Trigger_init)
        }
        TempVariable[bank][(u_short)var] = value;
    } else {
        GlobalVariable[(u_short)var] = value;
    }
}

// CT3 0x018d9560
void ssTrigRemoveEvent(int event, int global, int bank)
{
    u_char *flags = global == TRIG_SET_TEMP ? TempEventFlag[bank] : GlobalEventFlag;
    if (flags == NULL) {
        return;                     // PORT: no trigger data for the bank yet (Trigger_init)
    }
    flags[event >> 3] &= (u_char)~(1 << (event & 7));
}

// CT3 0x018d95e0
void ssTrigRegisterEvent(int event, int global, int bank)
{
    u_char *flags = global == TRIG_SET_TEMP ? TempEventFlag[bank] : GlobalEventFlag;
    if (flags == NULL) {
        return;                     // PORT: no trigger data for the bank yet (Trigger_init)
    }
    flags[event >> 3] |= (u_char)(1 << (event & 7));
}

// ---- the trigger data's byte readers (little-endian, unaligned; *pos advances) ------------------------

// CT3 0x018d9810
u_char ssTrigRead1Byte(int *pos, const u_char *data)
{
    return data[(*pos)++];
}

// CT3 0x018d9830
u_short ssTrigRead2Byte(int *pos, const u_char *data)
{
    u_short lo = data[(*pos)++];
    u_short hi = data[(*pos)++];
    return (u_short)(lo | hi << 8);
}

// CT3 0x018d9870
u_int ssTrigRead4Byte(int *pos, const u_char *data)
{
    u_int v = 0;
    for (int i = 0; i < 4; i++) {
        v |= (u_int)data[(*pos)++] << (i * 8);
    }
    return v;
}

// CT3 0x018d98e0
u_long ssTrigRead8Byte(int *pos, const u_char *data)
{
    u_long v = 0;
    for (int i = 0; i < 8; i++) {
        v |= (u_long)data[(*pos)++] << (i * 8);
    }
    return v;
}

// ---- the trigger blocks ------------------------------------------------------------------------------

// The bank's TRIG_DATA word `index` as a host pointer into its image (NULL for a bank without one).
static const u_char *TrigBlock(int bank, int index)
{
    const u_int *data = BANK_PTR(const u_int *, bank, All_table_TBL[bank]->trigger);
    if (data == NULL) {
        return NULL;                // PORT: the ROM would read EE address 0
    }
    return BANK_PTR(const u_char *, bank, data[index]);
}

// CT3 0x018d9650
const u_char *ssTrigGetOtherData(int bank)
{
    return TrigBlock(bank, offsetof(TRIG_DATA, other) / 4);
}

// CT3 0x018d9670
const u_char *ssTrigGetActData(int bank)
{
    return TrigBlock(bank, offsetof(TRIG_DATA, actors) / 4);
}

// CT3 0x018d9690
const u_char *ssTrigGetDetData(int bank)
{
    return TrigBlock(bank, offsetof(TRIG_DATA, detectors) / 4);
}

// CT3 0x018d96b0
u_char ssTrigReadNumDeactivateCmd(const TRIG_ACT_WORK *act, int bank)
{
    int pos = (int)act->cmds[ACT_CMDS_DEACTIVATE].pos;
    return ssTrigRead1Byte(&pos, ActData[bank]);
}

// CT3 0x018d96f0
u_char ssTrigReadNumActivateCmd(const TRIG_ACT_WORK *act, int bank)
{
    int pos = (int)act->cmds[ACT_CMDS_ACTIVATE].pos;
    return ssTrigRead1Byte(&pos, ActData[bank]);
}

// CT3 0x018d9750
u_char ssTrigReadActMode(const TRIG_DET_WORK *det, int bank)
{
    int pos = (int)det->actModePos;
    return ssTrigRead1Byte(&pos, DetData[bank]);
}

// CT3 0x018d9790
u_char ssTrigReadNumActors(const TRIG_DET_WORK *det, int bank)
{
    int pos = (int)det->actorsPos;
    return ssTrigRead1Byte(&pos, DetData[bank]);
}

// CT3 0x018d97d0
u_char ssTrigReadNumConds(const TRIG_DET_WORK *det, int bank)
{
    int pos = (int)det->condsPos;
    return ssTrigRead1Byte(&pos, DetData[bank]);
}

// ---- the delayed part switches (a ring per bank) -------------------------------------------------------

// CT3 0x018d3f30
// The front entry, popped (NULL when the ring is empty).
TRIG_DELAY_ENABLE *ssTrigDEPop_Front(TRIG_DELAY_ENABLE *ring, int bank)
{
    u_short front = ssDERingFront[bank];
    if (ssDERingRear[bank] == front) {
        return NULL;
    }
    ssDERingFront[bank] = (u_short)((front + 1) % numMaxDECount[bank]);
    return &ring[front];
}

// CT3 0x018d3f90
// Appends req (part and op; its wait starts at DE_WAIT_PENDING). Zero when the ring is full.
int ssTrigDEPush_Back(TRIG_DELAY_ENABLE *ring, int bank, const TRIG_DELAY_ENABLE *req)
{
    u_int rear = ssDERingRear[bank];
    u_int next = (rear + 1) % numMaxDECount[bank];
    if (next == ssDERingFront[bank]) {
        return 0;
    }
    ring[rear].part = req->part;
    ring[rear].op = req->op;
    ring[rear].wait = DE_WAIT_PENDING;
    ssDERingRear[bank] = (u_short)next;
    return 1;
}

// CT3 0x018d3e00
// Gives the first pending entry for part a new op; the walk stops at the first entry no longer pending.
int ssTrigDE_ChangeReq(TRIG_DELAY_ENABLE *ring, int bank, short part, u_char op)
{
    u_int i = ssDERingFront[bank];
    while (i != ssDERingRear[bank] && ring[i].wait == DE_WAIT_PENDING) {
        if (ring[i].part == part) {
            ring[i].op = op;
            return 1;
        }
        i = (i + 1) & 0xffff;
        if (i == numMaxDECount[bank]) {
            i = 0;
        }
    }
    return 0;
}

// CT3 0x018d3ea0
// Cancels every queued switch of part (DE_OP_CANCELLED: popped without effect).
void ssTrigDE_DisableReq(TRIG_DELAY_ENABLE *ring, int bank, short part)
{
    u_short rear = ssDERingRear[bank];
    u_short count = numMaxDECount[bank];
    u_int i = ssDERingFront[bank];
    while (i != rear) {
        if (ring[i].part == part) {
            ring[i].op = DE_OP_CANCELLED;
        }
        i = (i + 1) & 0xffff;
        if (i == count) {
            i = 0;
        }
    }
}

// CT3 0x018d4010
// Counts entry index's wait down; once it has run out, switches its part and pops the ring's front.
void ssTrigExecDelayEnable(int index, int bank)
{
    TRIG_DELAY_ENABLE *de = &DlyEnaWork[bank][index];
    u_char wait = de->wait--;
    if (wait != 0) {
        return;
    }
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    switch (de->op) {
    case DE_OP_HIDE:
        parts[(u_short)de->part].flags |= PARTS_FLAG_HIDDEN;
        break;
    case DE_OP_SHOW:
        parts[(u_short)de->part].flags &= ~PARTS_FLAG_HIDDEN;
        break;
    case DE_OP_TOGGLE:
        parts[(u_short)de->part].flags ^= PARTS_FLAG_HIDDEN;
        break;
    default:                        // DE_OP_CANCELLED and anything else: no switch
        break;
    }
    ssTrigDEPop_Front(DlyEnaWork[bank], bank);
}

// CT3 0x018d92d0
// Switches a part of the bank (DE_OP_*): at once (delayed == 0; its queued switches are cancelled), or
// queued on the delay-enable ring (a pending entry for the part takes the new op instead).
void ssTrigSetEnable(int part, int op, int delayed, int bank)
{
    if (delayed == 0) {
        MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
        if (DlyEnaWork[bank] != NULL) {     // PORT: the ROM walks an empty ring at EE address 0
            ssTrigDE_DisableReq(DlyEnaWork[bank], bank, (short)part);
        }
        switch ((u_char)op) {
        case DE_OP_TOGGLE:
            parts[(u_short)part].flags ^= PARTS_FLAG_HIDDEN;
            break;
        case DE_OP_SHOW:
            parts[(u_short)part].flags &= ~PARTS_FLAG_HIDDEN;
            break;
        case DE_OP_HIDE:
            parts[(u_short)part].flags |= PARTS_FLAG_HIDDEN;
            break;
        default:
            break;
        }
        return;
    }
    if (DlyEnaWork[bank] == NULL) {
        return;                     // PORT: no delay ring for the bank (Trigger_init found no slots)
    }
    if (ssTrigDE_ChangeReq(DlyEnaWork[bank], bank, (short)part, (u_char)op) == 0) {
        TRIG_DELAY_ENABLE req = {(short)part, (u_char)op, 0};
        ssTrigDEPush_Back(DlyEnaWork[bank], bank, &req);
    }
}

// ---- init and teardown ---------------------------------------------------------------------------------

// CT3 0x018d5230
// At boot (system_init): no bank has trigger work; the global flags clear, the global variables -1.
void TriggerGlobalInit(void)
{
    for (int i = 0; i < BANK_MAX; i++) {
        OthData[i] = NULL;
        DetData[i] = NULL;
        ActData[i] = NULL;
        DetWork[i] = NULL;
        ActWork[i] = NULL;
        DlyEnaWork[i] = NULL;
    }
    memset(GlobalEventFlag, 0, sizeof(GlobalEventFlag));
    for (int i = 0; i < GLOBAL_VARIABLE_COUNT; i++) {
        GlobalVariable[i] = -1;
    }
}

// CT3 0x018d4830
// Back to the scene's start (wideXcmd_TriggerInit, the axe AI's resets): every detector false, no actor
// running, the temp flags clear, the temp variables -1, the delay ring empty (its size from the header).
void Trigger_reset(int bank)
{
    int pos = 0;
    u_int numEvents, numVars, numDelay;
    const u_char *oth;
    if (bank >= BANK_MAX) {
        return;
    }
    oth = OthData[bank];
    if (oth == NULL) {
        return;                     // PORT: a bank without trigger data (the ROM reads EE address 0)
    }
    ssTrigRead8Byte(&pos, oth);     // the skipped word, the detector and actor counts
    numEvents = ssTrigRead2Byte(&pos, oth);
    numVars = ssTrigRead2Byte(&pos, oth);
    numDelay = ssTrigRead2Byte(&pos, oth);
    for (int i = 0; i < TrigWork[bank].numDetectors; i++) {
        DetWork[bank][i].state = DET_STATE_FALSE;
    }
    for (int i = 0; i < TrigWork[bank].numActors; i++) {
        ActWork[bank][i].state = 0;
    }
    if (numEvents != 0) {
        for (int i = 0; i < (int)(numEvents >> 3) + 1; i++) {
            TempEventFlag[bank][i] = 0;
        }
    }
    for (u_int i = 0; i < numVars; i++) {
        TempVariable[bank][i] = -1;
    }
    ssDERingRear[bank] = 0;
    ssDERingFront[bank] = 0;
    numMaxDECount[bank] = (u_short)(numDelay * DE_SLOTS_PER_COUNT);
}

// CT3 0x018d4be0
// A scene's teardown (scene_end): its blocks forgotten, its work freed.
void Trigger_deinit(int bank)
{
    if (bank >= BANK_MAX) {
        return;
    }
    OthData[bank] = NULL;
    DetData[bank] = NULL;
    ActData[bank] = NULL;
    _SunFree(&gHeapJunc, (void **)&DetWork[bank]);
    _SunFree(&gHeapJunc, (void **)&ActWork[bank]);
    _SunFree(&gHeapJunc, (void **)&TempEventFlag[bank]);
    _SunFree(&gHeapJunc, (void **)&TempVariable[bank]);
    _SunFree(&gHeapJunc, (void **)&DlyEnaWork[bank]);
}

// CT3 0x018d4c80
// A scene's start (scene_init): reads the bank's trigger header, builds a work record per detector and
// per actor, and allocates its temp event flags (cleared), temp variables (-1) and delay-enable ring.
void Trigger_init(int bank)
{
    if (bank >= BANK_MAX) {
        return;
    }
    OthData[bank] = ssTrigGetOtherData(bank);
    DetData[bank] = ssTrigGetDetData(bank);
    ActData[bank] = ssTrigGetActData(bank);
    if (OthData[bank] == NULL) {
        return;                     // PORT: a bank without trigger data (the ROM reads EE address 0)
    }

    TRIG_WORK *tw = &TrigWork[bank];
    int pos = 0;
    ssTrigRead4Byte(&pos, OthData[bank]);
    tw->numDetectors = ssTrigRead2Byte(&pos, OthData[bank]);
    tw->numActors = ssTrigRead2Byte(&pos, OthData[bank]);
    u_int numEvents = ssTrigRead2Byte(&pos, OthData[bank]);
    u_int numVars = ssTrigRead2Byte(&pos, OthData[bank]);
    u_int numDelay = ssTrigRead2Byte(&pos, OthData[bank]);
    tw->numRoots = (short)ssTrigRead2Byte(&pos, OthData[bank]);
    tw->roots = OthData[bank] + pos;

    if (tw->numDetectors != 0) {
        TRIG_DET_WORK *dw = (TRIG_DET_WORK *)_SunAlloc(&gHeapJunc,
                                                       tw->numDetectors * (int)sizeof(TRIG_DET_WORK), 0x10);
        DetWork[bank] = dw;
        pos = 0;
        for (int i = 0; i < tw->numDetectors; i++) {
            TRIG_DET_WORK *d = &dw[i];
            d->state = 0;
            d->actModePos = ssTrigRead4Byte(&pos, DetData[bank]);
            d->actMode = ssTrigReadActMode(d, bank);
            d->condsPos = ssTrigRead4Byte(&pos, DetData[bank]);
            d->numConds = ssTrigReadNumConds(d, bank);
            d->actorsPos = ssTrigRead4Byte(&pos, DetData[bank]);
            d->numActors = ssTrigReadNumActors(d, bank);
            int body = (int)ssTrigRead4Byte(&pos, DetData[bank]);
            d->numChildren = (short)ssTrigRead2Byte(&body, DetData[bank]);
            d->children = DetData[bank] + body;
        }
    }

    if (tw->numActors != 0) {
        TRIG_ACT_WORK *aw = (TRIG_ACT_WORK *)_SunAlloc(&gHeapJunc,
                                                       tw->numActors * (int)sizeof(TRIG_ACT_WORK), 0x10);
        ActWork[bank] = aw;
        pos = 0;
        for (int i = 0; i < tw->numActors; i++) {
            TRIG_ACT_WORK *a = &aw[i];
            a->state = 0;
            a->cmds[ACT_CMDS_ACTIVATE].pos = ssTrigRead4Byte(&pos, ActData[bank]);
            a->cmds[ACT_CMDS_ACTIVATE].num = ssTrigReadNumActivateCmd(a, bank);
            a->cmds[ACT_CMDS_DEACTIVATE].pos = ssTrigRead4Byte(&pos, ActData[bank]);
            a->cmds[ACT_CMDS_DEACTIVATE].num = ssTrigReadNumDeactivateCmd(a, bank);
        }
    }

    if (numEvents != 0) {
        int bytes = (int)(numEvents >> 3) + 1;
        TempEventFlag[bank] = (u_char *)_SunAlloc(&gHeapJunc, bytes, 0x10);
        memset(TempEventFlag[bank], 0, (size_t)bytes);
    }

    if (numVars != 0) {
        TempVariable[bank] = (int *)_SunAlloc(&gHeapJunc, (int)(numVars * 4), 0x10);
        for (u_int i = 0; i < numVars; i++) {
            TempVariable[bank][i] = -1;
        }
    }

    ssDERingRear[bank] = 0;
    ssDERingFront[bank] = 0;
    if (numDelay == 0) {
        numMaxDECount[bank] = 0;
    } else {
        // numDelay x 8 bytes: DE_SLOTS_PER_COUNT four-byte entries per counted switch
        DlyEnaWork[bank] = (TRIG_DELAY_ENABLE *)_SunAlloc(&gHeapJunc,
            (int)(numDelay * DE_SLOTS_PER_COUNT * sizeof(TRIG_DELAY_ENABLE)), 0x10);
        numMaxDECount[bank] = (u_short)(numDelay * DE_SLOTS_PER_COUNT);
    }
}

// ---- the frame (Trigger_main) --------------------------------------------------------------------------

// The scene banks' gate (0..5): drawn, not a behaviour bank, its animation not stopped.
static int TrigSceneBankLive(int i)
{
    const BANK_INFO *b = &BankInfo[i];
    return b->drawOn != 0 && b->type != FS_LOAD_BEHAVIOR && (b->flags & BANK_FLAG_ANIM_STOPPED) == 0;
}

// The other banks' gate (6..85): as the scene banks', except that a BANK_FLAG_DRAW_FIRST bank runs once
// loaded (state > 0) whether drawn or not.
static int TrigObjectBankLive(int i)
{
    const BANK_INFO *b = &BankInfo[i];
    int live = (b->flags & BANK_FLAG_DRAW_FIRST) != 0 ? b->state > 0 : b->drawOn != 0;
    return live && b->type != FS_LOAD_BEHAVIOR && (b->flags & BANK_FLAG_ANIM_STOPPED) == 0;
}

// The delay-enable pass's gate (both ranges): loaded rather than drawn.
static int TrigDelayBankLive(int i)
{
    const BANK_INFO *b = &BankInfo[i];
    return b->state > 0 && b->type != FS_LOAD_BEHAVIOR && (b->flags & BANK_FLAG_ANIM_STOPPED) == 0;
}

// Every running actor list of the bank one step on (the ROM inlines it in both passes).
static void TrigProceedActors(int bank)
{
    u_short n = TrigWork[bank].numActors;
    for (u_short i = 0; i < n; i++) {
        const TRIG_ACT_WORK *a = &ActWork[bank][i];
        if ((a->state & TRIG_RUN_ACTIVATE) != 0 && a->det[ACT_CMDS_ACTIVATE] != -1) {
            ssTrigProceedActor(i, TRIG_RUN_ACTIVATE, bank);
        }
        if ((a->state & TRIG_RUN_DEACTIVATE) != 0 && a->det[ACT_CMDS_DEACTIVATE] != -1) {
            ssTrigProceedActor(i, TRIG_RUN_DEACTIVATE, bank);
        }
    }
}

// The bank's delayed part switches, front to rear (the ring's ends as they stand before the walk).
static void TrigExecDelayRing(int bank)
{
    u_int i = ssDERingFront[bank];
    u_short rear = ssDERingRear[bank];
    u_short count = numMaxDECount[bank];
    while (i != rear) {
        ssTrigExecDelayEnable((int)i, bank);
        i = (i + 1) & 0xffff;
        if (i == count) {
            i = 0;
        }
    }
}

// CT3 0x018d4120
// Once a frame (main): the scene banks' detectors, actors and delayed switches, then, unless the world
// is paused, the other banks'. A bank that runs skips the rest of its group.
void Trigger_main(void)
{
    for (int i = 0; i < BANK_SCENE_MAX; i++) {
        if (TrigSceneBankLive(i) && DetData[i] != NULL) {
            ssTrigProcDetRecur(TrigWork[i].numRoots, TrigWork[i].roots, i);
            i += BankInfo[i].groupCount - 1;
        }
    }
    for (int i = 0; i < BANK_SCENE_MAX; i++) {
        if (TrigSceneBankLive(i) && ActData[i] != NULL) {
            TrigProceedActors(i);
            i += BankInfo[i].groupCount - 1;
        }
    }
    for (int i = 0; i < BANK_SCENE_MAX; i++) {
        if (TrigDelayBankLive(i) && DlyEnaWork[i] != NULL) {
            TrigExecDelayRing(i);
            i += BankInfo[i].groupCount - 1;
        }
    }
    if ((pause != 0 && (pause & PAUSE_RUNNING) == 0) || pause_reservation != 0) {
        return;
    }
    for (int i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (TrigObjectBankLive(i) && DetData[i] != NULL) {
            ssTrigProcDetRecur(TrigWork[i].numRoots, TrigWork[i].roots, i);
            i += BankInfo[i].groupCount - 1;
        }
    }
    for (int i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (TrigObjectBankLive(i) && ActData[i] != NULL) {
            TrigProceedActors(i);
            i += BankInfo[i].groupCount - 1;
        }
    }
    for (int i = BANK_SCENE_MAX; i < BANK_MAX; i++) {
        if (TrigDelayBankLive(i) && DlyEnaWork[i] != NULL) {
            TrigExecDelayRing(i);
            i += BankInfo[i].groupCount - 1;
        }
    }
    // callcheck: ssTrigProceedActor, ssTrigExecDelayEnable (in TrigProceedActors / TrigExecDelayRing,
    // which the ROM inlines in both passes)
}

// ---- detectors -----------------------------------------------------------------------------------------

// CT3 0x018d4710
// Runs count detectors (ids: u_shorts) and their actors; a detector that is true runs its children.
void ssTrigProcDetRecur(int count, const u_char *ids, int bank)
{
    int pos = 0;
    for (int i = 0; i < count; i++) {
        short det = (short)ssTrigRead2Byte(&pos, ids);
        int run = ssTrigProceedDetector(det, bank);
        if (run == TRIG_RUN_ACTIVATE) {
            ssTrigRunActors(det, TRIG_RUN_ACTIVATE, bank);
        } else if (run == TRIG_RUN_DEACTIVATE) {
            ssTrigRunActors(det, TRIG_RUN_DEACTIVATE, bank);
        }
        const TRIG_DET_WORK *d = &DetWork[bank][det];
        if (d->state == DET_STATE_TRUE && d->numChildren != 0) {
            ssTrigProcDetRecur(d->numChildren, d->children, bank);
        }
    }
}

// CT3 0x018d88a0
// The detector's new state (its shape, then its conditions) and what its actors do: a level detector
// activates every frame it is true and deactivates every frame it is false; an edge detector only when
// its state changes.
int ssTrigProceedDetector(short det, int bank)
{
    TRIG_DET_WORK *d = &DetWork[bank][det];
    if (d->actMode == DET_ACT_MODE_LEVEL) {
        if (!ssTrigCheckDetector(det, bank) || !ssTrigCheckConditions(d, bank)) {
            d->state = DET_STATE_FALSE;
            return TRIG_RUN_DEACTIVATE;
        }
        d->state = DET_STATE_TRUE;
        return TRIG_RUN_ACTIVATE;
    }
    if (!ssTrigCheckDetector(det, bank)) {
        if (d->state == DET_STATE_TRUE) {
            d->state = DET_STATE_FALSE;
            return TRIG_RUN_DEACTIVATE;
        }
        return TRIG_RUN_NONE;
    }
    if (!ssTrigCheckConditions(d, bank)) {
        if (d->state == DET_STATE_TRUE) {
            d->state = DET_STATE_FALSE;
            return TRIG_RUN_DEACTIVATE;
        }
    } else if (d->state == DET_STATE_FALSE) {
        d->state = DET_STATE_TRUE;
        return TRIG_RUN_ACTIVATE;
    }
    return TRIG_RUN_NONE;
}

// CT3 0x018d8580
// The detector's shape: the byte after its act mode names the kind, then the character it watches and
// the shapes' count; true as soon as one shape holds the character.
int ssTrigCheckDetector(short det, int bank)
{
    TRIG_DET_WORK *d = &DetWork[bank][det];
    const u_char *data = DetData[bank];
    d->cursor = (int)d->actModePos + 1;
    u_char kind = ssTrigRead1Byte(&d->cursor, data);
    if (kind == DET_SHAPE_ALWAYS) {
        return 1;
    }
    if (kind > DET_SHAPE_ALWAYS) {
        return 0;
    }
    u_char chara = ssTrigRead1Byte(&d->cursor, data);
    u_char count = ssTrigRead1Byte(&d->cursor, data);
    for (int i = 0; i < count; i++) {
        int hit;
        switch (kind) {
        case DET_SHAPE_BOX:
            hit = CheckAxisAlignedBounds(bank, chara, (short)ssTrigRead2Byte(&d->cursor, data));
            break;
        case DET_SHAPE_PLANE_ENTRY:
            hit = CheckPlaneEntry(bank, chara, (short)ssTrigRead2Byte(&d->cursor, data));
            break;
        case DET_SHAPE_INSIDE_PLANES:
            hit = CheckInsidePlanes(bank, chara, (short)ssTrigRead2Byte(&d->cursor, data));
            break;
        case DET_SHAPE_ROOM:
            hit = CheckRoomEntry(chara, ssTrigRead8Byte(&d->cursor, data));
            break;
        default:                    // DET_SHAPE_CONTACT
            hit = CheckContactSurface((short)bank, chara, (short)ssTrigRead2Byte(&d->cursor, data));
            break;
        }
        if (hit != 0) {
            return hit;
        }
    }
    return 0;
}

// CT3 0x018d3520
// The character's position inside the detector's box (its centre moved by the part's matrix, its half
// extents along the world axes). A hidden part holds nothing.
int CheckAxisAlignedBounds(int bank, int chara, short part)
{
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    if ((parts[part].flags & PARTS_FLAG_HIDDEN) != 0) {
        return 0;
    }
    u_int *list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->detectors);
    if (list == NULL) {
        return 0;                   // PORT: the ROM would read EE address 0
    }
    float pos[4];
    memcpy(pos, &Character[chara].pos, sizeof(pos));
    for (int i = 0; list[i] != 0; i++) {
        TRIG_DETECTOR *det = BANK_PTR(TRIG_DETECTOR *, bank, list[i]);
        if (det->part != part) {
            continue;
        }
        float (*m)[4] = BANK_PTR(float (*)[4], bank, det->matrix);
        float c[4];
        PortalTransform(c, m, det->centre);
        for (int k = 0; k < 3; k++) {
            if (!(fabsf(c[k] - pos[k]) < det->halfSize[k])) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

// CT3 0x018d3bb0
// The character's position behind every plane of the detector's volume (normals and points moved by the
// part's matrix). A hidden part holds nothing.
int CheckInsidePlanes(int bank, int chara, short part)
{
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    if ((parts[part].flags & PARTS_FLAG_HIDDEN) != 0) {
        return 0;
    }
    float pos[4];
    memcpy(pos, &Character[chara].pos, sizeof(pos));
    u_int *list = BANK_PTR(u_int *, bank, All_table_TBL[bank]->detectors);
    if (list == NULL) {
        return 0;                   // PORT: the ROM would read EE address 0
    }
    for (int i = 0; list[i] != 0; i++) {
        TRIG_DETECTOR *det = BANK_PTR(TRIG_DETECTOR *, bank, list[i]);
        if (det->part != part) {
            continue;
        }
        float (*m)[4] = BANK_PTR(float (*)[4], bank, det->matrix);
        const TRIG_PLANE *planes = BANK_PTR(const TRIG_PLANE *, bank, det->normal);
        const float (*points)[4] = BANK_PTR(const float (*)[4], bank, det->corners);
        int point = 0;
        for (int k = 0; k < det->numPlanes; k++) {
            float n[4], q[4];
            PortalTransform(n, m, planes[k].normal);
            PortalTransform(q, m, points[point]);
            float d = gaInnerProductXYZ(n, q);
            if (!(gaInnerProductXYZ(n, pos) - d < 0.0f)) {
                return 0;
            }
            point += planes[k].numPoints;
        }
        return 1;
    }
    return 0;
}

// The part a character's target stands on (the target object's +0x198 short), -1 for none. PORT: the
// targets (CHARACTER.w300[0] / [1], CHARA_TARGET / CHARA_TARGET2 in wideXcmd_DebugPrint.h) are EE words
// with no host form yet (open-items.md 1 (v), (ac)): the host answers -1 until their writer decides it.
static int ContactTargetPart(u_int target)
{
    (void)target;
    return -1;
}

// CT3 0x018d36f0
// The character in the bank's light, touching the part through one of its two targets.
int CheckContactSurface(short bank, int chara, short part)
{
    const CHARACTER *ch = &Character[chara];
    if (bank != ch->lightBank) {
        return 0;
    }
    if (ch->w300[0] != 0 && ContactTargetPart((u_int)ch->w300[0]) == part) {
        return 1;
    }
    if (ch->w300[1] != 0 && ContactTargetPart((u_int)ch->w300[1]) == part) {
        return 1;
    }
    return 0;
}

// ---- actors --------------------------------------------------------------------------------------------

// CT3 0x018d7a90
// Starts every actor of the detector (its u_short ids after the actor count) in mode.
void ssTrigRunActors(short det, int mode, int bank)
{
    TRIG_DET_WORK *d = &DetWork[bank][det];
    d->cursor = (int)d->actorsPos + 1;
    u_char n = d->numActors;
    for (int i = 0; i < n; i++) {
        u_short actor = ssTrigRead2Byte(&d->cursor, DetData[bank]);
        ssTrigRunActor(det, actor, (u_int)mode, bank);
    }
}

// Rewinds an actor's command list to its first command.
static void TrigRewindCmds(TRIG_ACT_CMDS *c)
{
    c->cursor = (int)c->pos + 1;
}

// CT3 0x018d79f0
// Starts the actor's activate or deactivate list from its first command, on behalf of det.
void ssTrigRunActor(short det, u_short actor, u_int mode, int bank)
{
    TRIG_ACT_WORK *a = &ActWork[bank][actor];
    a->state |= mode;
    if (mode == TRIG_RUN_DEACTIVATE || mode == TRIG_RUN_ACTIVATE) {
        int k = mode == TRIG_RUN_DEACTIVATE ? ACT_CMDS_DEACTIVATE : ACT_CMDS_ACTIVATE;
        a->det[k] = det;
        TrigRewindCmds(&a->cmds[k]);
        a->cmds[k].wait = -1;
        a->cmds[k].step = 0;
    }
}

// CT3 0x018d78f0
// One step of the actor's list (curActInfo says which for the commands); a finished list rewinds and
// stops running.
void ssTrigProceedActor(u_short actor, int mode, int bank)
{
    curActInfo.bank = (short)bank;
    curActInfo.mode = mode;
    curActInfo.actor = actor;
    TRIG_ACT_WORK *a = &ActWork[bank][actor];
    int done = 1;
    if (mode == TRIG_RUN_DEACTIVATE) {
        done = ssTrigExecuteActCmd(&a->cmds[ACT_CMDS_DEACTIVATE], bank);
    } else if (mode == TRIG_RUN_ACTIVATE) {
        done = ssTrigExecuteActCmd(&a->cmds[ACT_CMDS_ACTIVATE], bank);
    }
    if ((char)done == 0) {
        return;
    }
    if (mode == TRIG_RUN_DEACTIVATE) {
        TrigRewindCmds(&a->cmds[ACT_CMDS_DEACTIVATE]);
        a->state &= ~(u_int)TRIG_RUN_DEACTIVATE;
    } else if (mode == TRIG_RUN_ACTIVATE) {
        TrigRewindCmds(&a->cmds[ACT_CMDS_ACTIVATE]);
        a->state &= ~(u_int)TRIG_RUN_ACTIVATE;
    }
}

// CT3 0x018d9190
// Stops the actor's list and rewinds it.
void ssTrigCancelActor(u_short actor, int mode, int bank)
{
    TRIG_ACT_WORK *a = &ActWork[bank][actor];
    if (mode == TRIG_RUN_DEACTIVATE) {
        TrigRewindCmds(&a->cmds[ACT_CMDS_DEACTIVATE]);
        a->state &= ~(u_int)TRIG_RUN_DEACTIVATE;
    } else if (mode == TRIG_RUN_ACTIVATE) {
        TrigRewindCmds(&a->cmds[ACT_CMDS_ACTIVATE]);
        a->state &= ~(u_int)TRIG_RUN_ACTIVATE;
    }
}

// CT3 0x018d9220
// A script command that runs an actor to its end: the first call (*state == -1) starts it, each call
// steps it; true (and *state back to -1) once the list has finished.
int ssTrigExecuteActor(int *state, u_short actor, u_int mode, int bank)
{
    if (*state == -1) {
        ssTrigRunActor(-1, actor, mode, bank);
        *state = 1;
    }
    ssTrigProceedActor(actor, (int)mode, bank);
    if ((mode & ActWork[bank][actor].state) != 0) {
        return 0;
    }
    *state = -1;
    return 1;
}

// CT3 0x018d9730
void ssTrigGetCurActInfo(TRIG_CUR_ACT *out)
{
    *out = curActInfo;
}

// ---- waits and small checks ----------------------------------------------------------------------------

// CT3 0x018d94c0
// A wait on the global clock: the first call (*wait == -1) loads ticks, each later call takes this
// frame's global ticks off; true (and *wait back to -1) once it has run out.
int ssTrigWaitByAnimFrameForGlobal(int *wait, int ticks)
{
    if (*wait == -1) {
        *wait = ticks;
    } else {
        *wait -= gGlobalNextTicksInterval;
    }
    int left = *wait;
    if (left < 1) {
        *wait = -1;
    }
    return left < 1;
}

// CT3 0x018d9510
// The same on the scene clock (it stands while the world is paused).
int ssTrigWaitByAnimFrame(int *wait, int ticks)
{
    if (*wait == -1) {
        *wait = ticks;
    } else {
        *wait -= gNextTicksInterval;
    }
    int left = *wait;
    if (left < 1) {
        *wait = -1;
    }
    return left < 1;
}

// CT3 0x018d8e50
// True with percent chances in a hundred.
int ssTrigCheckRandom(int percent)
{
    return tnRand(100) < percent;
}

// CT3 0x018d89e0
// A drawn bank whose name starts with the four characters of nameWord (compared as one word).
int ssTrigCheckSceneDraw(int nameWord)
{
    for (int i = 0; i < BANK_MAX; i++) {
        if (BankInfo[i].drawOn == 0) {
            continue;
        }
        int word;
        memcpy(&word, BankInfo[i].name, sizeof(word));
        if (word == nameWord) {
            return 1;
        }
        i += BankInfo[i].groupCount - 1;
    }
    return 0;
}

// ---- a detector's conditions (slice 4, 2026-11-11) ------------------------------------------------------

// CT3 0x018d9070
// Whether one of the action's KEY_MAP buttons is set in pad 1's word of the mode (TRIG_INPUT_*).
int ssTrigCheckInput(int action, int mode)
{
    u_int pad;
    switch ((char)mode) {
    case TRIG_INPUT_KEY:
        pad = pad_key_button[0];
        break;
    case TRIG_INPUT_TRG:
        pad = pad_trg_button[0];
        break;
    case TRIG_INPUT_CNT:
        pad = pad_cnt_button[0];
        break;
    default:
        return 0;
    }
    return (pad & KEY_MAP[action]) != 0;
}

// CT3 0x018d8ec0
// A variable (global or the bank's temp set) compared with value by cmp (TRIG_CMP_*); 0 for another cmp.
int ssTrigCheckVariable(int var, int value, int global, int cmp, int bank)
{
    int v;
    if (cmp < TRIG_CMP_EQ || cmp > TRIG_CMP_GT) {
        return 0;
    }
    if (global == TRIG_SET_TEMP) {
        if (TempVariable[bank] == NULL) {
            return 0;               // PORT: no trigger data for the bank yet (Trigger_init)
        }
        v = TempVariable[bank][var];
    } else {
        v = GlobalVariable[var];
    }
    switch (cmp) {
    case TRIG_CMP_GT:
        return v > value;
    case TRIG_CMP_LT:
        return v < value;
    case TRIG_CMP_GE:
        return v >= value;
    case TRIG_CMP_LE:
        return v <= value;
    default:
        return v == value;
    }
}

// CT3 0x018d8cb0
// Whether a character faces the target within the view angle (degrees, the whole cone): the target is a
// Character, or with TRIG_COND_LOOK_PART a model part of the bank. Wraps the character's rot back into
// [0, 0x10000) by one turn as it reads it (the ROM writes it back).
// callcheck: dpmul, dpdiv, dptoli (the EE's software double: the host's double arithmetic)
int ssTrigLookAtCheck(int isPart, int chara, int angle, int target, int bank)
{
    CHARACTER *ch = &Character[chara];
    alignas(16) float from[4], to[4], d[4];
    int dir, diff, half;
    if (isPart == 0) {
        memcpy(to, &Character[target].pos, sizeof(to));
    } else {
        memcpy(to, ((const MODEL_PARTS *)pModelPartsHeader[bank])[target].pos, sizeof(to));
    }
    memcpy(from, &ch->pos, sizeof(from));
    for (int i = 0; i < 4; i++) {
        d[i] = to[i] - from[i];
    }
    dir = (int)((ssRadianByO(d, 1) * 65536.0) / 6.283185307179586);
    if (ch->rot < 0) {
        ch->rot += 0x10000;
    } else if (ch->rot > 0xffff) {
        ch->rot -= 0x10000;
    }
    diff = (0x10000 - (dir & 0xffff)) - ch->rot;
    half = (angle * 0x10000) / TRIG_LOOK_FULL_DEG;
    if (diff < 0) {
        diff = -diff;
    }
    return diff < half || diff > 0x10000 - half;
}

// The hit lists ssTrigAttackHitCheck walks in ALL_TABLE.hits, by the bank's type (names from this reader,
// 2026-11-11; the collision TU that writes them confirms). Each list is an int count, then the EE word of
// its records (the bank's); a record holds an EE word of the attacking object, whose short at idOff is
// the attack id, and the attacker's character and an is-object byte. layout: host (a reading table, no
// ROM data).
typedef struct HIT_LIST_FORM {
    u_short countOff;                   /* in the hits table; the records' EE word follows */
    u_short recSize;
    u_short objOff;                     /* the record's EE word of the object */
    u_short idOff;                      /* the object's short */
    u_short whoOff;                     /* the record's short: the attacking Character */
    u_short isObjectOff;                /* the record's byte: 0 = a character's blow */
} HIT_LIST_FORM;
#define HIT_LISTS 2
static const HIT_LIST_FORM HitListsScene[HIT_LISTS] = {    /* FS_LOAD_SCENE */
    {0x2b0, 0x20, 0x10, 0xda, 0x1c, 0x1e},
    {0x2b8, 0x20, 0x10, LOS_HIT_ID, 0x14, 0x18},
};
static const HIT_LIST_FORM HitListsShape[HIT_LISTS] = {    /* FS_LOAD_SHAPE: the second is HIT_REC */
    {0x1c, 0x20, 0x10, 0xda, 0x1c, 0x1e},
    {offsetof(HIT_TABLE, count), sizeof(HIT_REC), offsetof(HIT_REC, attacker), 0x1a0 /* id, before HIT_ATTACKER_KIND */,
     offsetof(HIT_REC, who), offsetof(HIT_REC, isObject)},
};

// PORT: an attacking object's EE word may point into any bank: the bank whose EE image starts nearest
// below it (BankOfHostPtr's rule, EE side). -1 (matches no u16 id) when no bank holds it.
static int HitObjectId(u_int ee, int idOff)
{
    int bank, best = -1;
    short id;
    if (ee == 0) {
        return -1;
    }
    for (bank = 0; bank < BANK_MAX; bank++) {
        if (BankImage[bank] != NULL && BankImageEE[bank] != 0 && BankImageEE[bank] <= ee &&
            (best < 0 || BankImageEE[bank] > BankImageEE[best])) {
            best = bank;
        }
    }
    if (best < 0) {
        return -1;
    }
    memcpy(&id, BANK_PTR(const u_char *, best, ee) + idOff, sizeof(id));
    return id;
}

// CT3 0x018d8a50
// Whether this frame's hits of the bank (a scene or shape bank) hold an object with the attack id, struck
// by character `who` (TRIG_HIT_ANY_CHARA: by anything, an object's blow too). The ROM compares the u16 id
// with the object's sign-extended short: ids from 0x8000 never match; kept.
int ssTrigAttackHitCheck(u_int attackId, u_int who, int bank)
{
    const HIT_LIST_FORM *forms;
    const u_char *hits;
    attackId &= 0xffff;
    who &= 0xffff;
    if (BankInfo[bank].type == FS_LOAD_SCENE) {
        forms = HitListsScene;
    } else if (BankInfo[bank].type == FS_LOAD_SHAPE) {
        forms = HitListsShape;
    } else {
        return 0;
    }
    if (All_table_TBL[bank] == NULL) {
        return 0;                   // PORT: the bank's table is not loaded
    }
    hits = BANK_PTR(const u_char *, bank, All_table_TBL[bank]->hits);
    if (hits == NULL) {
        return 0;
    }
    for (int l = 0; l < HIT_LISTS; l++) {
        const HIT_LIST_FORM *f = &forms[l];
        int count;
        u_int recsEE;
        const u_char *recs;
        memcpy(&count, hits + f->countOff, sizeof(count));
        memcpy(&recsEE, hits + f->countOff + 4, sizeof(recsEE));
        recs = BANK_PTR(const u_char *, bank, recsEE);
        for (int i = 0; i < count; i++) {
            const u_char *rec = recs + i * f->recSize;
            u_int objEE;
            short recWho;
            memcpy(&objEE, rec + f->objOff, sizeof(objEE));
            if ((int)attackId != HitObjectId(objEE, f->idOff)) {
                continue;
            }
            if (who == TRIG_HIT_ANY_CHARA) {
                return 1;
            }
            memcpy(&recWho, rec + f->whoOff, sizeof(recWho));
            if (rec[f->isObjectOff] == 0 && (int)who == recWho) {
                return 1;
            }
        }
    }
    return 0;
}

// The list kinds' entry sizes and any bits (0: not a list kind).
static int CondEntrySize(int kind)
{
    switch (kind) {
    case TRIG_COND_EVENT:
        return 3;
    case TRIG_COND_ENABLE:
    case TRIG_COND_ATTACK_HIT:
        return 2;
    case TRIG_COND_INPUT:
        return 1;
    case TRIG_COND_SCENE_DRAW:
        return 4;
    default:
        return 0;
    }
}

static int CondAnyBit(int kind)
{
    return kind == TRIG_COND_EVENT || kind == TRIG_COND_INPUT ? TRIG_COND_ANY2 : TRIG_COND_ANY;
}

// One entry of a list condition, read and checked (0 or 1).
static int CondEntry(int kind, int *pos, const u_char *data, int flags, int who, int bank)
{
    switch (kind) {
    case TRIG_COND_EVENT: {
        int event = ssTrigRead2Byte(pos, data);
        int global = ssTrigRead1Byte(pos, data);
        return ssTrigCheckEvent(event, global, bank) != 0;
    }
    case TRIG_COND_ENABLE:
        return ssTrigCheckEnable(ssTrigRead2Byte(pos, data), bank) != 0;
    case TRIG_COND_INPUT:
        return ssTrigCheckInput(ssTrigRead1Byte(pos, data), flags & TRIG_INPUT_MASK) != 0;
    case TRIG_COND_ATTACK_HIT:
        return ssTrigAttackHitCheck(ssTrigRead2Byte(pos, data), (u_int)who, bank) != 0;
    default:
        return ssTrigCheckSceneDraw((int)ssTrigRead4Byte(pos, data)) != 0;
    }
}

// CT3 0x018d7b70
// The detector's conditions (the format above TRIG_COND_EVENT in the header); 1 when it has none.
// callcheck: ssTrigCheckEvent, ssTrigCheckEnable, ssTrigCheckInput, ssTrigAttackHitCheck, ssTrigCheckSceneDraw (the list kinds' entries, in CondEntry)
int ssTrigCheckConditions(TRIG_DET_WORK *det, int bank)
{
    const u_char *data = DetData[bank];
    int *pos = &det->cursor;
    int result = 0;
    int skipping = 0;
    if (det->numConds == 0) {
        return 1;
    }
    *pos = (int)det->condsPos + 1;
    for (;;) {
        int kind = (signed char)ssTrigRead1Byte(pos, data);
        int size = CondEntrySize(kind);
        int negate;
        if (kind == TRIG_COND_END) {
            break;
        }
        if (kind == TRIG_COND_OR) {
            if (!skipping) {
                break;              // the group held
            }
            skipping = 0;
            continue;
        }
        if (size != 0) {
            // a list kind: negate, flags, count[, who], entries
            int flags, count, who = 0, i;
            if (skipping) {
                *pos += 2;
                count = ssTrigRead1Byte(pos, data);
                if (kind == TRIG_COND_ATTACK_HIT) {
                    *pos += 2;
                }
                *pos += count * size;
                continue;
            }
            negate = ssTrigRead1Byte(pos, data) & TRIG_COND_NEGATE;
            flags = ssTrigRead1Byte(pos, data);
            count = ssTrigRead1Byte(pos, data);
            if (kind == TRIG_COND_ATTACK_HIT) {
                who = ssTrigRead2Byte(pos, data);
            }
            for (i = 0; i < count; i++) {
                result = CondEntry(kind, pos, data, flags, who, bank) ^ negate;
                if (result == 0) {
                    if ((flags & CondAnyBit(kind)) == 0) {
                        i++;
                        break;      // one failed: all were asked
                    }
                } else if ((flags & CondAnyBit(kind)) != 0) {
                    i++;
                    *pos += (count - i) * size;
                    break;          // one held: any was asked
                }
            }
            if (result == 0) {
                skipping = 1;
                *pos += (count - i) * size;
            }
            continue;
        }
        switch (kind) {
        case TRIG_COND_ANIM_FRAME: {
            int flags, target, part, chara;
            u_long hash;
            u_int frame;
            if (skipping) {
                *pos += 0x11;
                continue;
            }
            negate = ssTrigRead1Byte(pos, data) & TRIG_COND_NEGATE;
            flags = ssTrigRead1Byte(pos, data);
            target = (flags & TRIG_COND_ANIM_TARGET1) != 0 ? 1 : 2;
            hash = ssTrigRead8Byte(pos, data);
            frame = ssTrigRead4Byte(pos, data);
            part = ssTrigRead2Byte(pos, data);
            chara = ssTrigRead1Byte(pos, data);
            result = (asCheckAnimationFlamePassing_S(hash, All_table_TBL[bank], frame, part,
                                                     flags & PASS_DIR_EITHER, target, chara) != 0) ^ negate;
            break;
        }
        case TRIG_COND_VARIABLE: {
            int var, value, global, cmp;
            if (skipping) {
                *pos += 9;
                continue;
            }
            negate = ssTrigRead1Byte(pos, data) & TRIG_COND_NEGATE;
            var = ssTrigRead2Byte(pos, data);
            value = (int)ssTrigRead4Byte(pos, data);
            global = ssTrigRead1Byte(pos, data);
            cmp = ssTrigRead1Byte(pos, data);
            result = (ssTrigCheckVariable(var, value, global, cmp, bank) != 0) ^ negate;
            break;
        }
        case TRIG_COND_LOOK_AT: {
            int isPart, chara, angle, target;
            if (skipping) {
                *pos += 7;
                continue;
            }
            negate = ssTrigRead1Byte(pos, data) & TRIG_COND_NEGATE;
            isPart = ssTrigRead1Byte(pos, data) & TRIG_COND_LOOK_PART;
            chara = ssTrigRead1Byte(pos, data);
            angle = ssTrigRead2Byte(pos, data);
            target = ssTrigRead2Byte(pos, data);
            result = (ssTrigLookAtCheck(isPart, chara, angle, target, bank) != 0) ^ negate;
            break;
        }
        case TRIG_COND_INPUT_ENABLE: {
            int mask;
            if (skipping) {
                *pos += 2;
                continue;
            }
            negate = ssTrigRead1Byte(pos, data) & TRIG_COND_NEGATE;
            mask = ssTrigRead1Byte(pos, data);
            result = (ssTrigCheckInputEnable((u_int)mask) != 0) ^ negate;
            break;
        }
        default:
            continue;               // the ROM reads on past an unknown kind byte
        }
        if (result == 0) {
            skipping = 1;
        }
    }
    return result;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018d5450
int ssTrigExecuteActCmd(TRIG_ACT_CMDS *cmds, int bank)
{
    (void)cmds;
    (void)bank;
    ALYSSAPAN_STUB("ssTrigExecuteActCmd", 0x018d5450);
    return 1;
}
