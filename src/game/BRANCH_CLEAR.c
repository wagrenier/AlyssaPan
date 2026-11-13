// game/BRANCH_CLEAR.c -- Clock Tower 3 module 0x01851f00..0x0185e9d0 (BRANCH_CLEAR .. rnd): one CodeWarrior object
// (its locals are one inseparable symtab run, 1088..1162; architecture.md "The TU map"). Until 2026-11-11 it was
// the TU map's game/QuatSlerp.c, whose first four functions (QuatSlerp, ahCos, ahSin, ahAcos) are that file now.
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "alyssapan/rendering/alyssapan_drawga.h"
#include "game/BRANCH_CLEAR.h"
#include "ga/gaInit3dlib.h"
#include "game/unassigned_data.h"
#include "ga/gaSinCos.h"
#include "game/ssGrpPortalCheck.h"
#include "game/main.h"
#include "sdk/eekernel.h"
#include "ga/gaInitDrawObject.h"
#include "game/main2.h"
#include "ga/gaBGT_DrawObject.h"
#include "ga/gaObjectClippingCheck.h"
#include "game/fileio.h"
#include "game/ahalloc.h"
#include "game/Convert.h"
#include "game/LinkMatrixTree.h"
#include "game/ssSetTexUVforCubeMap.h"
#include "game/wideXcmd_DebugPrint.h"
#include "game/sound.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/Character_Init.h"
#include "game/asMothPause.h"
#include "ga/gaDma.h"
#include "ga/gaDmaSync_VIF1.h"
#include "ga/gaVifInit.h"
#include "sdk/libgraph.h"
#include "ga/gaInitInterrupt.h"
#include "game/SdMasterVolumeSetSE.h"
#include "game/Clipping_Portal.h"
#include "game/ssHitCheckAttackToScn.h"
#include "game/movie.h"
#include "ga/gaInitMeter.h"
#include "alyssapan/rendering/alyssapan_drawga.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// CT3 0x019395b0 (RoomNameTbl, 95 names) and 0x01939730 (RoomIdTbl, 95 trigger events + -1): row i
// is a room (its files' first four characters) and the event RoomInFlagOn registers in it.
static const char *const RoomNameTbl[ROOM_TBL_COUNT] = {
    "0A01", "0A02", "0A03", "0A06", "0A07", "0A10", "0A11", "0A16", "0A17", "0A19",
    "0A23", "0A27", "0A30", "0B01", "0B03", "0B06", "0B07", "0B19", "1A02", "1A04",
    "1A10", "1A11", "1B01", "1B03", "1B04", "1B05", "1B07", "1B08", "1B09", "1B11",
    "1B12", "1C01", "1C02", "1C03", "1C04", "1C05", "1C06", "2A01", "2A02", "2B01",
    "2B02", "2B03", "2B04", "2B05", "2B06", "2B07", "2B08", "2D01", "2D02", "2D03",
    "3A01", "3A02", "3C01", "3C02", "3C03", "3C05", "3C06", "3C07", "3D01", "3D02",
    "3D03", "3D05", "3E01", "3F01", "4A01", "4A02", "4A03", "4A04", "4A05", "4A06",
    "4A07", "4A08", "4A09", "4A10", "4A11", "4A12", "4B01", "4B02", "4B03", "4B04",
    "4B05", "4B06", "4B07", "4B08", "4B09", "4B10", "4C01", "4N01", "4N02", "4N03",
    "5A01", "5A01", "5A02", "5A03", "5A04",
};
static const int RoomIdTbl[ROOM_TBL_COUNT + 1] = {
    1846, 1847, 1848, 1851, 1852, 1855, 1856, 1861, 1862, 1864, 1867, 1871,
    1922, 1872, 1874, 1875, 1876, 1879, 1801, 1802, 1803, 1804, 1806, 1808,
    1809, 1810, 1812, 1813, 1814, 1816, 1817, 1818, 1819, 1820, 1821, 1822,
    1823, 1833, 1834, 1835, 1836, 1837, 1838, 1839, 1840, 1841, 1842, 1843,
    1844, 1845, 1915, 1916, 1910, 1917, 1911, 1881, 1882, 1907, 1883, 1884,
    1908, 1885, 1909, 1886, 1887, 1888, 1919, 1889, 1893, 1894, 1890, 1918,
    1891, 1892, 1895, 1896, 1902, 1904, 1906, 1921, 1920, 1897, 1898, 1899,
    1914, 1905, 1912, 1900, 1901, 1903, 1913, 1913, 1925, 1923, 1924, -1,
};


// CT3 0x0185df90
// Written early (ahRelocateSunPs2ObjectArry.c slice 13): VU0 RSQRT of x2 + y2 + z2, then 1 / Q.
// (The ROM also stores the normalised vector to a stack slot nobody reads.)
float asVectorLength3D(const float *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

// ---- GS register words (written early, ahRelocateSunPs2ObjectArry.c slice 14: asShadowEnd) ----

// CT3 0x0185c450
u_long asDR_RGBAQ(u_char r, u_char g, u_char b, u_char a, float q)
{
    u_int qbits;
    memcpy(&qbits, &q, sizeof(qbits));
    return (u_long)r | (u_long)g << 8 | (u_long)b << 16 | (u_long)a << 24 | (u_long)qbits << 32;
}

// CT3 0x0185c3a0
u_int asDR_UV(u_short u, u_short v)
{
    return (u_int)u | (u_int)v << 16;
}

// CT3 0x0185c410: pixels to the GS's 12.4 window coordinates (offset 0x800).
u_long asDR_XYZ2(short x, short y, u_short z)
{
    return (u_long)(u_short)((x + 0x800) * 0x10) | (u_long)(u_short)((y + 0x800) * 0x10) << 16 | (u_long)z << 32;
}

// CT3 0x0185c3d0: as asDR_XYZ2 (the register it goes to differs, not the word).
u_long asDR_XYZ3(short x, short y, u_short z)
{
    return (u_long)(u_short)((x + 0x800) * 0x10) | (u_long)(u_short)((y + 0x800) * 0x10) << 16 | (u_long)z << 32;
}

// ---- the Cont* writers EA_010 calls (written early, ahRelocateSunPs2ObjectArry.c slice 14) ----
// Each walks n elements (the ROM's do / while on a non-zero count): float vectors 0x10 apart, packed
// ones 3 shorts apart.

// CT3 0x0185cf30: as asZipTrans2Trans_XYZ1, w = 0 (a direction).
void asZipTrans2Trans_XYZ0(float *out, const short *in, u_int shift)
{
    float scale = 1.0f / (float)(1 << shift);
    int i;
    for (i = 0; i < 3; i++)
        out[i] = (float)in[i] * scale;
    out[3] = 0.0f;
}

// CT3 0x0185d2a0
void asContInterpolateVector(float *out, const float *a, const float *b, const float *w, int n)
{
    for (; n != 0; n--, out += 4, a += 4, b += 4)
        asInterpolateVector(out, a, b, w);
}

// CT3 0x0185d1f0: a vertex whose w is skipW keeps its value.
void asContInterpolateVector_nw(float skipW, float *out, const float *a, const float *b, const float *w,
                                int n)
{
    for (; n != 0; n--, out += 4, a += 4, b += 4)
        if (a[3] != skipW)
            asInterpolateVector(out, a, b, w);
}

// CT3 0x0185c480: the qwords whose w is not skipW.
// CT3 0x0185c4b0 (asMemCopy128c_nw_body, game-inferred: its loop head, split by Ghidra).
void asMemCopy128c_nw(float skipW, void *dst, const void *src, int n)
{
    float *d = (float *)dst;
    const float *s = (const float *)src;
    for (; n != 0; n--, d += 4, s += 4)
        if (s[3] != skipW)
            memmove(d, s, 16);
}

// CT3 0x0185c9e0: packed normals (shift 14), w 0.
void asContZipNormal2Normal_XYZ0(float *out, const short *in, int n)
{
    for (; n != 0; n--, out += 4, in += 3)
        asZipTrans2Trans_XYZ0(out, in, ZIP_NORMAL_SHIFT);
}

// CT3 0x0185cd30
void asContZipTrans2Trans_XYZ1(float *out, const short *in, int n, u_int shift)
{
    for (; n != 0; n--, out += 4, in += 3)
        asZipTrans2Trans_XYZ1(out, in, shift);
}

// CT3 0x0185cc90: a vertex whose x short is `skip` keeps its value.
void asContZipTrans2Trans_XYZ1_nw(float *out, const short *in, int n, u_int shift, short skip)
{
    for (; n != 0; n--, out += 4, in += 3)
        if (in[0] != skip)
            asZipTrans2Trans_XYZ1(out, in, shift);
}

// CT3 0x0185cbe0
void asContInterpolateGA_VECTORZipTrans_XYZ1(float *out, const float *a, const short *b, const float *w,
                                             int n, u_int shift)
{
    alignas(16) float v[4];
    for (; n != 0; n--, out += 4, a += 4, b += 3) {
        asZipTrans2Trans_XYZ1(v, b, shift);
        asInterpolateVector(out, a, v, w);
    }
}

// CT3 0x0185cb20
void asContInterpolateGA_VECTORZipTrans_XYZ1_nw(float *out, const float *a, const short *b, const float *w,
                                                int n, u_int shift, short skip)
{
    alignas(16) float v[4];
    for (; n != 0; n--, out += 4, a += 4, b += 3) {
        if (b[0] != skip) {
            asZipTrans2Trans_XYZ1(v, b, shift);
            asInterpolateVector(out, a, v, w);
        }
    }
}

// CT3 0x0185c920: the blend renormalised (VU0 RSQRT: xyz * 1 / |xyz|; w as blended).
void asContInterpolateGA_VECTORZipNormal_XYZ0(float *out, const float *a, const short *b, const float *w,
                                              int n)
{
    alignas(16) float v[4];
    for (; n != 0; n--, out += 4, a += 4, b += 3) {
        float len;
        asZipTrans2Trans_XYZ0(v, b, ZIP_NORMAL_SHIFT);
        asInterpolateVector(out, a, v, w);
        len = 1.0f / sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
        out[0] *= len;
        out[1] *= len;
        out[2] *= len;
    }
}

// CT3 0x0185c5d0: an unsigned 7.9 fixed value (u_short / 512) into x; the other lanes untouched.
void asZipFloat0_7_9X2Float(float *out, const u_short *in)
{
    out[0] = (float)*in / 512.0f;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// CT3 0x01856c30: each drawn bank (a group counted once; BANK_TYPE_NO_ROOM_SOUND banks left out) runs
// its ambient animation over its parts' partsPos, copies each part's alpha into partsPos w, then the
// line-of-sight, distance and angle fades.
void AmbientAlphaUpdate(void)
{
    int bank, i;
    MODEL_PARTS *part;
    SCENE_ALPHA_ANIM *anim;
    float (*partsPos)[4];
    float q;

    for (bank = 0; bank < BANK_MAX; bank++) {
        if (BankInfo[bank].state <= 0)
            continue;
        if (BankInfo[bank].drawOn == 0 || BankInfo[bank].type == BANK_TYPE_NO_ROOM_SOUND) {
            if (BankInfo[bank].state != 0)
                bank += BankInfo[bank].groupCount - 1;
            continue;
        }
        anim = &scene_alpha_anim[bank];
        partsPos = BANK_PTR(float (*)[4], bank, All_table_TBL[bank]->partsPos);
        for (i = 0, part = (MODEL_PARTS *)pModelPartsHeader[bank]; (s_char)part->kind != PARTS_KIND_END;
             i++, part = (MODEL_PARTS *)((u_char *)part + PARTS_REC_SIZE)) {
            if (anim->on != 0) {
                if (anim->steps == 0) {
                    partsPos[i][0] = anim->target[0];
                    partsPos[i][1] = anim->target[1];
                    partsPos[i][2] = anim->target[2];
                } else {
                    q = 1.0f / (float)anim->steps;
                    partsPos[i][0] += (anim->target[0] - partsPos[i][0]) * q;
                    partsPos[i][1] += (anim->target[1] - partsPos[i][1]) * q;
                    partsPos[i][2] += (anim->target[2] - partsPos[i][2]) * q;
                }
            }
            partsPos[i][3] = part->alpha;
        }
        if (anim->steps != 0)
            anim->steps--;
        LosHitFadeUpdate(bank, 0);
        DistanceAttenuationUpdate(bank, 0);
        AngleAttenuationUpdate(bank, 0);
        if (BankInfo[bank].state != 0)
            bank += BankInfo[bank].groupCount - 1;
    }
}

// ---- the distance and angle fades (slice 5) --------------------------------------------------------
// The ROM hands the Target / AlphaSet functions the part's record (MODEL_PARTS + 0x40) and writes the
// fade at record +0xe8 / +0xec; the port passes the MODEL_PARTS (the same words: distAlpha, angleAlpha).

// The bank's part i.
static MODEL_PARTS *AttenPart(int bank, int i)
{
    return (MODEL_PARTS *)(pModelPartsHeader[bank] + i * PARTS_REC_SIZE);
}

// A character the fades see: both CHARACTER_FLAG_REFRACTION and _SHADOW.
static int AttenCharaLive(int chara)
{
    u_int flags = Character[chara].flags;
    return (flags & CHARACTER_FLAG_REFRACTION) != 0 && (flags & CHARACTER_FLAG_SHADOW) != 0;
}

// The character's part named by hash. The ROM looks it up in All_table_TBL[bank]->partsHeader, the
// header scene_init copies into pModelPartsHeader[bank]. PORT: NULL when the part is missing (the ROM
// would use asFindModelParts' answer anyway).
static MODEL_PARTS *AttenCharaPart(int chara, u_long hash)
{
    u_char *rec = asFindModelParts(pModelPartsHeader[Character[chara].bank], hash);
    return rec != NULL ? (MODEL_PARTS *)(rec - PARTS_HDR_RECORDS) : NULL;
}

// Lower the part's ALL_TABLE.partsPos w to the fade (both Update functions end each record so).
static void AttenLowerW(int bank, int index, float fade)
{
    float (*partsPos)[4] = BANK_PTR(float (*)[4], bank, All_table_TBL[bank]->partsPos);
    if (fade < partsPos[index][3])
        partsPos[index][3] = fade;
}

// CT3 0x01857c70
void DistanceAttenuationAlphaSet(const float *eye, const float *subj, DIST_ATTEN *at, MODEL_PARTS *part)
{
    float f = asVectorDistance(subj, eye);
    float max = at->max;

    f = max * (f - at->start) / (at->full - at->start);
    if (f < 0.0f)
        f = 0.0f;
    if (max < f)
        f = max;
    part->distAlpha = -f;
}

// CT3 0x01857af0: the part's fade for one eye, over the record's subjects.
void DistanceAttenuationTarget(int bank, int a1, MODEL_PARTS *part, DIST_ATTEN *at, const float *eye)
{
    // callcheck: asFindModelParts (AttenCharaPart makes it)
    ATTEN_CHARA *ch;
    MODEL_PARTS *subj;
    int *idx;

    (void)a1;
    if (at->flags & ATTEN_SUBJ_CHARA) {
        for (ch = BANK_PTR(ATTEN_CHARA *, bank, at->subjCharas); ch->chara != -1; ch++) {
            if (!AttenCharaLive(ch->chara))
                continue;
            subj = AttenCharaPart(ch->chara, ch->hash);
            if (subj != NULL)
                DistanceAttenuationAlphaSet(eye, subj->pos, at, part);
        }
    }
    if (at->flags & ATTEN_SUBJ_PARTS) {
        for (idx = BANK_PTR(int *, bank, at->subjParts); *idx != -1; idx++) {
            subj = AttenPart(bank, *idx);
            if ((subj->flags & PARTS_FLAG_FOG) == 0)
                DistanceAttenuationAlphaSet(eye, subj->pos, at, part);
        }
    }
}

// CT3 0x01857590: the distance fades of the bank's parts (AmbientAlphaUpdate, every frame).
void DistanceAttenuationUpdate(int bank, int a1)
{
    // callcheck: asFindModelParts (AttenCharaPart makes it)
    DIST_ATTEN *at;
    MODEL_PARTS *part, *eyePart;
    ATTEN_CHARA *ch;
    float eye[4];
    int *list;
    int i;

    if (All_table_TBL[bank]->distAtten == 0)
        return;
    for (at = BANK_PTR(DIST_ATTEN *, bank, All_table_TBL[bank]->distAtten); at->part != -1; at++) {
        part = AttenPart(bank, at->part);
        if (at->flags & ATTEN_EYE_CAMERA) {
            // the shared camera's word at +0xc0 (copied by value: the ROM's lq / sq)
            if (at->cuts == 0) {
                memcpy(eye, (u_char *)&Cam[CAMERA_SHARED] + CAMERA_EYE_POS, sizeof(eye));
                DistanceAttenuationTarget(bank, a1, part, at, eye);
            } else {
                for (list = BANK_PTR(int *, bank, at->cuts); *list != -1; list++) {
                    if (Cam[CAMERA_SHARED].cut == *list) {
                        memcpy(eye, (u_char *)&Cam[CAMERA_SHARED] + CAMERA_EYE_POS, sizeof(eye));
                        DistanceAttenuationTarget(bank, a1, part, at, eye);
                    }
                }
            }
        }
        if (at->flags & ATTEN_EYE_CHARA) {
            // ROM bug: with no list (eyeCharas 0) the ROM still walks every live character and reads
            // its part hash from the list (word 0 + a stale index * 0x10 + 8: low EE memory). PORT: no
            // hash, no part: those characters are skipped.
            if (at->eyeCharas != 0) {
                for (ch = BANK_PTR(ATTEN_CHARA *, bank, at->eyeCharas); ch->chara != -1; ch++) {
                    if (!AttenCharaLive(ch->chara))
                        continue;
                    eyePart = AttenCharaPart(ch->chara, ch->hash);
                    if (eyePart == NULL)
                        continue;
                    memcpy(eye, eyePart->pos, sizeof(eye));
                    DistanceAttenuationTarget(bank, a1, part, at, eye);
                }
            }
        }
        if (at->flags & ATTEN_EYE_PARTS) {
            if (at->eyeParts == 0) {
                for (i = 0; (s_char)AttenPart(bank, i)->kind != PARTS_KIND_END; i++) {
                    eyePart = AttenPart(bank, i);
                    if ((eyePart->flags & PARTS_FLAG_FOG) == 0) {
                        memcpy(eye, eyePart->pos, sizeof(eye));
                        DistanceAttenuationTarget(bank, a1, part, at, eye);
                    }
                }
            } else {
                for (list = BANK_PTR(int *, bank, at->eyeParts); *list != -1; list++) {
                    eyePart = AttenPart(bank, *list);
                    if ((eyePart->flags & PARTS_FLAG_FOG) == 0) {
                        memcpy(eye, eyePart->pos, sizeof(eye));
                        DistanceAttenuationTarget(bank, a1, part, at, eye);
                    }
                }
            }
        }
        AttenLowerW(bank, at->part, part->distAlpha);
    }
}

// Does the line from eye to the subject at pos block on a drawn scene bank (CheckLOS), or, by id, on
// this bank (CheckLOS_Multi, a hit whose id is listed)? A plain hit ends the subject walk at once; a hit
// by id does not.
static int LosHitBlocked(int bank, LOS_FADE *at, const float *eye, const float *pos, int *stop)
{
    LOS_HIT seg;                        // CheckLOS writes its point and record past the segment
    LOS_MULTI q;
    int *ids;
    u_int i;
    int b;

    *stop = 0;
    if (at->flags & LOS_FADE_BY_ID) {
        memcpy(q.from, eye, sizeof(q.from));
        q.from[3] = 1.0f;
        memcpy(q.to, pos, sizeof(q.to));
        q.to[3] = 1.0f;
        if (CheckLOS_Multi(bank, &q) == 0)
            return 0;
        for (i = 0; i < q.count; i++) {
            for (ids = BANK_PTR(int *, bank, at->hitIds); *ids != -1; ids++) {
                if (*(short *)((u_char *)q.hits[i] + LOS_HIT_ID) == *ids)
                    return 1;
            }
        }
        return 0;
    }
    memcpy(seg.seg.from, eye, sizeof(seg.seg.from));
    seg.seg.from[3] = 1.0f;
    memcpy(seg.seg.to, pos, sizeof(seg.seg.to));
    seg.seg.to[3] = 1.0f;
    for (b = GLOBAL_BANK_LAST + 1; b < BANK_MAX; b++) {
        if (BankInfo[b].drawOn == 0)
            continue;
        if (BankInfo[b].type == BANK_TYPE_SHADOW && CheckLOS(b, &seg) != 0) {
            *stop = 1;
            return 1;
        }
        b += BankInfo[b].groupCount - 1;
    }
    return 0;
}

// CT3 0x01858370: start the part's line-of-sight fade for one eye: out while a line to a subject is
// blocked, in when none is (each subject kind decides on its own; the later one wins).
void LosHitFadeUpdateTarget(int bank, int a1, MODEL_PARTS *part, LOS_FADE *at, const float *eye)
{
    // callcheck: CheckLOS, CheckLOS_Multi, asFindModelParts (LosHitBlocked and AttenCharaPart make them)
    ATTEN_CHARA *ch;
    MODEL_PARTS *subj;
    int *idx;
    int blocked, stop;

    (void)a1;
    if (at->flags & ATTEN_SUBJ_CHARA) {
        blocked = 0;
        for (ch = BANK_PTR(ATTEN_CHARA *, bank, at->subjCharas); ch->chara != -1; ch++) {
            if (!AttenCharaLive(ch->chara))
                continue;
            subj = AttenCharaPart(ch->chara, ch->hash);
            if (subj == NULL)
                continue;
            if (LosHitBlocked(bank, at, eye, subj->pos, &stop)) {
                blocked = 1;
                part->flags = (part->flags | PARTS_FLAG_LOS_FADING) & ~PARTS_FLAG_LOS_CLEAR;
                if (stop)
                    break;
            }
        }
        if (!blocked)
            part->flags |= PARTS_FLAG_LOS_CLEAR | PARTS_FLAG_LOS_FADING;
    }
    if (at->flags & ATTEN_SUBJ_PARTS) {
        blocked = 0;
        for (idx = BANK_PTR(int *, bank, at->subjParts); *idx != -1; idx++) {
            subj = AttenPart(bank, *idx);
            if (subj->flags & PARTS_FLAG_FOG)
                continue;
            if (LosHitBlocked(bank, at, eye, subj->pos, &stop)) {
                blocked = 1;
                part->flags = (part->flags | PARTS_FLAG_LOS_FADING) & ~PARTS_FLAG_LOS_CLEAR;
                if (stop)
                    break;
            }
        }
        if (!blocked)
            part->flags |= PARTS_FLAG_LOS_CLEAR | PARTS_FLAG_LOS_FADING;
    }
}

// CT3 0x01857d10: the line-of-sight fades of the bank's parts (AmbientAlphaUpdate, every frame). The eyes
// are DistanceAttenuationUpdate's; then a running fade steps and lowers the part's partsPos w.
void LosHitFadeUpdate(int bank, int a1)
{
    // callcheck: asFindModelParts (AttenCharaPart makes it)
    LOS_FADE *at;
    MODEL_PARTS *part, *eyePart;
    ATTEN_CHARA *ch;
    float eye[4];
    int *list;
    int i;

    if (All_table_TBL[bank]->losFade == 0)
        return;
    for (at = BANK_PTR(LOS_FADE *, bank, All_table_TBL[bank]->losFade); at->part != -1; at++) {
        part = AttenPart(bank, at->part);
        if (at->flags & ATTEN_EYE_CAMERA) {
            if (at->cuts == 0) {
                memcpy(eye, (u_char *)&Cam[CAMERA_SHARED] + CAMERA_EYE_POS, sizeof(eye));
                LosHitFadeUpdateTarget(bank, a1, part, at, eye);
            } else {
                for (list = BANK_PTR(int *, bank, at->cuts); *list != -1; list++) {
                    if (Cam[CAMERA_SHARED].cut == *list) {
                        memcpy(eye, (u_char *)&Cam[CAMERA_SHARED] + CAMERA_EYE_POS, sizeof(eye));
                        LosHitFadeUpdateTarget(bank, a1, part, at, eye);
                    }
                }
            }
        }
        // ROM bug as in DistanceAttenuationUpdate: no list reads the hash from low EE memory; skipped.
        if ((at->flags & ATTEN_EYE_CHARA) && at->eyeCharas != 0) {
            for (ch = BANK_PTR(ATTEN_CHARA *, bank, at->eyeCharas); ch->chara != -1; ch++) {
                if (!AttenCharaLive(ch->chara))
                    continue;
                eyePart = AttenCharaPart(ch->chara, ch->hash);
                if (eyePart == NULL)
                    continue;
                memcpy(eye, eyePart->pos, sizeof(eye));
                LosHitFadeUpdateTarget(bank, a1, part, at, eye);
            }
        }
        if (at->flags & ATTEN_EYE_PARTS) {
            if (at->eyeParts == 0) {
                for (i = 0; (s_char)AttenPart(bank, i)->kind != PARTS_KIND_END; i++) {
                    eyePart = AttenPart(bank, i);
                    if ((eyePart->flags & PARTS_FLAG_FOG) == 0) {
                        memcpy(eye, eyePart->pos, sizeof(eye));
                        LosHitFadeUpdateTarget(bank, a1, part, at, eye);
                    }
                }
            } else {
                for (list = BANK_PTR(int *, bank, at->eyeParts); *list != -1; list++) {
                    eyePart = AttenPart(bank, *list);
                    if ((eyePart->flags & PARTS_FLAG_FOG) == 0) {
                        memcpy(eye, eyePart->pos, sizeof(eye));
                        LosHitFadeUpdateTarget(bank, a1, part, at, eye);
                    }
                }
            }
        }
        if (part->flags & PARTS_FLAG_LOS_FADING) {
            if (part->flags & PARTS_FLAG_LOS_CLEAR) {
                part->losAlpha += 255.0f / at->frames;
                if (part->alpha < part->losAlpha) {
                    part->losAlpha = part->alpha;
                    part->flags &= ~PARTS_FLAG_LOS_FADING;
                }
            } else {
                part->losAlpha -= 255.0f / at->frames;
                if (part->losAlpha < -at->floor) {
                    part->losAlpha = -at->floor;
                    part->flags &= ~PARTS_FLAG_LOS_FADING;
                }
            }
        }
        AttenLowerW(bank, at->part, part->losAlpha);
    }
}

// CT3 0x018574b0
void AngleAttenuationAlphaSet(const float *eye, const float *dir, const float *subj, ANGLE_ATTEN *at,
                              MODEL_PARTS *part)
{
    float d[4], len2, q, f;
    int angle;

    // vsub, then vmulq.xyz by vrsqrt(|d|^2): the VU's 1/sqrt(0) is the largest float, so a subject at
    // the eye gives (0, 0, 0) and a quarter turn, not a NaN.
    d[0] = subj[0] - eye[0];
    d[1] = subj[1] - eye[1];
    d[2] = subj[2] - eye[2];
    d[3] = subj[3] - eye[3];
    len2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;
    d[0] *= q;
    d[1] *= q;
    d[2] *= q;
    angle = ahAcos(gaInnerProductXYZ(d, (float *)dir));
    f = at->max * ((float)(angle - at->start) / (float)(at->full - at->start));
    if (f < 0.0f)
        f = 0.0f;
    if (at->max < f)
        f = at->max;
    part->angleAlpha = -f;
}

// CT3 0x01857320: the part's fade for one eye and its facing, over the record's subjects.
void AngleAttenuationTarget(int bank, int a1, MODEL_PARTS *part, ANGLE_ATTEN *at, const float *eye,
                            const float *dir)
{
    // callcheck: asFindModelParts (AttenCharaPart makes it)
    ATTEN_CHARA *ch;
    MODEL_PARTS *subj;
    int *idx;

    (void)a1;
    if (at->flags & ATTEN_SUBJ_CHARA) {
        for (ch = BANK_PTR(ATTEN_CHARA *, bank, at->subjCharas); ch->chara != -1; ch++) {
            if (!AttenCharaLive(ch->chara))
                continue;
            subj = AttenCharaPart(ch->chara, ch->hash);
            if (subj != NULL)
                AngleAttenuationAlphaSet(eye, dir, subj->pos, at, part);
        }
    }
    if (at->flags & ATTEN_SUBJ_PARTS) {
        for (idx = BANK_PTR(int *, bank, at->subjParts); *idx != -1; idx++) {
            subj = AttenPart(bank, *idx);
            if ((subj->flags & PARTS_FLAG_FOG) == 0)
                AngleAttenuationAlphaSet(eye, dir, subj->pos, at, part);
        }
    }
}

// The angle fade's eye at a character: its last-frame position, facing row 2 of RotY(its last angle).
static void AngleAttenCharaEye(int bank, int a1, MODEL_PARTS *part, ANGLE_ATTEN *at, int chara)
{
    float eye[4], m[4][4];

    memcpy(eye, &Character[chara].posBefore, sizeof(eye));
    gaSetRotMatrixY(m, Character[chara].rotBefore);
    AngleAttenuationTarget(bank, a1, part, at, eye, m[2]);
}

// The angle fade's eye at the shared camera: its position and front from the view matrix. (The ROM
// first copies Cam[CAMERA_SHARED] +0xc0 into the position; asViewMatrix2PosRUF overwrites it.)
static void AngleAttenCameraEye(int bank, int a1, MODEL_PARTS *part, ANGLE_ATTEN *at)
{
    float eye[4], front[4];

    memcpy(eye, (u_char *)&Cam[CAMERA_SHARED] + CAMERA_EYE_POS, sizeof(eye));
    asViewMatrix2PosRUF(&Cam[CAMERA_SHARED].view[0][0], eye, NULL, NULL, front);
    AngleAttenuationTarget(bank, a1, part, at, eye, front);
}

// CT3 0x01856f20: the angle fades of the bank's parts (AmbientAlphaUpdate, every frame).
void AngleAttenuationUpdate(int bank, int a1)
{
    // callcheck: AngleAttenuationTarget, asViewMatrix2PosRUF, gaSetRotMatrixY (AngleAttenCameraEye and
    // AngleAttenCharaEye make them)
    ANGLE_ATTEN *at;
    MODEL_PARTS *part;
    ATTEN_CHARA *ch;
    int *list;
    int c;

    if (All_table_TBL[bank]->angleAtten == 0)
        return;
    for (at = BANK_PTR(ANGLE_ATTEN *, bank, All_table_TBL[bank]->angleAtten); at->part != -1; at++) {
        part = AttenPart(bank, at->part);
        if (at->flags & ATTEN_EYE_CAMERA) {
            if (at->cuts == 0) {
                AngleAttenCameraEye(bank, a1, part, at);
            } else {
                for (list = BANK_PTR(int *, bank, at->cuts); *list != -1; list++) {
                    if (Cam[CAMERA_SHARED].cut == *list)
                        AngleAttenCameraEye(bank, a1, part, at);
                }
            }
        }
        if (at->flags & ATTEN_EYE_CHARA) {
            if (at->eyeCharas == 0) {
                for (c = 0; c < CHARACTER_MAX; c++) {
                    if (AttenCharaLive(c))
                        AngleAttenCharaEye(bank, a1, part, at, c);
                }
            } else {
                for (ch = BANK_PTR(ATTEN_CHARA *, bank, at->eyeCharas); ch->chara != -1; ch++) {
                    if (AttenCharaLive(ch->chara))
                        AngleAttenCharaEye(bank, a1, part, at, ch->chara);
                }
            }
        }
        AttenLowerW(bank, at->part, part->angleAlpha);
    }
}

// CT3 0x01942970 (ClatteringData): the controller rattles (struggles the player shakes free of).
CLATTERING ClatteringData[CLATTERING_MAX];

// CT3 0x01852f10: drop a rattle; 1 for a slot out of range or already free, else 0.
int ControllerClatteringFree(int slot)
{
    if (slot < 0 || slot >= CLATTERING_MAX)
        return 1;
    if (ClatteringData[slot].state < 0)
        return 1;
    ClatteringData[slot].state = CLATTERING_FREE;
    return 0;
}

// CT3 0x01852f60: the rattle's state; a finished one (WON / LOST) is freed as it is read.
// (No range check: the ROM's.)
int ControllerClatteringGet(int slot)
{
    int state = ClatteringData[slot].state;
    if (state > 0)
        ClatteringData[slot].state = CLATTERING_FREE;
    return state;
}

// CT3 0x01852f90: per frame unless paused (PAUSE_RUNNING still runs): a button press on controller
// 1, or any of its stick bytes moving CLATTERING_STICK_SWING from the last counted position, counts
// one; decayFrames idle frames take one back. Won at goal, lost when timeLeft runs out.
void ControllerClatteringUpdate(void)
{
    int moved = 0;
    int i;

    if (pause != 0 && (pause & PAUSE_RUNNING) == 0)
        return;
    for (i = 0; i < CLATTERING_MAX; i++) {
        CLATTERING *c = &ClatteringData[i];
        if (c->state != CLATTERING_RUNNING)
            continue;
        // the ROM compares stick[2], [3], [0], [1] with the slot's [0..3] (it stores them so too)
        if (abs(pad[0].stick[2] - c->stick[0]) >= CLATTERING_STICK_SWING
            || abs(pad[0].stick[3] - c->stick[1]) >= CLATTERING_STICK_SWING
            || abs(pad[0].stick[0] - c->stick[2]) >= CLATTERING_STICK_SWING
            || abs(pad[0].stick[1] - c->stick[3]) >= CLATTERING_STICK_SWING) {
            // (moved stays set for the later slots: the ROM's)
            moved = 1;
            c->stick[0] = pad[0].stick[2];
            c->stick[1] = pad[0].stick[3];
            c->stick[2] = pad[0].stick[0];
            c->stick[3] = pad[0].stick[1];
        }
        if (pad_trg_button[0] != 0 || moved) {
            c->count++;
            c->idle = 0;
        } else if (++c->idle >= c->decayFrames) {
            c->idle = 0;
            if (--c->count < 0)
                c->count = 0;
        }
        if (--c->timeLeft == 0)
            c->state = CLATTERING_LOST;
        else if (c->count >= c->goal)
            c->state = CLATTERING_WON;
    }
}

// CT3 0x01853150: open a rattle in the first free slot and answer the slot.
int ControllerClatteringSet(const CLATTERING_SET *set)
{
    CLATTERING *c;
    int i;

    for (i = 0; i < CLATTERING_MAX; i++)
        if (ClatteringData[i].state < 0)
            break;
    // PORT: with no free slot the ROM fills slot 10, past the array (over pad_key_button); the host
    // answers CLATTERING_MAX without writing (ControllerClatteringFree refuses it).
    if (i == CLATTERING_MAX)
        return i;
    c = &ClatteringData[i];
    c->state = CLATTERING_RUNNING;
    c->count = 0;
    c->idle = 0;
    c->stick[0] = c->stick[1] = c->stick[2] = c->stick[3] = 0;
    c->w08 = set->w08 < 0 ? CLATTERING_DEF_W08 : set->w08;
    c->decayFrames = set->decayFrames < 0 ? CLATTERING_DEF_DECAY : set->decayFrames;
    c->goal = set->goal < 0 ? CLATTERING_DEF_GOAL : set->goal;
    c->timeLeft = set->time < 0 ? CLATTERING_DEF_TIME : set->time;
    return i;
}

// CT3 0x01853220: every slot free (the ROM unrolls the first eight).
void ControllerClatteringInit(void)
{
    int i;
    for (i = 0; i < CLATTERING_MAX; i++)
        ClatteringData[i].state = CLATTERING_FREE;
}

// CT3 0x01852010: bit `event` of SkipEventFlag (the demos the player has seen and may skip).
int DemoSkipCheckEvent(int event)
{
    return (u_char)SkipEventFlag[event >> 3] >> (event & 7) & 1;
}

// CT3 0x01852030: set bit `event` of SkipEventFlag (the demo is seen: from now on it may be skipped).
void DemoSkipRegisterEvent(int event)
{
    SkipEventFlag[event >> 3] |= (u_char)(1 << (event & 7));
}

// CT3 0x01938200 (SkipFlagTbl, .sdata, gp - 0x73f0): the trigger events of the skippable demos, in
// SkipEventFlag's bit order; SKIP_EVENT_END closes it (136 demos, 17 of SkipEventFlag's 20 bytes).
static int SkipFlagTbl[] = {
    950,  951,  952,  953,  954,  955,  956,  957,  958,  959,  961,  962,  963,  964,  965,  966,
    967,  968,  969,  970,  971,  972,  973,  974,  975,  976,  977,  978,  979,  980,  981,  982,
    983,  985,  986,  987,  988,  989,  990,  991,  992,  993,  994,  995,  996,  997,  984,  940,
    941,  942,  943,  944,  945,  946,  947,  948,  1060, 1064, 1069, 1070, 1072, 1367, 1347, 1356,
    1523, 1354, 1355, 1332, 1351, 1389, 960,  1376, 1368, 1366, 1350, 1369, 1370, 1371, 1427, 1392,
    1393, 1388, 1383, 1391, 1404, 1399, 1469, 1413, 1410, 1405, 1403, 1411, 1502, 1503, 1478, 1470,
    1479, 1529, 1530, 1531, 1472, 1467, 1466, 1468, 1475, 1495, 1464, 1419, 1425, 1423, 1527, 1432,
    1447, 1448, 1449, 1528, 1471, 1507, 1165, 1532, 1491, 1492, 1493, 938,  939,  1501, 1522, 1412,
    998,  1535, 1536, 1537, 1538, 1540, 1541, 1508, SKIP_EVENT_END,
};

// CT3 0x01852060: the SkipEventFlag bit of the demo whose trigger event is `event`, -1 when no demo
// has it.
int DemoSkipGetEvent(int event)
{
    int i;
    for (i = 0; SkipFlagTbl[i] != SKIP_EVENT_END; i++)
        if (event == SkipFlagTbl[i])
            return i;
    return -1;
}

// CT3 0x01852d30: 1 when one of the stalkers (Character[MONSTER_SLOT_FIRST..], both top flag bits set,
// with a brain) shares c's room (CHARACTER.lightBank), else 0.
int CheckSameRoomAsMonster(CHARACTER *c)
{
    int i;
    for (i = MONSTER_SLOT_FIRST; i < MONSTER_SLOT_FIRST + MONSTER_SLOT_COUNT; i++) {
        CHARACTER *m = &Character[i];
        if ((m->flags & CHARACTER_FLAG_SHADOW) && (m->flags & CHARACTER_FLAG_REFRACTION) && m->brain != 0 &&
            c->lightBank == m->lightBank)
            return 1;
    }
    return 0;
}

// CT3 0x01940e80 (Panicdef) and 0x01940e44 (AdvantageousTimeTime): .sbss beside PanicInfoData.
PANIC_DEF Panicdef;
int AdvantageousTimeTime;

// One eased value: 1 / time of the way to its target (int division truncates, as the ROM's div).
static float FogEaseF(float v, float target, short time)
{
    return v + (target - v) / (float)(int)time;
}

static int FogEaseI(int v, int target, short time)
{
    return v + (target - v) / (int)time;
}

// The fog block (fog[3], fogArg[3]) of FOG_INFO or the character's, toward an animation's targets:
// at once when time is 0, else eased.
static void FogAnimeBlock(float *fog, int *arg, const float *tFog, const int *tArg, u_char flags, short time)
{
    int i;
    for (i = 0; i < 3; i++)
        if (flags & (FOG_ANIME_FOG0 << i))
            fog[i] = time == 0 ? tFog[i] : FogEaseF(fog[i], tFog[i], time);
    if (flags & FOG_ANIME_FOG_ARG)
        for (i = 0; i < 3; i++)
            arg[i] = time == 0 ? tArg[i] : FogEaseI(arg[i], tArg[i], time);
}

// CT3 0x01856660
// The fog animations' frame (not while paused, unless the world runs on; not while TRIG_VAR_PANIC_OFF
// >= 0): for every loaded, drawn bank group that is not BANK_TYPE_NO_ROOM_SOUND, the bank's fog and
// character fog ease to FogAnimeBuff's targets; a clear colour change rewrites the frame clear too.
void FogAnimeUpdate(void)
{
    int i, k;

    if (pause != 0 && !(pause & PAUSE_RUNNING))
        return;
    if (ssTrigGetVariable(TRIG_VAR_PANIC_OFF, 1, 0) >= 0)
        return;
    for (i = 0; i < BANK_MAX; i++) {
        BANK_INFO *b = &BankInfo[i];
        FOG_ANIME *a;
        FOG_INFO *f;

        if (b->state <= 0)
            continue;
        if (b->drawOn != 0 && b->type != BANK_TYPE_NO_ROOM_SOUND) {
            a = &FogAnimeBuff[i];
            f = (FOG_INFO *)FogCtrlAddr[i];
            if (a->flags != 0) {
                FogAnimeBlock(f->fog, f->fogArg, a->fog, a->fogArg, a->flags, a->time);
                if (a->flags & FOG_ANIME_CLEAR_COLOR) {
                    for (k = 0; k < 3; k++)
                        f->clearColor[k] = a->time == 0 ? a->clearColor[k]
                                                        : FogEaseI(f->clearColor[k], a->clearColor[k], a->time);
                    drawcom_frameclear.rgbaq = FogClearColor((u_char *)f);
                }
                if (a->time == 0)
                    a->flags = 0;
                else
                    a->time--;
            }
            if (a->charaFlags != 0) {
                FogAnimeBlock(f->charaFog, f->charaFogArg, a->charaFog, a->charaFogArg, a->charaFlags,
                              a->charaTime);
                if (a->charaTime == 0)
                    a->charaFlags = 0;
                else
                    a->charaTime--;
            }
        }
        i += b->groupCount - 1;
    }
}

// The stalkers' own scene banks MonsterAIUpdate pauses with them (the strings at 0x01933838..58).
#define MONSTER_BANK_CHARA2 "E_02"
#define MONSTER_BANK_CHARA3 "E_03"
#define MONSTER_BANK_CHARA4 "E_04"
#define MONSTER_BANK_CHARA4B "E_14"
#define MONSTER_BANK_CHARA5_6 "E_05"

static void MonsterSceneAnim(const char *name, int mode)
{
    int bank = ssGetBankID(name);
    if (bank != -1)
        asForceOperateAnimation_Scene(bank, mode);
}

// CT3 0x01854d60
// Freeze (mode != 0) or release the stalkers (ssAtkCharaList2): TRG_CHAR_STOP_CHAR, then for each
// one loaded (its bank != -1) its animation, the enemy SEs, and its own scene
// banks' animations. The ROM has the loop twice, one per mode.
void MonsterAIUpdate(int mode)
{
    // callcheck: asForceOperateAnimation_Scene, ssGetBankID (the ROM's inline step, in the static MonsterSceneAnim)
    int i;
    mode = mode != 0;
    if (mode == 0)
        TrgCharStopFlag &= ~TRG_CHAR_STOP_CHAR;
    else
        TrgCharStopFlag |= TRG_CHAR_STOP_CHAR;
    for (i = 0; i < ATK_CHARA_LIST2_COUNT; i++) {
        int chara = ssAtkCharaList2[i];
        if (Character[chara].bank == -1)
            continue;
        asForceOperateAnimation_Character(chara, mode);
        SSEnemyPauseOnOff(mode);
        if (chara == 6 || chara == 5) {
            MonsterSceneAnim(MONSTER_BANK_CHARA5_6, mode);
        } else if (chara == 4) {
            MonsterSceneAnim(MONSTER_BANK_CHARA4, mode);
            MonsterSceneAnim(MONSTER_BANK_CHARA4B, mode);
        } else if (chara == 3) {
            MonsterSceneAnim(MONSTER_BANK_CHARA3, mode);
        } else if (chara == 2) {
            MonsterSceneAnim(MONSTER_BANK_CHARA2, mode);
        }
    }
}

// ---- the screen filters (slice 4, 2026-11-10) ----------------------------------------------------
// Each filter is a full-screen fan of one colour (SCREEN_FILTER_OBJ, the ROM's sdata packets) drawn
// over the frame with its own draw control. The panic vignette's centre vertex keeps its own colour.
static SCREEN_FILTER_OBJ ScreenFilterObj = SCREEN_FILTER_OBJ_INIT;          /* sdata 0x01937f00 */
static SCREEN_FILTER_OBJ XcmdScreenFilterObj = SCREEN_FILTER_OBJ_INIT;      /* sdata 0x01937fa0 */
static SCREEN_FILTER_OBJ DemoScreenFilterObj = SCREEN_FILTER_OBJ_INIT;      /* sdata 0x01938040 */
alignas(16) static GA_DRAWCTRL_DATA DemoScreenFilterDrawCtrl;               /* bss 0x0193cb80 */
alignas(16) static GA_DRAWCTRL_DATA XcmdScreenFilterDrawCtrl;               /* bss 0x0193cc40 */
alignas(16) static GA_DRAWCTRL_DATA ScreenFilterDrawCtrl;                   /* bss 0x0193cd00 */

// The filters' draw controls (gaSetupDrawCtrlData's 29 argument slots, from the disassembly at
// 0x01853680 / 0x01858cd4 / 0x01858d68): untextured Gouraud, alpha blended, no Z test.
#define SCREEN_FILTER_DRAWCTRL_ARGS(alphaB, alphaFix) {                                    \
    1, 0, 0, 1, 0, 0, 0, 0,      /* PRMODE: IIP, ABE */                                    \
    0, 5, 0x7f, 1, 0, 0, 0, 2,   /* TEST_1: ATST 5, AREF 0x7f, AFAIL 1, ZTST 2 (ATE, ZTE 0) */ \
    0, alphaB, 0, 1, alphaFix,   /* ALPHA_1: A 0 (Cs), B, C 0 (As), D 1 (Cd) */             \
    0,                           /* FBA_1 */                                               \
    0, 0, 1, 4, 0, 0,            /* TEX1_1: MMAG 1, MMIN 4 */                              \
    0}                           /* TEXA */
static const GA_DRAWCTRL_ARGS FILTER_BLEND_DRAWCTRL_ARGS = SCREEN_FILTER_DRAWCTRL_ARGS(1, 0);
static const GA_DRAWCTRL_ARGS FILTER_ADD_DRAWCTRL_ARGS = SCREEN_FILTER_DRAWCTRL_ARGS(2, 0x80);

#define PANIC_FILTER_CENTER_RGBAQ 0x3f80000000640000ull   /* Q 1.0, A 0, B 0x64: the vignette's centre */
#define PANIC_FILTER_EDGE_RGBAQ 0x3f80000000170000ull     /* Q 1.0, B 0x17; A = PanicInfoData.filterAlpha */
#define FILTER_RGBAQ_Q1 0x3f80000000000000ull

// The fade's colour as the fan's RGBAQ (Q 1.0).
static u_long FilterRGBAQ(const FILTER_CHANGE_WORK *work)
{
    return (u_long)(int64_t)work->current[0] << 24 | (u_long)(int64_t)work->current[3] << 16 |
           (u_long)(int64_t)work->current[1] | (u_long)(int64_t)work->current[2] << 8 | FILTER_RGBAQ_Q1;
}

// One frame of a fade: current moves 1/timer of the way to target; at timer 0 it is there, and a
// FILTER_OFF_AT_END filter switches off.
static void FilterChangeStep(FILTER_CHANGE_WORK *work)
{
    int i;

    if (work->timer == 0) {
        for (i = 0; i < 4; i++)
            work->current[i] = work->target[i];
        if (work->active & FILTER_OFF_AT_END)
            work->active = 0;
    } else {
        for (i = 0; i < 4; i++)
            work->current[i] += (work->target[i] - work->current[i]) / work->timer;
        work->timer--;
    }
}

// The draw control of a filter's mode; other modes keep the one set last.
static void FilterSetupDrawCtrl(GA_DRAWCTRL_DATA *ctrl, int mode)
{
    if (mode == FILTER_MODE_BLEND)
        gaSetupDrawCtrlData(ctrl, FILTER_BLEND_DRAWCTRL_ARGS);
    else if (mode == FILTER_MODE_ADD)
        gaSetupDrawCtrlData(ctrl, FILTER_ADD_DRAWCTRL_ARGS);
}

// The fan's six vertices take one colour.
static void FilterSetColor(SCREEN_FILTER_OBJ *obj, int first, u_long rgbaq)
{
    int i;
    for (i = first; i < SCREEN_FILTER_VERTICES; i++)
        obj->vtx[i].rgbaq = rgbaq;
}

// CT3 0x01853620: the panic vignette -- outside a pause and the events 0x26 / 0, while
// TRIG_EVENT_PANIC_48 is set and the filter alpha is not negative: blue edges at filterAlpha over a
// clear centre. The alpha climbs by Panicdef.filterUp to filterMax while PANIC_MODE_FULL, else falls by
// filterDown (below 0 the filter is off until it climbs again).
void PanicFilter(void)
{
    if (pause != 0 || ssTrigCheckEvent(TRIG_EVENT_PANIC_26, 1, 0) != 0 || ssTrigCheckEvent(0, 1, 0) != 0 ||
        ssTrigCheckEvent(TRIG_EVENT_PANIC_48, 1, 0) == 0 || PanicInfoData.filterAlpha < 0)
        return;

    gaSetupDrawCtrlData(&ScreenFilterDrawCtrl, FILTER_BLEND_DRAWCTRL_ARGS);
    ScreenFilterObj.vtx[0].rgbaq = PANIC_FILTER_CENTER_RGBAQ;
    FilterSetColor(&ScreenFilterObj, 1, (u_long)(int64_t)PanicInfoData.filterAlpha << 24 | PANIC_FILTER_EDGE_RGBAQ);
    gaBGT_ExecDrawCtrlData_VIF1(&ScreenFilterDrawCtrl);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&ScreenFilterObj);

    if ((PanicInfoData.mode & PANIC_MODE_FULL) == 0)
        PanicInfoData.filterAlpha -= Panicdef.filterDown;
    else if (PanicInfoData.filterAlpha < Panicdef.filterMax)
        PanicInfoData.filterAlpha += Panicdef.filterUp;
    else
        PanicInfoData.filterAlpha = Panicdef.filterMax;
}

// CT3 0x01858950: the demo filter (the skip's fade to black, main.c).
void DemoScreenFilter(void)
{
    // callcheck: gaSetupDrawCtrlData (the ROM inlines the mode switch: FilterSetupDrawCtrl makes the call)
    if (DemoFilterChangeWork.active == 0)
        return;
    FilterSetupDrawCtrl(&DemoScreenFilterDrawCtrl, DemoFilterChangeWork.mode);
    FilterChangeStep(&DemoFilterChangeWork);
    FilterSetColor(&DemoScreenFilterObj, 0, FilterRGBAQ(&DemoFilterChangeWork));
    gaBGT_ExecDrawCtrlData_VIF1(&DemoScreenFilterDrawCtrl);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&DemoScreenFilterObj);
}

// CT3 0x01858bc0: the X command filter's fade runs without drawing.
void XcmdScreenFilterSkip(void)
{
    if (FilterChangeWork.active != 0)
        FilterChangeStep(&FilterChangeWork);
}

// CT3 0x01858c90: the X command (script) filter, then the demo filter over it.
void XcmdScreenFilter(void)
{
    // callcheck: gaSetupDrawCtrlData (the ROM inlines the mode switch: FilterSetupDrawCtrl makes the call)
    if (FilterChangeWork.active != 0) {
        FilterSetupDrawCtrl(&XcmdScreenFilterDrawCtrl, FilterChangeWork.mode);
        FilterChangeStep(&FilterChangeWork);
        FilterSetColor(&XcmdScreenFilterObj, 0, FilterRGBAQ(&FilterChangeWork));
        gaBGT_ExecDrawCtrlData_VIF1(&XcmdScreenFilterDrawCtrl);
        gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&XcmdScreenFilterObj);
    }
    DemoScreenFilter();
}

// CT3 0x01854410: the player's panic state back to calm, and the panic tuning.
void PanicModeInit(void)
{
    int i;

    TransparentBodyTime = PANIC_TRANSPARENT_TIME;
    AdvantageousTimeTime = PANIC_ADVANTAGEOUS_TIME;
    PanicInfoData.mode = 0;
    PanicInfoData.recoveryItem = -1;
    PanicInfoData.recoveryTime = 0;
    for (i = 0; i < PANIC_RISE_COUNT; i++)
        PanicInfoData.rise[i] = 0.0f;
    PanicInfoData.gauge = 0.0f;
    PanicInfoData.gaugeMax = 10000.0f;
    PanicInfoData.filterAlpha = -1;
    Panicdef.calmFall = 3.5f;
    Panicdef.fullFall = 8.0f;
    Panicdef.beatFrom = 5000.0f;
    Panicdef.f0c = 2.0f;
    Panicdef.filterMax = 30;
    Panicdef.filterUp = 4;
    Panicdef.filterDown = 8;
    ssTrigSetVariable(TRIG_VAR_PANIC, 0, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_3, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_26, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_48, 1, 0);
}

// ---- the panic sounds (slice 3, 2026-11-10) -----------------------------------------------------
static int sPanicBeforeFlg = 1;                 /* data 0x019381f4: EventSoundUpdate's last argument */
static int sBeatCount;                          /* sbss 0x0193cb3c: ga_main_timer at the last heartbeat */
static int sHandleKeyONFinFin;                  /* sbss 0x0193cb40: the looping SEs' handles (0 = off) */
static int sHandleKeyONLavender;                /* sbss 0x0193cb44 */
static int sHandleKeyONInvisible;               /* sbss 0x0193cb48 */

// CT3 0x01853310: the panic, invisible and lavender sounds run while on; when it goes off (and was off
// the frame before too) the looping ones stop.
void EventSoundUpdate(int on)
{
    if (on == 0) {
        if (sPanicBeforeFlg == 0) {
            if (sHandleKeyONFinFin != 0) {
                SdStopSE(sHandleKeyONFinFin);
                sHandleKeyONFinFin = 0;
            }
            if (sHandleKeyONInvisible != 0) {
                SdStopSE(sHandleKeyONInvisible);
                sHandleKeyONInvisible = 0;
            }
            if (sHandleKeyONLavender != 0) {
                SdStopSE(sHandleKeyONLavender);
                sHandleKeyONLavender = 0;
            }
        }
    } else {
        PanicSound();
        InvisibleSound();
        LavenderSound();
    }
    sPanicBeforeFlg = on;
}

// CT3 0x018533b0: the heartbeat. Past Panicdef.beatFrom the gauge picks one of five levels (the top
// one while PANIC_MODE_FULL); each level beats at its own interval and volume, with a pad rattle when
// no motor runs. FinFin loops while PANIC_MODE_FULL.
void PanicSound(void)
{
    static const int beatFrames_2885[PANIC_BEAT_LEVELS] = {52, 44, 38, 32, 28}; /* @2885 0x019381c0 */
    static const int beatVolume_2886[PANIC_BEAT_LEVELS] = {90, 100, 110, 115, 127}; /* @2886 0x019381e0 */
    int level;
    int handle;

    if (!(Panicdef.beatFrom <= PanicInfoData.gauge)) {
        if (sHandleKeyONFinFin != 0) {
            SdStopSESDT(PANIC_SE_JLS, PANIC_SE_FINFIN, 1);
            sHandleKeyONFinFin = 0;
        }
        return;
    }
    level = (int)((PanicInfoData.gauge - Panicdef.beatFrom) / (PanicInfoData.gaugeMax - Panicdef.beatFrom) *
                  (float)PANIC_BEAT_LEVELS);
    if (PanicInfoData.mode & PANIC_MODE_FULL)
        level = PANIC_BEAT_LEVELS - 1;
    else if (level > PANIC_BEAT_LEVELS - 1)
        level = PANIC_BEAT_LEVELS - 1;
    if (beatFrames_2885[level] < ga_main_timer - sBeatCount) {
        handle = SdReqPlaySE(PANIC_SE_JLS, PANIC_SE_HEARTBEAT, 1);
        SdSetParamSE(handle, SD_SE_PARAM_VOLUME | SD_SE_PARAM_30, beatVolume_2886[level], level / 2);
        sBeatCount = ga_main_timer;
        if (act_small == 0 && act_large == 0)
            tiVibrationOn(0, 1, level * PANIC_VIBRATION_STEP + PANIC_VIBRATION_BASE, PANIC_VIBRATION_ARG3);
    }
    if ((PanicInfoData.mode & PANIC_MODE_FULL) == 0) {
        if (sHandleKeyONFinFin != 0) {
            SdStopSESDT(PANIC_SE_JLS, PANIC_SE_FINFIN, 1);
            sHandleKeyONFinFin = 0;
        }
    } else if (sHandleKeyONFinFin == 0) {
        sHandleKeyONFinFin = SdReqPlaySE(PANIC_SE_JLS, PANIC_SE_FINFIN, 1);
    }
}

// CT3 0x01853560: the lavender SE loops while its event is on.
void LavenderSound(void)
{
    int on = (char)ssTrigCheckEvent(TRIG_EVENT_LAVENDER, 1, 0);

    if (sHandleKeyONLavender == 0) {
        if (on != 0)
            sHandleKeyONLavender = SdReqPlaySE(PANIC_SE_JLS, PANIC_SE_LAVENDER, 1);
    } else if (on == 0) {
        SdStopSE(sHandleKeyONLavender);
        sHandleKeyONLavender = 0;
    }
}

// CT3 0x018535c0: the invisible SE loops while its event is on.
void InvisibleSound(void)
{
    int on = (char)ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, 0);

    if (sHandleKeyONInvisible == 0) {
        if (on != 0)
            sHandleKeyONInvisible = SdReqPlaySE(PANIC_SE_JLS, PANIC_SE_INVISIBLE, 1);
    } else if (on == 0) {
        SdStopSE(sHandleKeyONInvisible);
        sHandleKeyONInvisible = 0;
    }
}

// CT3 0x01854260: whether panic may run now: PANIC_CHECK_OFF when a scene forbids it (it wins), else
// PANIC_CHECK_ON when one forces it, else 0.
int PanicCheck(void)
{
    int result = ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_4D, 1, 0) != 0;

    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_8A, 1, 0) != 0)
        result = PANIC_CHECK_ON;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_257, 1, 0) != 0)
        result = PANIC_CHECK_ON;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_5E0, 1, 0) != 0)
        result = PANIC_CHECK_ON;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_AD, 1, 0) != 0)
        result = PANIC_CHECK_ON;
    if (TrgCharStopFlag & TRG_CHAR_STOP_CHAR)
        result = PANIC_CHECK_ON;
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_BATTLE, 1, 0) != 0)
        result = PANIC_CHECK_OFF;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_26, 1, 0) != 0)
        result = PANIC_CHECK_OFF;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_OFF_63, 1, 0) != 0)
        result = PANIC_CHECK_OFF;
    if (ssTrigCheckEvent(TRIG_EVENT_BOSS_ALYS_DEAD, 1, 0) != 0)
        result = PANIC_CHECK_OFF;
    if (SkipMain != 0 || DemoSkip != 0)
        result = PANIC_CHECK_OFF;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_OFF_14, 1, 0) != 0)
        result = PANIC_CHECK_OFF;
    if (ssTrigGetVariable(TRIG_VAR_PANIC_OFF, 1, 0) >= 0)
        result = PANIC_CHECK_OFF;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_OFF_07, 1, 0) != 0)
        result = PANIC_CHECK_OFF;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_OFF_08, 1, 0) != 0)
        result = PANIC_CHECK_OFF;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_OFF_19, 1, 0) != 0)
        result = PANIC_CHECK_OFF;
    return result;
}

// CT3 0x018540b0: the player uses calming item `item` (a PanicRecoveryTbl row): unless panic is
// forbidden it starts working for the row's frames, and TRIG_EVENT_LAVENDER (its looping sound) is on.
void PanicDownSet(int item)
{
    if (PanicCheck() >= 0) {
        PanicInfoData.recoveryItem = (s_char)item;
        PanicInfoData.recoveryTime = (short)PanicRecoveryTbl[item].frames;
        ssTrigRegisterEvent(TRIG_EVENT_LAVENDER, 1, 0);
    }
}

// CT3 0x019390b0 (PanicRecoveryTbl) and 0x019390f0 (HidePointUpValue): the ROM's data.
PANIC_RECOVERY PanicRecoveryTbl[PANIC_RECOVERY_COUNT] = {
    {66.5f, 50.0f, 180}, {1.0f, 1.0f, 100}, {1.0f, 1.0f, 100}, {1.0f, 1.0f, 100}, {1.0f, 1.0f, 100},
};
float HidePointUpValue[HIDE_POINT_LEVELS] = {1.0f, 1.0f, 1.2f, 1.5f, 2.0f};

static void PanicRiseClear(void)
{
    int i;
    for (i = 0; i < PANIC_RISE_COUNT; i++)
        PanicInfoData.rise[i] = 0.0f;
}

// PANIC_RISE_CONST..: the summed constant rises (the ROM skips zeros: same sum).
static float PanicRiseConst(void)
{
    float sum = 0.0f;
    int i;
    for (i = PANIC_RISE_CONST; i < PANIC_RISE_ATTACK; i++)
        if (PanicInfoData.rise[i] != 0.0f)
            sum += PanicInfoData.rise[i];
    return sum;
}

// A full panic ends: the mode bit, the filter off, the two events.
static void PanicFullEnd(void)
{
    PanicInfoData.mode &= ~PANIC_MODE_FULL;
    PanicInfoData.filterAlpha = -1;
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_FULL, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
}

// The calming item's frame: its fall (calm or full column) until its time runs out; the gauge stays >= 0.
static void PanicRecoveryStep(int full)
{
    if (PanicInfoData.recoveryTime == 0) {
        PanicInfoData.recoveryItem = -1;
        ssTrigRemoveEvent(TRIG_EVENT_LAVENDER, 1, 0);
    } else {
        PANIC_RECOVERY *r = &PanicRecoveryTbl[PanicInfoData.recoveryItem];
        PanicInfoData.recoveryTime--;
        PanicInfoData.gauge -= full ? r->fullFall : r->calmFall;
    }
    if (PanicInfoData.gauge < 0.0f)
        PanicInfoData.gauge = 0.0f;
}

// CT3 0x018537b0
// The panic gauge's frame. A dead player ends any panic; TRIG_VAR_PANIC_SUSPEND > 0 resets the
// invisible / advantage timers, ends the calming item and the panic, and empties the gauge. Paused,
// skipping, held (TRIG_EVENT_PANIC_HOLD) or with PANIC_SAFE_BANK in bank 0 nothing moves. With panic
// forbidden (PanicCheck < 0) the constant rises still add (capped below gaugeMax). Otherwise: the
// invisible and advantage timers count down; outside full panic the calming item falls, then (unless
// PanicCheck forces panic on) the gauge rises -- by the first threat (times the hiding factor) plus
// the attacks, or else by the constant rises -- or falls by calmFall when nothing rose and panic is
// not forced; reaching gaugeMax starts a full panic (the world frozen unless a KEEP event says no,
// the sting, the vibration). In full panic the gauge falls (fullFall, or the calming item's) and
// the panic ends at beatFrom. TRIG_VAR_PANIC gets gauge / 100 and the rises are cleared.
void PanicUpdate(void)
{
    int check;
    float mul, constRise, threat;
    int attacked, i;

    EventSoundUpdate(0);
    if (Character[Player_BankID].hp < 1) {
        PanicInfoData.filterAlpha = -1;
        PanicInfoData.mode &= ~PANIC_MODE_FULL;
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_FULL, 1, 0);
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
        ssTrigSetVariable(TRIG_VAR_PANIC, 0, 1, 0);
        return;
    }
    if (ssTrigGetVariable(TRIG_VAR_PANIC_SUSPEND, 1, 0) > 0) {
        if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, 0) != 0) {
            ssTrigRemoveEvent(TRIG_EVENT_INVISIBLE, 1, 0);
            TransparentBodyTime = PANIC_TRANSPARENT_TIME;
        }
        if (ssTrigCheckEvent(TRIG_EVENT_MC_LOADED, 1, 0) != 0) {
            ssTrigRemoveEvent(TRIG_EVENT_MC_LOADED, 1, 0);
            AdvantageousTimeTime = PANIC_ADVANTAGEOUS_TIME;
        }
        if (PanicInfoData.recoveryTime != 0) {
            PanicInfoData.recoveryItem = -1;
            PanicInfoData.recoveryTime = 0;
            ssTrigRemoveEvent(TRIG_EVENT_LAVENDER, 1, 0);
        }
        if (PanicInfoData.mode & PANIC_MODE_FULL)
            PanicFullEnd();
        PanicInfoData.gauge = 0.0f;
        return;
    }
    if (pause != 0 || pause_reservation != 0 || SkipMain != 0 || DemoSkip != 0)
        return;
    if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HOLD, 1, 0) != 0)
        return;
    if (strcmp(PANIC_SAFE_BANK, BankInfo[0].name) == 0)
        return;

    EventSoundUpdate(1);
    check = PanicCheck();
    if (check < 0) {
        PanicInfoData.gauge += PanicRiseConst();
        if (PanicInfoData.gaugeMax <= PanicInfoData.gauge)
            PanicInfoData.gauge = PanicInfoData.gaugeMax - PANIC_GAUGE_CAP_MARGIN;
        PanicRiseClear();
        return;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, 0) != 0) {
        if (TransparentBodyTime < 1) {
            ssTrigRemoveEvent(TRIG_EVENT_INVISIBLE, 1, 0);
            TransparentBodyTime = PANIC_TRANSPARENT_TIME;
        } else {
            TransparentBodyTime--;
        }
    }
    if (ssTrigCheckEvent(TRIG_EVENT_MC_LOADED, 1, 0) != 0) {
        if (AdvantageousTimeTime < 1) {
            ssTrigRemoveEvent(TRIG_EVENT_MC_LOADED, 1, 0);
            AdvantageousTimeTime = PANIC_ADVANTAGEOUS_TIME;
        } else {
            AdvantageousTimeTime--;
        }
    }

    if (!(PanicInfoData.mode & PANIC_MODE_FULL)) {
        if (PanicInfoData.recoveryItem >= 0)
            PanicRecoveryStep(0);
        if (check != PANIC_CHECK_ON) {
            if (ssTrigCheckEvent(TRIG_EVENT_PANIC_HIDING, 1, 0) == 0) {
                mul = 1.0f;
            } else {
                int level = ssTrigGetVariable(TRIG_VAR_HIDE_POINT, 1, 0) + 1;
                mul = level < HIDE_POINT_LEVELS - 1 ? HidePointUpValue[level] : HidePointUpValue[HIDE_POINT_LEVELS - 1];
            }
            constRise = PanicRiseConst();
            threat = 0.0f;
            attacked = 0;
            for (i = PANIC_RISE_THREAT; i < PANIC_RISE_THREAT + PANIC_RISE_THREAT_COUNT; i++)
                if (PanicInfoData.rise[i] != 0.0f) {
                    threat = PanicInfoData.rise[i];
                    break;
                }
            for (i = PANIC_RISE_ATTACK; i < PANIC_RISE_COUNT; i++)
                if (PanicInfoData.rise[i] != 0.0f) {
                    threat += PanicInfoData.rise[i];
                    attacked = 1;
                }
            if (attacked)
                ssTrigRegisterEvent(TRIG_EVENT_PANIC_ATTACKED, 1, 0);
            if (threat == 0.0f) {
                PanicInfoData.gauge += constRise;
                if (PanicInfoData.gaugeMax <= PanicInfoData.gauge)
                    PanicInfoData.gauge = PanicInfoData.gaugeMax - PANIC_GAUGE_CAP_MARGIN;
            } else {
                PanicInfoData.gauge += threat * mul;
                constRise = threat;
            }
            /* nothing rose and panic is not forced: the calm fall */
            if (PanicInfoData.gauge != 0.0f && constRise == 0.0f && check == 0) {
                PanicInfoData.gauge -= Panicdef.calmFall;
                if (PanicInfoData.gauge < 0.0f)
                    PanicInfoData.gauge = 0.0f;
            }
        }
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
        if (PanicInfoData.gaugeMax <= PanicInfoData.gauge) {
            if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MENU, 1, -1) == 0)
                ssMenuUpdate(1);
            if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_PLAYER, 1, -1) == 0)
                PlayerControlUpdate(1);
            if (ssTrigCheckEvent(TRIG_EVENT_PANIC_ON_257, 1, 0) == 0) {
                if (ssTrigCheckEvent(TRIG_EVENT_PANIC_KEEP_MONSTERS, 1, -1) == 0)
                    MonsterAIUpdate(1);
                if (ssTrigCheckEvent(TRIG_EVENT_PANIC_NO_RELOAD, 1, -1) == 0)
                    ssTrigRegisterEvent(TRIG_EVENT_MC_RELOADED, 1, -1);
                asMothPause(1);
            }
            SdReqPlaySE(PANIC_SE_JLS, PANIC_SE_FULL, 1);
            ssTrigRegisterEvent(TRIG_EVENT_PANIC_STARTED, 1, 0);
            PanicInfoData.filterAlpha = 0;
            PanicInfoData.mode |= PANIC_MODE_FULL;
            PanicInfoData.gauge = PanicInfoData.gaugeMax;
            ssTrigRegisterEvent(TRIG_EVENT_PANIC_FULL, 1, 0);
            ssTrigRemoveEvent(TRIG_EVENT_PANIC_48, 1, 0);
            ssTrigRegisterEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
            if (act_small == 0 && act_large == 0)
                tiVibrationOn(0, ssTrigCheckEvent(TRIG_EVENT_PANIC_VIBRATE, 1, 0),
                              ssTrigGetVariable(TRIG_VAR_PANIC_VIBRATE, 1, 0), PANIC_VIBRATE_TIME);
        }
    } else {
        if (PanicInfoData.recoveryItem < 0) {
            if (check == 0)
                PanicInfoData.gauge -= Panicdef.fullFall;
        } else {
            PanicRecoveryStep(1);
        }
        if (Panicdef.beatFrom < PanicInfoData.gauge) {
            if (ssTrigCheckEvent(TRIG_EVENT_PANIC_BEAT, 1, 0) == 0)
                ssTrigRegisterEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
        } else {
            PanicInfoData.gauge = Panicdef.beatFrom;
            PanicInfoData.mode &= ~PANIC_MODE_FULL;
            ssTrigRemoveEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
            ssTrigRemoveEvent(TRIG_EVENT_PANIC_FULL, 1, 0);
        }
    }
    ssTrigSetVariable(TRIG_VAR_PANIC, (int)(PanicInfoData.gauge / PANIC_GAUGE_UNIT), 1, 0);
    PanicRiseClear();
}

// CT3 0x01854d00: the player character stopped (mode != 0) or free (TRG_CHAR_STOP_PLAYER).
void PlayerControlUpdate2(int mode)
{
    if (mode == 0)
        TrgCharStopFlag &= ~TRG_CHAR_STOP_PLAYER;
    else
        TrgCharStopFlag |= TRG_CHAR_STOP_PLAYER;
}

// CT3 0x01851f10
// The game-over sequence's steps (GAME_OVER_*): the music and effects stop with the game-over sting
// (SE 0x3b in slot 4); everything is frozen, the characters freed, every non-global bank undrawn and
// the pad's rattle off (the play time saved for the continue); the effects stop.
void GAME_OVER_END(int step)
{
    int bank;
    switch (step) {
    case GAME_OVER_STOP_SE:
        SdStopSEAll();
        break;
    case GAME_OVER_FREEZE:
        backup_progress_time = progress_time;
        PlayerControlUpdate(1);
        ssMenuUpdate(1);
        PlayerControlUpdate2(1);
        MonsterAIUpdate(1);
        CharacterAllFree();
        for (bank = BANK_GLOBAL_MAX; bank < BANK_MAX; bank++)
            BankInfo[bank].drawOn = DRAW_BANK_OFF;
        tiVibrationOff(0);
        break;
    case GAME_OVER_STING:
        SdStopSQAll(0);
        SdReqPlaySE(GAME_OVER_SE_JLS, GAME_OVER_SE, 1);
        SdStopSEAllSp1();
        break;
    default:                                /* 1 does nothing */
        break;
    }
}

// CT3 0x018550b0: stop (mode != 0) or free the player's keys, with the trigger event that tells scripts.
void PlayerControlUpdate(int mode)
{
    if (mode == 0) {
        TrgCharStopFlag &= ~TRG_CHAR_STOP_KEYS;
        ssTrigRemoveEvent(TRIG_EVENT_PLAYER_STOP, 1, 0);
    } else {
        TrgCharStopFlag |= TRG_CHAR_STOP_KEYS;
        ssTrigRegisterEvent(TRIG_EVENT_PLAYER_STOP, 1, 0);
    }
}

// ---- the letterbox (slice 7, 2026-11-10) ---------------------------------------------------------
float VistaVisionHeightSize;
float VistaVisionCounter;
// (VistaVisionNumber and VistaVisionMode, sbss 0x0193fc90 / 94: unassigned_data, main.c sets them too)

// VistaVisionPacketUp / Down (sdata 0x01937e60 / 0x01937eb0, 0x50 bytes each): DMAtag cnt 4, VIF
// DIRECT 3, a GIFtag (PRE, PRIM SPRITE, REGLIST PRIM RGBAQ XYZ2 XYZ2) and one black sprite across the
// 512-wide frame (GS 0x8000..0xa000). VistaVisionMain writes the alpha and the inner edge's Y.
typedef struct VISTA_PACKET {
    u_char head[0x30];                  /* 0x00: DMAtag, VIF codes, GIFtag */
    u_long prim;                        /* 0x30: PRIM SPRITE */
    u_long rgbaq;                       /* 0x38: black, A = the bars' alpha, Q 1.0 */
    u_long xyz2[2];                     /* 0x40: the corners */
} VISTA_PACKET;
static_assert(sizeof(VISTA_PACKET) == 0x50, "VISTA_PACKET");
#define GS_PRIM_SPRITE 6
#define VISTA_PACKET_HEAD {                                                                          \
    0x04, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x50,   \
    0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x10, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#define VISTA_RGBAQ_INIT 0x3f80000060000000ull
alignas(16) static VISTA_PACKET VistaVisionPacketUp = {
    VISTA_PACKET_HEAD, GS_PRIM_SPRITE, VISTA_RGBAQ_INIT, {0x80008000ull, 0x80008000ull}};
alignas(16) static VISTA_PACKET VistaVisionPacketDown = {
    VISTA_PACKET_HEAD, GS_PRIM_SPRITE, VISTA_RGBAQ_INIT, {0x80008000ull, 0x9c00a000ull}};
#define VISTA_FRAME_HEIGHT 448.0f       /* the GS frame's lines */
#define VISTA_ALPHA_SCALE 1.28f         /* percent to GS alpha (100 -> 0x80) */
#define VISTA_GS_ORIGIN 0x8000          /* the frame's top-left in GS primitive coordinates */
#define VISTA_GS_RIGHT 0xa000           /* x 512 */
#define VISTA_RGBAQ_Q1 0x3f80000000000000ull /* RGBAQ: Q 1.0, black, alpha 0 */
#define VISTA_RINGBUFFER_SIZE 0x8000

// The bars' draw control (gaSetupDrawCtrlData's 29 argument slots from the disassembly at 0x0185aba8):
// flat, untextured, alpha blended (Cs - Cd) * As + Cd; alpha test GEQUAL 0x7f failing to the frame only
// (FB_ONLY: the bars still draw), no depth test; MMIN 4.
static const GA_DRAWCTRL_ARGS VISTA_DRAWCTRL_ARGS = {
    0, 0, 0, 1, 0, 0, 0, 0,       /* PRMODE: ABE */
    0, 5, 0x7f, 1, 0, 0, 0, 2,    /* TEST_1: ATST 5 (GEQUAL), AREF 0x7f, AFAIL 1 (FB_ONLY), ZTST 2 */
    0, 1, 0, 1, 0,                /* ALPHA_1: A 0 (Cs), B 1 (Cd), C 0 (As), D 1 (Cd) */
    0,                            /* FBA_1 */
    0, 0, 1, 4, 0, 0,             /* TEX1_1: MMAG 1, MMIN 4 */
    0};                           /* TEXA */

// CT3 0x0185aa50
// The letterbox's frame: runs the script's request (VISTA_*: at once, or sliding VISTA_STEP percent a
// frame, with its trigger events), then, while any request or VISTA_SHOWN stands, draws the two bars:
// each VistaVisionHeightSize * Counter / 100 lines tall, at Counter * 1.28 alpha.
void VistaVisionMain(void)
{
    alignas(16) GA_DRAWCTRL_DATA ctrl;
    float h;
    u_long rgbaq;

    if (VistaVisionNumber == VISTA_OFF)
        return;
    if (VistaVisionNumber == VISTA_HIDE) {
        VistaVisionCounter -= VISTA_STEP;
        if (VistaVisionCounter < 0.0f) {
            VistaVisionCounter = 0.0f;
            VistaVisionNumber = VISTA_OFF;
            ssTrigRegisterEvent(TRIG_EVENT_VISTA_HIDDEN, 1, 0);
            ssTrigRemoveEvent(TRIG_EVENT_VISTA_ON, 1, 0);
        }
    } else if (VistaVisionNumber == VISTA_SHOW) {
        VistaVisionCounter += VISTA_STEP;
        if (!(VistaVisionCounter <= VISTA_COUNTER_MAX)) {
            VistaVisionCounter = VISTA_COUNTER_MAX;
            VistaVisionNumber = VISTA_SHOWN;
            ssTrigRegisterEvent(TRIG_EVENT_VISTA_SHOWN, 1, 0);
        }
    } else if (VistaVisionNumber == VISTA_HIDE_NOW) {
        VistaVisionCounter = 0.0f;
        VistaVisionNumber = VISTA_OFF;
        ssTrigRegisterEvent(TRIG_EVENT_VISTA_HIDDEN, 1, 0);
        if (ssTrigCheckEvent(TRIG_EVENT_VISTA_KEEP, 1, 0) == 0)
            ssTrigRemoveEvent(TRIG_EVENT_VISTA_ON, 1, 0);
    } else if (VistaVisionNumber == VISTA_SHOW_NOW) {
        VistaVisionCounter = VISTA_COUNTER_MAX;
        VistaVisionNumber = VISTA_SHOWN;
        ssTrigRegisterEvent(TRIG_EVENT_VISTA_SHOWN, 1, 0);
    }
    // (a request that ends at VISTA_OFF still draws this frame, with the counter at 0)

    gaSetupDrawCtrlData(&ctrl, VISTA_DRAWCTRL_ARGS);
    gaBGT_Start(bgt_ringbuffer, VISTA_RINGBUFFER_SIZE);
    gaBGT_ExecDrawCtrlData_VIF1(&ctrl);
    h = VistaVisionHeightSize / VISTA_COUNTER_MAX * VistaVisionCounter;
    rgbaq = (u_long)(int64_t)(int)(VistaVisionCounter * VISTA_ALPHA_SCALE) << 24 | VISTA_RGBAQ_Q1;
    VistaVisionPacketUp.rgbaq = rgbaq;
    // callcheck: __fixsfdi (the int64_t casts: the 1/16-pixel Y, truncated)
    VistaVisionPacketUp.xyz2[1] = ((u_long)((int64_t)(h * 16.0f) + VISTA_GS_ORIGIN) & 0xffff) << 16 | VISTA_GS_RIGHT;
    VistaVisionPacketDown.rgbaq = rgbaq;
    VistaVisionPacketDown.xyz2[0] =
        ((u_long)((int64_t)((VISTA_FRAME_HEIGHT - h) * 16.0f) + VISTA_GS_ORIGIN) & 0xffff) << 16 | VISTA_GS_ORIGIN;
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&VistaVisionPacketDown);
    gaBGT_ExecDrawCommandData_VIF1((GA_DRAWCOMMAND_DATA *)&VistaVisionPacketUp);
    gaBGT_End();
}

// The scene bank (a loaded BANK_TYPE_SHADOW group head) whose room is roomHash, or -1. drawn: also
// require DRAW_BANK_ON (WarpObjectUpdate's test; WarpObject's has none). The ROM inlines it in both.
static int WarpFindRoomBank(u_long roomHash, int drawn)
{
    int i;
    for (i = 0; i < BANK_MAX; i++) {
        if (BankInfo[i].state != BANK_STATE_LOADED)
            continue;
        if (BankInfo[i].type == BANK_TYPE_SHADOW && (!drawn || BankInfo[i].drawOn == DRAW_BANK_ON)
            && roomHash == GetRoomName_file(BankInfo[i].name))
            return i;
        i += BankInfo[i].groupCount - 1;
    }
    return -1;
}

static void WarpReservationClear(WARP_CMD *cmd)
{
    cmd->state = -1;
    cmd->mode = 0;
    cmd->chara = 0;
    cmd->bank = 0;
    cmd->part = 0;
    cmd->bank2 = 0;
    cmd->partHash = 0;
    cmd->roomHash = 0;
}

// CT3 0x0185bd50: each parked warp whose room's bank is now loaded and drawn is queued (by hash,
// through ExecuteWarpObject2) and its slot freed.
void WarpObjectUpdate(void)
{
    // callcheck: GetRoomName_file (in WarpFindRoomBank: the ROM inlines the bank search)
    int i, bank;

    for (i = 0; i < WARP_RESERVE_MAX; i++) {
        WARP_CMD *cmd = &ReservationWarpCommand[i];
        if (cmd->state < 0)
            continue;
        bank = WarpFindRoomBank(cmd->roomHash, 1);
        if (bank < 0)
            continue;
        cmd->bank2 = bank;
        cmd->part = -1;
        cmd->roomHash = cmd->partHash;
        asWarpObjectBody(WARP_QUEUE, cmd);
        WarpReservationClear(cmd);
    }
}

// CT3 0x019423a0 (ReservationWarpCommand), 0x0193cb30 / 34 / 38 (lpWC, iWCnum, iWCcnt: the queue).
WARP_CMD ReservationWarpCommand[WARP_RESERVE_MAX];
static WARP_CMD *lpWC;
static int iWCnum;
static int iWCcnt;

// CT3 0x0185bf80: no parked warps, an empty queue (the ROM unrolls the clear by eight; it drops the
// queue's block without freeing it).
void WarpObjectUpdateInit(void)
{
    int i;
    for (i = 0; i < WARP_RESERVE_MAX; i++)
        WarpReservationClear(&ReservationWarpCommand[i]);
    lpWC = NULL;
    iWCnum = 0;
    iWCcnt = 0;
}

// CT3 0x0185bec0: park a warp in the first free slot (none free: dropped).
void WarpObjectUpdateSet(const WARP_CMD *cmd)
{
    int i;
    for (i = 0; i < WARP_RESERVE_MAX; i++) {
        WARP_CMD *r = &ReservationWarpCommand[i];
        if (r->state < 0) {
            r->state = 1;
            r->mode = cmd->mode;
            r->chara = cmd->chara;
            r->bank = cmd->bank;
            r->roomHash = cmd->roomHash;
            r->partHash = cmd->partHash;
            r->part = -1;
            r->bank2 = 0;
            return;
        }
    }
}

// CT3 0x0185bb20: warp chara to the part partHash of room's scene bank. An empty room name queues
// the warp with partHash's low word as the part index and bank as given. A room not loaded parks it
// (and, for the player, the warps of characters 11 / 12 whose CHARACTER_FLAG_REFRACTION bit is set);
// a loaded one finds the part's index in the bank and queues it.
void WarpObject(const char *room, int mode, int chara, u_long partHash, int bank)
{
    // callcheck: GetRoomName_file (in WarpFindRoomBank: the ROM inlines the bank search)
    WARP_CMD cmd;
    const u_char *hdr;
    int i;

    memset(&cmd, 0, sizeof(cmd));
    cmd.mode = (short)mode;
    cmd.chara = (short)chara;
    cmd.bank = (short)bank;
    if (room[0] == 0) {
        cmd.bank2 = -1;
        cmd.part = (int)partHash;
        cmd.roomHash = 0;
        asWarpObjectBody(WARP_QUEUE, &cmd);
        return;
    }
    cmd.roomHash = ahGetHash(room);
    cmd.bank2 = WarpFindRoomBank(cmd.roomHash, 0);
    if (cmd.bank2 < 0) {
        cmd.part = -1;
        cmd.bank2 = -1;
        cmd.partHash = partHash;
        WarpObjectUpdateSet(&cmd);
        if (cmd.chara == 0) {
            if (Character[WARP_FOLLOWER_1].flags & CHARACTER_FLAG_REFRACTION)
                WarpObject(room, mode, WARP_FOLLOWER_1, partHash, bank);
            if (Character[WARP_FOLLOWER_2].flags & CHARACTER_FLAG_REFRACTION)
                WarpObject(room, mode, WARP_FOLLOWER_2, partHash, bank);
        }
        return;
    }
    cmd.roomHash = 0;
    cmd.bank = (short)cmd.bank2;
    cmd.part = -1;
    hdr = (const u_char *)pModelPartsHeader[cmd.bank2];
    for (i = 0; (s_char)hdr[PARTS_HDR_RECORDS + i * PARTS_REC_SIZE + 2] != PARTS_KIND_END; i++) {
        u_long h;
        memcpy(&h, hdr + PARTS_HDR_RECORDS + i * PARTS_REC_SIZE + PARTS_REC_HASH, sizeof(h));
        if (h == partHash) {
            cmd.part = i;
            break;
        }
    }
    asWarpObjectBody(WARP_QUEUE, &cmd);
}

// The warps (slice 7, 2026-11-10). The ROM writes both bodies out in full; the shared steps are kept
// here as three statics in the ROM's order.
#define WARP_MODE_PARTS 0               /* ExecuteWarpObject*: copy a part's local matrices onto another part */
#define WARP_CHARA_PLAYER 100           /* chara: the player (Player_BankID) */
#define WARP_PART_MATRICES 0x240        /* MODEL_PARTS: three local matrices (0x240, PARTS_ANIM_ROT, _BASE_ROT) */
#define WARP_PART_MATRICES_SIZE 0xc0
#define WARP_ROOM_NAME_LEN 4            /* a room's hash is of its bank file's first four characters */

static void SqueezeCross(float *out, const float *u, const float *v);

static void WarpCopyPartMatrices(MODEL_PARTS *dst, MODEL_PARTS *src)
{
    memcpy((u_char *)dst + WARP_PART_MATRICES, (u_char *)src + WARP_PART_MATRICES, WARP_PART_MATRICES_SIZE);
}

// The character onto the marker part: its position (and last frame's), the part's warpH54, the turn
// about Y from the bank's part 0 row 2 to the marker's (negated when their cross product's y >= 0),
// then a visible character is positioned and its bank's tree rebuilt.
static void WarpPlaceCharacter(int chara, int bank, int part)
{
    CHARACTER *c = &Character[chara];
    MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
    MODEL_PARTS *m = &parts[part];
    float cross[4];

    memcpy(c->pos, m->pos, sizeof(c->pos));
    memcpy(c->posBefore, c->pos, sizeof(c->posBefore));
    c->h100 = m->warpH54;
    c->rot = ahAcos(gaInnerProductXYZ(parts[0].matrix[2], m->matrix[2]));
    SqueezeCross(cross, parts[0].matrix[2], m->matrix[2]);
    if (0.0f <= cross[1])
        c->rot = (int)((float)c->rot * -1.0f);
    if (!(c->flags & CHARACTER_FLAG_SHADOW) && c->bank >= 0) {
        as1CharacterPositioning(c);
        BuildMatrixTree(c->bank, 1, 1);
        asCopyDoubleBufferL2V(pModelPartsHeader[c->bank], 0);
    }
}

// The character now belongs to room `bank`: a room bank (BANK_TYPE_SHADOW) ends its ambient change and
// gives it the room's; roomHash (the bank file's first WARP_ROOM_NAME_LEN characters) and lightBank
// follow, the player's room becomes the current scene, and a forced light set is dropped.
static void WarpEnterRoom(int chara, int bank)
{
    CHARACTER *c = &Character[chara];
    char name[sizeof(BankInfo[0].name)];
    short set[4];
    int i;

    if (BankInfo[bank].type == BANK_TYPE_SHADOW) {
        for (i = 0; i < CHAR_AMB_CHANGE_MAX; i++) {
            if (chara == CharAmbChangeFlag[i].chara) {
                memset(&CharAmbChangeFlag[i], 0, offsetof(CHAR_AMB_CHANGE, shadow));
                CharAmbChangeColor[i] = 0;
            }
        }
        set[0] = (short)chara;
        set[1] = (short)bank;
        set[2] = 0;
        set[3] = 0;
        tnChangeCharAmbientColorSet(set);
    }
    strcpy(name, BankInfo[bank].name);
    name[WARP_ROOM_NAME_LEN] = '\0';
    c->roomHashPrev = c->roomHash;
    c->roomHash = ahGetHash(name);
    c->lightBank = (short)bank;
    if (chara == 0) {
        idCurrentScene = bank;
        RoomInFlagOn();
    }
    CharDirectLightSet[chara][CHAR_DLS_ON] = 0;
    CharDirectLightSet[chara][CHAR_DLS_FADE] = 0;
}

// CT3 0x0185b4c0
// WARP_MODE_PARTS: part `part`'s local matrices onto part `chara` of `bank`. Otherwise character
// `chara` (WARP_CHARA_PLAYER: the player) moves onto part `part` of `bank`, and enters that room
// unless it is already the character's.
void ExecuteWarpObject(int mode, int chara, int part, int bank)
{
    // callcheck: BuildMatrixTree, RoomInFlagOn, ahAcos, ahGetHash, as1CharacterPositioning, asCopyDoubleBufferL2V, gaInnerProductXYZ, strcpy, tnChangeCharAmbientColorSet (the ROM's inline steps, in WarpPlaceCharacter and WarpEnterRoom)
    if (mode == WARP_MODE_PARTS) {
        MODEL_PARTS *parts = (MODEL_PARTS *)pModelPartsHeader[bank];
        WarpCopyPartMatrices(&parts[chara], &parts[part]);
        return;
    }
    if (chara == WARP_CHARA_PLAYER)
        chara = Player_BankID;
    WarpPlaceCharacter(chara, bank, part);
    if (bank != CharID_TO_BankID(chara))
        WarpEnterRoom(chara, bank);
}

// CT3 0x0185ae90
// ExecuteWarpObject with the marker found by name: the part `partHash` of bank2. WARP_MODE_PARTS copies
// its matrices onto part `chara` of `bank`; otherwise the character always enters bank2's room.
void ExecuteWarpObject2(int mode, int bank, int chara, int bank2, u_long partHash)
{
    // callcheck: BuildMatrixTree, RoomInFlagOn, ahAcos, ahGetHash, as1CharacterPositioning, asCopyDoubleBufferL2V, gaInnerProductXYZ, strcpy, tnChangeCharAmbientColorSet (the ROM's inline steps, in WarpPlaceCharacter and WarpEnterRoom)
    int part = asFindModelPartsID(pModelPartsHeader[bank2], partHash);

    if (mode == WARP_MODE_PARTS) {
        WarpCopyPartMatrices(&((MODEL_PARTS *)pModelPartsHeader[bank])[chara],
                             &((MODEL_PARTS *)pModelPartsHeader[bank2])[part]);
        return;
    }
    if (chara == WARP_CHARA_PLAYER)
        chara = Player_BankID;
    WarpPlaceCharacter(chara, bank2, part);
    WarpEnterRoom(chara, bank2);
}

// CT3 0x0185b980: WARP_QUEUE appends cmd (the block grows by WARP_QUEUE_GROW in gHeapJunc);
// WARP_EXECUTE runs every queued warp in order and frees the queue. Other modes do nothing.
void asWarpObjectBody(int mode, const WARP_CMD *cmd)
{
    int i;

    if ((char)mode == WARP_EXECUTE) {
        for (i = 0; i < iWCcnt; i++) {
            const WARP_CMD *w = &lpWC[i];
            if (w->roomHash == 0)
                ExecuteWarpObject(w->mode, w->chara, w->part, w->bank);
            else
                ExecuteWarpObject2(w->mode, w->bank, w->chara, w->bank2, w->roomHash);
        }
        if (lpWC != NULL)
            _SunFree(&gHeapJunc, (void **)&lpWC);
        lpWC = NULL;
        iWCnum = 0;
        iWCcnt = 0;
    } else if ((char)mode == WARP_QUEUE) {
        if (iWCcnt >= iWCnum) {
            WARP_CMD *grown;
            iWCnum += WARP_QUEUE_GROW;
            grown = (WARP_CMD *)_SunAlloc(&gHeapJunc, iWCnum * (int)sizeof(WARP_CMD), 0x10);
            if (lpWC == NULL) {
                lpWC = grown;
            } else {
                for (i = 0; i < iWCcnt; i++)
                    grown[i] = lpWC[i];
                _SunFree(&gHeapJunc, (void **)&lpWC);
                lpWC = grown;
            }
        }
        lpWC[iWCcnt++] = *cmd;
    }
}

// CT3 0x0185c100
// Place the scene's part `part` at the world position pos: pos taken into part 0's frame (the inverse
// of its world matrix, VU0 MULA/MADD over xyzw) lands in the part's drawOnPos, and the part is marked
// PARTS_FLAG43_WARPED. table is the scene's ALL_TABLE (its partsHeader).
void asWarpSceneObject(ALL_TABLE *table, int part, float *pos)
{
    MODEL_PARTS *parts = NULL;
    alignas(16) float inv[4][4];
    int i;
    for (i = 0; i < BANK_MAX; i++)          /* PORT: the bank whose table it is resolves partsHeader */
        if (All_table_TBL[i] == table)
            parts = BANK_PTR(MODEL_PARTS *, i, table->partsHeader);
    if (parts == NULL)
        return;
    gaInversMatrix(inv, parts[0].matrix);
    for (i = 0; i < 4; i++)
        parts[part].drawOnPos[i] = inv[0][i] * pos[0] + inv[1][i] * pos[1] + inv[2][i] * pos[2] + inv[3][i] * pos[3];
    parts[part].flags43 |= PARTS_FLAG43_WARPED;
}

// CT3 0x01854d30: a menu is open (mode != 0) or closed.
void ssMenuUpdate(int mode)
{
    if (mode == 0)
        TrgCharStopFlag &= ~TRG_CHAR_STOP_MENU;
    else
        TrgCharStopFlag |= TRG_CHAR_STOP_MENU;
}

// ---- part lookup and name hashes (slice 1, 2026-11-10) ----

// CT3 0x0185e580
// Each character's low 7 bits, rotated left by 7 * i (mod 64), XORed in: the ROM builds the rotation
// from 32-bit halves in four shift ranges (< 25, < 32, < 58, and the wrap), which is a 64-bit rotate.
// A name of up to 9 characters never overlaps itself, so asConvertHash2String reads it back
// (PARTS_HASH_BIP01 is "Bip01", PARTS_HASH_BASE_POINT "BasePoint").
u_long ahGetHash(const char *name)
{
    u_long hash = 0;
    u_int shift = 0;

    if (name == NULL)
        return 0;
    for (; *name != '\0'; name++) {
        u_long c = (u_long)(*name & HASH_CHAR_MASK);
        hash ^= shift == 0 ? c : (c << shift | c >> (64 - shift));
        shift = (shift + HASH_CHAR_BITS) & 0x3f;
    }
    return hash;
}

// CT3 0x0193ce58 (szHash$257): 9 characters and the terminator.
static char szHash[10];

// CT3 0x0185e420
// The inverse for short names: nine 7-bit characters from the low end, terminated. (The ROM unrolls
// the first eight and writes each byte twice, zero first.)
char *asConvertHash2String(u_long hash)
{
    int i;
    for (i = 0; i < 9; i++) {
        szHash[i] = (char)(hash & HASH_CHAR_MASK);
        hash >>= HASH_CHAR_BITS;
    }
    szHash[9] = '\0';
    return szHash;
}

// CT3 0x0185e360: the global banks (0..5) keep their number; every scene / character bank answers 6.
int asCheckBankID(int bank)
{
    if (bank > GLOBAL_BANK_LAST)
        bank = GLOBAL_BANK_LAST + 1;
    return bank;
}

// CT3 0x0185e380: 1 for a scene / character bank (6..), 0 for a global one.
int asCheckBankType(int bank)
{
    return bank > GLOBAL_BANK_LAST;
}

// CT3 0x0185e150
// The index of the part whose hash matches, -1 when none (or no header). Four names have a cache word
// in the parts header (+0x10..+0x1c): 0 means not looked up yet, -1 not in this model, > 0 the index.
// A miss stores -1 and a hit its index in the cache of the name looked up (an index 0 is stored as 0,
// so part 0 is searched again each time: the ROM's).
int asFindModelPartsID(u_char *header, u_long hash)
{
    int cache = -1;             /* which cache word this name has; -1 none */
    u_int i, count;

    if (header == NULL)
        return -1;
    if (hash == PARTS_HASH_BIP01)
        cache = 0;
    else if (hash == PARTS_HASH_BASE_POINT)
        cache = 1;
    else if (hash == PARTS_HASH_CACHE2)
        cache = 2;
    else if (hash == PARTS_HASH_CACHE3)
        cache = 3;
    if (cache >= 0) {
        int known = *(int *)(header + PARTS_HDR_CACHE + cache * 4);
        if (known == -1)
            return -1;
        if (known > 0)
            return known;
    }

    count = *(u_short *)(header + PARTS_HDR_COUNT);
    for (i = 0; i < count; i++) {
        u_long partHash;
        memcpy(&partHash, header + PARTS_HDR_RECORDS + i * PARTS_REC_SIZE + PARTS_REC_HASH, sizeof(partHash));
        if (partHash == hash)
            break;
    }
    if (cache >= 0)
        *(int *)(header + PARTS_HDR_CACHE + cache * 4) = i < count ? (int)i : -1;
    return i < count ? (int)i : -1;
}

// CT3 0x0185deb0: answers its argument (a character's struct is its own handle).
void *asFindCharacterStruct(void *chara)
{
    return chara;
}

// drawctrl_zbuf (bss 0x0193cdc0, set up on every call; gaSetupDrawCtrlData's 29 argument slots from the
// disassembly at 0x01858f20): flat Gouraud, alpha blended to Cd, alpha test NEVER with AFAIL ZB_ONLY,
// depth tested GREATER: the parts write depth and no colour.
alignas(16) static GA_DRAWCTRL_DATA drawctrl_zbuf;
static const GA_DRAWCTRL_ARGS ZBUF_ONLY_DRAWCTRL_ARGS = {
    1, 0, 0, 1, 0, 0, 0, 0, /* PRMODE: IIP, ABE */
    1, 0, 0, 2, 0, 0, 1, 3, /* TEST_1: ATE, ATST 0 (NEVER), AFAIL 2 (ZB_ONLY), ZTE, ZTST 3 (GREATER) */
    0, 1, 2, 1, 0,          /* ALPHA_1: A 0 (Cs), B 1 (Cd), C 2 (FIX 0), D 1 (Cd) */
    1,                      /* FBA_1 */
    1, 0, 1, 1, 0, 0,       /* TEX1_1: LCM, MMAG 1, MMIN 1 */
    0};                     /* TEXA */

// CT3 0x01858f20
// The depth pre-pass of a PARTS_FLAG_ZONLY_FIRST part: from list entry `index` on, every entry of the
// same part (the list keeps a part's objects together) is drawn depth-only, clipped as main_rtn does
// (inside and not DRAW_OBJ_FLAG_CLIP: the no-clipping path). Returns the part, so main_rtn runs it once.
int OneMaxObjectDrawZOnly(int bank, int index)
{
    int parts = BANK_PTR(DRAW_OBJ_ENTRY *, bank, ALLobject[bank][index])->parts;

    gaSetupDrawCtrlData(&drawctrl_zbuf, ZBUF_ONLY_DRAWCTRL_ARGS);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_zbuf);
    for (; ALLobject[bank][index] != 0; index++) {
        DRAW_OBJ_ENTRY *ent = BANK_PTR(DRAW_OBJ_ENTRY *, bank, ALLobject[bank][index]);
        u_char *obj;
        int clip;

        if (ent->parts != parts)
            break;
        obj = BANK_PTR(u_char *, bank, ent->obj);
        clip = gaObjectClippingCheck(obj);
        if (clip < 0)
            continue;
        *(int *)(obj + DRAW_OBJ_ATTR) = ((MODEL_PARTS *)pModelPartsHeader[bank])[parts].attr;
        if (clip == 0 && !(obj[DRAW_OBJ_FLAGS] & DRAW_OBJ_FLAG_CLIP))
            gaBGT_DrawObject_NoClippingCheck2(obj, 0);
        else
            gaBGT_DrawObject(obj, 0);
    }
    return parts;
}

// ---- the view matrix read back as a camera (slice 1, 2026-11-10, from the disassembly) ----

// CT3 0x0185a670: a focal length in the PS2's projection units (screen_0[8], 512) to a Leica's mm.
float asFocalLength_PS22LEICA(float f)
{
    return f * FOCAL_LEICA_MM / FOCAL_PS2_UNITS;
}

// CT3 0x0185a6b0: the inverse.
float asFocalLength_LEICA2PS2(float f)
{
    return f * FOCAL_PS2_UNITS / FOCAL_LEICA_MM;
}

// CT3 0x0185a8a0
// The view matrix's rotation (its translation row replaced by vf0, (0, 0, 0, 1)) inverted in place:
// its rows are the camera's right, up and front (each copied when asked for). pos = -(row 3).xyz with
// row 3's own w, multiplied through the inverse (all four lanes).
void asViewMatrix2PosRUF(float *view, float *pos, float *right, float *up, float *front)
{
    float inv[4][4];
    float v[4];
    int i;

    memcpy(inv, view, sizeof(inv));
    inv[3][0] = inv[3][1] = inv[3][2] = 0.0f;
    inv[3][3] = 1.0f;
    gaInversMatrix(inv, inv);
    if (right != NULL)
        memcpy(right, inv[0], sizeof(inv[0]));
    if (up != NULL)
        memcpy(up, inv[1], sizeof(inv[1]));
    if (front != NULL)
        memcpy(front, inv[2], sizeof(inv[2]));
    if (pos != NULL) {
        v[0] = -view[12];
        v[1] = -view[13];
        v[2] = -view[14];
        v[3] = view[15];
        for (i = 0; i < 4; i++)
            pos[i] = inv[0][i] * v[0] + inv[1][i] * v[1] + inv[2][i] * v[2] + inv[3][i] * v[3];
    }
}

// ---- texture buffers (slice 3, 2026-11-10) ------------------------------------------------------
// Host: there is no GS. The packets are built byte for byte as the ROM builds them and go through the
// BGT DMA wrapper, which drops VIF1 transfers (gaInitDrawObject.c's rule); the texture itself is to reach
// the GPU decoded at its source, by the host draw module (open-items.md).
#define SUN_TBUFFER_SEND_QWC 8          /* sunBGT_ExecTBufferData_VIF1 sends 8 qwords */
#define SUN_TBUFFER_AD_PAIRS 5
#define SUN_MIP_TBP_LEVELS 7            /* level 0 and MIPTBP1 / MIPTBP2's six */
#define SUN_CLUT8_WORDS 0x100           /* PSMT8 / PSMT8H: a 16 x 16 CT32 CLUT */
#define SUN_CLUT8_W 16
#define SUN_CLUT8_H 16
#define SUN_CLUT4_WORDS 0x40            /* the rest: 8 x 2 */
#define SUN_CLUT4_W 8
#define SUN_CLUT4_H 2
#define SUN_MIP_MIN_WORDS 0x40          /* a level takes at least 0x40 words */
#define SUN_MIP_TBW_MIN 0x40            /* a level's buffer width, at least 64 pixels */
#define GS_BLOCK_SHIFT_SUN 6            /* word address -> TBP / CBP / DBP units (64 words) */
#define TEX0_TCC_RGBA_SUN (1ull << 34)
#define TEX0_CLD_LOAD (1ull << 61)      /* CLD 1: always load the CLUT buffer */
#define GIFTAG_AD_NOEOP(nloop) (0x1000000000000000ull | (u_long)(nloop))
#define GIFTAG_IMAGE(nloop) (0x0800000000008000ull | (u_long)(nloop)) /* FLG IMAGE, EOP */
#define SUN_LOADENV_QWC 6               /* the A+D qwords DIRECT sends before the image */
#define SUN_TEXLOAD_ENV_QWC 7           /* the CNT part: the packet's GA_LOADENV */
#define SUN_TEXLOAD_SEND_QWC 9          /* tag, GA_LOADENV, REF tag */
#define SUN_TEXFLUSH_QWC 3
#define SUN_LOAD_MARK(kind) (0xdddd0000ull | (u_long)(kind))
enum SUN_LOAD_MARK_KIND {               /* the upload packets' DMAtag upper halves (as ga's) */
    SUN_LOAD_CLUT_ENV = 0,
    SUN_LOAD_CLUT_REF = 1,
    SUN_LOAD_MIP_ENV = 2,
    SUN_LOAD_MIP_REF = 3,
    SUN_LOAD_TEXFLUSH = 0xf,
};

typedef struct SUN_TEXLOAD_PACKET {     /* at the scratchpad's mark packet area (EE 0x70000800) */
    u_long dmatag;                      /* 0x00: CNT 7 */
    u_long mark;                        /* 0x08 */
    GA_LOADENV env;                     /* 0x10 */
    u_long refTag;                      /* 0x80: REF, qwc = the image's, addr = the image */
    u_long refMark;                     /* 0x88 */
} SUN_TEXLOAD_PACKET;
static_assert(sizeof(SUN_TEXLOAD_PACKET) == 0x90, "SUN_TEXLOAD_PACKET");

typedef struct SUN_TEXFLUSH_PACKET {
    GA_PACKET_HEAD head;                /* 0x00: CNT 3, DIRECT 2, A+D x 1 (no EOP) */
    GA_AD texflush;                     /* 0x30 */
} SUN_TEXFLUSH_PACKET;
static_assert(sizeof(SUN_TEXFLUSH_PACKET) == 0x40, "SUN_TEXFLUSH_PACKET");

// PORT: tex_attr_A$2750 (data 0x01937c80, asShadowStart's): the shadow buffer read as a texture, 512 x
// 448, CT32, one level whose pixels are already in GS memory (no data words). The ROM's bytes.
#define TEX_ATTR_A_2750_EE 0x01937c80u
static const SUN_TEX_ATTR tex_attr_A_2750 = {512, 448, 9, 9, SCE_GS_PSMCT32, 0, 0x000e0000u, {0, 0, 0, 0}, 0, 0};

// PORT: an EE word of a texture record (TEX_INFO.tim, SUN_TEX_ATTR.clutData) as a host pointer. The ROM
// reads it directly; the host finds the bank whose relocated EE range holds it (as
// ahRelocateSunPs2ObjectArry.c's BankOfEeAddr), or the one game-data record a caller passes
// (tex_attr_A$2750). NULL when neither holds it. Free banks are skipped: the GLBL heap (EE 0x400000, 3 MB)
// and the scene slots (from 0x410000) share EE addresses, and a free slot always has its image, so a global
// bank's records past 0x410000 (ATLD's) resolved into empty slot 6 and the title drew untextured (2026-11-11).
static const void *SunTexEePtr(u_int ee)
{
    int bank, best = -1;

    if (ee == 0)
        return NULL;
    if (ee == TEX_ATTR_A_2750_EE)
        return &tex_attr_A_2750;
    for (bank = 0; bank < BANK_MAX; bank++) {
        if (BankImage[bank] != NULL && BankInfo[bank].state != BANK_STATE_FREE && BankImageEE[bank] <= ee &&
            (best < 0 || BankImageEE[bank] > BankImageEE[best]))
            best = bank;
    }
    return best < 0 ? NULL : BankEePtr(best, ee);
}

// CT3 0x01859530: a texture buffer packet from a texture record and where it sits in GS memory: TEX0_1
// (TCC RGBA, CLD 1), CLAMP_1, the mip levels packed after level 0 in MIPTBP1 / MIPTBP2, TEXFLUSH; then
// what sunBGT_LoadGSTexClutData_VIF1 needs. Addresses are GS word addresses. Unlike ga's copy, level 0's
// TBW is the width >> 6 as is, the levels' widths have one minimum, and CLD is always 1.
void sunSetupTBufferData(void *tbuffer, TEX_INFO tex)
{
    sunSetupTBufferDataFrom(tbuffer, tex, (const SUN_TEX_ATTR *)SunTexEePtr(tex.tim));
}

// PORT: sunSetupTBufferData with the record as a host pointer: for the records the game builds on its
// stack or keeps in its own data (Clipping_Portal.c's tiVRAMViewer, CaptureDBuffer), which have no bank
// EE address. tex.tim is not read.
void sunSetupTBufferDataFrom(void *tbuffer, TEX_INFO tex, const SUN_TEX_ATTR *attr)
{
    SUN_TBUFFER_DATA *tbuf = (SUN_TBUFFER_DATA *)tbuffer;
    u_int width, height, psm, bpp, clutWords, clutW, clutH;
    u_int levelTbp[SUN_MIP_TBP_LEVELS];
    u_int levelTbw[SUN_MIP_TBP_LEVELS];
    int level;

    tbuf->head.dmatag = GA_DMATAG_CNT(SUN_TBUFFER_SEND_QWC - 1);
    tbuf->head.mark = GA_PACKET_MARK(GA_PACKET_TBUFFER);
    tbuf->head.vifcode[0] = VC_NOP;
    tbuf->head.vifcode[1] = VC_NOP;
    tbuf->head.vifcode[2] = VC_NOP;
    tbuf->head.vifcode[3] = GA_VIFCODE_DIRECT(SUN_TBUFFER_SEND_QWC - 2);
    tbuf->head.giftag = GA_GIFTAG_AD(SUN_TBUFFER_AD_PAIRS);
    tbuf->head.giftagRegs = GA_GIFTAG_REGS_AD;
    if (attr == NULL)
        return; /* PORT: a record the host cannot find (the ROM would read it); the rest stays as was */

    width = attr->width;
    height = attr->height;
    psm = attr->psm;
    if (psm == SCE_GS_PSMT8H || psm == SCE_GS_PSMT8) {
        clutW = SUN_CLUT8_W;
        clutWords = SUN_CLUT8_WORDS;
        clutH = SUN_CLUT8_H;
    } else {
        clutWords = SUN_CLUT4_WORDS;
        clutW = SUN_CLUT4_W;
        clutH = SUN_CLUT4_H;
    }
    tbuf->tex0.data = ((u_long)tex.tfx << 35) + ((u_long)attr->th << 30) + ((u_long)attr->tw << 26) +
                      ((u_long)psm << 20) + (u_long)(tex.tbp >> GS_BLOCK_SHIFT_SUN) +
                      ((u_long)(width >> 6) << 14) + TEX0_CLD_LOAD + TEX0_TCC_RGBA_SUN +
                      ((u_long)((tex.cbp + clutWords * tex.clut) >> GS_BLOCK_SHIFT_SUN) << 37);
    tbuf->tex0.addr = SCE_GS_TEX0_1;
    tbuf->clamp.data = ((u_long)tex.maxv << 34) + ((u_long)tex.minv << 24) + ((u_long)tex.maxu << 14) +
                       ((u_long)tex.minu << 4) + (u_long)tex.wms + ((u_long)tex.wmt << 2);
    tbuf->clamp.addr = SCE_GS_CLAMP_1;

    // Bits per pixel as the ROM sizes the levels: PSMCT16 falls to 4 (ga's slip too), PSMT8H to 32.
    if (psm == SCE_GS_PSMCT16)
        bpp = 4;
    else if (psm == SCE_GS_PSMT8H || psm == SCE_GS_PSMCT24 || psm == SCE_GS_PSMCT32)
        bpp = 32;
    else if (psm != SCE_GS_PSMT8)
        bpp = 4;
    else
        bpp = 8;

    // Each level after the one before it, in words (w * h * bpp / 32), at least 0x40; widths halve, at
    // least 64. MIPTBP1 / MIPTBP2 get levels 1..6; only 0..3 are kept for the upload.
    levelTbp[0] = tex.tbp;
    levelTbw[0] = width;
    for (level = 1; level < SUN_MIP_TBP_LEVELS; level++) {
        u_int words = bpp * (width >> (level - 1)) * (height >> (level - 1)) >> 5;

        if (words < SUN_MIP_MIN_WORDS)
            words = SUN_MIP_MIN_WORDS;
        levelTbp[level] = levelTbp[level - 1] + words;
        levelTbw[level] = width >> level;
        if (levelTbw[level] < SUN_MIP_TBW_MIN)
            levelTbw[level] = SUN_MIP_TBW_MIN;
    }
    tbuf->miptbp1.data = (u_long)(levelTbp[1] >> 6) + ((u_long)(levelTbw[1] >> 6) << 14) +
                         ((u_long)(levelTbp[2] >> 6) << 20) + ((u_long)(levelTbw[2] >> 6) << 34) +
                         ((u_long)(levelTbp[3] >> 6) << 40) + ((u_long)(levelTbw[3] >> 6) << 54);
    tbuf->miptbp1.addr = SCE_GS_MIPTBP1_1;
    tbuf->miptbp2.data = (u_long)(levelTbp[4] >> 6) + ((u_long)(levelTbw[4] >> 6) << 14) +
                         ((u_long)(levelTbp[5] >> 6) << 20) + ((u_long)(levelTbw[5] >> 6) << 34) +
                         ((u_long)(levelTbp[6] >> 6) << 40) + ((u_long)(levelTbw[6] >> 6) << 54);
    tbuf->miptbp2.addr = SCE_GS_MIPTBP2_1;
    tbuf->texflush.data = 0;
    tbuf->texflush.addr = SCE_GS_TEXFLUSH;

    tbuf->mipMax = attr->mipMax;
    for (level = 0; level < SUN_MIP_LEVELS; level++) {
        tbuf->mipData[level] = attr->mipData[level];
        tbuf->mipTbp[level] = levelTbp[level];
        tbuf->mipTbw[level] = levelTbw[level];
    }
    tbuf->psm = attr->psm;
    tbuf->width = width;
    tbuf->height = height;
    tbuf->clutCount = attr->clutCount;
    tbuf->clutData = attr->clutData;
    tbuf->clutWords = clutWords;
    tbuf->cbp = tex.cbp;
    tbuf->clutWidth = clutW;
    tbuf->clutHeight = clutH;
}

// CT3 0x01859390: an upload's GS set-up (as gaBGT_SetLoadGSEnv): destination (dbp, dbw in their GS units,
// dpsm), the rectangle at (dsax, dsay) of rrw x rrh, host to local, and the IMAGE tag. Returns the image's
// qwords (0xffff for a format it does not size).
int sunBGT_SetLoadGSEnv(GA_LOADENV *env, u_int dbp, u_int dbw, u_int dpsm, u_int dsax, u_int dsay, int rrw,
                        int rrh)
{
    int qwc = 0xffff;

    if (dpsm == SCE_GS_PSMT4HH || dpsm == SCE_GS_PSMT4HL || dpsm == SCE_GS_PSMT4)
        qwc = rrw * rrh >> 5;
    else if (dpsm == SCE_GS_PSMT8H || dpsm == SCE_GS_PSMT8)
        qwc = rrw * rrh >> 4;
    else if (dpsm == SCE_GS_PSMCT16)
        qwc = rrw * rrh >> 3;
    else if (dpsm == SCE_GS_PSMCT24)
        qwc = rrw * rrh * 3 >> 4;
    else if (dpsm == SCE_GS_PSMCT32)
        qwc = rrw * rrh >> 2;
    env->vifcode[0] = VC_NOP;
    env->vifcode[1] = VC_NOP;
    env->vifcode[2] = VC_NOP;
    env->vifcode[3] = GA_VIFCODE_DIRECT((qwc + SUN_LOADENV_QWC) & 0xffff);
    env->giftag = GIFTAG_AD_NOEOP(4);
    env->giftagRegs = GA_GIFTAG_REGS_AD;
    env->bitbltbuf.data = ((u_long)dpsm << 56) | ((u_long)dbp << 32) | ((u_long)dbw << 48);
    env->bitbltbuf.addr = SCE_GS_BITBLTBUF;
    env->trxpos.data = (u_long)dsax | ((u_long)dsay << 16);
    env->trxpos.addr = SCE_GS_TRXPOS;
    env->trxreg.data = (u_long)(u_int)rrw | ((u_long)(u_int)rrh << 32);
    env->trxreg.addr = SCE_GS_TRXREG;
    env->trxdir.data = 0;
    env->trxdir.addr = SCE_GS_TRXDIR;
    env->imageTag = GIFTAG_IMAGE((u_int)qwc);
    env->imageTagRegs = 0;
    return qwc;
}

// A REF tag's address from an EE address, as the ROM converts it (ga's GaRefTagAddr, inlined twice here).
static u_int SunRefTagAddr(u_int ee)
{
    u_int addr = ee;
    u_int bit30;

    if (ee >= SCRATCHPAD_EE_BASE)
        addr = ee - SCRATCHPAD_EE_BASE;
    bit30 = addr & 0x40000000u;
    return (bit30 << 1) | ((addr & 0x1fffffffu) ^ (bit30 >> 2));
}

static void SunSetTexLoadRef(SUN_TEXLOAD_PACKET *packet, int envKind, u_int data, int qwc)
{
    packet->refTag = ((u_long)SunRefTagAddr(data) << 32) | (u_long)(u_int)qwc | GA_DMATAG_REF_EMPTY;
    packet->refMark = SUN_LOAD_MARK(envKind + 1);
}

// CT3 0x018590b0: upload a texture buffer's CLUTs (CT32, at cbp + clutWords * i) and its levels
// 0..mipMax, each through the scratchpad's upload packet with a REF to the data, after one TEXFLUSH
// (ga's copy flushes after as well; this one does not).
void sunBGT_LoadGSTexClutData_VIF1(void *tbuffer)
{
    SUN_TBUFFER_DATA *tbuf = (SUN_TBUFFER_DATA *)tbuffer;
    SUN_TEXFLUSH_PACKET *flush = (SUN_TEXFLUSH_PACKET *)SCRATCHPAD(GA_SPR_MARK_PACKET);
    SUN_TEXLOAD_PACKET *load = (SUN_TEXLOAD_PACKET *)SCRATCHPAD(GA_SPR_MARK_PACKET);
    const u_int *clutTable = (const u_int *)SunTexEePtr(tbuf->clutData); /* PORT: an EE word */
    u_int i;
    int qwc;

    gaDmaSync_fromSPR();
    flush->head.dmatag = GA_DMATAG_CNT(SUN_TEXFLUSH_QWC);
    flush->head.mark = SUN_LOAD_MARK(SUN_LOAD_TEXFLUSH);
    flush->head.vifcode[0] = VC_NOP;
    flush->head.vifcode[1] = VC_NOP;
    flush->head.vifcode[2] = VC_NOP;
    flush->head.vifcode[3] = GA_VIFCODE_DIRECT(SUN_TEXFLUSH_QWC - 1);
    flush->head.giftag = GIFTAG_AD_NOEOP(1);
    flush->head.giftagRegs = GA_GIFTAG_REGS_AD;
    flush->texflush.data = 0;
    flush->texflush.addr = SCE_GS_TEXFLUSH;
    gaBGT_DmaSendN_fromSPR_VIF1(flush, SUN_TEXFLUSH_QWC + 1);

    for (i = 0; i < tbuf->clutCount; i++) {
        gaDmaSync_fromSPR();
        load->dmatag = GA_DMATAG_CNT(SUN_TEXLOAD_ENV_QWC);
        load->mark = SUN_LOAD_MARK(SUN_LOAD_CLUT_ENV);
        qwc = sunBGT_SetLoadGSEnv(&load->env, (tbuf->cbp + tbuf->clutWords * i) >> GS_BLOCK_SHIFT_SUN, 1,
                                  SCE_GS_PSMCT32, 0, 0, (int)tbuf->clutWidth, (int)tbuf->clutHeight);
        SunSetTexLoadRef(load, SUN_LOAD_CLUT_ENV, clutTable != NULL ? clutTable[i] : 0, qwc);
        gaBGT_DmaSendN_fromSPR_VIF1(load, SUN_TEXLOAD_SEND_QWC);
    }
    // mipMax -1: no level (the ROM's unsigned compare against mipMax + 1 = 0). PORT: at most the four
    // levels the record keeps (the ROM would read past mipData into mipTbp; no texture seen asks it).
    for (i = 0; i < (u_int)tbuf->mipMax + 1 && i < SUN_MIP_LEVELS; i++) {
        gaDmaSync_fromSPR();
        load->dmatag = GA_DMATAG_CNT(SUN_TEXLOAD_ENV_QWC);
        load->mark = SUN_LOAD_MARK(SUN_LOAD_MIP_ENV);
        qwc = sunBGT_SetLoadGSEnv(&load->env, tbuf->mipTbp[i] >> GS_BLOCK_SHIFT_SUN,
                                  tbuf->mipTbw[i] >> GS_BLOCK_SHIFT_SUN, tbuf->psm, 0, 0,
                                  (int)(tbuf->width >> (i & 0x1f)), (int)(tbuf->height >> (i & 0x1f)));
        SunSetTexLoadRef(load, SUN_LOAD_MIP_ENV, tbuf->mipData[i], qwc);
        gaBGT_DmaSendN_fromSPR_VIF1(load, SUN_TEXLOAD_SEND_QWC);
    }
}

// PORT: the GS the packet below sets up is the host draw module (alyssapan_drawga.h): it gets the texture
// record's level 0 and the CLUT TEX0.CBP picks, in place in the bank, as the packet left them. The CLUT
// is found the way sunSetupTBufferData put it: CBP = cbp + clutWords x clut, so clut = (CBP - cbp) /
// clutWords, an index into SUN_TEX_ATTR.clutData's list (sunBGT_LoadGSTexClutData_VIF1 uploads them
// in that order).
#define TEX0_CBP_SHIFT 37
#define TEX0_CBP_MASK 0x3fffull
static void SunTBufferToHost(const SUN_TBUFFER_DATA *tbuf)
{
    AlyssaPanGaTexture texture;
    const u_int *clutList;
    u_int cbp;

    memset(&texture, 0, sizeof(texture));
    texture.tex0 = tbuf->tex0.data;
    texture.clamp = tbuf->clamp.data;
    texture.width = (int)tbuf->width;
    texture.height = (int)tbuf->height;
    texture.pixels = SunTexEePtr(tbuf->mipData[0]);
    clutList = (const u_int *)SunTexEePtr(tbuf->clutData);
    cbp = (u_int)((tbuf->tex0.data >> TEX0_CBP_SHIFT) & TEX0_CBP_MASK) << GS_BLOCK_SHIFT_SUN;
    if (clutList != NULL && tbuf->clutWords != 0 && cbp >= tbuf->cbp) {
        u_int clut = (cbp - tbuf->cbp) / tbuf->clutWords;

        if (clut < tbuf->clutCount)
            texture.clut = (const uint32_t *)SunTexEePtr(clutList[clut]);
    }
    AlyssaPan_DrawGaTexture(&texture);
}

// CT3 0x01859520: a tail jump -- send the packet's 8 qwords through the BGT ring.
void sunBGT_ExecTBufferData_VIF1(void *tbuffer)
{
    SunTBufferToHost((const SUN_TBUFFER_DATA *)tbuffer);
    gaBGT_DmaSendN_fromSPR_VIF1(tbuffer, SUN_TBUFFER_SEND_QWC);
}

// CT3 0x01854560: gaInit3dlibWork with the copy built in: the 3dlib work (32 floats, ga3dlibwork) is
// copied to dst (the scratchpad's 0x400 in every caller), then the renderer's constants are derived
// from the copy, store for store as gaInit3dlibWork does (the ROM keeps dst's address at +0xec; the
// host keeps its EE address, as gaInit3dlibWork does).
void tnInit3dlibWork(void *work, void *dst)
{
    GA_SPR_WORK *spr = (GA_SPR_WORK *)SCRATCHPAD(GA_SPR_WORK_OFFSET);
    float *w = (float *)dst;
    int i;

    spr->work3dlibEE = SCRATCHPAD_EE_BASE + (u_int)((u_char *)dst - g_eeScratchpad);
    for (i = 0; i < 32; i++)
        w[i] = ((const float *)work)[i];
    spr->viewScaleX = w[2] / w[0] / AlyssaPan_GaWiden(); /* PORT: widened as gaInit3dlibWork's */
    spr->viewScaleY = w[3] / w[1];
    spr->zScale = 1.0f / (w[11] - w[10]);
    spr->work6 = w[6];
    spr->zOffset = ((w[11] + w[10]) * -0.5f) / (w[11] - w[10]);
    spr->work21 = w[21];
    spr->work22 = w[22];
    spr->work23x16 = w[23] * 16.0f;
    spr->zero06c = 0;
    spr->work26 = w[26];
    spr->work27 = w[27];
    spr->work6b = w[6];
    spr->work9 = w[9];
    spr->work25 = w[25];
    spr->colourMax = 255.0f;
    spr->work24 = w[24];
    // (the ROM ends with SYNC)
}

// CT3 0x0185a6f0
// As asViewMatrix2PosRUF, but pos is the camera's eye: the view origin moved back along front by
// three focal lengths in mm (asFocalLength_PS22LEICA(screen_0[8]) * -3); w stays the origin's.
void asViewMatrix2CamRUF(float (*view)[4], float *pos, float *right, float *up, float *forward)
{
    float origin[4], r[4], u[4], f[4];
    float back;

    asViewMatrix2PosRUF(&view[0][0], origin, r, u, f);
    if (right != NULL)
        memcpy(right, r, sizeof(r));
    if (up != NULL)
        memcpy(up, u, sizeof(u));
    if (forward != NULL)
        memcpy(forward, f, sizeof(f));
    if (pos != NULL) {
        back = asFocalLength_PS22LEICA(screen_0[SCREEN_PROJ_DIST]) * -CAMERA_EYE_FOCALS;
        pos[0] = origin[0] + f[0] * back;
        pos[1] = origin[1] + f[1] * back;
        pos[2] = origin[2] + f[2] * back;
        pos[3] = origin[3];
    }
}

// CT3 0x01a4b900 (scene_alpha_anim): one ambient-alpha animation per bank.
SCENE_ALPHA_ANIM scene_alpha_anim[BANK_MAX];

// CT3 0x01856e70
// Each part's alpha (MODEL_PARTS.alpha) starts as its ALL_TABLE.partsPos w, up to the record whose
// kind byte is -1; the bank's alpha animation is cleared.
void AmbientAlphaInit(int scene)
{
    u_char *header = pModelPartsHeader[scene];
    float (*partsPos)[4] = BANK_PTR(float (*)[4], scene, All_table_TBL[scene]->partsPos);
    MODEL_PARTS *part;
    int i;

    for (i = 0, part = (MODEL_PARTS *)header; (s_char)part->kind != PARTS_KIND_END; i++, part = (MODEL_PARTS *)((u_char *)part + PARTS_REC_SIZE))
        part->alpha = partsPos[i][3];
    memset(&scene_alpha_anim[scene], 0, sizeof(scene_alpha_anim[scene]));
}

// ---- the paged textures (slice 10) ------------------------------------------------------------------
// PORT: every word of the table is an EE address inside the scene's bank (the relocator moved them all),
// read with BANK_PTR; the words copied into the SUN_TEX_ATTR stay EE addresses (sunSetupTBufferData
// resolves them).
static char page_set;                           /* sbss 0x0193cb4c: the double buffer's preloaded page is
                                                   the next one (1) or the previous one (0) */

static const char tex_page_dir[] = TEX_PAGE_DIR; /* @4243 */

// FSLoad page `page` of a set's file list into its buffer slot (page % count); with `cancel`, a load
// still queued for that slot is cancelled first (TexFileSetNext / Page; TexFileSetInit does not).
static void TexPageLoad(int scene, const TEX_FILE_LIST *files, TEX_PAGE_SLOT *slot, int page, int cancel)
{
    char dir[sizeof(tex_page_dir)];
    const u_int *pages = BANK_PTR(const u_int *, scene, files->pages);

    memcpy(dir, tex_page_dir, sizeof(dir));
    if (cancel && slot->handle >= 0)
        FSLoadCancelQue(slot->handle);
    slot->handle = FSLoad(dir, BANK_PTR(char *, scene, pages[page]), BANK_PTR(void *, scene, slot->mipData[0]));
}

static void TexPageWait(TEX_PAGE_SLOT *slot)
{
    FSLoadEndWait(slot->handle);
    slot->handle = TEX_PAGE_IDLE;
}

// The slide sets: the page's packed data unpacked into its slot.
static void TexPageThaw(int scene, const TEX_FILE_LIST *files, TEX_PAGE_SLOT *slot, int page)
{
    const u_int *pages = BANK_PTR(const u_int *, scene, files->pages);
    tnSlideThawing(BANK_PTR(const u_char *, scene, pages[page]), BANK_PTR(u_char *, scene, slot->mipData[0]));
}

// The first draw-list record of part `part` (objTrans, then objOpaque) shows the slot: its texture
// record's mip levels and first CLUT become the slot's.
static void TexFileSetPatch(int scene, int part, const TEX_PAGE_SLOT *slot)
{
    const ALL_TABLE *tbl = All_table_TBL[scene];
    int pass, i;

    for (pass = 0; pass < 2; pass++) {
        const u_int *list = BANK_PTR(const u_int *, scene, pass == 0 ? tbl->objTrans : tbl->objOpaque);
        for (i = 0; list[i] != 0; i++) {
            const DRAW_OBJ_ENTRY *e = BANK_PTR(const DRAW_OBJ_ENTRY *, scene, list[i]);
            if (e->parts == part) {
                const TEX_INFO *tex = BANK_PTR(const TEX_INFO *, scene, e->tex);
                SUN_TEX_ATTR *attr = BANK_PTR(SUN_TEX_ATTR *, scene, tex->tim);
                int l;
                for (l = 0; l < SUN_MIP_LEVELS; l++)
                    attr->mipData[l] = slot->mipData[l];
                *BANK_PTR(u_int *, scene, attr->clutData) = slot->clut;
                return;
            }
        }
    }
}

// The ring's first page around `page` (count > 2 buffers): half the ring before it.
static int TexRingFirst(int page, int count)
{
    return page - (count - 1) / 2;
}

// CT3 0x01856070: a scene's paged textures load their current page (and preload the next one, or the
// ring around it) and patch it into their part. TexFileSysInitStop set: nothing loads, the slots are
// only marked idle (the pages are already there), and the stop is cleared.
void TexFileSetInit(int scene)
{
    // callcheck: FSLoad, FSLoadEndWait, tnSlideThawing (the ROM inlines the page steps: TexPageLoad / Wait / Thaw make them)
    const TEX_FILE_SET_TABLE *tbl = BANK_PTR(const TEX_FILE_SET_TABLE *, scene, All_table_TBL[scene]->texFileSet);
    TEX_FILE_SET *set;

    if (tbl == NULL)
        return;
    if (TexFileSysInitStop == 0) {
        for (set = BANK_PTR(TEX_FILE_SET *, scene, tbl->sets); set->parts != 0; set++) {
            int part = *BANK_PTR(const int *, scene, set->parts);
            const TEX_FILE_LIST *files = BANK_PTR(const TEX_FILE_LIST *, scene, tbl->files) + set->files;
            const TEX_PAGE_BUFFERS *bufs = BANK_PTR(const TEX_PAGE_BUFFERS *, scene, tbl->buffers) + set->buffers;
            TEX_PAGE_SLOT *slots = BANK_PTR(TEX_PAGE_SLOT *, scene, bufs->slots);

            if (set->slide == 0) {
                TEX_PAGE_SLOT *cur = &slots[set->page % bufs->count];
                TexPageLoad(scene, files, cur, set->page, 0);
                TexPageWait(cur);
                if (bufs->count == TEX_PAGE_BUFFERS_DOUBLE) {
                    int next = set->page + 1;
                    if (next < files->count)
                        TexPageLoad(scene, files, &slots[next % 2], next, 0);
                    page_set = 1;
                } else if (bufs->count != 1) {
                    int first = TexRingFirst(set->page, bufs->count), p;
                    for (p = first; p < bufs->count + first; p++) {
                        /* the ROM's bound: the last page is never preloaded */
                        if (p != set->page && p >= 0 && p < files->count - 1)
                            TexPageLoad(scene, files, &slots[p % bufs->count], p, 0);
                    }
                }
            } else {
                TexPageThaw(scene, files, &slots[set->page % bufs->count], set->page);
            }
            TexFileSetPatch(scene, part, &slots[set->page % bufs->count]);
        }
    } else {
        for (set = BANK_PTR(TEX_FILE_SET *, scene, tbl->sets); set->parts != 0; set++) {
            const TEX_FILE_LIST *files = BANK_PTR(const TEX_FILE_LIST *, scene, tbl->files) + set->files;
            const TEX_PAGE_BUFFERS *bufs = BANK_PTR(const TEX_PAGE_BUFFERS *, scene, tbl->buffers) + set->buffers;
            TEX_PAGE_SLOT *slots = BANK_PTR(TEX_PAGE_SLOT *, scene, bufs->slots);

            if (set->slide != 0)
                continue;
            slots[set->page % bufs->count].handle = TEX_PAGE_IDLE;
            if (bufs->count == TEX_PAGE_BUFFERS_DOUBLE) {
                if (set->page + 1 < files->count)
                    slots[(set->page + 1) % 2].handle = TEX_PAGE_IDLE;
            } else if (bufs->count != 1) {
                int first = TexRingFirst(set->page, bufs->count), p;
                if (first < 0)
                    first = 0;
                for (p = first; p < bufs->count + first; p++) {
                    if (p != set->page && p >= 0 && p < files->count - 1)
                        slots[p % bufs->count].handle = TEX_PAGE_IDLE;
                }
            }
        }
        page_set = 1;
        TexFileSysInitStop = 0;
    }
}

// The set whose part list holds `part` (TexFileSetPage); NULL when none does.
static TEX_FILE_SET *TexFileSetFind(int scene, const TEX_FILE_SET_TABLE *tbl, int part)
{
    TEX_FILE_SET *set;

    for (set = BANK_PTR(TEX_FILE_SET *, scene, tbl->sets); set->parts != 0; set++) {
        const int *parts = BANK_PTR(const int *, scene, set->parts);
        int i;
        for (i = 0; parts[i] != -1; i++) {
            if (parts[i] == part)
                return set;
        }
    }
    return NULL;
}

// CT3 0x01855180: part's set turns one page forward (back == 0) or back, short of either end. One
// buffer: the page loads and is waited for. Two: the page is waited for (or loaded when the preload
// went the other way) and its neighbour in that direction preloads. A ring: the page is waited for and
// the page half a ring ahead (or behind) preloads. Slide sets unpack the page. Then the part shows it.
void TexFileSetNext(int scene, int part, int back)
{
    // callcheck: FSLoad, FSLoadCancelQue, FSLoadEndWait, tnSlideThawing (the ROM inlines the page steps: TexPageLoad / Wait / Thaw make them)
    const TEX_FILE_SET_TABLE *tbl = BANK_PTR(const TEX_FILE_SET_TABLE *, scene, All_table_TBL[scene]->texFileSet);
    TEX_FILE_SET *set, *last = NULL;
    const TEX_FILE_LIST *files;
    const TEX_PAGE_BUFFERS *bufs;
    TEX_PAGE_SLOT *slots;
    int count;

    if (tbl == NULL)
        return;
    set = TexFileSetFind(scene, tbl, part);
    if (set == NULL) {
        /* the ROM goes on with the last set it looked at (none: an unset register) */
        for (set = BANK_PTR(TEX_FILE_SET *, scene, tbl->sets); set->parts != 0; set++)
            last = set;
        if (last == NULL)
            return; /* PORT: the ROM would read garbage */
        set = last;
    }
    files = BANK_PTR(const TEX_FILE_LIST *, scene, tbl->files) + set->files;
    bufs = BANK_PTR(const TEX_PAGE_BUFFERS *, scene, tbl->buffers) + set->buffers;
    slots = BANK_PTR(TEX_PAGE_SLOT *, scene, bufs->slots);
    count = bufs->count;

    if (!back) {
        if (set->page >= files->count - 1)
            return;
        set->page++;
        if (set->slide != 0) {
            TexPageThaw(scene, files, &slots[set->page % count], set->page);
        } else if (count < 2) {
            TexPageLoad(scene, files, &slots[set->page % count], set->page, 1);
            TexPageWait(&slots[set->page % count]);
        } else if (count == TEX_PAGE_BUFFERS_DOUBLE) {
            int next = set->page + 1;
            if (page_set == 0)
                TexPageLoad(scene, files, &slots[set->page % 2], set->page, 1);
            TexPageWait(&slots[set->page % 2]);
            if (next < files->count) {
                TexPageLoad(scene, files, &slots[next % count], next, 1);
                page_set = 1;
            }
        } else {
            int ahead = set->page + count / 2;
            TexPageWait(&slots[set->page % count]);
            if (ahead < files->count)
                TexPageLoad(scene, files, &slots[ahead % count], ahead, 1);
        }
    } else {
        if (set->page == 0)
            return;
        set->page--;
        if (set->slide != 0) {
            TexPageThaw(scene, files, &slots[set->page % count], set->page);
        } else if (count < 2) {
            TexPageLoad(scene, files, &slots[set->page % count], set->page, 1);
            TexPageWait(&slots[set->page % count]);
        } else if (count == TEX_PAGE_BUFFERS_DOUBLE) {
            int prev = set->page - 1;
            if (page_set != 0)
                TexPageLoad(scene, files, &slots[set->page % 2], set->page, 1);
            TexPageWait(&slots[set->page % 2]);
            if (prev >= 0) {
                TexPageLoad(scene, files, &slots[prev % count], prev, 1);
                page_set = 0;
            }
        } else {
            int behind = TexRingFirst(set->page, count);
            TexPageWait(&slots[set->page % count]);
            if (behind >= 0)
                TexPageLoad(scene, files, &slots[behind % count], behind, 1);
        }
    }
    TexFileSetPatch(scene, part, &slots[set->page % count]);
}

// CT3 0x01855b70: part's set jumps to page `page` (ignored out of range): the page loads and is waited
// for, then the double buffer preloads the next page or the ring the pages around it; slide sets unpack
// it. Then the part shows it.
void TexFileSetPage(int scene, int part, int page)
{
    // callcheck: FSLoad, FSLoadCancelQue, FSLoadEndWait, tnSlideThawing (the ROM inlines the page steps: TexPageLoad / Wait / Thaw make them)
    const TEX_FILE_SET_TABLE *tbl = BANK_PTR(const TEX_FILE_SET_TABLE *, scene, All_table_TBL[scene]->texFileSet);
    TEX_FILE_SET *set;
    const TEX_FILE_LIST *files;
    const TEX_PAGE_BUFFERS *bufs;
    TEX_PAGE_SLOT *slots;
    int count;

    if (tbl == NULL)
        return;
    set = TexFileSetFind(scene, tbl, part);
    if (set == NULL)
        return;
    files = BANK_PTR(const TEX_FILE_LIST *, scene, tbl->files) + set->files;
    bufs = BANK_PTR(const TEX_PAGE_BUFFERS *, scene, tbl->buffers) + set->buffers;
    slots = BANK_PTR(TEX_PAGE_SLOT *, scene, bufs->slots);
    count = bufs->count;
    if (page < 0 || page >= files->count)
        return;

    set->page = page;
    if (set->slide == 0) {
        TexPageLoad(scene, files, &slots[page % count], page, 1);
        TexPageWait(&slots[page % count]);
        if (count == TEX_PAGE_BUFFERS_DOUBLE) {
            if (page + 1 < files->count)
                TexPageLoad(scene, files, &slots[(page + 1) % 2], page + 1, 1);
            page_set = 1;
        } else if (count != 1) {
            int first = TexRingFirst(page, count), p;
            for (p = first; p < count + first; p++) {
                if (p != page && p >= 0 && p < files->count - 1)
                    TexPageLoad(scene, files, &slots[p % count], p, 1);
            }
        }
    } else {
        TexPageThaw(scene, files, &slots[page % count], page);
    }
    TexFileSetPatch(scene, part, &slots[page % count]);
}

// CT3 0x01855160: the trigger action (part, direction, scene).
void tnTrigTexFileSetNext(int part, int back, int scene)
{
    TexFileSetNext(scene, part, back);
}

// CT3 0x01855b10: the trigger action; literal == 0 takes the page from trigger variable (page & 0xffff).
void tnTrigTexFileSetPage(int part, int literal, int a2, int page, int scene)
{
    if (literal == 0)
        page = ssTrigGetVariable(page & 0xffff, a2, scene);
    TexFileSetPage(scene, part, page);
}

#define PANIC_SE_BANK "C_0000"          /* @3592: the common SE bank the panic loops play from */

// CT3 0x018532a0: the panic sounds stop (the three loops by name, in the SE bank) and the heartbeat
// restarts its count; EventSoundUpdate's next call is taken as following an "on" frame.
void EventSoundEnd(void)
{
    sPanicBeforeFlg = 1;
    sBeatCount = 0;
    sHandleKeyONFinFin = 0;
    SSStopSE(PANIC_SE_BANK, PANIC_SE_FINFIN, 1);
    sHandleKeyONLavender = 0;
    SSStopSE(PANIC_SE_BANK, PANIC_SE_LAVENDER, 1);
    sHandleKeyONInvisible = 0;
    SSStopSE(PANIC_SE_BANK, PANIC_SE_INVISIBLE, 1);
}

// ---- banks by name (slice 4) -----------------------------------------------------------------------
// CT3 0x0185c220: the first four bytes of two names are the same (the ROM compares one word each).
int ssStrCmp4Byte(const char *a, const char *b)
{
    u_int wa, wb;
    memcpy(&wa, a, sizeof(wa));
    memcpy(&wb, b, sizeof(wb));
    return wa == wb;
}

// CT3 0x0185c190: the bank whose file name starts with name's first four bytes, a group counted once at
// its first bank; -1 when none does.
int ssGetBankID(const char *name)
{
    char key[16];
    int i;

    strcpy(key, name);
    i = 0;
    do {
        if (ssStrCmp4Byte(BankInfo[i].name, key))
            return i;
        if (BankInfo[i].groupCount != 0)
            i += BankInfo[i].groupCount - 1;
        i++;
    } while (i < BANK_MAX);
    return -1;
}

// ---- the slide decoder (slice 4) ---------------------------------------------------------------------
static int OutputCount;                         /* sbss 0x0193ce64: bytes written to dst */
static int InputCount;                          /* sbss 0x0193ce68: the next byte of src */
static int SlideFileSize;                       /* sbss 0x0193ce6c: src's first word: where it ends */
static int outcount;                            /* sbss 0x0193ce70: cleared, not read */
static u_char text[SLIDE_RING_SIZE + SLIDE_MATCH_MAX - 1]; /* bss 0x0194d3c0: the ring */

// CT3 0x0185e6a0: unpack a slide file (LZSS after an 8-byte header) into dst.
void tnSlideThawing(const u_char *src, u_char *dst)
{
    int size;

    outcount = 0;
    InputCount = 0;
    OutputCount = 0;
    memcpy(&size, src, sizeof(size));
    SlideFileSize = size;
    InputCount += SLIDE_HEADER_SIZE;
    decode(src, dst);
}

// CT3 0x0185e6d0: Okumura's LZSS: a flag byte per eight items, 1 a literal, 0 a 12-bit ring position
// and a 4-bit length (+ SLIDE_THRESHOLD + 1); the ring starts blank, written from N - F.
void decode(const u_char *src, u_char *dst)
{
    int i, j, k, c;
    int r;
    u_int flags;

    for (i = 0; i < SLIDE_RING_SIZE - SLIDE_MATCH_MAX; i++)
        text[i] = 0;
    r = SLIDE_RING_SIZE - SLIDE_MATCH_MAX;
    flags = 0;
    for (;;) {
        flags >>= 1;
        if ((flags & 0x100) == 0) {
            c = tn_getc(src);
            if (c == -1)
                return;
            flags = (u_int)c | 0xff00;
        }
        if (flags & 1) {
            c = tn_getc(src);
            if (c == -1)
                return;
            tn_putc(dst, (u_char)c);
            text[r] = (u_char)c;
            r = (r + 1) & (SLIDE_RING_SIZE - 1);
        } else {
            i = tn_getc(src);
            if (i == -1)
                return;
            j = tn_getc(src);
            if (j == -1)
                return;
            i |= (j & 0xf0) << 4;
            j = (j & 0x0f) + SLIDE_THRESHOLD;
            for (k = 0; k <= j; k++) {
                c = text[(i + k) & (SLIDE_RING_SIZE - 1)];
                tn_putc(dst, (u_char)c);
                text[r] = (u_char)c;
                r = (r + 1) & (SLIDE_RING_SIZE - 1);
            }
        }
    }
}

// CT3 0x0185e8b0
void tn_putc(u_char *dst, u_char c)
{
    dst[OutputCount] = c;
    OutputCount++;
}

// CT3 0x0185e8d0: the next byte of src, or -1 at SlideFileSize.
int tn_getc(const u_char *src)
{
    int c = -1;
    if (InputCount != SlideFileSize) {
        c = src[InputCount];
        InputCount++;
    }
    return c;
}

// CT3 0x0185a650: the eye of matrix_global_view.
void CameraMatrix2CameraPos(float *pos)
{
    asViewMatrix2CamRUF(matrix_global_view, pos, NULL, NULL, NULL);
}

// CT3 0x0185a5e0: the point CAMERA_TARGET_DIST (300) ahead of matrix_global_view's eye; w the eye's.
void CameraMatrix2CameraTrg3m(float *trg)
{
    float pos[4], front[4];

    asViewMatrix2CamRUF(matrix_global_view, pos, NULL, NULL, front);
    trg[0] = pos[0] + front[0] * CAMERA_TARGET_DIST;
    trg[1] = pos[1] + front[1] * CAMERA_TARGET_DIST;
    trg[2] = pos[2] + front[2] * CAMERA_TARGET_DIST;
    trg[3] = pos[3];
}

// ---- banks and characters (slice 1, 2026-11-10) ----

// CT3 0x0185a400
// The first character whose shape bank, or one of whose ten behaviour banks, is `bank`; -1 none.
int BankID_TO_CharID(short bank)
{
    int i, j;
    for (i = 0; i < CHARACTER_MAX; i++) {
        if (Character[i].bank == bank)
            return i;
        for (j = 0; j < 10; j++)
            if (Character[i].behaviorBank[j] == bank)
                return i;
    }
    return -1;
}

// CT3 0x0185a480
// The bank (0..85) holding the character's room: a bank in use whose file name's first four
// characters hash to the character's roomHash (0 never matches); -1 none.
int CharID_TO_BankID(int chara)
{
    u_long room;
    int i;
    for (i = 0; i < BANK_MAX; i++) {
        room = Character[chara].roomHash;
        if (room == GetRoomName_file(BankInfo[i].name) && room != 0 && BankInfo[i].state != 0)
            return i;
    }
    return -1;
}

// CT3 0x019392f0 (RoomNameID_STR_Tbl) and 0x01939110 (RoomNameID_ID_Tbl, + -1): a room's area number.
static const char *const RoomNameID_STR_Tbl[AREA_TBL_COUNT] = {
    "0A01", "0A02", "0A03", "0A19", "0A01", "0A10", "0A11", "0A06", "0A07", "0A27",
    "1A02", "1A10", "1A11", "1C01", "1C02", "1C03", "1C01", "1C02", "1C04", "1C05",
    "1C06", "1A04", "1B01", "1B03", "1B04", "1B08", "1B11", "1B05", "1B07", "1B04",
    "1B08", "1B09", "1B11", "1B12", "1B08", "2A01", "2B01", "2B06", "2B02", "2B03",
    "2B04", "2B01", "2A02", "2B08", "2B07", "2B05", "2D01", "2D02", "2D03", "0B01",
    "0B19", "0B03", "0A16", "0B01", "0A10", "0A11", "0A07", "0A17", "0B06", "0B07",
    "0A23", "3A01", "3A02", "3C06", "3C05", "3C01", "3C02", "3C03", "3C07", "0A11",
    "0A10", "0A01", "0A01", "0A02", "3D03", "3D01", "3D02", "3E01", "3F01", "3D05",
    "4A08", "4A03", "4A02", "4A01", "4A04", "4A10", "4A09", "4A07", "4A04", "4A06",
    "4A12", "4A11", "4A05", "4A06", "4C01", "5A01", "4B05", "4B04", "4B06", "4B07",
    "4B08", "4N01", "4N02", "4B09", "4N03", "4B01", "4B10", "4B02", "4B03", "0A16",
    "0A30", "5A01", "5A03", "5A04", "5A02", "0A11", "0A10", "0A01",
};
static const int RoomNameID_ID_Tbl[AREA_TBL_COUNT + 1] = {
     1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  3,  4,  4,  5,  5,  5,  6,  6,  6,  6,
     6,  7,  8,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9, 10, 10, 11, 12, 12, 12, 12,
    13, 13, 14, 14, 14, 14, 15, 16, 16, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18,
    18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 26, 26, 27, 27, 28, 29, 29, 30, 31, 31,
    32, 33, 34, 34, 35, 36, 36, 36, 36, 37, 38, 38, 38, 38, 39, 40, 41, 42, 43, 43,
    43, 43, 43, 44, 44, 45, 46, 46, 47, 47, 47, 48, 49, 50, 51, 52, 52, 52, -1,
};

// CT3 0x01852240: the area (1..52) the character's room belongs to, -1 when it is in no listed room
// (or in no bank: lightBank -1). Stages 3 and 4 override a few rooms of the home.
int GetAreaNumber(int chara)
{
    char room[16];
    int pass;
    int i;

    if (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0) == 3 && Character[chara].lightBank != -1) {
        strcpy(room, asConvertHash2String(Character[chara].roomHash));
        if (memcmp(room, "0A11", ROOM_NAME_LEN) == 0 || memcmp(room, "0A10", ROOM_NAME_LEN) == 0
            || memcmp(room, "0A01", ROOM_NAME_LEN) == 0)
            return AREA_STAGE3_HOME;
    }
    if (ssTrigGetVariable(TRIG_VAR_STAGE, 1, 0) == 4 && Character[chara].lightBank != -1) {
        strcpy(room, asConvertHash2String(Character[chara].roomHash));
        if (memcmp(room, "0A16", ROOM_NAME_LEN) == 0)
            return AREA_STAGE4_0A16;
    }
    pass = 0;
    if (Character[chara].lightBank != -1) {
        strcpy(room, asConvertHash2String(Character[chara].roomHash));
        for (i = 0; RoomNameID_ID_Tbl[i] != -1; i++) {
            if (memcmp(room, RoomNameID_STR_Tbl[i], ROOM_NAME_LEN) != 0)
                continue;
            // the ROM's test order: PASS2 skips a row unless one was skipped already; PASS3 then
            // skips one more unless two were
            if (ssTrigCheckEvent(TRIG_EVENT_AREA_PASS2, 1, 0) != 0 && pass != 1) {
                pass++;
                continue;
            }
            if (ssTrigCheckEvent(TRIG_EVENT_AREA_PASS3, 1, 0) != 0 && pass != 2) {
                pass++;
                continue;
            }
            return RoomNameID_ID_Tbl[i];
        }
    }
    return -1;
}

// CT3 0x0185c2e0: ahGetHash of the name's first four characters (a room's files share them).
u_long GetRoomName_file(const char *name)
{
    char room[ROOM_NAME_LEN + 1];
    memcpy(room, name, ROOM_NAME_LEN);
    room[ROOM_NAME_LEN] = '\0';
    return ahGetHash(room);
}

// CT3 0x0185e100: the part's record (header + 0x40 + i * 0x380), NULL when the model has none.
u_char *asFindModelParts(u_char *header, u_long hash)
{
    int id = asFindModelPartsID(header, hash);
    if (id < 0)
        return NULL;
    return header + PARTS_HDR_RECORDS + id * PARTS_REC_SIZE;
}

// CT3 0x01854b20: joint bank's part hash to srcBank's part of the same name (its world matrix and
// position, MODEL_PARTS +0x1c0 / +0x130); JOINT_NOT_FOUND_1 when srcBank has none.
int asJointParts(int srcBank, int bank, u_long hash)
{
    u_char *rec = asFindModelParts(pModelPartsHeader[srcBank], hash);
    MODEL_PARTS *src;
    if (rec == NULL)
        return JOINT_NOT_FOUND_1;
    src = (MODEL_PARTS *)(rec - PARTS_HDR_RECORDS);
    return asJointParts_byMatrix(src->matrix, src->pos, bank, hash);
}

static void SqueezeCross(float *out, const float *u, const float *v);

#define JOINT_ALIGNED_EPS 0.0001f       /* |cos| below it: the second axis is a quarter turn off */
#define JOINT_TWIST_COS 0.999f          /* asJointParts_byMatrix's check of its second turn */

static void JointIdentity(float (*m)[4])
{
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            m[i][j] = i == j ? 1.0f : 0.0f;
}

// CT3 0x01854750
// Joins `bank` to a portal frame: the bank's part `hash` (its joint) is turned so that its row 0 lies
// along mat's row 0 (about row0 x mat0, or a half turn about its row 2 when opposite), then about that
// row 0 so its row 1 meets mat's (a quarter turn when they are perpendicular, else back by the angle,
// then by the angle still left when that missed). The two turns become the bank's base rotation (part
// 0's PARTS_ANIM_BASE_ROT rows), and drawOnPos puts the joint's turned position on `pos`. The joint's
// world matrix keeps the first turn. JOINT_NOT_FOUND_2 when the bank has no such part, else 0.
int asJointParts_byMatrix(float (*mat)[4], float *pos, int bank, u_long hash)
{
    alignas(16) float r1[4][4], r2[4][4], t[4][4];
    float axis[4], p[4], d;
    u_char *hdr = pModelPartsHeader[bank];
    u_char *rec = asFindModelParts(hdr, hash);
    MODEL_PARTS *joint, *base;
    float (*m)[4];
    int i;

    JointIdentity(r1);
    JointIdentity(r2);
    JointIdentity(t);
    if (rec == NULL)
        return JOINT_NOT_FOUND_2;
    joint = (MODEL_PARTS *)(rec - PARTS_HDR_RECORDS);
    base = (MODEL_PARTS *)hdr;
    m = joint->matrix;

    InitMatrixTree(hdr);
    LinkMatrixTree(BANK_PTR(u_int *, bank, All_table_TBL[bank]->objOpaque), hdr);
    BuildMatrixTree(bank, 1, 0);

    d = gaInnerProductXYZ(mat[0], m[0]);
    if (d != 1.0f) {
        if (d == -1.0f) {
            asMakeRotMatrixByQuartanion(r1, m[2], ANGLE_HALF_TURN);
        } else {
            SqueezeCross(axis, mat[0], m[0]);
            asMakeRotMatrixByQuartanion(r1, axis, ahAcos(d));
        }
    }
    gaMulMatrix(m, r1, m);

    d = gaInnerProductXYZ(mat[1], m[1]);
    if (d != 1.0f) {
        if (d == -1.0f) {
            asMakeRotMatrixByQuartanion(r2, m[0], ANGLE_HALF_TURN);
        } else if (d <= JOINT_ALIGNED_EPS && !(d < -JOINT_ALIGNED_EPS)) {
            asMakeRotMatrixByQuartanion(r2, m[0], ANGLE_QUARTER_TURN);
            gaMulMatrix(t, r2, m);
            if (gaInnerProductXYZ(mat[1], t[1]) <= JOINT_TWIST_COS)
                asMakeRotMatrixByQuartanion(r2, m[0], ANGLE_HALF_TURN + ANGLE_QUARTER_TURN);
        } else {
            asMakeRotMatrixByQuartanion(r2, m[0], -ahAcos(d));
            gaMulMatrix(t, r2, m);
            d = gaInnerProductXYZ(mat[1], t[1]);
            // the ROM turns by the cosine it just measured (not back by the first angle)
            if (d <= JOINT_TWIST_COS)
                asMakeRotMatrixByQuartanion(r2, m[0], ahAcos(d));
        }
    }
    gaMulMatrix(r1, r2, r1);

    memcpy((u_char *)base + PARTS_ANIM_BASE_ROT, r1, sizeof(r1));
    // vmulax..vmaddw: the joint's position through the turns (row vector), negated, plus pos
    for (i = 0; i < 4; i++)
        p[i] = -(joint->pos[0] * r1[0][i] + joint->pos[1] * r1[1][i] + joint->pos[2] * r1[2][i] +
                 joint->pos[3] * r1[3][i]) + pos[i];
    p[3] = 1.0f;
    memcpy(base->drawOnPos, p, sizeof(p));
    return 0;
}

// CT3 0x01859f00
// Where the line through a and b crosses the plane through planePoint with the normal n:
// out = a + (a - b) * -(n . (a - planePoint)) / (n . (a - b)), xyz; out's w is a's.
void asGetCrossPointInPlaneWithLine(float out[4], float a[4], float b[4], float n[4], float planePoint[4])
{
    alignas(16) float d[4], e[4];
    float t;
    int i;
    for (i = 0; i < 3; i++) {
        d[i] = a[i] - b[i];
        e[i] = a[i] - planePoint[i];
    }
    d[3] = e[3] = a[3];
    t = -gaInnerProductXYZ(n, e) / gaInnerProductXYZ(n, d);
    for (i = 0; i < 3; i++)
        out[i] = d[i] * t + a[i];
    out[3] = a[3];
}

// CT3 0x018520b0
// A point light's range change in progress (REC_OMNI_CHANGING): the inverse range (REC_OMNI_INV_RANGE)
// moves 1 / steps of the way to 1 / REC_OMNI_RANGE each call; the last step lands on it and ends the
// change.
void OmniRangeChange(u_char *rec)
{
    int *changing = (int *)(rec + REC_OMNI_CHANGING);
    int *steps = (int *)(rec + REC_OMNI_STEPS);
    float range = *(float *)(rec + REC_OMNI_RANGE);
    float *inv = (float *)(rec + REC_OMNI_INV_RANGE);

    if (*changing == 0)
        return;
    if (*steps == 0) {
        *changing = 0;
        *inv = 1.0f / range;
    } else {
        *inv = *inv + (1.0f / range - *inv) / (float)*steps;
        (*steps)--;
    }
}

// ---- the rooms' trigger events (slice 1, 2026-11-10) ----

// CT3 0x01852130: every room's event removed.
void RoomFlagClear(void)
{
    int i;
    for (i = 0; RoomIdTbl[i] != -1; i++)
        ssTrigRemoveEvent(RoomIdTbl[i], 1, 0);
}

// CT3 0x01851f00: a jump to RoomFlagClear.
void BRANCH_CLEAR(void)
{
    RoomFlagClear();
}

// CT3 0x01852180: the event of the room the player character (Character[0]) is in, registered: its
// roomHash read back as a name, matched on four characters.
void RoomInFlagOn(void)
{
    char room[16];
    int i;

    strcpy(room, asConvertHash2String(Character[0].roomHash));
    for (i = 0; RoomIdTbl[i] != -1; i++) {
        if (memcmp(room, RoomNameTbl[i], ROOM_NAME_LEN) == 0) {
            ssTrigRegisterEvent(RoomIdTbl[i], 1, 0);
            return;
        }
    }
}

// CT3 0x0185e040
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 4): BuildMatrixTree indexes by it.
int absi(int v)
{
    return v < 0 ? -v : v;
}

// CT3 0x0185e390
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 5): Execute_ExecuteAnimationHistory calls it.
void AbsSignI32(int v, int *abs, int *sign)
{
    if (v < 0) {
        *abs = -v;
        *sign = -1;
    } else if (v > 0) {
        *abs = v;
        *sign = 1;
    } else {
        *abs = 0;
        *sign = 0;
    }
}

// CT3 0x0185d0d0
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 8): EA_100 calls it. sq zero, then
// each short into the high half of its float, 0x3f80 into w's.
void asZipVECTOR2VECTOR_XYZ1(float *out, const short *in)
{
    int i;
    for (i = 0; i < 3; i++) {
        u_int bits = (u_int)(u_short)in[i] << 16;
        memcpy(&out[i], &bits, sizeof(bits));
    }
    out[3] = 1.0f;
}

// ---- written early: the packed keys InterpolateAnimationKey reads (ahRelocateSunPs2ObjectArry.c
// slice 9, 2026-11-10; from the disassembly) ----

// vftoi0: truncate toward zero, saturating at the int range (the VU's clamp).
static int VuFtoi0(float f)
{
    if (f >= 2147483647.0f)
        return 0x7fffffff;
    if (f <= -2147483648.0f)
        return (int)0x80000000;
    return (int)f;
}

// CT3 0x0185c4d0
// CT3 0x0185c4e8 (asMemCopy128c_body, game-inferred: its loop head, split by Ghidra).
// n qwords (lq / sq), counted down from n.
void asMemCopy128c(void *dst, const void *src, u_int n)
{
    memmove(dst, src, (size_t)n * 16);
}

// CT3 0x0185c500
// n halfwords (lhu / sh).
void asMemCopy16c(void *dst, const void *src, u_int n)
{
    memmove(dst, src, (size_t)n * 2);
}

// CT3 0x0185c770
// q * ZIP_QUAT_SCALE (vmulx), vftoi0, each word's low half.
void asQuat2ZipQuat(short *out, const float *q)
{
    int i;
    for (i = 0; i < 4; i++)
        out[i] = (short)VuFtoi0(q[i] * ZIP_QUAT_SCALE);
}

// CT3 0x0185c700
// The packed quaternions unpacked, slerped by w ({1 - t, t}) and packed into out.
void asQuatSlerpZipVECTOR(short *out, const short *a, const short *b, float *w)
{
    alignas(16) float q[4], qa[4], qb[4];
    asZipQuat2Quat(qa, a);
    asZipQuat2Quat(qb, b);
    QuatSlerp(q, qa, qb, w);
    asQuat2ZipQuat(out, q);
}

// CT3 0x0185ce40
// v * (1 << shift) (vmulx), vftoi0, the low halves of xyz.
void asTrans2ZipTrans_XYZ(short *out, const float *v, u_int shift)
{
    float scale = (float)(1 << shift);
    int i;
    for (i = 0; i < 3; i++)
        out[i] = (short)VuFtoi0(v[i] * scale);
}

// CT3 0x0185cdb0
// The packed positions unpacked (XYZ1), blended by w (asInterpolateVector) and packed into out.
void asInterpolateZipTrans_XYZ(short *out, const short *a, const short *b, const float *w, u_int shift)
{
    alignas(16) float v[4], va[4], vb[4];
    asZipTrans2Trans_XYZ1(va, a, shift);
    asZipTrans2Trans_XYZ1(vb, b, shift);
    asInterpolateVector(v, va, vb, w);
    asTrans2ZipTrans_XYZ(out, v, shift);
}

// CT3 0x0185d100
// asZipVECTOR2VECTOR_XYZ1 with w 0 (sq zero, nothing stored over it).
void asZipVECTOR2VECTOR_XYZ0(float *out, const short *in)
{
    int i;
    for (i = 0; i < 3; i++) {
        u_int bits = (u_int)(u_short)in[i] << 16;
        memcpy(&out[i], &bits, sizeof(bits));
    }
    out[3] = 0.0f;
}

// CT3 0x0185d190
// The high halves of xyz (bfloat16).
void asVECTOR2ZipVECTOR_XYZ(short *out, const float *v)
{
    int i;
    for (i = 0; i < 3; i++) {
        u_int bits;
        memcpy(&bits, &v[i], sizeof(bits));
        out[i] = (short)(bits >> 16);
    }
}

// CT3 0x0185d120
// The packed vectors unpacked (XYZ0), blended by w and packed into out.
void asInterpolateZipVECTOR_XYZ(short *out, const short *a, const short *b, const float *w)
{
    alignas(16) float v[4], va[4], vb[4];
    asZipVECTOR2VECTOR_XYZ0(va, a);
    asZipVECTOR2VECTOR_XYZ0(vb, b);
    asInterpolateVector(v, va, vb, w);
    asVECTOR2ZipVECTOR_XYZ(out, v);
}

// CT3 0x0185d060
// u, v: the first two shorts / ZIP_UV_SCALE (div.s); the next two shorts as they are.
void asZipUV2UV(float *out, const short *in)
{
    out[0] = (float)in[0] / ZIP_UV_SCALE;
    out[1] = (float)in[1] / ZIP_UV_SCALE;
    out[2] = (float)in[2];
    out[3] = (float)in[3];
}

// CT3 0x0185d1b0
// a w[0] + b w[1]; a when a == b (c.eq.s).
float asInterpolateNumber(float a, float b, const float *w)
{
    if (a == b)
        return a;
    return a * w[0] + b * w[1];
}

// CT3 0x0185d330
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 8): EA_100 calls it. vsub into vf0 for
// the MAC zero flags: all four set leaves a; else vmulax / vmaddy.
void asInterpolateVector(float *out, const float *a, const float *b, const float *w)
{
    int i;
    float r[4];
    if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]) {
        memmove(out, a, sizeof(r));
        return;
    }
    for (i = 0; i < 4; i++)
        r[i] = a[i] * w[0] + b[i] * w[1];
    memcpy(out, r, sizeof(r));
}

// asMakeWatchMeMatrixToPosition's axis (the part record's REC_WATCH_AXIS byte): which of the part's axes
// is turned onto the target. Named from the turns each takes (X then Z, Z then X, Z, X); another value
// makes no last turn.
#define WATCH_AXIS_X 0                  /* one turn about X */
#define WATCH_AXIS_Z 1                  /* one turn about Z */
#define WATCH_AXIS_XZ 2                 /* about X, then Z */
#define WATCH_AXIS_ZX 3                 /* about Z, then X */

// mul.S by 65536, div.S by 2pi, cvt.w.S, andi 0xffff: radians to the 0x10000-a-turn angle.
static int WatchAngle(float rad)
{
    return (int)(rad * 65536.0f / 6.2831855f) & 0xffff;
}

// vmulax / vmadday / vmaddaz / vmaddw .xyzw: v = v * m (row vector).
static void WatchApply(float *v, float (*m)[4])
{
    float r[4];
    int i;
    for (i = 0; i < 4; i++)
        r[i] = v[0] * m[0][i] + v[1] * m[1][i] + v[2] * m[2][i] + v[3] * m[3][i];
    memcpy(v, r, sizeof(r));
}

// CT3 0x0185d4d0
// A part turned to watch `target`: m's frame is levelled step by step (about Y by its row 1's angle,
// about X by row 1's, about Y by row 0's less a quarter turn), the direction to the target (target -
// m's row 3) carried along each step; `axis` (WATCH_AXIS_*) then turns the part about X and / or Z
// towards that direction; the levelling is undone and m applied last.
void asMakeWatchMeMatrixToPosition(float (*out)[4], float (*m)[4], float *target, int axis)
{
    alignas(16) float l[4][4], ry[4][4], rx[4][4], ry2[4][4], r1[4][4], r2[4][4];
    float d[4];

    memcpy(l, m, sizeof(l));
    l[3][0] = l[3][1] = l[3][2] = 0.0f;
    // vsub.xyz: the ROM leaves d's w as the stack had it; every matrix's row 3 here is (0, 0, 0, 1)
    // and rows 0..2 have w 0, so it never reaches xyz.
    d[0] = target[0] - m[3][0];
    d[1] = target[1] - m[3][1];
    d[2] = target[2] - m[3][2];
    d[3] = 0.0f;

    gaSetRotMatrixY(ry, WatchAngle(asRadianByO(l[1], RADIAN_AXIS_Y)));
    gaMulMatrix(l, ry, l);
    WatchApply(d, ry);
    memcpy(out, ry, sizeof(ry));
    gaInversMatrix(ry, ry);

    gaSetRotMatrixX(rx, -WatchAngle(asRadianByO(l[1], RADIAN_AXIS_X)) & 0xffff);
    gaMulMatrix(l, rx, l);
    WatchApply(d, rx);
    gaMulMatrix(out, rx, out);
    gaInversMatrix(rx, rx);

    gaSetRotMatrixY(ry2, -(ANGLE_QUARTER_TURN - WatchAngle(asRadianByO(l[0], RADIAN_AXIS_Y))) & 0xffff);
    gaMulMatrix(l, ry2, l);
    WatchApply(d, ry2);
    gaMulMatrix(out, ry2, out);
    gaInversMatrix(ry2, ry2);

    switch (axis) {
    case WATCH_AXIS_ZX:
        gaSetRotMatrixZ(r1, (ANGLE_QUARTER_TURN - WatchAngle(asRadianByO(d, RADIAN_AXIS_Z))) & 0xffff);
        WatchApply(d, r1);
        gaInversMatrix(r1, r1);
        gaSetRotMatrixX(r2, WatchAngle(asRadianByO(d, RADIAN_AXIS_X)));
        gaMulMatrix(out, out, r1);
        gaMulMatrix(out, out, r2);
        break;
    case WATCH_AXIS_XZ:
        gaSetRotMatrixX(r1, -WatchAngle(asRadianByO(d, RADIAN_AXIS_X)) & 0xffff);
        WatchApply(d, r1);
        gaInversMatrix(r1, r1);
        gaSetRotMatrixZ(r2, -(ANGLE_QUARTER_TURN - WatchAngle(asRadianByO(d, RADIAN_AXIS_Z))) & 0xffff);
        gaMulMatrix(out, out, r1);
        gaMulMatrix(out, out, r2);
        break;
    case WATCH_AXIS_Z:
        // the ROM's a1 is the 2 its mode-2 test left (the decompiler drops it)
        gaSetRotMatrixZ(r1, -(ANGLE_QUARTER_TURN - WatchAngle(asRadianByO(d, RADIAN_AXIS_Z))) & 0xffff);
        gaMulMatrix(out, out, r1);
        break;
    case WATCH_AXIS_X:
        gaSetRotMatrixX(r1, WatchAngle(asRadianByO(d, RADIAN_AXIS_X)));
        gaMulMatrix(out, out, r1);
        break;
    }
    gaMulMatrix(out, ry2, out);
    gaMulMatrix(out, rx, out);
    gaMulMatrix(out, ry, out);
    gaMulMatrix(out, m, out);
}

// CT3 0x0185ddf0: the rotation of angle (0x10000 a turn) about axis, through the unit quaternion.
void asMakeRotMatrixByQuartanion(float (*m)[4], float *axis, int angle)
{
    float q[4];
    float s, c;
    float inv;

    // VU0: xyz normalised by RSQRT (w keeps axis[3] until the cosine replaces it)
    inv = 1.0f / sqrtf(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    q[0] = axis[0] * inv;
    q[1] = axis[1] * inv;
    q[2] = axis[2] * inv;
    gaSinCos(&s, &c, (int)((u_int)angle >> 1));
    q[0] *= s;
    q[1] *= s;
    q[2] *= s;
    q[3] = c;
    ConvertQuat2Matrix(m, q);
}

// VU0 OPMULA / OPMSUB .xyz: out = u x v (w left as out's: the ROM's vf1 keeps u's w).
static void SqueezeCross(float *out, const float *u, const float *v)
{
    float x = u[1] * v[2] - u[2] * v[1];
    float y = u[2] * v[0] - u[0] * v[2];
    float z = u[0] * v[1] - u[1] * v[0];
    out[0] = x;
    out[1] = y;
    out[2] = z;
    out[3] = u[3];
}

// vmul / vaddy / vaddz, vrsqrt, vmulq.xyz: xyz by 1 / |xyz| (the VU's RSQRT of 0 is the largest float).
static void SqueezeNormalize(float *v)
{
    float len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    float q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;
    v[0] *= q;
    v[1] *= q;
    v[2] *= q;
}

// CT3 0x0185d380
// The twist between two frames. a's row 1 is turned onto b's (about b[1] x a[1], by the angle between
// them) into out = R * a with b's translation; the answer is the signed angle (ahAcos units) from out's
// row 0 to b's normalised row 0, negative when that turn leans away from out's row 1.
int asDelSqueezeInformation(float (*out)[4], float (*a)[4], float (*b)[4])
{
    float rot[4][4];
    float axis[4], b0[4];
    int angle, i;

    SqueezeCross(axis, b[1], a[1]);
    SqueezeNormalize(axis);
    angle = ahAcos(gaInnerProductXYZ(a[1], b[1]));
    asMakeRotMatrixByQuartanion(rot, axis, angle);
    gaMulMatrix(out, rot, a);
    for (i = 0; i < 4; i++)
        out[3][i] = b[3][i];

    for (i = 0; i < 4; i++)
        b0[i] = b[0][i];
    SqueezeNormalize(b0);
    SqueezeCross(axis, out[0], b0);
    SqueezeNormalize(axis);
    angle = ahAcos(gaInnerProductXYZ(out[0], b0));
    if (gaInnerProductXYZ(axis, out[1]) < 0.0f)
        angle = -angle;
    return angle;
}

// CT3 0x0185c7d0
// Written early (2026-11-10, ahRelocateSunPs2ObjectArry.c slice 9; the stub carried 0x0185c6d0). The
// four shorts as floats, then vmulx by 1 / ZIP_QUAT_SCALE (0x38800000).
void asZipQuat2Quat(float *out, const short *in)
{
    int i;
    for (i = 0; i < 4; i++)
        out[i] = (float)in[i] * (1.0f / ZIP_QUAT_SCALE);
}

// CT3 0x0185ceb0
// Written early (slice 9; the stub carried 0x0185cdb0): xyz = the shorts * 1 / (1 << shift) (div.s,
// then vmulx), w = 1.
void asZipTrans2Trans_XYZ1(float *out, const short *in, u_int shift)
{
    float scale = 1.0f / (float)(1 << shift);
    int i;
    for (i = 0; i < 3; i++)
        out[i] = (float)in[i] * scale;
    out[3] = 1.0f;
}


// CT3 0x0185c840: packed normals (3 shorts, ZIP_NORMAL_SHIFT) blended, renormalised (VU0 RSQRT, all
// four lanes scaled) and packed again.
void asContInterpolateZipNormal_XYZ(short *out, const short *a, const short *b, const float *w, int n)
{
    alignas(16) float va[4], vb[4], v[4];
    for (; n != 0; n--, out += 3, a += 3, b += 3) {
        float len;
        asZipTrans2Trans_XYZ0(va, a, ZIP_NORMAL_SHIFT);
        asZipTrans2Trans_XYZ0(vb, b, ZIP_NORMAL_SHIFT);
        asInterpolateVector(v, va, vb, w);
        len = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
        len = len > 0.0f ? 1.0f / sqrtf(len) : FLT_MAX;    /* VU RSQRT of 0: the largest float */
        v[0] *= len;
        v[1] *= len;
        v[2] *= len;
        v[3] *= len;
        asTrans2ZipTrans_XYZ(out, v, ZIP_NORMAL_SHIFT);
    }
}

// CT3 0x0185ca50: packed positions (3 shorts, fixed point by shift) blended and packed again.
void asContInterpolateZipTrans_XYZ(short *out, const short *a, const short *b, const float *w, int n,
                                   u_int shift)
{
    alignas(16) float va[4], vb[4], v[4];
    for (; n != 0; n--, out += 3, a += 3, b += 3) {
        asZipTrans2Trans_XYZ1(va, a, shift);
        asZipTrans2Trans_XYZ1(vb, b, shift);
        asInterpolateVector(v, va, vb, w);
        asTrans2ZipTrans_XYZ(out, v, shift);
    }
}

// CT3 0x0185c5a0: a float to the 7.9 form (mul.s by 512, cvt.w.s truncates, the low halfword kept).
void asFloat2ZipFloat0_7_9X(short *out, const float *in)
{
    *out = (short)(int)(512.0f * *in);
}

// CT3 0x0185c530: both unpacked (unsigned), blended by asInterpolateNumber, packed again.
void asInterpolateZipFloat0_7_9X(short *out, const short *a, const short *b, const float *w)
{
    float fa, fb, f;
    asZipFloat0_7_9X2Float(&fa, (const u_short *)a);
    asZipFloat0_7_9X2Float(&fb, (const u_short *)b);
    f = asInterpolateNumber(fa, fb, w);
    asFloat2ZipFloat0_7_9X(out, &f);
}

// CT3 0x0185c6d0: a 1.1.14 fixed-point short to a float (div.s by 16384).
void asZipFloat1_1_14X2Float(float *out, const short *in)
{
    *out = (float)*in / ZIP_FLOAT_1_1_14_SCALE;
}

// CT3 0x0185c6a0: and back (mul.s, cvt.w.s truncates).
void asFloat2ZipFloat1_1_14X(short *out, const float *in)
{
    *out = (short)(int)(ZIP_FLOAT_1_1_14_SCALE * *in);
}

// CT3 0x0185c630: both unpacked, blended by asInterpolateNumber, packed again.
void asInterpolateZipFloat1_1_14X(short *out, const short *a, const short *b, const float *w)
{
    float fa, fb, f;
    asZipFloat1_1_14X2Float(&fa, a);
    asZipFloat1_1_14X2Float(&fb, b);
    f = asInterpolateNumber(fa, fb, w);
    asFloat2ZipFloat1_1_14X(out, &f);
}

// CT3 0x0185d020
// Not a packer: the first two floats (u, v) are copied into out's 8 bytes as they are (the ROM moves
// them as four halfwords), so what asInterpolateZipUV writes is a float pair.
void asUV2ZipUV_R(short *out, const float *uv)
{
    memcpy(out, uv, 2 * sizeof(float));
}

// CT3 0x0185cfb0: both packed UVs unpacked (asZipUV2UV), blended (asInterpolateVector), and the float
// u, v stored into out (asUV2ZipUV_R).
void asInterpolateZipUV(short *out, const short *a, const short *b, const float *w)
{
    float ua[4], ub[4], u[4];
    asZipUV2UV(ua, a);
    asZipUV2UV(ub, b);
    asInterpolateVector(u, ua, ub, w);
    asUV2ZipUV_R(out, u);
}

// CT3 0x0185dc30
// The angle 0..2pi of the vector in the plane about one axis (RADIAN_AXIS_*): (x, y) = (y, z) for X,
// (z, x) for Y, (x, y) for Z, measured from the first towards the second. The quadrants are picked by
// sign bits (a -0 second component counts as negative), and atanf runs on a positive ratio. Another
// axis reads two uninitialised stack words in the ROM; the port reads 0, 0 (answer 0).
float asRadianByO(float *v, int axis)
{
    float x = 0.0f, y = 0.0f;
    if (axis == RADIAN_AXIS_Z) {
        x = v[0];
        y = v[1];
    } else if (axis == RADIAN_AXIS_Y) {
        x = v[2];
        y = v[0];
    } else if (axis == RADIAN_AXIS_X) {
        x = v[1];
        y = v[2];
    }
    if (x == 0.0f) {
        if (signbit(y))
            return 4.712389f;                           /* 3pi/2 */
        return y == 0.0f ? 0.0f : 1.5707964f;           /* pi/2 */
    }
    if (!signbit(x)) {
        if (!signbit(y))
            return atanf(y / x);
        return 6.2831855f - atanf(-y / x);
    }
    if (!signbit(y))
        return 3.1415927f - atanf(y / -x);
    return atanf(y / x) + 3.1415927f;
}


// CT3 0x0185dec0
// The *squared* length: a tail jump to gaInnerProductXYZ(v, v).
float asVectorLength(const float *v)
{
    return gaInnerProductXYZ((float *)v, (float *)v);
}

// CT3 0x0185ded0
// The *squared* distance: vsub.xyz, then gaInnerProductXYZ of the difference with itself (no root).
float asVectorDistance(const float *a, const float *b)
{
    float d[4];
    d[0] = a[0] - b[0];
    d[1] = a[1] - b[1];
    d[2] = a[2] - b[2];
    d[3] = a[3];
    return gaInnerProductXYZ(d, d);
}


// ---- slice 8 (2026-11-10): panic setters, the movie break, bank lookup, part and stick helpers ------

// CT3 0x01854110: add rise to this frame's panic source; sources 4 and 5 always count, the others only
// while PanicCheck answers 0. Nothing rises in full panic. Source PANIC_RISE_GAUGE (asXcmd_PanicSet's
// "PanicValuePriority") is the ROM's rise[8]: the word after the array, the gauge itself.
void PanicSet(float rise, int source)
{
    if ((source == PANIC_SET_ALWAYS_LO || source == PANIC_SET_ALWAYS_HI || PanicCheck() == 0)
        && (PanicInfoData.mode & PANIC_MODE_FULL) == 0) {
        if (source == PANIC_RISE_GAUGE)
            PanicInfoData.gauge += rise;
        else
            PanicInfoData.rise[source] += rise;
    }
}

// CT3 0x019381a0 (PanicOffMovieFile, sdata, NULL-ended): the movies that end a panic.
static const char *const PanicOffMovieFile[] = {
    "S16_1.MOV", "S29_1.MOV", "S33.MOV", "K12.MOV", "K06.MOV", NULL,
};

// CT3 0x01854190: a movie in PanicOffMovieFile ends full panic and empties the gauge.
void PanicStopMovie(const char *file)
{
    char name[128];
    int found = 0;
    int i;

    strcpy(name, file);
    UpperCase(name);
    for (i = 0; PanicOffMovieFile[i] != NULL; i++) {
        if (strcmp(name, PanicOffMovieFile[i]) == 0) {
            found = 1;
            break;
        }
    }
    if (!found)
        return;
    if (PanicInfoData.mode & PANIC_MODE_FULL) {
        PanicInfoData.filterAlpha = -1;
        PanicInfoData.mode &= ~PANIC_MODE_FULL;
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_FULL, 1, 0);
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
    }
    PanicInfoData.gauge = 0.0f;
}

// CT3 0x01854500: spend a barrier charm: 0 when one was held (TRIG_EVENT_KEKKAI_USED registered; the
// last one clears KekkaiOn), 1 when none is.
int Kekkai_Check(void)
{
    if (ItemData[ITEM_KEKKAI][1] == 0)
        return 1;
    ItemData[ITEM_KEKKAI][1]--;
    ssTrigRegisterEvent(TRIG_EVENT_KEKKAI_USED, 1, 0);
    if (ItemData[ITEM_KEKKAI][1] == 0)
        KekkaiOn = 0;
    return 0;
}

// CT3 0x01854b90: 1 when pos (xyz, w = 1) seen through m lies between the near and far clip and inside
// the screen widened by margin on each side. screen_0: [2] width, [3] height, [4] / [5] the centre,
// [6] / [7] the aspect, [8] the projection distance, [9] / [10] near and far.
// PORT: the screen is the PS2's 512 x 448 frame: a wide display shows more than this answers for
// (open-items.md (fp)).
int ssPointCheckWithinScreen(float margin, const float *pos, float (*m)[4])
{
    float v[3];
    float half;
    float s;
    int i;

    for (i = 0; i < 3; i++)
        v[i] = m[0][i] * pos[0] + m[1][i] * pos[1] + m[2][i] * pos[2] + m[3][i];
    if (v[2] < screen_0[9] || v[2] > screen_0[10])
        return 0;
    s = screen_0[4] + screen_0[8] * v[0] / v[2];
    half = screen_0[2] / screen_0[6] * 0.5f;
    if (s < screen_0[4] - half - margin || s > screen_0[4] + half + margin)
        return 0;
    half = screen_0[3] / screen_0[7] * 0.5f;
    s = screen_0[5] + screen_0[8] * v[1] / v[2];
    if (s < screen_0[5] - half - margin || s > screen_0[5] + half + margin)
        return 0;
    return 1;
}

// CT3 0x01855110: resume == 0 reserves a pause (and reviews the player's place), else lifts it.
int PauseUpdate(int resume)
{
    if (resume == 0) {
        pause_reservation |= PAUSE_RESERVE;
        asCharacterPositionReviewInPausing(&Character[0]);
    } else {
        pause_reservation &= ~PAUSE_RESERVE;
    }
    return 1;
}

// CT3 0x0185c290: a..z to A..Z in place.
void UpperCase(char *s)
{
    int i;
    for (i = 0; s[i] != '\0'; i++)
        if (s[i] > '`' && s[i] < '{')
            s[i] -= 0x20;
}

// CT3 0x01859a50: the loaded bank whose file name's first four characters are name's (case blind), a
// group counted once at its head; -1 when none is.
int GetBankID(const char *name)
{
    char key[5];
    int i;

    memcpy(key, name, 4);
    key[4] = 0;
    UpperCase(key);
    for (i = 0; i < BANK_MAX; i++) {
        if (BankInfo[i].state != BANK_STATE_FREE) {
            if (memcmp(BankInfo[i].name, key, 4) == 0)
                return i;
            i += BankInfo[i].groupCount - 1;
        }
    }
    return -1;
}

static int save_ga_main_timer;                  /* sbss 0x0193ce54 */
static int save_ga_down_timer;                  /* sbss 0x0193ce50 */

// CT3 0x01859b00: play the movie the trigger set up: the timers kept across it, the invisible and
// advantage timers restarted, full panic ended, the GS and the BGT reset afterwards, the next frames
// skipped.
int PlayMovie(void)
{
    save_ga_main_timer = ga_main_timer;
    save_ga_down_timer = ga_down_timer;
    ExecuteAnimationHistory_Free();
    if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, 0) != 0) {
        ssTrigRemoveEvent(TRIG_EVENT_INVISIBLE, 1, 0);
        TransparentBodyTime = PANIC_TRANSPARENT_TIME;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_MC_LOADED, 1, 0) != 0) {
        ssTrigRemoveEvent(TRIG_EVENT_MC_LOADED, 1, 0);
        AdvantageousTimeTime = PANIC_ADVANTAGEOUS_TIME;
    }
    if (PanicInfoData.mode & PANIC_MODE_FULL) {
        PanicInfoData.mode &= ~PANIC_MODE_FULL;
        PanicInfoData.filterAlpha = -1;
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_FULL, 1, 0);
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
    }
    printf(" ** Movie Play Start!! ** \n");         /* @5147 0x01933870 */
    AA_StartMovie();
    printf(" ** Movie Play End!! ** \n");           /* @5148 0x01933890 */
    asCharacterOperate(&Character[0], 1, 1, 1);
    asCharacterPositioning(&Character[0], 1, 1);
    SystemReReset();
    ga_main_timer = save_ga_main_timer;
    ga_down_timer = save_ga_down_timer;
    SkipMain = PLAYMOVIE_SKIP_FRAMES;
    if (DemoSkip != 0) {
        DEMO_SKIP_FLAG = PLAYMOVIE_DEMO_SKIP_FLAG;
        ssTrigRemoveEvent(TRIG_EVENT_DEMO_SKIP_9E, 1, 0);
    }
    ssTrigRemoveEvent(TRIG_EVENT_MOVIE_END_62, 1, 0);
    if (MoviePlayCheck != 0)
        MovieEndCheck = 1;
    return 0;
}

// CT3 0x01859c90: reload the scene and character banks a movie parked (state > 1000), by their type.
void PlayMovieReturnLoad(void)
{
    char name[0x100];
    int i;

    for (i = GLBL_BANK_MAX; i < BANK_MAX; i++) {
        if (BankInfo[i].state <= 0)
            continue;
        if (BankInfo[i].state >= BANK_STATE_MOVIE_PARKED) {
            BankInfo[i].state = BANK_STATE_FREE;
            if (BankInfo[i].type == FS_LOAD_BEHAVIOR) {
                strcpy(name, BankInfo[i].name);
                name[4] = 0;
                TrgCharacterBehaviorLoad(BankID_TO_CharID((short)i), name, i, BankInfo[i].groupCount);
            } else if (BankInfo[i].type == FS_LOAD_SHAPE) {
                strcpy(name, BankInfo[i].name);
                name[4] = 0;
                TrgCharacterShapeLoad(BankID_TO_CharID((short)i), name, i, BankInfo[i].groupCount);
            } else if (BankInfo[i].type == FS_LOAD_SCENE) {
                strcpy(name, BankInfo[i].name);
                name[4] = 0;
                TrgSceneLoad(name, i, BankInfo[i].groupCount);
            }
        }
        i += BankInfo[i].groupCount - 1;
    }
}

// CT3 0x01859dd0: the GS, DMA, VIF and interrupts set up again after a movie, then one BGT frame that
// clears the screen (main's boot sequence, without its buffer addresses).
void SystemReReset(void)
{
    sceGsResetPath();
    sceDmaReset(1);
    sceGsSyncV(0);
    sceGsResetGraph(1, SCE_GS_INTERLACE, SCE_GS_NTSC, SCE_GS_FRAME);
    sceGsPutDispEnv(&ga_disp);
    gaDmaInit();
    gaVifInit();
    gaSetupGSInitData(&gsinit);
    FlushCache(WRITEBACK_DCACHE);
    gaInitInterrupt();
    gaInitMeter();
    gaBGT_Start(bgt_ringbuffer, 0x8000);           /* main.c BGT_RINGBUFFER_SIZE */
    gaBGT_ExecGSInitData_VIF1(&gsinit);
    gaBGT_ExecDBufferData_VIF1(&drawbuffer_frameclear);
    gaBGT_ExecDrawCtrlData_VIF1(&drawctrl_frameclear);
    gaBGT_ExecDrawCommandData_VIF1(&drawcom_frameclear);
    gaBGT_ExecDBufferData_VIF1(&drawbuffer_0);
    gaBGT_End();
}

// PORT: the parts of the bank whose table it is (ALL_TABLE.partsHeader is an EE word, decision (h)).
static MODEL_PARTS *TableParts(ALL_TABLE *table)
{
    int i;
    for (i = 0; i < BANK_MAX; i++)
        if (All_table_TBL[i] == table)
            return BANK_PTR(MODEL_PARTS *, i, table->partsHeader);
    return NULL;
}

// CT3 0x01859e90: a part's world position.
void asGetObjectPositionAtWorld(float *dst, ALL_TABLE *table, int part)
{
    MODEL_PARTS *parts = TableParts(table);
    if (parts != NULL)
        memcpy(dst, parts[part].pos, sizeof(parts[part].pos));
}

// CT3 0x01859eb0: Alyssa's (the player character's) position.
void asGetAlyssaPosition(float *dst)
{
    // The ROM indexes Character[] with asFindCharacterStruct(0)'s answer, which is its argument: 0.
    memcpy(dst, &Character[(intptr_t)asFindCharacterStruct(NULL)].pos, 4 * sizeof(float));
}

// CT3 0x01859fc0: an analog stick's direction in degrees (asRadianByO about Y of (x, 0, y)) and its
// tilt (0..~1.4). stick: pad STICK_PAD(stick), STICK_FIRST picks the pair. deadZone: the bytes within
// 0x80 -/+ deadZone read as centred; x reads positive to the left, y positive down.
void asGetAnalogStickInformation(int stick, int deadZone, float *degrees, float *magnitude)
{
    u_int hi = (u_int)((signed char)deadZone + 0x80) & 0xff;
    u_int lo = (u_int)(0x80 - (signed char)deadZone) & 0xff;
    PAD_DATA *p = &pad[(stick & 0xff) >> 1];
    u_int x, y;
    alignas(16) float v[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float len2, rs;

    if (stick & STICK_FIRST) {
        x = p->stick[0];
        y = p->stick[1];
    } else {
        x = p->stick[2];
        y = p->stick[3];
    }
    if (x < lo)
        v[0] = -(float)(int)(x - lo) / (float)lo;
    else if (x > hi)
        v[0] = -(float)(int)(x - hi) / (float)(int)(0xff - hi);
    if (y < lo)
        v[2] = (float)(int)(y - lo) / (float)lo;
    else if (y > hi)
        v[2] = (float)(int)(y - hi) / (float)(int)(0xff - hi);
    // VU0: vrsqrt of the squared length, xyz scaled by it, then 1 / that: the length. On the VU a zero
    // vector stays zero with a zero length; the host keeps it so.
    len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (len2 > 0.0f) {
        rs = 1.0f / sqrtf(len2);
        v[0] *= rs;
        v[1] *= rs;
        v[2] *= rs;
        *magnitude = 1.0f / rs;
    } else {
        *magnitude = 0.0f;
    }
    *degrees = asRadianByO(v, RADIAN_AXIS_Y) * 360.0f / 6.2831855f;
}

// CT3 0x0185a230: place a part at pos (w 1) and mark it placed.
void asSetObjectTrans(ALL_TABLE *table, int part, const float *pos)
{
    MODEL_PARTS *parts = TableParts(table);
    if (parts == NULL)
        return;
    memcpy(parts[part].drawOnPos, pos, sizeof(parts[part].drawOnPos));
    parts[part].drawOnPos[3] = 1.0f;
    parts[part].flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
}

// CT3 0x0185a260: a part's initial translation; the ROM tail-calls asVectorLength on it (its answer
// left in $f0 for whoever wants it: the callers in RotateCamera.c say).
float asGetObjectTransInitialValue(float *dst, ALL_TABLE *table, int part)
{
    MODEL_PARTS *parts = TableParts(table);
    if (parts != NULL)
        memcpy(dst, parts[part].transInit, sizeof(parts[part].transInit));
    return asVectorLength(dst);
}

// ---- slice 9 (2026-11-10): letterbox requests, damage, RUF view matrices, angles, lengths, random ----

// CT3 0x0185ad70: a scene starts: the letterbox comes in (now = 1 slides it, 0 puts it up at once), the
// invisibility and advantage times are granted now rather than later, a full panic ends quietly, and
// a skippable scene (skippable, with DEMO_SKIP_SET and no skip armed yet) arms the demo skip.
void VistaVisionStart(int now, int skippable)
{
    VistaVisionNumber = now * 2 + VISTA_SHOW_NOW;
    ssTrigRemoveEvent(TRIG_EVENT_VISTA_SHOWN, 1, 0);
    if (ssTrigCheckEvent(TRIG_EVENT_INVISIBLE, 1, 0) != 0) {
        ssTrigRemoveEvent(TRIG_EVENT_INVISIBLE, 1, 0);
        TransparentBodyTime = PANIC_TRANSPARENT_TIME;
    }
    if (ssTrigCheckEvent(TRIG_EVENT_MC_LOADED, 1, 0) != 0) {
        ssTrigRemoveEvent(TRIG_EVENT_MC_LOADED, 1, 0);
        AdvantageousTimeTime = PANIC_ADVANTAGEOUS_TIME;
    }
    if (PanicInfoData.mode & PANIC_MODE_FULL) {
        PanicInfoData.mode &= ~PANIC_MODE_FULL;
        PanicInfoData.filterAlpha = -1;
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_FULL, 1, 0);
        ssTrigRemoveEvent(TRIG_EVENT_PANIC_BEAT, 1, 0);
    }
    if (skippable != 0 && DEMO_SKIP_SET != 0 && Demo_Skip_Set == 0) {
        DemoSkip = 0;
        DEMO_SKIP_FLAG = 0;
        ssTrigRegisterEvent(TRIG_EVENT_DEMO_SKIP_9E, 1, 0);
        Demo_Skip_Set = 1;
    }
    MidiControl_DemoStart();
}

// CT3 0x0185ad30: a scene ends: the letterbox goes (now = 1 slides it out, 0 at once).
void VistaVisionEnd(int now)
{
    VistaVisionNumber = now * 2 + VISTA_HIDE_NOW;
    ssTrigRemoveEvent(TRIG_EVENT_VISTA_HIDDEN, 1, 0);
    MidiControl_DemoEnd();
}

// CT3 0x0185a550: the player loses `damage` hit points; 1 (and hp 0) when none are left.
int PlayerDamage(int damage)
{
    CHARACTER *c = &Character[Player_BankID];
    c->hp -= damage;
    if (c->hp < 1) {
        c->hp = 0;
        return 1;
    }
    return 0;
}

// CT3 0x0185a280: turn the part by `degrees` about one axis (ROTATE_AXIS_*): ROTATE_SET makes the turn
// its rotation (placeRot's rows; drawOnPos kept), ROTATE_MULTIPLY applies it to the whole placement
// matrix (placeRot and drawOnPos, gaMulMatrix(place, place, turn)). The part is marked placed and moved.
void asObjectRotate(float degrees, ALL_TABLE *table, int part, int multiply, int axis)
{
    MODEL_PARTS *parts = TableParts(table);
    // PORT: another axis leaves the ROM's turn as stack garbage; the unit matrix stands in for it.
    alignas(16) float turn[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                                    {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}};
    float (*place)[4];
    int angle = DEGREES_TO_ANGLE16(degrees);

    if (parts == NULL)
        return;
    place = parts[part].placeRot;               /* rows 0..2, drawOnPos row 3 (EE-contiguous) */
    if ((s_char)axis == ROTATE_AXIS_Z)
        gaSetRotMatrixZ(turn, angle);
    else if ((s_char)axis == ROTATE_AXIS_Y)
        gaSetRotMatrixY(turn, angle);
    else if ((s_char)axis == ROTATE_AXIS_X)
        gaSetRotMatrixX(turn, angle);
    if ((s_char)multiply == ROTATE_MULTIPLY)
        gaMulMatrix(place, place, turn);
    else if ((s_char)multiply == ROTATE_SET)
        memcpy(parts[part].placeRot, turn, sizeof(parts[part].placeRot));
    parts[part].flags43 = PARTS_FLAG43_WARPED | PARTS_FLAG43_TRANS;
}

// CT3 0x0185a9b0: the view matrix of a camera at pos with axes right / up / front: the inverse of
// the rows (right, up, front, (0, 0, 0, 1)), its translation row then minus pos taken through it.
void asPosRUF2ViewMatrix(float (*view)[4], const float *pos, const float *right, const float *up,
                         const float *front)
{
    float t[4];
    int i;

    memcpy(view[0], right, 4 * sizeof(float));
    memcpy(view[1], up, 4 * sizeof(float));
    memcpy(view[2], front, 4 * sizeof(float));
    view[3][0] = view[3][1] = view[3][2] = 0.0f;
    view[3][3] = 1.0f;
    gaInversMatrix(view, view);
    view[3][0] = view[3][1] = view[3][2] = 0.0f;
    view[3][3] = 1.0f;
    for (i = 0; i < 4; i++)
        t[i] = view[0][i] * pos[0] + view[1][i] * pos[1] + view[2][i] * pos[2] + view[3][i] * pos[3];
    view[3][0] = -t[0];
    view[3][1] = -t[1];
    view[3][2] = -t[2];
    view[3][3] = 1.0f;
}

// CT3 0x0185a7e0: as asPosRUF2ViewMatrix, from the eye: the view origin lies three focal lengths in mm
// (asFocalLength_PS22LEICA(screen_0[8]) * 3) ahead along front (asViewMatrix2CamRUF's inverse).
void asCamRUF2ViewMatrix(float (*view)[4], const float *eye, const float *right, const float *up,
                         const float *front)
{
    alignas(16) float pos[4];
    float ahead = asFocalLength_PS22LEICA(screen_0[SCREEN_PROJ_DIST]) * CAMERA_EYE_FOCALS;

    pos[0] = eye[0] + front[0] * ahead;
    pos[1] = eye[1] + front[1] * ahead;
    pos[2] = eye[2] + front[2] * ahead;
    pos[3] = front[3];                          /* VU0 .xyz: w is front's (unused: row 3 is (0,0,0,1)) */
    asPosRUF2ViewMatrix(view, pos, right, up, front);
}

// CT3 0x0185dae0: the angle (ahAcos units) between a and b seen along `axis` (that component dropped
// from both), signed by (a x b)[axis]: positive only when it is above 0.
int asRadianByVectors(const float *a, const float *b, int axis)
{
    alignas(16) float va[4], vb[4];
    float c;
    int angle;

    memcpy(va, a, sizeof(va));
    memcpy(vb, b, sizeof(vb));
    va[axis] = 0.0f;
    vb[axis] = 0.0f;
    angle = ahAcos(gaInnerProductXYZ(va, vb) / sqrtf(asVectorLength(va) * asVectorLength(vb)));
    if (axis == ROTATE_AXIS_X)
        c = va[1] * vb[2] - vb[1] * va[2];
    else if (axis == ROTATE_AXIS_Y)
        c = va[2] * vb[0] - vb[2] * va[0];
    else
        c = va[0] * vb[1] - vb[0] * va[1];
    // The ROM's sign: 0 for +/-0, -1 below, 1 otherwise (NaN too); only 1 keeps the angle.
    if (c == 0.0f || c < 0.0f)
        angle = -angle;
    return angle;
}

// CT3 0x0185da40: the angle from the facing (the unit matrix's Z row, turned about Y by `angle` when
// axis is ROTATE_AXIS_Y) to the line to - from, along `axis`, negated.
int asRadianByPositions_Rorate(const float *from, const float *to, int angle, int axis)
{
    // the ROM copies @521 (0x01937e20): the unit matrix
    alignas(16) float m[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}};
    alignas(16) float d[4];

    d[0] = from[0] - to[0];
    d[1] = from[1] - to[1];
    d[2] = from[2] - to[2];
    d[3] = from[3];
    if (axis == ROTATE_AXIS_Y)
        gaSetRotMatrixY(m, angle);
    return -asRadianByVectors(m[2], d, axis);
}

// CT3 0x0185de70: a - b of two 16-bit angles, wrapped to -0x8000..0x8000.
int asISubRotate16(u_int a, u_int b)
{
    int d = (int)(a & 0xffff) - (int)(b & 0xffff);
    if (d > 0x8000)
        d -= 0x10000;
    else if (d < -0x8000)
        d += 0x10000;
    return d;
}

// CT3 0x0185df10
float asVectorLength1D(float x2)
{
    return sqrtf(x2);
}

// CT3 0x0185df30: |(x, y, 0)| (VU0 RSQRT then 1 / Q, as asVectorLength3D).
float asVectorLength2D(float x, float y)
{
    return sqrtf(x * x + y * y);
}

// CT3 0x0185dfe0: |a - b| over xyz (asVectorDistance's, not squared).
float asVectorDistance2(const float *a, const float *b)
{
    float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

// CT3 0x0185c240
// CT3 0x0185c26c (LowerCase_body, game-inferred: the loop head 0x0185c240 branches to, split by Ghidra).
// ASCII capitals to small letters, in place.
void LowerCase(char *s)
{
    for (; *s != '\0'; s++)
        if (*s > '@' && *s < '[')
            *s += 0x20;
}

// CT3 0x0185e3e0
float absf(float x)
{
    if (x < 0.0f)
        x *= -1.0f;
    return x;
}

// CT3 0x0185c320: 0..n-1 (an unsigned remainder), -1 for n 0. The seed (tnRandSeed, sbss 0x01942390)
// has its bytes swapped pairwise, then times 5 plus 0x19283765.
int tnRand(int n)
{
    if (n == 0)
        return -1;
    tnRandSeed = ((tnRandSeed & 0xff00ff00u) >> 8) + ((tnRandSeed & 0x00ff00ffu) << 8);
    tnRandSeed *= 5;
    tnRandSeed += 0x19283765u;
    return (int)(tnRandSeed % (u_int)n);
}

// CT3 0x01937e00 (next, .sdata): the seed of rnd, 64 bits.
static u_long next = 0x1234123412341234ull;

// CT3 0x0185e960: a 64-bit LCG; its high word, low 31 bits.
u_int rnd(void)
{
    next = next * 0x7d2b89ddull + 0x3039;
    return (u_int)(next >> 32) & 0x7fffffffu;
}

// CT3 0x0185e910: 0 <= x < 1, the low 23 bits of rnd as a mantissa.
float rndf(void)
{
    u_int bits = (rnd() & 0x7fffffu) | 0x3f800000u;
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f - 1.0f;
}

// ---- slice 11 (2026-11-10): GetAreaFloor, asFindMotionID ------------------------------------------

#define ROOM_IS(room, name) (memcmp((name), (room), ROOM_NAME_LEN) == 0)

// Both floor flags off (the ROM's pair of removes, in its order).
static void AreaFloorClear(void)
{
    ssTrigRemoveEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
    ssTrigRemoveEvent(TRIG_EVENT_AREA_PASS3, 1, 0);
}

// CT3 0x018524e0: called on a portal crossing (ssScnPortalCheck). The rooms the ROM lists in pairs
// share an area; stepping from one into its partner sets TRIG_EVENT_AREA_PASS2 (GetAreaNumber's second
// row), 1B12 -> 1B08 sets TRIG_EVENT_AREA_PASS3; stepping in from anywhere else clears both, and so does
// entering any of the partners themselves. 1C01 <-> 1C02 keep PASS2 as it was. The tests run in the
// ROM's order, every one of them (a later match can undo an earlier one).
void GetAreaFloor(CHARACTER *chara)
{
    char room[16];
    char prev[16];

    // callcheck: memcmp, ssTrigRemoveEvent (ROOM_IS and AreaFloorClear make them)
    if (chara->bank < 0)
        return;
    strcpy(room, asConvertHash2String(chara->roomHash));
    strcpy(prev, asConvertHash2String(chara->roomHashPrev));

    if (ROOM_IS(room, "0A01")) {
        if (ROOM_IS(prev, "0A06") || ROOM_IS(prev, "0A10"))
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        else
            AreaFloorClear();
    }
    if (ROOM_IS(room, "0A06"))
        AreaFloorClear();
    if (ROOM_IS(room, "0A10"))
        AreaFloorClear();
    if (ROOM_IS(room, "0B01")) {
        if (ROOM_IS(prev, "0B06") || ROOM_IS(prev, "0A06") || ROOM_IS(prev, "0A10"))
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        else
            AreaFloorClear();
    }
    if (ROOM_IS(room, "0B06"))
        AreaFloorClear();
    if (ROOM_IS(room, "1C01")) {
        if (ROOM_IS(prev, "1C02")) {
            if (ssTrigCheckEvent(TRIG_EVENT_AREA_PASS2, 1, 0) == 0)
                AreaFloorClear();
            else
                ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        } else if (ROOM_IS(prev, "1C06")) {
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        } else {
            AreaFloorClear();
        }
    }
    if (ROOM_IS(room, "1C02")) {
        // no else here: from anywhere but 1C01 / 1C04 the flags stay as they were
        if (ROOM_IS(prev, "1C01")) {
            if (ssTrigCheckEvent(TRIG_EVENT_AREA_PASS2, 1, 0) == 0)
                AreaFloorClear();
            else
                ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        }
        if (ROOM_IS(prev, "1C04"))
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
    }
    if (ROOM_IS(room, "1C06"))
        AreaFloorClear();
    if (ROOM_IS(room, "1C04"))
        AreaFloorClear();
    if (ROOM_IS(room, "1B11")) {
        if (ROOM_IS(prev, "1B07"))
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        else
            AreaFloorClear();
    }
    if (ROOM_IS(room, "1B07"))
        AreaFloorClear();
    if (ROOM_IS(room, "1B08")) {
        if (ROOM_IS(prev, "1B12"))
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS3, 1, 0);
        else
            AreaFloorClear();
    }
    if (ROOM_IS(room, "1B12"))
        AreaFloorClear();
    if (ROOM_IS(room, "2B01")) {
        if (ROOM_IS(prev, "2B04"))
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        else
            AreaFloorClear();
    }
    if (ROOM_IS(room, "2B04"))
        AreaFloorClear();
    if (ROOM_IS(room, "4A04")) {
        if (ROOM_IS(prev, "4A06") || ROOM_IS(prev, "4A07") || ROOM_IS(prev, "4A09") || ROOM_IS(prev, "4A10"))
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        else
            AreaFloorClear();
    }
    if (ROOM_IS(room, "4A06")) {
        if (ROOM_IS(prev, "4A04") || ROOM_IS(prev, "4A05") || ROOM_IS(prev, "4A11") || ROOM_IS(prev, "4A12"))
            ssTrigRegisterEvent(TRIG_EVENT_AREA_PASS2, 1, 0);
        else
            AreaFloorClear();
    }
    if (ROOM_IS(room, "4A07") || ROOM_IS(room, "4A09") || ROOM_IS(room, "4A10") || ROOM_IS(room, "4A05")
        || ROOM_IS(room, "4A11") || ROOM_IS(room, "4A12"))
        AreaFloorClear();
}

#undef ROOM_IS

// PORT: CHARACTER +CHARA_MOTION_TABLE holds the EE address of the character's motion table, a word of
// its bank written by asInitPlayerBehaviorControler (wideXcmd_DebugPrint.c, which reads it the same way
// in MotionRecord): resolved through BankEePtr (open-items.md 1 h). NULL when the word is 0, where the
// ROM would read address 0.
static const u_char *MotionTableOf(const CHARACTER *ch)
{
    u_int ee;
    memcpy(&ee, (const u_char *)ch + CHARA_MOTION_TABLE, sizeof(ee));
    return (const u_char *)BankEePtr(ch->bank, ee);
}

// CT3 0x0185e060: the index of the character's motion record (CHARA_MOTION_SIZE bytes, led by its
// hash, a 0 hash ending the table) whose hash is `hash`; MOTION_NOT_FOUND when none has it, 0 when the
// character has no bank.
int asFindMotionID(int chara, u_long hash)
{
    const CHARACTER *ch = &Character[chara];
    const u_char *table;
    u_long h = 0;
    int i = 0;

    if (ch->bank < 0)
        return 0;
    table = MotionTableOf(ch);
    if (table != NULL)
        memcpy(&h, table, sizeof(h));
    while (h != 0) {
        if (h == hash)
            return i;
        i++;
        memcpy(&h, table + i * CHARA_MOTION_SIZE, sizeof(h));
    }
    return MOTION_NOT_FOUND;
}
