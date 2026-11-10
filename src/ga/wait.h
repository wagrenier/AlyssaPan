// ga/wait.h -- CT3 ga/wait.c: the declarations other TUs call (ga/gaDmaSync_VIF1.c first, 2026-11-10).
// The stop messages take the caller's return address in a0 (the hand-written syncs `move a0,ra` before
// their tail jump), so the message can say who waited too long; the TU that writes them confirms it.
#pragma once

#include "sdk/scetypes.h"

void gaDMAStopMessage_VIF1(void *ra); // CT3 0x018a8690
void gaDMAStopMessage_GIF(void *ra);  // CT3 0x018a8698
