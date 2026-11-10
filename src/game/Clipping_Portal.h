// game/Clipping_Portal.h -- CT3 game/Clipping_Portal.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void tiLightBelt2UpdateForALYSSA_Battle(void);                                           // CT3 0x01889f70
void tiLightBelt2UpdateForBOSS_Battle(void);                                             // CT3 0x0188a1f0
void tiLightBelt2Update(void);                                                           // CT3 0x0188a610
void tiCharaFootAdjust(void);                                                            // CT3 0x0188b790
void tiPipeJointUpdate(void);                                                            // CT3 0x0188bb60
void tiWindEnd(int scene);                                                               // CT3 0x0188cb10
void tiWindUpdate(void);                                                                 // CT3 0x0188cbe0
void tiVibrationTimer(void);                                                             // CT3 0x0188cd70
void tiVibrationOff(int mode);                                                           // CT3 0x0188cdc0
void tiDrawShadingOff(void);                                                             // CT3 0x0188ced0
void tiFade_Draw(void);                                                                  // CT3 0x0188d160
void tiXFadeOut_Draw(void);                                                              // CT3 0x0188d3f0
void tiDrawLensFlares(void);                                                             // CT3 0x0188df70
void tiParticleUpdate(void);                                                             // CT3 0x01890070
void tiHurikoUpdate(void);                                                               // CT3 0x01892ee0
void tiHurikoHitUpdate(void);                                                            // CT3 0x018931f0
void tiBumpSTValueUpdate(void);                                                          // CT3 0x01893850
void tiRefractionMapUpdate(void);                                                        // CT3 0x01894230
void tiFreeBossData(void);                                                               // CT3 0x01889df0
void tiWindEnd_All(void);                                                                // CT3 0x0188cb60
void ExportDrawToTex(void);                                                              // CT3 0x01893cf0
void Clipping_Portal(void *work, void *screen);                                         // CT3 0x01889cd0
void tiChara_Translucence_End2(int fbuf, int chara);                                     // CT3 0x0188d720
void tiChara_Translucence_Start2(void);                                                  // CT3 0x0188dbd0
void tiChara_Translucence_End(float alpha, int fbuf);                                    // CT3 0x0188dca0
void tiChara_Translucence_Start(void);                                                   // CT3 0x0188dd40
void tiLightBelt2Init(int scene);                                                        // CT3 0x0188b050
void tiWindInit(int scene);                                                              // CT3 0x0188ccb0
void tiLensFlaresInit(int scene);                                                        // CT3 0x0188e4a0
void tiParticleInit(int scene);                                                          // CT3 0x01890350
void tiHurikoInit(int scene);                                                            // CT3 0x018933f0
