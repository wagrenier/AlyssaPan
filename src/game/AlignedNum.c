// game/AlignedNum.c -- Clock Tower 3: rounding up to a multiple, a busy wait, and loading one IOP module
// (CT3 0x0189f620..0x0189f718). Reconstructed 2026-11-09.
//
// Object extent from the symtab: the three FUNC symbols (AlignedNum 0x28, WaitLoop 0x44, IopLoadModule
// 0x78) and one .rodata literal, @377 (0x01934e90, 1 byte: the empty argument string); the next object's
// .text section starts at 0x0189f718 (a lone `j gaDMAStopMessage_VIF1` nothing reaches, then
// gaDrawObject_Redraw_Polygon at 0x0189f720). The file name is the tool's (the first function's): no
// source string names it.

#include "game/AlignedNum.h"

#include "sdk/sifdev.h"

#include <string.h>

// @377 (.rodata 0x01934e90): the module argument string, empty (sceSifLoadModule is given length 0).
static const char s_noArgs[1] = "";

// CT3 0x0189f620: num rounded up to a multiple of align; 0 answers align (the ROM's branch).
int AlignedNum(int num, int align)
{
    if (num != 0) {
        align = align * ((num + align - 1) / align);
    }
    return align;
}

// CT3 0x0189f650: count down to below zero, doing nothing. The ROM pads the loop body with ten nops;
// `volatile` keeps the host's compiler from dropping the loop. Its loop head is
// CT3 0x0189f680 (WaitLoop_body, game-inferred: the branch target of 0x0189f650, split off by Ghidra).
void WaitLoop(int count)
{
    volatile int n = count;
    while (n-- > 0) {
    }
}

// CT3 0x0189f6a0: load dir + name (or name alone when dir is null) on the IOP, retrying until it
// answers a module id. The 0x70-byte stack buffer is the ROM's.
// callcheck: gaDMAStopMessage_VIF1 (not this function's: the next object's stray j at 0x0189f718)
void IopLoadModule(const char *dir, const char *name)
{
    char path[0x70];

    if (dir == NULL) {
        strcpy(path, name);
    } else {
        strcpy(path, dir);
        strcat(path, name);
    }
    while (sceSifLoadModule(path, 0, s_noArgs) < 0) {
        WaitLoop(IOP_LOAD_RETRY_WAIT);
    }
}
