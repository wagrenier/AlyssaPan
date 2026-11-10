// game/QuatSlerp.h -- CT3 game/QuatSlerp.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"
#include "ga/gaInitDrawObject.h"

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
// A warp command (0x20 bytes): move a character to a part (a marker) of a scene bank. WarpObject
// (the trigger scripts' warp) queues it at once when the room's bank is loaded, else parks it in
// ReservationWarpCommand until WarpObjectUpdate sees the bank drawn. asWarpObjectBody(WARP_QUEUE, cmd)
// queues it; main runs the queue (WARP_EXECUTE) at a safe point of the frame.
typedef struct WARP_CMD {
    short mode;                         /* 0x00: ExecuteWarpObject's first argument */
    short chara;                        /* 0x02: Character[] index */
    short bank;                         /* 0x04: the scene bank (WarpObject's last argument when no room) */
    short state;                        /* 0x06: ReservationWarpCommand: -1 free, 1 waiting */
    u_long partHash;                    /* 0x08: the target part's name hash (parked warps) */
    int part;                           /* 0x10: the target part's index (-1: by hash) */
    int bank2;                          /* 0x14: the bank ExecuteWarpObject2 searches */
    u_long roomHash;                    /* 0x18: 0: ExecuteWarpObject (indices); else ExecuteWarpObject2 */
} WARP_CMD;
static_assert(sizeof(WARP_CMD) == 0x20, "WARP_CMD");
static_assert(offsetof(WARP_CMD, roomHash) == 0x18, "WARP_CMD.roomHash");
#define WARP_RESERVE_MAX 10
#define WARP_QUEUE 0                    /* asWarpObjectBody: append cmd to the queue */
#define WARP_EXECUTE 1                  /* asWarpObjectBody: run and empty the queue */
#define WARP_QUEUE_GROW 16              /* the queue grows by this many commands (gHeapJunc) */
#define WARP_FOLLOWER_1 11              /* Character[] the player's parked warps take along when flagged */
#define WARP_FOLLOWER_2 12
extern WARP_CMD ReservationWarpCommand[WARP_RESERVE_MAX];                                // CT3 0x019423a0
void asWarpObjectBody(int mode, const WARP_CMD *cmd);                                    // CT3 0x0185b980
void WarpObject(const char *room, int mode, int chara, u_long partHash, int bank);       // CT3 0x0185bb20
void WarpObjectUpdateSet(const WARP_CMD *cmd);                                           // CT3 0x0185bec0
void ExecuteWarpObject(int mode, int chara, int part, int bank);                         // CT3 0x0185b4c0
void ExecuteWarpObject2(int mode, int bank, int chara, int bank2);                       // CT3 0x0185ae90
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
// ---- the game's texture buffers (slice 3, 2026-11-10): sun* are its own copies of the ga library's
// gaSetupTBufferData / gaBGT_* (gaInitDrawObject.h), with four mip levels and a smaller record.
#define SUN_MIP_LEVELS 4

// A texture record (TEX_INFO.tim; layout from sunSetupTBufferData's loads, tex_attr_A$2750 0x01937c80
// is one). layout: ee -- bank data; its address words stay EE addresses.
typedef struct SUN_TEX_ATTR {
    u_int width;                        /* 0x00: pixels */
    u_int height;                       /* 0x04 */
    u_int tw;                           /* 0x08: TEX0.TW (log2 width) */
    u_int th;                           /* 0x0c: TEX0.TH */
    u_int psm;                          /* 0x10: SCE_GS_PSM* */
    int mipMax;                         /* 0x14: the last level to upload; -1 = none */
    u_int pad18;                        /* unread (tex_attr_A: 0x000e0000) */
    u_int mipData[SUN_MIP_LEVELS];      /* 0x1c: EE address of each level's pixels */
    u_int clutCount;                    /* 0x2c */
    u_int clutData;                     /* 0x30: EE address of u_int[clutCount], each a CLUT's EE address */
} SUN_TEX_ATTR;
static_assert(sizeof(SUN_TEX_ATTR) == 0x34, "SUN_TEX_ATTR");

// A texture buffer packet (0xd8; tex_A$2748 has 0x100): TEX0_1, CLAMP_1, MIPTBP1_1, MIPTBP2_1, TEXFLUSH (8
// qwords sunBGT_ExecTBufferData_VIF1 sends), then what sunBGT_LoadGSTexClutData_VIF1 reads.
typedef struct SUN_TBUFFER_DATA {
    GA_PACKET_HEAD head;                /* 0x00 */
    GA_AD tex0;                         /* 0x30 */
    GA_AD clamp;                        /* 0x40 */
    GA_AD miptbp1;                      /* 0x50 */
    GA_AD miptbp2;                      /* 0x60 */
    GA_AD texflush;                     /* 0x70 */
    u_int mipData[SUN_MIP_LEVELS];      /* 0x80: SUN_TEX_ATTR.mipData */
    u_int mipTbp[SUN_MIP_LEVELS];       /* 0x90: GS word address of each level */
    u_int mipTbw[SUN_MIP_LEVELS];       /* 0xa0: each level's buffer width, pixels (level 0: the width) */
    u_int psm;                          /* 0xb0 */
    u_int width;                        /* 0xb4 */
    u_int height;                       /* 0xb8 */
    u_int clutCount;                    /* 0xbc */
    u_int clutData;                     /* 0xc0: SUN_TEX_ATTR.clutData (EE) */
    u_int cbp;                          /* 0xc4: TEX_INFO.cbp: CLUT i sits at cbp + clutWords * i */
    u_int clutWords;                    /* 0xc8: 0x100 (PSMT8 / PSMT8H) or 0x40 */
    u_int clutWidth;                    /* 0xcc: 16 or 8 */
    u_int clutHeight;                   /* 0xd0: 16 or 2 */
    int mipMax;                         /* 0xd4: SUN_TEX_ATTR.mipMax */
} SUN_TBUFFER_DATA;
static_assert(offsetof(SUN_TBUFFER_DATA, mipData) == 0x80, "SUN_TBUFFER_DATA.mipData");
static_assert(offsetof(SUN_TBUFFER_DATA, psm) == 0xb0, "SUN_TBUFFER_DATA.psm");
static_assert(offsetof(SUN_TBUFFER_DATA, cbp) == 0xc4, "SUN_TBUFFER_DATA.cbp");
static_assert(sizeof(SUN_TBUFFER_DATA) == 0xd8, "SUN_TBUFFER_DATA");

void sunBGT_LoadGSTexClutData_VIF1(void *tbuffer);                                      // CT3 0x018590b0
int sunBGT_SetLoadGSEnv(GA_LOADENV *env, u_int dbp, u_int dbw, u_int dpsm, u_int dsax, u_int dsay, int rrw,
                        int rrh);                                                        // CT3 0x01859390
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
// A controller rattle (a struggle: the player shakes the sticks or presses buttons until a count is
// reached). The attacking ghosts / bosses, the moths and the rapid lever open one with
// ControllerClatteringSet, poll it with ControllerClatteringGet, and drop it with ControllerClatteringFree.
typedef struct CLATTERING {             /* 0x20 bytes */
    int timeLeft;                       /* 0x00: frames left; at 0 the rattle is lost */
    int decayFrames;                    /* 0x04: idle frames before count drops by one */
    int w08;                            /* 0x08: set (default 0x70), not read by Update */
    int goal;                           /* 0x0c: the count that wins */
    int state;                          /* 0x10: CLATTERING_* */
    u_char stick[4];                    /* 0x14: the sticks' last position that counted */
    int count;                          /* 0x18: presses and stick swings, less the decay */
    int idle;                           /* 0x1c: frames since the last press or swing */
} CLATTERING;
static_assert(sizeof(CLATTERING) == 0x20, "CLATTERING");
static_assert(offsetof(CLATTERING, stick) == 0x14, "CLATTERING.stick");
#define CLATTERING_MAX 10
#define CLATTERING_FREE -1
#define CLATTERING_RUNNING 0
#define CLATTERING_WON 1                /* count reached goal */
#define CLATTERING_LOST 2               /* timeLeft ran out */
#define CLATTERING_STICK_SWING 0x60     /* a stick byte must move this far to count */
// ControllerClatteringSet's argument: four ints, < 0 takes the default.
typedef struct CLATTERING_SET {
    int time;                           /* default 150 */
    int decayFrames;                    /* default 10 */
    int w08;                            /* default 0x70 */
    int goal;                           /* default 50 */
} CLATTERING_SET;
static_assert(sizeof(CLATTERING_SET) == 0x10, "CLATTERING_SET");
#define CLATTERING_DEF_TIME 150
#define CLATTERING_DEF_DECAY 10
#define CLATTERING_DEF_W08 0x70
#define CLATTERING_DEF_GOAL 50
int ControllerClatteringFree(int slot);                                                  // CT3 0x01852f10
int ControllerClatteringGet(int slot);                                                   // CT3 0x01852f60
int ControllerClatteringSet(const CLATTERING_SET *set);                                  // CT3 0x01853150
int DemoSkipCheckEvent(int event);                                                       // CT3 0x01852010

// Panicdef (0x01940e80, 0x1c bytes): the panic tuning PanicModeInit sets. Fields by offset until the
// panic TU (PanicUpdate) reads them.
typedef struct PANIC_DEF {
    float f00;                          /* 3.5 */
    float f04;                          /* 8.0 */
    float beatFrom;                     /* 0x08: 5000.0: the gauge where PanicSound's heartbeat starts */
    float f0c;                          /* 2.0 */
    int w10;                            /* 30 */
    int w14;                            /* 4 */
    int w18;                            /* 8 */
} PANIC_DEF;
static_assert(sizeof(PANIC_DEF) == 0x1c, "PANIC_DEF");
extern PANIC_DEF Panicdef;                                                               // CT3 0x01940e80
extern int AdvantageousTimeTime;                                                         // CT3 0x01940e44
#define PANIC_TRANSPARENT_TIME 360      /* TransparentBodyTime (MC_LOAD_TRANSPARENT_TIME too) */
#define PANIC_ADVANTAGEOUS_TIME 90
#define TRIG_VAR_PANIC 0x1c             /* PanicModeInit sets it to 0 */
#define TRIG_EVENT_PANIC_3 0x03         /* the three events PanicModeInit removes (named from this use) */
#define TRIG_EVENT_PANIC_26 0x26
#define TRIG_EVENT_PANIC_48 0x48

// The panic sounds (slice 3, 2026-11-10). Named from their use here only; ssGrpPortalCheck.c owns the
// trigger events, the Sd driver the SE numbers (bank slot 4: the system SEs).
#define PANIC_MODE_FULL 0x1             /* PANIC_INFO.mode: the heartbeat at its fastest, FinFin looping */
#define PANIC_BEAT_LEVELS 5             /* the gauge from Panicdef.beatFrom to gaugeMax, in five levels */
#define PANIC_SE_JLS 4                  /* SdReqPlaySE's slot of the panic SEs */
#define PANIC_SE_HEARTBEAT 0x23
#define PANIC_SE_FINFIN 0x24            /* loops while PANIC_MODE_FULL */
#define PANIC_SE_LAVENDER 0x43          /* loops while TRIG_EVENT_LAVENDER */
#define PANIC_SE_INVISIBLE 0x45         /* loops while TRIG_EVENT_INVISIBLE */
#define PANIC_VIBRATION_BASE 10         /* tiVibrationOn's third argument: 10 + 4 * level */
#define PANIC_VIBRATION_STEP 4
#define PANIC_VIBRATION_ARG3 5
#define TRIG_EVENT_LAVENDER 0x1a
#define TRIG_EVENT_INVISIBLE 0x97
#define TRIG_EVENT_PLAYER_STOP 0x90     /* PlayerControlUpdate: registered while the keys are stopped */
// PanicCheck: 1 when one of these events is on (or TRG_CHAR_STOP_CHAR is set) ...
#define TRIG_EVENT_PANIC_ON_4D 0x4d
#define TRIG_EVENT_PANIC_ON_8A 0x8a
#define TRIG_EVENT_PANIC_ON_257 0x257
#define TRIG_EVENT_PANIC_ON_5E0 0x5e0
#define TRIG_EVENT_PANIC_ON_AD 0xad
// ... and -1, whatever else, when one of these is (or a skip runs, or TRIG_VAR_PANIC_OFF >= 0).
#define TRIG_EVENT_PANIC_OFF_63 0x63
#define TRIG_EVENT_PANIC_OFF_14 0x14
#define TRIG_EVENT_PANIC_OFF_07 0x07
#define TRIG_EVENT_PANIC_OFF_08 0x08
#define TRIG_EVENT_PANIC_OFF_19 0x19
#define TRIG_VAR_PANIC_OFF 0x0b
#define PANIC_CHECK_ON 1
#define PANIC_CHECK_OFF (-1)
void EventSoundUpdate(int on);                                                           // CT3 0x01853310
void PanicSound(void);                                                                   // CT3 0x018533b0
void LavenderSound(void);                                                                // CT3 0x01853560
void InvisibleSound(void);                                                               // CT3 0x018535c0
int PanicCheck(void);                                                                    // CT3 0x01854260

// GetAreaNumber: RoomNameID_STR_Tbl / RoomNameID_ID_Tbl (0x019392f0 / 0x01939110) give each room its
// area number (1..52, the save file's area message). A room listed more than once answers its first
// row, its second while TRIG_EVENT_AREA_PASS2 is set, its third while TRIG_EVENT_AREA_PASS3 is.
#define AREA_TBL_COUNT 118
#define TRIG_EVENT_AREA_PASS2 0x5db     /* named from this use */
#define TRIG_EVENT_AREA_PASS3 0x5da
#define AREA_STAGE3_HOME 52             /* stage 3 (TRIG_VAR_STAGE): 0A11 / 0A10 / 0A01 */
#define AREA_STAGE4_0A16 47             /* stage 4: 0A16 */
extern CLATTERING ClatteringData[CLATTERING_MAX];                                        // CT3 0x01942970
