// game/ahalloc.h -- CT3 game/ahalloc.c: SunAlloc, the game's block heaps (reconstructed 2026-11-09).
#pragma once

#include "sdk/scetypes.h"

#include <stddef.h>

// One block of a SunAlloc heap: a header, then the user's bytes. The blocks tile the heap's area in
// address order, linked both ways. layout: host -- on the EE the header is 0x10 bytes (EE offsets in the
// comments); here it is SUN_BLOCK_HEADER (0x20, a multiple of 16 like the EE's, so a size the game
// aligns keeps user data 16-aligned as it was on the EE).
typedef struct SUN_BLOCK {
    int tag;                 /* EE 0x00: the size the user asked for; SUN_BLOCK_FREE = free */
    int size;                /* EE 0x04: the whole block, header included */
    struct SUN_BLOCK *prev;  /* EE 0x08: the block below (NULL: the area's first) */
    struct SUN_BLOCK *next;  /* EE 0x0c: the block above (NULL: the area's last) */
    u_char pad[0x20 - 2 * sizeof(int) - 2 * sizeof(void *)];
} SUN_BLOCK;

#define SUN_BLOCK_FREE 0                          /* SUN_BLOCK.tag of a free block */
#define SUN_BLOCK_HEADER ((int)sizeof(SUN_BLOCK)) /* EE 0x10 */
#define SUN_BLOCK_MIN (SUN_BLOCK_HEADER + 4)      /* EE 0x14: the smallest block a split leaves */
#define SUN_PAD_TAG 4                             /* SetToEmptyBlock: the tag of the pad block it carves */
#define SUN_ALIGN_MIN 4                           /* _SunAlloc: a smaller alignment is raised to this */
#define SUN_DISP_ROW_BYTES 0x19000                /* SunAllocDebugDisp: one map row per 100 KB */
#define SUN_DISP_ROWS_MAX 7

static_assert(sizeof(SUN_BLOCK) == 0x20 && sizeof(SUN_BLOCK) % 16 == 0, "SUN_BLOCK");

// A SunAlloc heap (gHeapJunc, gHeapSound, gHeapGlobalScene: 0x34 bytes in the symtab). layout: host
// (EE offsets in the comments). SunAllocInit, _SunAlloc and _SunFree use only the first 0x14 bytes.
typedef struct SUN_HEAP {
    const char *name;        /* EE 0x00: printed by _SunAlloc's error */
    int size;                /* EE 0x04: the area's bytes */
    SUN_BLOCK *top;          /* EE 0x08: the area's first block */
    int used;                /* EE 0x0c: blocks handed out */
    SUN_BLOCK *empty;        /* EE 0x10: where _SunAlloc starts looking (a hint: free blocks may lie below) */
    u_char pad14[0x20];      /* EE 0x14..0x34: no ahalloc.c function reads it */
} SUN_HEAP;

void CheckAllocNum0(SUN_HEAP *heap);                                                     // CT3 0x0183d9e0
int SunAllocDebugDisp(SUN_HEAP *heap);                                                   // CT3 0x0183d9f0
int SetToEmptyBlock(SUN_BLOCK *prev, SUN_BLOCK **block, int tag, int need, int align);   // CT3 0x0183db70
void *_SunAlloc(SUN_HEAP *heap, int size, int align);                                    // CT3 0x0183ddf0
void _SunFree(SUN_HEAP *heap, void **mem);                                               // CT3 0x0183df50
void *SunAllocUsedBorderCheck(SUN_HEAP *heap);                                           // CT3 0x0183e190
void SunAllocInit(SUN_HEAP *heap, void *area, int size, const char *name);               // CT3 0x0183e200
