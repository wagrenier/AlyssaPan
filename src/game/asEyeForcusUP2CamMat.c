// game/asEyeForcusUP2CamMat.c -- Clock Tower 3: the camera (EE 01883db0..01889cd0). Each bank has a
// camera (Cam[], one per global bank 0..5 and Cam[6] shared by the scene banks): a cut (CamChange_S) moves
// it, camera_view / camera_main run it each frame, asChangeViewMatrix_forTheBank selects a bank's view.
// Slice 1 (2026-11-10): the view builder, the bank switch, init / destroy, the cut entry; the cut's
// destination search (CamChangeSub, asGetDestination*) and the per-frame camera are still stubs.
// Slice 2 (2026-11-10): the per-frame camera (camera_view, camera_view0), the move step, the cut by part
// hash and its stock; asGetDestinationEYE and camera_main are still stubs.
// Slice 3 (2026-11-10): asGetDestinationEYE. Slice 4 (2026-11-10): camera_main -- the TU's code is complete.

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "common/alyssapan_stub.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "ga/gaInit3dlib.h"
#include "ga/gaSinCos.h"
#include "game/LinkMatrixTree.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/main2.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"
#include "sdk/libpad.h"

// ---- data (symtab names; see the header) ---------------------------------------------------------------
float AlissaEyeHeight = CAMERA_EYE_HEIGHT_INIT; /* sdata 0x019386e0 */
float CameraBehind = CAMERA_BEHIND_INIT;    /* sdata 0x019386e4 */
int iLastCameraBank;                        /* sdata 0x019387c8 */
char TheFirstChangeCamera;                  /* sbss 0x0193cf0c */
int iSm;                                    /* sbss 0x0193cefc */
char szCamModeAdd[64];                      /* sbss 0x0193d010 */
alignas(16) float NDAlyssaPos[4];           /* sbss 0x0193d090 */
int iCameraMode;                            /* sbss 0x0193d0a0 */
alignas(16) int CameraAngle[4];             /* sbss 0x0193d0c0 */
float FOV_deg;                              /* sbss 0x01941830 */
float ZelCam[6];                            /* sbss 0x01941880 */
u_char CameGri_Mode;                        /* sbss 0x019418a8 */
alignas(8) int CamStock[6];                 /* sbss 0x019418b0 */
u_char *lpCopyMP[35];                       /* sbss 0x0193cf80 */
int iNeoCameraLock;                         /* sbss 0x0193cf00 */
char CameraError;                           /* sbss 0x0193cf04 */
float NDAlissaEyeHeight = CAMERA_EYE_HEIGHT_INIT; /* sdata 0x01938764 */
float NDCameraBehind = CAMERA_BEHIND_INIT;  /* sdata 0x01938768 */
alignas(16) int NDCameraAngle[4];           /* sbss 0x0193d070 */
float CameGri_dvsZ;                         /* sbss 0x01941898 */
float CameGri_dvsX;                         /* sbss 0x0194189c */
u_int CameGri_DU;                           /* sbss 0x019418a0 */
u_int CameGri_DR;                           /* sbss 0x019418a4 */
CAM_QUAKE CamQuake[CAM_QUAKE_AXES];         /* sbss 0x0193cf10 */
float NDCameraEyePoint;                     /* sbss 0x0193cef8 */
int NDbVerticalMove;                        /* sbss 0x0193d050 */
alignas(16) float NDdeltaMove[4];           /* sbss 0x0193d060 */
alignas(16) int NDAlyssaRot[4];             /* sbss 0x0193d080 */
int NDiChar = CHARACTER_MAX - 1;            /* sdata 0x019387c4: 34 */
alignas(16) static float deltaMove[4];      /* sbss 0x0193d0b0 (static) */
// camera_main's pressure tables (5 entries: a pressure byte blends two neighbours).
static const float foword_move_table[5] = {4.0f, 6.0f, 8.0f, 10.0f, 12.0f};   /* sdata 0x019386f0 (static) */
static const float side_move_table[5] = {4.0f, 6.0f, 8.0f, 10.0f, 12.0f};     /* sdata 0x01938710 (static) */
static const float length_move_table[5] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f};     /* sdata 0x01938730 (static) */
static const int turn_angle_table[5] = {16, 32, 64, 128, 256};                /* sdata 0x01938750 (static) */
const float NDfoword_move_table[5] = {4.0f, 6.0f, 8.0f, 10.0f, 12.0f};        /* sdata 0x01938770 */
const float NDside_move_table[5] = {4.0f, 6.0f, 8.0f, 10.0f, 12.0f};          /* sdata 0x01938790 */
const float NDlength_move_table[5] = {2.0f, 3.0f, 4.0f, 5.0f, 6.0f};          /* sdata 0x019387b0 */
static const char s_cameraId[] = "Camera ID %d";                              /* rodata 0x019344a0 (@2089) */

// vmulax / vmadday / vmaddaz / vmaddw.xyzw: out = v.x m[0] + v.y m[1] + v.z m[2] + v.w m[3] (out may be v).
static void ApplyMatrixXYZW(float *out, float (*m)[4], const float *v)
{
    float r[4];
    for (int k = 0; k < 4; k++)
        r[k] = m[0][k] * v[0] + m[1][k] * v[1] + m[2][k] * v[2] + m[3][k] * v[3];
    memcpy(out, r, sizeof(r));
}


// vmul / vaddy / vaddz, vrsqrt, vmulq.xyz: xyz by 1 / |xyz| (the VU's RSQRT of 0 is the largest float).
static void NormalizeXYZ(float *v)
{
    float len2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    float q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;
    v[0] *= q;
    v[1] *= q;
    v[2] *= q;
}

// vopmula / vopmsub .xyz: out.xyz = a x b; w is left as out had it (the ROM's register keeps a's w).
static void CrossXYZ(float *out, const float *a, const float *b)
{
    float x = a[1] * b[2] - b[1] * a[2];
    float y = a[2] * b[0] - b[2] * a[0];
    float z = a[0] * b[1] - b[0] * a[1];
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// CT3 0x01883db0
// The view matrix of an eye looking at focus with up as the rough up: front = |focus - eye|, right =
// |up x front|, up' = |front x right|, built by asCamRUF2ViewMatrix with screen_0's projection distance
// set to proj for the call (restored after). 0 (and the view untouched) when eye and focus coincide.
int asEyeForcusUP2CamMat(float proj, float (*view)[4], float *eye, float *focus, float *up)
{
    alignas(16) float right[4], front[4], upn[4];
    float saved;

    if (eye[0] == focus[0] && eye[1] == focus[1] && eye[2] == focus[2])
        return 0;
    // Each vector keeps the w of the register it was built in (the VU ops are .xyz).
    front[0] = focus[0] - eye[0];
    front[1] = focus[1] - eye[1];
    front[2] = focus[2] - eye[2];
    front[3] = focus[3];
    NormalizeXYZ(front);
    upn[0] = up[0];
    upn[1] = up[1];
    upn[2] = up[2];
    upn[3] = up[3];
    NormalizeXYZ(upn);
    right[3] = upn[3];
    CrossXYZ(right, upn, front);
    NormalizeXYZ(right);
    upn[3] = front[3];
    CrossXYZ(upn, front, right);
    NormalizeXYZ(upn);
    saved = screen_0[SCREEN_PROJ_DIST];
    screen_0[SCREEN_PROJ_DIST] = proj;
    asCamRUF2ViewMatrix(view, eye, right, upn, front);
    screen_0[SCREEN_PROJ_DIST] = saved;
    return 1;
}

// CT3 0x01883f00
// The camera's up moving from `from` to `to` over a cut's steps: step 0 takes from, -1 leaves out alone,
// any other blends with to's weight 0.5 + step * 0.29 / 20 (asInterpolateVector) and normalizes xyz.
void asCameraPositionTranserUP(int step, float *out, const float *from, const float *to)
{
    alignas(16) float v[4];
    float w[2];

    if (step == 0) {
        out[0] = from[0];
        out[1] = from[1];
        out[2] = from[2];
        out[3] = from[3];
        return;
    }
    if (step == -1)
        return;
    w[1] = ((float)step * 0.29f) / 20.0f + 0.5f;
    w[0] = 1.0f - w[1];
    asInterpolateVector(v, from, to, w);
    NormalizeXYZ(v);
    out[0] = v[0];
    out[1] = v[1];
    out[2] = v[2];
    out[3] = v[3];
}

// CT3 0x01883fd0
// One frame of a camera point's move from `from` toward `to`: speed 0 jumps, -1 leaves it; else it moves
// (ticks / 5) * 15 * (1 + cos(180 + half the distance, in degrees, at most 180)) / 2 * (20 / speed)
// along the line, and lands on `to` once half the distance is under that step.
void asCameraPositionTranser(int speed, float *out, const float *to, const float *from)
{
    alignas(16) float d[4];
    float len2, q, half, step, ticks;
    u_int angle;

    if (speed == 0) {
        memcpy(out, to, 4 * sizeof(float));
        return;
    }
    if (speed == -1)
        return;
    d[0] = to[0] - from[0];                 // vsub.xyz: w stays to's
    d[1] = to[1] - from[1];
    d[2] = to[2] - from[2];
    d[3] = to[3];
    len2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    q = len2 > 0.0f ? 1.0f / sqrtf(len2) : FLT_MAX;     // vrsqrt; then vdiv 1 / q: the length
    d[0] *= q;
    d[1] *= q;
    d[2] *= q;
    half = (1.0f / q) / 2.0f;
    if (half < 180.0f)
        angle = (u_int)(int)((half * 180.0f) / 180.0f) & 0xffff;
    else
        angle = 180;
    ticks = gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / 5.0f;
    step = ((15.0f * ticks) * (1.0f + ahCos((int)(((float)(int)(angle + 180) * 65536.0f) / 360.0f) & 0xffff))) /
           2.0f * (20.0f / (float)speed);
    if (half < step) {
        memcpy(out, to, 4 * sizeof(float));
        return;
    }
    for (int k = 0; k < 4; k++)             // vmulx.xyzw, vadd.xyzw
        out[k] = from[k] + d[k] * step;
}

// CT3 0x01884610
// End camera cam's cut move: the cut's eye / target kinds take over, the move clock and the focal blend stop.
void asCamModeChange(int cam)
{
    Cam[cam].eyeMode = Cam[cam].eyeKind;
    Cam[cam].trgMode = Cam[cam].trgKind;
    Cam[cam].moveTime = 0;
    Cam[cam].focalMoving = 0;
}

// CT3 0x018841f0
// The model part target record `target` of camera id's bank looks at: when the record is on a character
// (CAM_TRG_ON_CHARA == 1) whose bank is loaded, lpCopyMP[chara] is found again unless it already is the
// part with this hash; an unloaded character's slot is cleared.
void asCopyCharacterParameterTRG(u_int target, int id)
{
    const u_char *rec;
    int chara, bank;

    id = (signed char)id;                   // the ROM takes it as a char
    rec = (const u_char *)BankEePtr(Cam[id].bank, Cam[id].targets) + (target & 0xffff) * CAM_TRG_SIZE;
    if (rec[CAM_TRG_ON_CHARA] != 1)
        return;
    chara = *(const u_short *)(rec + CAM_TRG_CHARA);
    bank = Character[chara].bank;
    if (BankInfo[bank].state == BANK_STATE_LOADED) {
        if (lpCopyMP[chara] == NULL || *(u_long *)(lpCopyMP[chara] + 8) != *(const u_long *)rec)
            lpCopyMP[chara] = asFindModelParts(pModelPartsHeader[bank], *(const u_long *)rec);
    } else {
        lpCopyMP[chara] = NULL;
    }
}

// CT3 0x01884300
// The same for the eye of camera id's live cut (CAM_CUT_ON_CHARA / CAM_CUT_CHARA).
void asCopyCharacterParameterEYE(int id)
{
    const u_char *rec = (const u_char *)BankEePtr(Cam[id].bank, Cam[id].cuts) + Cam[id].cut * CAM_CUT_SIZE;
    int chara, bank;

    if (rec[CAM_CUT_ON_CHARA] != 1)
        return;
    chara = *(const u_short *)(rec + CAM_CUT_CHARA);
    bank = Character[chara].bank;
    if (BankInfo[bank].state == BANK_STATE_LOADED) {
        if (lpCopyMP[chara] == NULL || *(u_long *)(lpCopyMP[chara] + 8) != *(const u_long *)rec)
            lpCopyMP[chara] = asFindModelParts(pModelPartsHeader[bank], *(const u_long *)rec);
    } else {
        lpCopyMP[chara] = NULL;
    }
}

// CT3 0x01884410
// The focus a cut's target list gives (CAM_CUT_TRG_MODE, also the answer): ONE takes target 0's part
// position, AXES x / y / z from targets 0 / 1 / 2 with w 1; a character's part not found leaves the
// component and clears w (no focus). A target is a part of the camera's bank (partsHeader) or of a
// character (lpCopyMP, refreshed by asCopyCharacterParameterTRG).
int asGetDstinationFromTargetList(float *focus, u_char *partsHeader, u_char *cutRec, int id)
{
    signed char mode = (signed char)cutRec[CAM_CUT_TRG_MODE];
    const u_char *trg, *part;
    int i;

    if (mode == CAM_TRG_MODE_4 || mode == CAM_TRG_MODE_2)
        return mode;
    if (mode == CAM_TRG_MODE_AXES) {
        focus[3] = 1.0f;
        for (i = 0; i < 3; i++) {
            asCopyCharacterParameterTRG(cutRec[CAM_CUT_TARGETS + i], (signed char)id);
            trg = (const u_char *)BankEePtr(Cam[id].bank, Cam[id].targets) +
                  cutRec[CAM_CUT_TARGETS + i] * CAM_TRG_SIZE;
            if (trg[CAM_TRG_ON_CHARA] == 0) {
                part = PARTS_REC(partsHeader, *(const u_short *)(trg + CAM_TRG_PART));
                focus[i] = ((const float *)(part + CAM_PART_POS))[i];
            } else if ((part = lpCopyMP[*(const u_short *)(trg + CAM_TRG_CHARA)]) == NULL) {
                focus[3] = 0.0f;
            } else {
                focus[i] = ((const float *)(part + CAM_PART_POS))[i];
            }
        }
    } else if (mode == CAM_TRG_MODE_ONE) {
        asCopyCharacterParameterTRG(cutRec[CAM_CUT_TARGETS], (signed char)id);
        trg = (const u_char *)BankEePtr(Cam[id].bank, Cam[id].targets) + cutRec[CAM_CUT_TARGETS] * CAM_TRG_SIZE;
        if (trg[CAM_TRG_ON_CHARA] == 0) {
            memcpy(focus, PARTS_REC(partsHeader, *(const u_short *)(trg + CAM_TRG_PART)) + CAM_PART_POS,
                   4 * sizeof(float));
        } else if ((part = lpCopyMP[*(const u_short *)(trg + CAM_TRG_CHARA)]) == NULL) {
            focus[3] = 0.0f;
        } else {
            memcpy(focus, part + CAM_PART_POS, 4 * sizeof(float));
        }
    }
    return mode;
}

// CT3 0x01884680
// Where p lies against the quad's four edges seen along n: byte i of the answer is the sign of
// n . ((q[i] - p) x (q[i+1] - p)) -- 1, 0xff (-1) or 0. All four alike: p is inside the quad.
u_int asCheckThePositionInRectAngle(float (*quad)[4], float *p, float *n)
{
    alignas(16) float u[4], v[4];
    u_int bits = 0;
    float s;
    int i;

    for (i = 0; i < 4; i++) {
        u[0] = quad[i][0] - p[0], u[1] = quad[i][1] - p[1], u[2] = quad[i][2] - p[2], u[3] = quad[i][3];
        v[0] = quad[(i + 1) & 3][0] - p[0], v[1] = quad[(i + 1) & 3][1] - p[1];
        v[2] = quad[(i + 1) & 3][2] - p[2], v[3] = quad[(i + 1) & 3][3];
        CrossXYZ(u, u, v);
        if (gaInnerProductXYZ(n, u) == 0.0f)
            s = 0.0f;
        else if (gaInnerProductXYZ(n, u) == -0.0f) // never true after the test above: the ROM's
            s = 0.0f;
        else
            s = gaInnerProductXYZ(n, u) < 0.0f ? -1.0f : 1.0f;
        bits |= ((u_int)(int)s & 0xff) << ((i & 3) * 8);
    }
    return bits;
}

// CT3 0x018847f0
// Where the ray p + t dir meets the quad's plane (n = |(q2 - q1) x (q0 - q1)|): 0 with out = that
// point (w 1) when it lies inside the quad. Otherwise 1 and out is moved onto the quad's rim: with one
// edge exactly through it or one edge facing away, onto the line from that edge's start (i) along
// q[i] - q[i+1], solved in x / z (x / y when that is degenerate) against the edge from q[i+1] to q[i+2];
// with two edges facing away, onto the corner between them. out.w is 0 then.
int asGetPlaneRadTargetPos(float *out, float *p, float *dir, float (*quad)[4])
{
    int idx[4] = {-1, -1, -1, -1};          /* @1029 (sdata 0x019387d0) */
    alignas(16) float a[4], b[4], n[4], e1[4], e2[4], at[4];
    int zeros = 0, ones = 0, away = 0, i;
    u_int bits, byte;
    float t, det;

    a[0] = quad[2][0] - quad[1][0], a[1] = quad[2][1] - quad[1][1], a[2] = quad[2][2] - quad[1][2];
    a[3] = quad[2][3];
    b[0] = quad[0][0] - quad[1][0], b[1] = quad[0][1] - quad[1][1], b[2] = quad[0][2] - quad[1][2];
    b[3] = quad[0][3];
    n[3] = a[3];
    CrossXYZ(n, a, b);
    NormalizeXYZ(n);
    a[0] = quad[0][0] - p[0], a[1] = quad[0][1] - p[1], a[2] = quad[0][2] - p[2], a[3] = quad[0][3];
    t = gaInnerProductXYZ(n, a);
    t = t / gaInnerProductXYZ(n, dir);
    out[0] = dir[0] * t + p[0];
    out[1] = dir[1] * t + p[1];
    out[2] = dir[2] * t + p[2];
    out[3] = 1.0f;
    bits = asCheckThePositionInRectAngle(quad, out, n);
    if (bits == 0x01010101)
        return 0;
    for (i = 0; i < 4; i++) {
        byte = bits >> ((i & 3) * 8) & 0xff;
        if (byte == 0xff)
            away++;
        else if (byte == 1)
            ones++;
        else if (byte == 0)
            zeros++;
    }
    if (zeros > 0 || away == 1) {
        u_int want = zeros > 0 ? 0 : 0xff;
        for (i = 0; i < 4 && (bits >> ((i & 3) * 8) & 0xff) != want; i++)
            ;
        idx[0] = i;
        idx[1] = (i + 1) & 3;
        idx[2] = (i + 1) & 3;
        idx[3] = (i + 2) & 3;
    } else if (away == 2) {
        for (i = 0; i < 4; i++)
            if ((bits >> ((i & 3) * 8) & 0xff) == 0xff && (bits >> (((i + 1) & 3) * 8) & 0xff) == 0xff)
                break;
        memcpy(out, quad[(i + 1) & 3], 4 * sizeof(float));
        return 1;
    }
    if (idx[0] < 0 || idx[0] > 3) {
        // PORT: the ROM reads quad[-1] here (three or four edges facing away, which a convex quad and
        // its own normal cannot give); the host leaves out on the plane with no rim.
        out[3] = 0.0f;
        return 1;
    }
    memcpy(at, quad[idx[0]], sizeof(at));
    for (i = 0; i < 4; i++) {
        e1[i] = quad[idx[0]][i] - quad[idx[1]][i];
        e2[i] = quad[idx[2]][i] - quad[idx[3]][i];
    }
    det = e1[0] * e2[2] - e2[0] * e1[2];
    if (det == 0.0f)
        t = (e2[0] * (at[1] - out[1]) + e2[1] * (out[0] - at[0])) / (e1[0] * e2[1] - e2[0] * e1[1]);
    else
        t = (e2[0] * (at[2] - out[2]) + e2[2] * (out[0] - at[0])) / det;
    for (i = 0; i < 4; i++)
        out[i] = e1[i] * t + at[i];
    out[3] = 0.0f;
    return 1;
}

// CT3 0x01884c20
// The eye on line a-b that sees p at `angle` (ahSin units) off the line: foot is p's foot on the line
// (b - (b - a) (b - a).(b - p) / |b - a|^2). Angle 0: out = p, 2. A quarter turn: out = foot, 3 when it
// lies within a..b, else 1. Otherwise out = foot moved along the line by cos / sin times p's distance
// from it (asVectorDistance2 / asVectorLength1D: the ROM's units), 0 when within a..b, else 1.
int asGetLineRadTargetPos(float *a, float *b, float *p, float *out, float *foot, int angle)
{
    alignas(16) float ab[4], bp[4];
    float sinA = ahSin(angle), cosA = ahCos(angle), d2, k;

    d2 = asVectorDistance(b, a);
    ab[0] = b[0] - a[0], ab[1] = b[1] - a[1], ab[2] = b[2] - a[2], ab[3] = b[3];
    bp[0] = b[0] - p[0], bp[1] = b[1] - p[1], bp[2] = b[2] - p[2], bp[3] = b[3];
    k = -gaInnerProductXYZ(ab, bp) / d2;
    foot[0] = ab[0] * k + b[0];         // vmulx.xyzw, then vadd.xyz: w is ab's scaled
    foot[1] = ab[1] * k + b[1];
    foot[2] = ab[2] * k + b[2];
    foot[3] = ab[3] * k;
    if ((angle & 0x7fff) == 0) {
        memcpy(out, p, 4 * sizeof(float));
        return 2;
    }
    if ((angle & 0x7fff) == 0x4000) {
        memcpy(out, foot, 4 * sizeof(float));
        if (asCheckInnerPositions(a[0], out[0], b[0]) && asCheckInnerPositions(a[1], out[1], b[1]) &&
            asCheckInnerPositions(a[2], out[2], b[2]))
            return 3;
        return 1;
    }
    k = cosA * asVectorDistance2(p, foot) / sinA;
    k = k / asVectorLength1D(d2);
    out[0] = foot[0] + ab[0] * -k;
    out[1] = foot[1] + ab[1] * -k;
    out[2] = foot[2] + ab[2] * -k;
    out[3] = foot[3];
    if (asCheckInnerPositions(a[0], out[0], b[0]) && asCheckInnerPositions(a[1], out[1], b[1]) &&
        asCheckInnerPositions(a[2], out[2], b[2]))
        return 0;
    return 1;
}

// CT3 0x01884e60: 1 when x lies between a and b (either order, ends included).
int asCheckInnerPositions(float a, float x, float b)
{
    float lo = a;

    if (!(a <= b)) {
        lo = b;
        b = a;
    }
    return lo <= x && x <= b;
}

// CT3 0x01884eb0
// The focus point a cut moves to: kind CAM_TRG_KIND_LIST takes it from the cut's target list (1 when
// the list gives none), any other kind leaves it (1).
int asGetDestinationTRG(int kind, float *focus, int id)
{
    CAMERA *c = &Cam[id];

    if (kind != CAM_TRG_KIND_LIST)
        return 1;
    // ROM: All_table_TBL[bank]->partsHeader, the word scene_init keeps as pModelPartsHeader[bank].
    return asGetDstinationFromTargetList(focus, pModelPartsHeader[c->bank],
                                         (u_char *)BankEePtr(c->bank, c->cuts) + c->cut * CAM_CUT_SIZE,
                                         id) == 0 ? 1 : 0;
}

// CT3 0x018867d0
// Start bank cam's cut (Cam[].cut, set by CamChange_S): -1 when the bank is not loaded, has no camera or
// no such cut. The live eye / focus / up / focal become the *From set, the bank's cut and target lists
// are taken, the cut's DOF level switches the DOF pass (scene banks), and asGetDestinationEYE / _TRG
// fill the *To set. CAM_CHANGE_NEEDS_TARGETS: an eye kind 1 / 6 cut without a target list.
int CamChangeSub(int cam)
{
    int id = asCheckBankID(cam);
    ALL_TABLE *tbl;
    const u_char *ctrl, *rec;
    CAMERA *c;
    int n, i;

    if (cam < 0)
        return -1;
    if (BankInfo[cam].state != BANK_STATE_LOADED)
        return -1;
    tbl = All_table_TBL[cam];
    if (tbl == NULL)
        return -1;
    c = &Cam[id];
    if (c->cut < 0)
        return -1;
    ctrl = (const u_char *)BankEePtr(cam, tbl->ctrl);
    n = ctrl[CAM_CTRL_CUT_COUNT];
    if (n == 0 || c->cut >= n)
        return -1;
    for (i = 0; i < CHARACTER_MAX; i++)
        lpCopyMP[i] = NULL;
    memcpy(c->eyeFrom, c->eye, sizeof(c->eyeFrom));
    memcpy(c->focusFrom, c->focus, sizeof(c->focusFrom));
    memcpy(c->upFrom, c->up, sizeof(c->upFrom));
    c->focal[1] = c->focal[0];
    c->focalMoving = 1;
    c->cuts = *(const u_int *)(ctrl + CAM_CTRL_CUTS);
    c->targets = *(const u_int *)(ctrl + CAM_CTRL_TARGETS);
    c->ctrlList = *(const u_int *)(ctrl + OBJ_CTRL_LIST);
    rec = (const u_char *)BankEePtr(cam, c->cuts) + c->cut * CAM_CUT_SIZE;
    c->dofLevel = rec[CAM_CUT_DOF];
    if (asCheckBankType(cam) != 0) {
        if (c->dofLevel == CAM_DOF_NONE) {
            if (DrawMode == DRAW_MODE_DOF)
                DrawMode = DRAW_MODE_NORMAL;
        } else {
            DrawMode = DRAW_MODE_DOF;
        }
    }
    c->moveSpeed = rec[CAM_CUT_MOVE_SPEED];
    c->eyeKind = rec[CAM_CUT_EYE_KIND];
    if (rec[CAM_CUT_TRG_MODE] == CAM_TRG_MODE_NONE) {
        if (c->eyeKind == CAM_EYE_KIND_RAIL || c->eyeKind == CAM_EYE_KIND_SEGMENT)
            return CAM_CHANGE_NEEDS_TARGETS;
        c->trgKind = CAM_TRG_KIND_NONE;
    } else {
        c->trgKind = CAM_TRG_KIND_LIST;
    }
    if (asGetDestinationEYE(c->eyeKind, c->eyeTo, c->focusTo, c->upTo, &c->focal[2], id) == 1)
        asGetDestinationTRG(c->trgKind, c->focusTo, id);
    if (iCamChangeFlag == 0)
        memcpy(c->viewPrev, c->view2, sizeof(c->viewPrev));
    if (asCheckBankType(cam) != 0)
        ssTrigRegisterEvent(TRIG_EVENT_CAMERA_CUT, 1, 0);
    return 0;
}

// CT3 0x01886690
// Make `bank`'s camera the drawing view: its view2 becomes matrix_global_view and its live focal length
// (focal[3], LEICA mm) screen_0's projection distance. Banks past the globals share Cam[6].
void asChangeViewMatrix_forTheBank(int bank)
{
    iLastCameraBank = bank;
    memcpy(matrix_global_view, Cam[asCheckBankID(bank)].view2, sizeof(matrix_global_view));
    screen_0[SCREEN_PROJ_DIST] = asFocalLength_LEICA2PS2(Cam[asCheckBankID(bank)].focal[3]);
}

// CT3 0x01886770
u_char CamGetNowDOFLevel(void)
{
    return Cam[CAMERA_SHARED].dofLevel;
}

// CT3 0x01886730: a trigger command's DOF level for the shared camera; CAM_DOF_NONE ends the DOF pass.
void CamSetNowDOFLevel(short level)
{
    Cam[CAMERA_SHARED].dofLevel = (u_char)level;
    if (level == CAM_DOF_NONE) {
        if (DrawMode == DRAW_MODE_DOF)
            DrawMode = DRAW_MODE_NORMAL;
    } else {
        DrawMode = DRAW_MODE_DOF;
    }
}

// CT3 0x01886780: a trigger command's orbit camera (CAM_EYE_KIND_ORBIT's ZelCam); the third argument is
// unused, [2] is cleared. [4] / [5] keep the int words the ROM stores.
int CamChangeZELCAM_S(int maxDist, int minDist, int unused, int height, int angle, int fixed)
{
    (void)unused;
    memcpy(&ZelCam[ZELCAM_ANGLE], &angle, sizeof(int));
    memcpy(&ZelCam[ZELCAM_FIXED], &fixed, sizeof(int));
    ZelCam[2] = 0.0f;
    ZelCam[1] = (float)minDist;
    ZelCam[0] = (float)maxDist;
    ZelCam[3] = (float)height;
    return 0;
}

// ZelCam[4] / [5] as the int words CamChangeZELCAM_S stores there.
static int ZelCamWord(int i)
{
    int w;
    memcpy(&w, &ZelCam[i], sizeof(w));
    return w;
}

// PORT: a cut's character part not found yet (lpCopyMP NULL) reads the ROM's address 0; the host reads
// a zero record instead.
alignas(16) static const u_char NoPartRec[PARTS_REC_SIZE];

static const u_char *CamPartOrNone(const u_char *part)
{
    return part != NULL ? part : NoPartRec;
}

// CT3 0x01884f50
// Where a cut puts the eye (kind = CAM_EYE_KIND_*), with the focus from the cut's target list first (all
// kinds), the up from the eye part's z axis when the part has its own axes (else (0, 1, 0, 0)) and the
// cut's focal length. The answer is CAM_EYE_* (KEPT only for STAY: the caller then runs _TRG; -1 for an
// unknown kind). The path points (OBJ_CTRL of CAM_CUT_PATH) are in their part's space.
int asGetDestinationEYE(int kind, float *eye, float *focus, float *up, float *focal, int id)
{
    CAMERA *c = &Cam[id];
    u_char *cut = (u_char *)BankEePtr(c->bank, c->cuts) + c->cut * CAM_CUT_SIZE;
    const OBJ_CTRL *path;
    u_char *hdr = pModelPartsHeader[c->bank];   // ROM: All_table_TBL[bank] +0x2c
    const u_char *part, *trgPart, *trg;
    float (*world)[4], (*pts)[4];
    alignas(16) float m[4][4], dir[4], a[4], b[4], out[4], foot[4], best[4], quad[4][4];
    u_int partIdx;
    int targets, count, ans = CAM_EYE_PLACED, i;
    float bestDist, d, r, tol, h, horiz, diff, A, B, C, disc, s;

    asCopyCharacterParameterEYE(id);
    if (cut[CAM_CUT_ON_CHARA] == 0) {
        partIdx = *(u_short *)(cut + CAM_CUT_PART);
        part = PARTS_REC(hdr, partIdx);
    } else {
        partIdx = *(u_short *)(cut + CAM_CUT_CHARA);
        part = CamPartOrNone(lpCopyMP[partIdx]);
    }
    targets = asGetDstinationFromTargetList(focus, hdr, cut, id);
    trg = (const u_char *)BankEePtr(c->bank, c->targets) + cut[CAM_CUT_TARGETS] * CAM_TRG_SIZE;
    if (trg[CAM_TRG_ON_CHARA] == 0)
        trgPart = PARTS_REC(hdr, *(const u_short *)(trg + CAM_TRG_PART));
    else
        trgPart = CamPartOrNone(lpCopyMP[*(const u_short *)(trg + CAM_TRG_CHARA)]);
    // The ROM reads the path record here for every kind; the host only for the kinds that use it (a cut
    // without a path has CAM_CUT_PATH 0xff, past the list).
    if (kind == CAM_EYE_KIND_RAIL || kind == CAM_EYE_KIND_PLANE || kind == CAM_EYE_KIND_RING ||
        kind == CAM_EYE_KIND_SEGMENT) {
        path = BANK_PTR(const OBJ_CTRL *, c->bank, c->ctrlList) + cut[CAM_CUT_PATH];
        world = (float (*)[4])(PARTS_REC(hdr, path->part) + CAM_PART_WORLD);
        pts = BANK_PTR(float (*)[4], c->bank, path->ends);
        count = path->kind;
    } else {
        world = NULL, pts = NULL, count = 0;
    }
    memcpy(focal, cut + CAM_CUT_FOCAL, sizeof(float));

    if (part[CAM_PART_TYPE] == CAM_PART_TYPE_AXES) {
        memcpy(up, part + CAM_PART_AXIS_Z, 4 * sizeof(float));
        NormalizeXYZ(up);
    } else {
        up[0] = 0.0f, up[1] = 1.0f, up[2] = 0.0f, up[3] = 0.0f;
    }

    switch (kind) {
    case CAM_EYE_KIND_PART:
        memcpy(eye, part + CAM_PART_POS, 4 * sizeof(float));
        if (targets != CAM_TRG_MODE_NONE)
            return CAM_EYE_PLACED;
        if (part[CAM_PART_TYPE] == CAM_PART_TYPE_AXES) {
            memcpy(dir, part + CAM_PART_AXIS_Y, sizeof(dir));
            NormalizeXYZ(dir);
        } else {
            // ROM: Character[partIdx].rot even for a bank part (partIdx is then its record index).
            gaSetRotMatrixY(m, partIdx < CHARACTER_MAX ? Character[partIdx].rot : 0);
            memcpy(dir, m[2], sizeof(dir));
        }
        for (i = 0; i < 4; i++)
            focus[i] = eye[i] + dir[i] * CAM_EYE_PART_REACH;
        return CAM_EYE_PLACED;

    case CAM_EYE_KIND_RAIL: {
        int n = count < 0 ? -count : count;
        bestDist = -1.0f;
        if (targets != CAM_TRG_MODE_ONE && targets != CAM_TRG_MODE_AXES)
            return CAM_EYE_NO_TARGET;
        for (i = 0; i < n; i++) {
            if (i == n - 1) {
                if (count >= 0)         // an open path ends at its last point; a closed one wraps
                    break;
                ApplyMatrixXYZW(a, world, pts[i]);
                ApplyMatrixXYZW(b, world, pts[0]);
            } else {
                ApplyMatrixXYZW(a, world, pts[i]);
                ApplyMatrixXYZW(b, world, pts[i + 1]);
            }
            int r2 = asGetLineRadTargetPos(a, b, focus, out, foot, *(u_short *)(cut + CAM_CUT_ANGLE));
            if (r2 != 1 && r2 != 2) {
                d = asVectorDistance(out, focus);
                if (bestDist < 0.0f || d < bestDist) {
                    memcpy(best, out, sizeof(best));
                    bestDist = d;
                }
            }
        }
        if (!(0.0f <= bestDist))
            return CAM_EYE_RAIL_MISSED;
        memcpy(eye, best, sizeof(best));
        return CAM_EYE_PLACED;
    }

    case CAM_EYE_KIND_ORBIT:
        if (targets != CAM_TRG_MODE_ONE)
            return CAM_EYE_NO_TARGET;
        if (ZelCamWord(ZELCAM_FIXED) == 1) {
            // At ZelCam's angle around the target part's facing (its z row flattened), min distance back.
            gaSetRotMatrixY(m, ZelCamWord(ZELCAM_ANGLE));
            memcpy(dir, trgPart + CAM_PART_AXIS_Z, sizeof(dir));
            dir[1] = 0.0f;
            NormalizeXYZ(dir);
            ApplyMatrixXYZW(dir, m, dir);
            for (i = 0; i < 4; i++)
                dir[i] *= -ZelCam[1];
            dir[1] += ZelCam[3];
            for (i = 0; i < 4; i++)
                eye[i] = focus[i] + dir[i];
        } else {
            // On the line from the focus to the live eye, flattened, min distance out, raised.
            for (i = 0; i < 4; i++)
                dir[i] = c->eye[i] - focus[i];
            dir[1] = 0.0f;
            NormalizeXYZ(dir);
            for (i = 0; i < 4; i++)
                eye[i] = focus[i] + dir[i] * ZelCam[1];
            eye[1] += ZelCam[3];
        }
        return CAM_EYE_PLACED;

    case CAM_EYE_KIND_STAY:
        memcpy(eye, c->eye, 4 * sizeof(float));
        return CAM_EYE_KEPT;

    case CAM_EYE_KIND_PLANE:
        if (targets != CAM_TRG_MODE_ONE && targets != CAM_TRG_MODE_AXES)
            return CAM_EYE_NO_TARGET;
        if (count != CAM_PATH_CLOSED_QUAD)
            return CAM_EYE_NOT_QUAD;
        memcpy(dir, part + (part[CAM_PART_TYPE] == CAM_PART_TYPE_AXES ? CAM_PART_AXIS_Y : CAM_PART_AXIS_Z),
               sizeof(dir));
        NormalizeXYZ(dir);
        for (i = 0; i < 4; i++)
            dir[i] *= CAM_EYE_PART_REACH;
        for (i = 0; i < 4; i++)         // the quad's corners in reverse order
            ApplyMatrixXYZW(quad[3 - i], world, pts[i]);
        asGetPlaneRadTargetPos(out, focus, dir, quad);
        memcpy(eye, out, sizeof(out));
        for (i = 0; i < 4; i++)
            focus[i] = eye[i] + dir[i];
        return ans;

    case CAM_EYE_KIND_RING: {
        const float *p;
        if (targets != CAM_TRG_MODE_ONE && targets != CAM_TRG_MODE_AXES)
            return CAM_EYE_NO_TARGET;
        if (count < 1)
            return CAM_EYE_NO_RING;
        // ROM: the bank's record partIdx even for a character's part (then the character's slot).
        p = (const float *)(PARTS_REC(hdr, partIdx) + CAM_PART_POS);
        r = pts[0][0];
        tol = pts[0][1];
        h = p[1] - focus[1];
        h = r * r - h * h;
        d = 0.0f <= h ? asVectorLength1D(h) : 0.0f;
        horiz = asVectorLength2D(p[0] - focus[0], p[2] - focus[2]);
        diff = horiz - d;
        if (tol < absf(diff))
            d = 0.0f < diff ? horiz - tol : horiz + tol;
        for (i = 0; i < 4; i++)
            dir[i] = p[i] - focus[i];
        dir[1] = 0.0f;
        NormalizeXYZ(dir);
        for (i = 0; i < 4; i++)
            eye[i] = focus[i] + dir[i] * d;
        eye[1] = p[1];
        eye[3] = 1.0f;
        return ans;
    }

    case CAM_EYE_KIND_SEGMENT:
        if (targets != CAM_TRG_MODE_ONE && targets != CAM_TRG_MODE_AXES)
            return CAM_EYE_NO_TARGET;
        if (count < 2)
            return CAM_EYE_NO_SEGMENT;
        // The point of segment a-b at distance r from the focus: |a + t (b - a) - focus| = r.
        ApplyMatrixXYZW(a, world, pts[0]);
        ApplyMatrixXYZW(b, world, pts[1]);
        r = (float)*(u_short *)(cut + CAM_CUT_ANGLE);
        {
            alignas(16) float ap[4], ab[4], other[4];
            for (i = 0; i < 4; i++) {
                ap[i] = a[i] - focus[i];
                ab[i] = b[i] - a[i];
            }
            A = gaInnerProductXYZ(ab, ab);
            B = gaInnerProductXYZ(ap, ab);
            C = gaInnerProductXYZ(ap, ap);
            disc = (B * B - A * (C - r * r)) * 4.0f;
            if (disc < 0.0f) {
                // No such point: the line's closest point to the focus, pushed out horizontally to r.
                asGetCrossPointInPlaneWithLine(eye, a, b, ab, focus);
                for (i = 0; i < 4; i++)
                    eye[i] -= focus[i];
                h = eye[1];
                eye[1] = 0.0f;
                NormalizeXYZ(eye);
                s = asVectorLength1D(r * r - h * h);
                for (i = 0; i < 4; i++)
                    eye[i] *= s;
                eye[1] = h;
                for (i = 0; i < 4; i++)
                    eye[i] += focus[i];
            } else if (disc <= 0.0f) {
                for (i = 0; i < 4; i++)
                    eye[i] = ab[i] * (-B / A) + a[i];
            } else {
                // Two points: the one nearer b.
                s = asVectorLength1D(disc);
                for (i = 0; i < 4; i++) {
                    eye[i] = ab[i] * (-(B * 2.0f - s) / (A * 2.0f)) + a[i];
                    other[i] = ab[i] * (-(B * 2.0f + s) / (A * 2.0f)) + a[i];
                }
                if (asVectorDistance(b, other) < asVectorDistance(b, eye))
                    memcpy(eye, other, sizeof(other));
            }
        }
        eye[3] = 1.0f;
        return ans;

    default:
        return -1;
    }
}
// CT3 0x018867c0: a trigger command's FOV change; the ROM keeps only the answer.
int CamChangeFOV_S(void)
{
    return 1;
}

// CT3 0x01886dc0
// Cut bank `cam`'s camera to `cut`: CAMERA_CHANGE_SAME_CUT when it already shows it (the check is skipped
// once, for the first scene-bank cut, which also holds main for CAMERA_FIRST_CUT_SKIP frames), else the
// cut is recorded and CamChangeSub moves the camera.
int CamChange_S(int cam, short cut, u_char mode, int arg)
{
    CAMERA *c;
    int id;

    if (cam < 0)
        return CAMERA_CHANGE_NONE;
    id = asCheckBankID(cam);
    if (asCheckBankType(cam) == 0 || TheFirstChangeCamera != 0) {
        if (cut == Cam[id].cut && cam == Cam[id].bank)
            return CAMERA_CHANGE_SAME_CUT;
    } else {
        TheFirstChangeCamera = 1;
        SkipMain += CAMERA_FIRST_CUT_SKIP;
    }
    c = &Cam[id];
    c->eyeMode = 0xff;
    c->trgMode = 0xff;
    c->moveTicks = arg;
    c->moveTime = 0;
    c->bank = cam;
    c->cut = cut;
    c->changeMode = mode;
    CamStock[CAMSTOCK_BANK] = -1;
    return CamChangeSub(cam);
}

// CT3 0x018870c0
// Reset cameras: cam -1 all seven, else bank cam's own (the loop indexes Cam[] by the bank number as
// the ROM does: only banks 0..6 are passed). Mode 0 skips it when cam is a scene bank (the shared
// camera stays). A scene bank's reset also re-arms the first-cut skip and resets ZelCam; DOF drawing
// falls back to normal.
void camera_init0(int cam, int mode)
{
    int i = cam, last = cam;
    CAMERA *c;

    if (cam == -1) {
        last = 6;
        i = 0;
    }
    if (mode == 0 && asCheckBankID(cam) == CAMERA_SHARED)
        return;
    for (; i <= last; i++) {
        if (i < 0 || i >= CAMERA_MAX)       // PORT: the ROM would write past Cam[] (no caller does)
            break;
        c = &Cam[i];
        c->trgMode = asCheckBankType(i) == 0 ? 4 : 0xff;
        c->eyeMode = 0xff;
        for (int k = 0; k < 3; k++) {
            float *e = k == 0 ? c->eye : k == 1 ? c->eyeFrom : c->eyeTo;
            float *f = k == 0 ? c->focus : k == 1 ? c->focusFrom : c->focusTo;
            float *u = k == 0 ? c->up : k == 1 ? c->upFrom : c->upTo;
            e[0] = 10.0f, e[1] = 10.0f, e[2] = 10.0f, e[3] = 1.0f;
            f[0] = 0.0f, f[1] = 0.0f, f[2] = 0.0f, f[3] = 1.0f;
            u[0] = 0.0f, u[1] = 1.0f, u[2] = 0.0f, u[3] = 0.0f;
        }
        c->focal[0] = c->focal[1] = c->focal[2] = c->focal[3] = CAMERA_FOCAL_INIT;
        c->focalMoving = 0;
        c->b182 = 0;
        c->moveTicks = 0;
        c->moveTime = 0;
        memset(c->view, 0, sizeof(c->view));    // vf0 and its vmr32 rotations: the identity
        c->view[0][0] = c->view[1][1] = c->view[2][2] = c->view[3][3] = 1.0f;
        c->bank = -1;
        c->cuts = 0;
        c->targets = 0;
        c->ctrlList = 0;
        c->cut = -1;
        if (asCheckBankType(i) != 0)
            TheFirstChangeCamera = 0;
        c->dofLevel = CAM_DOF_NONE;
        c->moveSpeed = 0;
        if (asCheckBankType(i) != 0) {
            ZelCam[4] = 0.0f;
            ZelCam[0] = 400.0f;
            ZelCam[5] = 0.0f;
            ZelCam[1] = 200.0f;
            ZelCam[2] = 180.0f;
            ZelCam[3] = 30.0f;
        }
    }
    if (DrawMode == DRAW_MODE_DOF)
        DrawMode = DRAW_MODE_NORMAL;
}

// CT3 0x018875a0
void camera_destroy(int scene, int mode)
{
    camera_init0(asCheckBankID(scene), mode);
}

// CT3 0x01889c50
// The camera's start: eye height, distance behind, angles and mode cleared, a 40-degree field of view
// (into screen_0's projection distance), then every camera reset.
void camera_init(void)
{
    AlissaEyeHeight = CAMERA_EYE_HEIGHT_INIT;
    CameraBehind = CAMERA_BEHIND_INIT;
    CameraAngle[0] = CameraAngle[1] = CameraAngle[2] = CameraAngle[3] = 0;
    iCameraMode = 0;
    szCamModeAdd[0] = '\0';
    FOV_deg = CAMERA_FOV_DEG_INIT;
    screen_0[SCREEN_PROJ_DIST] = asFocalLength_LEICA2PS2(CAMERA_FOV_DEG_INIT);
    NDAlyssaPos[0] = NDAlyssaPos[1] = NDAlyssaPos[2] = NDAlyssaPos[3] = 0.0f;
    CameGri_Mode = 0xff;
    iSm = 0;
    camera_init0(-1, 0);
    CamStock[CAMSTOCK_BANK] = -1;
}

// CT3 0x01886b70
// Cut bank's camera to the cut whose eye is the bank part named `hash`: -1 when the bank is not loaded
// and drawn yet (the request is stocked in CamStock and camera_view retries it each frame), else as
// CamChange_S (CAMERA_CHANGE_NONE without a bank, CAMERA_CHANGE_SAME_CUT, or CamChangeSub's answer).
int CamChange_Hash_S_Hash(int bank, u_long hash, short mode, int arg)
{
    CAMERA *c;
    short cut;

    if (bank == -1)
        return CAMERA_CHANGE_NONE;
    if (BankInfo[bank].state != BANK_STATE_LOADED || BankInfo[bank].drawOn != DRAW_BANK_ON) {
        memcpy(&CamStock[CAMSTOCK_HASH], &hash, sizeof(hash));
        CamStock[CAMSTOCK_BANK] = bank;
        CamStock[CAMSTOCK_ARG] = arg;
        CamStock[CAMSTOCK_MODE] = mode;
        return -1;
    }
    c = &Cam[asCheckBankID(bank)];
    cut = asFindCamera(bank, hash);
    if (cut == c->cut && bank == c->bank)
        return CAMERA_CHANGE_SAME_CUT;
    c->eyeMode = 0xff;
    c->trgMode = 0xff;
    c->moveTicks = arg;
    c->moveTime = 0;
    c->bank = bank;
    c->cut = cut;
    c->changeMode = (u_char)mode;
    CamStock[CAMSTOCK_BANK] = -1;
    return CamChangeSub(bank);
}

// CT3 0x01886cf0
// The cut of bank's camera whose eye is the bank part named `hash` (cuts on a character skipped), -1
// when the bank is not loaded or no cut has it.
short asFindCamera(int bank, u_long hash)
{
    const u_char *ctrl, *cuts;
    const MODEL_PARTS *parts;
    short i;

    if (BankInfo[bank].state != BANK_STATE_LOADED)
        return -1;
    ctrl = (const u_char *)BankEePtr(bank, All_table_TBL[bank]->ctrl);
    cuts = (const u_char *)BankEePtr(bank, *(const u_int *)(ctrl + CAM_CTRL_CUTS));
    parts = (const MODEL_PARTS *)BankEePtr(bank, All_table_TBL[bank]->partsHeader);
    for (i = 0; i < (short)ctrl[CAM_CTRL_CUT_COUNT]; i++) {
        const u_char *rec = cuts + i * CAM_CUT_SIZE;
        if (rec[CAM_CUT_ON_CHARA] == 0 && parts[*(const u_short *)(rec + CAM_CUT_PART)].hash == hash)
            return i;
    }
    return -1;
}

// CT3 0x01886f30
// CamChange_S from a request record, without the first-cut skip.
int CamChange(CAM_CHANGE *req)
{
    int cam = req->bank;
    CAMERA *c;

    if (cam < 0)
        return CAMERA_CHANGE_NONE;
    c = &Cam[asCheckBankID(cam)];
    if (req->cut == c->cut && cam == c->bank)
        return CAMERA_CHANGE_SAME_CUT;
    c->eyeMode = 0xff;
    c->trgMode = 0xff;
    c->moveTicks = req->arg;
    c->moveTime = 0;
    c->bank = cam;
    c->cut = req->cut;
    c->changeMode = req->mode;
    CamStock[CAMSTOCK_BANK] = -1;
    return CamChangeSub(cam);
}

// CT3 0x01887040
// Keep every camera's view as view2 and its live focal length as focal[3] before camera_view moves
// them: the drawing view (asChangeViewMatrix_forTheBank) is the one built the frame before.
void asCopyCameraMatrix(void)
{
    for (int i = 0; i < CAMERA_MAX; i++) {
        memcpy(Cam[i].view2, Cam[i].view, sizeof(Cam[i].view2));
        Cam[i].focal[3] = Cam[i].focal[0];
    }
}

// a = a * w + b * rest on xyz (vmulx.xyz, vadd.xyz: a keeps its w).
static void BlendXYZ(float *a, const float *b, float w, float rest)
{
    for (int k = 0; k < 3; k++)
        a[k] = a[k] * w + b[k] * rest;
}

// CT3 0x01885d20
// One frame of camera cam (a loaded bank with a cut): the projection from the live focal length, the
// move clock, the eye / focus / up the eye and target kinds ask for (asGetDestinationEYE / _TRG; during
// the cut's move the cut's own kinds, blended from the *From set over moveTicks), the *To set from them
// (kind CAM_EYE_KIND_ORBIT keeps the eye in ZelCam's range), the live set moved toward it at moveSpeed,
// and the view matrix.
void camera_view0(int cam)
{
    CAMERA *c = &Cam[cam];
    alignas(16) float eye[4], focus[4], up[4];
    float focal, w, rest;
    int eyeKind, trgKind, speed = 0;

    if (c->bank < 0 || BankInfo[c->bank].state != BANK_STATE_LOADED || c->cut < 0 || c->cuts == 0)
        return;
    screen_0[SCREEN_PROJ_DIST] = asFocalLength_LEICA2PS2(c->focal[0]);
    if (gGlobalNextTicksInterval >= 0)
        c->moveTime += gGlobalNextTicksInterval;
    if ((s_char)c->eyeMode == (s_char)CAM_EYE_MODE_MOVE) {
        trgKind = (s_char)c->trgKind;
        eyeKind = (s_char)c->eyeKind;
    } else {
        trgKind = (s_char)c->trgMode;
        eyeKind = (s_char)c->eyeMode;
    }
    memcpy(eye, c->eye, sizeof(eye));
    memcpy(focus, c->focus, sizeof(focus));
    memcpy(up, c->up, sizeof(up));
    focal = c->focal[0];
    if (asGetDestinationEYE(eyeKind, eye, focus, up, &focal, cam) == 1)
        asGetDestinationTRG(trgKind, focus, cam);

    if (c->moveTicks == 0) {
        w = 1.0f;
    } else {
        if ((s_char)c->eyeMode != (s_char)CAM_EYE_MODE_MOVE)
            w = 1.0f;
        else if ((s_char)c->changeMode == CAM_CHANGE_MODE_EASE)
            w = (1.0f - ahCos((int)(((float)(long long)c->moveTime * 32768.0f) / (float)c->moveTicks))) / 2.0f;
        else
            w = (float)(long long)c->moveTime / (float)c->moveTicks;
        if (1.0f <= w)
            w = 1.0f;
    }
    rest = 1.0f - w;
    if ((s_char)c->trgMode == (s_char)CAM_EYE_MODE_MOVE) {
        BlendXYZ(focus, c->focusFrom, w, rest);
        BlendXYZ(eye, c->eyeFrom, w, rest);
        BlendXYZ(up, c->upFrom, w, rest);
        c->focal[2] = focal;
        focal = rest * c->focal[1] + w * focal;
    } else if (c->focalMoving == 1) {
        if (c->moveTicks < c->moveTime) {
            c->focalMoving = 0;
            focal = c->focal[2];
            c->focal[1] = c->focal[2];
        } else {
            focal = rest * c->focal[1] + w * c->focal[2];
        }
    } else {
        c->focal[2] = focal;
    }
    c->focal[0] = focal;

    switch ((s_char)c->eyeMode) {
    case 0:
    case 1:
    case 3:
    case 4:
    case 5:
    case 6:
        memcpy(c->eyeTo, eye, sizeof(eye));
        memcpy(c->focusTo, focus, sizeof(focus));
        memcpy(c->upTo, up, sizeof(up));
        speed = c->moveSpeed;
        break;
    case CAM_EYE_KIND_ORBIT: {
        float d, keep;
        memcpy(c->focusTo, focus, sizeof(focus));
        memcpy(c->upTo, up, sizeof(up));
        c->focusTo[1] += ZelCam[3];
        c->eyeTo[1] = c->focusTo[1];
        d = asVectorDistance2(c->focusTo, c->eyeTo);
        keep = ZelCam[1];
        if (ZelCam[1] <= d) {
            keep = ZelCam[0];
            if (d <= ZelCam[0])
                keep = d;
        }
        for (int k = 0; k < 3; k++)         // vsub.xyz, vmulx, vadd.xyz: w stays focusTo's
            c->eyeTo[k] = c->focusTo[k] + (c->eyeTo[k] - c->focusTo[k]) * (keep / d);
        c->eyeTo[3] = c->focusTo[3];
        c->focusTo[1] -= ZelCam[3];
        speed = c->moveSpeed;
        break;
    }
    case (s_char)CAM_EYE_MODE_MOVE:
        memcpy(c->eye, eye, sizeof(eye));
        memcpy(c->focus, focus, sizeof(focus));
        memcpy(c->up, up, sizeof(up));
        if (c->moveTicks <= c->moveTime) {
            memcpy(c->eyeTo, eye, sizeof(eye));
            memcpy(c->focusTo, focus, sizeof(focus));
            memcpy(c->upTo, up, sizeof(up));
            asCamModeChange(cam);
        }
        speed = -1;
        break;
    default:
        break;
    }
    asCameraPositionTranser(speed, c->eye, c->eyeTo, c->eye);
    asCameraPositionTranser(speed, c->focus, c->focusTo, c->focus);
    asCameraPositionTranserUP(speed, c->up, c->upTo, c->up);
    asEyeForcusUP2CamMat(asFocalLength_LEICA2PS2(c->focal[0]), c->view, c->eye, c->focus, c->up);
}

// The stick's deflection past the dead zone, -1..1 (camera_view; 0 inside it).
static float CameGriDeflection(u_int v, u_int lo, u_int hi)
{
    if (v < lo)
        return (float)(int)(v - lo) / 128.0f;
    if (hi < v)
        return (float)(int)(v - hi) / 128.0f;
    return 0.0f;
}

// camera_view's follow cameras (modes 0 / 1 and the ND camera): Cam[6].view is the turn
// RotX(-angle[0]) . RotY(-angle[1]) with the translation of (-pos - eye height on y) turned, pushed back
// `behind` on z; then the view is rebuilt through its position / right / up / front.
static void CameraFollowView(const float *pos, float eyeHeight, const int *angle, float behind)
{
    CAMERA *c = &Cam[CAMERA_SHARED];
    alignas(16) float m[4][4];
    float t[4] = {-pos[0], -pos[1] - eyeHeight, -pos[2], 1.0f};
    alignas(16) float p[4], r[4], u[4], f[4];

    gaSetRotMatrixY(m, -angle[1]);
    gaRotMatrixX(m, -angle[0], m);
    ApplyMatrixXYZW(t, m, t);
    t[2] += behind;
    memcpy(c->view, m, sizeof(c->view));
    c->view[3][0] += t[0];                  // vadd.xyz: w stays the turn's
    c->view[3][1] += t[1];
    c->view[3][2] += t[2];
    asViewMatrix2PosRUF(&c->view[0][0], p, r, u, f);
    asCamRUF2ViewMatrix(c->view, p, r, u, f);
}

// camera_view's scene-bank look-around: the stick (CameGri_Mode 0: pad[0]'s stick bytes 2 / 3, 1: bytes
// 0 / 1, else centred) eases CameGri_dvsX / _dvsZ toward its deflection; the eye's forward axis turns by
// CameGri_DU * dvsX about the view's up and CameGri_DR * dvsZ about its right, and the camera looks there.
static void CameraLookAround(int id)
{
    CAMERA *c = &Cam[id];
    float dx = 0.0f, dz = 0.0f, nx, ez;
    alignas(16) float inv[4][4], rot[4][4];
    alignas(16) float focus[4];

    if ((s_char)CameGri_Mode >= 0) {
        int dead = ((const s_char *)KEY_MAP)[KEY_MAP_STICK_DEAD_BYTE];
        u_int lo = (u_int)(CAMERA_STICK_CENTER - dead) & 0xff;
        u_int hi = (u_int)(CAMERA_STICK_CENTER + dead) & 0xff;
        u_int sx = CAMERA_STICK_CENTER, sz = CAMERA_STICK_CENTER;
        if (CameGri_Mode == 0) {
            sx = pad[0].stick[2];
            sz = pad[0].stick[3];
        } else if (CameGri_Mode == 1) {
            sx = pad[0].stick[0];
            sz = pad[0].stick[1];
        }
        dx = CameGriDeflection(sx, lo, hi);
        dz = CameGriDeflection(sz, lo, hi);
    }
    dx -= CameGri_dvsX;
    nx = CameGri_dvsX + (1.0f - ahCos((int)(dx * 32768.0f))) * 0.25f * dx;
    dz -= CameGri_dvsZ;
    CameGri_dvsZ = CameGri_dvsZ + (1.0f - ahCos((int)(dz * 32768.0f))) * 0.25f * dz;
    ez = (float)CameGri_DR * CameGri_dvsZ;
    memcpy(inv, c->view, sizeof(inv));
    CameGri_dvsX = nx;
    gaInversMatrix(inv, inv);
    asMakeRotMatrixByQuartanion(rot, inv[1], (int)((float)CameGri_DU * nx));
    ApplyMatrixXYZW(inv[2], rot, inv[2]);
    asMakeRotMatrixByQuartanion(rot, inv[0], (int)ez);
    ApplyMatrixXYZW(inv[2], rot, inv[2]);
    for (int k = 0; k < 4; k++)
        focus[k] = c->eye[k] + inv[2][k];
    asEyeForcusUP2CamMat(asFocalLength_LEICA2PS2(c->focal[0]), c->view, c->eye, focus, c->up);
}

// camera_view's shakes: each running CamQuake moves the eye along the view's right / up / forward axis
// by amp * sin(time / period + phase), the amplitude ramped over fadeIn and fadeOut ticks.
static void CameraQuake(int id)
{
    CAMERA *c = &Cam[id];
    alignas(16) float pos[4], axis[CAM_QUAKE_AXES][4];

    asViewMatrix2CamRUF(c->view, pos, axis[0], axis[1], axis[2]);
    for (int k = 0; k < CAM_QUAKE_AXES; k++) {
        CAM_QUAKE *q = &CamQuake[k];
        float amp, s;
        if (q->duration == 0)
            continue;
        if (q->time < q->fadeIn)
            amp = q->amp * (float)q->time / (float)q->fadeIn;
        else if ((int)q->time < (int)q->duration - (int)q->fadeOut)
            amp = q->amp;
        else
            amp = q->amp * (float)((int)q->duration - (int)q->time) / (float)q->fadeOut;
        // PORT: a 0 period would divide by zero (divu's answer is undefined on the EE): no sine then.
        s = ahSin((q->period ? (int)((float)(q->time % q->period) * 65536.0f / (float)q->period) : 0) +
                  q->phase);
        for (int j = 0; j < 4; j++)
            pos[j] += axis[k][j] * (amp * s);
        q->time = (u_short)(q->time + gGlobalNextTicksInterval);
        if (q->duration <= q->time) {
            q->duration = 0;
            q->time = 0;
        }
    }
    asCamRUF2ViewMatrix(c->view, pos, axis[0], axis[1], axis[2]);
}

// CT3 0x018875d0
// The frame's cameras: keep last frame's views (asCopyCameraMatrix), retry a stocked cut, run every
// loaded bank's camera (camera_view0: the cut's move; a scene bank's also the look-around stick and the
// shakes; a stopped scene bank's only once when b182 asks), make the shared camera the drawing view,
// then iCameraMode's follow camera. CameraError: the unit view instead.
void camera_view(void)
{
    int i;

    if (CameraError != 0) {
        memcpy(matrix_global_view, matUnit, sizeof(matrix_global_view));
        return;
    }
    asCopyCameraMatrix();
    if (CamStock[CAMSTOCK_BANK] >= 0) {
        u_long hash;
        memcpy(&hash, &CamStock[CAMSTOCK_HASH], sizeof(hash));
        CamChange_Hash_S_Hash(CamStock[CAMSTOCK_BANK], hash, (short)CamStock[CAMSTOCK_MODE],
                              CamStock[CAMSTOCK_ARG]);
    }
    for (i = 0; i < CAMERA_MAX; i++) {
        // ROM: indexes Cam[] by the loop and BankInfo by that camera's bank (-1 at init: BankInfo[-1]).
        if (Cam[i].bank < 0 || BankInfo[Cam[i].bank].state != BANK_STATE_LOADED)
            continue;
        if (asCheckBankType(i) == 0) {
            camera_view0(i);
        } else if (asCheckAnimationExecutable_Scene(Cam[i].bank) == 0) {
            if (Cam[asCheckBankID(i)].b182 == 1) {
                Cam[asCheckBankID(i)].b182 = 0;
                camera_view0(i);
            }
        } else {
            int id;
            camera_view0(i);
            id = asCheckBankID(i);
            CameraLookAround(id);
            if (CamQuake[0].duration != 0 || CamQuake[1].duration != 0 || CamQuake[2].duration != 0)
                CameraQuake(asCheckBankID(i));
        }
    }
    if (Cam[CAMERA_SHARED].bank != -1)
        asChangeViewMatrix_forTheBank(CAMERA_SHARED);
    switch (iCameraMode) {
    case CAMERA_MODE_ND:
        break;
    case CAMERA_MODE_SCENE:
        for (i = 0; i < CAMERA_MAX; i++) {
            if (Cam[i].cut < 0 && asCheckBankType(i) != 0 && idCurrentScene > GLOBAL_BANK_LAST &&
                BankInfo[idCurrentScene].state == BANK_STATE_LOADED &&
                BankInfo[idCurrentScene].drawOn != 0 &&
                ((const u_char *)BankEePtr(idCurrentScene,
                                           All_table_TBL[idCurrentScene]->ctrl))[CAM_CTRL_CUT_COUNT] != 0) {
                CAM_CHANGE req = {0, idCurrentScene, 0, 1, 0};
                CamChange(&req);
            }
        }
        if (iNeoCameraLock == 0)
            return;
        break;
    case CAMERA_MODE_FOLLOW1:
    case CAMERA_MODE_FOLLOW:
        CameraFollowView((const float *)Character[0].pos, AlissaEyeHeight, CameraAngle, CameraBehind);
        return;
    default:
        return;
    }
    CameraFollowView(NDAlyssaPos, NDAlissaEyeHeight, NDCameraAngle, NDCameraBehind);
}

// ---- camera_main: the pad's camera (slice 4, 2026-11-10) -----------------------------------------------

// A pressure byte through a 5-entry table: bits 6..7 pick the pair, bits 0..5 the blend (64 steps).
static float PressLerp(const float *tbl, u_char p)
{
    u_int f = p & 0x3f;
    int i = p >> 6;
    return ((float)f * tbl[i + 1] + (float)(int)(0x40 - f) * tbl[i]) / 64.0f;
}

// turn_angle_table holds ints: the same blend in integer steps, divided toward zero.
static int PressTurn(u_char p)
{
    int i = p >> 6;
    int r = (int)(0x40 - (p & 0x3f)) * turn_angle_table[i] + (int)(p & 0x3f) * turn_angle_table[i + 1];
    return r / 64;
}

// A stick byte with its dead zone: 0x53..0xad read as the rest value 120 (the ROM's, not 128).
static float StickByte(u_char b)
{
    return (b < CAMERA_STICK_DEAD_LO || CAMERA_STICK_DEAD_HI < b) ? (float)b : CAMERA_STICK_REST;
}

// This frame's ticks in fifths (0 while the interval is negative).
static float TickScale(void)
{
    return gGlobalNextTicksInterval < 0 ? 0.0f : (float)gGlobalNextTicksInterval / 5.0f;
}

// The flying cameras' (mode 0 / 1, ND flat / vertical) step in view space: the d-pad and L1 / L2.
static void FlyStep(float *delta, const PAD_DATA *pad, const float *fwd, const float *side, const float *len)
{
    delta[0] = 0.0f;
    delta[1] = 0.0f;
    delta[2] = 0.0f;
    delta[3] = 1.0f;
    if (pad_cnt_button[0] & SCE_PADLdown)
        delta[2] = -PressLerp(fwd, pad->press[PAD_PRESS_DOWN]);
    if (pad_cnt_button[0] & SCE_PADLup)
        delta[2] = PressLerp(fwd, pad->press[PAD_PRESS_UP]);
    if (pad_cnt_button[0] & SCE_PADLright)
        delta[0] = PressLerp(side, pad->press[PAD_PRESS_RIGHT]);
    if (pad_cnt_button[0] & SCE_PADLleft)
        delta[0] = -PressLerp(side, pad->press[PAD_PRESS_LEFT]);
    if (pad_cnt_button[0] & SCE_PADL1)
        delta[1] = PressLerp(len, pad->press[PAD_PRESS_L1]);
    if (pad_cnt_button[0] & SCE_PADL2)
        delta[1] = -PressLerp(len, pad->press[PAD_PRESS_L2]);
}

// The flying cameras' sticks and move: the right stick (bytes 2, 3) adds to the step, the left (0, 1)
// turns (angle[1] yaw, angle[0] pitch; rotOut takes the yaw + 0x8000 before [1] is masked, as the ROM
// does); the step turns by the yaw (and the pitch when vertical) and moves pos by it in ticks.
static void FlyMove(const PAD_DATA *pad, float *delta, int *angle, int *rotOut, int vertical, float *pos)
{
    float m[4][4];
    float v[4];
    float sx = StickByte(pad->stick[2]);
    float sy = StickByte(pad->stick[3]);
    float s;

    delta[0] -= (sx - CAMERA_STICK_REST) * 0.05f;
    delta[2] += (sy - CAMERA_STICK_REST) * 0.05f;
    sx = StickByte(pad->stick[0]);
    sy = StickByte(pad->stick[1]);
    angle[1] = (int)((float)angle[1] + (sx - CAMERA_STICK_REST) * 1.5f * TickScale());
    s = TickScale();
    *rotOut = (angle[1] + 0x8000) & 0xffff;
    angle[0] = (int)((float)angle[0] - (sy - CAMERA_STICK_REST) * 1.5f * s) & 0xffff;
    angle[1] &= 0xffff;
    angle[2] &= 0xffff;
    if (vertical == 0) {
        gaSetRotMatrixY(m, angle[1]);
    } else {
        gaSetRotMatrixX(m, angle[0]);
        gaRotMatrixY(m, angle[1], m);
    }
    ApplyMatrixXYZW(v, m, delta);
    pos[0] += v[0] * TickScale();
    pos[1] += v[1] * TickScale();
    pos[2] += v[2] * TickScale();
}

// R1 / R2 (or square / circle) scale a follow camera's distance and eye height.
static void ScaleFollow(float *behind, float *height, float k)
{
    *behind *= k;
    *height *= k;
}

// CT3 0x018881a0
// The frame's pad camera (ControllerMain; data_load_set with CAMERA_MODE_SCENE). GlobalCameraMode takes
// the mode. 0 / 1: Alyssa flies (d-pad, L1 / L2, sticks; R1 / R2 the follow distance; the face buttons
// turn), 1 along the pitch too. 2: the ND camera -- R1 cycles ND_MOVE_FLAT / _VERTICAL (the ND eye flies,
// started at Alyssa) and _DRIVE (R2 steps NDiChar; the pad walks that character, the eye rides it).
// 3: becomes CAMERA_MODE_SCENE with controller 4 and cut 0. 4: the scene camera; in controller mode 100
// L1 / L2 step the scene bank's cuts ("Camera ID %d" into szCamModeAdd). iCameraMode follows the mode.
void camera_main(PAD_DATA *pad, int mode)
{
    float m[4][4];
    int step = 0;
    int cur;

    GlobalCameraMode = mode;
    switch (mode) {
    case CAMERA_MODE_FOLLOW:
    case CAMERA_MODE_FOLLOW1:
        asCharacterMortionChangeRequest(0, 0, MOTION_DEBUG_CAMERA);
        FlyStep(deltaMove, pad, foword_move_table, side_move_table, length_move_table);
        if (pad_cnt_button[0] & SCE_PADR1)
            ScaleFollow(&CameraBehind, &AlissaEyeHeight, 0.95f);
        if (pad_cnt_button[0] & SCE_PADR2)
            ScaleFollow(&CameraBehind, &AlissaEyeHeight, 1.125f);
        if (pad_cnt_button[0] & SCE_PADRdown)
            CameraAngle[0] = (int)((float)CameraAngle[0] +
                                   TickScale() * (float)PressTurn(pad->press[PAD_PRESS_CROSS]));
        if (pad_cnt_button[0] & SCE_PADRup)
            CameraAngle[0] = (int)((float)CameraAngle[0] -
                                   TickScale() * (float)PressTurn(pad->press[PAD_PRESS_TRIANGLE]));
        if (pad_cnt_button[0] & SCE_PADRright)
            CameraAngle[1] = (int)((float)CameraAngle[1] -
                                   TickScale() * (float)PressTurn(pad->press[PAD_PRESS_CIRCLE]));
        if (pad_cnt_button[0] & SCE_PADRleft)
            CameraAngle[1] = (int)((float)CameraAngle[1] +
                                   TickScale() * (float)PressTurn(pad->press[PAD_PRESS_SQUARE]));
        bVerticalMove = mode;
        FlyMove(pad, deltaMove, CameraAngle, &Character[0].rot, mode, (float *)Character[0].pos);
        iCameraMode = mode;
        return;

    case CAMERA_MODE_ND:
        asCharacterMortionChangeRequest(0, 0, MOTION_DEBUG_CAMERA);
        if (NDAlyssaPos[3] == 0.0f) {
            NDAlyssaPos[0] = ((const float *)Character[0].pos)[0];
            NDAlyssaPos[1] = ((const float *)Character[0].pos)[1];
            NDAlyssaPos[2] = ((const float *)Character[0].pos)[2];
            NDAlyssaPos[3] = 1.0f;
            NDAlyssaRot[1] = Character[0].rot;
            NDbVerticalMove = ND_MOVE_FLAT;
        }
        if ((pad_trg_button[0] & SCE_PADR1) && ++NDbVerticalMove > ND_MOVE_DRIVE)
            NDbVerticalMove = ND_MOVE_FLAT;
        if (NDbVerticalMove != ND_MOVE_DRIVE) {
            if (NDbVerticalMove == ND_MOVE_FLAT || NDbVerticalMove == ND_MOVE_VERTICAL) {
                FlyStep(NDdeltaMove, pad, NDfoword_move_table, NDside_move_table, NDlength_move_table);
                if (pad_cnt_button[0] & SCE_PADRleft)
                    ScaleFollow(&NDCameraBehind, &NDAlissaEyeHeight, 0.95f);
                if (pad_cnt_button[0] & SCE_PADRright)
                    ScaleFollow(&NDCameraBehind, &NDAlissaEyeHeight, 1.125f);
                if (pad_cnt_button[0] & SCE_PADRup) {
                    NDAlissaEyeHeight = CAMERA_EYE_HEIGHT_INIT;
                    NDCameraBehind = CAMERA_BEHIND_INIT;
                }
                FlyMove(pad, NDdeltaMove, NDCameraAngle, &NDAlyssaRot[1], NDbVerticalMove, NDAlyssaPos);
            }
            iCameraMode = mode;
            return;
        }
        {
            float side = 0.0f, fwd = 0.0f, up = 0.0f, sx, sy, t0, t1;
            CHARACTER *ch;
            float *pos;

            if ((pad_key_button[0] & SCE_PADR2) && ++NDiChar > CHARACTER_MAX - 1)
                NDiChar = 0;
            sx = StickByte(pad->stick[0]);
            sy = StickByte(pad->stick[1]);
            t0 = TickScale();
            t1 = TickScale();
            NDCameraAngle[1] = (int)((float)NDCameraAngle[1] + (sx - CAMERA_STICK_REST) * 1.5f * t0) & 0xffff;
            NDCameraAngle[0] = (int)((float)NDCameraAngle[0] - (sy - CAMERA_STICK_REST) * 1.5f * t1) & 0xffff;
            if (pad_cnt_button[0] & SCE_PADLright)
                side = PressLerp(NDside_move_table, pad->press[PAD_PRESS_RIGHT]);
            if (pad_cnt_button[0] & SCE_PADLleft)
                side = -PressLerp(NDside_move_table, pad->press[PAD_PRESS_LEFT]);
            if (pad_cnt_button[0] & SCE_PADLup)
                fwd = PressLerp(NDfoword_move_table, pad->press[PAD_PRESS_UP]);
            if (pad_cnt_button[0] & SCE_PADLdown)
                fwd = -PressLerp(NDfoword_move_table, pad->press[PAD_PRESS_DOWN]);
            if (pad_cnt_button[0] & SCE_PADL1)
                up = PressLerp(NDlength_move_table, pad->press[PAD_PRESS_L1]);
            if (pad_cnt_button[0] & SCE_PADL2)
                up = -PressLerp(NDlength_move_table, pad->press[PAD_PRESS_L2]);
            if (pad_cnt_button[0] & SCE_PADRup)
                NDCameraEyePoint += PressLerp(NDlength_move_table, pad->press[PAD_PRESS_TRIANGLE]);
            if (pad_cnt_button[0] & SCE_PADRdown)
                NDCameraEyePoint -= PressLerp(NDlength_move_table, pad->press[PAD_PRESS_CROSS]);
            if (pad_cnt_button[0] & SCE_PADRleft)
                ScaleFollow(&NDCameraBehind, &NDAlissaEyeHeight, 0.95f);
            if (pad_cnt_button[0] & SCE_PADRright)
                ScaleFollow(&NDCameraBehind, &NDAlissaEyeHeight, 1.125f);
            if (NDiChar < 0) {
                iCameraMode = mode;
                return;
            }
            ch = &Character[NDiChar];
            pos = (float *)ch->pos;
            NDAlyssaPos[0] = pos[0];
            NDAlyssaPos[2] = pos[2];
            NDAlyssaPos[3] = 1.0f;
            NDAlyssaPos[1] = pos[1] + NDCameraEyePoint;
            sx = StickByte(pad->stick[2]);
            ch->rot += (int)((sx - CAMERA_STICK_REST) * TickScale()) * 8;
            gaSetRotMatrixY(m, ch->rot);
            // vmulx.xyz / vadd.xyz: the character walks along the turn's x row, then its z row; w stays.
            for (int k = 0; k < 3; k++)
                pos[k] += m[0][k] * side;
            for (int k = 0; k < 3; k++)
                pos[k] += m[2][k] * fwd;
            pos[1] += up;
        }
        iCameraMode = mode;
        return;

    case CAMERA_MODE_TO_SCENE:
        GlobalCameraMode = CAMERA_MODE_SCENE;
        controller_num = 4;
        iSm = 0;
        break;
    case CAMERA_MODE_SCENE:
        break;
    default:
        iCameraMode = mode;
        return;
    }

    cur = GlobalCameraMode;
    if (iCameraMode != GlobalCameraMode)
        asCharacterMortionChangeRequest(0, 0, 0);
    if (controller_mode == CONTROLLER_MODE_100 && BankInfo[idCurrentScene].state == BANK_STATE_LOADED) {
        int cuts = ((const u_char *)BankEePtr(idCurrentScene,
                                              All_table_TBL[idCurrentScene]->ctrl))[CAM_CTRL_CUT_COUNT];
        if (pad_trg_button[0] & SCE_PADL1) {
            iSm++;
            step = 1;
            if (cuts < iSm)
                iSm = 1;
        } else if (pad_trg_button[0] & SCE_PADL2) {
            iSm--;
            step = 1;
            if (iSm < 1)
                iSm = cuts;
        }
    }
    if (step) {
        CAM_CHANGE req = {CAMERA_CUT_STEP_TICKS, idCurrentScene, (short)(iSm - 1), 1, 0};
        CamChange(&req);
        sprintf(szCamModeAdd, s_cameraId, iSm - 1);
    }
    iCameraMode = cur;
}
