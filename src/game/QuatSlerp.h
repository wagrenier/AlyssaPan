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
int asFindModelPartsID(u_char *header, u_long hash);                                     // CT3 0x0185e150
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
