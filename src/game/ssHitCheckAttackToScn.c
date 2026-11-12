// game/ssHitCheckAttackToScn.c -- Clock Tower 3: the hit and collision module (ssHit*, ssCollision*, ssTrans*L2W,
// CheckLOS). Slice 1 (2026-11-11): the frame's collision pass (ssProcessHit), the attack request list,
// ssHitInit and the hit shapes' local -> world (ssTrans*HitDataL2W), the small hit-data helpers and
// ssRadianByO. Slice 2 (2026-11-11): the line-of-sight tests CheckLOS and CheckLOS_Multi, and the attack
// and damage objects' local -> world (ssTrans*AtkHitDataL2W, ssTransDmgHitDataL2W). Slice 3 (2026-11-11):
// the attack checks. Slice 4 (2026-11-11): the box tests -- ssHitCheckOBB_OBB, ssHitCheckOBB_Plane,
// ssHitCheckOBB_TriangleSides (separating axes) and ssHitCheckOneObject. Slice 5 (2026-11-11): the
// character box and the scene grid. Slice 6 (2026-11-12): the slide -- ssHitCheckAllObject,
// ssHitReCheckAllObject, ssHitCalcSlidePos2, ssHitCheckAndProcess -- and the frame's collision of one
// character, ssCollisionCheck (with ssColCheckChar_Chara): the TU's code is complete.

#include <float.h>
#include <math.h>
#include <string.h>

#include "common/alyssapan_stub.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/ct3_types.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaSinCos.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/fileio.h"
#include "game/main2.h"
#include "game/BRANCH_CLEAR.h"
#include "game/QuatSlerp.h"
#include "game/ssCalcInversMatrix2.h"
#include "game/tiAvoidObject.h"
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

// The rows' sum the velocity is measured on: ((r1 + r0) + r2) + r3, r3 left out with HIT_DATA_FLAG_TRIANGLE.
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
    int six = (h->flags & HIT_DATA_FLAG_TRIANGLE) != 0;
    ssApplyMatrix(h->worldN, part->matrix180, h->localN);
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
        h->half[1] = h->localHalf[1];
        h->half[0] = h->localHalf[0];
    } else {
        for (int i = 0; i < 6; i++)
            ssApplyMatrix(h->worldV[i], part->matrix180, h->localV[i]);
        h->half[0] = h->localHalf[0];
        h->half[1] = h->localHalf[1];
        h->half[2] = h->localHalf[2];
    }
    h->planeD = gaInnerProductXYZ(h->worldN, h->world[0]);
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

// ---- line of sight (slice 2, 2026-11-11) ----
// CheckLOS and CheckLOS_Multi are one body in the ROM, inlined twice (only registers differ): the grid
// walk and the polygon test are LosGatherShapes / LosCrossing here.

#define LOS_CANDIDATE_MAX 256           /* the ROM's 0x400-byte stack list (it never checks the bound) */
#define LOS_FACING_EPS -0.0001f         /* normal . dir below this: the segment runs into the front face */
#define LOS_T_NONE 1.1f                 /* CheckLOS's nearest t before any hit (past the segment's end) */

// The EE FPU's divide: x / 0 is +-FLT_MAX (the signs' xor), never inf or NaN. CheckLOS divides by the
// segment's x extent, which a vertical segment (tiCharaFootAdjust's) leaves 0.
static double LosEeDiv(float a, float b)
{
    if (b == 0.0f)
        return (signbit(a) != signbit(b)) ? -(double)FLT_MAX : (double)FLT_MAX;
    return (double)(a / b);
}

// The grid cells the segment's XZ line crosses within its bounding box, then the shapes listed in them
// (each once; NO_LOS shapes and shapes of NO_HIT parts left out). Answers the shape count.
static int LosGatherShapes(int bank, const LOS_SEG *seg, HIT_DATA **out)
{
    u_char *hdr = pModelPartsHeader[bank];
    MODEL_PARTS *parts = (MODEL_PARTS *)(hdr + PARTS_HDR_RECORDS);
    const HIT_SCENE_GRID *g = BANK_PTR(const HIT_SCENE_GRID *, bank, All_table_TBL[bank]->hits);
    const float *base = (const float *)((u_char *)parts + PARTS_ANIM_POS); // part 0's position
    u_char cells[HIT_GRID_N * HIT_GRID_N];
    int cellCount = 0;
    float dir[4];

    for (int k = 0; k < 4; k++)
        dir[k] = seg->to[k] - seg->from[k];
    float xMax = seg->from[0] < seg->to[0] ? seg->to[0] : seg->from[0];
    float xMin = seg->from[0] < seg->to[0] ? seg->from[0] : seg->to[0];
    float zMax = seg->from[2] < seg->to[2] ? seg->to[2] : seg->from[2];
    float zMin = seg->from[2] < seg->to[2] ? seg->from[2] : seg->to[2];
    float ox = g->originX + base[0];
    float oz = g->originZ + base[2];
    int x0 = (int)((xMin - ox) / g->cellW);
    int z0 = (int)((zMin - oz) / g->cellD);
    int x1 = (int)((xMax - ox) / g->cellW) + 1;
    int z1 = (int)((zMax - oz) / g->cellD) + 1;
    x0 = x0 < 0 ? 0 : (x0 > HIT_GRID_N - 1 ? HIT_GRID_N - 1 : x0);
    x1 = x1 < 1 ? 1 : (x1 > HIT_GRID_N ? HIT_GRID_N : x1);
    z0 = z0 < 0 ? 0 : (z0 > HIT_GRID_N - 1 ? HIT_GRID_N - 1 : z0);
    z1 = z1 < 1 ? 1 : (z1 > HIT_GRID_N ? HIT_GRID_N : z1);

    // The line z + k x + c = 0 through the segment; a cell is taken when it changes sign along its bottom,
    // right or top edge. PORT: in double, so a vertical segment's +-FLT_MAX slope keeps the signs the EE's
    // clamped arithmetic gives instead of inf - inf.
    double k = -LosEeDiv(dir[2], dir[0]);
    double c = -((double)seg->from[2] + k * seg->from[0]);
    for (int z = z0; z < z1; z++) {
        double zLo = oz + g->cellD * (float)z;
        double zHi = oz + g->cellD * (float)(z + 1);
        for (int x = x0; x < x1; x++) {
            double kxLo = k * (double)(ox + g->cellW * (float)x);
            double kxHi = k * (double)(ox + g->cellW * (float)(x + 1));
            double loHi = c + zLo + kxHi;
            if ((c + zLo + kxLo) * loHi <= 0.0 || loHi * (c + zHi + kxHi) <= 0.0 ||
                (c + zHi + kxHi) * (c + zHi + kxLo) <= 0.0)
                cells[cellCount++] = (u_char)(x + z * HIT_GRID_N);
        }
    }

    const u_int *list = BANK_PTR(const u_int *, bank, g->hitData);
    int n = 0;
    for (int i = 0; i < cellCount; i++) {
        for (int which = 0; which < 2; which++) {
            int cnt = which == 0 ? g->cellACount[cells[i]] : g->cellBCount[cells[i]];
            const u_short *idx =
                BANK_PTR(const u_short *, bank, which == 0 ? g->cellA[cells[i]] : g->cellB[cells[i]]);
            for (int j = 0; j < cnt; j++) {
                HIT_DATA *h = BANK_PTR(HIT_DATA *, bank, list[idx[j]]);
                if ((h->flags & HIT_DATA_FLAG_NO_LOS) != 0 || (parts[h->part].flags10 & PARTS_FLAG10_NO_HIT) != 0)
                    continue;
                int m = 0;
                while (m < n && out[m] != h)
                    m++;
                if (m == n && n < LOS_CANDIDATE_MAX) // PORT: the bound (the ROM writes on past its list)
                    out[n++] = h;
            }
        }
    }
    return n;
}

// The VU0's normalize (VMUL, two VADDbc, VRSQRT, VMULq): v * 1 / |v.xyz|.
static void LosNormalize(float *v)
{
    float q = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    for (int k = 0; k < 4; k++)
        v[k] *= q;
}

// Where the segment crosses h's plane from its front, at t in [0, 1] along it; answers 1 and the point
// when that point is inside the polygon (the angles the corners span from it add up to a full turn).
static int LosCrossing(const HIT_DATA *h, const LOS_SEG *seg, const float *dir, float *t, float *point)
{
    float dn = gaInnerProductXYZ((float *)h->worldN, (float *)dir);
    if (!(dn < LOS_FACING_EPS))
        return 0;
    float fn = gaInnerProductXYZ((float *)h->worldN, (float *)seg->from);
    *t = (h->planeD - fn) / dn;
    if (!(0.0f <= *t && *t <= 1.0f))
        return 0;
    for (int k = 0; k < 4; k++)
        point[k] = seg->from[k] + dir[k] * *t;
    int corners = (h->flags & HIT_DATA_FLAG_TRIANGLE) != 0 ? 3 : 4;
    float v[4][4];
    for (int i = 0; i < corners; i++) {
        for (int k = 0; k < 4; k++)
            v[i][k] = h->world[i][k] - point[k];
        LosNormalize(v[i]);
    }
    int angle = 0;
    for (int i = 0; i < corners; i++)
        angle += ahAcos(gaInnerProductXYZ(v[i], v[(i + 1) % corners]));
    return LOS_INSIDE_ANGLE <= (float)angle;
}

// CT3 0x018aa950
// Every polygon of the scene bank the segment crosses from its front (up to LOS_MULTI_MAX), in the order
// they were gathered; != 0 when there is one.
// callcheck: gaInnerProductXYZ, ahAcos (LosCrossing)
int CheckLOS_Multi(int bank, LOS_MULTI *q)
{
    LOS_SEG seg;
    HIT_DATA *cand[LOS_CANDIDATE_MAX];
    float dir[4], point[4], t;

    q->count = 0;
    memcpy(seg.from, q->from, sizeof(seg.from));
    memcpy(seg.to, q->to, sizeof(seg.to));
    for (int k = 0; k < 4; k++)
        dir[k] = seg.to[k] - seg.from[k];
    int n = LosGatherShapes(bank, &seg, cand);
    for (int i = 0; i < n; i++) {
        if (LosCrossing(cand[i], &seg, dir, &t, point)) {
            q->hits[q->count++] = cand[i];
            if (q->count == LOS_MULTI_MAX)
                break;
        }
    }
    return q->count != 0;
}

// CT3 0x018ab180
// The nearest polygon of the scene bank the segment crosses from its front: its point and record go to
// hit->point / hit->record (left as they were on a miss); != 0 on a hit.
// callcheck: gaInnerProductXYZ, ahAcos (LosCrossing)
int CheckLOS(int bank, LOS_HIT *hit)
{
    LOS_SEG seg = hit->seg;
    HIT_DATA *cand[LOS_CANDIDATE_MAX];
    HIT_DATA *best = NULL;
    float dir[4], point[4], t;
    float bestT = LOS_T_NONE;

    for (int k = 0; k < 4; k++)
        dir[k] = seg.to[k] - seg.from[k];
    int n = LosGatherShapes(bank, &seg, cand);
    for (int i = 0; i < n; i++) {
        if (LosCrossing(cand[i], &seg, dir, &t, point) && t < bestT) {
            memcpy(hit->point, point, sizeof(hit->point));
            bestT = t;
            best = cand[i];
        }
    }
    if (best != NULL)
        hit->record = best;
    return best != NULL;
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

// The object's rows into the world by its part's world matrix, four at a time.
static void ssTransObjRows(MODEL_PARTS *part, HIT_OBJ *obj)
{
    int bank = BankOfHostPtr(obj);      // PORT: the words are EE addresses of the object's bank
    float (*local)[4] = BANK_PTR(float (*)[4], bank, obj->local);
    float (*world)[4] = BANK_PTR(float (*)[4], bank, obj->world);
    for (int r = 0; r < obj->rows; r += 4)
        gaMulMatrix(world + r, part->matrix, local + r);
}

// CT3 0x018ac600
// A scene attack object into the world.
// callcheck: gaMulMatrix (ssTransObjRows)
void ssTransScnAtkHitDataL2W(MODEL_PARTS *parts, HIT_OBJ *obj)
{
    ssTransObjRows(&parts[obj->part], obj);
}

// CT3 0x018ac730
// The character bank's attack object for attack `atk` into the world; answers it, or NULL when there is
// none or its part is flagged PARTS_FLAG10_NO_HIT (then nothing moves).
// callcheck: gaMulMatrix (ssTransObjRows)
HIT_OBJ *ssTransAtkHitDataL2W(MODEL_PARTS *parts, HIT_TABLE *hits, int atk)
{
    int bank = BankOfHostPtr(hits);     // PORT: the list's words are EE addresses of the table's bank
    const u_int *objs = BANK_PTR(const u_int *, bank, hits->objs);
    int n = hits->objCount;
    for (int i = 0; i < n; i++) {
        HIT_OBJ *obj = BANK_PTR(HIT_OBJ *, bank, objs[i]);
        if (obj->atk == (s_char)atk) {
            MODEL_PARTS *part = &parts[obj->part];
            if ((part->flags10 & PARTS_FLAG10_NO_HIT) != 0)
                return NULL;
            ssTransObjRows(part, obj);
            return obj;
        }
    }
    return NULL;
}

// CT3 0x018ac840
// The character's damage shapes into the world by their parts' matrix180 (parts flagged
// PARTS_FLAG10_NO_HIT stay).
void ssTransDmgHitDataL2W(MODEL_PARTS *parts, HIT_TABLE *hits)
{
    int bank = BankOfHostPtr(hits);     // PORT: the list's words are EE addresses of the table's bank
    const u_int *list = BANK_PTR(const u_int *, bank, hits->hitData);
    int n = hits->dmgObjCount;
    for (int i = 0; i < n; i++) {
        HIT_DMG *d = BANK_PTR(HIT_DMG *, bank, list[i]);
        MODEL_PARTS *part = &parts[d->part];
        if ((part->flags10 & PARTS_FLAG10_NO_HIT) == 0) {
            for (int m = 0; m < 3; m++)
                gaMulMatrix(d->world[m], part->matrix180, d->local[m]);
            memcpy(d->w190, d->w0c0, sizeof(d->w190));
        }
    }
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

// ---- the attack checks (slice 3, 2026-11-11) ----

float ssHitNPos[4];
float ssHitBPos[4];

// The VU0's xyz normalize (VMUL, two VADDbc, VRSQRT, VMULq.xyz); w stays. PORT: VRSQRT of 0 is +FLT_MAX on
// the VU, so a zero vector stays zero instead of becoming NaN.
static void ssVuUnitXYZ(float *v)
{
    float len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    float q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;
    for (int k = 0; k < 3; k++)
        v[k] = v[k] == 0.0f ? 0.0f : v[k] * q;
}

// The attack part's two positions the box and the crossing point are made from.
static void ssHitSetAttackPos(const MODEL_PARTS *part)
{
    memcpy(ssHitNPos, part->transInit, sizeof(ssHitNPos));
    memcpy(ssHitBPos, part->drawOnPos, sizeof(ssHitBPos));
}

// The bank's parts and hit table.
static MODEL_PARTS *ssHitParts(int bank)
{
    return (MODEL_PARTS *)(pModelPartsHeader[bank] + PARTS_HDR_RECORDS);
}
static HIT_TABLE *ssHitTable(int bank)
{
    return BANK_PTR(HIT_TABLE *, bank, All_table_TBL[bank]->hits);
}

#define SS_HIT_B0D_SCENE 0x1            /* SS_HIT_ATTACK.b0d: the attack also strikes the scene bank a->bank */
#define SS_HIT_MOVE_EPS 0.01f           /* |N - B|^2 at or below: the crossing point is ssHitNPos itself */

// CT3 0x018a9fe0
// callcheck: gaInnerProductXYZ, ssHitCheckOneObject, ssHit_informationScn
int ssHitCheckAttackToScn(HIT_OBJ *obj, HIT_TABLE *scn, HIT_TABLE *atkHits, SS_HIT_ATTACK *a)
{
    float dir[4], unit[4], point[4];
    for (int k = 0; k < 3; k++)
        dir[k] = ssHitNPos[k] - ssHitBPos[k];
    dir[3] = ssHitNPos[3];              // vsub.xyz: w is the register's, N's
    memcpy(unit, dir, sizeof(unit));
    ssVuUnitXYZ(unit);
    int bank = BankOfHostPtr(scn);      // PORT: the list's words are EE addresses of the table's bank
    const u_int *list = BANK_PTR(const u_int *, bank, scn->dmgData);
    int n = scn->dmgCount;
    MODEL_PARTS *parts = ssHitParts(a->bank);
    for (int i = 0; i < n; i++) {
        HIT_DATA *h = BANK_PTR(HIT_DATA *, bank, list[i]);
        if ((parts[h->part].flags10 & PARTS_FLAG10_NO_HIT) != 0)
            continue;
        if (h->kind != SS_L2W_FRAME && !(gaInnerProductXYZ(h->worldN, unit) < 0.0f))
            continue;                   // a still shape is struck only from its front
        if (ssHitCheckOneObject(obj, h, NULL, NULL, NULL) == 0)
            continue;
        if (gaInnerProductXYZ(dir, dir) <= SS_HIT_MOVE_EPS) {
            memcpy(point, ssHitNPos, sizeof(point));
        } else {
            float nu = gaInnerProductXYZ(h->worldN, unit);
            float nb = gaInnerProductXYZ(h->worldN, ssHitBPos);
            float t = (float)LosEeDiv(h->planeD - nb, nu);
            for (int k = 0; k < 3; k++)
                point[k] = unit[k] * t + ssHitBPos[k];
            point[3] = unit[3];
        }
        ssHit_informationScn(a, obj, h, scn, atkHits, point);
        return 1;
    }
    return 0;
}

// CT3 0x018aa220
// callcheck: ssHitCheckOBB_OBB, ssHit_information
int ssHitCheckAttack(HIT_OBJ *obj, HIT_TABLE *dmg, SS_HIT_ATTACK *a, HIT_TABLE *atkHits, HIT_TABLE *scnHits)
{
    int hit = 0;
    int bank = BankOfHostPtr(dmg);      // PORT: the table's words are EE addresses of its bank
    MODEL_PARTS *parts = ssHitParts(Character[a->target].bank);
    if (ssHitCheckOBB_OBB(HIT_OBJ_OBB(obj), BANK_PTR(const SS_OBB *, bank, dmg->bodyObb)) != 0) {
        const u_int *list = BANK_PTR(const u_int *, bank, dmg->hitData);
        int n = dmg->dmgObjCount;
        for (int i = 0; i < n; i++) {
            HIT_DMG *d = BANK_PTR(HIT_DMG *, bank, list[i]);
            if ((parts[d->part].flags10 & PARTS_FLAG10_NO_HIT) == 0 &&
                ssHitCheckOBB_OBB(HIT_OBJ_OBB(obj), HIT_DMG_OBB(d)) != 0) {
                hit = 1;
                ssHit_information(a, obj, d, dmg, atkHits, scnHits);
            }
        }
    }
    return hit;
}

// The target character's damage shapes into the world and its body box; NULL when it takes no hits
// (CHARACTER_FLAG_NO_HIT) or has no shape on (CHARACTER_FLAG_SHADOW clear).
static HIT_TABLE *ssHitPrepareTarget(int chara)
{
    CHARACTER *t = &Character[chara];
    if ((t->flags & CHARACTER_FLAG_NO_HIT) != 0 || (t->flags & CHARACTER_FLAG_SHADOW) == 0)
        return NULL;
    HIT_TABLE *dmg = ssHitTable(t->bank);
    ssTransDmgHitDataL2W(ssHitParts(t->bank), dmg);
    ssHitMakeDamageOBB(dmg);
    return dmg;
}

// CT3 0x018aa370
// A scene bank's attack object (a->atk the bank, a->chara the object) strikes the scene bank a->bank
// (with SS_HIT_B0D_SCENE) and the character a->target, once: target is cleared when it is checked.
// callcheck: ssTransDmgHitDataL2W, ssHitMakeDamageOBB (ssHitPrepareTarget)
void ssHitCheckSceneAttack(SS_HIT_ATTACK *a)
{
    int bank = (u_short)a->atk;
    HIT_TABLE *hits = ssHitTable(bank);
    MODEL_PARTS *parts = ssHitParts(bank);
    if (a->chara >= hits->attackObjCount)
        return;
    const u_int *objs = BANK_PTR(const u_int *, bank, hits->attackObjs);
    HIT_OBJ *obj = BANK_PTR(HIT_OBJ *, bank, objs[a->chara]);
    if ((parts[obj->part].flags10 & PARTS_FLAG10_NO_HIT) != 0)
        return;
    ssTransScnAtkHitDataL2W(parts, obj);
    ssHitSetAttackPos(&parts[obj->part]);
    ssHitMakeAttackOBB(obj, 0);
    if ((a->b0d & SS_HIT_B0D_SCENE) != 0 && a->bank != -1) {
        HIT_TABLE *scn = ssHitTable(a->bank);
        ssTransScnDmgHitDataL2W(ssHitParts(a->bank), scn, SS_L2W_FRAME);
        ssHitCheckAttackToScn(obj, scn, NULL, a);
    }
    HIT_TABLE *dmg = ssHitPrepareTarget(a->target);
    if (dmg != NULL) {
        a->target = -1;
        ssHitCheckAttack(obj, dmg, a, NULL, hits);
    }
}

// CT3 0x018aa570
// A character's attack a->atk strikes the scene bank a->bank (with SS_HIT_B0D_SCENE, when it is a scene)
// and the other side's characters (charaAttr): a stalker the player, the player the stalkers.
// callcheck: ssTransDmgHitDataL2W, ssHitMakeDamageOBB (ssHitPrepareTarget)
void ssHitCheckCharacterAttack(SS_HIT_ATTACK *a)
{
    CHARACTER *c = &Character[a->chara];
    if ((c->flags & CHARACTER_FLAG_SHADOW) == 0)
        return;
    HIT_TABLE *atkHits = ssHitTable(c->bank);
    MODEL_PARTS *parts = ssHitParts(c->bank);
    HIT_OBJ *obj = ssTransAtkHitDataL2W(parts, atkHits, a->atk);
    if (obj == NULL)
        return;
    ssHitSetAttackPos(&parts[obj->part]);
    ssHitMakeAttackOBB(obj, 1);
    if ((a->b0d & SS_HIT_B0D_SCENE) != 0 && a->bank != -1 && BankInfo[a->bank].type == FS_LOAD_SCENE) {
        HIT_TABLE *scn = ssHitTable(a->bank);
        ssTransScnDmgHitDataL2W(ssHitParts(a->bank), scn, SS_L2W_FRAME);
        ssHitCheckAttackToScn(obj, scn, atkHits, a);
    }
    const short *targets = NULL;
    int count = 0;
    u_char attr = (u_char)charaAttr[a->chara];  // the ROM reads the low byte
    if (attr == CHARA_ATTR_STALKER) {
        targets = ssAtkCharaList1;
        count = ATK_CHARA_LIST1_COUNT;
    } else if (attr == CHARA_ATTR_PLAYER) {
        targets = ssAtkCharaList2;
        count = ATK_CHARA_LIST2_COUNT;
    }
    for (int i = 0; i < count; i++) {
        HIT_TABLE *dmg = ssHitPrepareTarget(targets[i]);
        if (dmg != NULL) {
            a->target = targets[i];
            ssHitCheckAttack(obj, dmg, a, atkHits, NULL);
        }
    }
}

// CT3 0x018b0860
// callcheck: gaInnerProductXYZ
float ssGetReflectVector3(float k, float *out, const float *n, const float *v)
{
    float d = k * gaInnerProductXYZ((float *)n, (float *)v);
    if (!(d <= 0.0f))
        return 0.0f;
    float r[4];
    for (int i = 0; i < 3; i++)
        r[i] = v[i] + n[i] * -d;
    r[3] = v[3];                        // vadd.xyz: v's w
    float len = sqrtf(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    ssVuUnitXYZ(r);
    memcpy(out, r, sizeof(r));
    return len;                         // 1 / (1 / |r|): VDIV of the VRSQRT
}


// ---- the box tests (slice 4, 2026-11-11): separating-axis tests of an attack / body box ----

// |a . b| over xyz (the ROM calls gaInnerProductXYZ, and again for the sign).
static float ssAbsDot(const float *a, const float *b)
{
    float d = gaInnerProductXYZ((float *)a, (float *)b);
    return d < 0.0f ? -d : d;
}

// VOPMULA.xyz / VOPMSUB.xyz: out = a x b (xyz; the sign never matters, every use is an |.|).
static void ssCross(float *out, const float *a, const float *b)
{
    out[0] = a[1] * b[2] - b[1] * a[2];
    out[1] = a[2] * b[0] - b[2] * a[0];
    out[2] = a[0] * b[1] - b[0] * a[1];
    out[3] = 0.0f;
}

// The box's axes scaled by its half sizes (VMULbc of each axis by its half).
static void ssObbScaledAxes(float (*s)[4], const SS_OBB *o)
{
    for (int i = 0; i < 3; i++)
        for (int k = 0; k < 4; k++)
            s[i][k] = o->axis[i][k] * o->half[i];
}

// CT3 0x018adf30
// callcheck: gaInnerProductXYZ
// The fifteen axes in the ROM's order: a's faces, b's faces, then a_i x b_j (i major). Each sum is added
// in the ROM's order; the first separating axis answers 0.
int ssHitCheckOBB_OBB(const SS_OBB *a, const SS_OBB *b)
{
    float t[4], as[3][4], bs[3][4], l[4];
    for (int k = 0; k < 4; k++)
        t[k] = b->center[k] - a->center[k];
    ssObbScaledAxes(as, a);
    ssObbScaledAxes(bs, b);
    for (int i = 0; i < 3; i++) {
        const float *ax = a->axis[i];
        float r = ssAbsDot(bs[2], ax) + a->half[i] + ssAbsDot(bs[0], ax) + ssAbsDot(bs[1], ax);
        if (r < ssAbsDot(t, ax))
            return 0;
    }
    for (int j = 0; j < 3; j++) {
        const float *ax = b->axis[j];
        float r = b->half[j] + ssAbsDot(as[2], ax) + ssAbsDot(as[0], ax) + ssAbsDot(as[1], ax);
        if (r < ssAbsDot(t, ax))
            return 0;
    }
    for (int i = 0; i < 3; i++) {
        int ilo = i == 0 ? 1 : 0, ihi = i == 2 ? 1 : 2;     // a's other two axes
        for (int j = 0; j < 3; j++) {
            int jlo = j == 0 ? 1 : 0, jhi = j == 2 ? 1 : 2; // b's other two
            ssCross(l, a->axis[i], b->axis[j]);
            float r = ssAbsDot(bs[jhi], l) + ssAbsDot(bs[jlo], l) + ssAbsDot(as[ilo], l) + ssAbsDot(as[ihi], l);
            if (r < ssAbsDot(t, l))
                return 0;
        }
    }
    return 1;
}

// CT3 0x018ad7a0
// callcheck: gaInnerProductXYZ
// Each side is a segment (centre worldV[i], unit direction worldV[3 + i], half length half[i]) tested
// against the box on seven axes: the box's three, box axis x side (three), the side itself. The first
// side no axis separates answers 1.
int ssHitCheckOBB_TriangleSides(const SS_OBB *box, const HIT_DATA *h)
{
    float bs[3][4], t[4], seg[4], l[4];
    ssObbScaledAxes(bs, box);
    for (int i = 0; i < 3; i++) {
        const float *dir = h->worldV[3 + i];
        for (int k = 0; k < 4; k++) {
            t[k] = h->worldV[i][k] - box->center[k];
            seg[k] = dir[k] * h->half[i];
        }
        int apart = 0;
        for (int a = 0; a < 3 && !apart; a++)
            apart = !(ssAbsDot(t, box->axis[a]) <= box->half[a] + ssAbsDot(seg, box->axis[a]));
        for (int a = 0; a < 3 && !apart; a++) {
            int alo = a == 0 ? 1 : 0, ahi = a == 2 ? 1 : 2;
            ssCross(l, box->axis[a], dir);
            apart = !(ssAbsDot(t, l) <= ssAbsDot(bs[alo], l) + ssAbsDot(bs[ahi], l));
        }
        if (apart)
            continue;
        float r = h->half[i] + ssAbsDot(bs[2], dir) + ssAbsDot(bs[0], dir) + ssAbsDot(bs[1], dir);
        if (ssAbsDot(t, dir) <= r)
            return 1;
    }
    return 0;
}

// CT3 0x018af580
// callcheck: gaInnerProductXYZ
// The quad as a flat box (centre worldV[0], axes worldV[3] / [4] by half[0] / half[1]) on eleven axes, in
// the ROM's order: the box's three, box axis i x quad axis (i major), the quad's two. Its normal is not
// one of them: ssHitCheckOneObject has already seen the corners cross the plane.
int ssHitCheckOBB_Plane(const SS_OBB *box, const HIT_DATA *h)
{
    float t[4], bs[3][4], qs[2][4], l[4];
    for (int k = 0; k < 4; k++) {
        t[k] = h->worldV[0][k] - box->center[k];
        qs[0][k] = h->worldV[3][k] * h->half[0];
        qs[1][k] = h->worldV[4][k] * h->half[1];
    }
    ssObbScaledAxes(bs, box);
    for (int a = 0; a < 3; a++) {
        const float *ax = box->axis[a];
        if (box->half[a] + ssAbsDot(qs[0], ax) + ssAbsDot(qs[1], ax) < ssAbsDot(t, ax))
            return 0;
    }
    for (int a = 0; a < 3; a++) {
        int alo = a == 0 ? 1 : 0, ahi = a == 2 ? 1 : 2;
        for (int q = 0; q < 2; q++) {
            ssCross(l, box->axis[a], h->worldV[3 + q]);
            if (ssAbsDot(qs[1 - q], l) + ssAbsDot(bs[alo], l) + ssAbsDot(bs[ahi], l) < ssAbsDot(t, l))
                return 0;
        }
    }
    for (int q = 0; q < 2; q++) {
        const float *ax = h->worldV[3 + q];
        if (h->half[q] + ssAbsDot(bs[2], ax) + ssAbsDot(bs[0], ax) + ssAbsDot(bs[1], ax) < ssAbsDot(t, ax))
            return 0;
    }
    return 1;
}

// Above the running maximum, a corner's distance is pushed up by this (the ROM keeps it in the list the
// edge search and the crossing read; it never changes a sign).
#define SS_HIT_PLANE_BIAS 0.3f

// The shape the caller stands on is tested `grow` wider along its first n half sizes.
static void ssHitGrowShape(HIT_DATA *h, int n, float grow)
{
    for (int k = 0; k < n; k++)
        h->half[k] += grow;
}

// CT3 0x018b0350
// callcheck: gaInnerProductXYZ, ssHitCheckOBB_TriangleSides, ahAcos, ssHitCheckOBB_Plane
int ssHitCheckOneObject(HIT_OBJ *obj, HIT_DATA *h, const float *v, float *out, HIT_DATA *self)
{
    const SS_OBB *box = HIT_OBJ_OBB(obj);
    // d[i + 1]: corner i's distance below h's plane; edge: the first i whose step from corner i - 1
    // changes side (d[0] is 0, so a path starting on the plane does not count).
    float d[9];
    float top = 0.0f;
    int edge = 0;
    d[0] = 0.0f;
    for (int i = 0; i < 8; i++) {
        d[i + 1] = h->planeD - gaInnerProductXYZ(h->worldN, (float *)box->corner[i]);
        if (edge == 0 && d[i] * d[i + 1] < 0.0f)
            edge = i;
        if (top < d[i + 1]) {
            top = d[i + 1] + SS_HIT_PLANE_BIAS;
            d[i + 1] = top;
        }
    }
    if (edge == 0)
        return 0;
    if ((h->flags & HIT_DATA_FLAG_TRIANGLE) == 0) {
        if (h == self)
            ssHitGrowShape(h, 2, SS_HIT_SELF_GROW);
        int met = ssHitCheckOBB_Plane(box, h);
        if (h == self)
            ssHitGrowShape(h, 2, -SS_HIT_SELF_GROW);
        if (!met)
            return 0;
    } else {
        if (h == self)
            ssHitGrowShape(h, 3, SS_HIT_SELF_GROW);
        int met = ssHitCheckOBB_TriangleSides(box, h);
        if (h == self)
            ssHitGrowShape(h, 3, -SS_HIT_SELF_GROW);
        if (!met) {
            // Does the crossing edge pierce the triangle? Where it meets the plane, the corners must be
            // seen all round (the CheckLOS angle test).
            const float *p0 = box->corner[edge - 1];
            float e[4], x[4], u[3][4];
            for (int k = 0; k < 3; k++)
                e[k] = box->corner[edge][k] - p0[k];
            e[3] = box->corner[edge][3];
            float t = (float)LosEeDiv(d[edge], gaInnerProductXYZ(h->worldN, e));
            if (t < 0.0f || 1.0f < t)
                return 0;
            for (int k = 0; k < 3; k++)
                x[k] = e[k] * t + p0[k];
            x[3] = e[3];
            for (int c = 0; c < 3; c++) {
                for (int k = 0; k < 3; k++)
                    u[c][k] = h->world[c][k] - x[k];
                u[c][3] = h->world[c][3];
                ssVuUnitXYZ(u[c]);
            }
            int seen = ahAcos(gaInnerProductXYZ(u[0], u[1]));
            seen += ahAcos(gaInnerProductXYZ(u[1], u[2]));
            seen += ahAcos(gaInnerProductXYZ(u[2], u[0]));
            if (!(LOS_INSIDE_ANGLE <= (float)seen))
                return 0;
        }
    }
    if (v != NULL) {
        float k = gaInnerProductXYZ((float *)v, h->worldN);
        for (int i = 0; i < 3; i++)
            out[i] = h->worldN[i] * -k + v[i];
        out[3] = h->worldN[3];          // vmulx.xyz / vadd.xyz on the loaded normal: its w
    }
    return 1;
}

// ---- a character's box and the scene grid (slice 5, 2026-11-11) ----

// ssMakeCharOBB's centre-and-axes matrix before the centre goes in (@881, data 0x019398f0): row 0 the
// centre, rows 1..3 the unit axes (w 1 as the ROM has them).
static const float ssCharObbAxes[3][4] = { { 1, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 1 } };
#define SS_OBB_MOVE_EPS 0.001f          /* a move this long or shorter faces nowhere: the character's rot */

// An angle in radians as a 0x10000-per-turn angle (the ROM's software doubles: dpmul, dpdiv, dptoli).
static int ssRadianToAngle(double r)
{
    return (int)(long long)((r * 65536.0) / 6.283185307179586) & 0xffff;
}

// CT3 0x018acdc0
// callcheck: dpmul, dpdiv, dptoli (ssRadianToAngle: the host's double)
void ssMakeCharOBB(CHARACTER *chara, HIT_TABLE *hits, int grow, int *yaw, int *pitch)
{
    float rot[4][4], inv[4][4], t[4];
    float m[4][4];                      // row 0 the centre, rows 1..3 the axes
    memcpy(m[1], ssCharObbAxes, sizeof(ssCharObbAxes));
    m[0][0] = m[0][1] = m[0][2] = m[0][3] = 0.0f;

    for (int k = 0; k < 3; k++)
        t[k] = ssHitNPos[k] - ssHitBPos[k];
    t[3] = ssHitNPos[3];                // vsub.xyz: w is N's
    float len = sqrtf(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
    if (SS_OBB_MOVE_EPS < len) {
        float q = 1.0f / len;           // vrsqrt
        for (int k = 0; k < 3; k++)
            t[k] *= q;
        if (*yaw == -1)
            *yaw = ssRadianToAngle(ssRadianByO(t, 1));
        gaSetRotMatrixY(rot, *yaw);
        if (*pitch != 0) {
            ssApplyMatrix(t, (const float (*)[4])rot, t);
            if (*pitch == -1)
                *pitch = ssRadianToAngle(ssRadianByO(t, 0));
            gaSetRotMatrixX(inv, *pitch);
            gaMulMatrix(rot, inv, rot);
        }
    } else {
        if (*yaw == -1)
            *yaw = -chara->rot;
        if (*pitch == -1)
            *pitch = 0;
        gaSetRotMatrixY(rot, *yaw);
    }
    gaInversMatrix(inv, rot);

    // The collision points' bounds in the turned frame.
    int bank = BankOfHostPtr(hits);     // PORT: colWorld is an EE word of the hit table's bank
    const float (*pts)[4] = BANK_PTR(const float (*)[4], bank, hits->colWorld);
    float mx[4], mn[4];
    ssApplyMatrix(mn, (const float (*)[4])rot, pts[0]);
    memcpy(mx, mn, sizeof(mx));
    for (int i = 1; i < hits->colCount; i++) {
        ssApplyMatrix(t, (const float (*)[4])rot, pts[i]);
        for (int k = 0; k < 3; k++) {
            if (mx[k] < t[k])
                mx[k] = t[k];
            else if (!(mn[k] <= t[k]))
                mn[k] = t[k];
        }
    }
    if (grow)
        mx[2] += len;                   // the box reaches as far ahead as the move goes

    SS_OBB *obb = CHARA_OBB(chara);
    memcpy(chara->obbPrev, chara->obb, sizeof(chara->obbPrev));
    for (int k = 0; k < 3; k++) {
        t[k] = (mx[k] - mn[k]) * 0.5f;  // t's w stays: the last point's (or the move's)
        m[0][k] = t[k] + mn[k];
    }
    m[0][3] = t[3];                     // vadd.xyz on the half: its w
    memcpy(obb->half, t, sizeof(obb->half));
    chara->obb[12][3] = t[3];           // the ROM stores the half as a whole qword
    const float c[8][3] = {
        { mn[0], mn[1], mn[2] }, { mx[0], mn[1], mn[2] }, { mx[0], mx[1], mn[2] }, { mn[0], mx[1], mn[2] },
        { mn[0], mx[1], mx[2] }, { mn[0], mn[1], mx[2] }, { mx[0], mn[1], mx[2] }, { mx[0], mx[1], mx[2] },
    };
    for (int i = 0; i < 8; i++) {
        obb->corner[i][0] = c[i][0];
        obb->corner[i][1] = c[i][1];
        obb->corner[i][2] = c[i][2];
        obb->corner[i][3] = 1.0f;
    }
    gaMulMatrix(obb->corner, inv, obb->corner);         // back into the world: corners 0..3
    gaMulMatrix(obb->corner + 4, inv, obb->corner + 4); // 4..7
    gaMulMatrix(&chara->obb[8], inv, m);                // centre and axes (obb rows 8..11)
}

// The grid cells [x0, x1) x [z0, z1) an XZ box covers, clamped to the grid (the ROM inlines it in both
// cell functions). Part 0's position places the grid.
static void ssGridCellRange(const HIT_SCENE_GRID *g, const MODEL_PARTS *parts, float xMin, float xMax,
                            float zMin, float zMax, int *x0, int *x1, int *z0, int *z1)
{
    const float *base = (const float *)((const u_char *)parts + PARTS_ANIM_POS);
    float ox = g->originX + base[0];
    float oz = g->originZ + base[2];
    int a = (int)((xMin - ox) / g->cellW);
    int b = (int)((xMax - ox) / g->cellW) + 1;
    int c = (int)((zMin - oz) / g->cellD);
    int d = (int)((zMax - oz) / g->cellD) + 1;
    *x0 = a < 0 ? 0 : (a > HIT_GRID_N - 1 ? HIT_GRID_N - 1 : a);
    *x1 = b < 1 ? 1 : (b > HIT_GRID_N ? HIT_GRID_N : b);
    *z0 = c < 0 ? 0 : (c > HIT_GRID_N - 1 ? HIT_GRID_N - 1 : c);
    *z1 = d < 1 ? 1 : (d > HIT_GRID_N ? HIT_GRID_N : d);
}

// The XZ bounds of n points (the ROM's "less than the minimum, else more than the maximum").
static void ssXZBounds(const float (*p)[4], int n, float *xMin, float *xMax, float *zMin, float *zMax)
{
    *xMin = *xMax = p[0][0];
    *zMin = *zMax = p[0][2];
    for (int i = 1; i < n; i++) {
        if (p[i][0] < *xMin)
            *xMin = p[i][0];
        else if (*xMax < p[i][0])
            *xMax = p[i][0];
        if (p[i][2] < *zMin)
            *zMin = p[i][2];
        else if (*zMax < p[i][2])
            *zMax = p[i][2];
    }
}

// CT3 0x018abf00
void ssGetCharSpaceCell(CHARACTER *chara, MODEL_PARTS *parts, HIT_SCENE_GRID *grid)
{
    float xMin, xMax, zMin, zMax;
    int x0, x1, z0, z1;
    ssXZBounds((const float (*)[4])CHARA_OBB(chara)->corner, 8, &xMin, &xMax, &zMin, &zMax);
    ssGridCellRange(grid, parts, xMin, xMax, zMin, zMax, &x0, &x1, &z0, &z1);
    for (int z = z0; z < z1; z++)
        for (int x = x0; x < x1; x++)
            chara->cells[chara->cellCount++] = (u_char)(x + z * HIT_GRID_N);
}

// CT3 0x018abc90
void ssDivideHitDataToSpaceCell(MODEL_PARTS *parts, HIT_SCENE_GRID *grid)
{
    int bank = BankOfHostPtr(grid);     // PORT: the words are EE addresses of the grid's bank
    const u_int *list = BANK_PTR(const u_int *, bank, grid->hitData);
    for (short n = 0; list[n] != 0; n++) {
        const HIT_DATA *h = BANK_PTR(const HIT_DATA *, bank, list[n]);
        if (h->kind != SS_L2W_FRAME || (parts[h->part].flags10 & PARTS_FLAG10_NO_HIT) != 0)
            continue;
        float xMin, xMax, zMin, zMax;
        int x0, x1, z0, z1;
        ssXZBounds((const float (*)[4])h->world, 4, &xMin, &xMax, &zMin, &zMax); // 4 also for a triangle
        ssGridCellRange(grid, parts, xMin, xMax, zMin, zMax, &x0, &x1, &z0, &z1);
        for (int z = z0; z < z1; z++) {
            for (int x = x0; x < x1; x++) {
                int cell = x + z * HIT_GRID_N;
                u_short *idx = BANK_PTR(u_short *, bank, grid->cellB[cell]);
                idx[grid->cellBCount[cell]++] = (u_short)n;
            }
        }
    }
}

// One cell list's shapes into the check list (the ROM's two copies of one loop).
static void ssAddCellToCheckList(const CHARACTER *chara, const MODEL_PARTS *parts, HIT_SCENE_GRID *grid,
                                 int bank, u_int idxEe, int count)
{
    const u_int *list = BANK_PTR(const u_int *, bank, grid->hitData);
    u_int *check = BANK_PTR(u_int *, bank, grid->checkList);
    const u_short *idx = BANK_PTR(const u_short *, bank, idxEe);
    int isPlayer = chara->id == Player_BankID;
    for (int j = 0; j < count; j++) {
        u_int ee = list[idx[j]];
        const HIT_DATA *h = BANK_PTR(const HIT_DATA *, bank, ee);
        u_int f = h->flags;
        if ((f & (HIT_DATA_FLAG_NO_COLLIDE | HIT_DATA_FLAG_CHECKING)) != 0 ||
            (parts[h->part].flags10 & PARTS_FLAG10_NO_HIT) != 0 ||
            ((f & HIT_DATA_FLAG_NOT_PLAYER) != 0 && isPlayer) ||
            ((f & HIT_DATA_FLAG_PLAYER_ONLY) != 0 && !isPlayer))
            continue;
        int m = 0;
        while (m < grid->checkCount && check[m] != ee)
            m++;
        if (m == grid->checkCount)
            check[grid->checkCount++] = ee; // the list keeps EE words, as the ROM's
    }
}

// CT3 0x018ab9c0
void ssAddHitDataToCheckList(CHARACTER *chara, MODEL_PARTS *parts, HIT_SCENE_GRID *grid, int withMoving)
{
    int bank = BankOfHostPtr(grid);     // PORT: the words are EE addresses of the grid's bank
    int n = chara->cellCount;
    for (int i = 0; i < n; i++) {
        int cell = chara->cells[i];
        ssAddCellToCheckList(chara, parts, grid, bank, grid->cellA[cell], grid->cellACount[cell]);
        if (withMoving)
            ssAddCellToCheckList(chara, parts, grid, bank, grid->cellB[cell], grid->cellBCount[cell]);
    }
}

// ---- the slide along the shapes the box meets (slice 6, 2026-11-12) ----

// 1 << i: the bisection step's divisor (data 0x019398d0, 16 shorts; ssHitReCheckAllObject reads 0..5).
const u_short ssPow_2data[16] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };

#define SS_HIT_FACE_MOVE_MIN 0.1f       /* a move shorter than this faces along the box's third axis */
#define SS_HIT_VEL_MIN2 0.01f           /* |vel|^2 above: a moving shape is tested whichever way it faces */
#define SS_HIT_RECHECK_START (-2.0f)    /* ssHitCheckAllObject's first bisection parameter */

// CT3 0x018b0950
// The shapes of c[0..n) that the box still meets when its centre is moved to base + c.move * (t - 1);
// those go to the front. Two or more: back off by 2 / 2^depth; none: forward by as much; until depth 6,
// where c[0] becomes the shape the character stands on (self) when it is among them, else the one whose
// normal faces the move most (the lowest dot; c[0]'s dot is not rewritten).
void ssHitReCheckAllObject(float t, CHARACTER *chara, const float *base, SS_HIT_CAND *c, int n, int depth,
                           HIT_DATA *self)
{
    if (depth >= SS_HIT_RECHECK_DEPTH) {
        float best = c[0].dot;
        for (int i = 1; i < n; i++) {
            if (c[i].hit == self) {
                c[0].hit = self;
                memcpy(c[0].out, c[i].out, sizeof(c[0].out));
                memcpy(c[0].move, c[i].move, sizeof(c[0].move));
                return;
            }
            if (c[i].dot <= best) {
                c[0].hit = c[i].hit;
                best = c[i].dot;
                memcpy(c[0].out, c[i].out, sizeof(c[0].out));
                memcpy(c[0].move, c[i].move, sizeof(c[0].move));
            }
        }
        return;
    }
    SS_OBB *box = CHARA_OBB(chara);
    int kept = 0;
    for (int i = 0; i < n; i++) {
        for (int k = 0; k < 3; k++)
            box->center[k] = c[i].move[k] * (t - 1.0f) + base[k];
        box->center[3] = c[i].move[3];  // vmulx.xyz / vadd.xyz on the loaded move: its w is stored
        if (ssHitCheckOneObject((HIT_OBJ *)box, c[i].hit, NULL, NULL, self) != 0) {
            c[kept].hit = c[i].hit;
            c[kept].dot = c[i].dot;
            memcpy(c[kept].out, c[i].out, sizeof(c[kept].out));
            memcpy(c[kept].move, c[i].move, sizeof(c[kept].move));
            kept++;
        }
    }
    float step = 2.0f / (float)ssPow_2data[depth];
    if (kept >= 2)
        ssHitReCheckAllObject(t - step, chara, base, c, kept, depth + 1, self);
    else if (kept == 0)
        ssHitReCheckAllObject(t + step, chara, base, c, n, depth + 1, self);
}

// CT3 0x018b0c30
// The shape of the grid's check list the character's box meets on its move ssHitBPos -> ssHitNPos (NULL
// for none), with *out the move slid along it. A still shape counts only when it faces the move (its
// normal . the move's direction < 0); a moving one (SS_L2W_FRAME, |vel|^2 > 0.01) always, its move taken
// relative to its own. When several meet, ssHitReCheckAllObject picks one by moving the box's centre
// along the move (put back afterwards).
// callcheck: gaInnerProductXYZ, ssHitCheckOneObject, ssHitReCheckAllObject
HIT_DATA *ssHitCheckAllObject(CHARACTER *chara, HIT_SCENE_GRID *grid, float *out, HIT_DATA *self)
{
    SS_OBB *box = CHARA_OBB(chara);
    SS_HIT_CAND cand[SS_HIT_CAND_MAX];
    float move[4], dir[4], slid[4];
    float dot = 1.0f;
    for (int k = 0; k < 3; k++)
        move[k] = ssHitNPos[k] - ssHitBPos[k];
    move[3] = ssHitNPos[3];             // vsub.xyz: w is the register's, N's
    memcpy(dir, move, sizeof(dir));
    float len2 = move[0] * move[0] + move[1] * move[1] + move[2] * move[2];
    ssVuUnitXYZ(dir);
    if (sqrtf(len2) < SS_HIT_FACE_MOVE_MIN)
        memcpy(dir, box->axis[2], sizeof(dir));
    int bank = BankOfHostPtr(grid);     // PORT: the list holds EE words of the grid's bank
    const u_int *list = BANK_PTR(const u_int *, bank, grid->checkList);
    int n = 0;
    for (int i = 0; i < grid->checkCount; i++) {
        HIT_DATA *h = BANK_PTR(HIT_DATA *, bank, list[i]);
        int test = h->kind == SS_L2W_FRAME && !(gaInnerProductXYZ(h->vel, h->vel) <= SS_HIT_VEL_MIN2);
        if (!test) {
            dot = gaInnerProductXYZ(h->worldN, dir);
            test = dot < 0.0f;
        }
        if (!test || ssHitCheckOneObject((HIT_OBJ *)box, h, move, slid, self) == 0)
            continue;
        if (n >= SS_HIT_CAND_MAX)
            break;                      // PORT: the ROM's 0xc00 stack bytes hold 64 and are not checked
        SS_HIT_CAND *c = &cand[n++];
        c->hit = h;
        memcpy(c->out, slid, sizeof(c->out));
        if (h->kind == SS_L2W_FRAME) {
            float u[4];
            for (int k = 0; k < 3; k++)
                c->move[k] = move[k] - h->vel[k];
            c->move[3] = move[3];
            memcpy(u, c->move, sizeof(u));
            ssVuUnitXYZ(u);
            c->dot = gaInnerProductXYZ(h->worldN, u);
        } else {
            memcpy(c->move, move, sizeof(c->move));
            c->dot = dot;
        }
    }
    if (n == 0)
        return NULL;
    if (n >= 2) {
        float base[4];
        memcpy(base, box->center, sizeof(base));
        ssHitReCheckAllObject(SS_HIT_RECHECK_START, chara, base, cand, n, 0, self);
        memcpy(box->center, base, sizeof(base));
    }
    memcpy(out, cand[0].out, sizeof(cand[0].out));
    return cand[0].hit;
}

// dst.xyz = a.xyz + b.xyz, dst.w = a.w (the VU's vadd.xyz on a register loaded from a; dst may be b).
static void ssVAddXYZ(float *dst, const float *a, const float *b)
{
    float r[4] = { a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] };
    memcpy(dst, r, sizeof(r));
}

static const float ssVuZero[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; /* vf0 */

#define SS_SLIDE_GAP 0.3f               /* how far off the shape's plane the box is put back */
#define SS_SLIDE_MIN 0.001f             /* a move or its part along the normal at or below this is none */
#define SS_SLIDE_SPLIT_MIN2 0.01f       /* |part|^2 above: ssHitCheckAndProcess moves along that part again */
#define SS_SLIDE_FACING_STEEP (-0.4f)   /* facing . a floor's normal below: the slide is stretched by 1.5 */
#define SS_SLIDE_STEEP_SCALE 1.5f

// CT3 0x018b1390
// The box (obb) has met h on the move ssHitBPos -> ssHitNPos: put ssHitBPos where the box just clears h's
// plane (SS_SLIDE_GAP off it, counting the box's reach along the normal), moving the character's
// collision points (hits) with it, then aim ssHitNPos along *slid from there -- shortened by what the
// put-back already covered, or nothing for SS_STOP shapes (and STAND ones the slide goes down).
// callcheck: gaInnerProductXYZ, ssMoveAllHitObjCoordinate
void ssHitCalcSlidePos2(HIT_TABLE *hits, SS_OBB *obb, HIT_DATA *h, float *slid)
{
    float u[4], c[4], a[3][4], n[4];
    for (int k = 0; k < 3; k++)
        u[k] = ssHitNPos[k] - ssHitBPos[k];
    u[3] = ssHitNPos[3];
    float len = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    ssVuUnitXYZ(u);
    memcpy(c, obb->center, sizeof(c));
    for (int i = 0; i < 3; i++) {
        for (int k = 0; k < 3; k++)
            a[i][k] = obb->axis[i][k] * obb->half[i];
        a[i][3] = obb->axis[i][3];
    }
    memcpy(n, h->worldN, sizeof(n));
    // The box's reach along the normal (the ROM adds axis 1's, axis 0's, then axis 2's).
    float r1 = fabsf(gaInnerProductXYZ(a[1], n));
    float r0 = fabsf(gaInnerProductXYZ(a[0], n));
    float r2 = fabsf(gaInnerProductXYZ(a[2], n));
    float reach = r2 + (r0 + r1);
    float s;
    if (len == 0.0f) {
        s = (SS_SLIDE_GAP + (h->planeD + reach)) - gaInnerProductXYZ(n, c);
        for (int k = 0; k < 3; k++)
            u[k] = n[k] * s;
        u[3] = n[3];
    } else {
        float k0 = gaInnerProductXYZ(n, u);
        if (fabsf(k0) <= SS_SLIDE_MIN) {
            memcpy(u, ssVuZero, sizeof(u));
        } else {
            s = (SS_SLIDE_GAP + (h->planeD + reach)) - gaInnerProductXYZ(n, c);
            float t = s / k0;
            if (t <= 0.0f) {
                for (int k = 0; k < 3; k++)
                    u[k] = n[k] * s;
                u[3] = n[3];
            } else {
                for (int k = 0; k < 3; k++)
                    u[k] *= t;
                float back = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
                if (len < back) {
                    // The put-back is longer than the move: the move stays as it was, nothing moves.
                    for (int k = 0; k < 3; k++)
                        slid[k] = ssHitNPos[k] - ssHitBPos[k];
                    slid[3] = ssHitNPos[3];
                    return;
                }
                float f = (len - back) / len;
                for (int k = 0; k < 3; k++)
                    slid[k] *= f;
            }
        }
    }
    ssVAddXYZ(ssHitBPos, u, ssHitBPos);
    ssMoveAllHitObjCoordinate(hits, u);
    if ((h->flags & HIT_DATA_FLAG_STOP) != 0 ||
        ((h->flags & HIT_DATA_FLAG_STAND) != 0 && slid[1] < 0.0f)) {
        memcpy(ssHitNPos, ssHitBPos, sizeof(ssHitNPos));
        memcpy(slid, ssVuZero, sizeof(ssVuZero));
        return;
    }
    ssVAddXYZ(ssHitNPos, slid, ssHitBPos);
}

// ssMakeCharOBB with the box facing ssHitBPos -> ssHitBPos + toward (ssHitNPos put back afterwards).
static void ssMakeCharOBBFacing(CHARACTER *chara, HIT_TABLE *hits, const float *toward, int *yaw, int *pitch)
{
    float keep[4];
    memcpy(keep, ssHitNPos, sizeof(keep));
    ssVAddXYZ(ssHitNPos, toward, ssHitBPos);
    ssMakeCharOBB(chara, hits, 0, yaw, pitch);
    memcpy(ssHitNPos, keep, sizeof(keep));
}

// CT3 0x018b0f30
// The character's move ssHitBPos -> ssHitNPos against the scene grid's shapes (hits: its shape bank's
// HIT_TABLE, parts: the scene bank's parts): the box stretched along the move, its cells' shapes (the
// moving ones too below depth 5), the one it meets (ssHitCheckAllObject); then the box is rebuilt facing
// the slide (STAND shapes: facing the normal, a FLOOR's slide stretched when the character faces down
// it), put back off the shape (ssHitCalcSlidePos2), and the slide is tried again, horizontal part then
// vertical, with the shape left out (CHECKING). Answers the last shape met, NULL for none; depth 6 stops.
// callcheck: ssMakeCharOBB, gaSetRotMatrixY, gaInnerProductXYZ, ssHitCalcSlidePos2, ssSplitVectorHandV, ssMoveAllHitObjCoordinate
HIT_DATA *ssHitCheckAndProcess(CHARACTER *chara, HIT_SCENE_GRID *grid, HIT_TABLE *hits, MODEL_PARTS *parts,
                               int *yaw, int *pitch, int depth, HIT_DATA *self)
{
    if (depth >= SS_HIT_PROCESS_DEPTH)
        return NULL;
    ssMakeCharOBB(chara, hits, 1, yaw, pitch);
    chara->cellCount = 0;
    grid->checkCount = 0;
    ssGetCharSpaceCell(chara, parts, grid);
    ssAddHitDataToCheckList(chara, parts, grid, depth == SS_HIT_PROCESS_DEPTH - 1 ? 0 : 1);
    float slid[4];
    HIT_DATA *h = ssHitCheckAllObject(chara, grid, slid, self);
    if (h == NULL)
        return NULL;
    HIT_DATA *met = h;
    if ((h->flags & HIT_DATA_FLAG_STAND) != 0) {
        if ((h->flags & LOS_HIT_FLAG_FLOOR) != 0) {
            float m[4][4];
            gaSetRotMatrixY(m, chara->rot);
            if (gaInnerProductXYZ(m[2], h->worldN) < SS_SLIDE_FACING_STEEP)
                for (int k = 0; k < 3; k++)
                    slid[k] *= SS_SLIDE_STEEP_SCALE;
        }
        *yaw = -1;
        *pitch = -1;
        ssMakeCharOBBFacing(chara, hits, h->worldN, yaw, pitch);
    } else {
        *yaw = -1;
        *pitch = -1;
        float dir[4];
        memcpy(dir, slid, sizeof(dir));
        float len = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        ssVuUnitXYZ(dir);
        if (len <= SS_SLIDE_MIN)
            ssMakeCharOBB(chara, hits, 0, yaw, pitch);
        else
            ssMakeCharOBBFacing(chara, hits, dir, yaw, pitch);
    }
    ssHitCalcSlidePos2(hits, CHARA_OBB(chara), h, slid);
    float hv[4], vv[4];
    ssSplitVectorHandV(hv, vv, slid);
    h->flags |= HIT_DATA_FLAG_CHECKING;
    if (!(gaInnerProductXYZ(hv, hv) <= SS_SLIDE_SPLIT_MIN2)) {
        ssVAddXYZ(ssHitNPos, hv, ssHitBPos);
        *yaw = -1;
        *pitch = 0;
        HIT_DATA *r = ssHitCheckAndProcess(chara, grid, hits, parts, yaw, pitch, depth + 1, self);
        if (r != NULL)
            met = r;
        float d[4];
        for (int k = 0; k < 3; k++)
            d[k] = ssHitNPos[k] - ssHitBPos[k];
        d[3] = ssHitNPos[3];
        if (gaInnerProductXYZ(d, d) != 0.0f) {
            ssVAddXYZ(ssHitBPos, d, ssHitBPos);
            ssMoveAllHitObjCoordinate(hits, d);
        }
    }
    if (!(gaInnerProductXYZ(vv, vv) <= SS_SLIDE_SPLIT_MIN2)) {
        ssVAddXYZ(ssHitNPos, vv, ssHitBPos);
        *pitch = -1;                    // the ROM leaves *yaw as the last call left it
        HIT_DATA *r = ssHitCheckAndProcess(chara, grid, hits, parts, yaw, pitch, depth + 1, self);
        if (r != NULL)
            met = r;
    }
    h->flags &= ~HIT_DATA_FLAG_CHECKING;
    return met;
}

// ---- the frame's collision of one character (slice 6, 2026-11-12) ----

#define SS_FALL_STEP 3.0f               /* the fall per CHARACTER.fallSteps (b2f1) */
#define SS_FALL_STEPS_LANDED 3          /* b2f1 when a floor is met */
#define SS_FALL_FRAMES_MAX 0xfe         /* b2f2 stops counting here */
#define SS_STAND_MOVE_SCALE 0.8f        /* an AI character's move on a STAND floor */
#define SS_MOVE_MIN2 0.001f             /* |pos change|^2 at or below: the character is not moved */

// The scene shape an EE word of the character's w300[] names in its scene bank (NULL for 0).
static HIT_DATA *ssCharaShape(int bank, int ee)
{
    return BANK_PTR(HIT_DATA *, bank, (u_int)ee);
}

// CT3 0x018b1840
// a (Alyssa) against b (another character in the same scene): their boxes (a's stretched along its move
// ssHitBPos -> ssHitNPos, b's where it stands) overlap in XZ (half[2] each, as radii) and in y (half[1]):
// ssHitNPos is pushed out along the XZ line between the centres by the overlap -- or, while Alyssa
// crawls, she stays where she was (ssHitBPos and ssHitNPos back to posBefore, rot to rotBefore). hits: a's
// shape bank's HIT_TABLE. 1 when they overlap.
// callcheck: ssMakeCharOBB, ssAI_IsHaiHaiPlaying
int ssColCheckChar_Chara(CHARACTER *a, CHARACTER *b, HIT_TABLE *hits)
{
    if (a->lightBank != b->lightBank)
        return 0;
    int yaw = 0, pitch = 0;
    ssMakeCharOBB(a, hits, 1, &yaw, &pitch);
    float keep[4];
    memcpy(keep, ssHitNPos, sizeof(keep));
    memcpy(ssHitNPos, ssHitBPos, sizeof(ssHitNPos));
    ssMakeCharOBB(b, BANK_PTR(HIT_TABLE *, b->bank, All_table_TBL[b->bank]->hits), 0, &yaw, &pitch);
    memcpy(ssHitNPos, keep, sizeof(keep));
    const SS_OBB *ba = CHARA_OBB(a), *bb = CHARA_OBB(b);
    float d[4];
    for (int k = 0; k < 3; k++)
        d[k] = ba->center[k] - bb->center[k];
    d[3] = ba->center[3];
    float dy = fabsf(d[1]);
    d[1] = 0.0f;
    float dist = sqrtf(d[0] * d[0] + d[2] * d[2]);
    ssVuUnitXYZ(d);
    float over = (ba->half[2] + bb->half[2]) - dist;
    if (over <= 0.0f || ba->half[1] + bb->half[1] <= dy)
        return 0;
    if (a->id == 0 && ssAI_IsHaiHaiPlaying(a) != 0) {
        memcpy(ssHitBPos, &a->posBefore, sizeof(ssHitBPos));
        memcpy(ssHitNPos, &a->posBefore, sizeof(ssHitNPos));
        a->rot = a->rotBefore;
    } else {
        for (int k = 0; k < 3; k++)
            d[k] *= over;
        ssVAddXYZ(ssHitNPos, d, ssHitNPos);
    }
    return 1;
}

// CT3 0x018b1a40
// One character's move this frame (posBefore -> pos) against the scene bank `bank` (its lightBank; list:
// the frame's colliding characters, Alyssa is pushed off the others first): the horizontal part slid
// along the walls (w300[1] = the wall met, the last frame's w300[3] leaves its own widened), then the
// vertical part less the fall (fallSteps * 3) against the floors (w300[0] = the floor stood on: fallSteps
// back to 3; none: the fall grows, 2 n + 3 for n frames in the air). A move of more than ~0.03 is
// applied (pos, asCharacterPositioningOnlyDeltaPos). The bank's moving shapes go to the world and the
// grid once per frame (gSceneL2WStatus).
// callcheck: ssTransColHitDataL2W, ssColCheckChar_Chara, ssSplitVectorHandV, ssTransHitDataL2W, ssDivideHitDataToSpaceCell, ssHitCheckAndProcess, ssAI_IsHaiHaiPlaying, gaInnerProductXYZ, ssMoveAllHitObjCoordinate, asCharacterPositioningOnlyDeltaPos
void ssCollisionCheck(struct CHARACTER *chara, int bank, SS_HIT_CHARA *list, int count)
{
    HIT_TABLE *scn = BANK_PTR(HIT_TABLE *, bank, All_table_TBL[bank]->hits);
    HIT_SCENE_GRID *grid = (HIT_SCENE_GRID *)scn;
    MODEL_PARTS *sparts = (MODEL_PARTS *)(pModelPartsHeader[bank] + PARTS_HDR_RECORDS);
    int cb = chara->bank;
    HIT_TABLE *hits = BANK_PTR(HIT_TABLE *, cb, All_table_TBL[cb]->hits);
    ssTransColHitDataL2W((MODEL_PARTS *)(pModelPartsHeader[cb] + PARTS_HDR_RECORDS), hits);
    memcpy(ssHitNPos, &chara->pos, sizeof(ssHitNPos));
    memcpy(ssHitBPos, &chara->posBefore, sizeof(ssHitBPos));
    if (chara == &Character[0])
        for (int i = 0; i < count; i++)
            if (list[i].chara != chara)
                ssColCheckChar_Chara(chara, list[i].chara, hits);

    float move[4], hv[4], vv[4];
    for (int k = 0; k < 3; k++)
        move[k] = ssHitNPos[k] - ssHitBPos[k];
    move[3] = ssHitNPos[3];
    if (chara->brain != 0 && chara->w300[CHARA_HIT_FLOOR_PREV] != 0 &&
        (ssCharaShape(bank, chara->w300[CHARA_HIT_FLOOR_PREV])->flags & HIT_DATA_FLAG_STAND) != 0) {
        for (int k = 0; k < 3; k++)
            move[k] *= SS_STAND_MOVE_SCALE;
        ssVAddXYZ(ssHitNPos, move, ssHitBPos);
    }
    ssSplitVectorHandV(hv, vv, move);
    if (gSceneL2WStatus[bank] == 0) {
        gSceneL2WStatus[bank] = 1;
        memset(grid->cellBCount, 0, sizeof(grid->cellBCount));
        ssTransHitDataL2W(sparts, scn, SS_L2W_FRAME);
        ssDivideHitDataToSpaceCell(sparts, grid);
    }

    // The horizontal part, against the walls.
    ssVAddXYZ(ssHitNPos, hv, ssHitBPos);
    int yaw = -1, pitch = 0;
    HIT_DATA *wall = ssHitCheckAndProcess(chara, grid, hits, sparts, &yaw, &pitch, 0,
                                          ssCharaShape(bank, chara->w300[CHARA_HIT_WALL_PREV]));
    chara->w300[CHARA_HIT_WALL] = (int)BankEeAddr(bank, wall); // PORT: the word stays the shape's EE address
    if (wall != NULL && chara->id == 0 && ssAI_IsHaiHaiPlaying(chara) != 0) {
        memcpy(ssHitBPos, &chara->posBefore, sizeof(ssHitBPos));
        memcpy(ssHitNPos, &chara->posBefore, sizeof(ssHitNPos));
        chara->rot = chara->rotBefore;
    }
    float d[4];
    for (int k = 0; k < 3; k++)
        d[k] = ssHitNPos[k] - ssHitBPos[k];
    d[3] = ssHitNPos[3];
    if (gaInnerProductXYZ(d, d) != 0.0f) {
        ssVAddXYZ(ssHitBPos, d, ssHitBPos);
        ssMoveAllHitObjCoordinate(hits, d);
    }

    // The vertical part and the fall, against the floors.
    vv[1] -= SS_FALL_STEP * (float)chara->b2f1;
    ssVAddXYZ(ssHitNPos, vv, ssHitBPos);
    pitch = -1;                         // the ROM keeps the yaw the wall pass left
    HIT_DATA *ground = ssHitCheckAndProcess(chara, grid, hits, sparts, &yaw, &pitch, 0,
                                           ssCharaShape(bank, chara->w300[CHARA_HIT_FLOOR_PREV]));
    if (ground != NULL) {
        chara->b2f2 = 0;
        chara->b2f1 = SS_FALL_STEPS_LANDED;
        chara->w300[CHARA_HIT_FLOOR] = (int)BankEeAddr(bank, ground);
    } else {
        chara->b2f2++;
        int steps = (chara->b2f2 + 1) * 2 + 1;
        if (chara->b2f2 >= 0xff)
            chara->b2f2 = SS_FALL_FRAMES_MAX;
        if (steps <= 0)
            steps = 1;
        else if (steps > 0xff)
            steps = 0xff;
        chara->b2f1 = (u_char)steps;
    }

    for (int k = 0; k < 3; k++)
        d[k] = ssHitNPos[k] - ((float *)&chara->pos)[k];
    d[3] = ssHitNPos[3];
    if (!(gaInnerProductXYZ(d, d) <= SS_MOVE_MIN2)) {
        memcpy(&chara->pos, ssHitNPos, sizeof(ssHitNPos));
        asCharacterPositioningOnlyDeltaPos(chara->bank, d);
    }
}
