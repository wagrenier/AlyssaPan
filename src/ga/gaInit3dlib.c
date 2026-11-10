// ga/gaInit3dlib.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "ga/gaInit3dlib.h"

#include <string.h>

// CT3 0x018a8210
// Written early (game/ahRelocateSunPs2ObjectArry.c slice 13): the 32 words of a ga draw object copied
// (the ROM loads all 32, then stores them).
void gaCopyObject2(void *dst, const void *src)
{
    memmove(dst, src, GA_OBJECT_SIZE);
}

// CT3 0x018a7780
// Written early (the same slice): VMUL.xyz, then x + y + z (VADDY.x, VADDZ.x).
float gaInnerProductXYZ(float *a, float *b)
{
    float s = a[0] * b[0];
    s += a[1] * b[1];
    return s + a[2] * b[2];
}

// ---- stubs (tools/reverse/stubgen.py) ----

// stub of CT3 0x018a7700
void gaInit3dlib(void)
{
    ALYSSAPAN_STUB("gaInit3dlib", 0x018a7700);
}

// stub of CT3 0x018a8548
void gaSetZRollMatrix(int roll)
{
    ALYSSAPAN_STUB("gaSetZRollMatrix", 0x018a8548);
}

// stub of CT3 0x018a7db8
void gaSet3dlibStruct(float clipZ, void *work, void *drawbuffer, void *fogctrl, void *screen)
{
    ALYSSAPAN_STUB("gaSet3dlibStruct", 0x018a7db8);
}

// stub of CT3 0x018a7900
void gaMulMatrix(float (*dst)[4], float (*a)[4], float (*b)[4])
{
    ALYSSAPAN_STUB("gaMulMatrix", 0x018a7900);
}

// stub of CT3 0x018a8258
void gaSetLight(void *lightWork, float (*view)[4])
{
    ALYSSAPAN_STUB("gaSetLight", 0x018a8258);
}


// stub of CT3 0x018a7b50
void gaInversMatrix(float (*dst)[4], float (*a)[4])
{
    ALYSSAPAN_STUB("gaInversMatrix", 0x018a7b50);
}
