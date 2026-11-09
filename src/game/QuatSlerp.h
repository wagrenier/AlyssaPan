// game/QuatSlerp.h -- CT3 game/QuatSlerp.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

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
