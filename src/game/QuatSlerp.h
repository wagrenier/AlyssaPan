// game/QuatSlerp.h -- CT3 game/QuatSlerp.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

void ControllerClatteringUpdate(void);                                                   // CT3 0x01852f90
void ControllerClatteringInit(void);                                                     // CT3 0x01853220
void PanicFilter(void);                                                                  // CT3 0x01853620
void PanicUpdate(void);                                                                  // CT3 0x018537b0
void PanicModeInit(void);                                                                // CT3 0x01854410
void ssMenuUpdate(int mode);                                                             // CT3 0x01854d30
void MonsterAIUpdate(int mode);                                                          // CT3 0x01854d60
void PlayerControlUpdate(int mode);                                                      // CT3 0x018550b0
void FogAnimeUpdate(void);                                                               // CT3 0x01856660
void AmbientAlphaUpdate(void);                                                           // CT3 0x01856c30
void XcmdScreenFilterSkip(void);                                                         // CT3 0x01858bc0
void XcmdScreenFilter(void);                                                             // CT3 0x01858c90
void VistaVisionMain(void);                                                              // CT3 0x0185aa50
void asWarpObjectBody(int a0, int a1);                                                   // CT3 0x0185b980
void WarpObjectUpdate(void);                                                             // CT3 0x0185bd50
void WarpObjectUpdateInit(void);                                                         // CT3 0x0185bf80
void asWarpSceneObject(void *table, int maxId, float *pos);                              // CT3 0x0185c100
// The packed keys: three shorts scaled by 1 / (1 << shift), w = 1; four shorts scaled by 1 / 16384.
void asZipTrans2Trans_XYZ1(float *out, const short *in, u_int shift);                     // CT3 0x0185ceb0
void asZipQuat2Quat(float *out, const short *in);                                        // CT3 0x0185c7d0
// out = the three shorts as the high halves of floats (bfloat16), w 1.
void asZipVECTOR2VECTOR_XYZ1(float *out, const short *in);                               // CT3 0x0185d0d0
// out = a w[0] + b w[1] (all four lanes); a when a == b (the VU's zero flags).
void asInterpolateVector(float *out, const float *a, const float *b, const float *w);    // CT3 0x0185d330
// ---- the packed keys' other forms (written early from the disassembly, ahRelocateSunPs2ObjectArry.c
// slice 9, 2026-11-10). w is the pair {1 - t, t}. ----
#define ZIP_QUAT_SCALE 16384.0f         /* a packed quaternion's unit (0x46800000; 2^-14 back) */
#define ZIP_UV_SCALE 1024.0f            /* a packed UV's unit (0x44800000) */
void asMemCopy128c(void *dst, const void *src, u_int n);                                 // CT3 0x0185c4d0 (n qwords)
void asMemCopy16c(void *dst, const void *src, u_int n);                                  // CT3 0x0185c500 (n halfwords)
void asQuat2ZipQuat(short *out, const float *q);                                         // CT3 0x0185c770
void asQuatSlerpZipVECTOR(short *out, const short *a, const short *b, float *w);         // CT3 0x0185c700
void asTrans2ZipTrans_XYZ(short *out, const float *v, u_int shift);                      // CT3 0x0185ce40
void asInterpolateZipTrans_XYZ(short *out, const short *a, const short *b, const float *w, u_int shift); // CT3 0x0185cdb0
void asZipVECTOR2VECTOR_XYZ0(float *out, const short *in);                               // CT3 0x0185d100
void asVECTOR2ZipVECTOR_XYZ(short *out, const float *v);                                 // CT3 0x0185d190
void asInterpolateZipVECTOR_XYZ(short *out, const short *a, const short *b, const float *w); // CT3 0x0185d120
float asInterpolateNumber(float a, float b, const float *w);                             // CT3 0x0185d1b0
// out = u, v (the first two shorts / ZIP_UV_SCALE), then the next two shorts as floats.
void asZipUV2UV(float *out, const short *in);                                            // CT3 0x0185d060
// Still stubs (prototypes from InterpolateAnimationKey's calls): the UV and the two float formats
// blend one packed key; the Cont* forms blend `n` packed vertices (a sub-record's +0xc count) at the
// host pointers the sub-records' EE words resolve to.
void asInterpolateZipUV(short *out, const short *a, const short *b, const float *w);     // CT3 0x0185cfb0
void asInterpolateZipFloat0_7_9X(short *out, const short *a, const short *b, const float *w); // CT3 0x0185c530
void asInterpolateZipFloat1_1_14X(short *out, const short *a, const short *b, const float *w); // CT3 0x0185c630
#define ZIP_FLOAT_1_1_14_SCALE 16384.0f /* 0x46800000 */
void asZipFloat1_1_14X2Float(float *out, const short *in);                               // CT3 0x0185c6d0
void asFloat2ZipFloat1_1_14X(short *out, const float *in);                               // CT3 0x0185c6a0
void asUV2ZipUV_R(short *out, const float *uv);                                          // CT3 0x0185d020: stores the floats
void asContInterpolateZipNormal_XYZ(short *out, const short *a, const short *b, const float *w, int n); // CT3 0x0185c840
void asContInterpolateZipTrans_XYZ(short *out, const short *a, const short *b, const float *w, int n,
                                   u_int shift);                                         // CT3 0x0185ca50
void asContInterpolateVector(float *out, const float *a, const float *b, const float *w, int n); // CT3 0x0185d2a0
// ---- part lookup and name hashes (slice 1, 2026-11-10) ----
#define HASH_CHAR_BITS 7                /* ahGetHash: each character's bits, rotated in 7 apart */
#define HASH_CHAR_MASK 0x7f
// The parts header (pModelPartsHeader[bank]): records of PARTS_REC_SIZE from +0x40.
#define PARTS_HDR_COUNT 0x02            /* u_short: the number of parts */
#define PARTS_HDR_CACHE 0x10            /* int[4]: asFindModelPartsID's cached indices (0 unknown, -1 none) */
#define PARTS_HDR_RECORDS 0x40          /* the first record */
#define PARTS_REC_SIZE 0x380
#define PARTS_REC_HASH 0x08             /* u_long: the part's name hash (MODEL_PARTS.hash from the header) */
// The four names with a cache word; the first two are their names read back by asConvertHash2String,
// the last two are longer than 9 characters (their ends read "...sa_ta" / "...sa_se").
#define PARTS_HASH_BIP01 0x3161c34c2ull           /* "Bip01" (sound.h's PARTS_HASH_SOUND_ORIGIN) */
#define PARTS_HASH_BASE_POINT 0x74dda77d0cbcf0c2ull /* "BasePoint" (EA_001_PART_HASH, SP_ANCHOR_HASH) */
#define PARTS_HASH_CACHE2 0x61e97f0f3932cf98ull
#define PARTS_HASH_CACHE3 0xe5e77f0f38f20ad1ull
#define GLOBAL_BANK_LAST 5              /* banks 0..5 are global; 6.. scene and character banks */
int asFindModelPartsID(u_char *header, u_long hash);                                    // CT3 0x0185e150
char *asConvertHash2String(u_long hash);                                                 // CT3 0x0185e420
void *asFindCharacterStruct(void *chara);                                                // CT3 0x0185deb0
int asCheckBankID(int bank);                                                             // CT3 0x0185e360
int asCheckBankType(int bank);                                                           // CT3 0x0185e380
u_long ahGetHash(const char *name);                                                      // CT3 0x0185e580
void CameraMatrix2CameraTrg3m(float *trg);                                              // CT3 0x0185a5e0
void CameraMatrix2CameraPos(float *pos);                                                 // CT3 0x0185a650
float asVectorDistance(const float *a, const float *b);                                  // CT3 0x0185ded0
void tnInit3dlibWork(void *work, void *spr);                                            // CT3 0x01854560
int OneMaxObjectDrawZOnly(int bank, int index);                                          // CT3 0x01858f20
void sunBGT_LoadGSTexClutData_VIF1(void *tbuffer);                                      // CT3 0x018590b0
void sunBGT_ExecTBufferData_VIF1(void *tbuffer);                                        // CT3 0x01859520
void sunSetupTBufferData(void *tbuffer, TEX_INFO tex);                                   // CT3 0x01859530 (tex by value, 0x2c bytes)
void TexFileSetInit(int scene);                                                          // CT3 0x01856070
void AmbientAlphaInit(int scene);                                                        // CT3 0x01856e70
// The camera read back from a view matrix (slice 1, 2026-11-10). Any output may be NULL.
#define FOCAL_LEICA_MM 4.3266616f       /* 0x408a7403 */
#define FOCAL_PS2_UNITS 680.32935f      /* 0x442a1514 */
#define SCREEN_PROJ_DIST 8              /* screen_0[8]: the projection distance (512) */
#define CAMERA_EYE_FOCALS 3.0f          /* the eye sits this many focal lengths behind the view origin */
#define CAMERA_TARGET_DIST 300.0f       /* CameraMatrix2CameraTrg3m: 3 m ahead */
float asFocalLength_PS22LEICA(float f);                                                  // CT3 0x0185a670
float asFocalLength_LEICA2PS2(float f);                                                  // CT3 0x0185a6b0
void asViewMatrix2PosRUF(float *view, float *pos, float *right, float *up, float *front); // CT3 0x0185a8a0
void asViewMatrix2CamRUF(float (*view)[4], float *pos, float *right, float *up, float *forward); // CT3 0x0185a6f0
void EventSoundEnd(void);                                                                // CT3 0x018532a0
// From data_save_set / data_load_set (Character_Init.c, 2026-11-10).
int GetAreaNumber(int a0);                                                               // CT3 0x01852240
int BankID_TO_CharID(short bank);                                                        // CT3 0x0185a400
int CharID_TO_BankID(int chara);                                                         // CT3 0x0185a480
#define ROOM_NAME_LEN 4                 /* GetRoomName_file: a room is its files' first four characters */
u_long GetRoomName_file(const char *name);                                               // CT3 0x0185c2e0: ahGetHash of the first 4 chars
// From game/main2.c's calls (2026-11-10). asJointParts / _byMatrix answer 1 or 2 when no joint portal
// part is found; _byMatrix also reads DrawOnPosSetHashTemp.
u_char *asFindModelParts(u_char *header, u_long hash);                                   // CT3 0x0185e100
// From asSeekBarAnimation (2026-11-10): out = where the line a..b (direction dir, unit) crosses the
// plane given by a part's position qword (its use there; the function is unread).
void asGetCrossPointInPlaneWithLine(float out[4], float a[4], float b[4], float dir[4], float plane[4]); // CT3 0x01859f00
int asJointParts(int srcBank, int bank, u_long hash);                                    // CT3 0x01854b20
int asJointParts_byMatrix(float (*mat)[4], int *pos, int bank);                          // CT3 0x01854750
// From BuildMatrixTree's calls (ahRelocateSunPs2ObjectArry.c slice 4, 2026-11-10); confirm when written.
int absi(int v);                                                                         // CT3 0x0185e040
// *abs = |v|, *sign = -1, 0 or 1.
void AbsSignI32(int v, int *abs, int *sign);                                            // CT3 0x0185e390
// The game's angles are u_short turns (asSearchObject's cone and step); from asSearchObject's calls.
#define ANGLE_QUARTER_TURN 0x4000
#define ANGLE_HALF_TURN 0x8000
#define ANGLE_FULL_TURN_F 65536.0f
#define AH_TWO_PI 6.2831855f            /* 0x40c90fdb */
#define AH_ACOS_STEPS 4096.0f           /* ah_acos_table's steps over 0..1 */
#define AH_ACOS_TABLE_SIZE 4098
#define AH_ACOS_NEAR_ONE 0.9988f        /* 0x3f7fb15b: acosf from here on */
float ahCos(int angle);                                                                  // CT3 0x01851d60
float ahSin(int angle);                                                                  // CT3 0x01851d90
int ahAcos(float c);                                                                     // CT3 0x01851dc0
// m = the rotation by angle about the unit axis (asSearchObject's turn step).
void asMakeRotMatrixByQuartanion(float (*m)[4], float *axis, int angle);                // CT3 0x0185ddf0
// |v.xyz| (the ROM: 1 / rsqrt; written early, ahRelocateSunPs2ObjectArry.c slice 13).
float asVectorLength3D(const float *v);                                                  // CT3 0x0185df90
// out = m turned to look at target; `axis` is the part's +0x34 byte.
void asMakeWatchMeMatrixToPosition(float (*out)[4], float (*m)[4], float *target, int axis); // CT3 0x0185d4d0
// a point light part's range (its record, MODEL_PARTS + 0x40).
void OmniRangeChange(u_char *rec);                                                       // CT3 0x018520b0
// q = the slerp from q0 to q1; ts = {1 - t, t} (asMakeStraw passes the pair it also gives
// asGetBeziePosition).
void QuatSlerp(float *q, float *q0, float *q1, float *ts);                               // CT3 0x01851bf0
// out = b with its twist about a's Y axis taken out; answers the twist (asMakeStraw negates it and
// spreads it over the vertices through asMakeRotMatrixByQuartanion).
int asDelSqueezeInformation(float (*out)[4], float (*a)[4], float (*b)[4]);              // CT3 0x0185d380
// The vector's angle (radians) for the rotation about one axis (asMakeMirrorMatrixByPlane: 1 for Y,
// then 0 for X); the TU that writes it confirms the axis meaning.
float asRadianByO(float *v, int axis);                                                   // CT3 0x0185dc30
// GS register words for the shadow pass's own packets (written early, ahRelocateSunPs2ObjectArry.c slice 14):
// RGBAQ (q as a float), UV (u low, v high, 1/16 texel), XYZ2 / XYZ3 (screen pixels + 0x800, * 16).
u_long asDR_RGBAQ(u_char r, u_char g, u_char b, u_char a, float q);                     // CT3 0x0185c450
u_int asDR_UV(u_short u, u_short v);                                                     // CT3 0x0185c3a0
u_long asDR_XYZ2(short x, short y, u_short z);                                           // CT3 0x0185c410
u_long asDR_XYZ3(short x, short y, u_short z);                                           // CT3 0x0185c3d0
// EA_010's packed-key writers (prototypes from the disassembly, ahRelocateSunPs2ObjectArry.c slice 14).
// The _nw forms skip an element whose first short is `skip` (packed) or whose w is skipW (floats).
void asZipFloat0_7_9X2Float(float *out, const u_short *in);                              // CT3 0x0185c5d0
void asFloat2ZipFloat0_7_9X(short *out, const float *in);                                 // CT3 0x0185c5a0
void asContZipNormal2Normal_XYZ0(float *out, const short *in, int n);                    // CT3 0x0185c9e0
void asContZipTrans2Trans_XYZ1(float *out, const short *in, int n, u_int shift);          // CT3 0x0185cd30
void asContZipTrans2Trans_XYZ1_nw(float *out, const short *in, int n, u_int shift, short skip); // CT3 0x0185cc90
void asMemCopy128c_nw(float skipW, void *dst, const void *src, int n);                   // CT3 0x0185c480
void asContInterpolateVector_nw(float skipW, float *out, const float *a, const float *b, const float *w,
                                int n);                                                  // CT3 0x0185d1f0
void asContInterpolateGA_VECTORZipTrans_XYZ1(float *out, const float *a, const short *b, const float *w,
                                             int n, u_int shift);                        // CT3 0x0185cbe0
void asContInterpolateGA_VECTORZipTrans_XYZ1_nw(float *out, const float *a, const short *b, const float *w,
                                                int n, u_int shift, short skip);         // CT3 0x0185cb20
void asContInterpolateGA_VECTORZipNormal_XYZ0(float *out, const float *a, const short *b, const float *w,
                                              int n);                                    // CT3 0x0185c920
#define ZIP_NORMAL_SHIFT 14             /* packed normals: shorts / 16384 */
void asZipTrans2Trans_XYZ0(float *out, const short *in, u_int shift);                     // CT3 0x0185cf30

// ---- slice 1 (2026-11-10): rooms, point lights, the alpha animation, the controller rattles ----
#define ROOM_TBL_COUNT 95               /* RoomNameTbl's rows; RoomIdTbl ends with -1 after them */
void RoomFlagClear(void);                                                                // CT3 0x01852130
void BRANCH_CLEAR(void);                                                                 // CT3 0x01851f00
void RoomInFlagOn(void);                                                                 // CT3 0x01852180
// A point light's record (PARTS_KIND_OMNI; record offsets, OmniRangeChange).
#define REC_OMNI_CHANGING 0x30          /* int: != 0 while the range changes */
#define REC_OMNI_STEPS 0x34             /* int: the steps left */
#define REC_OMNI_RANGE 0x38             /* float: the range it goes to */
#define REC_OMNI_INV_RANGE 0x2ec        /* float: 1 / the current range */
#define PARTS_KIND_END -1               /* MODEL_PARTS.kind (s_char) of the record after the last */
typedef struct SCENE_ALPHA_ANIM {       /* 0x14 bytes: AmbientAlphaInit zeroes it, AmbientAlphaUpdate reads */
    int w[5];
} SCENE_ALPHA_ANIM;
static_assert(sizeof(SCENE_ALPHA_ANIM) == 0x14, "SCENE_ALPHA_ANIM");
extern SCENE_ALPHA_ANIM scene_alpha_anim[BANK_MAX];                                      // CT3 0x01a4b900
typedef struct CLATTERING {             /* 0x20 bytes: one controller rattle */
    u_char pad00[0x10];
    int id;                             /* 0x10: CLATTERING_FREE when unused */
    u_char pad14[0xc];
} CLATTERING;
static_assert(sizeof(CLATTERING) == 0x20, "CLATTERING");
#define CLATTERING_MAX 10
#define CLATTERING_FREE -1
extern CLATTERING ClatteringData[CLATTERING_MAX];                                        // CT3 0x01942970
