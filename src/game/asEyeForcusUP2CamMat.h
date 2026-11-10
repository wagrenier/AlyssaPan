// game/asEyeForcusUP2CamMat.h -- CT3 game/asEyeForcusUP2CamMat.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"
#include "ga/pad_button_read.h"

void asChangeViewMatrix_forTheBank(int bank);                                            // CT3 0x01886690
short CamGetNowDOFLevel(void);                                                           // CT3 0x01886770
void camera_destroy(int scene, int mode);                                                // CT3 0x018875a0
void camera_view(void);                                                                  // CT3 0x018875d0
void camera_main(PAD_DATA *pad, int mode);                                               // CT3 0x018881a0
void camera_init(void);                                                                  // CT3 0x01889c50
int asEyeForcusUP2CamMat(float proj, float (*view)[4], float *eye, float *focus, float *up); // CT3 0x01883db0
