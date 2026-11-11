// game/asEyeForcusUP2CamMat.c -- Clock Tower 3: the camera (EE 01883db0..01889cd0). Each bank has a
// camera (Cam[], one per global bank 0..5 and Cam[6] shared by the scene banks): a cut (CamChange_S) moves
// it, camera_view / camera_main run it each frame, asChangeViewMatrix_forTheBank selects a bank's view.
// Slice 1 (2026-11-10): the view builder, the bank switch, init / destroy, the cut entry; the cut's
// destination search (CamChangeSub, asGetDestination*) and the per-frame camera are still stubs.

#include <float.h>
#include <math.h>
#include <string.h>

#include "common/alyssapan_stub.h"
#include "game/asEyeForcusUP2CamMat.h"
#include "ga/gaInit3dlib.h"
#include "game/QuatSlerp.h"
#include "game/ahRelocateSunPs2ObjectArry.h"
#include "game/fileio.h"
#include "game/main.h"
#include "game/ssGrpPortalCheck.h"
#include "game/unassigned_data.h"
#include "game/wideXcmd_DebugPrint.h"

// ---- data (symtab names; see the header) ---------------------------------------------------------------
float AlissaEyeHeight = CAMERA_EYE_HEIGHT_INIT; /* sdata 0x019386e0 */
float CameraBehind = CAMERA_BEHIND_INIT;    /* sdata 0x019386e4 */
int iLastCameraBank;                        /* sdata 0x019387c8 */
char TheFirstChangeCamera;                  /* sbss 0x0193cf0c */
int iSm;                                    /* sbss 0x0193cefc */
char szCamModeAdd[64];                      /* sbss 0x0193d010 */
alignas(16) float NDAlyssaPos[4];           /* sbss 0x0193d090 */
int iCameraMode;                            /* sbss 0x0193d0a0 */
alignas(16) float CameraAngle[4];           /* sbss 0x0193d0c0 */
float FOV_deg;                              /* sbss 0x01941830 */
float ZelCam[6];                            /* sbss 0x01941880 */
u_char CameGri_Mode;                        /* sbss 0x019418a8 */
int CamStock[6];                            /* sbss 0x019418b0 */
u_char *lpCopyMP[35];                       /* sbss 0x0193cf80 */

#define CAMSTOCK_RESET 2                    /* CamStock[2]: -1 at init and on every cut change */

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
    c->b17f = 1;
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
    c->b181 = rec[CAM_CUT_1B];
    c->eyeKind = rec[CAM_CUT_EYE_KIND];
    if (rec[CAM_CUT_TRG_MODE] == CAM_TRG_MODE_NONE) {
        if (c->eyeKind == CAM_EYE_KIND_1 || c->eyeKind == CAM_EYE_KIND_6)
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
    c->b17a = 0xff;
    c->b17c = 0xff;
    c->changeArg = arg;
    c->w164 = 0;
    c->bank = cam;
    c->cut = cut;
    c->changeMode = mode;
    CamStock[CAMSTOCK_RESET] = -1;
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
        c->b17c = asCheckBankType(i) == 0 ? 4 : 0xff;
        c->b17a = 0xff;
        for (int k = 0; k < 3; k++) {
            float *e = k == 0 ? c->eye : k == 1 ? c->eyeFrom : c->eyeTo;
            float *f = k == 0 ? c->focus : k == 1 ? c->focusFrom : c->focusTo;
            float *u = k == 0 ? c->up : k == 1 ? c->upFrom : c->upTo;
            e[0] = 10.0f, e[1] = 10.0f, e[2] = 10.0f, e[3] = 1.0f;
            f[0] = 0.0f, f[1] = 0.0f, f[2] = 0.0f, f[3] = 1.0f;
            u[0] = 0.0f, u[1] = 1.0f, u[2] = 0.0f, u[3] = 0.0f;
        }
        c->focal[0] = c->focal[1] = c->focal[2] = c->focal[3] = CAMERA_FOCAL_INIT;
        c->b17f = 0;
        c->b182 = 0;
        c->changeArg = 0;
        c->w164 = 0;
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
        c->b181 = 0;
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
    CameraAngle[0] = CameraAngle[1] = CameraAngle[2] = CameraAngle[3] = 0.0f;
    iCameraMode = 0;
    szCamModeAdd[0] = '\0';
    FOV_deg = CAMERA_FOV_DEG_INIT;
    screen_0[SCREEN_PROJ_DIST] = asFocalLength_LEICA2PS2(CAMERA_FOV_DEG_INIT);
    NDAlyssaPos[0] = NDAlyssaPos[1] = NDAlyssaPos[2] = NDAlyssaPos[3] = 0.0f;
    CameGri_Mode = 0xff;
    iSm = 0;
    camera_init0(-1, 0);
    CamStock[CAMSTOCK_RESET] = -1;
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018881a0
void camera_main(PAD_DATA *pad, int mode)
{
    ALYSSAPAN_STUB("camera_main", 0x018881a0);
}

// stub of CT3 0x018875d0
void camera_view(void)
{
    ALYSSAPAN_STUB("camera_view", 0x018875d0);
}


// stub of CT3 0x01884f50
int asGetDestinationEYE(int kind, float *eye, float *focus, float *up, float *focal, int id)
{
    ALYSSAPAN_STUB("asGetDestinationEYE", 0x01884f50);
    return {};
}
