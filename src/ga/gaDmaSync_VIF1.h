// ga/gaDmaSync_VIF1.h -- CT3 ga/gaDmaSync_VIF1.c: the declarations other TUs call (ga/gaVifInit.c first,
// 2026-11-09). Prototypes from the call sites' registers (a0 = packet, a1 = qword count); the TU that
// writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

void gaDmaSync_VIF1(void);                  // CT3 0x018a1740
void gaDmaSendN_VIF1(void *addr, int qwc);  // CT3 0x018a1778
