// game/ssGrpPortalCheck.h -- CT3 game/ssGrpPortalCheck.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void ProcessPortal(void);                                                                // CT3 0x018d33d0
void Trigger_main(void);                                                                 // CT3 0x018d4120
void Trigger_deinit(void);                                                               // CT3 0x018d4be0
void TriggerGlobalInit(void);                                                            // CT3 0x018d5230
int ssTrigCheckEvent(int event, int a1, int a2);                                         // CT3 0x018d9100
int ssTrigGetVariable(int var, int a1, int a2);                                          // CT3 0x018d9430
void ssTrigSetVariable(int var, int value, int a2, int a3);                              // CT3 0x018d9470
void ssTrigRemoveEvent(int event, int a1, int a2);                                       // CT3 0x018d9560
void ssTrigRegisterEvent(int event, int a1, int a2);                                     // CT3 0x018d95e0
void InitTriggerDetectorShape(int scene);                                                // CT3 0x018d3d90
void Trigger_init(void);                                                                 // CT3 0x018d4c80
