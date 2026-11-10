// game/ahalloc.c -- Clock Tower 3, SunAlloc: first-fit block heaps over fixed areas (CT3 0x0183d9e0..0x0183e274).
// Object extent from the symtab's local run (chunk 67: @373..@378, @439..@443, @500..@502, @542 in .data
// 0x019333f0..0x019335d0) and the source string "ahalloc.c". Reconstructed 2026-11-09.
//
// A heap (SUN_HEAP) owns one area, tiled by blocks (SUN_BLOCK) linked both ways in address order. A block
// is free when its tag is 0, else its tag is the size its user asked for. _SunAlloc walks from the
// heap's hint (empty) and takes the first free block that fits, padding it up to the alignment asked
// (the pad joins the block below, or becomes a small free block of its own) and splitting off the rest;
// _SunFree merges a freed block with free neighbours. The host header is 0x20 bytes, the EE's 0x10
// (ahalloc.h): the area holds a few bytes less per block than on the PS2.

#include "game/ahalloc.h"

#include "game/AlignedNum.h"
#include "game/fileio.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AHALLOC_FILE "ahalloc.c" /* data 0x019335e0 */

// _SunFree: what lies around the freed block.
#define SUN_FREE_PREV 0x1      /* a block below */
#define SUN_FREE_PREV_USED 0x2 /* ... and it is in use */
#define SUN_FREE_NEXT 0x4      /* a block above */
#define SUN_FREE_NEXT_USED 0x8 /* ... and it is in use */

// The address the ROM's modulo works on.
static inline uintptr_t SunAddr(const void *p)
{
    return (uintptr_t)p;
}

// CT3 0x0183d9e0: empty in the ROM (the check was compiled out).
void CheckAllocNum0(SUN_HEAP *heap)
{
    (void)heap;
}

// CT3 0x0183d9f0: prints the heap's map, one entry per block (rows scaled by size), and the used count.
int SunAllocDebugDisp(SUN_HEAP *heap)
{
    for (SUN_BLOCK *blk = heap->top; blk != NULL; blk = blk->next) {
        printf("         +------------------------------------------------\n"); /* @373 */
        if (blk->tag == SUN_BLOCK_FREE)
            printf("%08X |\n", (u_int)SunAddr(blk)); /* @375 */
        else
            printf("%08X | USER SIZE:%7d\n", (u_int)SunAddr(blk), blk->tag); /* @374 */

        int rows = blk->size / SUN_DISP_ROW_BYTES;
        if (rows < 1)
            rows = 1;
        for (int row = 0; row < rows && row < SUN_DISP_ROWS_MAX; row++) {
            if (row == 0)
                printf("%08X |      SIZE:%7d (%6.1fKB)  %07X<-PV NX->%07X\n", /* @376 */
                       (u_int)SunAddr(blk + 1), blk->size - SUN_BLOCK_HEADER, (double)((float)blk->size / 1024.0f),
                       (u_int)SunAddr(blk->prev), (u_int)SunAddr(blk->next));
            else
                printf("         |\n"); /* @377 */
        }
    }
    return printf("         +----- USED %04d BLOCKS --------------------------\n\n", heap->used); /* @378 */
}

// CT3 0x0183db70: makes *block (free, at least `need` bytes after alignment) a block of exactly `need`
// bytes tagged `tag`, its user bytes aligned to `align`. A misaligned block moves up: with a block below
// (prev), the pad joins it (returns 1); without one, the block's head becomes a small pad block, left free
// (returns 2). *block is the block taken. Returns 0 when it was aligned already, -1 when it does not fit.
int SetToEmptyBlock(SUN_BLOCK *prev, SUN_BLOCK **block, int tag, int need, int align)
{
    int result = 0;
    SUN_BLOCK *blk = *block;

    if (SunAddr(blk + 1) % (uintptr_t)align == 0) {
        if (blk->size < need)
            return -1;
    } else if (prev == NULL) {
        // A pad block of SUN_BLOCK_MIN bytes, then this block, moved up to the alignment.
        const u_char *user = (const u_char *)blk + SUN_BLOCK_MIN + SUN_BLOCK_HEADER;
        int pad = align - (int)(SunAddr(user) % (uintptr_t)align);
        if (blk->size < SUN_BLOCK_MIN + need + pad)
            return -1;
        if (SetToEmptyBlock(NULL, &blk, SUN_PAD_TAG, SUN_BLOCK_MIN, SUN_ALIGN_MIN) == -1)
            return -1;
        SUN_BLOCK *padBlock = blk;
        blk = blk->next;
        if (SetToEmptyBlock(padBlock, &blk, tag, need, align) == -1)
            return -1;
        padBlock->tag = SUN_BLOCK_FREE;
        result = 2;
        *block = blk;
    } else {
        // Move the block up by the pad; the block below grows by as much.
        int pad = align - (int)(SunAddr(blk + 1) % (uintptr_t)align);
        if (blk->size - pad < need)
            return -1;
        prev->size += pad;
        prev->next = (SUN_BLOCK *)((u_char *)prev->next + pad);
        int oldTag = blk->tag;
        int oldSize = blk->size;
        SUN_BLOCK *oldNext = blk->next;
        blk = prev->next;
        blk->tag = oldTag;
        blk->size = oldSize - pad;
        blk->next = oldNext;
        blk->prev = prev;
        if (oldNext != NULL)
            oldNext->prev = blk;
        result = 1;
        *block = blk;
    }

    // Take the block whole when the rest would be too small to stand alone, else split the rest off.
    if (need == blk->size || blk->size - need < SUN_BLOCK_MIN) {
        blk->tag = tag;
    } else {
        SUN_BLOCK *rest = (SUN_BLOCK *)((u_char *)blk + need);
        rest->tag = SUN_BLOCK_FREE;
        rest->size = blk->size - need;
        rest->next = blk->next;
        rest->prev = blk;
        if (rest->next != NULL)
            rest->next->prev = rest;
        blk->tag = tag;
        blk->size = need;
        blk->next = rest;
    }
    return result;
}

// CT3 0x0183ddf0: `size` bytes from the heap, aligned to `align` (at least 4); NULL (after the error
// and the heap's map) when no block fits.
void *_SunAlloc(SUN_HEAP *heap, int size, int align)
{
    if (size < 1) {
        fs_error(AHALLOC_FILE, 373, "SunAlloc", "alloc sizeに0がしていされた"); /* data 0x019335f0, 0x01933600 */
        return NULL;
    }
    if (align < SUN_ALIGN_MIN)
        align = SUN_ALIGN_MIN;
    int need = AlignedNum(size, align) + SUN_BLOCK_HEADER;

    // prev starts NULL even when the hint is not the area's first block (the ROM's: a misaligned first
    // candidate then gets a pad block of its own).
    int firstFree = 1;
    SUN_BLOCK *prev = NULL;
    for (SUN_BLOCK *blk = heap->empty; blk != NULL; blk = blk->next) {
        if (blk->tag == SUN_BLOCK_FREE) {
            if (SetToEmptyBlock(prev, &blk, size, need, align) >= 0) {
                if (firstFree)
                    heap->empty = blk->next;
                heap->used++;
                return blk + 1;
            }
            firstFree = 0;
        }
        prev = blk;
    }
    fs_error(AHALLOC_FILE, 440, "sunAlloc error", "%s size=%d", heap->name, need); /* data 0x01933620, 0x01933630 */
    SunAllocDebugDisp(heap);
    return NULL;
}

// CT3 0x0183df50: frees *mem's block, merges it with free neighbours, moves the hint down to the merged
// block, and clears *mem. NULL or a NULL *mem does nothing.
void _SunFree(SUN_HEAP *heap, void **mem)
{
    // The ROM reads *mem before testing mem; the host tests first.
    if (mem == NULL || *mem == NULL)
        return;
    SUN_BLOCK *blk = (SUN_BLOCK *)*mem - 1;
    SUN_BLOCK *prev = blk->prev;
    SUN_BLOCK *next = blk->next;

    u_int around = 0;
    if (prev != NULL) {
        around = SUN_FREE_PREV;
        if (prev->tag != SUN_BLOCK_FREE)
            around = SUN_FREE_PREV | SUN_FREE_PREV_USED;
    }
    if (next != NULL) {
        around |= SUN_FREE_NEXT;
        if (next->tag != SUN_BLOCK_FREE)
            around |= SUN_FREE_NEXT_USED;
    }
    blk->tag = SUN_BLOCK_FREE;

    switch (around) {
    case 0: // the area's only block
        heap->empty = blk;
        break;
    case SUN_FREE_PREV: // the last block, a free one below: join it
        prev->size += blk->size;
        prev->next = NULL;
        if (blk <= heap->empty)
            heap->empty = prev;
        break;
    case SUN_FREE_PREV | SUN_FREE_PREV_USED: // the last block, a used one below
        if (blk <= heap->empty)
            heap->empty = blk;
        break;
    case SUN_FREE_NEXT:                                      // the first block, a free one above: take it in
    case SUN_FREE_PREV | SUN_FREE_PREV_USED | SUN_FREE_NEXT: // a used block below, a free one above
        blk->size += next->size;
        blk->next = next->next;
        if (next->next != NULL)
            next->next->prev = blk;
        if (blk <= heap->empty)
            heap->empty = blk;
        break;
    case SUN_FREE_PREV | SUN_FREE_NEXT: // free on both sides: all three become the block below
        prev->size += blk->size + next->size;
        prev->next = next->next;
        if (next->next != NULL)
            next->next->prev = prev;
        if (blk <= heap->empty)
            heap->empty = prev;
        break;
    case SUN_FREE_NEXT | SUN_FREE_NEXT_USED: // the first block, a used one above: the ROM leaves the hint
        break;
    case SUN_FREE_PREV | SUN_FREE_NEXT | SUN_FREE_NEXT_USED: // a free block below, a used one above
        prev->size += blk->size;
        prev->next = next;
        next->prev = prev;
        if (blk <= heap->empty)
            heap->empty = prev;
        break;
    case SUN_FREE_PREV | SUN_FREE_PREV_USED | SUN_FREE_NEXT | SUN_FREE_NEXT_USED: // used on both sides
        if (blk <= heap->empty)
            heap->empty = blk;
        break;
    default: // unreachable: the used bits come only with their block bits
        fs_error(AHALLOC_FILE, 333, "SunFree", "allocこうぞうが,こわれている"); /* data 0x01933640, 0x01933650 */
        break;
    }
    heap->used--;
    *mem = NULL;
}

// CT3 0x0183e190: where the heap's used part ends -- the last block's user bytes when it is free, else
// the area's end. NULL (and an error line) for a heap with no blocks.
void *SunAllocUsedBorderCheck(SUN_HEAP *heap)
{
    SUN_BLOCK *blk = heap->top;
    if (blk == NULL) {
        printf("ERR: SunAllocUsedBorderCheck!!\n"); /* @542, data 0x019335b0 */
        return NULL;
    }
    while (blk->next != NULL)
        blk = blk->next;
    if (blk->tag != SUN_BLOCK_FREE)
        return (u_char *)blk + blk->size;
    return blk + 1;
}

// CT3 0x0183e200: clears the area and makes it one free block.
void SunAllocInit(SUN_HEAP *heap, void *area, int size, const char *name)
{
    memset(area, 0, (size_t)size);
    SUN_BLOCK *blk = (SUN_BLOCK *)area;
    heap->name = name;
    heap->size = size;
    heap->top = blk;
    heap->used = 0;
    heap->empty = blk;
    blk->tag = SUN_BLOCK_FREE;
    blk->size = size;
    blk->prev = NULL;
    blk->next = NULL;
}
