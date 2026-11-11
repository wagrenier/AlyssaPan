// game/ssGrpPortalCheck.c -- Clock Tower 3: the trigger system and the room portals (EE 018d2ee0..018d9a40).
// Each bank carries trigger data (detectors, conditions, actors, its own event flags and variables);
// Trigger_main runs them every frame, ProcessPortal moves the characters between rooms.
// Slice 1 (2026-11-10): the event / variable API, the script byte readers, the enable checks,
// InitTriggerDetectorShape, CheckRoomEntry and the portals (ProcessPortal, ssScnPortalCheck,
// CheckPlaneEntry, ssGrpPortalCheck). Slice 2 (2026-11-10): Trigger_init / _deinit, TriggerGlobalInit,
// the trigger block getters and readers, the delay-enable ring. Trigger_main is still a stub.

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
    int pos = (int)act->deactivatePos;
    return ssTrigRead1Byte(&pos, ActData[bank]);
}

// CT3 0x018d96f0
u_char ssTrigReadNumActivateCmd(const TRIG_ACT_WORK *act, int bank)
{
    int pos = (int)act->activatePos;
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
    tw->header12 = (short)ssTrigRead2Byte(&pos, OthData[bank]);
    tw->script = OthData[bank] + pos;

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
            d->kind = (short)ssTrigRead2Byte(&body, DetData[bank]);
            d->body = DetData[bank] + body;
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
            a->activatePos = ssTrigRead4Byte(&pos, ActData[bank]);
            a->numActivate = ssTrigReadNumActivateCmd(a, bank);
            a->deactivatePos = ssTrigRead4Byte(&pos, ActData[bank]);
            a->numDeactivate = ssTrigReadNumDeactivateCmd(a, bank);
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

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018d4120
void Trigger_main(void)
{
    ALYSSAPAN_STUB("Trigger_main", 0x018d4120);
}
