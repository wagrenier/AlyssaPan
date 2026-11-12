// game/ssCalcInversMatrix2.c -- 0x018a88e0..0x018a9fe0 of CT3 (symtab local chunk 102): a 4x4 inverse and the
// attack collision's boxes and hit records -- ssHitMakeAttackOBB / ssHitMakeDamageOBB build the boxes
// ssHitCheckOBB_OBB tests, ssHit_information / ssHit_informationScn write what an attack struck into the
// tables the x commands read (HIT_TABLE.recs, .atkHits, .objHits, .scnHits; wideXcmd_DebugPrint.h).
// Written 2026-11-12 from the decompilation and the disassembly (the boxes are VU0 macro code).
//
// PORT: the tables' words are EE addresses of their bank (BankEePtr); a host pointer written into one
// goes back through BankEeAddr of the bank it lies in (BankOfHostPtr).

#include <math.h>
#include <string.h>
#include "common/alyssapan_stub.h"
#include "game/ssCalcInversMatrix2.h"
#include "game/ct3_types.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/BRANCH_CLEAR.h"
#include "game/QuatSlerp.h"
#include "game/unassigned_data.h"
#include "ga/gaInit3dlib.h"

// The boxes' starting frame: row 0 (the centre, overwritten) zero, then the unit axes with w 1. The ROM
// keeps two equal copies, one per box maker (data 0x01939930 @1876 and 0x01939970 @2073).
static const float AttackObbBasis_1876[4][4] = {
    {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};
static const float DamageObbBasis_2073[4][4] = {
    {0.0f, 0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}};
// ssCalcInversMatrix2's scale matrix before its diagonal becomes 1 / det (data 0x019399b0).
static const float InverseScale[4][4] = {
    {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}};

#define SS_HIT_ATTACK_MIN_MOVE 0.1f     /* a character's attack moving less than this keeps the world axes */
#define SS_HIT_POWER_SCALE 1000         /* HIT_OBJ.damage * power / this comes off a scene shape's hp */
#define HIT_OBJ_REC_CHARA_NONE (-1)     /* HIT_OBJ_REC.chara of a scene shape a character's attack struck */
#define HIT_OBJ_REC_KIND_CHARA 0        /* HIT_OBJ_REC.kind: a character's damage shape was struck */
#define HIT_OBJ_REC_KIND_SCENE 1        /* a scene shape was struck */

// The EE address a host pointer into bank data had (PORT).
static u_int ssHitEeWord(const void *p)
{
    return BankEeAddr(BankOfHostPtr(p), p);
}

// CT3 0x018a88e0
// out = the inverse of m: the adjugate (cofactors written out, as the ROM does), then every row times
// 1 / det through a scale matrix. A det whose word is 0 leaves out untouched (-0.0 goes on, as the ROM's
// integer test does).
void ssCalcInversMatrix2(float (*out)[4], float (*m)[4])
{
    const float *p = &m[0][0];
    float m0_5 = p[0] * p[5], m10_15 = p[10] * p[15], m0_6 = p[0] * p[6], m0_7 = p[0] * p[7];
    float m9_15 = p[9] * p[15], m8_15 = p[8] * p[15], m8_14 = p[8] * p[14], m8_13 = p[8] * p[13];
    float m11_14 = p[11] * p[14], m11_13 = p[11] * p[13], m11_12 = p[11] * p[12], m10_12 = p[10] * p[12];
    float m9_14 = p[9] * p[14], m10_13 = p[10] * p[13], m9_12 = p[9] * p[12];
    float m1_4 = p[1] * p[4], m1_6 = p[1] * p[6], m1_7 = p[1] * p[7], m2_4 = p[2] * p[4];
    float m3_4 = p[3] * p[4], m2_5 = p[2] * p[5], m3_5 = p[3] * p[5], m2_7 = p[2] * p[7];
    float m3_6 = p[3] * p[6];
    float det =
        m3_6 * m9_12 +
        (((m3_5 * m8_14 + m3_4 * m10_13 +
           (((m2_7 * m8_13 + m2_5 * m11_12 +
              (((m2_4 * m9_15 + m1_7 * m10_12 +
                 (((m1_6 * m8_15 + m1_4 * m11_14 +
                    (((m0_7 * m9_14 + m0_6 * m11_13 + ((m0_5 * m10_15 - m0_5 * m11_14) - m0_6 * m9_15)) -
                      m0_7 * m10_13) -
                     m1_4 * m10_15)) -
                   m1_6 * m11_12) -
                  m1_7 * m8_14)) -
                m2_4 * m11_13) -
               m2_5 * m8_15)) -
             m2_7 * m9_12) -
            m3_4 * m9_14)) -
          m3_5 * m10_12) -
         m3_6 * m8_13);
    u_int bits;
    memcpy(&bits, &det, sizeof(bits));
    if (bits == 0)
        return;
    float adj[4][4];
    float *o = &adj[0][0];
    o[0] = (p[7] * m9_14 + p[6] * m11_13 + ((p[5] * m10_15 - p[5] * m11_14) - p[6] * m9_15)) - p[7] * m10_13;
    o[1] = p[3] * m10_13 + (((p[2] * m9_15 + p[1] * m11_14 + -p[1] * m10_15) - p[2] * m11_13) - p[3] * m9_14);
    o[2] = (m3_5 * p[14] + m2_7 * p[13] + ((m1_6 * p[15] - m1_7 * p[14]) - m2_5 * p[15])) - m3_6 * p[13];
    o[3] = m3_6 * p[9] + (((m2_5 * p[11] + m1_7 * p[10] + -m1_6 * p[11]) - m2_7 * p[9]) - m3_5 * p[10]);
    o[4] = p[7] * m10_12 + (((p[6] * m8_15 + p[4] * m11_14 + -p[4] * m10_15) - p[6] * m11_12) - p[7] * m8_14);
    o[5] = (p[3] * m8_14 + p[2] * m11_12 + ((p[0] * m10_15 - p[0] * m11_14) - p[2] * m8_15)) - p[3] * m10_12;
    o[6] = m3_6 * p[12] + (((m2_4 * p[15] + m0_7 * p[14] + -m0_6 * p[15]) - m2_7 * p[12]) - m3_4 * p[14]);
    o[7] = (m3_4 * p[10] + m2_7 * p[8] + ((m0_6 * p[11] - m0_7 * p[10]) - m2_4 * p[11])) - m3_6 * p[8];
    o[8] = (p[7] * m8_13 + p[5] * m11_12 + ((p[4] * m9_15 - p[4] * m11_13) - p[5] * m8_15)) - p[7] * m9_12;
    o[9] = p[3] * m9_12 + (((p[1] * m8_15 + p[0] * m11_13 + -p[0] * m9_15) - p[1] * m11_12) - p[3] * m8_13);
    o[10] = (m3_4 * p[13] + m1_7 * p[12] + ((m0_5 * p[15] - m0_7 * p[13]) - m1_4 * p[15])) - m3_5 * p[12];
    o[11] = m3_5 * p[8] + (((m1_4 * p[11] + m0_7 * p[9] + -m0_5 * p[11]) - m1_7 * p[8]) - m3_4 * p[9]);
    o[12] = p[6] * m9_12 + (((p[5] * m8_14 + p[4] * m10_13 + -p[4] * m9_14) - p[5] * m10_12) - p[6] * m8_13);
    o[13] = (p[2] * m8_13 + p[1] * m10_12 + ((p[0] * m9_14 - p[0] * m10_13) - p[1] * m8_14)) - p[2] * m9_12;
    o[14] = m2_5 * p[12] + (((m1_4 * p[14] + m0_6 * p[13] + -m0_5 * p[14]) - m1_6 * p[12]) - m2_4 * p[13]);
    o[15] = (m2_4 * p[9] + m1_6 * p[8] + ((m0_5 * p[10] - m0_6 * p[9]) - m1_4 * p[10])) - m2_5 * p[8];
    float scale[4][4];
    float inv = 1.0f / det;
    memcpy(scale, InverseScale, sizeof(scale));
    scale[0][0] = scale[1][1] = scale[2][2] = scale[3][3] = inv;
    gaMulMatrix(out, adj, scale);       // out = scale * adj: each row over det
}

// The bounds' running max / min, as the ROM keeps them: a value above the max moves only the max.
static void ssHitGrowBounds(float *max, float *min, const float *v)
{
    for (int k = 0; k < 3; k++) {
        if (max[k] < v[k])
            max[k] = v[k];
        else if (v[k] < min[k])
            min[k] = v[k];
    }
}

// The box's eight corners from its bounds (w 1), in the order ssHitCheckOneObject walks them: the min z
// face 0-1-2-3, then 4-5-6-7 at max z.
static void ssHitBoxCorners(SS_OBB *box, const float *max, const float *min)
{
    static const u_char useMax[8][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                        {0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}};
    for (int i = 0; i < 8; i++) {
        for (int k = 0; k < 3; k++)
            box->corner[i][k] = useMax[i][k] ? max[k] : min[k];
        box->corner[i][3] = 1.0f;
    }
}

// The box's half sizes (max - min) / 2, w too (0), into half / halfW, and its centre half + min.
static void ssHitHalfAndCenter(SS_OBB *box, float *center, const float *max, const float *min)
{
    float half[4];
    for (int k = 0; k < 4; k++) {
        half[k] = (max[k] - min[k]) * 0.5f;
        center[k] = half[k] + min[k];
    }
    memcpy(box->half, half, sizeof(box->half));
    box->halfW = half[3];
}

// CT3 0x018a9570
// The attack object's box (its SS_OBB head) around its world rows. A character's attack (chara != 0)
// that moved more than SS_HIT_ATTACK_MIN_MOVE (ssHitBPos -> ssHitNPos) is boxed in a frame whose z is the
// move, its far face pushed on by the move's length, then turned back; otherwise the box keeps the world
// axes.
// callcheck: gaInnerProductXYZ, ahAcos, asMakeRotMatrixByQuartanion, gaInversMatrix, gaMulMatrix
void ssHitMakeAttackOBB(HIT_OBJ *obj, int chara)
{
    SS_OBB *box = (SS_OBB *)obj;
    int bank = BankOfHostPtr(obj);
    float (*rows)[4] = BANK_PTR(float (*)[4], bank, obj->world);
    int n = obj->rows;
    float basis[4][4];
    float max[4], min[4];
    memcpy(basis, AttackObbBasis_1876, sizeof(basis));
    float move[4];
    for (int k = 0; k < 3; k++)
        move[k] = ssHitNPos[k] - ssHitBPos[k];
    move[3] = ssHitNPos[3];             // vsub.xyz keeps the position's w

    if (chara != 0) {
        float q = 1.0f / sqrtf(move[0] * move[0] + move[1] * move[1] + move[2] * move[2]);
        float dir[4] = {move[0] * q, move[1] * q, move[2] * q, move[3]};
        float len = 1.0f / q;
        if (SS_HIT_ATTACK_MIN_MOVE < len) {
            // The turn that takes z onto the move: about z x dir, by acos(z . dir).
            float *z = basis[3];
            float axis[4] = {z[1] * dir[2] - z[2] * dir[1], z[2] * dir[0] - z[0] * dir[2],
                             z[0] * dir[1] - z[1] * dir[0], 1.0f}; // w: not written by vopmsub.xyz
            float rot[4][4], back[4][4], tmp[4][4];
            int angle = ahAcos(gaInnerProductXYZ(z, dir));
            asMakeRotMatrixByQuartanion(rot, axis, angle);
            gaInversMatrix(back, rot);
            int first = 1;
            for (int i = 0; i < n; i += 4) {
                gaMulMatrix(tmp, rot, rows + i);
                if (first) {
                    memcpy(max, tmp[0], sizeof(max));
                    memcpy(min, tmp[0], sizeof(min));
                    first = 0;
                }
                for (int r = 0; r < 4; r++)
                    ssHitGrowBounds(max, min, tmp[r]);
            }
            max[2] += len;
            ssHitBoxCorners(box, max, min);
            gaMulMatrix(box->corner, back, box->corner);
            gaMulMatrix(box->corner + 4, back, box->corner + 4);
            ssHitHalfAndCenter(box, basis[0], max, min);
            float turned[4][4];
            gaMulMatrix(turned, back, basis); // the centre and the axes, turned back
            memcpy(box->center, turned[0], sizeof(box->center));
            memcpy(box->axis, turned + 1, sizeof(box->axis));
            return;
        }
    }

    // The world axes: the bounds of the rows as they are (four per step, as the ROM reads them).
    memcpy(max, rows[0], sizeof(max));
    memcpy(min, rows[0], sizeof(min));
    for (int i = 0; i < n; i += 4)
        for (int r = 0; r < 4; r++)
            ssHitGrowBounds(max, min, rows[i + r]);
    ssHitBoxCorners(box, max, min);
    memcpy(box->axis, basis + 1, sizeof(box->axis));
    ssHitHalfAndCenter(box, box->center, max, min);
}

// CT3 0x018a9390
// The character's body box (HIT_TABLE.bodyObb): the bounds of every damage shape's eight corners (its
// world[0] and world[1] rows), on the world axes. Nothing with no damage shapes.
void ssHitMakeDamageOBB(HIT_TABLE *dmg)
{
    int n = dmg->dmgObjCount;
    if (n == 0)
        return;
    int bank = BankOfHostPtr(dmg);
    const u_int *list = BANK_PTR(const u_int *, bank, dmg->hitData);
    const HIT_DMG *d0 = BANK_PTR(const HIT_DMG *, bank, list[0]);
    float max[4], min[4];
    memcpy(max, d0->world[0][0], sizeof(max)); // w: the first corner's, for both
    memcpy(min, d0->world[0][0], sizeof(min));
    for (int i = 0; i < n; i++) {
        const HIT_DMG *d = BANK_PTR(const HIT_DMG *, bank, list[i]);
        const float (*corner)[4] = (const float (*)[4])d->world; // world[0] and world[1]: 8 corners
        for (int j = 0; j < 8; j++)
            ssHitGrowBounds(max, min, corner[j]);
    }
    SS_OBB *box = BANK_PTR(SS_OBB *, bank, dmg->bodyObb);
    memcpy(box->axis, DamageObbBasis_2073 + 1, sizeof(box->axis));
    ssHitHalfAndCenter(box, box->center, max, min);
}

// obj's record in a table of HIT_OBJ_REC: the existing one (*found 1), or the free one at the end.
static HIT_OBJ_REC *ssHitFindObjRec(HIT_OBJ_REC *recs, int count, u_int obj, int *found)
{
    for (int i = 0; i < count; i++) {
        if (recs[i].object == obj) {
            *found = 1;
            return &recs[i];
        }
    }
    *found = 0;
    return &recs[count];
}

// CT3 0x018a91d0
// The damage shape d of the target character (a->target) was struck by obj: obj's record in the
// attacker's table (atkHits, a character's; else the scene bank's scnHits) learns the shape and the target
// if it had none, and the target's table (dmg->recs) records the attack, once per shape.
void ssHit_information(SS_HIT_ATTACK *a, HIT_OBJ *obj, HIT_DMG *d, HIT_TABLE *dmg, HIT_TABLE *atkHits,
                       HIT_TABLE *scnHits)
{
    short target = a->target;
    u_int objEe = ssHitEeWord(obj);
    u_int dEe = ssHitEeWord(d);
    HIT_TABLE *t = atkHits != NULL ? atkHits : scnHits;
    int *count = atkHits != NULL ? &t->w1c : &t->objHitCount;
    u_int recsEe = atkHits != NULL ? t->atkHits : t->objHits;
    HIT_OBJ_REC *recs = BANK_PTR(HIT_OBJ_REC *, BankOfHostPtr(t), recsEe);
    int found;
    HIT_OBJ_REC *r = ssHitFindObjRec(recs, *count, objEe, &found);
    if (found) {
        if (r->chara == HIT_OBJ_REC_CHARA_NONE) {
            r->chara = target;
            r->id = (int)dEe;
        }
    } else {
        r->object = objEe;
        r->kind = HIT_OBJ_REC_KIND_CHARA;
        r->materials = 0;
        r->chara = target;
        r->id = (int)dEe;
        (*count)++;
    }

    HIT_REC *hr = BANK_PTR(HIT_REC *, BankOfHostPtr(dmg), dmg->recs);
    for (int i = 0; i < dmg->count; i++)
        if (hr[i].attacker == dEe)
            return;
    HIT_REC *nr = &hr[dmg->count];
    nr->attacker = dEe;
    nr->isObject = a->kind;
    nr->attack = a->motion;
    nr->who = a->chara;
    nr->bank = (u_char)a->atk;
    dmg->count++;
}

// CT3 0x018a8e00
// The scene shape h of scn was struck by obj at pos: scn's table of struck shapes (scnHits) records it
// once; unless the shape is HIT_DATA_HP_UNBREAKABLE, the attack's damage comes off its hp, and a shape
// whose hp went below 0 is left at 0 with nothing more recorded. Otherwise obj's record in the attacker's
// table (atkHits, a character's; else the attacking scene bank's -- a->atk's ALL_TABLE.hits objHits)
// gains the shape's material bit, or is made.
void ssHit_informationScn(SS_HIT_ATTACK *a, HIT_OBJ *obj, HIT_DATA *h, HIT_TABLE *scn, HIT_TABLE *atkHits,
                          const float *pos)
{
    u_int objEe = ssHitEeWord(obj);
    u_int hEe = ssHitEeWord(h);
    HIT_ATK_REC *sr = BANK_PTR(HIT_ATK_REC *, BankOfHostPtr(scn), scn->scnHits);
    int seen = 0;
    for (int i = 0; i < scn->w2b8; i++) {
        if (sr[i].object == hEe) {
            seen = 1;
            break;
        }
    }
    if (!seen) {
        HIT_ATK_REC *r = &sr[scn->w2b8];
        r->object = hEe;
        memcpy(r->pos, pos, sizeof(r->pos));
        r->byObject = (u_char)a->kind;
        r->motion = a->motion;
        r->attackObj = a->chara;
        r->attackBank = (u_short)a->atk;
        scn->w2b8++;
    }

    if (h->hp != HIT_DATA_HP_UNBREAKABLE) {
        h->hp = (s_char)(h->hp - (s_char)((obj->damage * (int)(u_short)a->power) / SS_HIT_POWER_SCALE));
        if (h->hp < 0) {
            h->hp = 0;
            return;
        }
    }

    HIT_TABLE *t;
    int *count;
    u_int recsEe;
    if (atkHits == NULL) {
        int atkBank = (u_short)a->atk;
        t = BANK_PTR(HIT_TABLE *, atkBank, All_table_TBL[atkBank]->hits);
        count = &t->objHitCount;
        recsEe = t->objHits;
    } else {
        t = atkHits;
        count = &t->w1c;
        recsEe = t->atkHits;
    }
    HIT_OBJ_REC *recs = BANK_PTR(HIT_OBJ_REC *, BankOfHostPtr(t), recsEe);
    u_int bit = 1u << (h->material & 0x1f);
    int found;
    HIT_OBJ_REC *r = ssHitFindObjRec(recs, *count, objEe, &found);
    if (found) {
        r->materials |= bit;
    } else {
        r->object = objEe;
        r->kind = HIT_OBJ_REC_KIND_SCENE;
        r->materials = bit;
        r->chara = atkHits == NULL ? a->bank : HIT_OBJ_REC_CHARA_NONE;
        r->id = (int)hEe;
        memcpy(r->pos, pos, sizeof(r->pos));
        (*count)++;
    }
}
