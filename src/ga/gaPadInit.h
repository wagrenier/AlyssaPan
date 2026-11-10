// ga/gaPadInit.h -- CT3 ga/gaPadInit.c: libpad start-up for the ga library's pad reader.
#pragma once

#include "sdk/scetypes.h"

#define GA_PAD_PORT_0 0 /* controller port 1 */
#define GA_PAD_PORT_1 1 /* controller port 2 */

int gaPadInit(void); // CT3 0x018a1130: 1 when both ports opened
