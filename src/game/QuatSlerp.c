// game/QuatSlerp.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "game/QuatSlerp.h"

#include <math.h>
#include <string.h>

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

// stub of CT3 0x01856c30
void AmbientAlphaUpdate(void)
{
    ALYSSAPAN_STUB("AmbientAlphaUpdate", 0x01856c30);
}

// stub of CT3 0x01853220
void ControllerClatteringInit(void)
{
    ALYSSAPAN_STUB("ControllerClatteringInit", 0x01853220);
}

// stub of CT3 0x01852f90
void ControllerClatteringUpdate(void)
{
    ALYSSAPAN_STUB("ControllerClatteringUpdate", 0x01852f90);
}

// stub of CT3 0x01856660
void FogAnimeUpdate(void)
{
    ALYSSAPAN_STUB("FogAnimeUpdate", 0x01856660);
}

// stub of CT3 0x01854d60
void MonsterAIUpdate(int mode)
{
    ALYSSAPAN_STUB("MonsterAIUpdate", 0x01854d60);
}

// stub of CT3 0x01853620
void PanicFilter(void)
{
    ALYSSAPAN_STUB("PanicFilter", 0x01853620);
}

// stub of CT3 0x01854410
void PanicModeInit(void)
{
    ALYSSAPAN_STUB("PanicModeInit", 0x01854410);
}

// stub of CT3 0x018537b0
void PanicUpdate(void)
{
    ALYSSAPAN_STUB("PanicUpdate", 0x018537b0);
}

// stub of CT3 0x018550b0
void PlayerControlUpdate(int mode)
{
    ALYSSAPAN_STUB("PlayerControlUpdate", 0x018550b0);
}

// stub of CT3 0x0185aa50
void VistaVisionMain(void)
{
    ALYSSAPAN_STUB("VistaVisionMain", 0x0185aa50);
}

// stub of CT3 0x0185bd50
void WarpObjectUpdate(void)
{
    ALYSSAPAN_STUB("WarpObjectUpdate", 0x0185bd50);
}

// stub of CT3 0x0185bf80
void WarpObjectUpdateInit(void)
{
    ALYSSAPAN_STUB("WarpObjectUpdateInit", 0x0185bf80);
}

// stub of CT3 0x01858c90
void XcmdScreenFilter(void)
{
    ALYSSAPAN_STUB("XcmdScreenFilter", 0x01858c90);
}

// stub of CT3 0x01858bc0
void XcmdScreenFilterSkip(void)
{
    ALYSSAPAN_STUB("XcmdScreenFilterSkip", 0x01858bc0);
}

// stub of CT3 0x0185b980
void asWarpObjectBody(int a0, int a1)
{
    ALYSSAPAN_STUB("asWarpObjectBody", 0x0185b980);
}

// stub of CT3 0x0185c100
void asWarpSceneObject(void *table, int maxId, float *pos)
{
    ALYSSAPAN_STUB("asWarpSceneObject", 0x0185c100);
}

// stub of CT3 0x01854d30
void ssMenuUpdate(int mode)
{
    ALYSSAPAN_STUB("ssMenuUpdate", 0x01854d30);
}

// stub of CT3 0x0185e580
u_long ahGetHash(const char *name)
{
    ALYSSAPAN_STUB("ahGetHash", 0x0185e580);
    return {};
}

// stub of CT3 0x0185e360
int asCheckBankID(int bank)
{
    ALYSSAPAN_STUB("asCheckBankID", 0x0185e360);
    return {};
}

// stub of CT3 0x0185e150
int asFindModelPartsID(u_char *header, u_long hash)
{
    ALYSSAPAN_STUB("asFindModelPartsID", 0x0185e150);
    return {};
}

// stub of CT3 0x01858f20
int OneMaxObjectDrawZOnly(int bank, int index)
{
    ALYSSAPAN_STUB("OneMaxObjectDrawZOnly", 0x01858f20);
    return {};
}

// stub of CT3 0x0185e380
int asCheckBankType(int bank)
{
    ALYSSAPAN_STUB("asCheckBankType", 0x0185e380);
    return {};
}

// stub of CT3 0x0185a8a0
void asViewMatrix2PosRUF(float *view, float *pos, float *right, float *up, float *front)
{
    ALYSSAPAN_STUB("asViewMatrix2PosRUF", 0x0185a8a0);
}

// stub of CT3 0x01859520
void sunBGT_ExecTBufferData_VIF1(void *tbuffer)
{
    ALYSSAPAN_STUB("sunBGT_ExecTBufferData_VIF1", 0x01859520);
}

// stub of CT3 0x018590b0
void sunBGT_LoadGSTexClutData_VIF1(void *tbuffer)
{
    ALYSSAPAN_STUB("sunBGT_LoadGSTexClutData_VIF1", 0x018590b0);
}

// stub of CT3 0x01859530
void sunSetupTBufferData(void *tbuffer, TEX_INFO tex)
{
    ALYSSAPAN_STUB("sunSetupTBufferData", 0x01859530);
}

// stub of CT3 0x01854560
void tnInit3dlibWork(void *work, void *spr)
{
    ALYSSAPAN_STUB("tnInit3dlibWork", 0x01854560);
}

// stub of CT3 0x0185a6f0
void asViewMatrix2CamRUF(float (*view)[4], float *pos, float *right, float *up, float *forward)
{
    ALYSSAPAN_STUB("asViewMatrix2CamRUF", 0x0185a6f0);
}

// stub of CT3 0x01856e70
void AmbientAlphaInit(int scene)
{
    ALYSSAPAN_STUB("AmbientAlphaInit", 0x01856e70);
}

// stub of CT3 0x01856070
void TexFileSetInit(int scene)
{
    ALYSSAPAN_STUB("TexFileSetInit", 0x01856070);
}

// stub of CT3 0x018532a0
void EventSoundEnd(void)
{
    ALYSSAPAN_STUB("EventSoundEnd", 0x018532a0);
}

// stub of CT3 0x0185a650
void CameraMatrix2CameraPos(float *pos)
{
    ALYSSAPAN_STUB("CameraMatrix2CameraPos", 0x0185a650);
}

// stub of CT3 0x0185a5e0
void CameraMatrix2CameraTrg3m(float *trg)
{
    ALYSSAPAN_STUB("CameraMatrix2CameraTrg3m", 0x0185a5e0);
}

// stub of CT3 0x0185a400
int BankID_TO_CharID(short bank)
{
    ALYSSAPAN_STUB("BankID_TO_CharID", 0x0185a400);
    return {};
}

// stub of CT3 0x01852240
int GetAreaNumber(int a0)
{
    ALYSSAPAN_STUB("GetAreaNumber", 0x01852240);
    return {};
}

// stub of CT3 0x0185c2e0
u_long GetRoomName_file(const char *name)
{
    ALYSSAPAN_STUB("GetRoomName_file", 0x0185c2e0);
    return {};
}

// stub of CT3 0x0185e100
u_char * asFindModelParts(u_char *header, u_long hash)
{
    ALYSSAPAN_STUB("asFindModelParts", 0x0185e100);
    return {};
}

// stub of CT3 0x01854b20
int asJointParts(int srcBank, int bank, u_long hash)
{
    ALYSSAPAN_STUB("asJointParts", 0x01854b20);
    return {};
}

// stub of CT3 0x01854750
int asJointParts_byMatrix(float (*mat)[4], int *pos, int bank)
{
    ALYSSAPAN_STUB("asJointParts_byMatrix", 0x01854750);
    return {};
}

// stub of CT3 0x01859f00
void asGetCrossPointInPlaneWithLine(float out[4], float a[4], float b[4], float dir[4], float plane[4])
{
    ALYSSAPAN_STUB("asGetCrossPointInPlaneWithLine", 0x01859f00);
}

// stub of CT3 0x018520b0
void OmniRangeChange(u_char *rec)
{
    ALYSSAPAN_STUB("OmniRangeChange", 0x018520b0);
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

// stub of CT3 0x0185d4d0
void asMakeWatchMeMatrixToPosition(float (*out)[4], float (*m)[4], float *target, int axis)
{
    ALYSSAPAN_STUB("asMakeWatchMeMatrixToPosition", 0x0185d4d0);
}

// stub of CT3 0x01851dc0
int ahAcos(float c)
{
    ALYSSAPAN_STUB("ahAcos", 0x01851dc0);
    return {};
}

// stub of CT3 0x01851d60
float ahCos(int angle)
{
    ALYSSAPAN_STUB("ahCos", 0x01851d60);
    return {};
}

// stub of CT3 0x0185ddf0
void asMakeRotMatrixByQuartanion(float (*m)[4], float *axis, int angle)
{
    ALYSSAPAN_STUB("asMakeRotMatrixByQuartanion", 0x0185ddf0);
}

// stub of CT3 0x01851bf0
void QuatSlerp(float *q, float *q0, float *q1, float *ts)
{
    ALYSSAPAN_STUB("QuatSlerp", 0x01851bf0);
}

// stub of CT3 0x0185d380
int asDelSqueezeInformation(float (*out)[4], float (*a)[4], float (*b)[4])
{
    ALYSSAPAN_STUB("asDelSqueezeInformation", 0x0185d380);
    return {};
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


// stub of CT3 0x0185c840
void asContInterpolateZipNormal_XYZ(short *out, const short *a, const short *b, const float *w, int n)
{
    ALYSSAPAN_STUB("asContInterpolateZipNormal_XYZ", 0x0185c840);
}

// stub of CT3 0x0185ca50
void asContInterpolateZipTrans_XYZ(short *out, const short *a, const short *b, const float *w, int n,
                                   u_int shift)
{
    ALYSSAPAN_STUB("asContInterpolateZipTrans_XYZ", 0x0185ca50);
}

// stub of CT3 0x0185c530
void asInterpolateZipFloat0_7_9X(short *out, const short *a, const short *b, const float *w)
{
    ALYSSAPAN_STUB("asInterpolateZipFloat0_7_9X", 0x0185c530);
}

// stub of CT3 0x0185c630
void asInterpolateZipFloat1_1_14X(short *out, const short *a, const short *b, const float *w)
{
    ALYSSAPAN_STUB("asInterpolateZipFloat1_1_14X", 0x0185c630);
}

// stub of CT3 0x0185cfb0
void asInterpolateZipUV(short *out, const short *a, const short *b, const float *w)
{
    ALYSSAPAN_STUB("asInterpolateZipUV", 0x0185cfb0);
}

// stub of CT3 0x0185dc30
float asRadianByO(float *v, int axis)
{
    ALYSSAPAN_STUB("asRadianByO", 0x0185dc30);
    return {};
}

