// game/LinkMatrixTree.h -- CT3 game/LinkMatrixTree.c: the matrix tree set-up and the game clock (written
// 2026-11-10).
#pragma once

#include "sdk/scetypes.h"

#define TICKS_PER_VSYNC 5               /* the clocks' ticks per VSync (300 a second) */

// The game clock (ahTimeMaintenance): the scene clock stops while paused, the global one never.
extern int gNextTicksInterval;                  /* sbss 0x0193cb18: this frame's scene ticks */
extern int gNextTicks;                          /* sbss 0x0193cb1c */
extern int gTotalTicks;                         /* sbss 0x0193cb20 */
extern int gGlobalNextTicksInterval;            /* sbss 0x0193cb08: this frame's global ticks */
extern int gGlobalNextTicks;                    /* sbss 0x0193cb0c */
extern int gGlobalTotalTicks;                   /* sbss 0x0193cb10 */

void ahTimeMaintenance(int paused);                                                      // CT3 0x018517d0
void ahTimeInit(void);                                                                   // CT3 0x01851880
// objList: the bank's three draw lists (ALL_TABLE.objOpaque on), each of DRAW_OBJ_ENTRY EE words ended
// by 0; partsHeader: pModelPartsHeader[bank].
void LinkMatrixTree(u_int *objList, u_char *partsHeader);                                // CT3 0x018514d0
void InitMatrixTree(u_char *partsHeader);                                                // CT3 0x018516a0
