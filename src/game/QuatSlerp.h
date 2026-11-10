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
void asContInterpolateZipNormal_XYZ(short *out, const short *a, const short *b, const float *w, int n); // CT3 0x0185c840
void asContInterpolateZipTrans_XYZ(short *out, const short *a, const short *b, const float *w, int n,
                                   u_int shift);                                         // CT3 0x0185ca50
void asContInterpolateVector(float *out, const float *a, const float *b, const float *w, int n); // CT3 0x0185d2a0
int asFindModelPartsID(u_char *header, u_long hash);                                    // CT3 0x0185e150
int asCheckBankID(int bank);                                                             // CT3 0x0185e360
int asCheckBankType(int bank);                                                           // CT3 0x0185e380
u_long ahGetHash(const char *name);                                                      // CT3 0x0185e580
void CameraMatrix2CameraTrg3m(float *trg);                                              // CT3 0x0185a5e0
void CameraMatrix2CameraPos(float *pos);                                                 // CT3 0x0185a650
void tnInit3dlibWork(void *work, void *spr);                                            // CT3 0x01854560
int OneMaxObjectDrawZOnly(int bank, int index);                                          // CT3 0x01858f20
void sunBGT_LoadGSTexClutData_VIF1(void *tbuffer);                                      // CT3 0x018590b0
void sunBGT_ExecTBufferData_VIF1(void *tbuffer);                                        // CT3 0x01859520
void sunSetupTBufferData(void *tbuffer, TEX_INFO tex);                                   // CT3 0x01859530 (tex by value, 0x2c bytes)
void TexFileSetInit(int scene);                                                          // CT3 0x01856070
void AmbientAlphaInit(int scene);                                                        // CT3 0x01856e70
void asViewMatrix2PosRUF(float *view, float *pos, float *right, float *up, float *front); // CT3 0x0185a8a0
void asViewMatrix2CamRUF(float (*view)[4], float *pos, float *right, float *up, float *forward); // CT3 0x0185a6f0
void EventSoundEnd(void);                                                                // CT3 0x018532a0
// From data_save_set / data_load_set (Character_Init.c, 2026-11-10).
int GetAreaNumber(int a0);                                                               // CT3 0x01852240
int BankID_TO_CharID(short bank);                                                        // CT3 0x0185a400
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
float ahCos(int angle);                                                                  // CT3 0x01851d60
int ahAcos(float c);                                                                     // CT3 0x01851dc0
// m = the rotation by angle about the unit axis (asSearchObject's turn step).
void asMakeRotMatrixByQuartanion(float (*m)[4], float *axis, int angle);                // CT3 0x0185ddf0
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
