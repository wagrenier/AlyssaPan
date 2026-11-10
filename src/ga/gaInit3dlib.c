// ga/gaInit3dlib.c -- Clock Tower 3 module (file from the inference in alyssapan_functions.txt).
// Functions below are stubs until reconstructed; see tools/reverse/stage.py.

#include "common/alyssapan_stub.h"
#include "ga/gaInit3dlib.h"

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

// stub of CT3 0x018a7780
float gaInnerProductXYZ(float *a, float *b)
{
    ALYSSAPAN_STUB("gaInnerProductXYZ", 0x018a7780);
    return {};
}
