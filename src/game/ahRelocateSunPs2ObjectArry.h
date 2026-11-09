// game/ahRelocateSunPs2ObjectArry.h -- CT3 game/ahRelocateSunPs2ObjectArry.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

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
