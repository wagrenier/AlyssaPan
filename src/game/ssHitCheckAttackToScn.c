// game/ssHitCheckAttackToScn.c -- Clock Tower 3: the hit and collision module (ssHit*, ssCollision*, ssTrans*L2W,
// CheckLOS). Slice 1 (2026-11-11): the frame's collision pass (ssProcessHit), the attack request list,
// ssHitInit and the hit shapes' local -> world (ssTrans*HitDataL2W), the small hit-data helpers and
// ssRadianByO; the collision tests themselves are still stubs.

#include <math.h>

#include "common/alyssapan_stub.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/ct3_types.h"
#include "ga/gaInit3dlib.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/fileio.h"
#include "game/main2.h"
#include "game/BRANCH_CLEAR.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"

u_char gSceneL2WStatus[86];

// The queued attacks of this frame (a static of 0xe04 bytes: 256 records, then the count byte at 0xe00).
static struct {
    SS_HIT_ATTACK list[256];
    u_char count;
} ssHitAtList;

// Character ids ssProcessHit leaves out of the collision pass (names from this one use).
#define CHARA_ID_NO_COLLIDE_1D 0x1d
#define CHARA_ID_NO_COLLIDE_1E 0x1e     /* with 0x1f: ids 0x1e..0x1f */
#define CHARA_ID_NO_COLLIDE_1F 0x1f
// Character[] slots 11 and 12 never collide (ssProcessHit skips them before reading anything).
#define CHARA_SLOT_NO_COLLIDE_LO 11
#define CHARA_SLOT_NO_COLLIDE_HI 12
#define CHARACTER_FLAG_NO_COLLIDE 0x08000000u /* set: ssProcessHit leaves the character out */

// The ROM's matrix-vector product (VMULAx / VMADDAy / VMADDAz / VMADDw over the rows): out = v.x r0 +
// v.y r1 + v.z r2 + v.w r3.
static void ssApplyMatrix(float *out, const float (*m)[4], const float *v)
{
    float r[4];
    for (int i = 0; i < 4; i++)
        r[i] = m[0][i] * v[0] + m[1][i] * v[1] + m[2][i] * v[2] + m[3][i] * v[3];
    for (int i = 0; i < 4; i++)
        out[i] = r[i];
}

// The rows' sum the velocity is measured on: ((r1 + r0) + r2) + r3, r3 left out with HIT_DATA_FLAG_SIX.
static void ssHitRowSum(float *out, const float (*w)[4], int six)
{
    for (int k = 0; k < 4; k++) {
        float s = w[1][k] + w[0][k];
        s = w[2][k] + s;
        if (!six)
            s = w[3][k] + s;
        out[k] = s;
    }
}

// The body both ssTrans*HitDataL2W share for one shape: the world frame, point, vectors and plane;
// in the per-frame mode also the velocity, from `before` (NULL: the shape's world frame as it is).
static void ssTransOneHitData(MODEL_PARTS *part, HIT_DATA *h, int mode, float (*before)[4])
{
    int six = (h->flags & HIT_DATA_FLAG_SIX) != 0;
    ssApplyMatrix(h->worldPt, part->matrix180, h->localPt);
    if (mode == SS_L2W_FRAME) {
        float b[4], a[4];
        if (before != NULL)
            gaMulMatrix(h->world, before, h->local);
        ssHitRowSum(b, h->world, six);
        gaMulMatrix(h->world, part->matrix180, h->local);
        ssHitRowSum(a, h->world, six);
        float scale = six ? 0x1.55553ep-2f /* 0x3eaaaa9f, just under 1/3 */ : 0.25f;
        for (int k = 0; k < 4; k++)
            h->vel[k] = (a[k] - b[k]) * scale;
    } else {
        gaMulMatrix(h->world, part->matrix180, h->local);
    }
    if (!six) {
        ssApplyMatrix(h->worldV[0], part->matrix180, h->localV[0]);
        ssApplyMatrix(h->worldV[3], part->matrix180, h->localV[3]);
        ssApplyMatrix(h->worldV[4], part->matrix180, h->localV[4]);
        h->w180[1] = h->w110[1];
        h->w180[0] = h->w110[0];
    } else {
        for (int i = 0; i < 6; i++)
            ssApplyMatrix(h->worldV[i], part->matrix180, h->localV[i]);
        h->w180[0] = h->w110[0];
        h->w180[1] = h->w110[1];
        h->w180[2] = h->w110[2];
    }
    h->planeD = gaInnerProductXYZ(h->worldPt, h->world[0]);
}

// CT3 0x018aa830
void ssHitCheckAttackList(void)
{
    int count = ssHitAtList.count;
    for (int i = 0; i < count; i++) {
        SS_HIT_ATTACK *a = &ssHitAtList.list[i];
        if (a->kind == SS_HIT_ATK_CHARACTER)
            ssHitCheckCharacterAttack(a);
        else if (a->target != -1)
            ssHitCheckSceneAttack(a);
    }
}

// CT3 0x018aa8d0
void ssHitClearAttackRequest(void)
{
    ssHitAtList.count = 0;
}

// CT3 0x018aa8e0
void ssHitAddAttackRequest(int chara, int atk, int kind, int power, int target, int b0d, int motion, int bank)
{
    if (ssHitAtList.count != SS_HIT_ATTACK_MAX) {
        SS_HIT_ATTACK *a = &ssHitAtList.list[ssHitAtList.count];
        a->power = (short)power;
        a->chara = (short)chara;
        a->atk = (short)atk;
        a->target = (short)target;
        a->motion = (short)motion;
        a->bank = (short)bank;
        a->kind = (u_char)kind;
        a->b0d = (u_char)b0d;
        ssHitAtList.count++;
    }
}

// CT3 0x018ac0d0
// The scene's damage shapes of kind `mode` into the world (parts flagged PARTS_FLAG10_NO_HIT stay).
// The per-frame velocity is measured from the part's matrix (the frame before) to its matrix180.
// callcheck: gaMulMatrix, gaInnerProductXYZ (the ROM's inlined per-shape body: ssTransOneHitData)
void ssTransScnDmgHitDataL2W(MODEL_PARTS *parts, HIT_TABLE *hits, int mode)
{
    int bank = BankOfHostPtr(hits);     // PORT: the list's words are EE addresses of the table's bank
    const u_int *list = BANK_PTR(const u_int *, bank, hits->dmgData);
    int n = hits->dmgCount;
    for (int i = 0; i < n; i++) {
        HIT_DATA *h = BANK_PTR(HIT_DATA *, bank, list[i]);
        if (h->kind == (s_char)mode) {
            MODEL_PARTS *part = &parts[h->part];
            if ((part->flags10 & PARTS_FLAG10_NO_HIT) == 0)
                ssTransOneHitData(part, h, mode, part->matrix);
        }
    }
}

// CT3 0x018ac570
// The character's collision points into the world, each by its part's world matrix.
void ssTransColHitDataL2W(MODEL_PARTS *parts, HIT_TABLE *hits)
{
    // PORT: the words are EE addresses of the hit table's own bank.
    int bank = BankOfHostPtr(hits);
    const float (*local)[4] = BANK_PTR(const float (*)[4], bank, hits->colLocal);
    float (*world)[4] = BANK_PTR(float (*)[4], bank, hits->colWorld);
    const short *part = BANK_PTR(const short *, bank, hits->colParts);
    int n = hits->colCount;
    for (int i = 0; i < n; i++)
        ssApplyMatrix(world[i], parts[part[i]].matrix, local[i]);
}

// CT3 0x018ac6a0
// The id (HIT_OBJ_ID) of character bank `bank`'s attack object whose attack is `atk`; -1 for none or
// when the bank is not a character shape.
int ssHit_GetMaxIDFromAtkID(int bank, int atk)
{
    if (BankInfo[bank].type == FS_LOAD_SHAPE) {
        HIT_TABLE *hits = BANK_PTR(HIT_TABLE *, bank, All_table_TBL[bank]->hits);
        const u_int *objs = BANK_PTR(const u_int *, bank, hits->objs);
        int n = hits->objCount;
        for (int i = 0; i < n; i++) {
            const u_char *obj = BANK_PTR(const u_char *, bank, objs[i]);
            if (*(const s_char *)(obj + HIT_OBJ_ATK) == (s_char)atk)
                return *(const short *)(obj + HIT_OBJ_ID);
        }
    }
    return -1;
}

// CT3 0x018ac920
// The bank's hit shapes of kind `mode` into the world; a moving shape (kind 2) stays while its part is
// flagged PARTS_FLAG10_NO_HIT. The per-frame velocity is measured from the shape's last world frame.
// callcheck: gaMulMatrix, gaInnerProductXYZ (the ROM's inlined per-shape body: ssTransOneHitData)
void ssTransHitDataL2W(MODEL_PARTS *parts, HIT_TABLE *hits, int mode)
{
    int bank = BankOfHostPtr(hits);     // PORT: the list's words are EE addresses of the table's bank
    const u_int *list = BANK_PTR(const u_int *, bank, hits->hitData);
    for (int i = 0; list[i] != 0; i++) {
        HIT_DATA *h = BANK_PTR(HIT_DATA *, bank, list[i]);
        if (h->kind == (s_char)mode &&
            (h->kind != SS_L2W_FRAME || (parts[h->part].flags10 & PARTS_FLAG10_NO_HIT) == 0))
            ssTransOneHitData(&parts[h->part], h, mode, NULL);
    }
}

// CT3 0x018ad300
// The direction of v in the plane that drops `axis` (0: y/z, 1: x/z, 2: x/y; the second component over
// the first), in radians. Software doubles on the EE; the length is the VU0's float.
// callcheck: fptodp, dpdiv, dpadd, dpsub, _dpflt, _dpfgt, _dpfge (the EE's software double: the host's
// double arithmetic and compares)
double ssRadianByO(const float *v, int axis)
{
    double c = 0.0;                     // the component the angle is measured from
    double s = 0.0;
    if (axis == 2) {
        c = v[1];
        s = v[0];
    } else if (axis == 1) {
        c = v[2];
        s = v[0];
    } else if (axis == 0) {
        c = v[2];
        s = v[1];
    }
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    double cn = c / (double)len;
    double r;
    if (!((double)-0.01f < cn) || !(cn < (double)0.01f)) {  // float literals, promoted
        if (axis == 0) {                // (-pi/2, pi/2]: the ROM's branches, kept as they are
            if (!(c > 0.0)) {
                if (!(s > 0.0))
                    r = -atan(s / c);
                else
                    r = atan(s / -c);
            } else if (!(s >= 0.0)) {
                r = -atan(-s / c);
            } else {
                r = atan(s / c);
            }
        } else {                        // [0, 2pi)
            if (!(c > 0.0)) {
                if (!(s > 0.0))
                    r = atan(s / c) + 3.141592653589793;
                else
                    r = 3.141592653589793 - atan(s / -c);
            } else if (!(s >= 0.0)) {
                r = 6.283185307179586 - atan(-s / c);
            } else {
                r = atan(s / c);
            }
        }
    } else {
        // c is about 0: a quarter turn either way along s, or no direction at all.
        double sn = s / (double)len;
        if (sn < (double)-0.1f)
            r = axis == 0 ? -1.5707963267948966 : 4.71238898038469;
        else if (sn > (double)0.1f)
            r = 1.5707963267948966;
        else
            r = 0.0;
    }
    return r;
}

// CT3 0x018ad760
// Moves the hit table's world collision points by d (the ROM adds the whole qword, w too).
void ssMoveAllHitObjCoordinate(HIT_TABLE *hits, const float *d)
{
    float (*world)[4] = BANK_PTR(float (*)[4], BankOfHostPtr(hits), hits->colWorld);
    int n = hits->colCount;
    for (int i = 0; i < n; i++)
        for (int k = 0; k < 4; k++)
            world[i][k] = d[k] + world[i][k];
}

// CT3 0x018b1820
// src split into its horizontal part (y cleared) and its vertical part (x and z cleared); w kept in both.
void ssSplitVectorHandV(float *h, float *v, const float *src)
{
    float t[4] = { src[0], src[1], src[2], src[3] };
    for (int k = 0; k < 4; k++)
        v[k] = t[k];
    for (int k = 0; k < 4; k++)
        h[k] = t[k];
    v[0] = 0.0f;
    v[2] = 0.0f;
    h[1] = 0.0f;
}

// CT3 0x018b1e60
// Once a frame (main): clear the loaded banks' per-frame hit counts, collide each solid character with
// the scene bank it is in (CHARACTER.lightBank), then run the queued attacks and empty the queue.
void ssProcessHit(void)
{
    SS_HIT_CHARA list[SS_HIT_CHARA_MAX];
    int count = 0;

    for (int bank = 0; bank < BANK_MAX; bank++) {
        gSceneL2WStatus[bank] = 0;
        if (BankInfo[bank].drawOn != 0) {
            if (BankInfo[bank].type == FS_LOAD_SCENE) {
                HIT_TABLE *hits = BANK_PTR(HIT_TABLE *, bank, All_table_TBL[bank]->hits);
                hits->objHitCount = 0;
                hits->w2b8 = 0;
            } else if (BankInfo[bank].type == FS_LOAD_SHAPE) {
                HIT_TABLE *hits = BANK_PTR(HIT_TABLE *, bank, All_table_TBL[bank]->hits);
                hits->w1c = 0;
                hits->count = 0;
            }
            bank += BankInfo[bank].groupCount - 1;
        }
    }

    for (int i = 0; i < CHARACTER_MAX; i++) {
        if (i == CHARA_SLOT_NO_COLLIDE_LO || i == CHARA_SLOT_NO_COLLIDE_HI)
            continue;
        CHARACTER *ch = &Character[i];
        ch->w300[2] = ch->w300[0];
        ch->w300[3] = ch->w300[1];
        ch->w300[0] = 0;
        ch->w300[1] = 0;
        if ((ch->flags & CHARACTER_FLAG_SHADOW) != 0 && (ch->flags & CHARACTER_FLAG_NO_COLLIDE) == 0 &&
            ch->id != CHARA_ID_NO_COLLIDE_1D &&
            (ch->id < CHARA_ID_NO_COLLIDE_1E || ch->id > CHARA_ID_NO_COLLIDE_1F)) {
            short bank = ch->lightBank;
            if (bank != -1 && BankInfo[bank].state == BANK_STATE_LOADED) {
                list[count].chara = ch;
                list[count].bank = bank;
                count++;
            }
        }
    }
    for (int i = 0; i < count; i++)
        ssCollisionCheck(list[i].chara, list[i].bank, list, count);

    ssHitCheckAttackList();
    ssHitClearAttackRequest();
}

// CT3 0x018b2070
// A scene bank's hit data into the world when it is set up (scene_draw_init).
void ssHitInit(int scene)
{
    MODEL_PARTS *parts = (MODEL_PARTS *)(pModelPartsHeader[scene] + PARTS_HDR_RECORDS);
    HIT_TABLE *hits = BANK_PTR(HIT_TABLE *, scene, All_table_TBL[scene]->hits);
    ssTransHitDataL2W(parts, hits, SS_L2W_INIT);
    ssTransScnDmgHitDataL2W(parts, hits, SS_L2W_INIT);
}

// CT3 0x018b20e0
// (a tail jump in the ROM)
void ssHitGlobalInit(void)
{
    ssHitClearAttackRequest();
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018ab180
int CheckLOS(int bank, LOS_SEG *seg)
{
    ALYSSAPAN_STUB("CheckLOS", 0x018ab180);
    return {};
}

// stub of CT3 0x018aa950
int CheckLOS_Multi(int bank, LOS_MULTI *q)
{
    ALYSSAPAN_STUB("CheckLOS_Multi", 0x018aa950);
    return {};
}

// stub of CT3 0x018b1a40
void ssCollisionCheck(struct CHARACTER *chara, int bank, SS_HIT_CHARA *list, int count)
{
    ALYSSAPAN_STUB("ssCollisionCheck", 0x018b1a40);
}

// stub of CT3 0x018aa570
void ssHitCheckCharacterAttack(SS_HIT_ATTACK *a)
{
    ALYSSAPAN_STUB("ssHitCheckCharacterAttack", 0x018aa570);
}

// stub of CT3 0x018aa370
void ssHitCheckSceneAttack(SS_HIT_ATTACK *a)
{
    ALYSSAPAN_STUB("ssHitCheckSceneAttack", 0x018aa370);
}
