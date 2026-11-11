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
// From PanicSound's call (0, 1, 10 + 4 * level, 5): the argument names wait for this TU.
void tiVibrationOn(int port, int a1, int a2, int a3);                                     // CT3 0x0188ce10
void tiDrawShadingOff(void);                                                             // CT3 0x0188ced0
// time: seconds (f12; x30 frames), type: Fade_Type (a0, a byte). MemorycardUpdate leaves a0 unset (open-items.md).
#define FADE_TYPE_MC_LOAD 4             /* mc_load_set (name from that use; = FADE_TYPE_OUT2_BLACK) */
// tiXcmd_FadeStart's kinds (2026-11-11): direction by twos, white the odd one.
#define FADE_TYPE_IN_BLACK 0
#define FADE_TYPE_OUT_BLACK 1
#define FADE_TYPE_IN_WHITE 2
#define FADE_TYPE_OUT_WHITE 3
#define FADE_TYPE_OUT2_BLACK 4
#define FADE_TYPE_OUT2_WHITE 5
#define FADE_TYPE_IN2_BLACK 6
#define FADE_TYPE_IN2_WHITE 7
void tiFade_Start(float time, int type);                                                 // CT3 0x0188d3c0
void tiFade_Draw(void);                                                                // CT3 0x0188d160
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
void tiPipeJointInit(int bank);                                                          // CT3 0x0188bf30 (main2.c: chara 3's shape)

// From ssTrigExecuteActCmd's commands (2026-11-11): the arguments the command passes; the last is 1 there.
void tiSet_CharaTranslucenceON(int chara, int level, int flag);                          // CT3 0x0188ded0
void tiSet_CharaTranslucenceOFF(int chara, int flag);                                    // CT3 0x0188ddc0
void tiXFadeOut_Start(float time);                                                       // CT3 0x0188d6e0
// From tiXcmd_XFadeOut2Start (2026-11-11): time in seconds, kind a byte.
void tiXFadeOut_Start2(float time, u_char kind);                                         // CT3 0x0188d5e0

// From tiXcmd_TransItem.c (2026-11-11). dst = src turned about the point pos (pos's translation out,
// src, then back: two gaMulMatrix with matUnit + -pos / + pos rows).
void tiGetTransRotMatrix(float (*dst)[4], float *pos, float (*src)[4]);                  // CT3 0x018949c0
// Sets bit 2 of the pendulums (huriko) of bank `bank` whose id is `id` (0: all); byChara: `bank` is a
// Character[] index and its bank is taken.
void tiHurikoReset_On(int bank, int id, int byChara);                                    // CT3 0x01890ea0
