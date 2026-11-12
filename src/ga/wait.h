// ga/wait.h -- CT3 ga/wait.c: the ga library's debug object (0x018a85c8..0x018a86a8). The release build
// compiled every body out but wait's: the dumps and the stop messages are `jr ra; nop`.
// The stop messages take the caller's return address in a0 (the hand-written syncs `move a0,ra` before
// their tail jump), so a debug build's message could say who waited too long; the release ROM ignores it.
#pragma once

#include "sdk/scetypes.h"

void wait(void); // CT3 0x018a85c8 -- a busy delay (500 x 9,999 empty iterations); nothing calls it

// The dumps: empty in the ROM and called from nowhere (the calls went with the bodies), so their
// arguments are unknown; declared without any.
void gaDump128(void);         // CT3 0x018a8610
void gaDump64(void);          // CT3 0x018a8618
void gaDump32(void);          // CT3 0x018a8620
void gaDumpF4(void);          // CT3 0x018a8628
void gaDumpF3I1(void);        // CT3 0x018a8630
void gaDumpVector(void);      // CT3 0x018a8638
void gaDumpIVector(void);     // CT3 0x018a8640
void gaDumpMatrix(void);      // CT3 0x018a8648
void gaDumpDmaEnv(void);      // CT3 0x018a8650
void gaDumpStatus_VIF0(void); // CT3 0x018a8658
void gaDumpStatus_VIF1(void); // CT3 0x018a8660
void gaDumpStatus_GIF(void);  // CT3 0x018a8668
void gaDumpVu0Regf(void);     // CT3 0x018a8670
void gaDumpVu1Regs(void);     // CT3 0x018a8678
void gaDumpVu1Regi(void);     // CT3 0x018a8680
void gaDumpVu1Regf(void);     // CT3 0x018a8688
void gaDebugDump(void);       // CT3 0x018a86a0

void gaDMAStopMessage_VIF1(void *ra); // CT3 0x018a8690
void gaDMAStopMessage_GIF(void *ra);  // CT3 0x018a8698
