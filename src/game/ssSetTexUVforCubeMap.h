// game/ssSetTexUVforCubeMap.h -- CT3 game/ssSetTexUVforCubeMap.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void ReflectMapUpdate(void);                                                             // CT3 0x01895c10
// set: {chara, bank, 0, 0} (ExecuteWarpObject's call: the character takes the room bank's ambient).
void tnChangeCharAmbientColorSet(short *set);                                            // CT3 0x018962a0
void ssChangeCharAmbientColor(void);                                                     // CT3 0x01895fb0
void ssPartMotionBlurUpdate(void);                                                       // CT3 0x01896d50
void ssPartMotionBlurAllEnd(int scene);                                                  // CT3 0x01896f00
void ssPartMotionBlurGlobalInit(void);                                                   // CT3 0x01897e30
void ssParticleDeInit(int scene);                                                        // CT3 0x0189a920
void ssParticle_main(void);                                                              // CT3 0x0189ae30
void ssBGT_VaselineDrawDiamond(void);                                                    // CT3 0x0189b7e0
void ssBGT_VaselineSetDrawEnvPostRend(void);                                             // CT3 0x0189b840
void ssBGT_WaveDrawDiamond(void);                                                        // CT3 0x0189bff0
void ssBGT_WaveSetDrawEnvPostRend(void);                                                 // CT3 0x0189c070
void ss2DParticle_main(void);                                                            // CT3 0x0189cd60
void ssBGT_HazyDrawSprite(int level, int transTime);                                     // CT3 0x0189d330
void ssBGT_HazySetDrawEnvPostRend(void);                                                 // CT3 0x0189d560
void ssBGT_UltimaDrawSprite(int *frame);                                                 // CT3 0x0189d640
void ssBGT_UltimaSetDrawEnvPostRend(void);                                               // CT3 0x0189d820
void ssTrigScreenBlurEnd(void);                                                          // CT3 0x0189d900
void ssBGT_BlurDrawSprite(int level);                                                    // CT3 0x0189d940
void ssBGT_BlurSetDrawEnvPostRend(void);                                                 // CT3 0x0189d9b0
void ssBGT_DOFDrawSprite(int pass, int level);                                           // CT3 0x0189db60
void ssBGT_DOFSetDrawEnvPostRend(int pass, int level);                                   // CT3 0x0189dc40
void ssBGT_DOFSetDrawEnvPreRend(int pass, int level);                                    // CT3 0x0189dd70
int ssDOF_ClipObjectFromZRange(u_char *obj, u_int mode);                                // CT3 0x0189da90
int ssGetNumBlurDrawObjForScene(int scene, int mode);                                    // CT3 0x01896920
void ssBGT_ZBufferClear(void);                                                           // CT3 0x0189dec0
void ssPartMotionBlurInsertObjToDrawTable(int bank, void *table, u_int *list, int count, int mode); // CT3 0x01896a90
void ssBGT_HQDOF_WriteBackFB(int alpha);                                                 // CT3 0x0189b390
void ssBGT_HQDOF_SetDrawEnvPreRend(void);                                                // CT3 0x0189b510
void ssSetAmbColorToAllObj(int scene);                                                   // CT3 0x01895f00
void ssParticleInit(int scene);                                                          // CT3 0x0189a9d0
