// game/ahRelocateSunPs2ObjectArry.h -- CT3 game/ahRelocateSunPs2ObjectArry.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "game/ct3_types.h"

void asForceOperateAnimation_Scene(int scene, int mode);                                 // CT3 0x01842400
void asUpDateGAobjectsAnimation(void);                                                   // CT3 0x01842540
void asUpDateSceneAnimation(void);                                                       // CT3 0x018426e0
void asUpdateAnimation_Matrix(void);                                                     // CT3 0x018427f0
void asSeekBarFreeForX(int scene);                                                       // CT3 0x01842fe0
void asShadowPolygonPositions(int bank);                                                 // CT3 0x01844df0
void asGetShadowMatrixGlobalView(void);                                                  // CT3 0x01847330
void asShadowDrawing_Fin(int scene);                                                     // CT3 0x01847bd0
void Shadow_OnceInit(void);                                                              // CT3 0x01847c30
void ExecuteAnimationHistory_init(void);                                                 // CT3 0x0184ef00
void asCopyDoubleBuffer(void);                                                           // CT3 0x01851150
void ExecuteAnimationHistory_Free(void);                                                 // CT3 0x0184eed0
void BuildMatrixTree(int scene, int a1, int a2);                                         // CT3 0x0184fd30
void asDrawShadowVolumeLoop(void);                                                       // CT3 0x018443c0
void asBuildMirrorMatrixTreeBack(void *tree);                                            // CT3 0x0184f120
void asBuildMirrorMatrixTree(void *tree);                                                // CT3 0x0184f220
void asGAobjectControlPolygonDisplayRotateFlagReverse(ALL_TABLE *tbl);                   // CT3 0x0184f330
void asMirrorCulling(int on);                                                            // CT3 0x0184f390
int asMakeMirrorMatrixByPlane(float *planeVec, float *planePoint);                       // CT3 0x0184f910
void asMirrorPlaneCalc(float *planeVec, float *planePoint);                              // CT3 0x0184fb20
int asEnumMirror(ALL_TABLE *tbl);                                                        // CT3 0x0184fb40
void asInitMirrorMatrix(void);                                                           // CT3 0x0184fd00
void asBuildDOFMatrixTree(u_char *partsHeader);                                          // CT3 0x0184f020
void asSeekBarInitializeForX(int scene);                                                 // CT3 0x018430d0
void asSeatArrange_Init(int scene);                                                      // CT3 0x018436a0
void asShadowDrawing_Init(int scene);                                                    // CT3 0x01847b10
void asBehaviorStopAnimation(void *behavior);                                            // CT3 0x01847dc0
void asSceneAutoStartAnimation(void *behavior);                                          // CT3 0x01847fd0
void asForceOperateAnimation_Character(int chara, int mode);                              // CT3 0x018423c0 (main2.c)
