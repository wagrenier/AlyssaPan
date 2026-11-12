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
// ssTrigSetEnable and Trigger_reset; slice 5 the actor commands (ssTrigExecuteActCmd, ssXcmd_functions,
// suXcmd_AnmDirection).

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "common/alyssapan_stub.h"
#include "game/ssGrpPortalCheck.h"
#include "ga/gaInit3dlib.h"
#include "game/ahalloc.h"
#include "game/Character_Init.h"
#include "game/BRANCH_CLEAR.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/sound.h"
#include "game/ssSetTexUVforCubeMap.h"
#include "game/fileio.h"
#include "game/unassigned_data.h"
#include "game/LinkMatrixTree.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "game/movie.h"
#include "game/Clipping_Portal.h"
#include "game/xcmd_tn.h"
#include "game/wideXcmd_TriggerInit.h"
#include "game/tiXcmd_TransItem.h"
#include "game/tnXcmd.h"
#include "game/PicPaz.h"
#include "game/RotateCamera.h"
#include "game/tkXDTrg_GetSceneHash.h"
#include "game/asXcmd.h"
#include "game/hhXcmd_DressUp.h"
#include "game/hhXcmd_BGM_SE_VOLUME.h"

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
        if (BankImage[bank] != NULL && BankInfo[bank].state != BANK_STATE_FREE && BankImageEE[bank] != 0 &&
            BankImageEE[bank] <= ee &&
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

// ---- the actor commands (slice 5, 2026-11-11) -------------------------------------------------------

// ssXcmd_functions (data 0x0193a500, 256 words): TRIG_CMD_XCMD's commands by index byte. The symtab gives
// no object for the table; its one reader is ssTrigExecuteActCmd, so it lives here. Three slots are written
// with their own prototypes (cast: CodeWarrior passes argc / argv / bank in a0..a2 whatever the callee
// declares).
SS_XCMD ssXcmd_functions[TRIG_XCMD_COUNT] = {
    ssXcmd_SetHideAttackPole,                               /*   0: 0x018de410 */
    wideXcmd_TriggerInit,                                   /*   1: 0x018dd550 */
    ssXcmd_test3,                                           /*   2: 0x018e0e30 */
    tkXcmd_CharSceneChange,                                 /*   3: 0x018e8d20 */
    ssXcmd_ArrowSetStartPos,                                /*   4: 0x018e0420 */
    tnXcmd_Probability,                                     /*   5: 0x018f3eb0 */
    tnXcmd_PlayerDamage,                                    /*   6: 0x018f3e80 */
    tnXcmd_PlayerControllStart,                             /*   7: 0x018f3e60 */
    tnXcmd_PlayerControllStop,                              /*   8: 0x018f3e40 */
    tnXcmd_MonsterControllStart,                            /*   9: 0x018f3e20 */
    tnXcmd_MonsterControllStop,                             /*  10: 0x018f3e00 */
    tnXcmd_LeverClattering,                                 /*  11: 0x018f3cb0 */
    tnXcmd_LeverClatteringInit,                             /*  12: 0x018f3dd0 */
    PicPaz,                                                 /*  13: 0x018da090 */
    LockerCamera,                                           /*  14: 0x018d9b50 */
    tiXcmd___________Free0,                                 /*  15: 0x018e20b0 */
    RotateCamera,                                           /*  16: 0x018d9a40 */
    tkXcmd_AiSoundPosition,                                 /*  17: 0x018ee5e0 */
    ssXcmd_ScnAtkRequest,                                   /*  18: 0x018e0c10 */
    ssXcmd_SetDurabilityToVar,                              /*  19: 0x018e0b40 */
    tiXcmd_SetRotation,                                     /*  20: 0x018e89d0 */
    tiXcmd_SetScale,                                        /*  21: 0x018e8930 */
    ssXcmd_SetArrowPath,                                    /*  22: 0x018e0670 */
    tiXcmd_VibrationON_OFF,                                 /*  23: 0x018e7810 */
    tiXcmd_VibrationCheckFlg,                               /*  24: 0x018e77b0 */
    tiXcmd_VibrationStart,                                  /*  25: 0x018e75e0 */
    tiXcmd_VibrationEnd,                                    /*  26: 0x018e75b0 */
    tiXcmd___________Free1,                                 /*  27: 0x018e2020 */
    tiXcmd___________Free2,                                 /*  28: 0x018e1fc0 */
    ssXcmd_LockToTargChara,                                 /*  29: 0x018dfd70 */
    ssXcmd_SetArrowPath2,                                   /*  30: 0x018e0590 */
    tiXcmd_StereoON_OFF,                                    /*  31: 0x018e75a0 */
    tiXcmd_StereoCheckFlg,                                  /*  32: 0x018e7590 */
    tiXcmd_SetParaDefault,                                  /*  33: 0x018e7580 */
    ssXcmd_GetAtkHitInfoID,                                 /*  34: 0x018dfb80 */
    ssXcmd_CheckAtkHitMaterial,                             /*  35: 0x018dfa10 */
    tiXcmd_CheckMCFile,                                     /*  36: 0x018e71f0 */
    tiXcmd_CopyVariable,                                    /*  37: 0x018e88a0 */
    tiXcmd_SetReset,                                        /*  38: 0x018e8890 */
    tiXcmd_GetMCflag,                                       /*  39: 0x018e7130 */
    tiXcmd_SetmcMain_ID,                                    /*  40: 0x018e70d0 */
    tiXcmd_SetmcSlot_ID,                                    /*  41: 0x018e7070 */
    tiXcmd_SetmcFile_ID,                                    /*  42: 0x018e7010 */
    tiXcmd_SetmcErr_ID,                                     /*  43: 0x018e6fb0 */
    tiXcmd_SetmcAccess_ID,                                  /*  44: 0x018e6f50 */
    tiXcmd___________Free3,                                 /*  45: 0x018e1fb0 */
    tiXcmd_SetPlayerChara,                                  /*  46: 0x018e7d10 */
    tiXcmd_GetItem,                                         /*  47: 0x018e6bf0 */
    tiXcmd_UseItem,                                         /*  48: 0x018e6330 */
    tiXcmd_GetIDfromList,                                   /*  49: 0x018e6160 */
    tiXcmd_SetStatusBar,                                    /*  50: 0x018e57b0 */
    tiXcmd_SceneAnimationSkipToVariable,                    /*  51: 0x018e86d0 */
    tiXcmd_AddVariable,                                     /*  52: 0x018e8640 */
    tiXcmd_SubVariable,                                     /*  53: 0x018e85b0 */
    tiXcmd_GetItemData,                                     /*  54: 0x018e5ce0 */
    tiXcmd_SetItemData,                                     /*  55: 0x018e5a50 */
    ssXcmd_CheckAtkHitChara,                                /*  56: 0x018df8a0 */
    ssXcmd_ArrowSetChild,                                   /*  57: 0x018e01d0 */
    tiXcmd_GetItemListLastIndex,                            /*  58: 0x018e5930 */
    tiXcmd_CopySunPs2Object,                                /*  59: 0x018e83d0 */
    tiXcmd_CompareVariable,                                 /*  60: 0x018e8260 */
    tiXcmd_MoveItem,                                        /*  61: 0x018e5870 */
    ssXcmd_ArrowSetHitPos,                                  /*  62: 0x018e0a90 */
    tnXcmd_TexFileSetPage,                                  /*  63: 0x018f3c30 */
    tnXcmd_TexFileSetNext,                                  /*  64: 0x018f3bd0 */
    ssXcmd_ObjectSetHitPos,                                 /*  65: 0x018e09c0 */
    tnCurrentSceneSet,                                      /*  66: 0x018f3ba0 */
    wideXcmd_CinemaTheaterSet,                              /*  67: 0x018eeb70 */
    wideXcmd_TitleStart,                                    /*  68: 0x018eeb40 */
    ssXcmd_GetCharFootEndMaterial,                          /*  69: 0x018df610 */
    tiXcmd_SetKeyToKeyMap,                                  /*  70: 0x018e5590 */
    tiXcmd_GetKeyFromKeyMap,                                /*  71: 0x018e54f0 */
    tiXcmd_ResetKeyMap,                                     /*  72: 0x018e54c0 */
    tkXcmd_SceneDraw,                                       /*  73: 0x018ee5b0 */
    suXcmd_AnmDirection,                                    /*  74: 0x018d99c0 */
    ssXcmd_GetAttackMotionID,                               /*  75: 0x018e08a0 */
    ssXcmd_SetArwRotMaxID,                                  /*  76: 0x018e0170 */
    tiXcmd_ItemCheckInit,                                   /*  77: 0x018e1550 */
    tiXcmd_TransItem,                                       /*  78: 0x018e0e80 */
    tiXcmd_SetSceneID,                                      /*  79: 0x018e1610 */
    tiXcmd_CheckLife,                                       /*  80: 0x018e5700 */
    tnXcmd_VistaVision,                                     /*  81: 0x018f37e0 */
    tiXcmd_XFadeOut2Start,                                  /*  82: 0x018e4c00 */
    tiXcmd_SetDrawMode_Global,                              /*  83: 0x018e8210 */
    tiXcmd_GetAlyssaLife,                                   /*  84: 0x018e56a0 */
    tiXcmd_OperateItem,                                     /*  85: 0x018e0fa0 */
    tiXcmd___________Free4,                                 /*  86: 0x018e1fa0 */
    tiXcmd___________Free5,                                 /*  87: 0x018e1f90 */
    tiXcmd___________Free6,                                 /*  88: 0x018e1f80 */
    tkXcmd_CaptionFlag,                                     /*  89: 0x018e8bc0 */
    tiXcmd_SetPointForGhostArea,                            /*  90: 0x018e1ed0 */
    aaXcmd_MovieFedeOutCancel,                              /*  91: 0x018da0a0 */
    tiXcmd_Balows2th,                                       /*  92: 0x018e1bc0 */
    tnXcmd_SetAllControlDisableCamera,                      /*  93: 0x018ee9c0 */
    tiXcmd_TEST00,                                          /*  94: 0x018e8b70 */
    tnXcmd_Character_Ambient_Change,                        /*  95: 0x018f36b0 */
    test_ssXcmd_ChangeGroupID,                              /*  96: 0x018df600 */
    tkXcmd_WaitUntil,                                       /*  97: 0x018ee5a0 */
    tiXcmd_SetMapData,                                      /*  98: 0x018e1810 */
    wideXcmd_VariableCal,                                   /*  99: 0x018f34a0 */
    wideXcmd_PlayAnimeRange,                                /* 100: 0x018f3280 */
    wideXcmd_PlayAnimeToFream,                              /* 101: 0x018f30e0 */
    tnXcmd_1P01_PazzleInit,                                 /* 102: 0x018f2fa0 */
    tnXcmd_1P01_PazzleAnim,                                 /* 103: 0x018f2c50 */
    tiXcmd_CompareVar2,                                     /* 104: 0x018e7eb0 */
    tiXcmd_CheckCurrentScene,                               /* 105: 0x018e7e20 */
    wideXcmd_PlayAnimeToFream2,                             /* 106: 0x018f28f0 */
    asXcmd_ShadowOrderBW,                                   /* 107: 0x018dc1b0 */
    asXcmd_PlayMotionByNowMotion,                           /* 108: 0x018dc050 */
    test_ssXcmd_BrainSet,                                   /* 109: 0x018df5f0 */
    tnXcmd_ShapeFree,                                       /* 110: 0x018f2830 */
    tnXcmd_BehaviorFree,                                    /* 111: 0x018f2720 */
    tiXcmd_SetCharacterEdgeShading,                         /* 112: 0x018e4b40 */
    tnXcmd_SetFarClip,                                      /* 113: 0x018f26f0 */
    tnXcmd_SetNearClip,                                     /* 114: 0x018f26c0 */
    tnXcmd_SetPanic,                                        /* 115: 0x018f25c0 */
    tiXcmd_LightBeltStart,                                  /* 116: 0x018e4b30 */
    tiXcmd_LightBeltEnd,                                    /* 117: 0x018e4b20 */
    tnXcmd_SetMovieWorkBuff,                                /* 118: 0x018f2430 */
    asXcmd_CameraGuriGuriSetting,                           /* 119: 0x018dbf40 */
    test_ssXcmd_SetGroupID,                                 /* 120: 0x018dee50 */
    tnXcmd_SceneFogAnimation,                               /* 121: 0x018f2240 */
    tnXcmd_CharFogAnimation,                                /* 122: 0x018f20b0 */
    test_ssXcmd_BrainSet2,                                  /* 123: 0x018df5e0 */
    test_ssXcmd_BrainSet3,                                  /* 124: 0x018df5d0 */
    tiXcmd_TranslucenceAlphaAnim,                           /* 125: 0x018e4a20 */
    tiXcmd_HurikoReset,                                     /* 126: 0x018e4990 */
    tnXcmd_LoadWait,                                        /* 127: 0x018f1ff0 */
    test_ssXcmd_BrainEnd,                                   /* 128: 0x018deed0 */
    tnXcmd_BankFree,                                        /* 129: 0x018f1e20 */
    asXcmd_SeekBarSetting,                                  /* 130: 0x018dbd80 */
    test_ssXcmd_BrainSet4,                                  /* 131: 0x018def30 */
    tnXcmd_ScneName,                                        /* 132: 0x018f1da0 */
    tnXcmd_TimeSet,                                         /* 133: 0x018f1a10 */
    ssXcmd_GetChaseAIState,                                 /* 134: 0x018ded70 */
    ssXcmd_SetChaseFlagAndTimer,                            /* 135: 0x018dec60 */
    ssXcmd_SetAppearanceTimer,                              /* 136: 0x018dec30 */
    tkXcmd_DoorTrigger,                                     /* 137: 0x018ed9e0 */
    tnXcmd_Scene_Ambient_Change,                            /* 138: 0x018f1940 */
    tnXcmd_Char_DirectLight_Change,                         /* 139: 0x018f1880 */
    tnXcmd_Filter_Change,                                   /* 140: 0x018f1710 */
    wideXcmd_Variable_Copy,                                 /* 141: 0x018f1680 */
    tnXcmd_Menu_MapLoad,                                    /* 142: 0x018f1570 */
    tnXcmd_Menu_MapOff,                                     /* 143: 0x018f1510 */
    tnXcmd_Room_IN_Check,                                   /* 144: 0x018f1480 */
    tnXcmd_Room_IN_ALYSSA,                                  /* 145: 0x018f1340 */
    tnXcmd_Key_Lock,                                        /* 146: 0x018efc50 */
    tiXcmd_KeyNumToFrame,                                   /* 147: 0x018e5360 */
    hhXcmd_LoadSE,                                          /* 148: 0x018dd4d0 */
    hhXcmd_PurgeSE,                                         /* 149: 0x018dd490 */
    hhXcmd_PlaySE,                                          /* 150: 0x018dd200 */
    hhXcmd_VolumeChangeSE,                                  /* 151: 0x018dcba0 */
    hhXcmd_PitchChangeSE,                                   /* 152: 0x018dcaf0 */
    hhXcmd_PanChangeSE,                                     /* 153: 0x018dca40 */
    hhXcmd_StopSE,                                          /* 154: 0x018dc9e0 */
    hhXcmd_EffectSE,                                        /* 155: 0x018dc930 */
    hhXcmd_PlayADS,                                         /* 156: 0x018dc890 */
    hhXcmd_StopADS,                                         /* 157: 0x018dc7e0 */
    hhXcmd_VolumeADS,                                       /* 158: 0x018dc810 */
    tnXcmd_SetRoodersPts,                                   /* 159: 0x018ee5f0 */
    wideXcmd_ReadChengeMsg,                                 /* 160: 0x018efb80 */
    tiXcmd_GetLetterData,                                   /* 161: 0x018e5120 */
    tiXcmd_FadeStart,                                       /* 162: 0x018e47b0 */
    aaXcmd_SceneEndBank,                                    /* 163: 0x018da900 */
    tiXcmd_BOSS_BattleStart,                                /* 164: 0x018e3460 */
    tiXcmd_BOSS_BattleEnd,                                  /* 165: 0x018e2e70 */
    tiXcmd_CharaEdgeShadeAnim,                              /* 166: 0x018e43d0 */
    hhXcmd_StopSESp1,                                       /* 167: 0x018dc5e0 */
    hhXcmd_PlaySESp1,                                       /* 168: 0x018dc640 */
    hhXcmd_PlayMIDI,                                        /* 169: 0x018dc500 */
    hhXcmd_StopMIDI,                                        /* 170: 0x018dc420 */
    hhXcmd_VolumeMIDI,                                      /* 171: 0x018dc3a0 */
    hhXcmd___________Free3,                                 /* 172: 0x018dc390 */
    hhXcmd___________Free4,                                 /* 173: 0x018dc380 */
    tiXcmd_GetINDEX_FromLetterList,                         /* 174: 0x018e5030 */
    ssXcmd_SetAllControlDisable,                            /* 175: 0x018de9d0 */
    asXcmd_CharacterAutoAiming,                             /* 176: 0x018dbc20 */
    tnXcmd_StayRoom,                                        /* 177: 0x018efa90 */
    asXcmd_I_have_x_Arrows,                                 /* 178: 0x018dba30 */
    asXcmd_ShootArrow,                                      /* 179: 0x018dba20 */
    ssXcmd_SetNoReactionTimer,                              /* 180: 0x018de9a0 */
    ssXcmd_StayGroup,                                       /* 181: 0x018de890 */
    asXcmd_AttachObject,                                    /* 182: 0x018db880 */
    asXcmd_CameraQuake,                                     /* 183: 0x018db7b0 */
    tnXcmd_OmniRange,                                       /* 184: 0x018ef810 */
    tnXcmd_CharOmniEffect,                                  /* 185: 0x018ef670 */
    tnXcmd_AllOmniEffect,                                   /* 186: 0x018ef550 */
    tnXcmd_EndPanic,                                        /* 187: 0x018ef3d0 */
    tnXcmd_Continue,                                        /* 188: 0x018ef320 */
    hhXcmd___________Free5,                                 /* 189: 0x018dc370 */
    hhXcmd_DressUp,                                         /* 190: 0x018dc330 */
    hhXcmd_BGM_SE_VOLUME,                                   /* 191: 0x018dc1e0 */
    aaXcmd_McWorkAlloc,                                     /* 192: 0x018da210 */
    ssXcmd_IsMonAI_SearchMode,                              /* 193: 0x018de7e0 */
    asXcmd_SlowMotion,                                      /* 194: 0x018db770 */
    tnXcmd_TimeStop,                                        /* 195: 0x018ef1c0 */
    tiXcmd_BOSS_SetChain,                                   /* 196: 0x018e31d0 */
    asXcmd_SetHitPoint,                                     /* 197: 0x018db580 */
    tiXcmd_XFade_Break,                                     /* 198: 0x018e4390 */
    tiXcmd_ChangeCam_Reserve,                               /* 199: 0x018e4c90 */
    tiXcmd_ChangeCam_Execute,                               /* 200: 0x018e4c50 */
    asXcmd_SettingCameraReset,                              /* 201: 0x018db510 */
    wideXcmd_GetNumberOfBeam,                               /* 202: 0x018e7d60 */
    ssXcmd_SetMonReturnTimer,                               /* 203: 0x018de670 */
    tiXcmd_CheckEarthquake,                                 /* 204: 0x018e2d10 */
    ssXcmd_AXE_InitParam,                                   /* 205: 0x018de260 */
    ssXcmd_AXE_SetStartPos,                                 /* 206: 0x018de100 */
    ssXcmd_AXE_SetPath,                                     /* 207: 0x018ddcd0 */
    wideXcmd_Delay,                                         /* 208: 0x018e8f00 */
    ssXcmd_AXE_CheckHitAndCompDir,                          /* 209: 0x018dd760 */
    tiXcmd_BOSS_SetWarp,                                    /* 210: 0x018e2e90 */
    tiXcmd_ShadeOff,                                        /* 211: 0x018e41f0 */
    tnXcmd_TexFileInitPageStop,                             /* 212: 0x018ef1b0 */
    asXcmd_AddForLockToTargChara,                           /* 213: 0x018db340 */
    ssXcmd_AXE_ThrowEnd,                                    /* 214: 0x018dd6b0 */
    ssXcmd_ForceIntoSearchMode,                             /* 215: 0x018de5e0 */
    asXcmd_I_have_x_Mothes,                                 /* 216: 0x018db160 */
    asXcmd_SetMoth,                                         /* 217: 0x018db000 */
    ssXcmd_MonsterSetDraw,                                  /* 218: 0x018de580 */
    tiXcmd_Init_SPAttack_SCISSORS_MAN,                      /* 219: 0x018e2b30 */
    tiXcmd_End_SPAttack_SCISSORS_MAN,                       /* 220: 0x018e2ae0 */
    tiXcmd_SPAttack_SCISSORS_MAN,                           /* 221: 0x018e22f0 */
    tiXcmd_CheckinLetterList,                               /* 222: 0x018e4cf0 */
    tiXcmd_FreeGhostPole,                                   /* 223: 0x018e7b70 */
    tiXcmd_SetGhostPole,                                    /* 224: 0x018e7bf0 */
    wideXcmd_GlobalSetEnable,                               /* 225: 0x018dd590 */
    tnXcmd_Room_IN_Icon,                                    /* 226: 0x018f13f0 */
    asXcmd_ForceOperateAnimation,                           /* 227: 0x018daf80 */
    tiXcmd_BOSS_SetHitBacks,                                /* 228: 0x018e33d0 */
    wideXcmd_GetRate,                                       /* 229: 0x018e79d0 */
    wideXcmd_GetCharaData,                                  /* 230: 0x018e78c0 */
    wideXcmd_DemoSkipCheck,                                 /* 231: 0x018ef0e0 */
    wideXcmd_GameOver,                                      /* 232: 0x018ef0b0 */
    ssXcmd_CheckAtkHitALYSSA,                               /* 233: 0x018df6c0 */
    wideXcmd_BranchClear,                                   /* 234: 0x018ef090 */
    tiXcmd_WaitUntilBrainFree,                              /* 235: 0x018e7870 */
    (SS_XCMD)wideXcmd_DebugPrint,                           /* 236: 0x018f3f60 */
    ssXcmd_GetMonAImode,                                    /* 237: 0x018de4e0 */
    wideXcmd_GlobalFree,                                    /* 238: 0x018eeff0 */
    tiXcmd_BOSS_SetChain_Alyssa,                            /* 239: 0x018e2f70 */
    asXcmd_ForceOperateMirroring,                           /* 240: 0x018daf30 */
    asXcmd_ALYSSA_ComeHere,                                 /* 241: 0x018dae40 */
    tiXcmd_CheckRange,                                      /* 242: 0x018e2170 */
    wideXcmd_ScreenAdjust,                                  /* 243: 0x018eedf0 */
    wideXcmd_FrameWait,                                     /* 244: 0x018eed50 */
    aaXcmd_CheckMCFile,                                     /* 245: 0x018da530 */
    aaXcmd_MCFileList,                                      /* 246: 0x018da420 */
    wideXcmd_SkipWait,                                      /* 247: 0x018eed40 */
    wideXcmd_MovieWait,                                     /* 248: 0x018eecf0 */
    aaXcmd_McCursorUpDate,                                  /* 249: 0x018da280 */
    asXcmd_DistanceBetweenChar,                             /* 250: 0x018dad50 */
    tiXcmd_GetBloodAttackObject,                            /* 251: 0x018e20c0 */
    ssXcmd_SetAdvantageousTime,                             /* 252: 0x018de4a0 */
    asXcmd_PanicSet,                                        /* 253: 0x018dab60 */
    tnXcmd_AutoLoadInit,                                    /* 254: 0x018eecd0 */
    tnXcmd_AutoLoad,                                        /* 255: 0x018eebb0 */
};

// TRIG_CMD_SHADOW_*'s first byte: a Character[] slot's bank, or this bank (TRIG_SHADOW_THIS_BANK).
static int TrigShadowBank(u_char chara, int bank)
{
    return chara == TRIG_SHADOW_THIS_BANK ? bank : Character[chara].bank;
}

// TRIG_CMD_CAM_*'s times: the stored u16 * 300 / 1000.
static int TrigCamTime(u_short t)
{
    return (int)t * TRIG_TIME_SCALE_NUM / TRIG_TIME_SCALE_DEN;
}

// TRIG_CMD_CAM_CHANGE: make `scene` the live camera bank and hash its 4-letter room name.
static void TrigSetCameraScene(int scene)
{
    char room[16];

    idCameraScene = scene;
    strcpy(room, BankInfo[scene].name);
    room[XCMD_ROOM_NAME_LEN] = '\0';
    haCameraRoom = ahGetHash(room);
    SwapDrawingDoor();
}

// TRIG_CMD_ANIM_RANGE / _PLAY: start the bank's animation, and the same on a second bank (part 0) when one
// is named and loaded.
static void TrigAnimStart(int *pos, const u_char *data, int range, int bank)
{
    u_long hash = ssTrigRead8Byte(pos, data);
    int from = range ? (int)ssTrigRead4Byte(pos, data) : -1;
    int to = (int)ssTrigRead4Byte(pos, data);
    float speed = (float)ssTrigRead1Byte(pos, data) / TRIG_ANGLE_SCALE;
    u_short part = ssTrigRead2Byte(pos, data);
    s_char pose = (s_char)ssTrigRead1Byte(pos, data);
    s_char other = (s_char)ssTrigRead1Byte(pos, data);
    s_char byPart = (s_char)ssTrigRead1Byte(pos, data);
    s_char e4 = (s_char)ssTrigRead1Byte(pos, data);
    s_char e5 = (s_char)ssTrigRead1Byte(pos, data);
    u_char target = ssTrigRead1Byte(pos, data);
    u_char chara = ssTrigRead1Byte(pos, data);
    u_short bank2 = ssTrigRead2Byte(pos, data);
    u_long hash2 = ssTrigRead8Byte(pos, data);
    // RANGE passes e4 as `started` (toEnd 0); PLAY passes it as `toEnd` (started 0): the ROM's stack slots.
    int started = range ? e4 : 0;
    int toEnd = range ? 0 : e4;

    asSceneAnimationStart(speed, hash, All_table_TBL[bank], from, to, part, pose, other, byPart, started,
                          toEnd, e5, target, chara);
    if (bank2 != 0xffff && BankInfo[bank2].state > 0) {
        asSceneAnimationStart(speed, hash2, All_table_TBL[bank2], from, to, 0, pose, other, byPart, started,
                              toEnd, e5, target, chara);
    }
}

// CT3 0x018d5450
// Run an actor's command list from its cursor until a command waits (answers 0: the cursor is stepped back
// over it, so it runs again next frame), TRIG_CMD_RETURN or TRIG_CMD_END. TRIG_CMD_IF opens a block
// (cmds->step counts the depth); a false TRIG_CMD_IF_* test skips to that depth's ELSE / ENDIF: skipped
// commands step over their operands without running (`skip` is the depth being skipped, 0 = running).
// Answers the last command's answer (0 = waiting).
int ssTrigExecuteActCmd(TRIG_ACT_CMDS *cmds, int bank)
{
    const u_char *data = ActData[bank];
    int *pos = &cmds->cursor;
    int answer = 1;
    u_char skip = 0;
    int end = 0;

    do {
        u_short id = ssTrigRead2Byte(pos, data);
        if (id == TRIG_CMD_END) {
            end = 1;
            continue;
        }
        switch (id) {
        case TRIG_CMD_CHARA_DRAW:
            if (skip == 0) {
                int on = ssTrigRead1Byte(pos, data) - 1;
                TrgCharacterDraw(on, ssTrigRead1Byte(pos, data));
            } else {
                *pos += 2;
            }
            break;
        case TRIG_CMD_CHARA_BEHAVIOR_SET:
            if (skip == 0) {
                char *name = (char *)data + *pos;
                *pos += TRIG_CMD_NAME_LEN;
                u_char chara = ssTrigRead1Byte(pos, data);
                TrgCharacterBehaviorSet(chara, name, ssTrigRead2Byte(pos, data));
            } else {
                *pos += 8;
            }
            break;
        case TRIG_CMD_CHARA_SHAPE_SET:
            if (skip == 0) {
                char *name = (char *)data + *pos;
                *pos += TRIG_CMD_NAME_LEN;
                u_char chara = ssTrigRead1Byte(pos, data);
                u_short shapeBank = ssTrigRead2Byte(pos, data);
                u_char initA = ssTrigRead1Byte(pos, data);
                TrgCharacterShapeSet(chara, name, shapeBank, initA, ssTrigRead2Byte(pos, data));
            } else {
                *pos += 11;
            }
            break;
        case TRIG_CMD_CHARA_BEHAVIOR_LOAD:
        case TRIG_CMD_CHARA_SHAPE_LOAD:
            if (skip == 0) {
                char *name = (char *)data + *pos;
                *pos += TRIG_CMD_NAME_LEN;
                u_char chara = ssTrigRead1Byte(pos, data);
                u_short loadBank = ssTrigRead2Byte(pos, data);
                u_short arg = ssTrigRead2Byte(pos, data);
                if (id == TRIG_CMD_CHARA_BEHAVIOR_LOAD) {
                    TrgCharacterBehaviorLoad(chara, name, loadBank, arg);
                } else {
                    TrgCharacterShapeLoad(chara, name, loadBank, arg);
                }
            } else {
                *pos += 10;
            }
            break;
        case TRIG_CMD_MOVIE_PLAY:
            if (skip == 0) {
                AA_PlayMovieReady();
            }
            break;
        case TRIG_CMD_MOVIE_OPEN: {
            u_char movieLen = ssTrigRead1Byte(pos, data);
            u_char captionLen = ssTrigRead1Byte(pos, data);
            if (skip == 0) {
                const char *movie = (const char *)data + *pos;
                *pos += movieLen;
                const char *caption = (const char *)data + *pos;
                *pos += captionLen;
                PanicStopMovie(movie);
                AA_OpenMovie(movie, caption, ssTrigRead2Byte(pos, data));
            } else {
                *pos += movieLen + captionLen + 2;
            }
            break;
        }
        case TRIG_CMD_NOP_1601:
        case TRIG_CMD_NOP_1600:
            break;
        case TRIG_CMD_SHADOW_TRANS_MODE:
            if (skip == 0) {
                asShadowDrawing_SetTransMode((char)ssTrigRead1Byte(pos, data));
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_SHADOW_DETACH:
            if (skip == 0) {
                asShadowDrawing_DetachLightPosition(TrigShadowBank(ssTrigRead1Byte(pos, data), bank));
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_SHADOW_DISABLE:
            if (skip == 0) {
                asShadowDrawing_Disable(TrigShadowBank(ssTrigRead1Byte(pos, data), bank));
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_SHADOW_ATTACH:
            if (skip == 0) {
                int shadowBank = TrigShadowBank(ssTrigRead1Byte(pos, data), bank);
                asShadowDrawing_AttachLightPosition(shadowBank, All_table_TBL[bank], ssTrigRead2Byte(pos, data));
                asShadowDrawing_SetShadowLevel(shadowBank, ssTrigRead1Byte(pos, data));
                asShadowDrawing_SetShadowValue(shadowBank, (s_char)ssTrigRead1Byte(pos, data));
                asShadowDrawing_SetShadowAngleLock(shadowBank, ssTrigRead1Byte(pos, data));
                float lock64 = (float)ssTrigRead2Byte(pos, data);
                float lock68 = (float)ssTrigRead2Byte(pos, data);
                asShadowDrawing_SetShadowAngleLockParameter(shadowBank, lock64 / TRIG_ANGLE_SCALE,
                                                            lock68 / TRIG_ANGLE_SCALE);
            } else {
                *pos += 10;
            }
            break;
        case TRIG_CMD_TRANSLUCENT_OFF:
            if (skip == 0) {
                tiSet_CharaTranslucenceOFF(ssTrigRead1Byte(pos, data), 1);
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_TRANSLUCENT_ON:
            if (skip == 0) {
                u_char chara = ssTrigRead1Byte(pos, data);
                tiSet_CharaTranslucenceON(chara, ssTrigRead1Byte(pos, data), 1);
            } else {
                *pos += 2;
            }
            break;
        case TRIG_CMD_FADE_OUT:
            if (skip == 0) {
                tiXFadeOut_Start((float)ssTrigRead1Byte(pos, data));
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_HQDOF_END:
            if (skip == 0) {
                ssTrigHQDOFEnd();
            }
            break;
        case TRIG_CMD_HQDOF_START:
            if (skip == 0) {
                u_char a = ssTrigRead1Byte(pos, data);
                ssTrigHQDOFStart(a, ssTrigRead1Byte(pos, data));
            } else {
                *pos += 2;
            }
            break;
        case TRIG_CMD_VASELINE_END:
            if (skip == 0) {
                ssTrigVaselineEnd();
            }
            break;
        case TRIG_CMD_VASELINE_START:
            if (skip == 0) {
                u_short a = ssTrigRead2Byte(pos, data);
                u_short b = ssTrigRead2Byte(pos, data);
                u_short c = ssTrigRead2Byte(pos, data);
                u_short d = ssTrigRead2Byte(pos, data);
                u_char e = ssTrigRead1Byte(pos, data);
                u_char f = ssTrigRead1Byte(pos, data);
                u_char g = ssTrigRead1Byte(pos, data);
                u_short h = ssTrigRead2Byte(pos, data);
                u_char i = ssTrigRead1Byte(pos, data);
                ssTrigVaselineStart(a, b, c, d, e, f, g, h, i);
            } else {
                *pos += 14;
            }
            break;
        case TRIG_CMD_WAVE_END:
            if (skip == 0) {
                ssTrigWaveEnd();
            }
            break;
        case TRIG_CMD_WAVE_START:
            if (skip == 0) {
                u_short a = ssTrigRead2Byte(pos, data);
                u_short b = ssTrigRead2Byte(pos, data);
                u_short c = ssTrigRead2Byte(pos, data);
                u_short d = ssTrigRead2Byte(pos, data);
                u_char e = ssTrigRead1Byte(pos, data);
                u_char f = ssTrigRead1Byte(pos, data);
                u_char g = ssTrigRead1Byte(pos, data);
                u_char h = ssTrigRead1Byte(pos, data);
                u_char i = ssTrigRead1Byte(pos, data);
                u_short j = ssTrigRead2Byte(pos, data);
                u_char k = ssTrigRead1Byte(pos, data);
                ssTrigWaveStart(a, b, c, d, e, f, g, h, i, j, bank, k);
            } else {
                *pos += 16;
            }
            break;
        case TRIG_CMD_HAZY_END:
            if (skip == 0) {
                ssTrigHazyScreenEnd();
            }
            break;
        case TRIG_CMD_HAZY_START:
            if (skip == 0) {
                u_char level = ssTrigRead1Byte(pos, data);
                ssTrigHazyScreenStart(level, ssTrigRead2Byte(pos, data));
            } else {
                *pos += 3;
            }
            break;
        case TRIG_CMD_SCREEN_BLUR_END:
            if (skip == 0) {
                ssTrigScreenBlurEnd();
            }
            break;
        case TRIG_CMD_SCREEN_BLUR_START:
            if (skip == 0) {
                ssTrigScreenBlurStart(ssTrigRead1Byte(pos, data));
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_MOTION_BLUR_END:
            if (skip == 0) {
                ssPartMotionBlurEnd(ssTrigRead2Byte(pos, data), bank);
            } else {
                *pos += 2;
            }
            break;
        case TRIG_CMD_MOTION_BLUR_START:
            if (skip == 0) {
                u_short part = ssTrigRead2Byte(pos, data);
                u_char a = ssTrigRead1Byte(pos, data);
                u_char b = ssTrigRead1Byte(pos, data);
                u_char c = ssTrigRead1Byte(pos, data);
                ssPartMotionBlurStart(part, a, bank, b, c);
            } else {
                *pos += 5;
            }
            break;
        case TRIG_CMD_PARTICLE_STOP:
            if (skip == 0) {
                if (ssTrigRead1Byte(pos, data) == 0) {
                    ssTrigParticleStop(ssTrigRead2Byte(pos, data), bank);
                } else {
                    ssTrigParticleAllOffImmediately(ssTrigRead2Byte(pos, data), bank);
                }
            } else {
                *pos += 3;
            }
            break;
        case TRIG_CMD_PARTICLE_START:
            if (skip == 0) {
                u_short particle = ssTrigRead2Byte(pos, data);
                ssTrigParticleStart(particle, bank, ssTrigRead2Byte(pos, data));
            } else {
                *pos += 4;
            }
            break;
        case TRIG_CMD_XCMD:
            if (skip == 0) {
                char *argv[TRIG_XCMD_COUNT];
                u_short length = ssTrigRead2Byte(pos, data);
                u_char index = ssTrigRead1Byte(pos, data);
                u_char argc = ssTrigRead1Byte(pos, data);
                for (int i = 0; i < argc; i++) {
                    argv[i] = (char *)data + *pos;
                    *pos += (u_short)(strlen(argv[i]) + 1);
                }
                answer = (s_char)ssXcmd_functions[index](argc, argv, bank);   // the ROM tests the low byte
                if (answer == 0) {
                    *pos -= (u_short)(length + 6);      // id, length, index, argc and the strings
                }
            } else {
                *pos += (u_short)(ssTrigRead2Byte(pos, data) + 2);
            }
            break;
        case TRIG_CMD_CANCEL_ACTOR:
            if (skip == 0) {
                u_short actor = ssTrigRead2Byte(pos, data);
                ssTrigCancelActor(actor, ssTrigRead1Byte(pos, data), bank);
            } else {
                *pos += 3;
            }
            break;
        case TRIG_CMD_RUN_ACTOR:
            if (skip == 0) {
                u_short actor = ssTrigRead2Byte(pos, data);
                answer = (s_char)ssTrigExecuteActor(&cmds->wait, actor, ssTrigRead1Byte(pos, data), bank);
                if (answer == 0) {
                    *pos -= 5;
                }
            } else {
                *pos += 3;
            }
            break;
        case TRIG_CMD_IF_SCENE_DRAW:
            if (skip == 0) {
                u_char drawn = (u_char)ssTrigCheckSceneDraw((int)ssTrigRead4Byte(pos, data));
                u_char expected = ssTrigRead1Byte(pos, data);
                skip = drawn == (u_char)(expected - 1) ? 0 : cmds->step;
            } else {
                *pos += 5;
            }
            break;
        case TRIG_CMD_IF_RANDOM:
            if (skip == 0) {
                skip = ssTrigCheckRandom(ssTrigRead1Byte(pos, data)) != 0 ? 0 : cmds->step;
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_IF_VARIABLE:
            if (skip == 0) {
                u_short var = ssTrigRead2Byte(pos, data);
                int value = (int)ssTrigRead4Byte(pos, data);
                u_char global = ssTrigRead1Byte(pos, data);
                u_char cmp = ssTrigRead1Byte(pos, data);
                skip = ssTrigCheckVariable(var, value, global, cmp, bank) != 0 ? 0 : cmds->step;
            } else {
                *pos += 8;
            }
            break;
        case TRIG_CMD_IF_EVENT:
            if (skip == 0) {
                u_short event = ssTrigRead2Byte(pos, data);
                u_char global = ssTrigRead1Byte(pos, data);
                u_char set = (u_char)ssTrigCheckEvent(event, global, bank);
                u_char expected = ssTrigRead1Byte(pos, data);
                skip = set == (u_char)(expected - 1) ? 0 : cmds->step;
            } else {
                *pos += 4;
            }
            break;
        case TRIG_CMD_IF_ENABLE:
            if (skip == 0) {
                u_char on = (u_char)ssTrigCheckEnable(ssTrigRead2Byte(pos, data), bank);
                u_char expected = ssTrigRead1Byte(pos, data);
                skip = on == (u_char)(expected - 1) ? 0 : cmds->step;
            } else {
                *pos += 3;
            }
            break;
        case TRIG_CMD_RETURN:
            if (skip == 0) {
                end = 1;
            }
            break;
        case TRIG_CMD_ELSE:
            if (skip == 0) {
                skip = cmds->step;
            } else if (skip == cmds->step) {
                skip = 0;
            }
            break;
        case TRIG_CMD_ENDIF:
            if (skip != 0 && skip == cmds->step) {
                skip = 0;
            }
            cmds->step--;
            break;
        case TRIG_CMD_IF:
            cmds->step++;
            break;
        case TRIG_CMD_SET_VARIABLE:
            if (skip == 0) {
                u_short var = ssTrigRead2Byte(pos, data);
                int value = (int)ssTrigRead4Byte(pos, data);
                ssTrigSetVariable(var, value, ssTrigRead1Byte(pos, data), bank);
            } else {
                *pos += 7;
            }
            break;
        case TRIG_CMD_SELF_DISCARD_LOAD:
            if (skip == 0) {
                char *name = (char *)data + *pos;
                *pos += TRIG_CMD_NAME_LEN;
                u_short loadBank = ssTrigRead2Byte(pos, data);
                u_short loadArg = ssTrigRead2Byte(pos, data);
                int draw = ssTrigRead1Byte(pos, data) - 1;
                int byPos = ssTrigRead1Byte(pos, data) - 1;
                SelfDiscardSceneLoad(name, loadBank, loadArg, draw, byPos, (short)ssTrigRead2Byte(pos, data), bank);
            } else {
                *pos += 13;
            }
            break;
        case TRIG_CMD_SCENE_DRAW:
            if (skip == 0) {
                char *name = (char *)data + *pos;
                *pos += TRIG_CMD_NAME_LEN;
                int on = ssTrigRead1Byte(pos, data) - 1;
                ssTrigRead1Byte(pos, data);                     // read and dropped by the ROM
                u_short part = ssTrigRead2Byte(pos, data);
                TrgSceneDraw(on, bank, part, name, ssTrigRead2Byte(pos, data));
            } else {
                *pos += 11;
            }
            break;
        case TRIG_CMD_SCENE_LOAD:
            if (skip == 0) {
                char *name = (char *)data + *pos;
                *pos += TRIG_CMD_NAME_LEN;
                u_short loadBank = ssTrigRead2Byte(pos, data);
                TrgSceneLoad(name, loadBank, ssTrigRead2Byte(pos, data));
            } else {
                *pos += 9;
            }
            break;
        case TRIG_CMD_WAIT_ANIM:
            if (skip == 0) {
                u_long hash = ssTrigRead8Byte(pos, data);
                u_short part = ssTrigRead2Byte(pos, data);
                u_char pose = ssTrigRead1Byte(pos, data);
                u_char other = ssTrigRead1Byte(pos, data);
                u_char byPart = ssTrigRead1Byte(pos, data);
                u_char target = ssTrigRead1Byte(pos, data);
                u_char chara = ssTrigRead1Byte(pos, data);
                answer = asCheckAnimationState_S(hash, All_table_TBL[bank], part, pose, other, byPart, target,
                                                 chara) != 1;
                if (answer == 0) {
                    *pos -= 17;
                }
            } else {
                *pos += 15;
            }
            break;
        case TRIG_CMD_WAIT_FRAMES:
            if (skip == 0) {
                int ticks = (int)ssTrigRead4Byte(pos, data) * 100 / anim_play_slow;
                if (BankInfo[bank].type == BANK_TYPE_0) {
                    answer = (s_char)ssTrigWaitByAnimFrameForGlobal(&cmds->wait, ticks);
                } else {
                    answer = (s_char)ssTrigWaitByAnimFrame(&cmds->wait, ticks);
                }
                if (answer == 0) {
                    *pos -= 6;
                }
            } else {
                *pos += 4;
            }
            break;
        case TRIG_CMD_TEX_SET_NEXT:
            if (skip == 0) {
                u_short part = ssTrigRead2Byte(pos, data);
                tnTrigTexFileSetNext(part, ssTrigRead1Byte(pos, data) - 1, bank);
            } else {
                *pos += 3;
            }
            break;
        case TRIG_CMD_TEX_SET_PAGE:
            if (skip == 0) {
                u_short part = ssTrigRead2Byte(pos, data);
                int literal = ssTrigRead1Byte(pos, data) - 1;
                u_char a2 = ssTrigRead1Byte(pos, data);
                tnTrigTexFileSetPage(part, literal, a2, ssTrigRead2Byte(pos, data), bank);
            } else {
                *pos += 6;
            }
            break;
        case TRIG_CMD_ANIM_PLAY:
            if (skip == 0) {
                TrigAnimStart(pos, data, 0, bank);
            } else {
                *pos += 32;
            }
            break;
        case TRIG_CMD_ANIM_RANGE:
            if (skip == 0) {
                TrigAnimStart(pos, data, 1, bank);
            } else {
                *pos += 36;
            }
            break;
        case TRIG_CMD_ANIM_STOP:
            if (skip == 0) {
                u_long hash = ssTrigRead8Byte(pos, data);
                u_short part = ssTrigRead2Byte(pos, data);
                u_char pose = ssTrigRead1Byte(pos, data);
                u_char other = ssTrigRead1Byte(pos, data);
                s_char byPart = (s_char)ssTrigRead1Byte(pos, data);
                u_char target = ssTrigRead1Byte(pos, data);
                u_char chara = ssTrigRead1Byte(pos, data);
                asSceneAnimationStop_S(hash, All_table_TBL[bank], part, pose, other, byPart, target, chara);
            } else {
                *pos += 15;
            }
            break;
        case TRIG_CMD_CAM_DOF:
            if (skip == 0) {
                CamSetNowDOFLevel(ssTrigRead1Byte(pos, data));
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_CAM_FOV:
            if (skip == 0) {
                // The ROM passes (short)(degrees * 0x10000 / 360) and the time; CamChangeFOV_S ignores both.
                ssTrigRead2Byte(pos, data);
                ssTrigRead1Byte(pos, data);
                CamChangeFOV_S();
            } else {
                *pos += 3;
            }
            break;
        case TRIG_CMD_CAM_CHANGE: {
            short scene = (short)ssTrigRead2Byte(pos, data);
            if (scene == TRIG_CAM_THIS_SCENE) {
                if (skip == 0) {
                    if (BankInfo[bank].type == BANK_TYPE_SHADOW && idCameraScene != bank) {
                        TrigSetCameraScene(bank);
                    }
                    u_char cut = ssTrigRead1Byte(pos, data);
                    u_char mode = ssTrigRead1Byte(pos, data);
                    CamChange_S(bank, cut, mode, TrigCamTime(ssTrigRead2Byte(pos, data)));
                } else {
                    *pos += 4;
                }
            } else if (skip == 0) {
                *pos += TRIG_CMD_NAME_LEN;              // a name the ROM steps over
                if (BankInfo[bank].type == BANK_TYPE_SHADOW && BankInfo[scene].type == BANK_TYPE_SHADOW) {
                    TrigSetCameraScene(scene);
                }
                u_long hash = ssTrigRead8Byte(pos, data);
                u_char mode = ssTrigRead1Byte(pos, data);
                CamChange_Hash_S_Hash(scene, hash, mode, TrigCamTime(ssTrigRead2Byte(pos, data)));
            } else {
                *pos += 16;
            }
            break;
        }
        case TRIG_CMD_CAM_ZELCAM:
            if (skip == 0) {
                u_short maxDist = ssTrigRead2Byte(pos, data);
                u_short minDist = ssTrigRead2Byte(pos, data);
                u_short unused = ssTrigRead2Byte(pos, data);
                u_short height = ssTrigRead2Byte(pos, data);
                u_short angle = ssTrigRead2Byte(pos, data);
                CamChangeZELCAM_S(maxDist, minDist, unused, height, angle, ssTrigRead1Byte(pos, data));
            } else {
                *pos += 11;
            }
            break;
        case TRIG_CMD_EVENT:
            if (skip == 0) {
                u_char op = ssTrigRead1Byte(pos, data);
                u_short event = ssTrigRead2Byte(pos, data);
                u_char global = ssTrigRead1Byte(pos, data);
                if (op == TRIG_EVENT_REMOVE) {
                    ssTrigRemoveEvent(event, global, bank);
                } else {
                    ssTrigRegisterEvent(event, global, bank);
                }
            } else {
                *pos += 4;
            }
            break;
        case TRIG_CMD_SE_VOLUME:
            if (skip == 0) {
                u_short id2 = ssTrigRead2Byte(pos, data);
                ssTrigRead1Byte(pos, data);                     // passed on by the ROM; SSActVolume is empty
                ssTrigRead1Byte(pos, data);
                SSActVolume(bank, id2);
            } else {
                *pos += 4;
            }
            break;
        case TRIG_CMD_SE_LOAD_BANK:
            if (skip == 0) {
                const char *name = (const char *)data + *pos;
                *pos += TRIG_CMD_SE_NAME_LEN;
                ssTrigRead2Byte(pos, data);                     // passed on (with 1) by the ROM; unused there
                SSActLoadBankSE(name);
            } else {
                *pos += 15;
            }
            break;
        case TRIG_CMD_SE_STOP:
            if (skip == 0) {
                SSActStopSound(bank, ssTrigRead2Byte(pos, data));
            } else {
                *pos += 2;
            }
            break;
        case TRIG_CMD_SE_PLAY:
            if (skip == 0) {
                u_short obj = ssTrigRead2Byte(pos, data);
                ssTrigRead1Byte(pos, data);                     // passed on by the ROM; SSActPlaySound ignores it
                SSActPlaySound(bank, obj);
            } else {
                *pos += 3;
            }
            break;
        case TRIG_CMD_MENU:
            if (skip == 0) {
                ssMenuUpdate(ssTrigRead1Byte(pos, data) - 1);
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_PAUSE:
            if (skip == 0) {
                answer = (s_char)PauseUpdate(ssTrigRead1Byte(pos, data) - 1);
                if (answer == 0) {
                    *pos -= 3;
                }
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_MONSTER_AI:
            if (skip == 0) {
                MonsterAIUpdate(ssTrigRead1Byte(pos, data) - 1);
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_PLAYER_CONTROL:
            if (skip == 0) {
                PlayerControlUpdate(ssTrigRead1Byte(pos, data) - 1);
            } else {
                *pos += 1;
            }
            break;
        case TRIG_CMD_WARP:
            if (skip == 0) {
                const char *room = (const char *)data + *pos;
                *pos += TRIG_CMD_NAME_LEN;
                int mode = ssTrigRead1Byte(pos, data) - 1;
                u_short chara = ssTrigRead2Byte(pos, data);
                WarpObject(room, mode, chara, ssTrigRead8Byte(pos, data), bank);
            } else {
                *pos += 16;
            }
            break;
        case TRIG_CMD_SET_ENABLE:
            if (skip == 0) {
                u_short part = ssTrigRead2Byte(pos, data);
                int op = (s_char)ssTrigRead1Byte(pos, data) - 1;
                u_char delayed = ssTrigRead1Byte(pos, data);
                ssTrigSetEnable(part, op, delayed, bank);
            } else {
                *pos += 4;
            }
            break;
        default:
            break;                              // the ROM steps over nothing for an unknown id
        }
    } while (answer != 0 && !end);
    return answer;
}

// CT3 0x018d99c0
// x command: set script variable argv[1] (bank-local) to the direction part argv[0]'s animation plays
// (asCheckAnimationDirection_X, pose 1).
int suXcmd_AnmDirection(int argc, char **argv, int bank)
{
    (void)argc;
    int dir = asCheckAnimationDirection_X(All_table_TBL[bank], atoi(argv[0]) & 0xffff, 1, 0, 0);
    ssTrigSetVariable(atoi(argv[1]) & 0xffff, dir, 0, bank);
    return XCMD_DONE;
}
