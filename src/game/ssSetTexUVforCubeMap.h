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

// The screen effects the trigger commands start and end (ssTrigExecuteActCmd, 2026-11-11): the arguments in
// the order the command reads them; the ROM call sites give the counts (Wave: 12, the last four on the stack).
void ssTrigParticleStart(int particle, int bank, int arg);                               // CT3 0x018988b0
void ssTrigParticleStop(int particle, int bank);                                         // CT3 0x018988a0
void ssTrigParticleAllOffImmediately(int particle, int bank);                            // CT3 0x01898710
void ssPartMotionBlurStart(int part, int a, int bank, int b, int c);                     // CT3 0x018976d0
void ssPartMotionBlurEnd(int part, int bank);                                            // CT3 0x01897140
void ssTrigScreenBlurStart(int level);                                                   // CT3 0x0189d920
void ssTrigHazyScreenStart(int level, int time);                                         // CT3 0x0189d310
void ssTrigHazyScreenEnd(void);                                                          // CT3 0x0189d2f0
void ssTrigWaveStart(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int bank,
                     int k);                                                             // CT3 0x0189ce60
void ssTrigWaveEnd(void);                                                                // CT3 0x0189ce30
void ssTrigVaselineStart(int a, int b, int c, int d, int e, int f, int g, int h, int i); // CT3 0x0189bd40
void ssTrigVaselineEnd(void);                                                            // CT3 0x0189b920
void ssTrigHQDOFStart(int a, int b);                                                     // CT3 0x0189b5a0
void ssTrigHQDOFEnd(void);                                                               // CT3 0x0189b580

// The particle curve readers (from tiCalc_Orbit, 2026-11-11; argument order from the disassembly). age: the
// record's PARTICLE2.age; curve: the entry's curve (host pointer: the ROM passes the bank's EE word);
// cache: the record's cache word for that curve. ssParticleCashClear runs before each pass over a ring.
void ssParticleCashClear(void);                                                          // CT3 0x01899070
void tiParticleGetSizeValue(int age, float *scale, void *curve, short *cache);           // CT3 0x01898a60
void tiParticleGetRefractive_IndexValue(int age, float *index, void *curve, short *cache); // CT3 0x01898980
// r, g, b: signed bytes (the ROM reads them back with lb).
void ssParticleGetVRGBValue(int age, s_char *r, s_char *g, s_char *b, void *curve, short *cache); // CT3 0x01898bc0
// alpha: percent (lh; tiCalc_Orbit divides by 100).
void ssParticleGetAlphaValue(int age, short *alpha, void *curve, short *cache);          // CT3 0x01898e60
// u, v: the draw object's +0x78 / +0x7c (DRAW_OBJ_UV_U / _V).
void ssParticleGetUVValue(int age, void *u, void *v, void *curve, short *cache);         // CT3 0x01898d20
