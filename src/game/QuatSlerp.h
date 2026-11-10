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
void PlayerControlUpdate2(int mode);                                                     // CT3 0x01854d00
// GAME_OVER_END's steps (xcmd_tn.c's script command picks one).
#define GAME_OVER_STING 0               /* the music stops, the game-over SE plays, the Sp1 SEs stop */
#define GAME_OVER_FREEZE 2              /* player, menu and monsters frozen, characters freed, banks 6.. undrawn */
#define GAME_OVER_STOP_SE 3             /* SdStopSEAll */
#define GAME_OVER_SE_JLS 4              /* SdReqPlaySE's slot */
#define GAME_OVER_SE 0x3b               /* the sting's SE number */
void GAME_OVER_END(int step);                                                            // CT3 0x01851f10
void MonsterAIUpdate(int mode);                                                          // CT3 0x01854d60
void PlayerControlUpdate(int mode);                                                      // CT3 0x018550b0
void FogAnimeUpdate(void);                                                               // CT3 0x01856660
void AmbientAlphaUpdate(void);                                                           // CT3 0x01856c30
void XcmdScreenFilterSkip(void);                                                         // CT3 0x01858bc0
void XcmdScreenFilter(void);                                                             // CT3 0x01858c90
void VistaVisionMain(void);                                                              // CT3 0x0185aa50
// The letterbox ("VistaVision": black bars sliding in from the top and bottom for a scene). The script
// sets VistaVisionNumber to a VISTA_* request; VistaVisionMain runs it and leaves VISTA_SHOWN or VISTA_OFF.
#define VISTA_OFF 0
#define VISTA_SHOW_NOW 1                /* bars at once */
#define VISTA_HIDE_NOW 2                /* bars gone at once (TRIG_EVENT_VISTA_KEEP keeps the ON event) */
#define VISTA_SHOW 3                    /* bars slide in, VISTA_STEP a frame */
#define VISTA_HIDE 4                    /* bars slide out */
#define VISTA_SHOWN 100                 /* bars fully in (drawn every frame) */
#define VISTA_COUNTER_MAX 100.0f        /* VistaVisionCounter: 0..100 percent of the bars */
#define VISTA_STEP 8.0f
#define TRIG_EVENT_VISTA_ON 0x15        /* on while bars show (removed when they are gone) */
#define TRIG_EVENT_VISTA_KEEP 0x16      /* VISTA_HIDE_NOW leaves TRIG_EVENT_VISTA_ON set */
#define TRIG_EVENT_VISTA_SHOWN 0xab     /* registered when the bars are fully in */
#define TRIG_EVENT_VISTA_HIDDEN 0xac    /* registered when they are fully out */
extern float VistaVisionHeightSize;     /* sbss 0x0193fc88: each bar's full height (frame lines of 448) */
extern float VistaVisionCounter;        /* sbss 0x0193fc8c: percent of the height drawn */
// VistaVisionNumber (sbss 0x0193fc90, unassigned_data.h): VISTA_*
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
    u_long roomHash;                    /* 0x18: 0: ExecuteWarpObject (indices); else ExecuteWarpObject2's
                                           marker part hash in bank2 (the ROM passes it in t0) */
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
void ExecuteWarpObject2(int mode, int bank, int chara, int bank2, u_long partHash);      // CT3 0x0185ae90
void WarpObjectUpdate(void);                                                             // CT3 0x0185bd50
void WarpObjectUpdateInit(void);                                                         // CT3 0x0185bf80
void asWarpSceneObject(ALL_TABLE *table, int part, float *pos);                         // CT3 0x0185c100
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
float asVectorLength(const float *v);                                                     // CT3 0x0185dec0
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
int asJointParts_byMatrix(float (*mat)[4], float *pos, int bank, u_long hash);           // CT3 0x01854750
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
// The vector's angle (0..2pi) in the plane about one axis (written 2026-11-10, slice 6).
#define RADIAN_AXIS_X 0                 /* the angle of (y, z) */
#define RADIAN_AXIS_Y 1                 /* the angle of (z, x) */
#define RADIAN_AXIS_Z 2                 /* the angle of (x, y) */
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
// A bank's ambient animation (0x14 bytes: AmbientAlphaInit zeroes it, AmbientAlphaUpdate runs it): every
// part's ALL_TABLE.partsPos xyz moves 1/steps of the way to target each frame (at steps 0: set to it).
typedef struct SCENE_ALPHA_ANIM {
    int on;                             /* 0x00: != 0: the animation runs */
    int steps;                          /* 0x04: frames left (counts down) */
    float target[3];                    /* 0x08: the xyz it goes to (the ROM packs the words as they are) */
} SCENE_ALPHA_ANIM;
static_assert(offsetof(SCENE_ALPHA_ANIM, target) == 0x08, "SCENE_ALPHA_ANIM.target");
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
void DemoSkipRegisterEvent(int event);                                                   // CT3 0x01852030
#define SKIP_EVENT_END (-1)             /* SkipFlagTbl's last entry (QuatSlerp.c's static table) */
int DemoSkipGetEvent(int event);                                                         // CT3 0x01852060
// The stalkers' slots of Character[] (CheckSameRoomAsMonster reads 1..6; ssAtkCharaList2 lists them).
#define MONSTER_SLOT_FIRST 1
#define MONSTER_SLOT_COUNT 6
int CheckSameRoomAsMonster(CHARACTER *c);                                                // CT3 0x01852d30
void PanicDownSet(int item);                                                             // CT3 0x018540b0

// Panicdef (0x01940e80, 0x1c bytes): the panic tuning PanicModeInit sets (PanicUpdate named the falls).
typedef struct PANIC_DEF {
    float calmFall;                     /* 0x00: 3.5: the gauge's fall per frame with nothing rising */
    float fullFall;                     /* 0x04: 8.0: its fall per frame in full panic */
    float beatFrom;                     /* 0x08: 5000.0: the gauge where PanicSound's heartbeat starts;
                                           full panic ends when the gauge falls to it */
    float f0c;                          /* 2.0 */
    int filterMax;                      /* 0x10: 30: PanicFilter's vignette alpha at its top */
    int filterUp;                       /* 0x14: 4: added each frame while PANIC_MODE_FULL */
    int filterDown;                     /* 0x18: 8: taken each frame otherwise */
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
// PanicUpdate (slice 6, 2026-11-10): the events and variables it reads, named from this use.
#define TRIG_EVENT_PANIC_HOLD 0x00      /* set: the gauge does not move this frame */
#define TRIG_EVENT_PANIC_BEAT 0x02      /* registered while full panic is above beatFrom */
#define TRIG_EVENT_PANIC_FULL 0x03      /* registered for a full panic (TRIG_EVENT_PANIC_3) */
#define TRIG_EVENT_PANIC_HIDING 0x04    /* the player hides: threat rises scale by HidePointUpValue */
#define TRIG_EVENT_PANIC_KEEP_MONSTERS 0x36 /* set: full panic leaves the stalkers running */
#define TRIG_EVENT_PANIC_KEEP_PLAYER 0x37 /* set: ... the player's keys */
#define TRIG_EVENT_PANIC_KEEP_MENU 0x38 /* set: ... the menu */
#define TRIG_EVENT_PANIC_ATTACKED 0x3a  /* registered when an attack rise counts */
#define TRIG_EVENT_PANIC_VIBRATE 0x54   /* tiVibrationOn's first motor argument at full panic */
#define TRIG_EVENT_PANIC_NO_RELOAD 0xcc /* unset: full panic registers TRIG_EVENT_MC_RELOADED (sic: 0xca) */
#define TRIG_EVENT_PANIC_STARTED 0xd3   /* registered when full panic starts */
#define TRIG_VAR_PANIC_SUSPEND 0x55     /* > 0: no panic (the invisible / advantage timers reset) */
#define TRIG_VAR_HIDE_POINT 0x28        /* the hiding place's level: HidePointUpValue[level + 1] */
#define TRIG_VAR_PANIC_VIBRATE 0x49     /* tiVibrationOn's second motor argument */
#define PANIC_SE_FULL 9                 /* SdReqPlaySE (slot PANIC_SE_JLS) when full panic starts */
#define PANIC_VIBRATE_TIME 30
#define PANIC_GAUGE_CAP_MARGIN 100.0f   /* a rise outside full panic stops this far below gaugeMax */
#define PANIC_GAUGE_UNIT 100.0f         /* TRIG_VAR_PANIC = gauge / 100 */
#define PANIC_RECOVERY_COUNT 5
#define PANIC_SAFE_BANK "MCLS.BIN"      /* @3854: with it in bank 0 the gauge does not move */
// PanicRecoveryTbl (data 0x019390b0): the calming items; the user sets PANIC_INFO.recoveryItem / Time.
typedef struct PANIC_RECOVERY {
    float calmFall;                     /* 0x00: the gauge's extra fall per frame outside full panic */
    float fullFall;                     /* 0x04: ... in full panic (instead of Panicdef.fullFall) */
    int frames;                         /* 0x08: how long it works (read by the item's TU) */
} PANIC_RECOVERY;
static_assert(sizeof(PANIC_RECOVERY) == 0xc, "PANIC_RECOVERY");
extern PANIC_RECOVERY PanicRecoveryTbl[PANIC_RECOVERY_COUNT];                            // CT3 0x019390b0
#define HIDE_POINT_LEVELS 5
extern float HidePointUpValue[HIDE_POINT_LEVELS];                                        // CT3 0x019390f0
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

// ---- the screen filters (slice 4) -----------------------------------------------------------------
// A screen filter's packet (ScreenFilterObj, XcmdScreenFilterObj, DemoScreenFilterObj: 0xa0 bytes of
// sdata, the same bytes each): DMAtag cnt 9, DIRECT 8, a GIFtag (REGLIST, 14 registers: PRIM, then
// RGBAQ / XYZ2 pairs, XYZ3) and a TRIFAN over the 512x448 frame (GS 0x8000..0xa000, 0x8000..0x9c00)
// from its centre. The filters write only the RGBAQs.
#define SCREEN_FILTER_VERTICES 6
typedef struct SCREEN_FILTER_VTX {
    u_long rgbaq;
    u_long xyz2;
} SCREEN_FILTER_VTX;
static_assert(sizeof(SCREEN_FILTER_VTX) == 0x10, "SCREEN_FILTER_VTX");
typedef struct SCREEN_FILTER_OBJ {
    u_char head[0x38];                  /* 0x00: DMAtag, VIF codes, GIFtag, PRIM (TRIFAN) */
    SCREEN_FILTER_VTX vtx[SCREEN_FILTER_VERTICES]; /* 0x38: the centre, then the corners round to the first */
    u_long xyz3;                        /* 0x98: the centre again, no kick */
} SCREEN_FILTER_OBJ;
static_assert(sizeof(SCREEN_FILTER_OBJ) == 0xa0, "SCREEN_FILTER_OBJ");
static_assert(offsetof(SCREEN_FILTER_OBJ, vtx) == 0x38, "SCREEN_FILTER_OBJ.vtx");
#define SCREEN_FILTER_RGBAQ_INIT 0x3f800000ff208080ull
#define SCREEN_FILTER_OBJ_INIT {                                                                      \
    {0x09, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x50, \
     0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe4, 0x10, 0x15, 0x15, 0x15, 0x15, 0x15, 0xd5, 0x00, \
     0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},                                               \
    {{0x3f80000000208080ull, 0x8e009000ull},                                                        \
     {SCREEN_FILTER_RGBAQ_INIT, 0x80008000ull},                                                     \
     {SCREEN_FILTER_RGBAQ_INIT, 0x8000a000ull},                                                     \
     {SCREEN_FILTER_RGBAQ_INIT, 0x9c00a000ull},                                                     \
     {SCREEN_FILTER_RGBAQ_INIT, 0x9c008000ull},                                                     \
     {SCREEN_FILTER_RGBAQ_INIT, 0x80008000ull}},                                                    \
    0x8e009000ull}
void DemoScreenFilter(void);                                                             // CT3 0x01858950

// ---- banks by name, the slide decoder (slice 4) ---------------------------------------------------
int ssGetBankID(const char *name);                                                       // CT3 0x0185c190
int ssStrCmp4Byte(const char *a, const char *b);                                         // CT3 0x0185c220
// A slide file is LZSS (Okumura's: a 4096-byte ring, 18-byte matches, threshold 2) after an 8-byte
// header whose first word is the file's size: decoding stops at that byte.
#define SLIDE_RING_SIZE 4096
#define SLIDE_MATCH_MAX 18
#define SLIDE_THRESHOLD 2
#define SLIDE_HEADER_SIZE 8
void tnSlideThawing(const u_char *src, u_char *dst);                                     // CT3 0x0185e6a0
void decode(const u_char *src, u_char *dst);                                             // CT3 0x0185e6d0
void tn_putc(u_char *dst, u_char c);                                                     // CT3 0x0185e8b0
int tn_getc(const u_char *src);                                                          // CT3 0x0185e8d0
// ---- the distance and angle fades (slice 5, 2026-11-10) -------------------------------------------
// A bank lists parts that fade by the distance (ALL_TABLE.distAtten) or the angle (.angleAtten) between
// an eye (the shared camera, live characters, or the bank's own parts: ATTEN_EYE_*) and a subject
// (parts of live characters, or the bank's own parts: ATTEN_SUBJ_*). For each eye and subject the
// part's distAlpha / angleAlpha becomes minus max * (v - start) / (full - start), clamped to 0..max
// (the last pair wins); the part's ALL_TABLE.partsPos w is then lowered to it. The distance is
// asVectorDistance's, squared; the angle ahAcos's, of the eye's facing and the line to the subject.
// Every list is relocated bank data (BankEePtr) ended by -1; a 0 word means "all" (cuts: any cut).
#define ATTEN_EYE_CAMERA 0x1            /* the shared camera (Cam[CAMERA_SHARED]), in the listed cuts */
#define ATTEN_EYE_CHARA 0x2             /* the listed characters (all live ones if none listed) */
#define ATTEN_EYE_PARTS 0x4             /* the bank's listed parts (distance only) */
#define ATTEN_SUBJ_CHARA 0x10000        /* the listed characters' parts (by hash) */
#define ATTEN_SUBJ_PARTS 0x20000        /* the bank's listed parts */
#define CAMERA_SHARED 6                 /* Cam[6]: the camera every bank shares */
#define CAMERA_EYE_POS 0xc0             /* CAMERA: the qword the fades take as the camera's place */
#define ATTEN_CHARA_LIVE (CHARACTER_FLAG_REFRACTION | CHARACTER_FLAG_SHADOW) /* both bits: a character
                                           the fades see (the same test as the refraction pass) */
// A character in an eye or subject list (0x10 bytes).
typedef struct ATTEN_CHARA {
    int chara;                          /* 0x00: Character[] index, -1 ends the list */
    int pad04;
    u_long hash;                        /* 0x08: the part (asFindModelParts) */
} ATTEN_CHARA;
static_assert(sizeof(ATTEN_CHARA) == 0x10, "ATTEN_CHARA");
// One faded part of ALL_TABLE.distAtten (0x28 bytes, ended by part -1). layout: ee.
typedef struct DIST_ATTEN {
    int part;                           /* 0x00: the faded part's index */
    float full;                         /* 0x04: the squared distance of the full fade (max) */
    float start;                        /* 0x08: the squared distance the fade starts at (0) */
    float max;                          /* 0x0c */
    u_int flags;                        /* 0x10: ATTEN_EYE_* | ATTEN_SUBJ_* */
    u_int cuts;                         /* 0x14: int[]: Cam[CAMERA_SHARED].cut values, 0 = any */
    u_int eyeCharas;                    /* 0x18: ATTEN_CHARA[], 0 = every live character (see the code) */
    u_int eyeParts;                     /* 0x1c: int[] of the bank's parts, 0 = all */
    u_int subjCharas;                   /* 0x20: ATTEN_CHARA[] */
    u_int subjParts;                    /* 0x24: int[] of the bank's parts */
} DIST_ATTEN;
static_assert(sizeof(DIST_ATTEN) == 0x28, "DIST_ATTEN");
// One faded part of ALL_TABLE.angleAtten (0x24 bytes, ended by part -1). layout: ee.
typedef struct ANGLE_ATTEN {
    int part;                           /* 0x00 */
    int full;                           /* 0x04: the angle (ahAcos units) of the full fade */
    int start;                          /* 0x08: the angle the fade starts at */
    float max;                          /* 0x0c */
    u_int flags;                        /* 0x10: ATTEN_EYE_CAMERA / _CHARA | ATTEN_SUBJ_* */
    u_int cuts;                         /* 0x14 */
    u_int eyeCharas;                    /* 0x18: ATTEN_CHARA[] (only .chara read), 0 = every live one */
    u_int subjCharas;                   /* 0x1c: ATTEN_CHARA[] */
    u_int subjParts;                    /* 0x20: int[] */
} ANGLE_ATTEN;
static_assert(sizeof(ANGLE_ATTEN) == 0x24, "ANGLE_ATTEN");
// One faded part of ALL_TABLE.losFade (0x28 bytes, ended by part -1). layout: ee. The part fades out
// (MODEL_PARTS.losAlpha down to -floor, 255 / frames a frame) while it blocks the line from an eye to a
// subject (CheckLOS on any drawn scene bank; with LOS_FADE_BY_ID, CheckLOS_Multi on the part's own bank
// and one of hitIds among the hits), and back in (up to MODEL_PARTS.alpha) when no line is blocked.
#define LOS_FADE_BY_ID 0x80000000u      /* LOS_FADE.flags: CheckLOS_Multi, hits matched against hitIds */
#define PARTS_FLAG_LOS_FADING 0x10      /* MODEL_PARTS.flags: the line-of-sight fade runs (cleared at its end) */
#define PARTS_FLAG_LOS_CLEAR 0x20       /* with _FADING: no line was blocked, fade back in */
typedef struct LOS_FADE {
    int part;                           /* 0x00 */
    float frames;                       /* 0x04: the fade's length (255 / frames a frame) */
    float floor;                        /* 0x08: losAlpha goes down to -floor */
    u_int flags;                        /* 0x0c: ATTEN_EYE_* | ATTEN_SUBJ_* | LOS_FADE_BY_ID */
    u_int hitIds;                       /* 0x10: int[]: the hit records' ids (LOS_HIT_ID) that count */
    u_int cuts;                         /* 0x14 */
    u_int eyeCharas;                    /* 0x18: ATTEN_CHARA[] */
    u_int eyeParts;                     /* 0x1c: int[] */
    u_int subjCharas;                   /* 0x20: ATTEN_CHARA[] */
    u_int subjParts;                    /* 0x24: int[] */
} LOS_FADE;
static_assert(sizeof(LOS_FADE) == 0x28, "LOS_FADE");
void LosHitFadeUpdate(int bank, int a1);                                                 // CT3 0x01857d10
void LosHitFadeUpdateTarget(int bank, int a1, MODEL_PARTS *part, LOS_FADE *at,
                            const float *eye);                                           // CT3 0x01858370
void DistanceAttenuationUpdate(int bank, int a1);                                        // CT3 0x01857590
void DistanceAttenuationTarget(int bank, int a1, MODEL_PARTS *part, DIST_ATTEN *at,
                               const float *eye);                                        // CT3 0x01857af0
void DistanceAttenuationAlphaSet(const float *eye, const float *subj, DIST_ATTEN *at,
                                 MODEL_PARTS *part);                                     // CT3 0x01857c70
void AngleAttenuationUpdate(int bank, int a1);                                           // CT3 0x01856f20
void AngleAttenuationTarget(int bank, int a1, MODEL_PARTS *part, ANGLE_ATTEN *at, const float *eye,
                            const float *dir);                                           // CT3 0x01857320
void AngleAttenuationAlphaSet(const float *eye, const float *dir, const float *subj, ANGLE_ATTEN *at,
                              MODEL_PARTS *part);                                        // CT3 0x018574b0

// ---- slice 8 (2026-11-10): panic setters, the movie break, bank lookup, part and stick helpers ------
void PanicSet(float rise, int source);                                                  // CT3 0x01854110
void PanicStopMovie(const char *file);                                                  // CT3 0x01854190
#define PANIC_SET_ALWAYS_LO PANIC_RISE_CONST /* sources 4 and 5 rise even when PanicCheck is not 0 */
#define PANIC_SET_ALWAYS_HI (PANIC_RISE_CONST + 1)
#define TRIG_EVENT_KEKKAI_USED 0x98     /* registered each time a barrier charm is spent (named from this use) */
#define ITEM_KEKKAI 2                   /* ItemData's row of the barrier charm (byte 1: held) */
int Kekkai_Check(void);                                                                 // CT3 0x01854500
int ssPointCheckWithinScreen(float margin, const float *pos, float (*m)[4]);            // CT3 0x01854b90
int PauseUpdate(int resume);                                                            // CT3 0x01855110
#define PAUSE_RESERVE 0x1               /* pause_reservation: PauseUpdate's bit */
void UpperCase(char *s);                                                                // CT3 0x0185c290
int GetBankID(const char *name);                                                        // CT3 0x01859a50
int PlayMovie(void);                                                                    // CT3 0x01859b00
void PlayMovieReturnLoad(void);                                                         // CT3 0x01859c90
void SystemReReset(void);                                                               // CT3 0x01859dd0
#define TRIG_EVENT_MOVIE_END_62 0x62    /* removed after a movie (named from PlayMovie's use) */
#define TRIG_EVENT_DEMO_SKIP_9E 0x9e    /* removed when a skip ran through a movie */
#define PLAYMOVIE_SKIP_FRAMES 4         /* SkipMain after a movie */
#define PLAYMOVIE_DEMO_SKIP_FLAG 10     /* DEMO_SKIP_FLAG when a skip ran through it */
void asGetObjectPositionAtWorld(float *dst, ALL_TABLE *table, int part);                // CT3 0x01859e90
void asGetAlyssaPosition(float *dst);                                                   // CT3 0x01859eb0
void asGetAnalogStickInformation(int stick, int deadZone, float *degrees, float *magnitude); // CT3 0x01859fc0
#define STICK_PAD(stick) ((stick) >> 1) /* asGetAnalogStickInformation: pad[stick >> 1] */
#define STICK_FIRST 0x1                 /* odd: PAD_DATA.stick[0..1], even: stick[2..3] */
void asSetObjectTrans(ALL_TABLE *table, int part, const float *pos);                    // CT3 0x0185a230
float asGetObjectTransInitialValue(float *dst, ALL_TABLE *table, int part);             // CT3 0x0185a260

// Slice 9 (2026-11-10): the letterbox requests, damage, the RUF view matrices, angles, lengths, random.
void VistaVisionStart(int now, int skippable);                                          // CT3 0x0185ad70
void VistaVisionEnd(int now);                                                           // CT3 0x0185ad30
// VistaVisionStart / End: VistaVisionNumber = now * 2 + VISTA_SHOW_NOW / VISTA_HIDE_NOW, so now = 1
// asks for the slide (VISTA_SHOW / VISTA_HIDE), 0 for the bars at once.
int PlayerDamage(int damage);                                                           // CT3 0x0185a550
void asObjectRotate(float degrees, ALL_TABLE *table, int part, int multiply, int axis); // CT3 0x0185a280
#define ROTATE_AXIS_X 0                 /* asObjectRotate's axis / asRadianByVectors' dropped axis */
#define ROTATE_AXIS_Y 1
#define ROTATE_AXIS_Z 2
#define ROTATE_SET 0                    /* asObjectRotate: the part's rotation becomes the turn */
#define ROTATE_MULTIPLY 1               /* asObjectRotate: the turn applied to the placement matrix */
#define DEGREES_TO_ANGLE16(d) ((int)((d) * 65536.0f / 360.0f) & 0xffff) /* gaSetRotMatrix* units */
void asPosRUF2ViewMatrix(float (*view)[4], const float *pos, const float *right, const float *up,
                         const float *front);                                           // CT3 0x0185a9b0
void asCamRUF2ViewMatrix(float (*view)[4], const float *eye, const float *right, const float *up,
                         const float *front);                                           // CT3 0x0185a7e0
int asRadianByVectors(const float *a, const float *b, int axis);                        // CT3 0x0185dae0
int asRadianByPositions_Rorate(const float *from, const float *to, int angle, int axis); // CT3 0x0185da40
int asISubRotate16(u_int a, u_int b);                                                   // CT3 0x0185de70
float asVectorLength1D(float x2);                                                       // CT3 0x0185df10
float asVectorLength2D(float x, float y);                                               // CT3 0x0185df30
float asVectorDistance2(const float *a, const float *b);                                // CT3 0x0185dfe0
void LowerCase(char *s);                                                                // CT3 0x0185c240
float absf(float x);                                                                    // CT3 0x0185e3e0
int tnRand(int n);                                                                      // CT3 0x0185c320
u_int rnd(void);                                                                        // CT3 0x0185e960
float rndf(void);                                                                       // CT3 0x0185e910
