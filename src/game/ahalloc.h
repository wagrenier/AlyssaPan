// game/ahalloc.h -- CT3 game/ahalloc.c: the declarations other TUs call (game/main.c first, 2026-11-09).
// Prototypes from the call sites; the TU that writes these functions confirms them.
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// A SunAlloc heap (0x34 bytes: gHeapJunc, gHeapSound, gHeapGlobalScene). Its fields are ahalloc.c's.
typedef struct SUN_HEAP {
    u_char data[0x34];
} SUN_HEAP;
static_assert(sizeof(SUN_HEAP) == 0x34, "SUN_HEAP");

void CheckAllocNum0(SUN_HEAP *heap);                                                     // CT3 0x0183d9e0
void SunAllocInit(SUN_HEAP *heap, void *area, int size, const char *name);               // CT3 0x0183e200
