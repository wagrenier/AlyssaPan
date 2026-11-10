// ga/gaInit3dlib.h -- CT3 ga/gaInit3dlib.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void gaInit3dlib(void);                                                                  // CT3 0x018a7700
void gaSetZRollMatrix(int roll);                                                         // CT3 0x018a8548
// clipZ: main_rtn passes 50000.0f ($f12); screen: screen_0's twelve floats.
void gaSet3dlibStruct(float clipZ, void *work, void *drawbuffer, void *fogctrl, void *screen); // CT3 0x018a7db8
// dst = a * b (Character_Init.c's CharLightSet: mrr_ReverseGlobalView = matrix_global_view * mrr_ReverseMatrix).
void gaMulMatrix(float (*dst)[4], float (*a)[4], float (*b)[4]);                        // CT3 0x018a7900
// a.xyz . b.xyz (asSearchObject's calls: a0, a1 -> f0).
float gaInnerProductXYZ(float *a, float *b);                                            // CT3 0x018a7780
// Loads a part's lights (GA_LIGHT_WORK, ct3_types.h) for drawing under the view matrix view.
void gaSetLight(void *lightWork, float (*view)[4]);                                      // CT3 0x018a8258
// dst = a's inverse (asMakeMirrorMatrixByPlane calls it in place).
void gaInversMatrix(float (*dst)[4], float (*a)[4]);                                     // CT3 0x018a7b50
